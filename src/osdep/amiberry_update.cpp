#include "sysconfig.h"
#include "sysdeps.h"
#include "amiberry_update.h"
#include "options.h"
#include "threaddep/thread.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <regex>
#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <spawn.h>
#include <sys/wait.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/xattr.h>
#include <crt_externs.h>
#endif

namespace {

static constexpr const char* k_github_api_base = "https://api.github.com/repos/BlitterStudio/amiberry/releases";
static constexpr long k_http_timeout_sec = 30;

struct SemVer {
	int major = 0;
	int minor = 0;
	int patch = 0;
	bool has_pre = false;
	int pre_number = 0;
	bool valid = false;

	static SemVer parse(const std::string& version_str)
	{
		SemVer out;
		std::string s = version_str;

		auto trim = [](std::string& v) {
			while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) {
				v.erase(v.begin());
			}
			while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) {
				v.pop_back();
			}
		};

		trim(s);
		if (s.rfind("preview-v", 0) == 0) {
			s = s.substr(std::strlen("preview-v"));
		} else if (!s.empty() && s[0] == 'v') {
			s = s.substr(1);
		}

		static const std::regex re(R"(^([0-9]+)\.([0-9]+)\.([0-9]+)(?:-pre\.([0-9]+))?$)");
		std::smatch m;
		if (!std::regex_match(s, m, re)) {
			return out;
		}

		out.major = std::atoi(m[1].str().c_str());
		out.minor = std::atoi(m[2].str().c_str());
		out.patch = std::atoi(m[3].str().c_str());
		if (m[4].matched) {
			out.has_pre = true;
			out.pre_number = std::atoi(m[4].str().c_str());
		}
		out.valid = true;
		return out;
	}

	int compare(const SemVer& other) const
	{
		if (!valid || !other.valid) {
			return 0;
		}
		if (major != other.major) {
			return major < other.major ? -1 : 1;
		}
		if (minor != other.minor) {
			return minor < other.minor ? -1 : 1;
		}
		if (patch != other.patch) {
			return patch < other.patch ? -1 : 1;
		}
		if (has_pre != other.has_pre) {
			return has_pre ? -1 : 1;
		}
		if (!has_pre) {
			return 0;
		}
		if (pre_number != other.pre_number) {
			return pre_number < other.pre_number ? -1 : 1;
		}
		return 0;
	}
};

struct Sha256Ctx {
	std::array<uint32_t, 8> state{};
	std::array<uint8_t, 64> buffer{};
	uint64_t bitlen = 0;
	size_t buffer_len = 0;
};

static inline uint32_t rotr(uint32_t v, uint32_t n)
{
	return (v >> n) | (v << (32 - n));
}

static void sha256_transform(Sha256Ctx& ctx, const uint8_t* data)
{
	static constexpr std::array<uint32_t, 64> k = {
		0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
		0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
		0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
		0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
		0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
		0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
		0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
		0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
	};

	uint32_t w[64] = {};
	for (int i = 0; i < 16; ++i) {
		w[i] = (static_cast<uint32_t>(data[i * 4]) << 24)
			| (static_cast<uint32_t>(data[i * 4 + 1]) << 16)
			| (static_cast<uint32_t>(data[i * 4 + 2]) << 8)
			| static_cast<uint32_t>(data[i * 4 + 3]);
	}
	for (int i = 16; i < 64; ++i) {
		const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
		const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint32_t a = ctx.state[0];
	uint32_t b = ctx.state[1];
	uint32_t c = ctx.state[2];
	uint32_t d = ctx.state[3];
	uint32_t e = ctx.state[4];
	uint32_t f = ctx.state[5];
	uint32_t g = ctx.state[6];
	uint32_t h = ctx.state[7];

	for (int i = 0; i < 64; ++i) {
		const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
		const uint32_t ch = (e & f) ^ ((~e) & g);
		const uint32_t temp1 = h + s1 + ch + k[i] + w[i];
		const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
		const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		const uint32_t temp2 = s0 + maj;

		h = g;
		g = f;
		f = e;
		e = d + temp1;
		d = c;
		c = b;
		b = a;
		a = temp1 + temp2;
	}

	ctx.state[0] += a;
	ctx.state[1] += b;
	ctx.state[2] += c;
	ctx.state[3] += d;
	ctx.state[4] += e;
	ctx.state[5] += f;
	ctx.state[6] += g;
	ctx.state[7] += h;
}

static void sha256_init(Sha256Ctx& ctx)
{
	ctx.state = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };
	ctx.buffer.fill(0);
	ctx.bitlen = 0;
	ctx.buffer_len = 0;
}

static void sha256_update(Sha256Ctx& ctx, const uint8_t* data, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		ctx.buffer[ctx.buffer_len++] = data[i];
		if (ctx.buffer_len == 64) {
			sha256_transform(ctx, ctx.buffer.data());
			ctx.bitlen += 512;
			ctx.buffer_len = 0;
		}
	}
}

