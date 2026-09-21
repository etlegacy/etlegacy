#version 450

// Static world geometry vertex shader (base * lightmap): reads the per-map
// static world buffer and reproduces the stage's rgbGen/alphaGen from the
// raw BSP vertex color: color = in * mul + add.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 colorMul;
	vec4 colorAdd;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 fragUV0;
layout(location = 1) out vec2 fragUV1;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragFogDepth;
layout(location = 4) out vec3 fragEyePos;

void main()
{
	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV0      = inUV0;
	fragUV1      = inUV1;
	fragColor    = inColor * pc.colorMul + pc.colorAdd;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
