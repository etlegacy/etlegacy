/* lightMapping_vp.glsl */
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_TexCoord1;
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
attribute vec3 attr_Normal;
attribute vec4 attr_Color;

uniform mat4 u_DiffuseTextureMatrix;
uniform mat4 u_NormalTextureMatrix;
uniform mat4 u_SpecularTextureMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

uniform float u_Time;

uniform vec4 u_ColorModulate;
uniform vec4 u_Color;
uniform vec3 u_LightDir;

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexSpecular;
varying vec2 var_TexLight;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;
varying vec4 var_Color;

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
#if 1
	gl_Position = u_ModelViewProjectionMatrix * position;
#else
	gl_Position.xy = attr_TexCoord1 * 2.0 - 1.0;
	gl_Position.z  = 0.0;
	gl_Position.w  = 1.0;
#endif


	// transform diffusemap texcoords
	var_TexDiffuseNormal.st = (u_DiffuseTextureMatrix * attr_TexCoord0).st;
	var_TexLight            = attr_TexCoord1.st;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexDiffuseNormal.pq = (u_NormalTextureMatrix * attr_TexCoord0).st;

	// transform specularmap texcoords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
#endif


#if 0

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	var_Normal.xyz = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;
#endif

#else

	var_Position = position.xyz;
	var_Normal   = attr_Normal.xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent  = attr_Tangent.xyz;
	var_Binormal = attr_Binormal.xyz;
#endif

#endif

	var_Color = attr_Color * u_ColorModulate + u_Color;
}
