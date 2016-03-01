#include "game_constants.h"

#include "models.h"

namespace mx_Models
{

static const unsigned char robot_model[]=
#include "../models/robot.h"
;

static const unsigned char pyramid_robot_model[]=
#include "../models/pyramid_robot.h"
;

const unsigned char cube[]=
#include "../models/cube.h"
;

const unsigned char icosahedron[]=
#include "../models/icosahedron.h"
;

extern const unsigned char* const models[LastModel]=
{
	robot_model,
	pyramid_robot_model,
	cube,
	icosahedron,
};

const float models_scale[LastModel]=
{
	0.125f,
	0.25f,
	0.1f,
	0.15f,
};

const Model monster_to_model_table[LastMonster]=
{
	ModelOcotMonster,
	ModelPyramidRobot,
};

} // namespace mx_Models
