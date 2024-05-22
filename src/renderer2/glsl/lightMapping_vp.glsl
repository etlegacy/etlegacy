/* lightMapping_vp.glsl */
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_TexCoord1;
attribute vec4 attr_Color;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
#endif // USE_NORMAL_MAPPING

uniform mat4 u_DiffuseTextureMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
uniform vec3 u_LightDir;
uniform vec3  u_LightColor;
uniform float u_Time;
uniform vec3 u_ViewOrigin;
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
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4  u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexLight;
varying vec4 var_Color;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
varying vec3 var_LightDirection;
varying vec3 var_ViewOrigin; // position - vieworigin
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S;
varying vec3 var_ViewOrigin2; // in worldspace
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
#if 1
	gl_Position = u_ModelViewProjectionMatrix * position;
#else // 1
	gl_Position.xy = attr_TexCoord1 * 2.0 - 1.0;
	gl_Position.z  = 0.0;
	gl_Position.w  = 1.0;
#endif // 1

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// transform diffusemap texcoords
	var_TexDiffuseNormal.st = (u_DiffuseTextureMatrix * attr_TexCoord0).st;
	// get lightmap texture coordinates
	var_TexLight = attr_TexCoord1.st;

	// transform tangentspace axis
	var_Normal.xyz = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
#if defined(USE_NORMAL_MAPPING)
	vec3 tangent  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	vec3 binormal = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// in a vertex-shader there exists no gl_FrontFacing
	var_tangentMatrix = mat3(-tangent, -binormal, -var_Normal.xyz);
//var_tangentMatrix = mat3(tangent, binormal, var_Normal.xyz);

	// transform normalmap texcoords
	var_TexDiffuseNormal.pq = (u_NormalTextureMatrix * attr_TexCoord0).st;

#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	// transform specularmap texcoords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
#endif // USE_REFLECTIONS || USE_SPECULAR

	var_LightDirection = -normalize(u_LightDir);
//var_LightDirection = normalize(var_tangentMatrix * -u_LightDir);
//var_LightDirection = -vec3(dot(u_LightDir,tangent), dot(u_LightDir,binormal), dot(u_LightDir,var_Normal.xyz));

	// the vieworigin
	var_ViewOrigin = normalize(var_Position - u_ViewOrigin);
//	var_ViewOrigin = normalize(u_ViewOrigin - var_Position);
//var_ViewOrigin = normalize(var_tangentMatrix * (var_Position - u_ViewOrigin));
//var_ViewOrigin = vec3(dot((u_ViewOrigin - var_Position),tangent), dot((u_ViewOrigin - var_Position),binormal), dot((u_ViewOrigin - var_Position),var_Normal.xyz));


#if defined(USE_PARALLAX_MAPPING)
//	vec3 viewOrigin2 = normalize(var_tangentMatrix * var_ViewOrigin);
var_ViewOrigin2 = vec3(dot(var_ViewOrigin,tangent), dot(var_ViewOrigin,binormal), dot(var_ViewOrigin,var_Normal.xyz));
//vec3 viewOrigin2 = var_ViewOrigin;
	var_S = var_ViewOrigin2.xy / var_ViewOrigin2.z * u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

#if defined(USE_PORTAL_CLIPPING)
	// in front, or behind, the portalplane
	var_BackSide = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
#endif // USE_PORTAL_CLIPPING

	// assign color
	var_Color = attr_Color * u_ColorModulate + u_Color;
}
