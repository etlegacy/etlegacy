/* debugShadowMap_vp.glsl */

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;

varying vec2 var_TexCoord;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	var_TexCoord = attr_TexCoord0.st;
}
