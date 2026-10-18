#version 450

// Instanced foliage (RB_SurfaceFoliage): the model's vertices come from the
// static world buffer (vk3DVertexLM_t), each instance's origin and color
// (vkFoliageInstance_t) from the per-instance stream. Every vertex repeats
// its instance's cull; a culled instance is moved outside the clip volume,
// so it draws nothing and the other instances keep their order. The
// stage's rgbGen/alphaGen is reproduced from the instance color (alpha
// faded by distance) as in tr3d_static.vert: color = in * mul + add.

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

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;  // per instance
layout(location = 3) in vec3 inOrigin; // per instance
layout(location = 4) in uint inRadix;  // per instance: its index & 3 in the surface

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

// the batch's per-instance cull parameters (RB_SurfaceFoliageInstanced)
layout(set = 1, binding = 0) uniform FoliageCull
{
	vec4 frustum[5];     // normal, dist
	vec4 distanceVector; // view depth: dot(origin, xyz) + w
	vec4 distanceCull;   // inner, outer, alpha threshold, 1 / (outer - inner)
	vec4 fovScale;       // x
} fc;

float Dot3(vec3 a, vec3 b)
{
	precise float r = a.x * b.x + a.y * b.y + a.z * b.z;

	return r;
}

// renderer1's per-instance part of RB_SurfaceFoliage (RB_FoliageInstanceColor
// on the CPU path, same arithmetic): distance and frustum cull, then the
// distance-faded alpha
bool FoliageInstanceColor(out vec4 color)
{
	color = inColor;
	if (fc.distanceCull.y > 0.0)
	{
		precise float z = fc.fovScale.x * (Dot3(inOrigin, fc.distanceVector.xyz) + fc.distanceVector.w);
		precise float alpha;
		int           i, a;

		if (z < -64.0)
		{
			return false;
		}

		for (i = 0; i < 5; i++)
		{
			precise float dist = Dot3(inOrigin, fc.frustum[i].xyz) - fc.frustum[i].w;

			if (dist < -64.0)
			{
				return false;
			}
		}

		// radix
		if ((inRadix & 1u) != 0u)
		{
			z *= 1.25;
			if ((inRadix & 2u) != 0u)
			{
				z *= 1.25;
			}
		}

		alpha = (fc.distanceCull.y - z) * fc.distanceCull.w;
		if (alpha < fc.distanceCull.z)
		{
			return false;
		}

		a       = alpha > 1.0 ? 255 : int(alpha * 255.0);
		color.a = float(a) / 255.0;
	}
	return true;
}

void main()
{
	precise vec3 pos = inPos + inOrigin;
	vec4         color;

	if (!FoliageInstanceColor(color))
	{
		gl_Position  = vec4(2.0, 2.0, 2.0, 1.0);
		fragUV       = vec2(0.0);
		fragColor    = vec4(0.0);
		fragEyePos   = vec3(0.0);
		fragFogDepth = 0.0;
		return;
	}

	gl_Position  = pc.mvp * vec4(pos, 1.0);
	fragUV       = inUV;
	fragColor    = color * pc.colorMul + pc.colorAdd;
	fragEyePos   = (pc.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
