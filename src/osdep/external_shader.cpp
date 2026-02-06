#ifdef USE_OPENGL
#include "external_shader.h"
#include "sysdeps.h"
#include "uae.h"
#include "zfile.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <cstring>

// Constructor
ExternalShader::ExternalShader()
	: program_(0)
	, vertex_shader_(0)
	, fragment_shader_(0)
	, input_texture_(0)
	, input_vbo_(0)
	, input_vao_(0)
	, u_texture_(-1)
	, u_texture_size_(-1)
	, u_input_size_(-1)
	, u_output_size_(-1)
	, u_frame_count_(-1)
	, u_mvp_matrix_(-1)
{
}

// Destructor
ExternalShader::~ExternalShader()
{
	cleanup();
}

// Cleanup OpenGL resources
void ExternalShader::cleanup()
{
	if (vertex_shader_ != 0) {
		glDeleteShader(vertex_shader_);
		vertex_shader_ = 0;
	}
	if (fragment_shader_ != 0) {
		glDeleteShader(fragment_shader_);
		fragment_shader_ = 0;
	}
	if (program_ != 0) {
		glDeleteProgram(program_);
		program_ = 0;
	}
	if (input_texture_ != 0) {
		if (glIsTexture(input_texture_))
			glDeleteTextures(1, &input_texture_);
		input_texture_ = 0;
	}
	if (input_vbo_ != 0) {
		if (glIsBuffer(input_vbo_))
			glDeleteBuffers(1, &input_vbo_);
		input_vbo_ = 0;
	}
	if (input_vao_ != 0) {
		if (glIsVertexArray(input_vao_))
			glDeleteVertexArrays(1, &input_vao_);
		input_vao_ = 0;
	}
	uniform_locations_.clear();
}

// Load shader from file
bool ExternalShader::load_from_file(const char* filepath)
{
	filepath_ = filepath;
	
	// Check if file exists
	if (!zfile_exists(filepath)) {
		error_message_ = std::string("Shader file not found: ") + filepath;
		write_log("%s\n", error_message_.c_str());
		return false;
	}
	
	// Read file contents
	std::ifstream file(filepath);
	if (!file.is_open()) {
		error_message_ = std::string("Failed to open shader file: ") + filepath;
		write_log("%s\n", error_message_.c_str());
		return false;
	}
	
	std::stringstream buffer;
	buffer << file.rdbuf();
	shader_source_ = buffer.str();
	file.close();
	
	// Parse the shader
	if (!parse_shader_file(shader_source_)) {
		return false;
	}
	
	write_log("External shader loaded successfully: %s\n", filepath);
	return true;
}

// Reload shader (for hot-reload)
bool ExternalShader::reload()
{
	if (filepath_.empty()) {
		error_message_ = "Cannot reload: no filepath set";
		return false;
	}
	
	write_log("Reloading shader: %s\n", filepath_.c_str());
	cleanup();
	return load_from_file(filepath_.c_str());
}

// Parse shader file
bool ExternalShader::parse_shader_file(const std::string& source)
{
	// Parse #pragma parameters first
	parse_pragma_parameters(source);
	
	// Extract vertex and fragment shaders
	std::string vs_source = extract_vertex_shader(source);
	std::string fs_source = extract_fragment_shader(source);
	
	if (vs_source.empty() || fs_source.empty()) {
		error_message_ = "Failed to extract vertex or fragment shader sections";
		write_log("%s\n", error_message_.c_str());
		return false;
	}
	
	// Compile shaders
	if (!compile_shader(vs_source.c_str(), GL_VERTEX_SHADER, vertex_shader_)) {
		return false;
	}
	
	if (!compile_shader(fs_source.c_str(), GL_FRAGMENT_SHADER, fragment_shader_)) {
		glDeleteShader(vertex_shader_);
		vertex_shader_ = 0;
		return false;
	}
	
	// Link program
	if (!link_program(vertex_shader_, fragment_shader_)) {
		return false;
	}
	
	// Cache standard uniform locations
	u_texture_ = get_uniform_location("Texture");
	u_texture_size_ = get_uniform_location("TextureSize");
	u_input_size_ = get_uniform_location("InputSize");
	u_output_size_ = get_uniform_location("OutputSize");
	u_frame_count_ = get_uniform_location("FrameCount");
	u_mvp_matrix_ = get_uniform_location("MVPMatrix");
	
	// Set parameter uniforms to their default values
	use();
	for (const auto& param : parameters_) {
		GLint loc = get_uniform_location(param.name.c_str());
		if (loc >= 0) {
			glUniform1f(loc, param.current_value);
		}
	}
	
	return true;
}

