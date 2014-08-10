/* shadowFill_vp.glsl */
#include "lib/vertexSkinning"
#include "lib/vertexAnimation"
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec3 attr_Normal;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_Color;

attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;

uniform float u_VertexInterpolation;

uniform vec4 u_Color;

uniform mat4 u_ColorTextureMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

uniform float u_Time;

varying vec3 var_Position;
varying vec2 var_Tex;
varying vec4 var_Color;

void main()
{
	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)

	VertexSkinning_P_N(attr_Position, attr_Normal,
	                   position, normal);

#elif defined(USE_VERTEX_ANIMATION)

	VertexAnimation_P_N(attr_Position, attr_Position2,
	                    attr_Normal, attr_Normal2,
	                    u_VertexInterpolation,
	                    position, normal);

#else
	position = attr_Position;
	normal   = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position,
	                           normal,
	                           attr_TexCoord0.st,
	                           u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
	// transform position into world space
#if defined(LIGHT_DIRECTIONAL)
	var_Position = gl_Position.xyz / gl_Position.w;
#else
	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;
#endif

	// transform texcoords
	var_Tex = (u_ColorTextureMatrix * attr_TexCoord0).st;

	// assign color
	var_Color = u_Color;
}
