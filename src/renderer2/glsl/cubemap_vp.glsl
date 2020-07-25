/* cubemap_vp.glsl */

attribute vec4 attr_Position;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

varying vec3 var_ViewDirW;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	vec3 posW = (u_ModelMatrix * attr_Position).xyz;

	// the viewdirection in world space
	var_ViewDirW = normalize(posW - u_ViewOrigin);
}
