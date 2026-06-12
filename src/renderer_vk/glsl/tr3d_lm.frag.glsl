#version 450

// lightmapped (base * lightmap) static 3D geometry fragment
// shader. uBase is bound in descriptor set 0, uLightmap in descriptor set
// 1 - both reuse the same per-image single-combined-sampler descriptor
// set layout/pool as the base texture path (vk.uiDescriptorSetLayout).

layout(set = 0, binding = 0) uniform sampler2D uBase;
layout(set = 1, binding = 0) uniform sampler2D uLightmap;

layout(location = 0) in vec2 fragUV0;
layout(location = 1) in vec2 fragUV1;
layout(location = 2) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(uBase, fragUV0) * texture(uLightmap, fragUV1) * fragColor;
}