static std::array<uint8_t, 32> sha256_final(Sha256Ctx& ctx)
{
	std::array<uint8_t, 32> out{};

	size_t i = ctx.buffer_len;
	if (ctx.buffer_len < 56) {
		ctx.buffer[i++] = 0x80;
		while (i < 56) {
			ctx.buffer[i++] = 0;
		}
	} else {
		ctx.buffer[i++] = 0x80;
		while (i < 64) {
			ctx.buffer[i++] = 0;
		}
		sha256_transform(ctx, ctx.buffer.data());
		ctx.buffer.fill(0);
	}

	ctx.bitlen += static_cast<uint64_t>(ctx.buffer_len) * 8ULL;
	ctx.buffer[63] = static_cast<uint8_t>(ctx.bitlen);
	ctx.buffer[62] = static_cast<uint8_t>(ctx.bitlen >> 8);
	ctx.buffer[61] = static_cast<uint8_t>(ctx.bitlen >> 16);
	ctx.buffer[60] = static_cast<uint8_t>(ctx.bitlen >> 24);
	ctx.buffer[59] = static_cast<uint8_t>(ctx.bitlen >> 32);
	ctx.buffer[58] = static_cast<uint8_t>(ctx.bitlen >> 40);
	ctx.buffer[57] = static_cast<uint8_t>(ctx.bitlen >> 48);
	ctx.buffer[56] = static_cast<uint8_t>(ctx.bitlen >> 56);
	sha256_transform(ctx, ctx.buffer.data());

	for (size_t j = 0; j < 8; ++j) {
		out[j * 4] = static_cast<uint8_t>(ctx.state[j] >> 24);
		out[j * 4 + 1] = static_cast<uint8_t>(ctx.state[j] >> 16);
		out[j * 4 + 2] = static_cast<uint8_t>(ctx.state[j] >> 8);
		out[j * 4 + 3] = static_cast<uint8_t>(ctx.state[j]);
	}
	return out;
}

struct HttpContext {
	std::string buffer;
};

struct DownloadContext {
	std::ofstream* out = nullptr;
	DownloadProgressCallback progress_cb;
	std::atomic<bool>* cancel_flag = nullptr;
};

static std::thread s_update_thread;
static std::mutex s_update_mutex;
static std::atomic<bool> s_update_running{false};
static std::atomic<bool> s_update_cancel{false};
static UpdateInfo s_update_result;

#ifdef _WIN32
static std::string s_windows_restart_script;
#endif
#ifdef __APPLE__
static std::string s_macos_new_executable;
#endif

static void ensure_curl_initialized()
{
	static std::once_flag s_once;
	std::call_once(s_once, []() {
		curl_global_init(CURL_GLOBAL_DEFAULT);
	});
}

static bool starts_with(const std::string& v, const std::string& prefix)
{
	return v.rfind(prefix, 0) == 0;
}

static std::string to_lower(std::string v)
{
	std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return v;
}

static std::string get_executable_path()
{
#ifdef _WIN32
	std::array<wchar_t, MAX_DPATH> pathw{};
	DWORD len = GetModuleFileNameW(nullptr, pathw.data(), static_cast<DWORD>(pathw.size()));
	if (len == 0 || len >= pathw.size()) {
		return {};
	}
	int needed = WideCharToMultiByte(CP_UTF8, 0, pathw.data(), static_cast<int>(len), nullptr, 0, nullptr, nullptr);
	if (needed <= 0) {
		return {};
	}
	std::string out;
	out.resize(static_cast<size_t>(needed));
	WideCharToMultiByte(CP_UTF8, 0, pathw.data(), static_cast<int>(len), out.data(), needed, nullptr, nullptr);
	return out;
#elif defined(__APPLE__)
	uint32_t sz = 0;
	_NSGetExecutablePath(nullptr, &sz);
	if (sz == 0) {
		return {};
	}
	std::string out;
	out.resize(sz);
	if (_NSGetExecutablePath(out.data(), &sz) != 0) {
		return {};
	}
	out.resize(std::strlen(out.c_str()));
	return out;
#else
	std::array<char, PATH_MAX> path{};
	ssize_t len = readlink("/proc/self/exe", path.data(), path.size() - 1);
	if (len <= 0) {
		return {};
	}
	path[static_cast<size_t>(len)] = '\0';
	return std::string(path.data());
#endif
}

static std::string get_platform_asset_suffix()
{
#if defined(_WIN32)
	return "windows-x64";
#elif defined(__APPLE__)
	return "macOS-universal";
#else
	return "";
#endif
}

static bool matches_platform_asset(const std::string& name)
{
#if defined(__linux__)
	// Linux release assets use various naming conventions per distro/arch
	// (e.g. "debian-bookworm-amd64", "fedora-x86_64", "ubuntu-24.04-arm64").
	// Exclude assets for other platforms that share architecture substrings.
	const std::string name_lower = to_lower(name);
	if (name_lower.find("macos") != std::string::npos || name_lower.find("windows") != std::string::npos)
		return false;
	#if defined(__aarch64__) || defined(CPU_AARCH64)
	return name_lower.find("arm64") != std::string::npos || name_lower.find("aarch64") != std::string::npos;
	#elif defined(__arm__) || defined(CPU_arm)
	return name_lower.find("armhf") != std::string::npos;
	#else
	return name_lower.find("amd64") != std::string::npos || name_lower.find("x86_64") != std::string::npos;
	#endif
#else
	const std::string suffix = get_platform_asset_suffix();
	if (suffix.empty()) return true;
	return name.find(suffix) != std::string::npos;
#endif
}

