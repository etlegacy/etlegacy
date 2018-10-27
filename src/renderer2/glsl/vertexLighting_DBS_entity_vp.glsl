/* vertexLighting_DBS_entity_vp.glsl */
#include "lib/vertexSkinning"
#include "lib/vertexAnimation"
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
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

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_DiffuseTextureMatrix;
uniform vec3  u_LightColor;
uniform float u_Time;
uniform float u_VertexInterpolation;
#if defined(USE_NORMAL_MAPPING)
uniform mat4 u_NormalTextureMatrix;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
uniform mat4 u_SpecularTextureMatrix;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec2 var_TexDiffuse;
varying vec4 var_LightColor;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying vec2 var_TexNormal;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
varying vec3 var_Tangent;
varying vec3 var_Binormal;
#endif // USE_NORMAL_MAPPING


void main()
{
	vec4 position;
	vec3 tangent;
	vec3 binormal;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)

#if defined(USE_NORMAL_MAPPING)
	VertexSkinning_P_TBN(attr_Position, attr_Tangent, attr_Binormal, attr_Normal,
	                     position, tangent, binormal, normal);
	#else
	VertexSkinning_P_N(attr_Position, attr_Normal,
	                   position, normal);
#endif // USE_NORMAL_MAPPING

#elif defined(USE_VERTEX_ANIMATION)

#if defined(USE_NORMAL_MAPPING)
	VertexAnimation_P_TBN(attr_Position, attr_Position2,
	                      attr_Tangent, attr_Tangent2,
	                      attr_Binormal, attr_Binormal2,
	                      attr_Normal, attr_Normal2,
	                      u_VertexInterpolation,
	                      position, tangent, binormal, normal);
#else // USE_NORMAL_MAPPING
	VertexAnimation_P_N(attr_Position, attr_Position2,
	                    attr_Normal, attr_Normal2,
	                    u_VertexInterpolation,
	                    position, normal);
#endif // USE_NORMAL_MAPPING

#else // USE_VERTEX_ANIMATION
	position = attr_Position;

#if defined(USE_NORMAL_MAPPING)
	tangent  = attr_Tangent;
	binormal = attr_Binormal;
#endif // USE_NORMAL_MAPPING

	normal = attr_Normal;
#endif // USE_VERTEX_ANIMATION, USE_VERTEX_SKINNING

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position,
	                           normal,
	                           attr_TexCoord0.st,
	                           u_Time);
#endif // USE_DEFORM_VERTEXES

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	// transform diffusemap texcoords
	var_TexDiffuse = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

	var_Normal.xyz = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz  = (u_ModelMatrix * vec4(tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(binormal, 0.0)).xyz;

	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * attr_TexCoord0).st;

#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING


	var_LightColor = vec4(u_LightColor, 1.0);
}
