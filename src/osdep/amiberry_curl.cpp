#include "amiberry_curl.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {

bool is_readable_regular_file(const std::string& path)
{
	if (path.empty())
		return false;
	std::error_code error;
	if (!std::filesystem::is_regular_file(path, error) || error)
		return false;
	std::ifstream input(path, std::ios::binary);
	return input.good();
}

void append_environment_path(std::vector<std::string>& candidates, const char* name)
{
	const auto* value = std::getenv(name);
	if (value != nullptr && value[0] != '\0')
		candidates.emplace_back(value);
}

}

std::string select_readable_ca_bundle(const std::vector<std::string>& candidates)
{
	for (const auto& candidate : candidates)
	{
		if (is_readable_regular_file(candidate))
			return candidate;
	}
	return {};
}

std::vector<std::string> get_ca_bundle_candidates(const std::string& compiled_default)
{
	// User-configured trust stores come first: OpenSSL's SSL_CERT_FILE and the
	// curl tool's CURL_CA_BUNDLE. The distribution locations cover binaries
	// built on one distribution and run on another, such as the AppImage.
	std::vector<std::string> candidates;
	append_environment_path(candidates, "SSL_CERT_FILE");
	append_environment_path(candidates, "CURL_CA_BUNDLE");
	if (!compiled_default.empty())
		candidates.push_back(compiled_default);
	for (const auto* path : {
		"/etc/ssl/certs/ca-certificates.crt",
		"/etc/pki/tls/certs/ca-bundle.crt",
		"/etc/ssl/ca-bundle.pem",
		"/var/lib/ca-certificates/ca-bundle.pem",
		"/etc/ca-certificates/extracted/tls-ca-bundle.pem"
	})
		candidates.emplace_back(path);
	return candidates;
}

const std::string& get_effective_ca_bundle()
{
#if defined(__linux__)
#ifdef AMIBERRY_CURL_CA_BUNDLE
	static constexpr const char* compiled_default = AMIBERRY_CURL_CA_BUNDLE;
#else
	static constexpr const char* compiled_default = "";
#endif
	static const std::string selected = select_readable_ca_bundle(get_ca_bundle_candidates(compiled_default));
	return selected;
#else
	static const std::string empty;
	return empty;
#endif
}

#ifdef AMIBERRY_HAS_CURL
bool apply_ca_bundle_to_curl(CURL* handle, const std::string& ca_bundle, std::string* error_out)
{
	if (handle == nullptr)
	{
		if (error_out != nullptr)
			*error_out = "libcurl handle is null";
		return false;
	}
	// Without a readable bundle, keep libcurl's own trust configuration. Peer
	// and host verification stay enabled either way.
	if (ca_bundle.empty())
		return true;
	const CURLcode result = curl_easy_setopt(handle, CURLOPT_CAINFO, ca_bundle.c_str());
	if (result != CURLE_OK)
	{
		if (error_out != nullptr)
			*error_out = curl_easy_strerror(result);
		return false;
	}
	return true;
}

bool apply_effective_ca_bundle_to_curl(CURL* handle, std::string* error_out)
{
#if defined(__linux__)
	return apply_ca_bundle_to_curl(handle, get_effective_ca_bundle(), error_out);
#else
	(void)handle;
	(void)error_out;
	return true;
#endif
}
#endif
