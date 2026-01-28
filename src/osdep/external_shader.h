#ifndef EXTERNAL_SHADER_H
#define EXTERNAL_SHADER_H

#include "gl_platform.h"
#include <string>
#include <vector>
#include <map>

// Shader parameter structure for #pragma parameter support
struct ShaderParameter {
	std::string name;
	std::string description;
	float default_value;
	float min_value;
	float max_value;
	float step;
	float current_value;
};

// External shader class
class ExternalShader {
public:
	ExternalShader();
	~ExternalShader();

	// Load shader from file
	bool load_from_file(const char* filepath);
	
	// Reload shader (for hot-reload support)
	bool reload();
	
	// Use this shader for rendering
	void use();
	
	// Set standard uniforms
	void set_texture_size(float width, float height);
	void set_input_size(float width, float height);
	void set_output_size(float width, float height);
	void set_frame_count(int count);
	void set_mvp_matrix(const float* matrix);
	
	// Bind texture to shader
	void bind_texture(GLuint texture_id, int texture_unit = 0);
	
	// Get shader parameters
	const std::vector<ShaderParameter>& get_parameters() const { return parameters_; }
	
	// Set parameter value
	bool set_parameter(const std::string& name, float value);
	
	// Check if shader is valid
	bool is_valid() const { return program_ != 0; }
	
	// Get program handle
	GLuint get_program() const { return program_; }

	// Get error message
	const std::string& get_error() const { return error_message_; }

	// Getters for input resources
	GLuint get_input_texture() const { return input_texture_; }
	void set_input_texture(GLuint tex) { input_texture_ = tex; }
	GLuint get_input_vbo() const { return input_vbo_; }
	void set_input_vbo(GLuint vbo) { input_vbo_ = vbo; }
	GLuint get_input_vao() const { return input_vao_; }
	void set_input_vao(GLuint vao) { input_vao_ = vao; }

private:
	// Shader compilation and linking
	bool compile_shader(const char* source, GLenum shader_type, GLuint& shader_id);
	bool link_program(GLuint vertex_shader, GLuint fragment_shader);
	
	// Shader parsing
	bool parse_shader_file(const std::string& source);
	std::string extract_vertex_shader(const std::string& source);
	std::string extract_fragment_shader(const std::string& source);
	void parse_pragma_parameters(const std::string& source);
	
	// Uniform location caching
	GLint get_uniform_location(const char* name);
	
	// Cleanup
	void cleanup();

	GLuint program_;
	GLuint vertex_shader_;
	GLuint fragment_shader_;
	
	GLuint input_texture_;
	GLuint input_vbo_;
	GLuint input_vao_;
	
	std::string filepath_;
	std::string shader_source_;
	std::string error_message_;
	
	std::vector<ShaderParameter> parameters_;
	std::map<std::string, GLint> uniform_locations_;
	
	// Standard uniform locations (cached)
	GLint u_texture_;
	GLint u_texture_size_;
	GLint u_input_size_;
	GLint u_output_size_;
	GLint u_frame_count_;
	GLint u_mvp_matrix_;
};

// Global shader management functions
ExternalShader* create_external_shader(const char* shader_name);
void destroy_external_shader(ExternalShader* shader);

#endif // EXTERNAL_SHADER_H
