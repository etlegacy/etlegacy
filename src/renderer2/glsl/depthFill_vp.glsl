/* depthFill_vp.glsl */

attribute vec4 attr_Position;
attribute vec3 attr_Normal;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_Color;

uniform mat4 u_ColorTextureMatrix;
uniform vec3 u_AmbientColor;
uniform mat4 u_ModelViewProjectionMatrix;

uniform int   u_DeformGen;
uniform vec4  u_DeformWave;         // [base amplitude phase freq]
uniform vec3  u_DeformBulge;        // [width height speed]
uniform float u_DeformSpread;
uniform float u_Time;

varying vec2 var_Tex;
varying vec4 var_Color;

void main()
{
#if defined(USE_VERTEX_SKINNING)
	vec4 position = vec4(0.0);

	for (int i = 0; i < 4; i++)
	{
		int   boneIndex  = int(attr_BoneIndexes[i]);
		float boneWeight = attr_BoneWeights[i];
		mat4  boneMatrix = u_BoneMatrix[boneIndex];

		position += (boneMatrix * attr_Position) * boneWeight;
	}

	//position = DeformPosition(position, attr_Normal, attr_TexCoord0.st);
#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(u_DeformGen,
	                          u_DeformWave,     // [base amplitude phase freq]
	                          u_DeformBulge,    // [width height speed]
	                          u_DeformSpread,
	                          u_Time,
	                          position,
	                          attr_Normal,
	                          attr_TexCoord0.st);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
#else

	vec4 position = DeformPosition(attr_Position, attr_Normal, attr_TexCoord0.st);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
#endif

	// transform texcoords
	var_Tex = (u_ColorTextureMatrix * attr_TexCoord0).st;

#if defined(r_precomputedLighting)
	// assign color
	var_Color = attr_Color;
#else
	var_Color = vec4(u_AmbientColor, 1.0);
#endif
}
