#pragma once

// The display filters a configuration can name: the shaders (built-in and
// the presets in the shaders folder) and the bezel images in the bezels
// folder. Scanned on first use and again when the folder changes or after
// an invalidate. Main thread.

#include <string>
#include <vector>

// The built-in shaders first ("none" first of all), then the presets found.
const std::vector<std::string>& get_available_shader_names();
void invalidate_shader_catalog();

// "none" first, then the bezel images (PNG, JPEG) by relative path.
const std::vector<std::string>& get_available_bezel_names();
void invalidate_bezel_catalog();
