#include "mx_assert.h"

#include "vertex_buffer.h"

#define MX_BUFFER_NOT_CREATED 0xffffffff

mx_VertexBuffer::mx_VertexBuffer()
	: vertex_size_(0)
	, index_vbo_(MX_BUFFER_NOT_CREATED), vertex_vbo_(MX_BUFFER_NOT_CREATED), vao_(MX_BUFFER_NOT_CREATED)
	, vertex_count_(0), index_data_size_(0)
{
}

mx_VertexBuffer::~mx_VertexBuffer()
{
	// By design, there is no resourse releasing.
}

void mx_VertexBuffer::VertexData( const void* data, unsigned int data_size, unsigned int vertex_size )
{
	if( vao_ == MX_BUFFER_NOT_CREATED )
		glGenVertexArrays( 1, &vao_ );
	glBindVertexArray(vao_);

	if( vertex_vbo_ == MX_BUFFER_NOT_CREATED )
		glGenBuffers( 1, &vertex_vbo_ );

	glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo_ );
	glBufferData( GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW );

	vertex_size_= vertex_size;
	vertex_count_= data_size / vertex_size_;
}

void mx_VertexBuffer::IndexData( const void* data, unsigned int data_size )
{
	if( vao_ == MX_BUFFER_NOT_CREATED )
		glGenVertexArrays( 1, &vao_ );
	glBindVertexArray(vao_);

	if( index_vbo_ == MX_BUFFER_NOT_CREATED )
		glGenBuffers( 1, &index_vbo_ );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo_ );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW );

	index_data_size_= data_size;
}

void mx_VertexBuffer::VertexSubData( const void* data, unsigned int data_size, unsigned int shift )
{
	MX_ASSERT( vertex_vbo_ != MX_BUFFER_NOT_CREATED );

	glBindVertexArray(vao_);
	glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo_ );
	glBufferSubData( GL_ARRAY_BUFFER, shift, data_size, data );
}

void mx_VertexBuffer::IndexSubData( const void* data, unsigned int data_size, unsigned int shift )
{
	MX_ASSERT( index_vbo_ != MX_BUFFER_NOT_CREATED );

	glBindVertexArray(vao_);
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo_ );
	glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, shift, data_size, data );
}

void mx_VertexBuffer::VertexAttrib( int attrib, unsigned int components, GLenum type, bool normalized, unsigned int shift )
{
	glVertexAttribPointer( attrib, components, type, normalized, vertex_size_, (void*) shift );
	glEnableVertexAttribArray( attrib );
}

void mx_VertexBuffer::VertexAttribInt( int attrib, unsigned int components, GLenum type, unsigned int shift )
{
	glVertexAttribIPointer( attrib, components, type, vertex_size_, (void*) shift );
	glEnableVertexAttribArray( attrib );
}