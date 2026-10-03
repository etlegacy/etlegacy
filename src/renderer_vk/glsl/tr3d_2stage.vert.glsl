#version 450

// Two-stage 3D vertex shader (VK_Draw3DTwoStage): passes both stages'
// texture coords and colors (computed on the CPU by ComputeColors() /
// ComputeTexCoords(), as for a single stage) to tr3d_2stage.frag, which
// blends stage 1 over stage 0 in one draw.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 blend1;    // stage 1 blendFunc: x = GLS_SRCBLEND_*, y = GLS_DSTBLEND_* >> 4
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec4 inColor0;
layout(location = 4) in vec4 inColor1;

layout(location = 0) out vec2 fragUV0;
layout(location = 1) out vec2 fragUV1;
layout(location = 2) out vec4 fragColor0;
layout(location = 3) out vec4 fragColor1;
layout(location = 4) out float fragFogDepth;
layout(location = 5) out vec3 fragEyePos;

void main()
{
	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV0      = inUV0;
	fragUV1      = inUV1;
	fragColor0   = inColor0;
	fragColor1   = inColor1;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
