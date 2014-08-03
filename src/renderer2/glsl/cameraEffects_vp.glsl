/* cameraEffects_vp.glsl */
attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ColorTextureMatrix;

varying vec2 var_Tex;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	var_Tex = (u_ColorTextureMatrix * attr_TexCoord0).st;
}
