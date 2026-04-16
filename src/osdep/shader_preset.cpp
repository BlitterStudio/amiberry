#ifdef USE_OPENGL
#include "shader_preset.h"
#include "sysdeps.h"
#include "uae.h"
#include <SDL3_image/SDL_image.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>

#ifndef GL_BGRA
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#else
#define GL_BGRA 0x80E1
#endif
#endif

extern SDL_PixelFormat pixel_format;

ShaderPreset::ShaderPreset() = default;

ShaderPreset::~ShaderPreset()
{
	cleanup();
}

void ShaderPreset::cleanup()
{
	for (auto& pass : passes_) {
		if (pass.fbo != 0) {
			glDeleteFramebuffers(1, &pass.fbo);
			pass.fbo = 0;
		}
		if (pass.output_texture != 0) {
			if (glIsTexture(pass.output_texture))
				glDeleteTextures(1, &pass.output_texture);
			pass.output_texture = 0;
		}
		pass.shader.reset();
	}
	passes_.clear();

	for (auto& lut : lut_textures_) {
		if (lut.texture_id != 0) {
			if (glIsTexture(lut.texture_id))
				glDeleteTextures(1, &lut.texture_id);
			lut.texture_id = 0;
		}
	}
	lut_textures_.clear();

	if (source_texture_ != 0) {
		if (glIsTexture(source_texture_))
			glDeleteTextures(1, &source_texture_);
		source_texture_ = 0;
	}
	if (original_texture_ != 0) {
		if (glIsTexture(original_texture_))
			glDeleteTextures(1, &original_texture_);
		original_texture_ = 0;
	}
	if (flip_fbo_ != 0) {
		// Matches pass.fbo cleanup above — glIsFramebuffer is not loaded
		// through gl_platform's function pointer table, so skip the check.
		glDeleteFramebuffers(1, &flip_fbo_);
		flip_fbo_ = 0;
	}
	if (flip_program_ != 0) {
		if (glIsProgram(flip_program_))
			glDeleteProgram(flip_program_);
		flip_program_ = 0;
	}
	flip_u_source_ = -1;
	if (shared_vbo_ != 0) {
		if (glIsBuffer(shared_vbo_))
			glDeleteBuffers(1, &shared_vbo_);
		shared_vbo_ = 0;
	}
	if (shared_vao_ != 0) {
		if (glIsVertexArray(shared_vao_))
			glDeleteVertexArrays(1, &shared_vao_);
		shared_vao_ = 0;
	}

	all_parameters_.clear();
	valid_ = false;
}

// ---- String utilities ----

std::string ShaderPreset::trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

std::string ShaderPreset::strip_quotes(const std::string& value)
{
	std::string v = trim(value);
	if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
		return v.substr(1, v.size() - 2);
	}
	return v;
}

ScaleType ShaderPreset::parse_scale_type(const std::string& value)
{
	std::string v = strip_quotes(value);
	if (v == "viewport") return ScaleType::Viewport;
	if (v == "absolute") return ScaleType::Absolute;
	return ScaleType::Source;
}

WrapMode ShaderPreset::parse_wrap_mode(const std::string& value)
{
	std::string v = strip_quotes(value);
	if (v == "repeat") return WrapMode::Repeat;
	if (v == "clamp_to_border") return WrapMode::ClampToBorder;
	if (v == "mirrored_repeat") return WrapMode::MirroredRepeat;
	return WrapMode::ClampToEdge;
}

GLenum ShaderPreset::wrap_mode_to_gl(WrapMode mode)
{
	switch (mode) {
	case WrapMode::Repeat: return GL_REPEAT;
	case WrapMode::ClampToBorder: return GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_BORDER may not be available on GLES
	case WrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
	default: return GL_CLAMP_TO_EDGE;
	}
}

std::string ShaderPreset::resolve_path(const std::string& relative_path, const std::string& base_dir)
{
	namespace fs = std::filesystem;
	fs::path p = fs::path(base_dir) / relative_path;
	try {
		return fs::weakly_canonical(p).string();
	} catch (...) {
		return p.string();
	}
}

// ---- .glslp Parser ----

