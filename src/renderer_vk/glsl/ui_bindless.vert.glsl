#version 450

// Simple 2D/UI vertex shader, bindless variant (vk.bindless):
// ui.vert plus each vertex's texture slot in the bindless array, passed flat
// to ui_bindless.frag, so batched 2D draws can use different textures.
// Transforms screen-space pixel coordinates directly into Vulkan NDC.
// Vulkan NDC is already Y-down (matching screen space), so unlike the
// legacy glOrtho(0, w, h, 0, 0, 1) path, no Y-flip is required here.
//
// alphaTest.x is forwarded unused to the fragment shader via the shared
// push-constant block (see ui.frag.glsl) so both stages can use one
// vkCmdPushConstants call per VK_Draw2D() batch.

layout(push_constant) uniform PushConstants
{
	vec2 scale;  // 2.0 / vidWidth, 2.0 / vidHeight
	vec2 offset; // -1.0, -1.0
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80 (used by fragment stage only)
} pc;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in uint inTexture;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out uint fragTexture;

void main()
{
	gl_Position = vec4(inPos * pc.scale + pc.offset, 0.0, 1.0);
	fragUV      = inUV;
	fragColor   = inColor;
	fragTexture = inTexture;
}
