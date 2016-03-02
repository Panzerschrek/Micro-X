#pragma once

#include "game_constants.h"

namespace mx_Models
{

enum Model
{
	ModelOcotMonster,
	ModelPyramidRobot,
	ModelCube,
	ModelIcosahedron,
	LastModel,
};

extern const unsigned char* const models[LastModel];
extern const float models_scale[LastModel];

extern const Model monster_to_model_table[LastMonster];

} // namespace mx_Models
