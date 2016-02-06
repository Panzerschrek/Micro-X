#pragma once

class mx_Texture;

void mxGenGraniteTexture( mx_Texture * texture );
void mxGenSteelPlateTexture( mx_Texture * texture );

extern void (* const gen_monsters_textures_func_table[])( mx_Texture * texture );