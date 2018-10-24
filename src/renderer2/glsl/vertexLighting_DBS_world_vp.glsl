/* vertexLighting_DBS_world_vp.glsl */
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
attribute vec3 attr_Normal;
attribute vec4 attr_Color;

uniform mat4 u_DiffuseTextureMatrix;
uniform mat4 u_NormalTextureMatrix;
uniform mat4 u_SpecularTextureMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

uniform float u_Time;

uniform vec4 u_ColorModulate;
uniform vec4 u_Color;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexSpecular;
varying vec4 var_LightColor;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;

void main()
{
	vec4 position = attr_Position;
#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position,
	                           attr_Normal,
	                           attr_TexCoord0.st,
	                           u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// assign position in object space
	var_Position = position.xyz;

	// transform diffusemap texcoords
	var_TexDiffuseNormal.st = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexDiffuseNormal.pq = (u_NormalTextureMatrix * attr_TexCoord0).st;

	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
#endif

	// assign color
	var_LightColor = attr_Color * u_ColorModulate + u_Color;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent  = attr_Tangent;
	var_Binormal = attr_Binormal;
#endif

	var_Normal = attr_Normal;
}