// Extract vertex shader from combined source
std::string ExternalShader::extract_vertex_shader(const std::string& source)
{
	// Only inject PARAMETER_UNIFORM if #pragma parameter directives were found.
	// Without this, shaders that use #ifdef PARAMETER_UNIFORM / #else blocks
	// (like crt-royale) would declare uniforms that never get set, defaulting
	// to 0.0 and causing black screen due to division by zero / NaN.
	std::string defines = "#define VERTEX\n";
	if (!parameters_.empty()) {
		defines += "#define PARAMETER_UNIFORM\n";
	}

	// Check for #version and insert defines after it
	size_t version_pos = source.find("#version");
	if (version_pos != std::string::npos) {
		size_t newline_pos = source.find("\n", version_pos);
		if (newline_pos != std::string::npos) {
			std::string vs_source = source;
			vs_source.insert(newline_pos + 1, defines);
			return vs_source;
		}
	}
	return defines + source;
}

// Extract fragment shader from combined source
std::string ExternalShader::extract_fragment_shader(const std::string& source)
{
	std::string defines = "#define FRAGMENT\n";
	if (!parameters_.empty()) {
		defines += "#define PARAMETER_UNIFORM\n";
	}

	// Check for #version and insert defines after it
	size_t version_pos = source.find("#version");
	if (version_pos != std::string::npos) {
		size_t newline_pos = source.find("\n", version_pos);
		if (newline_pos != std::string::npos) {
			std::string fs_source = source;
			fs_source.insert(newline_pos + 1, defines);
			return fs_source;
		}
	}
	return defines + source;
}

// Parse #pragma parameter directives
void ExternalShader::parse_pragma_parameters(const std::string& source)
{
	parameters_.clear();
	
	// Regex to match: #pragma parameter NAME "Description" default min max step
	std::regex param_regex(R"(#pragma\s+parameter\s+(\w+)\s+\"([^\"]+)\"\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+))");
	
	std::sregex_iterator iter(source.begin(), source.end(), param_regex);
	std::sregex_iterator end;
	
	while (iter != end) {
		std::smatch match = *iter;
		ShaderParameter param;
		param.name = match[1].str();
		param.description = match[2].str();
		param.default_value = std::stof(match[3].str());
		param.min_value = std::stof(match[4].str());
		param.max_value = std::stof(match[5].str());
		param.step = std::stof(match[6].str());
		param.current_value = param.default_value;
		
		parameters_.push_back(param);
		++iter;
	}
}

// Compile shader
bool ExternalShader::compile_shader(const char* source, GLenum shader_type, GLuint& shader_id)
{
	shader_id = glCreateShader(shader_type);
	if (shader_id == 0) {
		error_message_ = "Failed to create shader object";
		write_log("%s\n", error_message_.c_str());
		return false;
	}
	
	glShaderSource(shader_id, 1, &source, nullptr);
	glCompileShader(shader_id);
	
	GLint compiled = 0;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
	
	if (!compiled) {
		GLint log_length = 0;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
		
		if (log_length > 0) {
			std::vector<char> log(log_length);
			glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
			error_message_ = std::string("Shader compilation failed:\n") + log.data();
		} else {
			error_message_ = "Shader compilation failed (no log available)";
		}
		
		write_log("%s\n", error_message_.c_str());
		glDeleteShader(shader_id);
		shader_id = 0;
		return false;
	}
	
	return true;
}

