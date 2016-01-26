#pragma once

#include <cstdlib>

#include "coroutine.h"

struct mx_PatrolPath
{
	float points[2][3];
};

class mx_Monster : public mx_Coroutine
{
public:
	mx_Monster( const float* pos, const mx_PatrolPath* patrol_path= NULL );
	~mx_Monster();

	const float* Pos() const;

protected:
	virtual void ExecFunc();

private:
	mx_Monster(const mx_Monster&);
	mx_Monster& operator=(const mx_Monster&);

private:
	float pos_[3];

	mx_PatrolPath patrol_path_;
	const bool have_patrol_path_;

	bool destroy_;
};

inline const float* mx_Monster::Pos() const
{
	return pos_;
}