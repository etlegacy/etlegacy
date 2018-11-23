/* liquid_vp.glsl */

attribute vec4 attr_Position;
attribute vec4 attr_Color;
attribute vec3 attr_Normal;
#if defined(USE_NORMAL_MAPPING)
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
attribute vec4 attr_TexCoord0;
#endif // USE_NORMAL_MAPPING

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_LightDir;
uniform mat4 u_NormalTextureMatrix;

varying vec4 var_LightColor;
varying vec3 var_LightDirection;
varying vec3 var_Position;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec2 var_TexNormal; // these coords are never moving
#if defined(USE_WATER)
varying vec2 var_TexNormal2; // these coords might be moving (tcMod)
#endif // USE_WATER
#endif // USE_NORMAL_MAPPING

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = (u_ModelMatrix * attr_Position).xyz;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexNormal.xy = attr_TexCoord0.st;
#if defined(USE_WATER)
	var_TexNormal2.xy = (u_NormalTextureMatrix * attr_TexCoord0).st;
#endif // USE_WATER

	var_Tangent.xyz  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;
#endif // USE_NORMAL_MAPPING

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

	var_LightColor     = attr_Color;
	var_LightDirection = u_LightDir;
}
