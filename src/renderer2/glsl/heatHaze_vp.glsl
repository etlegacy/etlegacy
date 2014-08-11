/* heatHaze_vp.glsl */
#include "lib/vertexSkinning"
#include "lib/vertexAnimation"
#include "lib/deformVertexes"

attribute vec4 attr_Position;
attribute vec3 attr_Normal;
attribute vec4 attr_TexCoord0;

attribute vec4 attr_Position2;
attribute vec3 attr_Normal2;

uniform float u_VertexInterpolation;

uniform float u_Time;

uniform mat4 u_NormalTextureMatrix;
uniform mat4 u_ProjectionMatrixTranspose;
uniform mat4 u_ModelViewMatrixTranspose;
uniform mat4 u_ModelViewProjectionMatrix;

uniform float u_DeformMagnitude;

varying vec2  var_TexNormal;
varying float var_Deform;

void main()
{
	vec4  deformVec;
	float d1, d2;

	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)
	VertexSkinning_P_N(attr_Position, attr_Normal, position, normal);
#elif defined(USE_VERTEX_ANIMATION)
	VertexAnimation_P_N(attr_Position, attr_Position2, attr_Normal, attr_Normal2, u_VertexInterpolation, position, normal);
#else
	position = attr_Position;
	normal   = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(position, normal, attr_TexCoord0.st, u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// take the deform magnitude and scale it by the projection distance
	deformVec   = vec4(1, 0, 0, 1);
	deformVec.z = dot(u_ModelViewMatrixTranspose[2], position);

	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * attr_TexCoord0).st;

	d1 = dot(u_ProjectionMatrixTranspose[0], deformVec);
	d2 = dot(u_ProjectionMatrixTranspose[3], deformVec);

	// clamp the distance so the the deformations don't get too wacky near the view
	var_Deform = min(d1 * (1.0 / max(d2, 1.0)), 0.02) * u_DeformMagnitude;
}
