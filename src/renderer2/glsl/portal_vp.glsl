/* portal_vp.glsl */

attribute vec4 attr_Position;
attribute vec4 attr_Color;

uniform mat4 u_ModelViewMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

varying vec3 var_Position;
varying vec4 var_Color;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform vertex position into camera space
	var_Position = (u_ModelViewMatrix * attr_Position).xyz;

	// assign color
	var_Color = attr_Color;
}
