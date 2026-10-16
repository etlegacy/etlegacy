#version 450

// Fog pass of instanced foliage (RB_FogPass): renderer1's
// RB_CalcFogTexCoords() per vertex of each instance from the fog volume's
// s/t vectors (RB_CalcFogTexCoordParams), with the fog's constant color.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 color;  // fog color
	vec4 fogS;   // s = dot(pos, fogS) + fogS.w
	vec4 fogT;   // t = dot(pos, fogT) + fogT.w + fogEye.x
	vec4 fogEye;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;  // per instance
layout(location = 3) in vec3 inOrigin; // per instance

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

float Dot3(vec3 a, vec3 b)
{
	precise float r = a.x * b.x + a.y * b.y + a.z * b.z;

	return r;
}

void main()
{
	precise vec3 pos = inPos + inOrigin;

	gl_Position  = pc.mvp * vec4(pos, 1.0);
	precise float fogS = Dot3(pos, pc.fogS.xyz) + pc.fogS.w;
	precise float fogT = Dot3(pos, pc.fogT.xyz) + pc.fogT.w;

	fogT        += pc.fogEye.x;
	fragUV       = vec2(fogS, fogT);
	fragColor    = pc.color;
	fragEyePos   = (pc.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
