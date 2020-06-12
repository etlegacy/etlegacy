/* forwardLighting_vp.glsl */
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
attribute vec4 attr_Color;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Normal;
#if defined(USE_VERTEX_ANIMATION)
attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;
#endif // USE_VERTEX_ANIMATION

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
#if !defined(LIGHT_DIRECTIONAL)
uniform mat4 u_LightAttenuationMatrix;
#endif // LIGHT_DIRECTIONAL
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

varying vec3 var_Position;
varying vec4 var_Color;
#if !defined(LIGHT_DIRECTIONAL)
varying vec4 var_TexAttenuation;
#endif // LIGHT_DIRECTIONAL
varying vec4 var_Normal;
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)
	VertexSkinning_PN(attr_Position, attr_Normal, position, normal);
#elif defined(USE_VERTEX_ANIMATION)
	VertexAnimation_PN(attr_Position, attr_Position2, attr_Normal, attr_Normal2, u_VertexInterpolation, position, normal);
#else
	position = attr_Position;
	normal = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, normal, attr_TexCoord0.st, u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

#if !defined(LIGHT_DIRECTIONAL)
	// calc light xy,z attenuation in light space
	var_TexAttenuation = u_LightAttenuationMatrix * position;
#endif // LIGHT_DIRECTIONAL

	// assign color
	vec4 color = attr_Color * u_ColorModulate + u_Color;

	var_Normal.xyz = mat3(u_ModelMatrix) * normal;

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
