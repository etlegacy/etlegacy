#version 450

// 2D/UI fragment shader.
// Samples the bound texture and modulates it by the per-vertex color
// (colorGen/alphaGen are already baked into the vertex color by the
// frontend before the stage iterator runs). Also applies the shader
// stage's GL_ALPHA_TEST mode via discard, needed e.g. by the automap's
// circular-mask trick (cg_commandmap.c draws a mask image with
// "alphafunc GE128" to carve a circular hole in the depth buffer before
// the actual map image is drawn with "depthFunc equal").

layout(binding = 0) uniform sampler2D uTexture;

layout(push_constant) uniform PushConstants
{
	vec2 scale;
	vec2 offset;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
} pc;

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
