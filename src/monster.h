#pragma once

#include <cstdlib>

#include "coroutine.h"
#include "pawn.h"

class mx_Player;
class mx_Level;

enum MonsterType
{
	MonsterOctoRobot,
	PyramidRobot,
	LastMonster
};

struct mx_PatrolPath
{
	// 0 - yz
	// 1 - xz
	// 2 - xy
	int path_plane;
	float points[2][3];
};

class mx_Monster : public mx_Pawn, public mx_Coroutine
{
public:
	mx_Monster( MonsterType type, const float* pos, const mx_PatrolPath* patrol_path= NULL );
	~mx_Monster();

	MonsterType GetType() const;
	int GetHealth() const;

	void Hit( int damage );

protected:
	virtual void ExecFunc();

private:
	mx_Monster(const mx_Monster&);
	mx_Monster& operator=(const mx_Monster&);

private:
	const MonsterType type_;

	int health_;

	mx_PatrolPath patrol_path_;
	const bool have_patrol_path_;

	bool destroy_;
};

inline MonsterType mx_Monster::GetType() const
{
	return type_;
}

inline int mx_Monster::GetHealth() const
{
	return health_;
}

inline void mx_Monster::Hit( int damage )
{
	health_-= damage;
}