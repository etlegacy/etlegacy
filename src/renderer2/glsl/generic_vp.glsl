/* generic_vp.glsl */
#include "lib/vertexSkinning"
#include "lib/vertexAnimation"
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_TexCoord1;
attribute vec3 attr_Normal;
attribute vec4 attr_Color;

attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;

uniform float u_VertexInterpolation;

uniform mat4 u_ColorTextureMatrix;
uniform vec3 u_ViewOrigin;
//uniform int  u_TCGen_Environment;

uniform float u_Time;

uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

varying vec3 var_Position;
varying vec2 var_Tex;
varying vec4 var_Color;

void main()
{
	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)
	VertexSkinning_P_N(attr_Position, attr_Normal, position, normal);
#elif defined(USE_VERTEX_ANIMATION)
	VertexAnimation_P_N(attr_Position, attr_Position2, attr_Normal, attr_Normal2, u_VertexInterpolation, position, normal);
#else
	position = attr_Position;
	normal   = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, normal, attr_TexCoord0.st, u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = mat3(u_ModelMatrix) * position.xyz;

	// transform texcoords
	vec4 texCoord;
#if defined(USE_TCGEN_ENVIRONMENT)
	{
		vec3 viewer = normalize(u_ViewOrigin - position.xyz);

		float d = dot(attr_Normal, viewer);

		vec3 reflected = attr_Normal * 2.0 * d - viewer;

		texCoord.s = 0.5 + reflected.x * 0.5;
		texCoord.t = 0.5 - reflected.z * 0.5;
		texCoord.q = 0;
		texCoord.w = 1;
	}
#elif defined(USE_TCGEN_LIGHTMAP)
	texCoord = attr_TexCoord1;
#else
	texCoord = attr_TexCoord0;
#endif

	var_Tex = (u_ColorTextureMatrix * texCoord).st;

	var_Color = attr_Color * u_ColorModulate + u_Color;
}
