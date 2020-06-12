/* entity_vp.glsl */
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
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
#if defined(USE_VERTEX_ANIMATION)
attribute vec3 attr_Tangent2;
attribute vec3 attr_Binormal2;
#endif // USE_VERTEX_ANIMATION
#endif // USE_NORMAL_MAPPING

uniform mat4  u_ModelMatrix;
uniform mat4  u_ModelViewProjectionMatrix;
uniform mat4  u_DiffuseTextureMatrix;
#if defined(USE_NORMAL_MAPPING)
uniform vec3  u_ViewOrigin;
uniform vec3  u_LightDir;
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING
#if defined(USE_DEFORM_VERTEXES)
uniform float u_Time;
#endif // USE_DEFORM_VERTEXES
#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexInterpolation;
#endif // USE_VERTEX_ANIMATION

varying vec2 var_TexDiffuse;
varying vec3 var_Position;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec3 var_LightDirection;
varying vec3 var_ViewDirW;           // view direction in world space
#if defined(USE_PARALLAX_MAPPING)
varying vec3 var_ViewDirT;           // view direction in tangentspace
varying float var_distanceToCam;     //
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
	vec4 position;
	vec3 normal;
#if defined(USE_NORMAL_MAPPING)
	vec3 tangent;
	vec3 binormal;
#endif // USE_NORMAL_MAPPING


#if defined(USE_VERTEX_SKINNING)

#if defined(USE_NORMAL_MAPPING)
	VertexSkinning_PTBN(attr_Position, attr_Tangent, attr_Binormal, attr_Normal,
	                    position,      tangent,      binormal,      normal);
#else
	VertexSkinning_PN(attr_Position, attr_Normal,
	                  position,      normal);
#endif // USE_NORMAL_MAPPING

#elif defined(USE_VERTEX_ANIMATION)

#if defined(USE_NORMAL_MAPPING)
	VertexAnimation_PTBN(attr_Position, attr_Position2,
	                     attr_Tangent,  attr_Tangent2,
	                     attr_Binormal, attr_Binormal2,
	                     attr_Normal,   attr_Normal2,
	                     u_VertexInterpolation,
	                     position, tangent, binormal, normal);
#else // USE_NORMAL_MAPPING
	VertexAnimation_PN(attr_Position, attr_Position2,
	                   attr_Normal,   attr_Normal2,
	                   u_VertexInterpolation,
	                   position,      normal);
#endif // USE_NORMAL_MAPPING

#else // USE_VERTEX_ANIMATION

	position = attr_Position;
	normal   = attr_Normal;
#if defined(USE_NORMAL_MAPPING)
	tangent  = attr_Tangent;
	binormal = attr_Binormal;
#endif // USE_NORMAL_MAPPING
#endif // USE_VERTEX_ANIMATION, USE_VERTEX_SKINNING

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, normal, attr_TexCoord0.st, u_Time);
#endif // USE_DEFORM_VERTEXES

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// transform diffusemap texcoords
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

	var_Normal.xyz = (u_ModelMatrix * vec4(normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	tangent  = normalize((u_ModelMatrix * vec4(tangent, 0.0)).xyz);
	binormal = normalize((u_ModelMatrix * vec4(binormal, 0.0)).xyz);

	var_tangentMatrix = mat3(-tangent, -binormal, -var_Normal.xyz);
//var_tangentMatrix = transpose(mat3(tangent, binormal, var_Normal.xyz)); // somehow this looks weird.. TODO: check

	var_LightDirection = -normalize(u_LightDir);

	vec3 viewVec = var_Position - u_ViewOrigin;
	var_ViewDirW = normalize(viewVec);

#if defined(USE_PARALLAX_MAPPING)
	var_distanceToCam = length(viewVec);
	mat3 tangentMatrix = transpose(mat3(tangent, binormal, var_Normal.xyz)); // this works with parallax, but specular is woot
	var_ViewDirT = tangentMatrix * viewVec; // do not normalize..
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
