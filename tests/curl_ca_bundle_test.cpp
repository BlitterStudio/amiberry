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

void write_bundle(const std::filesystem::path& path)
{
	std::ofstream output(path);
	output << "test certificate bundle\n";
}

}

int main()
{
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

	std::filesystem::remove_all(root, error);
	std::cout << "curl CA bundle policy test passed\n";
	return 0;
}