bool ShaderPreset::parse_preset_file(const std::string& filepath, std::map<std::string, std::string>& values)
{
	std::ifstream file(filepath);
	if (!file.is_open()) {
		error_message_ = "Failed to open preset file: " + filepath;
		write_log("%s\n", error_message_.c_str());
		return false;
	}

	namespace fs = std::filesystem;
	std::string base_dir = fs::path(filepath).parent_path().string();

	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty()) continue;

		// Handle #reference directive
		if (line.rfind("#reference", 0) == 0) {
			std::string ref_value = trim(line.substr(10));
			ref_value = strip_quotes(ref_value);
			std::string ref_path = resolve_path(ref_value, base_dir);
			// Parse referenced preset first (base values)
			std::map<std::string, std::string> ref_values;
			if (!parse_preset_file(ref_path, ref_values)) {
				write_log("Warning: Failed to parse referenced preset: %s\n", ref_path.c_str());
			} else {
				// Merge: referenced values are the base, current file overrides
				for (auto& kv : ref_values) {
					if (values.find(kv.first) == values.end()) {
						values[kv.first] = kv.second;
					}
				}
			}
			continue;
		}

		// Skip comments
		if (line[0] == '#' || (line.size() >= 2 && line[0] == '/' && line[1] == '/'))
			continue;

		// Find key = value
		auto eq_pos = line.find('=');
		if (eq_pos == std::string::npos) continue;

		std::string key = trim(line.substr(0, eq_pos));
		std::string val = line.substr(eq_pos + 1);

		// Strip inline comments (# after value, but not inside quotes)
		bool in_quotes = false;
		for (size_t i = 0; i < val.size(); i++) {
			if (val[i] == '"') in_quotes = !in_quotes;
			if (!in_quotes && val[i] == '#') {
				val = val.substr(0, i);
				break;
			}
		}

		val = strip_quotes(trim(val));

		if (!key.empty()) {
			values[key] = val;
		}
	}

	return true;
}

bool ShaderPreset::load_from_file(const char* preset_path_cstr)
{
	cleanup();

	preset_path_ = preset_path_cstr;
	namespace fs = std::filesystem;
	base_dir_ = fs::path(preset_path_).parent_path().string();

	// Parse the preset file
	std::map<std::string, std::string> values;
	if (!parse_preset_file(preset_path_, values)) {
		return false;
	}

	if (!build_from_values(values, base_dir_)) {
		return false;
	}

	// Aggregate parameters from all passes
	all_parameters_.clear();
	for (auto& pass : passes_) {
		if (pass.shader) {
			const auto& params = pass.shader->get_parameters();
			for (const auto& p : params) {
				all_parameters_.push_back(p);
			}
		}
	}

	// Apply parameter overrides from preset
	auto param_it = values.find("parameters");
	if (param_it != values.end()) {
		// parameters = "PARAM1;PARAM2;PARAM3"
		std::string params_list = param_it->second;
		std::stringstream ss(params_list);
		std::string param_name;
		while (std::getline(ss, param_name, ';')) {
			param_name = trim(param_name);
			if (param_name.empty()) continue;
			auto val_it = values.find(param_name);
			if (val_it != values.end()) {
				float val = std::stof(val_it->second);
				set_parameter(param_name, val);
			}
		}
	}

	valid_ = true;
	write_log("Shader preset loaded: %s (%d passes, %d LUT textures, %d parameters)\n",
		preset_path_cstr,
		static_cast<int>(passes_.size()),
		static_cast<int>(lut_textures_.size()),
		static_cast<int>(all_parameters_.size()));
	return true;
}

