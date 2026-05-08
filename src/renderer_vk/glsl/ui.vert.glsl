#version 450

// simple 2D/UI vertex shader.
// Transforms screen-space pixel coordinates directly into Vulkan NDC.
// Vulkan NDC is already Y-down (matching screen space), so unlike the
// legacy glOrtho(0, w, h, 0, 0, 1) path, no Y-flip is required here.

layout(push_constant) uniform PushConstants
{
	vec2 scale;  // 2.0 / vidWidth, 2.0 / vidHeight
	vec2 offset; // -1.0, -1.0
} pc;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main()
{
	gl_Position = vec4(inPos * pc.scale + pc.offset, 0.0, 1.0);
	fragUV      = inUV;
	fragColor   = inColor;
}