static bool is_allowed_download_url(const std::string& url)
{
	if (!starts_with(url, "https://")) {
		return false;
	}
	const size_t host_start = std::strlen("https://");
	const size_t slash = url.find('/', host_start);
	std::string host = slash == std::string::npos ? url.substr(host_start) : url.substr(host_start, slash - host_start);
	host = to_lower(host);
	if (host == "github.com" || host == "objects.githubusercontent.com") {
		return true;
	}
	return false;
}

static size_t write_data_to_string(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	const size_t total = size * nmemb;
	auto* ctx = static_cast<HttpContext*>(userdata);
	ctx->buffer.append(static_cast<const char*>(ptr), total);
	return total;
}

static size_t write_data_to_file(void* ptr, size_t size, size_t nmemb, void* stream)
{
	auto* ctx = static_cast<DownloadContext*>(stream);
	if (!ctx || !ctx->out || !ctx->out->is_open()) {
		return 0;
	}
	const size_t total = size * nmemb;
	ctx->out->write(static_cast<const char*>(ptr), static_cast<std::streamsize>(total));
	if (!ctx->out->good()) {
		return 0;
	}
	return total;
}

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
{
	auto* ctx = static_cast<DownloadContext*>(clientp);
	if (!ctx) {
		return 0;
	}
	if (ctx->cancel_flag && ctx->cancel_flag->load()) {
		return 1;
	}
	if (ctx->progress_cb) {
		const bool should_cancel = ctx->progress_cb(static_cast<int64_t>(dlnow), static_cast<int64_t>(dltotal));
		if (should_cancel) {
			return 1;
		}
	}
	return 0;
}

static bool http_get(const std::string& url, std::string& response_body, long& http_code, std::atomic<bool>* cancel_flag)
{
	ensure_curl_initialized();

	CURL* curl = curl_easy_init();
	if (!curl) {
		write_log("Updater: curl_easy_init failed\n");
		return false;
	}

	HttpContext ctx;

	const std::string ua = std::string("User-Agent: amiberry/") + AMIBERRY_VERSION;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ua.c_str());
	headers = curl_slist_append(headers, "Accept: application/vnd.github+json");

	DownloadContext progress;
	progress.cancel_flag = cancel_flag;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_to_string);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, k_http_timeout_sec);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	const CURLcode code = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	response_body.swap(ctx.buffer);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (code != CURLE_OK) {
		write_log("Updater: HTTP GET failed (%s) for %s\n", curl_easy_strerror(code), url.c_str());
		return false;
	}
	return true;
}

static bool parse_release_json(const nlohmann::json& rel, UpdateInfo& out)
{
	if (!rel.is_object()) {
		return false;
	}

	if (!rel.contains("tag_name") || !rel["tag_name"].is_string()) {
		return false;
	}

	out.tag_name = rel["tag_name"].get<std::string>();
	out.latest_version = out.tag_name;
	if (starts_with(out.latest_version, "preview-v")) {
		out.latest_version = out.latest_version.substr(std::strlen("preview-v"));
	}
	if (!out.latest_version.empty() && out.latest_version[0] == 'v') {
		out.latest_version.erase(out.latest_version.begin());
	}

	if (rel.contains("html_url") && rel["html_url"].is_string()) {
		out.release_url = rel["html_url"].get<std::string>();
	}
	if (rel.contains("body") && rel["body"].is_string()) {
		out.release_notes = rel["body"].get<std::string>();
	}
	if (rel.contains("prerelease") && rel["prerelease"].is_boolean()) {
		out.is_prerelease = rel["prerelease"].get<bool>();
	}

	std::string sha256_url;

	if (rel.contains("assets") && rel["assets"].is_array()) {
		for (const auto& asset : rel["assets"]) {
			if (!asset.is_object()) {
				continue;
			}
			if (!asset.contains("name") || !asset["name"].is_string()) {
				continue;
			}
			const std::string name = asset["name"].get<std::string>();
			if (name == "SHA256SUMS") {
				if (asset.contains("browser_download_url") && asset["browser_download_url"].is_string()) {
					sha256_url = asset["browser_download_url"].get<std::string>();
				}
				continue;
			}

			if (!matches_platform_asset(name)) {
				continue;
			}

			if (!out.download_url.empty()) {
				continue;
			}

			if (asset.contains("browser_download_url") && asset["browser_download_url"].is_string()) {
				out.download_url = asset["browser_download_url"].get<std::string>();
				out.asset_name = name;
				if (asset.contains("size") && asset["size"].is_number_integer()) {
					out.asset_size = asset["size"].get<int64_t>();
				}
			}
		}
	}

	if (!sha256_url.empty() && is_allowed_download_url(sha256_url)) {
		std::string sums_body;
		long sums_code = 0;
		if (http_get(sha256_url, sums_body, sums_code, nullptr) && sums_code >= 200 && sums_code < 300) {
			std::istringstream iss(sums_body);
			std::string line;
			static const std::regex sha_re(R"(^\s*([A-Fa-f0-9]{64})\s+\*?(.+?)\s*$)");
			while (std::getline(iss, line)) {
				if (out.asset_name.empty() || line.find(out.asset_name) == std::string::npos) {
					continue;
				}
				std::smatch m;
				if (std::regex_match(line, m, sha_re)) {
					out.sha256_expected = to_lower(m[1].str());
					break;
				}
			}
		}
	}

	return true;
}

