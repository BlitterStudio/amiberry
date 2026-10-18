#pragma once

#include <string>
#include <vector>

#ifdef AMIBERRY_HAS_CURL
#include <curl/curl.h>
#endif

std::string select_readable_ca_bundle(const std::string& compiled_default,
	const std::vector<std::string>& fallback_candidates);
const std::string& get_effective_ca_bundle();

#ifdef AMIBERRY_HAS_CURL
bool apply_ca_bundle_to_curl(CURL* handle, const std::string& ca_bundle,
	std::string* error_out = nullptr);
bool apply_effective_ca_bundle_to_curl(CURL* handle, std::string* error_out = nullptr);
#endif
