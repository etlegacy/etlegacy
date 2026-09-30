#version 450

// Static world geometry vertex shader (base * lightmap) for dynamically lit
// batches: tr3d_lm_static.vert plus renderer1's RB_ComputeDlightColors() per
// vertex, so the dlight pass (dst * light + dst) is folded into this draw by
// tr3d_lm_dlight.frag. Ball dlights only (directed ones need normals, which
// the static world buffer doesn't have; such batches stay on the CPU).
// Light values are byte-quantized per vertex exactly like the CPU colors.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 colorMul;
	vec4 colorAdd;
} pc;

// the batch's dlights (VK_Draw3DStatic): light l = lights[3l] (origin in
// model space, radius), lights[3l + 1] (color * 255, intensity),
// lights[3l + 2] (radiusInverseCubed); info.x = count, info.y = 1: one
// combined light (r_dynamicLight 1), 2: one light per output (at most 4)
layout(set = 2, binding = 0) uniform Dlights
{
	vec4 info;
	vec4 lights[3 * 32];
} dl;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 fragUV0;
layout(location = 1) out vec2 fragUV1;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragFogDepth;
layout(location = 4) out vec3 fragEyePos;
layout(location = 5) out vec3 fragDlight[4]; // 0..1, multiplied in as (1 + light)

// light l's contribution added to color (bytes, clamped to 255), as the CPU loop
vec3 AddDlight(int l, vec3 color)
{
	vec4  o   = dl.lights[3 * l];
	vec4  c   = dl.lights[3 * l + 1];
	vec3  dir = vec3(o.w) - abs(o.xyz - inPos);
	float modulate;

	if (dir.x <= 0.0 || dir.y <= 0.0 || dir.z <= 0.0)
	{
		return color;
	}

	modulate = c.w * dir.x * dir.y * dir.z * dl.lights[3 * l + 2].x;
	if (modulate < (1.0 / 128.0))
	{
		return color;
	}
	modulate = min(modulate, 1.0);

	return min(color + vec3(ivec3(c.rgb * modulate)), vec3(255.0));
}

void main()
{
	int count = int(dl.info.x);
	int i;

	gl_Position  = pc.mvp * vec4(inPos, 1.0);
	fragUV0      = inUV0;
	fragUV1      = inUV1;
	fragColor    = inColor * pc.colorMul + pc.colorAdd;
	fragEyePos   = (pc.modelView * vec4(inPos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;

	for (i = 0; i < 4; i++)
	{
		fragDlight[i] = vec3(0.0);
	}

	if (dl.info.y < 1.5)
	{
		vec3 color = vec3(0.0);

		for (i = 0; i < count; i++)
		{
			color = AddDlight(i, color);
		}
		fragDlight[0] = color / 255.0;
	}
	else
	{
		for (i = 0; i < count && i < 4; i++)
		{
			fragDlight[i] = AddDlight(i, vec3(0.0)) / 255.0;
		}
	}
}
