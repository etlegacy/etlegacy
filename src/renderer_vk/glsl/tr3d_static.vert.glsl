#version 450

// Static world geometry vertex shader (single texture): reads the per-map
// static world buffer (vk3DVertexLM_t layout; the pipeline binds the
// texture or lightmap coords to inUV) and reproduces the stage's
// rgbGen/alphaGen from the raw BSP vertex color: color = in * mul + add.

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
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

void main()
{
	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV       = inUV;
	fragColor    = inColor * pc.colorMul + pc.colorAdd;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
