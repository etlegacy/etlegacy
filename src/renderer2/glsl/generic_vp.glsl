/* generic_vp.glsl */
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
#if defined(USE_TCGEN_LIGHTMAP)
attribute vec4 attr_TexCoord1;
#endif // USE_TCGEN_LIGHTMAP

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_Color;
uniform vec4 u_ColorModulate;
uniform mat4 u_ColorTextureMatrix;
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES
#if defined(USE_TCGEN_ENVIRONMENT)
uniform vec3 u_ViewOrigin;
#endif // USE_TCGEN_ENVIRONMENT
#if defined(USE_PORTAL_CLIPPING)
uniform vec4 u_PortalPlane;
#endif// USE_PORTAL_CLIPPING


varying vec4 var_Color;
varying vec2 var_Tex;
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
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	var_Color = attr_Color * u_ColorModulate + u_Color;

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

#if defined(USE_PORTAL_CLIPPING)
	// transform position into world space
	vec3 varPosition = mat3(u_ModelMatrix) * position.xyz;
	// in front, or behind, the portalplane
	var_BackSide = dot(varPosition.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
