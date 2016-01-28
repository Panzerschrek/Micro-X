#pragma once

#include <cstdlib>

#include "coroutine.h"
#include "pawn.h"

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
	mx_Monster( const float* pos, const mx_PatrolPath* patrol_path= NULL );
	~mx_Monster();

protected:
	virtual void ExecFunc();

private:
	mx_Monster(const mx_Monster&);
	mx_Monster& operator=(const mx_Monster&);

private:

	mx_PatrolPath patrol_path_;
	const bool have_patrol_path_;

	bool destroy_;
};
