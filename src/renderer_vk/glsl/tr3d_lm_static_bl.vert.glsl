#version 450

// tr3d_lm_static.vert for merged world batches (bindless textures,
// multi-draw indirect): the stage's colors and texture slots come from the
// draw's entry in the per-draw buffer instead of push constants.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
} pc;

// per-draw data of a merged world batch (VK_FlushWorldBatch), indexed by
// the draw's firstInstance: bindless texture slots and the stage's
// color = vertex color * colorMul + colorAdd (VK_StaticStageColor)
struct WorldDraw
{
	uvec4 textures; // x: base, y: lightmap
	vec4  colorMul;
	vec4  colorAdd;
};

layout(std430, set = 1, binding = 0) readonly buffer WorldDraws
{
	WorldDraw draws[];
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 fragUV0;
layout(location = 1) out vec2 fragUV1;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragFogDepth;
layout(location = 4) out vec3 fragEyePos;
layout(location = 5) flat out uvec2 fragTextures;

void main()
{
	WorldDraw d = draws[gl_InstanceIndex];

	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV0      = inUV0;
	fragUV1      = inUV1;
	fragColor    = inColor * d.colorMul + d.colorAdd;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
	fragTextures = d.textures.xy;
}
