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
uniform vec3 u_LightDir;
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
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_LightColor;
varying vec4 var_TexDiffuseNormal;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying vec3 var_Tangent;
varying vec3 var_Binormal;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING

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
	var_Tangent.xyz  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// transform normalmap texcoords
	var_TexDiffuseNormal.pq = (u_NormalTextureMatrix * attr_TexCoord0).st;

#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING

	// assign color
	var_LightColor = attr_Color * u_ColorModulate + u_Color;
}
