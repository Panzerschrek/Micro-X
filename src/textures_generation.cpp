#include <algorithm>

#include "game_constants.h"
#include "mx_assert.h"
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

static const float g_powerups_bg[4]= { 0.1f, 0.1f, 0.1f, 0.9f };

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

static void SetAlphaToOne( mx_Texture* texture )
{
	static const float c_zero_alpha[4]= { 1.0f, 1.0f, 1.0f, 0.0f };
	static const float c_one_alpha[4]= { 0.0f, 0.0f, 0.0f, 1.0f };
	texture->Mul( c_zero_alpha );
	texture->Add( c_one_alpha );
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
	SetAlphaToOne( texture );
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
	SetAlphaToOne( texture );
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

static void GenMapScreenTexture( mx_Texture * texture )
{
	MX_ASSERT( texture->SizeX() == texture->SizeY() );
	unsigned int size= texture->SizeX();

	texture->Fill( g_powerups_bg );

	mx_Rand rand;

	for( unsigned int i= 0; i < 64; i++ )
	{
		static const float c_line_color[4]= { 1.0f, 1.0f, 1.0f, 0.1f };
		texture->DrawLine(
			rand.RandI( 1, size ),
			rand.RandI( 1, size ),
			rand.RandI( 1, size ),
			rand.RandI( 1, size ),
			c_line_color );
	}
}

static void GenMapScreenTextureHeightMap( mx_Texture * texture )
{
	texture->Fill( g_black );
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

static void GetAmmoBoxTextureBase( mx_Texture* texture, const float* color )
{
	MX_ASSERT( texture->SizeX() == texture->SizeY() );
	unsigned int size= texture->SizeX();
	texture->Fill( g_powerups_bg );

	unsigned int border_size= size >> 4;
	unsigned int corner_size= border_size * 3;

	// Left
	texture->FillRect(
		0, 0,
		border_size, size,
		color );
	// Right
	texture->FillRect(
		size - border_size, 0,
		border_size, size,
		color );

	// Bottom
	texture->FillRect(
		0, 0,
		size, border_size,
		color );
	// Top
	texture->FillRect(
		0, size - border_size,
		size, border_size,
		color );

	// Lower left
	texture->FillRect(
		0, 0,
		corner_size, corner_size,
		color );
	// Lower Right
	texture->FillRect(
		size - corner_size, 0,
		corner_size, corner_size,
		color );

	// Upper left
	texture->FillRect(
		0, size - corner_size,
		corner_size, corner_size,
		color );
	// Upper Right
	texture->FillRect(
		size - corner_size, size - corner_size,
		corner_size, corner_size,
		color );
}

static void SetBulletColor( float* color, BulletType bullet )
{
	for( unsigned int i= 0; i < 3; i++ )
		color[i]= mx_GameConstants::bullets_colors[bullet][i];
	color[3]= 0.0f;
}

static void GenBulletAmmoBoxTexture( mx_Texture* texture )
{
	MX_ASSERT( texture->SizeX() == texture->SizeY() );
	unsigned int size= texture->SizeX();

	float color[4];
	SetBulletColor( color, MachinegunBullet );

	GetAmmoBoxTextureBase( texture, color );

	for( unsigned int x= 0; x < 3; x++ )
	for( unsigned int y= 0; y < 2; y++ )
		texture->FillRect(
			( size * ( 9 +  6 * x ) ) >> 5,
			( size * ( 6 + 12 * y ) ) >> 5,
			( 2 * size ) >> 5,
			( 8 * size ) >> 5,
			color );
}

static void GenRocketAmmoBoxTexture( mx_Texture* texture )
{
	MX_ASSERT( texture->SizeX() == texture->SizeY() );
	unsigned int size= texture->SizeX();

	float color[4];
	SetBulletColor( color, Rocket );

	GetAmmoBoxTextureBase( texture, color );

	texture->FillRect(
		( size * 11 ) >> 5,
		( size *  6 ) >> 5,
		( size * 10 ) >> 5,
		( size * 20 ) >> 5,
		color );
}

static void GenPlasmaAmmoBoxTexture( mx_Texture* texture )
{
	MX_ASSERT( texture->SizeX() == texture->SizeY() );
	unsigned int size= texture->SizeX();

	float color[4];
	SetBulletColor( color, PlasmaBall );

	GetAmmoBoxTextureBase( texture, color );

	for( unsigned int x= 0; x < 2; x++ )
	for( unsigned int y= 0; y < 2; y++ )
		texture->FillEllipse(
			( size * ( 5 + x * 6 ) ) >> 4,
			( size * ( 5 + y * 6 ) ) >> 4,
			size >> 3,
			color );
}

void mxGenIcosahedronTexture( mx_Texture* texture )
{
	MX_ASSERT( texture->SizeX() == texture->SizeY() );
	unsigned int size= texture->SizeX();
	unsigned int size_log2= texture->SizeXLog2();

	texture->Fill( g_powerups_bg );

	const unsigned int c_border_size= 16;

	float* d= texture->GetData();
	for( unsigned int y= c_border_size; y < size - c_border_size; y++ )
	{
		for( unsigned int x= y + c_border_size; x < size - c_border_size; x++ )
		{
			float* pix= d + ( ( x + (y << size_log2) ) << 2 );
			for( unsigned int j= 0; j < 3; j++ )
				pix[j]= mx_GameConstants::icosahedron_color[j];
			pix[3]= 0.0f;
		}
	}
}

void (* const gen_monsters_textures_func_table[LastMonster])( mx_Texture* texture )=
{
	GenOctoRobotTexture,
	GenPyramidRobotTexture,
};

void (* const gen_ammo_textures_func_table[LastBullet])( mx_Texture* texture )=
{
	GenBulletAmmoBoxTexture,
	GenRocketAmmoBoxTexture,
	GenPlasmaAmmoBoxTexture,
};

void (* const gen_level_textures_func_table[LastLevelTexture])( mx_Texture* texture )=
{
	GenGraniteTexture,
	GenSteelPlateTexture,
	GenMapScreenTexture,
};

void (* const gen_level_textures_height_map_func_table[LastLevelTexture])( mx_Texture* height_map )=
{
	GenGraniteTextureHeightMap,
	GenSteelPlateTextureHeightMap,
	GenMapScreenTextureHeightMap,
};