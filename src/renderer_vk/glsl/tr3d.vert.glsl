#version 450

// Static 3D geometry vertex shader (single texture).
// pc.mvp already has the GL->Vulkan clip-space correction baked in
// (Y-flip + depth range 0..1), computed CPU-side in VK_SetProjectionMatrix.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
	vec4 fogColor;
	vec4 fogParams; // x: fog enabled, y: start, z: end, w: clip plane enabled
	vec4 clipPlane; // portal/mirror eye-space clip plane (xyz=normal, w=dist)
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
	fragColor    = inColor;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	// after the GL->Vulkan flip-matrix baked into modelView, the viewer
	// looks down -Z, so -eyePos.z is the (positive) distance from camera.
	fragFogDepth = -fragEyePos.z;
}
