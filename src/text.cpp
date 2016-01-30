#include "main_loop.h"
#include "mx_assert.h"
#include "shaders.h"

#include "text.h"

static const unsigned char g_font_data[]=
#include "../textures/fixedsys8x18c.h"
;

const unsigned char mx_Text::default_color[4]= {255, 255, 255, 32 };
const unsigned char* mx_Text::font_data_= NULL;

mx_Text::mx_Text()
	: vertex_buffer_pos_(0)
{
	CreateTexture();

	vertices_= new mx_TextVertex[ MX_MAX_TEXT_BUFFER_SIZE * 4 ];

	unsigned short* quad_indeces= new unsigned short[ MX_MAX_TEXT_BUFFER_SIZE * 6 ];
	for( unsigned int i= 0, j= 0; i< MX_MAX_TEXT_BUFFER_SIZE*6; i+= 6, j+=4 )
	{
		quad_indeces[i]= (unsigned short)j;
		quad_indeces[i + 1]= (unsigned short)(j + 1);
		quad_indeces[i + 2]= (unsigned short)(j + 2);

		quad_indeces[i + 3]= (unsigned short)(j + 2);
		quad_indeces[i + 4]= (unsigned short)(j + 3);
		quad_indeces[i + 5]= (unsigned short)(j);
	}

	text_vbo_.VertexData( NULL, MX_MAX_TEXT_BUFFER_SIZE * 4 * sizeof(mx_TextVertex), sizeof(mx_TextVertex) );
	text_vbo_.IndexData( quad_indeces, MX_MAX_TEXT_BUFFER_SIZE * 6 * sizeof(short) );
	delete[] quad_indeces;

	text_vbo_.VertexAttrib( 0, 2, GL_FLOAT, false, 0 );//vertex coordinates
	text_vbo_.VertexAttrib( 1, 2, GL_UNSIGNED_SHORT, false, sizeof(float)*2 );//texture coordinates
	text_vbo_.VertexAttrib( 2, 4, GL_UNSIGNED_BYTE, true, sizeof(float)*2 + 2*sizeof(short) );//color

	text_shader_.SetAttribLocation( "v", 0 );
	text_shader_.SetAttribLocation( "tc", 1 );
	text_shader_.SetAttribLocation( "c", 2 );
	text_shader_.Create( mx_Shaders::text_shader_v, mx_Shaders::text_shader_f );
	text_shader_.FindUniform( "tex" );
}

mx_Text::~mx_Text()
{
	glDeleteTextures( 1, &font_texture_id_ );
}

void mx_Text::AddText( unsigned int colomn, unsigned int row, unsigned int size, const unsigned char* color, const char* text )
{
	MX_ASSERT( vertex_buffer_pos_ <= MX_MAX_TEXT_BUFFER_SIZE * 4 );

	const char* str= text;

	float x, x0, y;
	float dx, dy;

	float viewport_width = float(mx_MainLoop::Instance()->ViewportWidth ());
	float viewport_height= float(mx_MainLoop::Instance()->ViewportHeight());

	x0= x= 2.0f * float( colomn * MX_LETTER_WIDTH ) / viewport_width - 1.0f;
	y= -2.0f * float( row * MX_LETTER_HEIGHT ) / viewport_height + 1.0f;

	dx= 2.0f * float( MX_LETTER_WIDTH  * size ) / viewport_width;
	dy= 2.0f * float( MX_LETTER_HEIGHT * size ) / viewport_height;
	y-= dy;

	mx_TextVertex* v= vertices_ + vertex_buffer_pos_;
	while( *str != 0 )
	{
		if( *str == '\n' )
		{
			x= x0;
			y-=dy;
			str++;
			continue;
		}
		v[0].pos[0]= x;
		v[0].pos[1]= y;
		v[0].tex_coord[0]= *str - 32;
		v[0].tex_coord[1]= 0;

		v[1].pos[0]= x;
		v[1].pos[1]= y + dy;
		v[1].tex_coord[0]= *str - 32;
		v[1].tex_coord[1]= 1;

		v[2].pos[0]= x + dx;
		v[2].pos[1]= y + dy;
		v[2].tex_coord[0]= *str - 32 + 1;
		v[2].tex_coord[1]= 1;

		v[3].pos[0]= x + dx;
		v[3].pos[1]= y;
		v[3].tex_coord[0]= *str - 32 + 1;
		v[3].tex_coord[1]= 0;

		for( unsigned int i= 0; i< 4; i++ )
			*((int*)v[i].color)= *((int*)color);//copy 4 bytes per one asm command

		x+= dx;
		v+= 4;
		str++;
	}
	vertex_buffer_pos_= v - vertices_;
}

void mx_Text::Draw()
{
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, font_texture_id_ );

	text_vbo_.Bind();
	text_vbo_.VertexSubData( vertices_, vertex_buffer_pos_ * sizeof(mx_TextVertex), 0 );

	text_shader_.Bind();
	text_shader_.UniformInt( "tex", 0 );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_DEPTH_TEST );

	glDrawElements( GL_TRIANGLES, vertex_buffer_pos_ * 6 / 4, GL_UNSIGNED_SHORT, NULL );

	glEnable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	vertex_buffer_pos_= 0;
}

void mx_Text::CreateTexture()
{
	MX_ASSERT( font_data_ == NULL );
	MX_ASSERT( sizeof(g_font_data) * 8 == MX_FONT_BITMAP_WIDTH * MX_FONT_BITMAP_HEIGHT );

	unsigned char* decompressed_font= new unsigned char[ MX_FONT_BITMAP_WIDTH * MX_FONT_BITMAP_HEIGHT ];
	font_data_= decompressed_font;

	//mfMonochromeImageTo8Bit( font_data, decompressed_font, MF_FONT_BITMAP_WIDTH * MF_FONT_BITMAP_HEIGHT );
	for( unsigned int i= 0; i< MX_FONT_BITMAP_WIDTH * MX_FONT_BITMAP_HEIGHT / 8; i++ )
		for( unsigned int j= 0; j< 8; j++ )
			decompressed_font[ (i<<3) + (7-j) ]= ( (g_font_data[i] & (1<<j)) >> j ) * 255;

	glGenTextures( 1, &font_texture_id_ );

	glBindTexture( GL_TEXTURE_2D, font_texture_id_ );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_R8,
		MX_FONT_BITMAP_WIDTH, MX_FONT_BITMAP_HEIGHT, 0,
		GL_RED, GL_UNSIGNED_BYTE, decompressed_font );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
}