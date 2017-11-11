/* skybox_vp.glsl */

attribute vec4 attr_Position;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

varying vec3 var_Position;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = (u_ModelMatrix * attr_Position).xyz;
}
