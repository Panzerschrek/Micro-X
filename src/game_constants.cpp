#include "game_constants.h"

namespace mx_GameConstants
{

const int initial_player_health= 100;

const int initial_monsters_health[LastMonster]=
{
	30,
	20,
};

const int bullets_damage[LastBullet]=
{
	 5,
	10,
};

const float bullets_speed[LastBullet]=
{
	50.0f,
	 6.0f,
};

const float player_radius= 0.3f;

const float machinegun_shot_interval= 1.0f / 8.0f;
const float rocket_launcher_shot_interval= 1.0f / 2.0f;

} // namespace mx_GameConstants