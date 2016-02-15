#pragma once

enum MonsterType
{
	MonsterOctoRobot,
	PyramidRobot,
	LastMonster,
};

enum BulletType
{
	MachinegunBullet,
	Rocket,
	PlasmaBall,
	LastBullet,
};

namespace mx_GameConstants
{

extern const int initial_player_health;

extern const int initial_monsters_health[LastMonster];
extern const float monsters_radius[LastMonster];

extern const int rocket_blast_max_damage;
extern const float rocket_blast_damage_on_distance_1;
extern const float rocket_blast_max_damage_distance;

extern const int bullets_damage[LastBullet];
// units per second
extern const float bullets_speed[LastBullet];

extern const unsigned int player_initial_ammo[LastBullet];

extern const float player_radius;

extern const float machinegun_shot_interval;
extern const float rocket_launcher_shot_interval;
extern const float plasmagun_shot_interval;

} // namespace mx_GameConstants