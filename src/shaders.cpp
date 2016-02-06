#include "shaders.h"

#define VERSION_HEADER "#version 330\n"
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
"n_" - output normal in fragment shader
*/

//text shader
const char text_shader_v[]=
VERSION_HEADER
"in vec2 p;"//in position
"in vec2 tc;"//in texture coord
"in vec4 c;"//in color
"out vec4 fc;"//out color
"out vec2 ftc;"//out texture coord
"void main()"
"{"
	"fc=c;"
	"ftc=tc*vec2(1.0/96.0,1.0);"
	"gl_Position=vec4(p,-1.0,1.0);"
"}"
;

const char text_shader_f[]=
VERSION_HEADER
"uniform sampler2D tex;"
"in vec4 fc;"
"in vec2 ftc;"
"out vec4 c_;"
"void main()"
"{"
	"float x=texture(tex,ftc).x;"
	"c_=vec4(fc.xyz*x,max(fc.a,x));"
"}"
;

// world shader
const char world_shader_v[]=
VERSION_HEADER
"in vec3 p;"
"in vec3 n;"
"in vec3 tc;"
"uniform mat4 mat;"
"out vec3 fn;"
"out vec3 ftc;"
"void main()"
"{"
	"fn=n*0.5+vec3(0.5,0.5,0.5);"
	"ftc=tc;"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

const char world_shader_f[]=
VERSION_HEADER
"uniform sampler2D tex;"
"out vec4 c_;"
"out vec4 n_;"
"in vec3 fn;"
"in vec3 ftc;"
"void main()"
"{"
	"c_=texture(tex,ftc.xy);"
	"n_=vec4(fn,1.0);"
"}"
;

const char plasma_ball_shader_v[]=
VERSION_HEADER
"in vec3 p;"
"uniform mat4 mat;"
"void main()"
"{"
"vec4 v=mat*vec4(p,1.0);"
"gl_Position=v;"
"gl_PointSize=64.0/v.w;"
"}"
;

const char plasma_ball_shader_f[]=
VERSION_HEADER
"out vec4 c_;"
"void main()"
"{"
"vec2 r=gl_PointCoord.xy-vec2(0.5,0.5);"
"c_=vec4(1.0,0.6,0.1,1.0-4.0*dot(r,r));"
"}"
;

const char fullscreen_postprocessing_shader_v[]=
VERSION_HEADER
"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,0.0),vec2(1.0,1.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"
"void main()"
"{"
	"gl_Position=vec4(coord[gl_VertexID]*2.0-vec2(1.0,1.0),0.0,1.0);"
"}"
;

const char fullscreen_postprocessing_shader_f[]=
VERSION_HEADER
"uniform sampler2D atex;" // albedo buffer
"uniform sampler2D dtex;" // depth buffer
"out vec4 c_;"
"void main()"
"{"
	"ivec2 tc=ivec2(gl_FragCoord.xy);"
	"c_=0.05*texelFetch(atex,tc,0);"
	"gl_FragCoord=texelFetch(dtex,tc,0);"
"}"
;

const char postprocessing_shader_v[]=
VERSION_HEADER
/*"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,0.0),vec2(1.0,1.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"*/

//"out vec2 ftc;"
"in vec3 p;"
"uniform mat4 mat;"
"void main()"
"{"
	//"ftc=coord[gl_VertexID];"
	//"gl_Position=vec4(coord[gl_VertexID]*2.0-vec2(1.0,1.0),0.0,1.0);"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

const char postprocessing_shader_f[]=
VERSION_HEADER
"uniform sampler2D atex;" // albedo buffer
"uniform sampler2D ntex;" // normals buffer
"uniform sampler2D dtex;" // depth buffer
"uniform vec3 lp;" // light position
"uniform vec3 lc;" // light color
"uniform mat4 imat;" // onverse matrix for transofrmation
// numbers from raw perspective mat
"uniform  float m10;"
"uniform  float m14;"
//"in vec2 ftc;"
"out vec4 c_;"

"vec4 GetWorldPosition()"
"{"
	"vec4 screen_space_pos;"
	"screen_space_pos.xy= 2.0 * gl_FragCoord.xy / vec2(textureSize(atex, 0)) - vec2(1.0, 1.0);"
	"screen_space_pos.z= 2.0 * texelFetch(dtex, ivec2(gl_FragCoord.xy), 0).x - 1.0;"

	"screen_space_pos.w= m14 / ( screen_space_pos.z - m10 );"
	"screen_space_pos.xyz*= screen_space_pos.w;"

	"return imat*screen_space_pos;"
"}"

"void main()"
"{"
	"c_=texelFetch(atex, ivec2(gl_FragCoord.xy), 0);"

	"vec3 vec_to_light= lp - GetWorldPosition().xyz;"
	"vec3 vec_to_light_normalized= normalize(vec_to_light);"

	"vec3 normal= texelFetch(ntex, ivec2(gl_FragCoord.xy), 0).xyz * vec3(2.0,2.0,2.0) - vec3(1.0,1.0,1.0);"
	"float normal_k= max( 0.0, dot(normal, vec_to_light_normalized) );"

	"c_.xyz*= lc * (normal_k / dot(vec_to_light,vec_to_light));"
	//"c_=vec4(0.1,0.0, 0.1, 0.5);"
"}"
;


} // namespace mx_Shaders