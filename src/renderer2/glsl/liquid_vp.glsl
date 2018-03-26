/* liquid_vp.glsl */

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Tangent;
attribute vec3 attr_Binormal;
attribute vec3 attr_Normal;
attribute vec4 attr_Color;
attribute vec3 attr_LightDirection;

uniform mat4 u_NormalTextureMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

varying vec3 var_Position;
varying vec2 var_TexNormal;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;
varying vec4 var_LightColor;
varying vec3 var_LightDirection;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = (u_ModelMatrix * attr_Position).xyz;

	// transform normalmap texcoords
	var_TexNormal.xy = (u_NormalTextureMatrix * attr_TexCoord0).st;

	var_Tangent.xyz  = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

	var_LightColor     = attr_Color;
	var_LightDirection = attr_LightDirection;
}
