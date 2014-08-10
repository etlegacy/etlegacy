/* depthToColor_vp.glsl */
#include "lib/vertexSkinning"

attribute vec4 attr_Position;
attribute vec3 attr_Normal;
uniform mat4   u_ModelViewProjectionMatrix;

void main()
{
#if defined(USE_VERTEX_SKINNING)
	{
		vec4 vertex = vec4(0.0);
		vec4 position;
		vec3 normal;

		VertexSkinning_P_N(attr_Position, attr_Normal, position, normal);

		// transform vertex position into homogenous clip-space
		gl_Position = u_ModelViewProjectionMatrix * vertex;
	}
#else
	{
		// transform vertex position into homogenous clip-space
		gl_Position = u_ModelViewProjectionMatrix * attr_Position;
	}
#endif
}