static bool read_release(UpdateChannel channel, UpdateInfo& out, std::atomic<bool>* cancel_flag)
{
	std::string url;
	if (channel == UpdateChannel::Stable) {
		url = std::string(k_github_api_base) + "/latest";
	} else {
		url = k_github_api_base;
	}

	std::string body;
	long code = 0;
	if (!http_get(url, body, code, cancel_flag)) {
		out.error_message = "Failed to query GitHub releases";
		return false;
	}
	if (code < 200 || code >= 300) {
		out.error_message = "GitHub API returned HTTP " + std::to_string(code);
		return false;
	}

	nlohmann::json doc = nlohmann::json::parse(body, nullptr, false);
	if (doc.is_discarded()) {
		out.error_message = "Failed to parse GitHub API response";
		return false;
	}

	if (channel == UpdateChannel::Stable) {
		if (!parse_release_json(doc, out)) {
			out.error_message = "GitHub latest release payload missing fields";
			return false;
		}
		return true;
	}

	if (!doc.is_array() || doc.empty()) {
		out.error_message = "GitHub releases payload is empty";
		return false;
	}

	for (const auto& rel : doc) {
		if (!rel.is_object()) {
			continue;
		}
		if (rel.contains("draft") && rel["draft"].is_boolean() && rel["draft"].get<bool>()) {
			continue;
		}
		if (parse_release_json(rel, out)) {
			return true;
		}
	}

	out.error_message = "No usable release found";
	return false;
}

static std::string compute_sha256_hex(const std::string& file_path)
{
	std::ifstream file(file_path, std::ios::binary);
	if (!file.is_open()) {
		return {};
	}

	Sha256Ctx ctx;
	sha256_init(ctx);

	std::array<uint8_t, 8192> buf{};
	while (file.good()) {
		file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
		const std::streamsize got = file.gcount();
		if (got > 0) {
			sha256_update(ctx, buf.data(), static_cast<size_t>(got));
		}
	}

	const auto digest = sha256_final(ctx);
	static constexpr char hex[] = "0123456789abcdef";
	std::string out;
	out.reserve(64);
	for (uint8_t b : digest) {
		out.push_back(hex[(b >> 4) & 0x0f]);
		out.push_back(hex[b & 0x0f]);
	}
	return out;
}

}

UpdateMethod get_update_method()
{
#if defined(__ANDROID__) || defined(LIBRETRO)
	return UpdateMethod::DISABLED;
#elif defined(__linux__)
	if (getenv("FLATPAK_ID")) {
		return UpdateMethod::DISABLED;
	}
	return UpdateMethod::NOTIFY_ONLY;
#elif defined(_WIN32)
	return UpdateMethod::SELF_UPDATE;
#elif defined(__APPLE__)
	return UpdateMethod::SELF_UPDATE;
#else
	return UpdateMethod::NOTIFY_ONLY;
#endif
}

std::string get_current_semver()
{
	std::string ver = std::to_string(AMIBERRY_VERSION_MAJOR) + "."
		+ std::to_string(AMIBERRY_VERSION_MINOR) + "."
		+ std::to_string(AMIBERRY_VERSION_PATCH);
	const char* pre = AMIBERRY_VERSION_PRE_RELEASE;
	if (pre && pre[0] != '\0') {
		ver += "-pre." + std::string(pre);
	}
	return ver;
}

int compare_versions(const std::string& a, const std::string& b)
{
	const SemVer va = SemVer::parse(a);
	const SemVer vb = SemVer::parse(b);
	return va.compare(vb);
}

UpdateInfo check_for_updates(UpdateChannel channel)
{
	UpdateInfo info;
	info.current_version = get_current_semver();

	if (get_update_method() == UpdateMethod::DISABLED) {
		info.error_message = "Update checks are disabled on this platform";
		return info;
	}

	if (!read_release(channel, info, &s_update_cancel)) {
		if (s_update_cancel.load()) {
			info.error_message = "Update check cancelled";
		}
		return info;
	}

	if (!info.download_url.empty() && !is_allowed_download_url(info.download_url)) {
		write_log("Updater: rejected download URL with unexpected host: %s\n", info.download_url.c_str());
		info.download_url.clear();
		info.asset_name.clear();
	}

	const int cmp = compare_versions(info.current_version, info.latest_version);
	info.update_available = cmp < 0;

	if (!info.update_available) {
		write_log("Updater: current %s is up to date with %s\n", info.current_version.c_str(), info.latest_version.c_str());
	}
	return info;
}

