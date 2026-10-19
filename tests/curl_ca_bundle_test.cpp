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

void verify_ca_bundle_application(const std::string& compiled_default, const std::string& https_url)
{
	const std::string ca_bundle = select_readable_ca_bundle(compiled_default, {});
	expect_equal(ca_bundle, compiled_default, "local HTTPS CA bundle selection");

	CURL* handle = curl_easy_init();
	expect_true(handle != nullptr, "curl handle initialization");

	std::string error;
	expect_true(apply_ca_bundle_to_curl(handle, ca_bundle, &error),
		"applying selected CA bundle to curl handle");
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
	expect_true(curl_easy_perform(handle) == CURLE_OK,
		"local HTTPS request trusted through CURLOPT_CAINFO");
	curl_easy_cleanup(handle);
}

void verify_ca_bundle_failures(const std::string& readable_bundle)
{
	std::string error;
	expect_true(!apply_ca_bundle_to_curl(nullptr, readable_bundle, &error),
		"null curl handle rejected");

	CURL* handle = curl_easy_init();
	expect_true(handle != nullptr, "curl handle initialization for failure coverage");
	const std::string no_bundle = select_readable_ca_bundle({}, {});
	expect_true(no_bundle.empty(), "no readable CA bundle selected");
	expect_true(!apply_ca_bundle_to_curl(handle, no_bundle, &error),
		"missing CA bundle rejected");
	curl_easy_cleanup(handle);
}

}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "usage: curl_ca_bundle_test <ca-bundle> <https-url>\n";
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
	const auto opensuse = root / "opensuse.pem";
	write_bundle(compiled);
	write_bundle(fedora);
	write_bundle(debian);
	write_bundle(opensuse);

	expect_equal(select_readable_ca_bundle(compiled.string(), {fedora.string()}),
		compiled.string(), "readable compiled default precedence");
	expect_equal(select_readable_ca_bundle((root / "missing.pem").string(), {fedora.string()}),
		fedora.string(), "Fedora fallback selection");
	expect_equal(select_readable_ca_bundle({}, {debian.string(), opensuse.string()}),
		debian.string(), "Debian fallback ordering");
	expect_equal(select_readable_ca_bundle({}, {opensuse.string()}),
		opensuse.string(), "openSUSE fallback selection");
	expect_equal(select_readable_ca_bundle({}, {root.string()}), "",
		"directory rejected as CA bundle");
	expect_equal(select_readable_ca_bundle({}, {}), "",
		"missing CA bundle failure");

	verify_ca_bundle_application(argv[1], argv[2]);
	verify_ca_bundle_failures(argv[1]);

	std::filesystem::remove_all(root, error);
	curl_global_cleanup();
	std::cout << "curl CA bundle policy test passed\n";
	return 0;
}
