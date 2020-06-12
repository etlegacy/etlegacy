/* depthFill_vp.glsl */
#if defined(USE_VERTEX_SKINNING)
#include "lib/vertexSkinning"
#endif // USE_VERTEX_SKINNING
#if defined(USE_VERTEX_ANIMATION)
#include "lib/vertexAnimation"
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
#include "lib/deformVertexes"
#endif // USE_DEFORM_VERTEXES

attribute vec4 attr_Position;
attribute vec3 attr_Normal;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_Color;
#if defined(USE_VERTEX_ANIMATION)
attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;
#endif // USE_VERTEX_ANIMATION

uniform mat4  u_ModelViewProjectionMatrix;
uniform mat4  u_ColorTextureMatrix;
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
uniform int   u_DeformGen;
uniform vec4  u_DeformWave;         // [base amplitude phase freq]
uniform vec3  u_DeformBulge;        // [width height speed]
uniform float u_DeformSpread;
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES
#if defined(USE_TCGEN_ENVIRONMENT)
uniform vec3  u_ViewOrigin;
#endif // USE_TCGEN_ENVIRONMENT

varying vec4 var_Color;
varying vec2 var_Tex;

void main()
{
	vec4 position;
#if defined(USE_VERTEX_SKINNING)
	VertexSkinning_P(attr_Position, position);
#elif defined(USE_VERTEX_ANIMATION)
	VertexAnimation_P(attr_Position, attr_Position2, u_VertexInterpolation, position);
#else
	position = attr_Position;
#endif


#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(u_DeformGen,
	                          u_DeformWave,     // [base amplitude phase freq]
	                          u_DeformBulge,    // [width height speed]
	                          u_DeformSpread,
	                          u_Time,
	                          position,
	                          attr_Normal,
	                          attr_TexCoord0.st);
//#else // USE_DEFORM_VERTEXES
//	vec4 position = DeformPosition(attr_Position, attr_Normal, attr_TexCoord0.st);
#endif // USE_DEFORM_VERTEXES


	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;


	// transform texcoords
	vec4 texCoord;
#if defined(USE_TCGEN_ENVIRONMENT)
	{
		vec3 viewer = normalize(u_ViewOrigin - position.xyz);
		float d = dot(attr_Normal, viewer);
		vec3 reflected = attr_Normal * 2.0 * d - viewer;
		texCoord.s = 0.5 + reflected.y * 0.5;
		texCoord.t = 0.5 - reflected.z * 0.5;
		texCoord.q = 0;
		texCoord.w = 1;
	}
#elif defined(USE_TCGEN_LIGHTMAP)
	texCoord = attr_TexCoord1;
#else
	texCoord = attr_TexCoord0;
#endif // USE_TCGEN_ENVIRONMENT,USE_TCGEN_LIGHTMAP
	var_Tex = (u_ColorTextureMatrix * texCoord).st;

	// assign color
	var_Color = attr_Color;
}
