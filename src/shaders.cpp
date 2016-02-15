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
"in vec3 p;" // position
"in vec3 b;" // binormal
"in vec3 t;" // tangent
"in vec3 n;" // normal
"in float texn;" // texture number
"in vec2 tc;"
"uniform mat4 mat;"
"out mat3 fbtn;"
"out vec3 ftc;"
"void main()"
"{"
	"fbtn=mat3(b,t,n);"
	"ftc=vec3(tc,texn+0.1);"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

const char world_shader_f[]=
VERSION_HEADER
"uniform sampler2DArray tex;"
"uniform sampler2DArray nmap;" // normal map
"out vec4 c_;"
"out vec4 n_;"
"in mat3 fbtn;"
"in vec3 ftc;"
"void main()"
"{"
	"c_=texture(tex,ftc);"
	"vec3 n=texture(nmap, ftc).xyz*2.0-vec3(1.0,1.0,1.0);"
	"n_=vec4(normalize(fbtn*n)*0.5+vec3(0.5,0.5,0.5),0.0);"
"}"
;

// monster shader
const char monster_shader_v[]=
VERSION_HEADER
"in vec3 p;"
"in vec3 n;"
"in vec2 tc;"
"uniform mat4 mat;"
"uniform mat3 nmat;"
"out vec3 fn;"
"out vec2 ftc;"
"void main()"
"{"
	"fn=(nmat*n)*0.5+vec3(0.5,0.5,0.5);"
	"ftc=tc;"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

const char monster_shader_f[]=
VERSION_HEADER
"uniform sampler2DArray tex;"
"uniform float texn;"
"out vec4 c_;"
"out vec4 n_;"
"in vec3 fn;"
"in vec2 ftc;"
"void main()"
"{"
	"c_=texture(tex,vec3(ftc,texn));"
	"n_=vec4(fn,1.0);"
"}"
;

// plasma ball shader
const char plasma_ball_shader_v[]=
VERSION_HEADER
"in vec3 p;"
"in vec3 c;"
"uniform mat4 mat;"
"out vec3 fc;"
"void main()"
"{"
	"fc=c;"
	"vec4 v=mat*vec4(p,1.0);"
	"gl_Position=v;"
	"gl_PointSize=64.0/v.w;"
"}"
;

const char plasma_ball_shader_f[]=
VERSION_HEADER
"in vec3 fc;"
"out vec4 c_;"
"void main()"
"{"
	"vec2 r=gl_PointCoord.xy-vec2(0.5,0.5);"
	"c_=vec4(fc,max(1.0-4.0*dot(r,r),0.0));"
"}"
;

const char fullscreen_postprocessing_shader_v[]=
VERSION_HEADER
"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,1.0),vec2(1.0,0.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"
"void main()"
"{"
	"gl_Position=vec4(coord[gl_VertexID]*2.0-vec2(1.0,1.0),-1.0,1.0);"
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
	"gl_FragDepth=texelFetch(dtex,tc,0).x;"
	"vec4 t=texelFetch(atex,tc,0);"
	// alpha = 1.0 - material is diffuse
	// alpha = 0.0 - material is emissive
	"c_=(0.1+(1.0-t.a))*t;"
"}"
;

const char postprocessing_shader_v[]=
VERSION_HEADER
"in vec3 p;"
"uniform mat4 mat;"
"void main()"
"{"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;

/*
light = light_intensity / (distanse * distance ) - lsb
lsb - min valuable light.
For RGBA8 color buffer min valuable light is near 1 / 256, but you can make it greater, if you wish more perfomance.
*/
const char postprocessing_shader_f[]=
VERSION_HEADER
"uniform sampler2D atex;" // albedo buffer
"uniform sampler2D ntex;" // normals buffer
"uniform sampler2D dtex;" // depth buffer
"uniform vec3 lp;" // light position
"uniform vec4 lsb;" // value, substructed from 1 / dist*dist
"uniform vec4 lc;" // light color (used only rgb )
"uniform mat4 imat;" // onverse matrix for transofrmation
// numbers from raw perspective mat
"uniform float m10;"
"uniform float m14;"

"out vec4 c_;"

"vec4 GWP(ivec2 tc)" // GetWorldPosition
"{"
	"vec4 p;"
	"p.xy=2.0*gl_FragCoord.xy/vec2(textureSize(atex,0))-vec2(1.0,1.0);"
	"p.z=2.0*texelFetch(dtex,tc,0).x-1.0;"

	"p.w=m14/(p.z-m10);"
	"p.xyz*=p.w;"

	"return imat*p;"
"}"

"void main()"
"{"
	"ivec2 tc=ivec2(gl_FragCoord.xy);"
	"vec3 vtl=lp-GWP(tc).xyz;" // vector to light source

	"vec3 n=texelFetch(ntex,tc,0).xyz*vec3(2.0,2.0,2.0)-vec3(1.0,1.0,1.0);" // normal
	"float nk= max(0.0,dot(n,normalize(vtl)));" // light angle cos

	// alpha = 1.0 - material is diffuse
	// alpha = 0.0 - material is emissive
	"vec4 t=texelFetch(atex,tc,0);"
	"c_=max(lc/dot(vtl,vtl)-lsb,vec4(0.0,0.0,0.0,0.0))*t*(t.a*nk);"
"}"
;

extern const char* const tonemapping_shader_v= fullscreen_postprocessing_shader_v;
extern const char tonemapping_shader_f[]=
VERSION_HEADER
"uniform sampler2D tex;"
"out vec4 c_;"
"void main()"
"{"
	"vec4 c=vec4(1.0,1.0,1.0,1.0)-exp(-texelFetch(tex,ivec2(gl_FragCoord.xy),0)*1.0);" // tonemapping

	// color correction - mix between color and grayscale color.
	// 0 - full grayscale, 1 - full color, 1.5 - much more colors
	"float g=(c.x+c.y+c.z)*0.333;"
	"c_=mix(vec4(g,g,g,g),c,1.5);"
"}"
;

} // namespace mx_Shaders