#version 450

// Lightmapped (base * lightmap) static 3D geometry vertex
// shader, used by RB_StageIteratorLightmappedMultitexture (and the
// equivalent 2-stage case in RB_StageIteratorGeneric).

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest; // unused for the lightmapped path, kept for layout parity
	vec4 fogColor;
	vec4 fogParams; // x: fog enabled, y: start, z: end, w: clip plane enabled
	vec4 clipPlane; // portal/mirror eye-space clip plane (xyz=normal, w=dist)
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
	fragColor    = inColor;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
