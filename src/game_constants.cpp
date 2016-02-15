#include "game_constants.h"

namespace mx_GameConstants
{

const int initial_player_health= 100;

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
const float rocket_blast_damage_on_distance_1= 8.0f;
const float rocket_blast_max_damage_distance= 3.0f;

const int bullets_damage[LastBullet]=
{
	 5,
	10,
	 7,
};

const float bullets_speed[LastBullet]=
{
	50.0f,
	 4.0f,
	 6.0f,
};

const unsigned int player_initial_ammo[LastBullet]=
{
	100,
	 10,
	 30,
};

const float player_radius= 0.3f;

const float machinegun_shot_interval= 1.0f / 8.0f;
const float rocket_launcher_shot_interval= 1.0f / 2.0f;
const float plasmagun_shot_interval= 1.0f / 6.0f;

} // namespace mx_GameConstants