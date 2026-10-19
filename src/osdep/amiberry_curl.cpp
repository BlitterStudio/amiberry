#include "amiberry_curl.h"

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

}

std::string select_readable_ca_bundle(const std::string& compiled_default,
	const std::vector<std::string>& fallback_candidates)
{
	if (is_readable_regular_file(compiled_default))
		return compiled_default;
	for (const auto& candidate : fallback_candidates)
	{
		if (candidate != compiled_default && is_readable_regular_file(candidate))
			return candidate;
	}
	return {};
}

const std::string& get_effective_ca_bundle()
{
#if defined(__linux__)
#ifdef AMIBERRY_CURL_CA_BUNDLE
	static constexpr const char* compiled_default = AMIBERRY_CURL_CA_BUNDLE;
#else
	static constexpr const char* compiled_default = "";
#endif
	static const std::string selected = select_readable_ca_bundle(compiled_default, {
		"/etc/ssl/certs/ca-certificates.crt",
		"/etc/pki/tls/certs/ca-bundle.crt",
		"/etc/ssl/ca-bundle.pem",
		"/var/lib/ca-certificates/ca-bundle.pem",
		"/etc/ca-certificates/extracted/tls-ca-bundle.pem"
	});
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
	if (ca_bundle.empty())
	{
		if (error_out != nullptr)
			*error_out = "no readable CA bundle was found";
		return false;
	}
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
