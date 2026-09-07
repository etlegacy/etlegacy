#version 450

// renderer1's tr_gamma.c screen gamma program - copies the
// offscreen scene image to the swapchain applying r_gamma and the
// overbright multiplier (1 << tr.overbrightBits).

layout(set = 0, binding = 0) uniform sampler2D uScene;

layout(push_constant) uniform PushConstants
{
	float gamma;
	float overbright;
} pc;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(pow(texture(uScene, fragUV).rgb, vec3(1.0 / pc.gamma)) * pc.overbright, 1.0);
}
