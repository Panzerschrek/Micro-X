#pragma once

#include "gl/funcs.h"

class mx_VertexBuffer
{
public:
	mx_VertexBuffer();
	~mx_VertexBuffer();

	void Bind();

	void VertexData( const void* data, unsigned int data_size, unsigned int vertex_size );
	void IndexData( const void* data, unsigned int data_size );
	void VertexSubData( const void* data, unsigned int data_size, unsigned int shift );
	void IndexSubData( const void* data, unsigned int data_size, unsigned int shift );

	// You must bind buffer before setup of v0ertex attribute.
	void VertexAttrib( int attrib, unsigned int components, GLenum type, bool normalized, unsigned int shift );
	void VertexAttribInt( int attrib, unsigned int components, GLenum type, unsigned int shift );

	unsigned int VertexCount() const;
	unsigned int IndexDataSize() const;

private:
	unsigned int vertex_size_;
	GLuint index_vbo_, vertex_vbo_, vao_;
	unsigned int vertex_count_, index_data_size_;
};

inline void mx_VertexBuffer::Bind()
{
	glBindVertexArray(vao_);
}

inline unsigned int mx_VertexBuffer::VertexCount() const
{
	return vertex_count_;
}

inline unsigned int mx_VertexBuffer::IndexDataSize() const
{
	return index_data_size_;
}