#pragma once

enum MonsterType
{
	MonsterOctoRobot,
	PyramidRobot,
	LastMonster
};

enum BulletType
{
	MachinegunBullet,
	Rocket,
	LastBullet
};

namespace mx_GameConstants
{

extern const int initial_player_health;

extern const int initial_monsters_health[LastMonster];

extern const int bullets_damage[LastBullet];
// units per second
extern const float bullets_speed[LastBullet];

} // namespace mx_GameConstants