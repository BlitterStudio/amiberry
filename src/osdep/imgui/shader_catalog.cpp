#include "shader_catalog.h"

#include <algorithm>
#include <filesystem>
#include <iterator>

std::string get_shaders_path();

namespace {

constexpr const char* builtin_shaders[] = { "none", "tv", "pc", "lite", "1084" };

std::vector<std::string> shader_names;
std::string scanned_shaders_path;
bool shader_catalog_initialized = false;

void scan_shaders()
{
	shader_names.clear();
	for (const auto* builtin_shader : builtin_shaders)
		shader_names.emplace_back(builtin_shader);

	scanned_shaders_path = get_shaders_path();
	if (!scanned_shaders_path.empty()) {
		namespace fs = std::filesystem;
		const fs::path base_path(scanned_shaders_path);
		try {
			for (const auto& entry : fs::recursive_directory_iterator(base_path,
				fs::directory_options::skip_permission_denied)) {
				if (!entry.is_regular_file())
					continue;

				const std::string filename = entry.path().filename().string();
				const bool is_glslp = filename.size() > 6
					&& filename.substr(filename.size() - 6) == ".glslp";
				const bool is_glsl = !is_glslp && filename.size() > 5
					&& filename.substr(filename.size() - 5) == ".glsl";
				if (!is_glsl && !is_glslp)
					continue;

				const fs::path relative_path = fs::relative(entry.path(), base_path);
				if (is_glsl && !relative_path.parent_path().empty())
					continue;

				shader_names.push_back(relative_path.generic_string());
			}
		}
		catch (...) {
			// A missing or unreadable shader directory simply contributes no presets.
		}
	}

	std::sort(shader_names.begin() + std::size(builtin_shaders), shader_names.end());
	shader_names.erase(std::unique(shader_names.begin(), shader_names.end()), shader_names.end());
	shader_catalog_initialized = true;
}

} // namespace

const std::vector<std::string>& get_available_shader_names()
{
	if (!shader_catalog_initialized || scanned_shaders_path != get_shaders_path())
		scan_shaders();
	return shader_names;
}

void invalidate_shader_catalog()
{
	shader_catalog_initialized = false;
}
