/* liquid_vp.glsl */
#if defined(USE_DEFORM_VERTEXES)
#include "lib/deformVertexes"
#endif // USE_DEFORM_VERTEXES

attribute vec4 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
#endif // USE_NORMAL_MAPPING

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_ColorModulate;
#if defined(USE_DIFFUSE)
uniform mat4 u_DiffuseTextureMatrix;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
uniform vec3 u_ViewOrigin;
uniform vec3 u_LightDir;
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

varying vec4 var_LightColor;
varying float var_alphaGen;
varying vec3 var_Position;
varying vec3 var_Normal;
#if defined(USE_DIFFUSE)
varying vec2 var_TexDiffuse; // possibly moving coords
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
varying vec3 var_ViewOrigin;
varying mat3 var_tangentMatrix;
varying vec2 var_TexNormal; // these coords are never moving
varying vec3 var_LightDirection;
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
	vec4 position = attr_Position;

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, attr_Normal, attr_TexCoord0.st, u_Time);
#endif // USE_DEFORM_VERTEXES

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	var_LightColor = attr_Color;

#if defined(USE_DIFFUSE)
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

	// the alpha value is the one set by alphaGen const <value>
	var_alphaGen = u_ColorModulate.a * 0.5 + 0.5;
#endif // USE_DIFFUSE

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	vec3 tangent = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	vec3 binormal = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// in a vertex-shader there exists no gl_FrontFacing
	var_tangentMatrix = mat3(-tangent, -binormal, -var_Normal.xyz);

	// normalmap texcoords are not transformed
	var_TexNormal = attr_TexCoord0.st;

	var_LightDirection = -normalize(u_LightDir);

	var_ViewOrigin = normalize(var_Position - u_ViewOrigin);

#if defined(USE_PARALLAX_MAPPING)
	vec3 viewOrigin2 = normalize(var_tangentMatrix * var_ViewOrigin);
	var_S = viewOrigin2.xy * -u_DepthScale / viewOrigin2.z;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
