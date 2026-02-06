#ifndef SHADER_PRESET_H
#define SHADER_PRESET_H

#ifdef USE_OPENGL

#include "gl_platform.h"
#include "external_shader.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Scale type for a pass dimension
enum class ScaleType {
	Source,     // relative to input (previous pass output)
	Viewport,  // relative to final viewport
	Absolute   // fixed pixel dimensions
};

// Wrap mode for textures
enum class WrapMode {
	ClampToEdge,
	Repeat,
	ClampToBorder,
	MirroredRepeat
};

// Per-pass configuration parsed from .glslp
struct ShaderPassConfig {
	std::string shader_path;       // resolved absolute path to .glsl file
	std::string alias;             // optional alias name (e.g., "BloomPass")
	bool filter_linear = false;
	ScaleType scale_type_x = ScaleType::Source;
	ScaleType scale_type_y = ScaleType::Source;
	float scale_x = 1.0f;
	float scale_y = 1.0f;
	bool srgb_framebuffer = false;
	bool float_framebuffer = false;
	bool mipmap_input = false;
	WrapMode wrap_mode = WrapMode::ClampToEdge;
	int frame_count_mod = 0;       // 0 = no modulo
};

// LUT texture configuration
struct LutTextureConfig {
	std::string name;              // uniform sampler name (e.g., "SamplerLUT1")
	std::string path;              // resolved path to image file
	bool linear = false;
	bool mipmap = false;
	WrapMode wrap_mode = WrapMode::ClampToEdge;
};

// Runtime state for a single pass
struct ShaderPass {
	ShaderPassConfig config;
	std::unique_ptr<ExternalShader> shader;
	GLuint fbo = 0;
	GLuint output_texture = 0;
	int output_width = 0;
	int output_height = 0;
};

// Runtime state for a LUT texture
struct LutTexture {
	LutTextureConfig config;
	GLuint texture_id = 0;
	int width = 0;
	int height = 0;
};

class ShaderPreset {
public:
	ShaderPreset();
	~ShaderPreset();

	// Load preset from .glslp file
	bool load_from_file(const char* preset_path);

	// Check if loaded and valid
	bool is_valid() const { return valid_; }

	// Render the full multi-pass pipeline
	void render(const unsigned char* pixels, int width, int height, int pitch,
		int viewport_x, int viewport_y, int viewport_w, int viewport_h,
		int frame_count);

	// Get all parameters from all passes (for GUI)
	std::vector<ShaderParameter>& get_all_parameters() { return all_parameters_; }

	// Set parameter by name (searches all passes)
	bool set_parameter(const std::string& name, float value);

	// Reload all shaders
	bool reload();

	// Get error message
	const std::string& get_error() const { return error_message_; }

	// Get number of passes
	int get_pass_count() const { return static_cast<int>(passes_.size()); }

private:
	// Parsing
	bool parse_preset_file(const std::string& filepath, std::map<std::string, std::string>& values);
	bool build_from_values(const std::map<std::string, std::string>& values, const std::string& base_dir);
	std::string resolve_path(const std::string& relative_path, const std::string& base_dir);
	static std::string strip_quotes(const std::string& value);
	static std::string trim(const std::string& s);
	static ScaleType parse_scale_type(const std::string& value);
	static WrapMode parse_wrap_mode(const std::string& value);
	static GLenum wrap_mode_to_gl(WrapMode mode);

	// GL resource management
	bool create_pass_fbo(int pass_index, int input_w, int input_h,
		int viewport_w, int viewport_h);
	void calculate_pass_output_size(const ShaderPassConfig& config,
		int input_w, int input_h,
		int viewport_w, int viewport_h,
		int& out_w, int& out_h);
	bool load_lut_texture(LutTexture& lut);
	void setup_shared_vertex_data();
	void cleanup();

	// Uniform and texture binding for a pass
	void bind_pass_uniforms(int pass_index, int input_w, int input_h,
		int output_w, int output_h, int viewport_w, int viewport_h,
		int frame_count);
	void bind_pass_textures(int pass_index);

	std::vector<ShaderPass> passes_;
	std::vector<LutTexture> lut_textures_;
	std::vector<ShaderParameter> all_parameters_;

	// Original input texture (Amiga frame)
	GLuint original_texture_ = 0;
	int original_width_ = 0;
	int original_height_ = 0;

	// Shared vertex resources
	GLuint shared_vbo_ = 0;
	GLuint shared_vao_ = 0;

	// Buffer for flipping pixel rows during texture upload
	std::vector<unsigned char> flip_buffer_;

	std::string preset_path_;
	std::string base_dir_;
	std::string error_message_;
	bool valid_ = false;
};

// Global management functions
ShaderPreset* create_shader_preset(const char* preset_name);
void destroy_shader_preset(ShaderPreset* preset);

#endif // USE_OPENGL

#endif // SHADER_PRESET_H
