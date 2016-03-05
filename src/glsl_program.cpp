#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "mx_assert.h"

#include "glsl_program.h"

#define MX_SHADER_OT_CREATED 0xFFFFFFFF

static void LoadShader( GLuint program, GLenum shader_type, const char* text )
{
	if( text == NULL ) return;

	GLuint shader= glCreateShader( shader_type );

	int len= std::strlen( text );
	glShaderSource( shader, 1, &text, &len );
	glCompileShader( shader );

#ifdef MX_DEBUG
	int compile_status;
	char build_log[1024];
	int build_log_len;

	glGetShaderiv( shader, GL_COMPILE_STATUS, &compile_status );
	if( !compile_status )
	{
		glGetShaderInfoLog( shader, sizeof(build_log)-1, &build_log_len, build_log );
		std::printf( "shader error:\n\n%s\nerrors:\n%s\n", text, build_log );
	}
#endif
	glAttachShader( program, shader );
}

mx_GLSLProgram::mx_GLSLProgram()
	: attrib_count_(0), uniform_count_(0), frag_out_attrib_count_(0)
	, program_id_(MX_SHADER_OT_CREATED)
{
}

mx_GLSLProgram::~mx_GLSLProgram()
{
	// By design, there is no resourse releasing.
}

void mx_GLSLProgram::SetAttribLocation( const char* attrib_name, unsigned int attrib )
{
	MX_ASSERT( program_id_ == MX_SHADER_OT_CREATED );
	MX_ASSERT( attrib_count_ < MX_MAX_SHADER_ATTRIBS );
	MX_ASSERT( std::strlen(attrib_name) <= MX_MAX_SHADER_ATTRIB_NAME );

	strcpy( attrib_names_[ attrib_count_ ], attrib_name );
	attribs_[ attrib_count_ ]= attrib;
	attrib_count_++;
}

void mx_GLSLProgram::SetFragDataLocation( const char* name, unsigned int index )
{
	MX_ASSERT( program_id_ == MX_SHADER_OT_CREATED );
	MX_ASSERT( frag_out_attrib_count_ < MX_MAX_SHADER_FRAG_OUT_ATTRIBS );
	MX_ASSERT( std::strlen(name) <= MX_MAX_SHADER_FRAG_OUT_NAME );

	strcpy( frag_out_attribs_names_[ frag_out_attrib_count_ ], name );
	frag_out_attribs_[ frag_out_attrib_count_ ]= index;
	frag_out_attrib_count_++;
}

void mx_GLSLProgram::Create( const char* vertex_shader, const char* fragment_shader, const char* geometry_shader )
{
	MX_ASSERT( program_id_ == MX_SHADER_OT_CREATED );

	program_id_= glCreateProgram();

	LoadShader( program_id_, GL_VERTEX_SHADER, vertex_shader );
	LoadShader( program_id_, GL_FRAGMENT_SHADER, fragment_shader );
	LoadShader( program_id_, GL_GEOMETRY_SHADER, geometry_shader );

	for( unsigned int i= 0; i< attrib_count_; i++ )
		glBindAttribLocation( program_id_, attribs_[i], attrib_names_[i] );

	for( unsigned int i= 0; i< frag_out_attrib_count_; i++ )
		glBindFragDataLocation( program_id_, frag_out_attribs_[i], frag_out_attribs_names_[i] );

	glLinkProgram( program_id_ );

#ifdef MX_DEBUG
	int compile_status;
	glGetProgramiv( program_id_, GL_LINK_STATUS, &compile_status );
	if( ! compile_status )
	{
		char build_log[1024];
		int build_log_len;

		glGetProgramInfoLog( program_id_, sizeof(build_log)-1, &build_log_len, build_log );
		std::printf( "shader link error:\n %s\n", build_log );
	}
#endif
}
/*
unsigned int mx_GLSLProgram::GetUniformBlockIndex( const char* name )
{
	return glGetUniformBlockIndex( program_id_, name );
}

void mx_GLSLProgram::UniformBlockBinding( unsigned int uniform_block_index, unsigned int binding )
{
	glUniformBlockBinding( program_id_, uniform_block_index, binding );
}
*/
void mx_GLSLProgram::FindUniform( const char* uniform )
{
	MX_ASSERT( uniform_count_ < MX_MAX_SHADER_UNIFORMS );
	MX_ASSERT( std::strlen(uniform) <= MX_MAX_SHADER_UNIFORM_NAME );

	uniforms_[ uniform_count_ ]= glGetUniformLocation( program_id_, uniform );
	strcpy( uniform_names_[uniform_count_], uniform );
	uniform_count_++;
}

void mx_GLSLProgram::FindUniforms( const char* const* names, unsigned int count )
{
	for( unsigned int i= 0; i< count; i++ )
		FindUniform( names[i] );
}

void mx_GLSLProgram::UniformInt( const char* name, int i )
{
	glUniform1i( GetUniformId(name), i );
}

void mx_GLSLProgram::UniformIntArray( const char* name, unsigned int count, const int* i )
{
	int id= GetUniformId(name);
	for( unsigned int j= 0; j< count; j++ )
		glUniform1i( id + j, i[j] );
}

void mx_GLSLProgram::UniformMat4( const char* name, const float* mat )
{
	glUniformMatrix4fv( GetUniformId(name), 1, GL_FALSE, mat );
}

void mx_GLSLProgram::UniformMat3( const char* name, const float* mat )
{
	glUniformMatrix3fv( GetUniformId(name), 1, GL_FALSE, mat );
}

void mx_GLSLProgram::UniformVec3( const char* name, const float* v )
{
	glUniform3f( GetUniformId(name), v[0], v[1], v[2] );
}

void mx_GLSLProgram::UniformVec3Array( const char* name, unsigned int count, const float* v )
{
	GLint id= GetUniformId(name);
	for( unsigned int i= 0; i< count; i++ )
		glUniform3f( i + id, v[0+i*3], v[1+i*3], v[2+i*3] );
}

void mx_GLSLProgram::UniformVec4 ( const char* name, const float* v )
{
	glUniform4f( GetUniformId(name), v[0], v[1], v[2], v[3] );
}

void mx_GLSLProgram::UniformVec4Array( const char* name, unsigned int count, const float* v )
{
	GLint id= GetUniformId(name);
	for( unsigned int i= 0; i< count; i++ )
		glUniform4f( i + id, v[0+i*4], v[1+i*4], v[2+i*4], v[3+i*4] );
}

void mx_GLSLProgram::UniformMat4Array( const char* name, unsigned int count, const float* mat )
{
	glUniformMatrix4fv( GetUniformId(name), count, GL_FALSE, mat );
}

void mx_GLSLProgram::UniformMat3Array( const char* name, unsigned int count, const float* mat )
{
	glUniformMatrix3fv( GetUniformId(name), count, GL_FALSE, mat );
}

void mx_GLSLProgram::UniformFloat( const char* name, float f )
{
	glUniform1f( GetUniformId(name), f );
}

void mx_GLSLProgram::UniformFloatArray( const char* name, unsigned int count, const float* f )
{
	glUniform1fv( GetUniformId(name), count, f );
}

GLint mx_GLSLProgram::GetUniformId( const char* name )
{
	for( unsigned int i= 0; i< uniform_count_; i++ )
	{
		if( std::strcmp( uniform_names_[i], name ) == 0 )
			return uniforms_[i];
	}

	MX_ASSERT(false);
	return -1;
}

