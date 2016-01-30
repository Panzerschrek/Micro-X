#pragma once

#include "glsl_program.h"
#include "vertex_buffer.h"

#pragma pack( push, 1 )
struct mx_TextVertex
{
	float pos[2];
	unsigned short tex_coord[2];
	unsigned char color[4];
};
#pragma pack (pop)


#define MX_MAX_TEXT_BUFFER_SIZE 8192

#define MX_LETTER_WIDTH 8
#define MX_LETTER_HEIGHT 18
#define MX_FONT_BITMAP_WIDTH 768
#define MX_FONT_BITMAP_HEIGHT 18

class mx_Text
{
public:
	mx_Text();
	~mx_Text();

	void AddText( unsigned int colomn, unsigned int row, unsigned int size, const unsigned char* color, const char* text );
	void Draw();

	// Hack. Get font data in places, wher text is not accessible.
	static const unsigned char* GetFontData();

	static const unsigned char default_color[4];

private:
	void CreateTexture();

private:
	static const unsigned char* font_data_;

	mx_GLSLProgram text_shader_;
	mx_TextVertex* vertices_;
	mx_VertexBuffer text_vbo_;
	unsigned int vertex_buffer_pos_;

	GLuint font_texture_id_;
};

inline const unsigned char* mx_Text::GetFontData()
{
	return font_data_;
}