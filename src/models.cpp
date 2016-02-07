#include "monster.h"

#include "models.h"

namespace mx_Models
{

static const unsigned char robot_model[]=
#include "../models/robot.h"
;

static const unsigned char pyramid_robot_model[]=
#include "../models/pyramid_robot.h"
;

const unsigned char icosahedron[]=
#include "../models/icosahedron.h"
;

const unsigned char* monsters_models[LastMonster]=
{
	robot_model,
	pyramid_robot_model,
};

const float monsters_models_scale[LastMonster]=
{
	0.125f,
	0.25f,
};

} // namespace mx_Models