// Link program
bool ExternalShader::link_program(GLuint vertex_shader, GLuint fragment_shader)
{
	program_ = glCreateProgram();
	if (program_ == 0) {
		error_message_ = "Failed to create shader program";
		write_log("%s\n", error_message_.c_str());
		return false;
	}
	
	glAttachShader(program_, vertex_shader);
	glAttachShader(program_, fragment_shader);
	
	// Bind attribute locations (standard for RetroArch shaders)
	glBindAttribLocation(program_, 0, "VertexCoord");
	glBindAttribLocation(program_, 1, "Color");
	glBindAttribLocation(program_, 2, "TexCoord");
	
	glLinkProgram(program_);
	
	GLint linked = 0;
	glGetProgramiv(program_, GL_LINK_STATUS, &linked);
	
	if (!linked) {
		GLint log_length = 0;
		glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &log_length);
		
		if (log_length > 0) {
			std::vector<char> log(log_length);
			glGetProgramInfoLog(program_, log_length, nullptr, log.data());
			error_message_ = std::string("Shader linking failed:\n") + log.data();
		} else {
			error_message_ = "Shader linking failed (no log available)";
		}
		
		write_log("%s\n", error_message_.c_str());
		glDeleteProgram(program_);
		program_ = 0;
		return false;
	}
	
	return true;
}

// Get uniform location (with caching)
GLint ExternalShader::get_uniform_location(const char* name)
{
	auto it = uniform_locations_.find(name);
	if (it != uniform_locations_.end()) {
		return it->second;
	}
	
	GLint location = glGetUniformLocation(program_, name);
	uniform_locations_[name] = location;
	return location;
}

// Use this shader
void ExternalShader::use()
{
	if (program_ != 0) {
		glUseProgram(program_);
	}
}

// Set texture size uniform
void ExternalShader::set_texture_size(float width, float height)
{
	if (u_texture_size_ >= 0) {
		glUniform2f(u_texture_size_, width, height);
	}
}

// Set input size uniform
void ExternalShader::set_input_size(float width, float height)
{
	if (u_input_size_ >= 0) {
		glUniform2f(u_input_size_, width, height);
	}
}

// Set output size uniform
void ExternalShader::set_output_size(float width, float height)
{
	if (u_output_size_ >= 0) {
		glUniform2f(u_output_size_, width, height);
	}
}

// Set frame count uniform
void ExternalShader::set_frame_count(int count)
{
	if (u_frame_count_ >= 0) {
		glUniform1i(u_frame_count_, count);
	}
}

// Set MVP matrix uniform
void ExternalShader::set_mvp_matrix(const float* matrix)
{
	if (u_mvp_matrix_ >= 0) {
		glUniformMatrix4fv(u_mvp_matrix_, 1, GL_FALSE, matrix);
	}
}

// Bind texture
void ExternalShader::bind_texture(GLuint texture_id, int texture_unit)
{
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	
	if (u_texture_ >= 0) {
		glUniform1i(u_texture_, texture_unit);
	}
}

// Set an integer uniform by name
void ExternalShader::set_uniform_int(const char* name, int value)
{
	GLint loc = get_uniform_location(name);
	if (loc >= 0) {
		glUniform1i(loc, value);
	}
}

// Set a vec2 uniform by name
void ExternalShader::set_uniform_vec2(const char* name, float x, float y)
{
	GLint loc = get_uniform_location(name);
	if (loc >= 0) {
		glUniform2f(loc, x, y);
	}
}

// Set parameter value (stores value only, no GL calls)
// The actual uniform update happens in apply_parameter_uniforms() during the render loop,
// because set_parameter() may be called from the GUI which runs in a different GL context.
bool ExternalShader::set_parameter(const std::string& name, float value)
{
	for (auto& param : parameters_) {
		if (param.name == name) {
			// Clamp value to valid range
			param.current_value = std::max(param.min_value, std::min(param.max_value, value));
			return true;
		}
	}
	return false;
}

// Apply all parameter uniforms to the currently bound shader program.
// Must be called from the emulator's render loop (correct GL context) after use().
void ExternalShader::apply_parameter_uniforms()
{
	for (const auto& param : parameters_) {
		GLint loc = get_uniform_location(param.name.c_str());
		if (loc >= 0) {
			glUniform1f(loc, param.current_value);
		}
	}
}

// Global shader management functions
ExternalShader* create_external_shader(const char* shader_name)
{
	// Build full path: <data_path>/shaders/<shader_name>
	const std::string shader_path = get_shaders_path() + shader_name;
	
	write_log("Attempting to load external shader: %s\n", shader_path.c_str());
	
	auto* shader = new ExternalShader();
	if (!shader->load_from_file(shader_path.c_str())) {
		delete shader;
		return nullptr;
	}
	
	return shader;
}

void destroy_external_shader(ExternalShader* shader)
{
	delete shader;
}
#endif