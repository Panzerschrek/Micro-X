#include <algorithm>

#include "mx_math.h"
#include "texture.h"

#include "textures_generation.h"

static const float g_white[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
static const float g_black[4]= { 0.0f, 0.0f, 0.0f, 0.0f };
static const float g_one= 1.0f;

static const float g_monsters_body_color[4]= { 0.6f, 0.6f, 0.6f, 1.0f };
static const float g_monsters_eyes_bg_color[4]= { 0.1f, 0.1f, 0.1f, 1.0f };
static const float g_monsters_eyes_color[4]= { 1.0f, 0.2f, 0.2f, 0.0f };
static const float g_monsters_antenna_color[4]= { 1.0f, 0.2f, 0.2f, 1.0f };
static const float g_monsters_dark_color[4]= { 0.08f, 0.08f, 0.08f, 1.0f };

static void AddHemisphereToHeightmap(
	mx_Texture* height_map,
	unsigned int center_x, unsigned int center_y,
	unsigned int radius, float depth )
{
	int r2= int(radius) * int(radius);

	float* d= height_map->GetData();
	for( unsigned int y= center_y - radius; y<= center_y + radius; y++ )
	{
		int dy2= int(y) - center_y;
		dy2= dy2 * dy2;
		int r2_minus_dy2= r2 - dy2;

		unsigned int y_shift= y << height_map->SizeXLog2();

		for( unsigned int x= center_x - radius; x<= center_x + radius; x++ )
		{
			int dx= int(x) - center_x;
			float h= std::sqrtf( float( std::max( r2_minus_dy2 - dx * dx, 0 ) ) ) - depth;
			if( h >= 0.0f )
				d[ ( x + y_shift ) * 4u + 3u ]+= h;
		}
	}
}

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

static void GenGraniteTexture( mx_Texture * texture )
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
}

static void GenGraniteTextureHeightMap( mx_Texture * height_map )
{
	static const float g_mul[4]= { 4.0f, 4.0f, 4.0f, 4.0f };
	height_map->Noise( 1567489, 5 );

	height_map->Mul( g_mul );
}

static void GenSteelPlateTexture( mx_Texture * texture )
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
}

static void GenSteelPlateTextureHeightMap( mx_Texture* height_map )
{
	static const unsigned int c_border_width= 32;
	static const float c_slope= 0.333f;
	static const float c_up_color[4]= { 0.0f, 0.0f, 0.0f, c_slope * float(c_border_width) };

	height_map->Fill( c_up_color );

	float* d= height_map->GetData();
	for( unsigned int r= 0; r < c_border_width; r++ )
	{
		float h= c_slope * float(r);
		for( unsigned int y= r; y < height_map->SizeY() - r; y++ )
		{
			unsigned int y_shift= y << height_map->SizeXLog2();
			d[ ( r + y_shift ) * 4 + 3 ]= h;
			d[ ( height_map->SizeX() - 1 - r + y_shift ) * 4 + 3 ]= h;
		}

		for( unsigned int x= r; x < height_map->SizeX() - r; x++ )
		{
			d[ ( x + (r<<height_map->SizeXLog2()) ) * 4 + 3 ]= h;
			d[ ( x + ( (height_map->SizeY()-1-r) << height_map->SizeXLog2() ) ) * 4 + 3 ]= h;
		}
	}

	// AddHemisphereToHeightmap( height_map, 512, 512, 128, 128 - 8 );
}

static void GenOctoRobotTexture( mx_Texture * texture )
{
	texture->Fill( g_monsters_body_color );

	// Eyes
	texture->FillRect( 29, 32, 68, 68, g_monsters_eyes_bg_color );
	texture->FillEllipse( 62, 67, 12, g_monsters_eyes_color );

	// Radiator bars
	for( unsigned int i= 0; i < 4; i++ )
		texture->FillRect( 332 + 36 * i, 50, 20, 72, g_monsters_dark_color );

	// Rockets battery
	for( unsigned int i= 0; i < 3; i++ )
	{
		texture->FillEllipse( 48 + i * 38, 250, 14, g_monsters_dark_color );
		texture->FillEllipse( 48 + i * 38, 250, 8, g_monsters_eyes_color );

		texture->FillEllipse( 208 + i * 38, 256, 14, g_monsters_dark_color );
	}

	// Antena
	texture->FillRect( 336, 264, 70, 70, g_monsters_antenna_color );
}

static void GenPyramidRobotTexture( mx_Texture * texture )
{
	static const float c_dark_gray[4]= { 0.5f, 0.5f, 0.5f, 0.0f };
	texture->Fill( g_monsters_body_color );

	// Antenna
	texture->FillRect( 436, 24, 48, 48, g_monsters_antenna_color );

	// Eyes
	texture->FillRect( 116, 32, 52, 52, g_monsters_dark_color );
	texture->FillEllipse( 140, 58, 8, g_monsters_eyes_color );

	// Engine
	static const float c_engine_color[4]= {0.1f, 0.05f, 0.4f, 0.0f };
	texture->FillRect( 15, 16, 98, 92, c_engine_color );

	// Machinegun
	texture->FillRect( 180, 64, 40, 40, g_monsters_dark_color );
}

void (* const gen_monsters_textures_func_table[LastMonster])( mx_Texture * texture )=
{
	GenOctoRobotTexture,
	GenPyramidRobotTexture,
};

void (* const gen_level_textures_func_table[LastLevelTexture])( mx_Texture* texture )=
{
	GenGraniteTexture,
	GenSteelPlateTexture,
};

void (* const gen_level_textures_height_map_func_table[LastLevelTexture])( mx_Texture* height_map )=
{
	GenGraniteTextureHeightMap,
	GenSteelPlateTextureHeightMap,
};