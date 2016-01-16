#include "shaders.h"

namespace mx_Shaders
{

/*
common attributes:
"p" - position
"n" - normal
"tc" - texture coordinates
"c" - color

common uniforms names:
"mat" - view matrix ( or view matrix, combined with other transformation )
"nmat" - normal matrix
"smat" - shadow matrix
"tex" - diffuse texture
"texn" - number of layer in texture array
"sun" - normalized vector of directional light source (sun/moon/etc)
"sl" - color of directional light
"al" - color of ambient light

common input/output names
"fp" - fragment position
"fn" - fragment normal
"ftc" - fragment texture coordinates
"fc" - fragment color
"c_" - output color of fragment shader
*/

const char world_shader_v[]=
"#version 330\n"
"in vec3 p;"
"uniform mat4 mat;"
"void main()"
"{"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

const char world_shader_f[]=
"#version 330\n"
"out vec4 c_;"
"void main()"
"{"
	"c_=vec4(1.0,1.0,1.0,0.5);"
"}"
;

} // namespace mx_Shaders