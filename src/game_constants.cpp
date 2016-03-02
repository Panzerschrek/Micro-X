#include "game_constants.h"

namespace mx_GameConstants
{

const float player_respawn_time= 5.0f;

const int initial_player_health= 100;
const int player_max_health= 100;
const int health_pack_health= 25;

const float health_pack_drop_chance= 0.5f;

const int initial_monsters_health[LastMonster]=
{
	30,
	20,
};

const float monsters_radius[LastMonster]=
{
	0.7f,
	0.5f,
};

const int rocket_blast_max_damage= 16;
const float rocket_blast_damage_on_distance_1= 20.0f;
const float rocket_blast_max_damage_distance= 3.0f;

const int bullets_damage[LastBullet]=
{
	 5,
	10,
	 7,
};

const float bullets_speed[LastBullet]=
{
	36.0f,
	 6.0f,
	12.0f,
};

const float bullets_colors[LastBullet][3]=
{
	{ 0.9f, 0.9f, 0.9f },
	{ 0.9f, 0.7f, 0.3f },
	{ 0.2f, 0.9f, 0.2f },
};

const float icosahedron_color[3]= { 0.9f, 0.9f, 0.1f };

const unsigned int player_initial_ammo[LastBullet]=
{
	100,
	 10,
	 30,
};

const unsigned int ammo_boxes_bullets_count[LastBullet]=
{
	50,
	10,
	20,
};

const float ammo_pockup_radius= 0.4f;
const float icosahedron_pickup_radius= 0.4f;
const float health_pack_pickup_radius= 0.4f;
const float player_radius= 0.3f;

const float machinegun_shot_interval= 1.0f / 8.0f;
const float rocket_launcher_shot_interval= 1.0f / 2.0f;
const float plasmagun_shot_interval= 1.0f / 6.0f;

} // namespace mx_GameConstants