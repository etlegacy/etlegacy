#version 450

// static 3D geometry fragment shader (single texture).
// Samples the bound texture and modulates it by the per-vertex color
// (rgbGen/alphaGen already baked in by ComputeColors()); optionally
// discards fragments to emulate the legacy GL_ALPHA_TEST modes, since
// Vulkan has no fixed-function alpha test.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
} pc;

layout(binding = 0) uniform sampler2D uTexture;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 color = texture(uTexture, fragUV) * fragColor;

	if (pc.alphaTest.x > 0.5)
	{
		if (pc.alphaTest.x < 1.5 && color.a <= 0.0)
		{
			discard;
		}
		else if (pc.alphaTest.x >= 1.5 && pc.alphaTest.x < 2.5 && color.a >= 0.5)
		{
			discard;
		}
		else if (pc.alphaTest.x >= 2.5 && color.a < 0.5)
		{
			discard;
		}
	}

	outColor = color;
}
