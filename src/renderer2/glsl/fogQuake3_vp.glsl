/* fogQuake3_vp.glsl */
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
attribute vec4 attr_Color;
#if defined(USE_VERTEX_ANIMATION)
attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
attribute vec4 attr_TexCoord0;
#endif // USE_DEFORM_VERTEXES

uniform mat4  u_ModelMatrix;
uniform mat4  u_ModelViewProjectionMatrix;
uniform vec4  u_ColorModulate;
uniform vec4  u_Color;
uniform vec4  u_FogDistanceVector;
uniform vec4  u_FogDepthVector;
#if defined(EYE_OUTSIDE)
uniform float u_FogEyeT;
#endif // EYE_OUTSIDE
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES

#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

varying vec3 var_Position;
varying vec2 var_Tex;
varying vec4 var_Color;
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING

void main()
{
	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)
	VertexSkinning_PN(attr_Position, attr_Normal,
	                  position,      normal);
#elif defined(USE_VERTEX_ANIMATION)
	VertexAnimation_PN(attr_Position, attr_Position2,
	                   attr_Normal,   attr_Normal2,
					   u_VertexInterpolation,
					   position,      normal);
#else
	position = attr_Position;
	normal   = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, normal, attr_TexCoord0.st, u_Time);
#endif // USE_DEFORM_VERTEXES

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// calculate the length in fog
	float s = dot(position.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	float t = dot(position.xyz, u_FogDepthVector.xyz) + u_FogDepthVector.w;

	// partially clipped fogs use the T axis
#if defined(EYE_OUTSIDE)
	if (t < 1.0)
	{
		//t = 1.0 / 32.0; // point is outside, so no fogging
		t = 0.03125; // 1.0 / 32.0   // is this optimized by a compiler?..
	}
	else
	{
		//t = 1.0 / 32.0 + 30.0 / 32.0 * t / (t - u_FogEyeT); // cut the distance at the fog plane
		t = 0.03125 + 0.9375 * t / (t - u_FogEyeT); // 1.0 / 32.0 + 30.0 / 32.0 * t / (t - u_FogEyeT)
	}
#else
	if (t < 0.0)
	{
		//t = 1.0 / 32.0; // point is outside, so no fogging
		t = 0.03125; // 1.0 / 32.0
	}
	else
	{
		//t = 31.0 / 32.0;
		t = 0.9375; // 31.0 / 32.0
	}
#endif

	var_Tex = vec2(s, t);

	var_Color =  attr_Color * u_ColorModulate + u_Color;

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
