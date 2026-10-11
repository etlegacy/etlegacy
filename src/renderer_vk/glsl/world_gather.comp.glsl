#version 450

// Merged world batches (VK_FlushWorldBatch): copies each batched draw's
// group command, written by world_cull.comp (VkDrawIndexedIndirectCommand:
// indexCount, instanceCount, firstIndex, vertexOffset, firstInstance), to
// the draw's slot in the batch command buffer, so a batch's draws are
// consecutive for one multi-draw indirect call, and sets firstInstance to
// the slot - the index of the draw's per-draw data.

layout(local_size_x = 64) in;

layout(std430, set = 0, binding = 3) readonly buffer Arena
{
	uint arena[];
};

// per draw: word offset of the group command in the arena
layout(std430, set = 0, binding = 5) readonly buffer Gather
{
	uint gather[];
};

layout(std430, set = 0, binding = 6) writeonly buffer BatchCommands
{
	uint batchCmds[];
};

layout(push_constant) uniform PushConstants
{
	uint first; // first slot of this frame's draws
	uint count;
} pc;

void main()
{
	uint i = gl_GlobalInvocationID.x;
	uint slot, src;

	if (i >= pc.count)
	{
		return;
	}

	slot = pc.first + i;
	src  = gather[slot];

	batchCmds[slot * 5u]      = arena[src];
	batchCmds[slot * 5u + 1u] = arena[src + 1u];
	batchCmds[slot * 5u + 2u] = arena[src + 2u];
	batchCmds[slot * 5u + 3u] = arena[src + 3u];
	batchCmds[slot * 5u + 4u] = slot;
}
