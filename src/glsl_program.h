#pragma once
#include "gl/funcs.h"

#define MX_MAX_SHADER_ATTRIBS 8
#define MX_MAX_SHADER_ATTRIB_NAME 8
#define MX_MAX_SHADER_UNIFORMS 20
#define MX_MAX_SHADER_UNIFORM_NAME 32

class mx_GLSLProgram
{
public:
	mx_GLSLProgram();
	~mx_GLSLProgram();

	void SetAttribLocation( const char* attrib_name, unsigned int attrib );

	void Create( const char* vertex_shader, const char* fragment_shader= NULL, const char* geometry_shader= NULL );

	void Bind();

	unsigned int GetUniformBlockIndex( const char* name );
	void UniformBlockBinding( unsigned int uniform_block_index, unsigned int binding );

	void FindUniform( const char* name );
	void FindUniforms( const char* const* names, unsigned int count );

	void UniformInt ( const char* name, int i );
	void UniformIntArray ( const char* name, unsigned int count, const int* i );
	void UniformMat4( const char* name, const float* mat );
	void UniformMat3( const char* name, const float* mat );
	void UniformMat4Array( const char* name, unsigned int count, const float* mat );
	void UniformMat3Array( const char* name, unsigned int count, const float* mat );
	void UniformVec3 ( const char* name, const float* v );
	void UniformVec3Array( const char* name, unsigned int count, const float* v );
	void UniformVec4 ( const char* name, const float* v );
	void UniformVec4Array( const char* name, unsigned int count, const float* v );
	void UniformFloat( const char* name, float f );
	void UniformFloatArray( const char* name, unsigned int count, const float* f );

private:
	GLint GetUniformId( const char* name );

private:
	GLuint program_id_;

	GLuint attribs_[ MX_MAX_SHADER_ATTRIBS ];
	char attrib_names_[ MX_MAX_SHADER_ATTRIBS ][ MX_MAX_SHADER_ATTRIB_NAME + 1 ];
	unsigned int attrib_count_;

	GLint uniforms_[ MX_MAX_SHADER_UNIFORMS ];
	char uniform_names_[ MX_MAX_SHADER_UNIFORMS ][ MX_MAX_SHADER_UNIFORM_NAME + 1 ];
	unsigned int uniform_count_;
};

inline void mx_GLSLProgram::Bind()
{
	glUseProgram( program_id_ );
}