std::string download_update(const UpdateInfo& info, DownloadProgressCallback progress_cb)
{
	if (info.download_url.empty()) {
		write_log("Updater: no download URL in update info\n");
		return {};
	}
	if (!is_allowed_download_url(info.download_url)) {
		write_log("Updater: refusing download from non-GitHub domain: %s\n", info.download_url.c_str());
		return {};
	}

	const std::string exe_path = get_executable_path();
	std::filesystem::path dest_dir;
	if (!exe_path.empty()) {
		dest_dir = std::filesystem::path(exe_path).parent_path();
	}
	if (dest_dir.empty()) {
		dest_dir = std::filesystem::temp_directory_path();
	}

	std::string asset_name = info.asset_name;
	if (asset_name.empty()) {
		const size_t slash = info.download_url.find_last_of('/');
		asset_name = slash == std::string::npos ? "amiberry-update.bin" : info.download_url.substr(slash + 1);
	}

	const std::filesystem::path final_path = dest_dir / asset_name;
	const std::filesystem::path temp_path = final_path.string() + ".tmp";

	std::error_code ec;
	std::filesystem::remove(temp_path, ec);

	std::ofstream out(temp_path, std::ios::binary);
	if (!out.is_open()) {
		write_log("Updater: failed to open temp file for download: %s\n", temp_path.string().c_str());
		return {};
	}

	ensure_curl_initialized();
	CURL* curl = curl_easy_init();
	if (!curl) {
		write_log("Updater: curl_easy_init failed in download_update\n");
		out.close();
		std::filesystem::remove(temp_path, ec);
		return {};
	}

	const std::string ua = std::string("User-Agent: amiberry/") + AMIBERRY_VERSION;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ua.c_str());

	DownloadContext dctx;
	dctx.out = &out;
	dctx.progress_cb = progress_cb;
	dctx.cancel_flag = &s_update_cancel;

	curl_easy_setopt(curl, CURLOPT_URL, info.download_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_to_file);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dctx);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &dctx);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	const CURLcode code = curl_easy_perform(curl);
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	out.close();

	if (code != CURLE_OK) {
		write_log("Updater: download failed (%s)\n", curl_easy_strerror(code));
		std::filesystem::remove(temp_path, ec);
		return {};
	}
	if (http_code < 200 || http_code >= 300) {
		write_log("Updater: download HTTP error %ld\n", http_code);
		std::filesystem::remove(temp_path, ec);
		return {};
	}

	std::filesystem::remove(final_path, ec);
	std::filesystem::rename(temp_path, final_path, ec);
	if (ec) {
		write_log("Updater: failed to finalize download: %s\n", ec.message().c_str());
		std::filesystem::remove(temp_path, ec);
		return {};
	}

	return final_path.string();
}

bool verify_update_checksum(const std::string& file_path, const std::string& expected_sha256)
{
	if (expected_sha256.empty()) {
		write_log("Updater: SHA256 not provided, skipping verification\n");
		return true;
	}
	const std::string computed = compute_sha256_hex(file_path);
	if (computed.empty()) {
		write_log("Updater: failed to compute SHA256 for %s\n", file_path.c_str());
		return false;
	}
	const bool ok = to_lower(computed) == to_lower(expected_sha256);
	if (!ok) {
		write_log("Updater: SHA256 mismatch for %s\n", file_path.c_str());
		write_log("Updater: expected %s\n", expected_sha256.c_str());
		write_log("Updater: got      %s\n", computed.c_str());
	}
	return ok;
}