bool ShaderPreset::build_from_values(const std::map<std::string, std::string>& values, const std::string& base_dir)
{
	// Get pass count
	auto it = values.find("shaders");
	if (it == values.end()) {
		error_message_ = "No 'shaders' key found in preset";
		write_log("%s\n", error_message_.c_str());
		return false;
	}

	int pass_count = std::stoi(it->second);
	if (pass_count < 1 || pass_count > 26) {
		error_message_ = "Invalid pass count: " + std::to_string(pass_count);
		write_log("%s\n", error_message_.c_str());
		return false;
	}

	// Parse per-pass configuration
	passes_.resize(pass_count);
	for (int i = 0; i < pass_count; i++) {
		std::string idx = std::to_string(i);
		auto& config = passes_[i].config;

		// Shader path (required)
		auto shader_it = values.find("shader" + idx);
		if (shader_it == values.end()) {
			error_message_ = "Missing shader" + idx + " in preset";
			write_log("%s\n", error_message_.c_str());
			return false;
		}
		config.shader_path = resolve_path(shader_it->second, base_dir);

		// Alias
		auto alias_it = values.find("alias" + idx);
		if (alias_it != values.end()) {
			config.alias = alias_it->second;
		}

		// Filter
		auto filter_it = values.find("filter_linear" + idx);
		if (filter_it != values.end()) {
			config.filter_linear = (filter_it->second == "true" || filter_it->second == "1");
		}

		// Scale type (unified or per-axis)
		auto st_it = values.find("scale_type" + idx);
		if (st_it != values.end()) {
			config.scale_type_x = parse_scale_type(st_it->second);
			config.scale_type_y = config.scale_type_x;
		}
		auto stx_it = values.find("scale_type_x" + idx);
		if (stx_it != values.end()) {
			config.scale_type_x = parse_scale_type(stx_it->second);
		}
		auto sty_it = values.find("scale_type_y" + idx);
		if (sty_it != values.end()) {
			config.scale_type_y = parse_scale_type(sty_it->second);
		}

		// Scale factor (unified or per-axis)
		auto s_it = values.find("scale" + idx);
		if (s_it != values.end()) {
			config.scale_x = std::stof(s_it->second);
			config.scale_y = config.scale_x;
		}
		auto sx_it = values.find("scale_x" + idx);
		if (sx_it != values.end()) {
			config.scale_x = std::stof(sx_it->second);
		}
		auto sy_it = values.find("scale_y" + idx);
		if (sy_it != values.end()) {
			config.scale_y = std::stof(sy_it->second);
		}

		// Framebuffer format
		auto srgb_it = values.find("srgb_framebuffer" + idx);
		if (srgb_it != values.end()) {
			config.srgb_framebuffer = (srgb_it->second == "true" || srgb_it->second == "1");
		}
		auto float_it = values.find("float_framebuffer" + idx);
		if (float_it != values.end()) {
			config.float_framebuffer = (float_it->second == "true" || float_it->second == "1");
		}

		// Mipmap input
		auto mipmap_it = values.find("mipmap_input" + idx);
		if (mipmap_it != values.end()) {
			config.mipmap_input = (mipmap_it->second == "true" || mipmap_it->second == "1");
		}

		// Wrap mode
		auto wrap_it = values.find("wrap_mode" + idx);
		if (wrap_it != values.end()) {
			config.wrap_mode = parse_wrap_mode(wrap_it->second);
		}
		auto twrap_it = values.find("texture_wrap_mode" + idx);
		if (twrap_it != values.end()) {
			config.wrap_mode = parse_wrap_mode(twrap_it->second);
		}

		// Frame count mod
		auto fcm_it = values.find("frame_count_mod" + idx);
		if (fcm_it != values.end()) {
			config.frame_count_mod = std::stoi(fcm_it->second);
		}

		// Compile the shader for this pass
		passes_[i].shader = std::make_unique<ExternalShader>();
		if (!passes_[i].shader->load_from_file(config.shader_path.c_str())) {
			error_message_ = "Failed to compile shader for pass " + idx + ": " + config.shader_path
				+ "\n" + passes_[i].shader->get_error();
			write_log("%s\n", error_message_.c_str());
			return false;
		}
		write_log("  Pass %d: %s%s\n", i, config.shader_path.c_str(),
			config.alias.empty() ? "" : (" (alias: " + config.alias + ")").c_str());
	}

	// Parse LUT textures
	auto tex_it = values.find("textures");
	if (tex_it != values.end() && !tex_it->second.empty()) {
		std::stringstream ss(tex_it->second);
		std::string tex_name;
		while (std::getline(ss, tex_name, ';')) {
			tex_name = trim(tex_name);
			if (tex_name.empty()) continue;

			LutTexture lut;
			lut.config.name = tex_name;

			// Path
			auto path_it = values.find(tex_name);
			if (path_it == values.end()) {
				write_log("Warning: LUT texture '%s' declared but no path found\n", tex_name.c_str());
				continue;
			}
			lut.config.path = resolve_path(path_it->second, base_dir);

			// Linear filtering
			auto lin_it = values.find(tex_name + "_linear");
			if (lin_it != values.end()) {
				lut.config.linear = (lin_it->second == "true" || lin_it->second == "1");
			}

			// Mipmap
			auto mip_it = values.find(tex_name + "_mipmap");
			if (mip_it != values.end()) {
				lut.config.mipmap = (mip_it->second == "true" || mip_it->second == "1");
			}

			// Wrap mode
			auto wrap_it2 = values.find(tex_name + "_wrap_mode");
			if (wrap_it2 != values.end()) {
				lut.config.wrap_mode = parse_wrap_mode(wrap_it2->second);
			}

			// Load the texture
			if (load_lut_texture(lut)) {
				lut_textures_.push_back(std::move(lut));
			} else {
				write_log("Warning: Failed to load LUT texture: %s\n", lut.config.path.c_str());
			}
		}
	}

	return true;
}

