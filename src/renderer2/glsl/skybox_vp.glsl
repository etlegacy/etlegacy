/* skybox_vp.glsl */

attribute vec4 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_Color;
varying vec2 var_Tex;
uniform vec3 u_ViewOrigin;
uniform mat4 u_ColorTextureMatrix;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
vec4 texCoord;
varying vec3 var_Position;
uniform vec4 u_Color;
//varying vec4 var_Color;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = (u_ModelMatrix * attr_Position).xyz;

	texCoord = attr_TexCoord0;

	var_Tex = (u_ColorTextureMatrix * texCoord).st;

	//var_Color = attr_Color + u_Color;
}
