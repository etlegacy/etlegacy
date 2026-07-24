#version 450

// lightmapped (base * lightmap) static 3D geometry fragment
// shader. uBase is bound in descriptor set 0, uLightmap in descriptor set
// 1 - both reuse the same per-image single-combined-sampler descriptor
// set layout/pool as the base texture path (vk.uiDescriptorSetLayout).

layout(set = 0, binding = 0) uniform sampler2D uBase;
layout(set = 1, binding = 0) uniform sampler2D uLightmap;

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams; // x: fog enabled, y: start, z: end, w: clip plane enabled
	vec4 clipPlane; // portal/mirror eye-space clip plane (xyz=normal, w=dist)
} pc;

layout(location = 0) in vec2 fragUV0;
layout(location = 1) in vec2 fragUV1;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragFogDepth;
layout(location = 4) in vec3 fragEyePos;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 color = texture(uBase, fragUV0) * texture(uLightmap, fragUV1) * fragColor;

	// portal/mirror clip plane (see tr3d.frag.glsl for details).
	if (pc.fogParams.w > 0.5)
	{
		if (dot(fragEyePos, pc.clipPlane.xyz) + pc.clipPlane.w < 0.0)
		{
			discard;
		}
	}

	if (pc.fogParams.x > 0.5)
	{
		float fogFactor = clamp((pc.fogParams.z - fragFogDepth) / max(pc.fogParams.z - pc.fogParams.y, 0.001), 0.0, 1.0);

		color.rgb = mix(pc.fogColor.rgb, color.rgb, fogFactor);
	}

	outColor = color;
}