#ifndef _WIN32
static int run_tool(const std::vector<std::string>& args)
{
	if (args.empty()) return -1;
	std::vector<char*> argv;
	for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
	argv.push_back(nullptr);

	pid_t pid = 0;
#ifdef __APPLE__
	int ret = posix_spawn(&pid, argv[0], nullptr, nullptr, argv.data(), *_NSGetEnviron());
#else
	extern char **environ;
	int ret = posix_spawn(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
#endif
	if (ret != 0) {
		write_log("Updater: failed to spawn %s: %s\n", argv[0], strerror(ret));
		return -1;
	}
	int status = 0;
	waitpid(pid, &status, 0);
	return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif

#ifdef __APPLE__
static void remove_quarantine_recursive(const std::filesystem::path& path)
{
	removexattr(path.c_str(), "com.apple.quarantine", XATTR_NOFOLLOW);
	std::error_code ec;
	if (std::filesystem::is_directory(path, ec)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(
				path, std::filesystem::directory_options::skip_permission_denied, ec)) {
			removexattr(entry.path().c_str(), "com.apple.quarantine", XATTR_NOFOLLOW);
		}
	}
}
#endif

bool apply_update(const std::string& downloaded_file, const UpdateInfo&)
{
	if (downloaded_file.empty()) {
		write_log("Updater: apply_update called with empty file path\n");
		return false;
	}

	if (get_update_method() != UpdateMethod::SELF_UPDATE) {
		write_log("Updater: platform is not self-update capable\n");
		return false;
	}

#ifdef __linux__
	const std::string exe_path = get_executable_path();
	if (exe_path.empty()) {
		write_log("Updater: failed to locate executable path\n");
		return false;
	}

	const auto install_dir = std::filesystem::path(exe_path).parent_path();
	const std::string lower = to_lower(downloaded_file);
	const bool is_zip = lower.find(".zip") != std::string::npos;

	if (!is_zip && (lower.find(".tar") != std::string::npos || lower.find(".gz") != std::string::npos
		|| lower.find(".bz2") != std::string::npos || lower.find(".xz") != std::string::npos
		|| lower.find(".7z") != std::string::npos)) {
		write_log("Updater: unsupported archive format for self-update: %s\n", downloaded_file.c_str());
		return false;
	}

	if (is_zip) {
		const auto staging_dir = install_dir / "_amiberry_update";

		std::error_code ec;
		std::filesystem::remove_all(staging_dir, ec);
		std::filesystem::create_directories(staging_dir, ec);
		if (ec) {
			write_log("Updater: failed to create staging directory: %s\n", ec.message().c_str());
			return false;
		}

		int ret = run_tool({"/usr/bin/unzip", "-o", downloaded_file, "-d", staging_dir.string()});
		if (ret != 0) {
			write_log("Updater: failed to extract ZIP (exit code %d). Is 'unzip' installed?\n", ret);
			std::filesystem::remove_all(staging_dir, ec);
			return false;
		}

		std::filesystem::path content_root;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(staging_dir, ec)) {
			if (entry.is_regular_file() && entry.path().filename() == "amiberry") {
				content_root = entry.path().parent_path();
				break;
			}
		}
		if (content_root.empty()) {
			write_log("Updater: could not find amiberry binary in extracted archive\n");
			std::filesystem::remove_all(staging_dir, ec);
			return false;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(content_root, ec)) {
			const auto relative = std::filesystem::relative(entry.path(), content_root, ec);
			if (ec) { ec.clear(); continue; }
			const auto dest_path = install_dir / relative;

			if (entry.is_directory()) {
				std::filesystem::create_directories(dest_path, ec);
			} else if (entry.is_regular_file()) {
				std::filesystem::create_directories(dest_path.parent_path(), ec);
				std::filesystem::copy_file(entry.path(), dest_path,
					std::filesystem::copy_options::overwrite_existing, ec);
				if (ec) {
					write_log("Updater: warning: failed to copy %s: %s\n",
						relative.string().c_str(), ec.message().c_str());
					ec.clear();
				}
			}
		}

		if (chmod(exe_path.c_str(), 0755) != 0) {
			write_log("Updater: chmod failed for updated executable: %s\n", std::strerror(errno));
		}

		std::filesystem::remove_all(staging_dir, ec);
		std::filesystem::remove(downloaded_file, ec);
	} else {
		const std::filesystem::path exe = exe_path;
		const std::filesystem::path old = exe_path + ".old";

		std::error_code ec;
		std::filesystem::remove(old, ec);
		std::filesystem::rename(exe, old, ec);
		if (ec) {
			write_log("Updater: failed to backup executable: %s\n", ec.message().c_str());
			return false;
		}

		std::filesystem::copy_file(downloaded_file, exe, std::filesystem::copy_options::overwrite_existing, ec);
		if (ec) {
			write_log("Updater: failed to copy new executable: %s\n", ec.message().c_str());
			std::error_code ec_restore;
			std::filesystem::rename(old, exe, ec_restore);
			if (ec_restore) {
				write_log("Updater: failed to restore previous executable: %s\n", ec_restore.message().c_str());
			}
			return false;
		}

		if (chmod(exe_path.c_str(), 0755) != 0) {
			write_log("Updater: chmod failed for updated executable: %s\n", std::strerror(errno));
		}
	}

	write_log("Updater: update applied successfully, restart required\n");
	return true;

#elif defined(_WIN32)
	const std::string exe_path = get_executable_path();
	if (exe_path.empty()) {
		write_log("Updater: failed to locate executable path\n");
		return false;
	}

	const auto install_dir = std::filesystem::path(exe_path).parent_path();
	const auto staging_dir = install_dir / "_amiberry_update";

	std::error_code ec;
	std::filesystem::remove_all(staging_dir, ec);
	std::filesystem::create_directories(staging_dir, ec);
	if (ec) {
		write_log("Updater: failed to create staging directory: %s\n", ec.message().c_str());
		return false;
	}

	{
		std::wstring wide_zip;
		int needed = MultiByteToWideChar(CP_UTF8, 0, downloaded_file.c_str(), -1, nullptr, 0);
		if (needed <= 0) {
			write_log("Updater: failed to convert ZIP path to wide string\n");
			std::filesystem::remove_all(staging_dir, ec);
			return false;
		}
		wide_zip.resize(static_cast<size_t>(needed));
		MultiByteToWideChar(CP_UTF8, 0, downloaded_file.c_str(), -1, wide_zip.data(), needed);
		wide_zip.pop_back();

		std::wstring ps_cmd =
			L"powershell.exe -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -Command "
			L"\"Expand-Archive -LiteralPath '" + wide_zip + L"'"
			L" -DestinationPath '" + staging_dir.wstring() + L"' -Force\"";

		STARTUPINFOW si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi{};

		if (!CreateProcessW(nullptr, ps_cmd.data(), nullptr, nullptr, FALSE,
				CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
			write_log("Updater: failed to launch PowerShell for ZIP extraction (error %lu)\n", GetLastError());
			std::filesystem::remove_all(staging_dir, ec);
			return false;
		}

		const DWORD wait_result = WaitForSingleObject(pi.hProcess, 120000);
		DWORD exit_code = 1;
		GetExitCodeProcess(pi.hProcess, &exit_code);
		if (wait_result == WAIT_TIMEOUT || exit_code == STILL_ACTIVE) {
			TerminateProcess(pi.hProcess, 1);
			exit_code = 1;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (exit_code != 0) {
			write_log("Updater: ZIP extraction failed (PowerShell exit code %lu)\n", exit_code);
			std::filesystem::remove_all(staging_dir, ec);
			return false;
		}
	}

	std::filesystem::path content_root;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(staging_dir, ec)) {
		if (entry.is_regular_file() && to_lower(entry.path().filename().string()) == "amiberry.exe") {
			content_root = entry.path().parent_path();
			break;
		}
	}
	if (content_root.empty()) {
		write_log("Updater: could not find amiberry.exe in extracted archive\n");
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	const auto script_path = install_dir / "_amiberry_update.bat";
	{
		std::ofstream script(script_path);
		if (!script.is_open()) {
			write_log("Updater: failed to create update script\n");
			std::filesystem::remove_all(staging_dir, ec);
			return false;
		}

		const DWORD pid = GetCurrentProcessId();

		script << "@echo off\r\n";
		script << "setlocal\r\n";
		script << "echo Waiting for Amiberry to exit...\r\n";
		script << ":waitloop\r\n";
		script << "tasklist /FI \"PID eq " << pid << "\" 2>nul | find /I \"" << pid << "\" >nul\r\n";
		script << "if not errorlevel 1 (\r\n";
		script << "    timeout /t 1 /nobreak >nul\r\n";
		script << "    goto waitloop\r\n";
		script << ")\r\n";
		script << "echo Applying update...\r\n";
		script << "xcopy /E /Y /Q \"" << content_root.string() << "\\*\" \"" << install_dir.string() << "\\\"\r\n";
		script << "echo Cleaning up...\r\n";
		script << "rmdir /S /Q \"" << staging_dir.string() << "\"\r\n";
		script << "del /Q \"" << downloaded_file << "\"\r\n";
		script << "echo Starting updated Amiberry...\r\n";
		script << "start \"\" \"" << exe_path << "\"\r\n";
		script << "del \"%~f0\" & exit\r\n";
		script.close();
	}

	s_windows_restart_script = script_path.string();
	write_log("Updater: Windows update prepared, script at %s\n", s_windows_restart_script.c_str());
	return true;

#elif defined(__APPLE__)
	const std::string exe_path = get_executable_path();
	if (exe_path.empty()) {
		write_log("Updater: failed to locate executable path\n");
		return false;
	}

	auto app_bundle = std::filesystem::path(exe_path).parent_path().parent_path().parent_path();
	if (app_bundle.extension() != ".app") {
		write_log("Updater: executable is not inside a .app bundle (%s), cannot self-update\n",
			app_bundle.string().c_str());
		return false;
	}

	const auto app_parent = app_bundle.parent_path();
	const auto staging_dir = app_parent / "_amiberry_update_staging";

	std::error_code ec;
	std::filesystem::remove_all(staging_dir, ec);
	std::filesystem::create_directories(staging_dir, ec);
	if (ec) {
		write_log("Updater: failed to create staging directory: %s\n", ec.message().c_str());
		return false;
	}

	int ret = run_tool({"/usr/bin/ditto", "-x", "-k", downloaded_file, staging_dir.string()});
	if (ret != 0) {
		write_log("Updater: failed to extract ZIP with ditto (exit code %d)\n", ret);
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	std::filesystem::path dmg_path;
	for (const auto& entry : std::filesystem::directory_iterator(staging_dir, ec)) {
		if (entry.is_regular_file() && to_lower(entry.path().extension().string()) == ".dmg") {
			dmg_path = entry.path();
			break;
		}
	}
	if (dmg_path.empty()) {
		write_log("Updater: no DMG found in extracted ZIP\n");
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	const auto mount_dir = staging_dir / "dmg_mount";
	std::filesystem::create_directories(mount_dir, ec);

	ret = run_tool({"/usr/bin/hdiutil", "attach", dmg_path.string(),
		"-mountpoint", mount_dir.string(),
		"-noverify", "-nobrowse", "-noautoopen"});
	if (ret != 0) {
		write_log("Updater: failed to mount DMG (exit code %d)\n", ret);
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	std::filesystem::path new_app_src;
	for (const auto& entry : std::filesystem::directory_iterator(mount_dir, ec)) {
		if (entry.is_directory() && entry.path().extension() == ".app") {
			new_app_src = entry.path();
			break;
		}
	}
	if (new_app_src.empty()) {
		write_log("Updater: no .app bundle found in mounted DMG\n");
		run_tool({"/usr/bin/hdiutil", "detach", mount_dir.string(), "-force"});
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	const auto staged_app = staging_dir / app_bundle.filename();
	ret = run_tool({"/usr/bin/ditto", new_app_src.string(), staged_app.string()});
	if (ret != 0) {
		write_log("Updater: failed to copy app from DMG (ditto exit code %d)\n", ret);
		run_tool({"/usr/bin/hdiutil", "detach", mount_dir.string(), "-force"});
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	run_tool({"/usr/bin/hdiutil", "detach", mount_dir.string(), "-force"});

	remove_quarantine_recursive(staged_app);

	const auto old_app = std::filesystem::path(app_bundle.string() + ".old");
	std::filesystem::remove_all(old_app, ec);

	std::filesystem::rename(app_bundle, old_app, ec);
	if (ec) {
		write_log("Updater: failed to move old app bundle aside: %s\n", ec.message().c_str());
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	std::filesystem::rename(staged_app, app_bundle, ec);
	if (ec) {
		write_log("Updater: failed to move new app bundle into place: %s\n", ec.message().c_str());
		std::filesystem::rename(old_app, app_bundle, ec);
		std::filesystem::remove_all(staging_dir, ec);
		return false;
	}

	std::filesystem::remove_all(old_app, ec);
	std::filesystem::remove_all(staging_dir, ec);
	std::filesystem::remove(downloaded_file, ec);

	s_macos_new_executable = (app_bundle / "Contents" / "MacOS" / "Amiberry").string();
	write_log("Updater: macOS update applied, restart required\n");
	return true;

#else
	write_log("Updater: self-update not implemented for this platform\n");
	(void)downloaded_file;
	return false;
#endif
}

void restart_after_update()
{
#ifdef _WIN32
	if (s_windows_restart_script.empty()) {
		write_log("Updater: restart script not set on Windows\n");
		return;
	}
	SHELLEXECUTEINFOW sei{};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = L"open";
	sei.nShow = SW_HIDE;
	std::wstring script;
	int len = MultiByteToWideChar(CP_UTF8, 0, s_windows_restart_script.c_str(), -1, nullptr, 0);
	if (len > 0) {
		script.resize(static_cast<size_t>(len));
		MultiByteToWideChar(CP_UTF8, 0, s_windows_restart_script.c_str(), -1, script.data(), len);
		sei.lpFile = script.c_str();
		if (ShellExecuteExW(&sei)) {
			ExitProcess(0);
		}
	}
	write_log("Updater: failed to launch Windows restart script\n");
#elif defined(__APPLE__)
	// After apply_update() renames the .app bundle, _NSGetExecutablePath returns
	// the stale path. Use the stored path to the new executable instead.
	std::string exe = s_macos_new_executable;
	if (exe.empty())
		exe = get_executable_path();
	if (exe.empty()) {
		write_log("Updater: failed to resolve executable path for restart\n");
		return;
	}

	auto app_bundle = std::filesystem::path(exe).parent_path().parent_path().parent_path();
	if (app_bundle.extension() == ".app") {
		write_log("Updater: relaunching %s via open(1)\n", app_bundle.c_str());
		std::string app_str = app_bundle.string();
		char* args[] = {
			const_cast<char*>("/usr/bin/open"),
			const_cast<char*>("-n"),
			const_cast<char*>(app_str.c_str()),
			nullptr
		};
		pid_t pid = 0;
		int ret = posix_spawn(&pid, "/usr/bin/open", nullptr, nullptr, args, *_NSGetEnviron());
		if (ret == 0) {
			_exit(0);
		}
		write_log("Updater: open(1) failed (%s), falling back to execl\n", strerror(ret));
	}

	write_log("Updater: restarting from %s\n", exe.c_str());
	execl(exe.c_str(), exe.c_str(), nullptr);
	write_log("Failed to restart after update: %s\n", std::strerror(errno));
#else
	char exe_path[PATH_MAX] = {};
	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (len <= 0) {
		return;
	}
	exe_path[len] = '\0';
	execl(exe_path, exe_path, nullptr);
	write_log("Failed to restart after update: %s\n", std::strerror(errno));
#endif
}

void start_async_update_check(UpdateChannel channel)
{
	if (s_update_running.load()) {
		return;
	}

	s_update_running.store(true);
	s_update_cancel.store(false);

	if (s_update_thread.joinable()) {
		s_update_thread.join();
	}

	s_update_thread = std::thread([channel]() {
		UpdateInfo result = check_for_updates(channel);
		{
			std::lock_guard<std::mutex> lock(s_update_mutex);
			s_update_result = result;
		}
		s_update_running.store(false);
	});
}

bool is_update_check_running()
{
	return s_update_running.load();
}

UpdateInfo get_async_update_result()
{
	if (s_update_thread.joinable() && !s_update_running.load()) {
		s_update_thread.join();
	}
	std::lock_guard<std::mutex> lock(s_update_mutex);
	return s_update_result;
}

void cancel_async_update_check()
{
	s_update_cancel.store(true);
	if (s_update_thread.joinable()) {
		s_update_thread.join();
	}
	s_update_running.store(false);
}
