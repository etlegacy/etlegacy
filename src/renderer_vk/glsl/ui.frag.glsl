#version 450

// simple 2D/UI fragment shader.
// Samples the bound texture and modulates it by the per-vertex color
// (colorGen/alphaGen are already baked into the vertex color by the
// frontend before the stage iterator runs).

layout(binding = 0) uniform sampler2D uTexture;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(uTexture, fragUV) * fragColor;
}
