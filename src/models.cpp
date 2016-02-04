#include "monster.h"

#include "models.h"

namespace mx_Models
{

static const unsigned char robot_model[]=
#include "../models/robot.h"
;

const unsigned char* monsters_models[LastMonster]=
{
	robot_model,
};

const float monsters_models_scale[LastMonster]=
{
	0.125f,
};

} // namespace mx_Models
