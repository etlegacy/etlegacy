#version 450

// static 3D geometry vertex shader (single texture).
// pc.mvp already has the GL->Vulkan clip-space correction baked in
// (Y-flip + depth range 0..1), computed CPU-side in VK_SetProjectionMatrix.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main()
{
	gl_Position = pc.mvp * vec4(inPos, 1.0);
	fragUV      = inUV;
	fragColor   = inColor;
}
