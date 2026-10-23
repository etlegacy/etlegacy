#version 450
#extension GL_EXT_nonuniform_qualifier : require

// 2D/UI fragment shader, bindless variant (vk.bindless): as
// ui.frag, but samples the vertex's texture slot in the bindless array
// (set 0, VK_BINDLESS_MAX_IMAGES entries).
// Samples the bound texture and modulates it by the per-vertex color
// (colorGen/alphaGen are already baked into the vertex color by the
// frontend before the stage iterator runs). Also applies the shader
// stage's GL_ALPHA_TEST mode via discard, needed e.g. by the automap's
// circular-mask trick (cg_commandmap.c draws a mask image with
// "alphafunc GE128" to carve a circular hole in the depth buffer before
// the actual map image is drawn with "depthFunc equal").

// bindless textures and the shared samplers; a texture index holds the slot
// in its low 12 bits and the sampler above them (VK_BindlessIndex())
layout(set = 0, binding = 0) uniform texture2D uTextures[4096]; // VK_BINDLESS_MAX_IMAGES
layout(set = 0, binding = 1) uniform sampler uSamplers[4];      // VK_BINDLESS_NUM_SAMPLERS

vec4 BindlessTexture(uint index, vec2 uv)
{
	return texture(nonuniformEXT(sampler2D(uTextures[nonuniformEXT(index & 4095u)], uSamplers[nonuniformEXT(index >> 12)])), uv);
}

layout(push_constant) uniform PushConstants
{
	vec2 scale;
	vec2 offset;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
} pc;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragTexture;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 color = BindlessTexture(fragTexture, fragUV) * fragColor;

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
