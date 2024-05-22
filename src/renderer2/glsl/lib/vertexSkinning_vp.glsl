/* vertexSkinning_vp.glsl - GPU vertex skinning for skeletal meshes */

attribute vec4 attr_BoneIndexes;
attribute vec4 attr_BoneWeights;
uniform int    u_VertexSkinning;
uniform mat4   u_BoneMatrix[MAX_GLSL_BONES];

void VertexSkinning_P_N(const vec4 inPosition,
                        const vec3 inNormal,
                        inout vec4 position,
                        inout vec3 normal)
{
	position = vec4(0.0, 0.0, 0.0, 1.0);
	normal   = vec3(0.0);

#if 1
	for (int i = 0; i < 4; i++)
	{
		int   boneIndex  = int(attr_BoneIndexes[i]);
		float boneWeight = attr_BoneWeights[i];
		mat4  boneMatrix = u_BoneMatrix[boneIndex];

		position.xyz += (boneMatrix * inPosition).xyz * boneWeight;

		normal += (mat3(boneMatrix) * inNormal) * boneWeight;
	}
#else
	// unrolled version

	// first bone
	int   boneIndex  = int(attr_BoneIndexes[0]);
	float boneWeight = attr_BoneWeights[0];
	mat4  boneMatrix = u_BoneMatrix[boneIndex];

	position.xyz += (boneMatrix * inPosition).xyz * boneWeight;
	normal       += (boneMatrix * vec4(inNormal, 0.0)).xyz * boneWeight;

	// second bone
	boneIndex  = int(attr_BoneIndexes[1]);
	boneWeight = attr_BoneWeights[1];
	boneMatrix = u_BoneMatrix[boneIndex];

	position.xyz += (boneMatrix * inPosition).xyz * boneWeight;
	normal       += (boneMatrix * vec4(inNormal, 0.0)).xyz * boneWeight;

	// third
	boneIndex  = int(attr_BoneIndexes[2]);
	boneWeight = attr_BoneWeights[2];
	boneMatrix = u_BoneMatrix[boneIndex];

	position.xyz += (boneMatrix * inPosition).xyz * boneWeight;
	normal       += (boneMatrix * vec4(inNormal, 0.0)).xyz * boneWeight;

	// fourth
	boneIndex  = int(attr_BoneIndexes[3]);
	boneWeight = attr_BoneWeights[3];
	boneMatrix = u_BoneMatrix[boneIndex];

	position.xyz += (boneMatrix * inPosition).xyz * boneWeight;
	normal       += (boneMatrix * vec4(inNormal, 0.0)).xyz * boneWeight;

#endif

	normal = normalize(normal);
}

void VertexSkinning_P_TBN(const vec4 inPosition,
                          const vec3 inTangent,
                          const vec3 inBinormal,
                          const vec3 inNormal,

                          inout vec4 position,
                          inout vec3 tangent,
                          inout vec3 binormal,
                          inout vec3 normal)
{
	position = vec4(0.0, 0.0, 0.0, 1.0);

	tangent  = vec3(0.0);
	binormal = vec3(0.0);
	normal   = vec3(0.0);

	for (int i = 0; i < 4; i++)
	{
		int   boneIndex  = int(attr_BoneIndexes[i]);
		float boneWeight = attr_BoneWeights[i];
		mat4  boneMatrix = u_BoneMatrix[boneIndex];

		position.xyz += (boneMatrix * inPosition).xyz * boneWeight;

		tangent  += (boneMatrix * vec4(inTangent, 0.0)).xyz * boneWeight;
		binormal += (boneMatrix * vec4(inBinormal, 0.0)).xyz * boneWeight;
		normal   += (boneMatrix * vec4(inNormal, 0.0)).xyz * boneWeight;
	}
	
	tangent  = normalize(tangent);
	binormal = normalize(binormal);
	normal   = normalize(normal);
}
