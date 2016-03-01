#pragma once
#include "fwd.h"
#include "game_constants.h"

enum LevelTexture
{
	TextureGranite,
	TextureSteelPlate,
	TextureMapScreen,
	LastLevelTexture,
};

enum ModelTexture
{
	TextureOctoRobot,
	TexturePyramidRobot,
	TextureBulletAmmo,
	TextureRocketAmmo,
	TexturePlasmaAmmo,
	TextureIcosahedron,
	TextureHealthPack,
	LastModelTexture,
};

extern void (* const gen_models_textures_func_table[LastModelTexture])( mx_Texture* texture );

extern void (* const gen_level_textures_func_table[LastLevelTexture])( mx_Texture* texture );
extern void (* const gen_level_textures_height_map_func_table[LastLevelTexture])( mx_Texture* height_map );