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

} // namespace mx_GameConstants