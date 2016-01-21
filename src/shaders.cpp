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
"in vec3 n;"
"in vec3 tc;"
"uniform mat4 mat;"
"out vec4 fc;"
"out vec3 ftc;"
"void main()"
"{"
	"fc= vec4(n*0.5+vec3(0.5,0.5,0.5),0.5);"
	"ftc=tc;"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

const char world_shader_f[]=
"#version 330\n"
"out vec4 c_;"
"in vec4 fc;"
"in vec3 ftc;"
"void main()"
"{"
"c_=fc * 0.25 * (step(1.0, mod(ftc.x, 2.0)) + 1.0) * ( step(1.0, mod(ftc.y, 2.0)) + 1.0);"
"}"
;

} // namespace mx_Shaders