/* vertexLighting_DBS_world_vp.glsl */
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
#endif // USE_NORMAL_MAPPING

uniform mat4 u_DiffuseTextureMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ModelMatrix;
uniform float u_Time;
uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
uniform vec3 u_LightColor;
#if defined(USE_NORMAL_MAPPING)
uniform mat4 u_NormalTextureMatrix;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
uniform mat4 u_SpecularTextureMatrix;
#if defined(USE_REFLECTIONS)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif // USE_REFLECTIONS
#endif // USE_REFLECTIONS || USE_SPECULAR
uniform vec3  u_LightDir;
uniform vec3 u_ViewOrigin;
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

varying vec3 var_Position;
varying vec4 var_LightColor;
varying vec4 var_TexDiffuseNormal;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
varying vec3 var_LightDirection;
varying vec3 var_ViewOrigin; // position - vieworigin
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S; // size and start position of search in texture space
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
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform diffusemap texcoords
	var_TexDiffuseNormal.st = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// transform tangentspace axis
	var_Normal.xyz = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
#if defined(USE_NORMAL_MAPPING)
	vec3 tangent  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	vec3 binormal = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// in a vertex-shader there exists no gl_FrontFacing
	var_tangentMatrix = mat3(-tangent, -binormal, -var_Normal.xyz);

	// transform normalmap texcoords
	var_TexDiffuseNormal.pq = (u_NormalTextureMatrix * attr_TexCoord0).st;

#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
#endif // USE_REFLECTIONS || USE_SPECULAR

	// assign color
	var_LightColor = attr_Color * u_ColorModulate + u_Color;
	var_LightDirection = -normalize(u_LightDir);

	var_ViewOrigin = normalize(var_Position - u_ViewOrigin);

#if defined(USE_PARALLAX_MAPPING)
	// transform the vieworigin from tangentspace to worldspace
	vec3 viewOrigin2 = normalize(var_tangentMatrix * var_ViewOrigin);
	var_S = viewOrigin2.xy * -u_DepthScale / viewOrigin2.z;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
//need todo this for r_normalmapping 0 so color wont be 0 and we render black
// assign color
	var_LightColor = attr_Color * u_ColorModulate + u_Color;
#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING
}
