#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "imgui/shader_catalog.h"

namespace {

std::string shaders_path;
int failures;

void expect(const bool condition, const char* message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

bool contains(const std::vector<std::string>& names, const std::string& name)
{
	return std::find(names.begin(), names.end(), name) != names.end();
}

void create_file(const std::filesystem::path& path)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream(path).put('\n');
}

} // namespace

std::string get_shaders_path()
{
	return shaders_path;
}

int main()
{
	const auto suffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	const auto root = std::filesystem::temp_directory_path()
		/ ("amiberry-shader-catalog-" + std::to_string(suffix));
	const auto alternate_root = root.string() + "-alternate";

	try {
		create_file(root / "custom.glsl");
		create_file(root / "crt" / "preset.glslp");
		create_file(root / "crt" / "component.glsl");
		create_file(root / "ignored.txt");
		shaders_path = root.string();

		const auto& names = get_available_shader_names();
		expect(names.size() >= 7, "Catalog must include built-in and external shaders");
		expect(names[0] == "none" && names[1] == "tv" && names[2] == "pc"
			&& names[3] == "lite" && names[4] == "1084",
			"Built-in shaders must retain their UI order");
		expect(contains(names, "custom.glsl"), "Top-level GLSL shaders must be listed");
		expect(contains(names, "crt/preset.glslp"), "Nested GLSLP presets must be listed");
		expect(!contains(names, "crt/component.glsl"), "Nested GLSL components must not be listed");
		expect(!contains(names, "ignored.txt"), "Non-shader files must not be listed");

		create_file(root / "new.glslp");
		expect(!contains(get_available_shader_names(), "new.glslp"),
			"Catalog must stay cached until invalidated");
		invalidate_shader_catalog();
		expect(contains(get_available_shader_names(), "new.glslp"),
			"Invalidating the catalog must discover new presets");

		create_file(std::filesystem::path(alternate_root) / "alternate.glslp");
		shaders_path = alternate_root;
		expect(contains(get_available_shader_names(), "alternate.glslp"),
			"Changing the shader path must rescan automatically");
		expect(!contains(get_available_shader_names(), "custom.glsl"),
			"Changing the shader path must discard the previous catalog");
	}
	catch (const std::exception& error) {
		std::cerr << error.what() << '\n';
		failures++;
	}

	std::filesystem::remove_all(root);
	std::filesystem::remove_all(alternate_root);
	return failures == 0 ? 0 : 1;
}
