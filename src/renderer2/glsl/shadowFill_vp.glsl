/* shadowFill_vp.glsl */

#if defined(USE_VERTEX_SKINNING)
#include "lib/vertexSkinning"
#endif // USE_VERTEX_SKINNING
#if defined(USE_VERTEX_ANIMATION)
#include "lib/vertexAnimation"
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
#include "lib/deformVertexes"
#endif // USE_DEFORM_VERTEXES

attribute vec4 attr_TexCoord0;
attribute vec4 attr_Position;
attribute vec3 attr_Normal;
#if defined(USE_VERTEX_ANIMATION)
attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;
#endif // USE_VERTEX_ANIMATION

#if defined(USE_ALPHA_TESTING)
uniform mat4  u_ColorTextureMatrix;
#endif // USE_ALPHA_TESTING
uniform mat4  u_ModelMatrix;
uniform mat4  u_ModelViewProjectionMatrix;
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES

#if defined(VSM) || defined(EVSM) || defined(ESM)
#if !defined(LIGHT_DIRECTIONAL)
uniform vec3  u_LightOrigin;
uniform float u_LightRadius;

varying float var_Distance;
#if defined(VSM)
varying float var_DistanceSquared;
#endif
#endif // LIGHT_DIRECTIONAL
#endif // VSM, EVSM, ESM

varying vec3 var_Position;
#if defined(USE_ALPHA_TESTING)
varying vec2 var_Tex;
#endif // USE_ALPHA_TESTING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING

#if !defined(LIGHT_DIRECTIONAL)
#if defined(EVSM)
#if !defined(r_EVSMPostProcess)
varying vec4 var_FragColorEVSM;


vec2 WarpDepth(float depth)
{
	// rescale depth into [-1, 1]
	depth = 2.0 * depth - 1.0;
	float pos = exp(r_EVSMExponents.x * depth);
	float neg = -exp(-r_EVSMExponents.y * depth);

	return vec2(pos, neg);
}

vec4 ShadowDepthToEVSM(float depth)
{
	vec2 warpedDepth = WarpDepth(depth);
	return vec4(warpedDepth.xy, warpedDepth.xy * warpedDepth.xy);
}
#endif // r_EVSMPostProcess
#endif // EVSM
#endif // LIGHT_DIRECTIONAL



void main()
{
	vec4 position;
	vec3 normal;


#if defined(USE_VERTEX_SKINNING)
	VertexSkinning_P_N(attr_Position, attr_Normal,
	                   position, normal);

#elif defined(USE_VERTEX_ANIMATION)
	VertexAnimation_P_N(attr_Position, attr_Position2, attr_Normal, attr_Normal2, u_VertexInterpolation,
	                    position, normal);
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
#if defined(LIGHT_DIRECTIONAL)
	var_Position = gl_Position.xyz / gl_Position.w;
#else
	var_Position = (u_ModelMatrix * position).xyz;
#endif

#if defined(VSM) || defined(EVSM) || defined(ESM)
#if !defined(LIGHT_DIRECTIONAL)
	var_Distance = length(var_Position - u_LightOrigin) / u_LightRadius;
#if defined(VSM)
	var_DistanceSquared = var_Distance * var_Distance;
#endif
#endif // LIGHT_DIRECTIONAL
#endif // VSM, EVSM, ESM

#if !defined(LIGHT_DIRECTIONAL)
#if defined(EVSM)
#if !defined(r_EVSMPostProcess)
	var_FragColorEVSM = ShadowDepthToEVSM(var_Distance);
#endif // r_EVSMPostProcess
#endif // EVSM
#endif // LIGHT_DIRECTIONAL

#if defined(USE_ALPHA_TESTING)
	// transform texcoords
	var_Tex = (u_ColorTextureMatrix * attr_TexCoord0).st;
#endif // USE_ALPHA_TESTING

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