bool ShaderPreset::load_lut_texture(LutTexture& lut)
{
	SDL_Surface* surface = IMG_Load(lut.config.path.c_str());
	if (!surface) {
		write_log("IMG_Load failed for %s: %s\n", lut.config.path.c_str(), SDL_GetError());
		return false;
	}

	// Convert to RGBA32
	SDL_Surface* rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
	SDL_DestroySurface(surface);
	if (!rgba) {
		write_log("SDL_ConvertSurface failed for LUT texture\n");
		return false;
	}

	glGenTextures(1, &lut.texture_id);
	glBindTexture(GL_TEXTURE_2D, lut.texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);

	lut.width = rgba->w;
	lut.height = rgba->h;
	SDL_DestroySurface(rgba);

	// Filtering
	GLenum min_filter = lut.config.linear ? GL_LINEAR : GL_NEAREST;
	GLenum mag_filter = min_filter;
	if (lut.config.mipmap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		min_filter = lut.config.linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	// Wrap mode
	GLenum wrap = wrap_mode_to_gl(lut.config.wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

	write_log("  LUT loaded: %s (%dx%d, %s, %s)\n",
		lut.config.name.c_str(), lut.width, lut.height,
		lut.config.linear ? "linear" : "nearest",
		lut.config.mipmap ? "mipmapped" : "no mipmap");

	return true;
}

// ---- FBO Management ----

void ShaderPreset::calculate_pass_output_size(const ShaderPassConfig& config,
	int input_w, int input_h, int viewport_w, int viewport_h,
	int& out_w, int& out_h)
{
	switch (config.scale_type_x) {
	case ScaleType::Source:
		out_w = static_cast<int>(input_w * config.scale_x);
		break;
	case ScaleType::Viewport:
		out_w = static_cast<int>(viewport_w * config.scale_x);
		break;
	case ScaleType::Absolute:
		out_w = static_cast<int>(config.scale_x);
		break;
	}
	switch (config.scale_type_y) {
	case ScaleType::Source:
		out_h = static_cast<int>(input_h * config.scale_y);
		break;
	case ScaleType::Viewport:
		out_h = static_cast<int>(viewport_h * config.scale_y);
		break;
	case ScaleType::Absolute:
		out_h = static_cast<int>(config.scale_y);
		break;
	}
	out_w = std::max(1, out_w);
	out_h = std::max(1, out_h);
}

bool ShaderPreset::create_pass_fbo(int pass_index, int input_w, int input_h,
	int viewport_w, int viewport_h)
{
	auto& pass = passes_[pass_index];
	bool is_last = (pass_index == static_cast<int>(passes_.size()) - 1);

	if (is_last) {
		// Last pass renders to screen
		pass.output_width = viewport_w;
		pass.output_height = viewport_h;
		return true;
	}

	int out_w, out_h;
	calculate_pass_output_size(pass.config, input_w, input_h, viewport_w, viewport_h, out_w, out_h);

	// Check if FBO already exists with correct size
	if (pass.fbo != 0 && pass.output_width == out_w && pass.output_height == out_h) {
		return true;
	}

	// Cleanup old resources
	if (pass.fbo != 0) {
		glDeleteFramebuffers(1, &pass.fbo);
		pass.fbo = 0;
	}
	if (pass.output_texture != 0) {
		glDeleteTextures(1, &pass.output_texture);
		pass.output_texture = 0;
	}

	// Create output texture
	glGenTextures(1, &pass.output_texture);
	glBindTexture(GL_TEXTURE_2D, pass.output_texture);

	// Choose internal format
	GLenum internal_format = GL_RGBA;
	if (pass.config.float_framebuffer)
		internal_format = GL_RGBA16F;
	else if (pass.config.srgb_framebuffer)
		internal_format = GL_SRGB8_ALPHA8;

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, out_w, out_h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Filtering
	GLenum filter = pass.config.filter_linear ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	// Wrap mode
	GLenum wrap = wrap_mode_to_gl(pass.config.wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

	// Create FBO
	glGenFramebuffers(1, &pass.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, pass.output_texture, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		error_message_ = "FBO incomplete for pass " + std::to_string(pass_index)
			+ " (status: " + std::to_string(status) + ", size: "
			+ std::to_string(out_w) + "x" + std::to_string(out_h) + ")";
		write_log("%s\n", error_message_.c_str());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	pass.output_width = out_w;
	pass.output_height = out_h;
	return true;
}

// ---- Shared Vertex Data ----

void ShaderPreset::setup_shared_vertex_data()
{
	if (shared_vao_ != 0) return;

	glGenVertexArrays(1, &shared_vao_);
	glGenBuffers(1, &shared_vbo_);

	glBindVertexArray(shared_vao_);
	glBindBuffer(GL_ARRAY_BUFFER, shared_vbo_);

	// RetroArch convention: TexCoord (0,0) at bottom-left, (1,1) at top-right.
	// The SDL surface arrives top-down; an internal Y-flip pre-pass (see
	// run_flip_pass) writes the properly-oriented image into original_texture_
	// so these texcoords produce the correct orientation for user passes.
	static const float vertices[] = {
		// VertexCoord (x,y,z,w), TexCoord (s,t,0,0)
		-1.0f, -1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 0.0f,  // Bottom-right
		 1.0f,  1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 0.0f,  // Top-right
		-1.0f,  1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 0.0f   // Top-left
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bake attribute configuration into the VAO so the per-pass loop only
	// needs to bind the VAO to render. Matches the attribute locations
	// assigned by ExternalShader (VertexCoord=0, Color=1, TexCoord=2).
	const GLsizei stride = 8 * sizeof(float);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	// Attribute 1 is a constant white — supplied via glVertexAttrib4f.
	glDisableVertexAttribArray(1);
	glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
		reinterpret_cast<void*>(4 * sizeof(float)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---- Y-flip pre-pass ----

// Minimal passthrough shader that samples the top-down source texture with
// Y-inverted coordinates, producing a GL-convention image in the destination
// FBO. Uses legacy-style `attribute`/`varying`/`gl_FragColor`/`texture2D` so
// the same source compiles on both GLSL 120 (no preamble) and the modern
// core/GLES preambles returned by get_gl_shader_preambles() (which `#define`
// these to their core equivalents).
namespace {

const char* k_flip_vs =
	"attribute vec4 VertexCoord;\n"
	"attribute vec2 TexCoord;\n"
	"varying vec2 vTex;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = VertexCoord;\n"
	"    vTex = TexCoord;\n"
	"}\n";

const char* k_flip_fs =
	"varying vec2 vTex;\n"
	"uniform sampler2D Source;\n"
	"void main()\n"
	"{\n"
	"    gl_FragColor = texture2D(Source, vec2(vTex.x, 1.0 - vTex.y));\n"
	"}\n";

} // namespace

bool ShaderPreset::ensure_flip_resources()
{
	if (flip_program_ != 0) return true;

	const auto& preambles = get_gl_shader_preambles();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char* vs_sources[] = { preambles.vs, k_flip_vs };
	glShaderSource(vs, 2, vs_sources, nullptr);
	glCompileShader(vs);
	GLint compiled = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char infoLog[512];
		glGetShaderInfoLog(vs, sizeof(infoLog), nullptr, infoLog);
		write_log("ShaderPreset flip VS compile error: %s\n", infoLog);
		glDeleteShader(vs);
		return false;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fs_sources[] = { preambles.fs, k_flip_fs };
	glShaderSource(fs, 2, fs_sources, nullptr);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char infoLog[512];
		glGetShaderInfoLog(fs, sizeof(infoLog), nullptr, infoLog);
		write_log("ShaderPreset flip FS compile error: %s\n", infoLog);
		glDeleteShader(vs);
		glDeleteShader(fs);
		return false;
	}

	flip_program_ = glCreateProgram();
	glAttachShader(flip_program_, vs);
	glAttachShader(flip_program_, fs);
	// Match the shared VAO's attribute locations.
	glBindAttribLocation(flip_program_, 0, "VertexCoord");
	glBindAttribLocation(flip_program_, 2, "TexCoord");
	glLinkProgram(flip_program_);

	GLint linked = 0;
	glGetProgramiv(flip_program_, GL_LINK_STATUS, &linked);
	glDeleteShader(vs);
	glDeleteShader(fs);
	if (!linked) {
		char infoLog[512];
		glGetProgramInfoLog(flip_program_, sizeof(infoLog), nullptr, infoLog);
		write_log("ShaderPreset flip program link error: %s\n", infoLog);
		glDeleteProgram(flip_program_);
		flip_program_ = 0;
		return false;
	}

	flip_u_source_ = glGetUniformLocation(flip_program_, "Source");

	if (flip_fbo_ == 0) {
		glGenFramebuffers(1, &flip_fbo_);
	}

	return flip_program_ != 0 && flip_fbo_ != 0;
}

bool ShaderPreset::run_flip_pass(int width, int height)
{
	if (!ensure_flip_resources()) return false;

	// Ensure destination (original_texture_) matches the current frame size.
	// flip_dest_w_/flip_dest_h_ are member fields — not function-local
	// statics — so that multiple ShaderPreset instances keep independent
	// caches, and recreating original_texture_ on the same instance still
	// triggers a fresh glTexImage2D + FBO attach.
	if (original_texture_ == 0) {
		glGenTextures(1, &original_texture_);
		glBindTexture(GL_TEXTURE_2D, original_texture_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		flip_dest_w_ = 0;
		flip_dest_h_ = 0;
	}
	if (width != flip_dest_w_ || height != flip_dest_h_) {
		glBindTexture(GL_TEXTURE_2D, original_texture_);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		flip_dest_w_ = width;
		flip_dest_h_ = height;

		glBindFramebuffer(GL_FRAMEBUFFER, flip_fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, original_texture_, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			write_log("ShaderPreset flip FBO incomplete at %dx%d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, flip_fbo_);
	}

	glViewport(0, 0, width, height);
	// Flip pass never uses sRGB framebuffer encoding — the frame is already
	// in its target color space.
	glDisable(GL_FRAMEBUFFER_SRGB);

	glUseProgram(flip_program_);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source_texture_);
	if (flip_u_source_ >= 0) {
		glUniform1i(flip_u_source_, 0);
	}

	glBindVertexArray(shared_vao_);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

// ---- Uniform and Texture Binding ----

void ShaderPreset::bind_pass_textures(int pass_index)
{
	auto& pass = passes_[pass_index];
	auto* shader = pass.shader.get();
	int next_unit = 2; // 0 = Source, 1 = OrigTexture

	// Unit 0: Source/Texture (previous pass output or original)
	GLuint source_tex;
	if (pass_index == 0) {
		source_tex = original_texture_;
	} else {
		source_tex = passes_[pass_index - 1].output_texture;
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source_tex);
	// Apply this pass's filter_linear setting to the source texture
	GLenum src_filter = pass.config.filter_linear ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, src_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, src_filter);
	shader->set_uniform_int("Texture", 0);
	shader->set_uniform_int("Source", 0);

	// Unit 1: OrigTexture (always original Amiga frame)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, original_texture_);
	shader->set_uniform_int("OrigTexture", 1);
	shader->set_uniform_int("Original", 1);

	// Set original texture size uniforms
	shader->set_uniform_vec2("OrigTextureSize",
		static_cast<float>(original_width_), static_cast<float>(original_height_));
	shader->set_uniform_vec2("OrigInputSize",
		static_cast<float>(original_width_), static_cast<float>(original_height_));

	// Bind aliased pass outputs
	for (int j = 0; j < pass_index; j++) {
		if (passes_[j].config.alias.empty()) continue;
		if (passes_[j].output_texture == 0) continue;

		const std::string& alias = passes_[j].config.alias;

		// Check if this shader uses this alias (bare name or with Texture suffix)
		std::string alias_tex = alias + "Texture";
		GLint loc = shader->get_uniform_location(alias.c_str());
		GLint loc_tex = shader->get_uniform_location(alias_tex.c_str());
		if (loc < 0 && loc_tex < 0) continue;

		glActiveTexture(GL_TEXTURE0 + next_unit);
		glBindTexture(GL_TEXTURE_2D, passes_[j].output_texture);
		// Set both the bare alias and the Texture-suffixed variant
		shader->set_uniform_int(alias.c_str(), next_unit);
		shader->set_uniform_int(alias_tex.c_str(), next_unit);

		// Also set size uniforms for the alias
		shader->set_uniform_vec2((alias + "TextureSize").c_str(),
			static_cast<float>(passes_[j].output_width),
			static_cast<float>(passes_[j].output_height));
		shader->set_uniform_vec2((alias + "InputSize").c_str(),
			static_cast<float>(passes_[j].output_width),
			static_cast<float>(passes_[j].output_height));
		// Some shaders use aliasSize instead of aliasTextureSize
		shader->set_uniform_vec2((alias + "Size").c_str(),
			static_cast<float>(passes_[j].output_width),
			static_cast<float>(passes_[j].output_height));

		next_unit++;
	}

	// Bind PassPrevN textures (N-th previous pass from current)
	// PassPrev1 = immediate previous, PassPrev2 = 2 passes back, etc.
	for (int n = 1; n <= pass_index; n++) {
		int prev_idx = pass_index - n;
		std::string name = "PassPrev" + std::to_string(n);
		GLint loc = shader->get_uniform_location(name.c_str());
		if (loc < 0) {
			// Also try with "Texture" suffix
			loc = shader->get_uniform_location((name + "Texture").c_str());
		}
		if (loc < 0) continue;

		GLuint tex = (prev_idx == 0 && passes_[prev_idx].output_texture == 0)
			? original_texture_ : passes_[prev_idx].output_texture;

		glActiveTexture(GL_TEXTURE0 + next_unit);
		glBindTexture(GL_TEXTURE_2D, tex);
		shader->set_uniform_int(name.c_str(), next_unit);
		shader->set_uniform_int((name + "Texture").c_str(), next_unit);

		shader->set_uniform_vec2((name + "TextureSize").c_str(),
			static_cast<float>(passes_[prev_idx].output_width),
			static_cast<float>(passes_[prev_idx].output_height));
		shader->set_uniform_vec2((name + "InputSize").c_str(),
			static_cast<float>(passes_[prev_idx].output_width),
			static_cast<float>(passes_[prev_idx].output_height));

		next_unit++;
	}

	// Bind PassN textures (by absolute pass index)
	for (int j = 0; j < pass_index; j++) {
		std::string name = "Pass" + std::to_string(j);
		GLint loc = shader->get_uniform_location(name.c_str());
		if (loc < 0) {
			loc = shader->get_uniform_location((name + "Texture").c_str());
		}
		if (loc < 0) continue;

		glActiveTexture(GL_TEXTURE0 + next_unit);
		glBindTexture(GL_TEXTURE_2D, passes_[j].output_texture);
		shader->set_uniform_int(name.c_str(), next_unit);
		shader->set_uniform_int((name + "Texture").c_str(), next_unit);

		shader->set_uniform_vec2((name + "TextureSize").c_str(),
			static_cast<float>(passes_[j].output_width),
			static_cast<float>(passes_[j].output_height));

		next_unit++;
	}

	// Bind LUT textures
	for (auto& lut : lut_textures_) {
		GLint loc = shader->get_uniform_location(lut.config.name.c_str());
		if (loc < 0) continue;

		glActiveTexture(GL_TEXTURE0 + next_unit);
		glBindTexture(GL_TEXTURE_2D, lut.texture_id);
		shader->set_uniform_int(lut.config.name.c_str(), next_unit);

		// Set LUT size uniform if shader uses it
		shader->set_uniform_vec2((lut.config.name + "Size").c_str(),
			static_cast<float>(lut.width), static_cast<float>(lut.height));

		next_unit++;
	}
}

void ShaderPreset::bind_pass_uniforms(int pass_index, int input_w, int input_h,
	int output_w, int output_h, int viewport_w, int viewport_h,
	int frame_count)
{
	auto* shader = passes_[pass_index].shader.get();
	auto& config = passes_[pass_index].config;

	shader->set_texture_size(static_cast<float>(input_w), static_cast<float>(input_h));
	shader->set_input_size(static_cast<float>(input_w), static_cast<float>(input_h));
	shader->set_output_size(static_cast<float>(output_w), static_cast<float>(output_h));

	// FinalViewportSize - always the actual screen viewport, needed by complex shaders
	shader->set_uniform_vec2("FinalViewportSize",
		static_cast<float>(viewport_w), static_cast<float>(viewport_h));

	int fc = frame_count;
	if (config.frame_count_mod > 0) {
		fc = frame_count % config.frame_count_mod;
	}
	shader->set_frame_count(fc);

	// FrameDirection (always forward)
	shader->set_uniform_int("FrameDirection", 1);

	// MVP matrix: identity for all passes.
	// The original texture is uploaded with rows flipped to match GL convention,
	// so no Y-flip is needed in the MVP.
	float mvp[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	shader->set_mvp_matrix(mvp);
}

// ---- Multi-Pass Render ----

void ShaderPreset::render(const unsigned char* pixels, int width, int height, int pitch,
	int viewport_x, int viewport_y, int viewport_w, int viewport_h,
	int frame_count)
{
	if (!valid_ || passes_.empty()) return;
	if (!pixels) return;

	// Clear GL errors
	(void)glGetError();

	// Set up shared vertex data
	setup_shared_vertex_data();

	// Set up GL state
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);

	// Determine pixel format for the raw Amiga surface upload.
	GLenum gl_fmt = GL_RGBA;
	GLenum gl_type = GL_UNSIGNED_BYTE;
	int bpp = 4;
	if (pixel_format == SDL_PIXELFORMAT_ARGB8888) {
		gl_fmt = GL_BGRA;
		gl_type = GL_UNSIGNED_BYTE;
		bpp = 4;
	// Currently unreachable — update_pixel_format() always selects 32-bit host format
	} else if (pixel_format == SDL_PIXELFORMAT_RGB565) {
		gl_fmt = GL_RGB;
		gl_type = GL_UNSIGNED_SHORT_5_6_5;
		bpp = 2;
	} else if (pixel_format == SDL_PIXELFORMAT_XRGB1555) {
		gl_fmt = GL_RGBA;
		gl_type = GL_UNSIGNED_SHORT_5_5_5_1;
		bpp = 2;
	}

	// Upload the Amiga frame top-down (no CPU row reversal). The flip is
	// performed on the GPU via run_flip_pass() below, avoiding a per-frame
	// full-surface memcpy (~8 MB at 1080p × 32 bpp).
	if (source_texture_ == 0) {
		glGenTextures(1, &source_texture_);
		glBindTexture(GL_TEXTURE_2D, source_texture_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source_texture_);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / bpp);

	const bool size_changed = (width != original_width_ || height != original_height_);
	if (size_changed) {
		original_width_ = width;
		original_height_ = height;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			gl_fmt, gl_type, pixels);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
			gl_fmt, gl_type, pixels);
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	// Produce the correctly-oriented original_texture_ for user passes by
	// running the internal Y-flip pass. Must happen after source_texture_
	// is up-to-date and before any user pass reads OrigTexture.
	if (!run_flip_pass(width, height)) {
		// Flip pass failed (e.g. shader compile error); drop the frame
		// rather than render an upside-down image.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	// Render each pass
	int pass_count = static_cast<int>(passes_.size());
	for (int i = 0; i < pass_count; i++) {
		auto& pass = passes_[i];
		bool is_last = (i == pass_count - 1);

		// Calculate input dimensions for this pass
		int input_w, input_h;
		if (i == 0) {
			input_w = original_width_;
			input_h = original_height_;
		} else {
			input_w = passes_[i - 1].output_width;
			input_h = passes_[i - 1].output_height;
		}

		// Create/update FBO for this pass
		if (!create_pass_fbo(i, input_w, input_h, viewport_w, viewport_h)) {
			write_log("Failed to create FBO for pass %d, aborting render\n", i);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return;
		}

		int output_w = pass.output_width;
		int output_h = pass.output_height;

		// Bind render target
		if (is_last) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
			glDisable(GL_FRAMEBUFFER_SRGB);
		} else {
			glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);
			glViewport(0, 0, output_w, output_h);
			// Enable sRGB encoding when writing to sRGB framebuffers.
			// This ensures proper linear-to-sRGB conversion on write,
			// matching the sRGB-to-linear decode that happens on read.
			if (pass.config.srgb_framebuffer) {
				glEnable(GL_FRAMEBUFFER_SRGB);
			} else {
				glDisable(GL_FRAMEBUFFER_SRGB);
			}
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		// Use pass shader
		pass.shader->use();

		// Handle mipmap_input: generate mipmaps on the input texture
		if (pass.config.mipmap_input) {
			GLuint input_tex = (i == 0) ? original_texture_ : passes_[i - 1].output_texture;
			glBindTexture(GL_TEXTURE_2D, input_tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		// Bind textures and set uniforms
		bind_pass_textures(i);
		bind_pass_uniforms(i, input_w, input_h, output_w, output_h,
			viewport_w, viewport_h, frame_count);

		// Apply parameter uniforms from the emulator's GL context.
		// set_parameter() only stores values; the actual glUniform1f calls
		// must happen here in the render loop where the correct GL context is active.
		pass.shader->apply_parameter_uniforms();

		// Draw fullscreen quad. Attribute layout is baked into the VAO
		// (see setup_shared_vertex_data), so we only need to bind it.
		glBindVertexArray(shared_vao_);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	// Cleanup
	glDisable(GL_FRAMEBUFFER_SRGB);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ---- Parameter Management ----

bool ShaderPreset::set_parameter(const std::string& name, float value)
{
	bool found = false;
	// Update in aggregated list
	for (auto& param : all_parameters_) {
		if (param.name == name) {
			param.current_value = std::max(param.min_value, std::min(param.max_value, value));
			found = true;
			break;
		}
	}

	// Update in each pass's shader (stores value only, no GL calls).
	// The actual uniform update happens in render() via apply_parameter_uniforms(),
	// because set_parameter() is called from the GUI which runs in a different GL context.
	for (auto& pass : passes_) {
		if (pass.shader) {
			if (pass.shader->set_parameter(name, value)) {
				found = true;
			}
		}
	}
	return found;
}

bool ShaderPreset::reload()
{
	if (preset_path_.empty()) return false;
	write_log("Reloading shader preset: %s\n", preset_path_.c_str());
	return load_from_file(preset_path_.c_str());
}

// ---- Global Management ----

ShaderPreset* create_shader_preset(const char* preset_name)
{
	const std::string preset_path = get_shaders_path() + preset_name;

	write_log("Attempting to load shader preset: %s\n", preset_path.c_str());

	auto* preset = new ShaderPreset();
	if (!preset->load_from_file(preset_path.c_str())) {
		delete preset;
		return nullptr;
	}
	return preset;
}

void destroy_shader_preset(ShaderPreset* preset)
{
	delete preset;
}

#endif // USE_OPENGL
