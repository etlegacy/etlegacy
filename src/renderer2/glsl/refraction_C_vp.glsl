/* refraction_C_vp.glsl */
#include "lib/vertexSkinning"

attribute vec4 attr_Position;
attribute vec3 attr_Normal;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

varying vec3 var_Position;
varying vec3 var_Normal;

void main()
{
	vec4 vertex;
	vec3 normal;
	vec4 position;

#if defined(USE_VERTEX_SKINNING)

	VertexSkinning_P_N(attr_Position, attr_Normal, position, normal);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * vertex;

	// transform position into world space
	var_Position = mat3(u_ModelMatrix) * position.xyz;

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;

	// transform position into world space
	var_Position = mat3(u_ModelMatrix) * attr_Position.xyz;

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
}
