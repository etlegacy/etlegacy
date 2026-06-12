#version 450

// lightmapped (base * lightmap) static 3D geometry vertex
// shader, used by RB_StageIteratorLightmappedMultitexture (and the
// equivalent 2-stage case in RB_StageIteratorGeneric).

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	vec4 alphaTest; // unused for the lightmapped path, kept for layout parity
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 fragUV0;
layout(location = 1) out vec2 fragUV1;
layout(location = 2) out vec4 fragColor;

void main()
{
	gl_Position = pc.mvp * vec4(inPos, 1.0);
	fragUV0     = inUV0;
	fragUV1     = inUV1;
	fragColor   = inColor;
}
