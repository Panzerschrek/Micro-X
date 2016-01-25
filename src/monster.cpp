#include "mx_math.h"

#include "monster.h"

mx_Monster::mx_Monster( const float* pos )
{
	VEC3_CPY( pos_, pos );
}

mx_Monster::~mx_Monster()
{
}