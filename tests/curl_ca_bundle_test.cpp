#include "amiberry_curl.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void expect_equal(const std::string& actual, const std::string& expected, const char* scenario)
{
	if (actual == expected)
		return;
	std::cerr << scenario << ": expected '" << expected << "', got '" << actual << "'\n";
	std::exit(1);
}

void expect_true(const bool condition, const char* scenario)
{
	if (condition)
		return;
	std::cerr << scenario << '\n';
	std::exit(1);
}

size_t discard_response(char*, const size_t size, const size_t count, void*)
{
	return size * count;
}

void write_bundle(const std::filesystem::path& path)
{
	std::ofstream output(path);
	output << "test certificate bundle\n";
}

// Performs a verified request to the local fixture after applying ca_bundle.
CURLcode request_with_ca_bundle(const std::string& ca_bundle, const std::string& https_url)
{
	CURL* handle = curl_easy_init();
	expect_true(handle != nullptr, "curl handle initialization");

	std::string error;
	expect_true(apply_ca_bundle_to_curl(handle, ca_bundle, &error),
		"applying CA bundle selection to curl handle");
	expect_true(curl_easy_setopt(handle, CURLOPT_PROXY, "") == CURLE_OK,
		"disabling proxy use for local HTTPS fixture");
	expect_true(curl_easy_setopt(handle, CURLOPT_URL, https_url.c_str()) == CURLE_OK,
		"setting local HTTPS URL");
	expect_true(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK,
		"enabling TLS peer verification");
	expect_true(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK,
		"enabling TLS hostname verification");
	expect_true(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, discard_response) == CURLE_OK,
		"setting response discard callback");
	expect_true(curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10L) == CURLE_OK,
		"setting local HTTPS timeout");
	const CURLcode result = curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	return result;
}

void verify_environment_precedence(const std::filesystem::path& root)
{
	const auto compiled = root / "compiled.pem";
	const auto ssl_cert_file = root / "ssl-cert-file.pem";
	const auto curl_ca_bundle = root / "curl-ca-bundle.pem";
	write_bundle(ssl_cert_file);
	write_bundle(curl_ca_bundle);

	setenv("SSL_CERT_FILE", ssl_cert_file.c_str(), 1);
	setenv("CURL_CA_BUNDLE", curl_ca_bundle.c_str(), 1);
	expect_equal(select_readable_ca_bundle(get_ca_bundle_candidates(compiled.string())),
		ssl_cert_file.string(), "SSL_CERT_FILE precedence");

	setenv("SSL_CERT_FILE", (root / "missing.pem").c_str(), 1);
	expect_equal(select_readable_ca_bundle(get_ca_bundle_candidates(compiled.string())),
		curl_ca_bundle.string(), "CURL_CA_BUNDLE after unreadable SSL_CERT_FILE");

	unsetenv("SSL_CERT_FILE");
	unsetenv("CURL_CA_BUNDLE");
	expect_equal(select_readable_ca_bundle(get_ca_bundle_candidates(compiled.string())),
		compiled.string(), "compiled default without configured bundles");
}

}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "usage: " << argv[0] << " <local-ca.pem> <https-url>\n";
		return 2;
	}
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
	{
		std::cerr << "curl global initialization failed\n";
		return 1;
	}

	const auto root = std::filesystem::temp_directory_path() / "amiberry-ca-bundle-test";
	std::error_code error;
	std::filesystem::remove_all(root, error);
	std::filesystem::create_directories(root);

	const auto compiled = root / "compiled.pem";
	const auto fedora = root / "fedora.pem";
	const auto debian = root / "debian.pem";
	write_bundle(compiled);
	write_bundle(fedora);
	write_bundle(debian);

	expect_equal(select_readable_ca_bundle({compiled.string(), fedora.string()}),
		compiled.string(), "first readable candidate precedence");
	expect_equal(select_readable_ca_bundle({(root / "missing.pem").string(), fedora.string()}),
		fedora.string(), "fallback after missing candidate");
	expect_equal(select_readable_ca_bundle({root.string(), debian.string()}),
		debian.string(), "directory rejected as CA bundle");
	expect_equal(select_readable_ca_bundle({}), "", "no readable CA bundle");

	verify_environment_precedence(root);

	const std::string local_ca = argv[1];
	const std::string https_url = argv[2];
	expect_true(request_with_ca_bundle(local_ca, https_url) == CURLE_OK,
		"local HTTPS request trusted through CURLOPT_CAINFO");

	// No readable bundle keeps libcurl's own trust store, which does not
	// contain the fixture's private CA, so verification must still reject it.
	const CURLcode untrusted = request_with_ca_bundle({}, https_url);
	expect_true(untrusted == CURLE_PEER_FAILED_VERIFICATION || untrusted == CURLE_SSL_CACERT_BADFILE,
		"missing CA bundle keeps TLS verification enabled");

	std::string null_error;
	expect_true(!apply_ca_bundle_to_curl(nullptr, local_ca, &null_error),
		"null curl handle rejected");

	std::filesystem::remove_all(root, error);
	curl_global_cleanup();
	std::cout << "curl CA bundle policy test passed\n";
	return 0;
}
