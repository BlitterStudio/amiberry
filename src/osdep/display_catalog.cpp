#include "display_catalog.h"

#include <algorithm>
#include <filesystem>
#include <iterator>

std::string get_shaders_path();
std::string get_bezels_path();

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

namespace {

std::vector<std::string> bezel_names;
std::string scanned_bezels_path;
bool bezel_catalog_initialized = false;

void scan_bezels()
{
	bezel_names.clear();
	bezel_names.emplace_back("none");

	scanned_bezels_path = get_bezels_path();
	if (!scanned_bezels_path.empty()) {
		namespace fs = std::filesystem;
		const fs::path base_path(scanned_bezels_path);
		try {
			for (const auto& entry : fs::recursive_directory_iterator(base_path,
				fs::directory_options::skip_permission_denied)) {
				if (!entry.is_regular_file())
					continue;
				std::string ext = entry.path().extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
				if (ext != ".png" && ext != ".jpg" && ext != ".jpeg")
					continue;
				bezel_names.push_back(fs::relative(entry.path(), base_path).generic_string());
			}
		}
		catch (...) {
			// A missing or unreadable bezel directory simply contributes no images.
		}
	}

	std::sort(bezel_names.begin() + 1, bezel_names.end());
	bezel_catalog_initialized = true;
}

} // namespace

const std::vector<std::string>& get_available_bezel_names()
{
	if (!bezel_catalog_initialized || scanned_bezels_path != get_bezels_path())
		scan_bezels();
	return bezel_names;
}

void invalidate_bezel_catalog()
{
	bezel_catalog_initialized = false;
}
