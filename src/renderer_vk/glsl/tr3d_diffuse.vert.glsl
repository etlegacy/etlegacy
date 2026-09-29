#version 450

// Single-texture 3D vertex shader for rgbGen lightingDiffuse stages
// (models): computes renderer1's RB_CalcDiffuseColor() per vertex from the
// model-space normal instead of the CPU. The CPU table (entityLightInt,
// R_SetupEntityLighting) holds ambient + j / 63 * directed, clamped to 255,
// for j = 0..63, and each vertex picks entry (int)(64 * dot(normal,
// lightDir)) clamped to 0..63. Alpha still comes from the vertex color
// (alphaGen runs on the CPU).

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 lightDir;  // entity lightDir (model space), w unused
	vec4 ambient;   // entity ambientLight (0..255), w unused
	vec4 directed;  // entity directedLight (0..255), w unused
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

void main()
{
	int  dp    = clamp(int(64.0 * dot(inNormal, pc.lightDir.xyz)), 0, 63);
	vec3 light = min(floor(pc.ambient.rgb + (float(dp) / 63.0) * pc.directed.rgb), vec3(255.0));

	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV       = inUV;
	fragColor    = vec4(light / 255.0, inColor.a);
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
