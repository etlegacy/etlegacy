/* cubemap_vp.glsl */
// these cubeprobes are rendered inside out.
// That way they are more easy to compare with the environment around them.

attribute vec4 attr_Position;
attribute vec3 attr_Normal;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

varying vec3 var_Position;
varying vec3 var_Normal;
varying vec3 var_ViewDirW;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = (u_ModelMatrix * attr_Position).xyz;

	// normal in world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

	var_ViewDirW = normalize(var_Position - u_ViewOrigin);
}
