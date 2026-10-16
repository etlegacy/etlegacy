#version 450

// Instanced foliage (RB_SurfaceFoliage): the model's vertices come from the
// static world buffer (vk3DVertexLM_t), each instance's origin and color
// (foliageInstance_t, distance-faded alpha already applied on the CPU) from
// the per-instance stream. The stage's rgbGen/alphaGen is reproduced from
// the instance color as in tr3d_static.vert: color = in * mul + add.

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
layout(location = 2) in vec4 inColor;  // per instance
layout(location = 3) in vec3 inOrigin; // per instance

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

void main()
{
	precise vec3 pos = inPos + inOrigin;

	gl_Position  = pc.mvp * vec4(pos, 1.0);
	fragUV       = inUV;
	fragColor    = inColor * pc.colorMul + pc.colorAdd;
	fragEyePos   = (pc.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
