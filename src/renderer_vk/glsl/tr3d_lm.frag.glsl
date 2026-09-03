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
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80; y: texenv 0=modulate, 1=add, 2=replace, 3=decal
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
	// renderer1's DrawMultitextured(): unit 0 is base * vertex color
	// (GL_MODULATE), unit 1 combines with it per shader->multitextureEnv.
	vec4 prev  = texture(uBase, fragUV0) * fragColor;
	vec4 tex1  = texture(uLightmap, fragUV1);
	vec4 color;

	if (pc.alphaTest.y < 0.5)
	{
		color = prev * tex1;
	}
	else if (pc.alphaTest.y < 1.5)
	{
		color = vec4(prev.rgb + tex1.rgb, prev.a * tex1.a);
	}
	else if (pc.alphaTest.y < 2.5)
	{
		color = tex1;
	}
	else
	{
		color = vec4(mix(prev.rgb, tex1.rgb, tex1.a), prev.a);
	}

	// portal/mirror clip plane (see tr3d.frag.glsl for details).
	if (pc.fogParams.w > 0.5)
	{
		if (dot(fragEyePos, pc.clipPlane.xyz) + pc.clipPlane.w < 0.0)
		{
			discard;
		}
	}

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

	if (pc.fogParams.x > 0.5)
	{
		float fogFactor = clamp((pc.fogParams.z - fragFogDepth) / max(pc.fogParams.z - pc.fogParams.y, 0.001), 0.0, 1.0);

		color.rgb = mix(pc.fogColor.rgb, color.rgb, fogFactor);
	}

	outColor = color;
}
