#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "amiberry_filesys_permissions.h"

static int failures;

static void expect_mode(const mode_t actual, const mode_t expected, const char* message)
{
	if ((actual & 07777) != expected) {
		std::cerr << message << ": expected 0" << std::oct << expected
			<< ", got 0" << (actual & 07777) << std::dec << '\n';
		failures++;
	}
}

static void test_new_regular_file_is_not_executable()
{
	const mode_t mode = amiberry_filesys_apply_protection_bits(
		S_IFREG | 0640, false, false, false);
	expect_mode(mode, 0644, "new regular file must use normal data-file permissions");
}

static void test_new_directory_remains_searchable()
{
	const mode_t mode = amiberry_filesys_apply_protection_bits(
		S_IFDIR | 0755, false, false, false);
	expect_mode(mode, 0755, "new directory must retain execute/search permissions");
}

static void test_existing_regular_file_execute_bits_are_preserved()
{
	const mode_t mode = amiberry_filesys_apply_protection_bits(
		S_IFREG | 0750, false, false, false);
	expect_mode(mode, 0754, "existing host executable must retain its execute bits");
}

static void test_execute_protection_clears_regular_file_execute_bits()
{
	const mode_t mode = amiberry_filesys_apply_protection_bits(
		S_IFREG | 0755, false, false, true);
	expect_mode(mode, 0644, "execute protection must remove host execute bits");
}

static void test_execute_protection_clears_directory_search_bits()
{
	const mode_t mode = amiberry_filesys_apply_protection_bits(
		S_IFDIR | 0755, false, false, true);
	expect_mode(mode, 0644, "execute protection must remove directory search bits");
}

int main()
{
	test_new_regular_file_is_not_executable();
	test_new_directory_remains_searchable();
	test_existing_regular_file_execute_bits_are_preserved();
	test_execute_protection_clears_regular_file_execute_bits();
	test_execute_protection_clears_directory_search_bits();
	return failures == 0 ? 0 : 1;
}
