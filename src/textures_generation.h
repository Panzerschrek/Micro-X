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

extern void (* const gen_monsters_textures_func_table[LastMonster])( mx_Texture* texture );
extern void (* const gen_ammo_textures_func_table[LastBullet])( mx_Texture* texture );

extern void (* const gen_level_textures_func_table[LastLevelTexture])( mx_Texture* texture );
extern void (* const gen_level_textures_height_map_func_table[LastLevelTexture])( mx_Texture* height_map );

void mxGenIcosahedronTexture( mx_Texture* texture );
void mxGenHealthPackTextire( mx_Texture* texture );