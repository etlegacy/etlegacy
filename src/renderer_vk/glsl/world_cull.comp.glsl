#version 450

// GPU world culling: one thread per GPU-culled world surface (see
// R_VkBuildWorldCull). A surface is drawn when one of its BSP leaves is in
// the view's visible-leaf bitset (set by the CPU's node walk: PVS, areamask
// and node frustum culling) and it passes R_CullSurface()'s face plane and
// sphere tests. A visible surface appends its triangles (indexes into the
// static world buffer) to its group's region of the output index buffer and
// grows the group's indirect draw command (VkDrawIndexedIndirectCommand,
// 5 uints: indexCount, instanceCount, firstIndex, vertexOffset,
// firstInstance) - one command per shader, drawn by VK_Draw3DStatic().
// Surfaces the CPU draws itself in this view (dlit) carry the view's stamp.

layout(local_size_x = 64) in;

struct Surface
{
	vec4 sphere;     // origin, radius
	vec4 plane;      // normal, dist
	uint firstIndex; // in worldIndexes
	uint numIndexes;
	uint group;
	uint leafFirst;  // in surfLeaves
	uint leafCount;
	uint cullMode;   // 0: no face plane cull, 1: front sided, 2: back sided
	uint pad0, pad1;
};

layout(std430, set = 0, binding = 0) readonly buffer Surfaces
{
	Surface surfs[];
};

layout(std430, set = 0, binding = 1) readonly buffer SurfLeaves
{
	uint surfLeaves[];
};

layout(std430, set = 0, binding = 2) readonly buffer WorldIndexes
{
	uint worldIndexes[];
};

// per-frame host-written data: skip stamps, then per view the leaf bitset
// and the draw commands
layout(std430, set = 0, binding = 3) buffer Arena
{
	uint arena[];
};

layout(std430, set = 0, binding = 4) writeonly buffer OutIndexes
{
	uint outIndexes[];
};

layout(push_constant) uniform PushConstants
{
	vec4 frustum[5];   // normal, dist
	vec4 viewOrigin;
	uint numSurfs;
	uint leafBitsOfs;  // word offsets in arena
	uint skipOfs;
	uint cmdOfs;
	uint viewStamp;
	uint flags;        // 1: r_noCull, 2: r_facePlaneCull
	uint pad0, pad1;
} pc;

float Dot3(vec3 a, vec3 b)
{
	precise float d = a.x * b.x + a.y * b.y + a.z * b.z;

	return d;
}

void main()
{
	uint    s = gl_GlobalInvocationID.x;
	Surface sf;
	bool    visible = false;
	uint    i, cmd, dst;

	if (s >= pc.numSurfs || arena[pc.skipOfs + s] == pc.viewStamp)
	{
		return;
	}

	sf = surfs[s];

	for (i = 0u; i < sf.leafCount; i++)
	{
		uint leaf = surfLeaves[sf.leafFirst + i];

		if ((arena[pc.leafBitsOfs + (leaf >> 5)] & (1u << (leaf & 31u))) != 0u)
		{
			visible = true;
			break;
		}
	}
	if (!visible)
	{
		return;
	}

	if ((pc.flags & 1u) == 0u)
	{
		if (sf.cullMode != 0u && (pc.flags & 2u) != 0u)
		{
			precise float d = Dot3(pc.viewOrigin.xyz, sf.plane.xyz) - sf.plane.w;

			if ((sf.cullMode == 1u && d < -8.0) || (sf.cullMode == 2u && d > 8.0))
			{
				return;
			}
		}

		for (i = 0u; i < 5u; i++)
		{
			precise float dist = Dot3(sf.sphere.xyz, pc.frustum[i].xyz) - pc.frustum[i].w;

			if (dist < -sf.sphere.w)
			{
				return;
			}
		}
	}

	cmd = pc.cmdOfs + sf.group * 5u;
	dst = arena[cmd + 2u] + atomicAdd(arena[cmd], sf.numIndexes);
	for (i = 0u; i < sf.numIndexes; i++)
	{
		outIndexes[dst + i] = worldIndexes[sf.firstIndex + i];
	}
}
