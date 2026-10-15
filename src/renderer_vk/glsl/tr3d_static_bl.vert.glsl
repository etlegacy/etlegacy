#version 450

// tr3d_static.vert for merged world batches (bindless textures, multi-draw
// indirect): the stage's colors and texture slot come from the draw's
// entry in the per-draw buffer instead of push constants.

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
	vec4  fogS;     // fog pass texcoords (RB_CalcFogTexCoordParams):
	vec4  fogT;     // s = dot(pos, fogS) + fogS.w, t = dot(pos, fogT) + fogT.w + fogEye.x
	vec4  fogEye;
};

layout(std430, set = 1, binding = 0) readonly buffer WorldDraws
{
	WorldDraw draws[];
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;
layout(location = 4) flat out uint fragTexture;

void main()
{
	WorldDraw d = draws[gl_InstanceIndex];

	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV       = inUV;
	fragColor    = inColor * d.colorMul + d.colorAdd;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
	fragTexture  = d.textures.x;
}
