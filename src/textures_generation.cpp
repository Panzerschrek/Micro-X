#include "monster.h"
#include "texture.h"

#include "textures_generation.h"

static const float g_white[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
static const float g_black[4]= { 0.0f, 0.0f, 0.0f, 0.0f };
static const float g_one= 1.0f;

static void GenNoisedColorMix(
	mx_Texture * texture,
	const float* bg_color,
	const float* colors, unsigned int color_count,
	unsigned int noise_octaves )
{
	texture->Fill( bg_color );

	mx_Texture tex2( texture->SizeXLog2(), texture->SizeYLog2() );
	for( unsigned int i= 0; i < color_count; i++ )
	{
		tex2.Noise( i, noise_octaves );

		static const float c_edge= 135.0f / 255.0f;
		for( float* d= texture->GetData(),
			*d_end= texture->GetData() + (1 << (tex2.SizeXLog2() + tex2.SizeYLog2() + 2)),
			*d_src= tex2.GetData();
			d < d_end; d+= 4, d_src+= 4 )
		{
			if( *d_src >= c_edge )
			{
				for( unsigned int j= 0; j < 4; j++ )
					d[j]= colors[i * 4 + j];
			}
		}
	}
}

void mxGenGraniteTexture( mx_Texture * texture )
{
	static const float c_bg_color[4]= { 162.0f / 255.0f, 154.0f / 255.0f, 167.0f / 255.0f, 0.0f };
	static const float c_colors[6][4]=
	{
		{ 142.0f / 255.0f, 145.0f / 255.0f, 155.0f / 255.0f, 0.0f },
		{ 145.0f / 255.0f, 154.0f / 255.0f, 162.0f / 255.0f, 0.0f },
		{ 166.0f / 255.0f, 141.0f / 255.0f, 140.0f / 255.0f, 0.0f },
		{ 153.0f / 255.0f, 153.0f / 255.0f, 153.0f / 255.0f, 0.0f },
		{ 148.0f / 255.0f, 129.0f / 255.0f, 134.0f / 255.0f, 0.0f },
		{ 124.0f / 255.0f, 125.0f / 255.0f, 137.0f / 255.0f, 0.0f },
	};

	GenNoisedColorMix( texture, c_bg_color, c_colors[0], 6, 4 );

	texture->LinearNormalization( g_one );
}

void mxGenSteelPlateTexture( mx_Texture * texture )
{
	static const float c_bg_color[4]= { 149.0f / 255.0f, 157.0f / 255.0f, 160.0f / 255.0f, 0 };
	static const float c_colors[8][4]=
	{
		{ 141.0f / 255.0f, 149.0f / 255.0f, 152.0f / 255.0f, 0.0f },
		{ 128.0f / 255.0f, 135.0f / 255.0f, 141.0f / 255.0f, 0.0f },
		{ 202.0f / 255.0f, 207.0f / 255.0f, 211.0f / 255.0f, 0.0f },
		{ 121.0f / 255.0f, 126.0f / 255.0f, 132.0f / 255.0f, 0.0f },
		{ 146.0f / 255.0f, 151.0f / 255.0f, 154.0f / 255.0f, 0.0f },
		{ 155.0f / 255.0f, 162.0f / 255.0f, 168.0f / 255.0f, 0.0f },
		{ 132.0f / 255.0f, 136.0f / 255.0f, 139.0f / 255.0f, 0.0f },
		{ 136.0f / 255.0f, 139.0f / 255.0f, 144.0f / 255.0f, 0.0f },
	};

	GenNoisedColorMix( texture, c_bg_color, c_colors[0], 8, 3 );

	{
		mx_Texture tex2( texture->SizeXLog2(), texture->SizeYLog2() );
		tex2.Fill( g_white );

		static const float c_gray[4]= { 0.8f, 0.8f, 0.8f, 0.8f };
		const unsigned int c_shift= 7;

		tex2.FillRect( 0, 0, tex2.SizeX() >> c_shift, tex2.SizeY(), c_gray );
		tex2.FillRect( tex2.SizeX() - (tex2.SizeX() >> c_shift), 0, tex2.SizeX() >> c_shift, tex2.SizeY(), c_gray );

		tex2.FillRect( 0, 0, tex2.SizeX(), tex2.SizeY() >> c_shift, c_gray );
		tex2.FillRect( 0, tex2.SizeY() - (tex2.SizeY() >> c_shift), tex2.SizeX(), tex2.SizeY() >> c_shift, c_gray );

		texture->Mul( &tex2 );
	}

	texture->LinearNormalization( g_one );
}

static void GenOctoRobotTexture( mx_Texture * texture )
{
	static const float c_bg_color[4]= { 0.6f, 0.6f, 0.6f, 0.0f };
	texture->Fill( c_bg_color );

	// Eyes
	static const float c_dark_gray[4]= { 0.1f, 0.1f, 0.1f, 0.0f };
	texture->FillRect( 29, 32, 68, 68, c_dark_gray );
	static const float c_red[4]= { 1.0f, 0.2f, 0.2f, 0.0f };
	texture->FillEllipse( 62, 67, 12, c_red );

	// Radiator bars
	for( unsigned int i= 0; i < 4; i++ )
		texture->FillRect( 332 + 36 * i, 50, 20, 72, c_dark_gray );

	// Rockets battery
	for( unsigned int i= 0; i < 3; i++ )
	{
		texture->FillEllipse( 48 + i * 38, 250, 14, c_dark_gray );
		texture->FillEllipse( 48 + i * 38, 250, 8, c_red );

		texture->FillEllipse( 208 + i * 38, 256, 14, c_dark_gray );
	}

	// Antena
	texture->FillRect( 336, 264, 70, 70, c_red );

	texture->LinearNormalization( g_one );
}

void (* const gen_monsters_textures_func_table[LastMonster])( mx_Texture * texture )=
{
	GenOctoRobotTexture,
};