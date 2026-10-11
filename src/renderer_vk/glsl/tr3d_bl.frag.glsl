#version 450
#extension GL_EXT_nonuniform_qualifier : require

// tr3d.frag for merged world batches: samples the draw's texture slot in
// the bindless array (set 0) instead of a per-image descriptor set.
//
// Static 3D geometry fragment shader (single texture).
// Samples the bound texture and modulates it by the per-vertex color
// (rgbGen/alphaGen already baked in by ComputeColors()); optionally
// discards fragments to emulate the legacy GL_ALPHA_TEST modes, since
// Vulkan has no fixed-function alpha test.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
	                // z: fog mode 0=linear, 1=exp (GL_EXP); w: fog density (exp)
	vec4 fogColor;
	vec4 fogParams; // x: fog enabled, y: start, z: end, w: clip plane enabled
	vec4 clipPlane; // portal/mirror eye-space clip plane (xyz=normal, w=dist)
} pc;

layout(set = 0, binding = 0) uniform sampler2D uTextures[4096]; // VK_BINDLESS_MAX_IMAGES

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragFogDepth;
layout(location = 3) in vec3 fragEyePos;
layout(location = 4) flat in uint fragTexture;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 color = texture(uTextures[nonuniformEXT(fragTexture)], fragUV) * fragColor;

	// Portal/mirror clip plane — replicates
	// GL_CLIP_PLANE0's semantics (discard fragments on the plane's
	// negative side) since Vulkan has no portable fixed-function
	// equivalent.
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
		float fogFactor;

		if (pc.alphaTest.z > 0.5)
		{
			// GL_EXP: f = e^(-density * z)
			fogFactor = clamp(exp(-pc.alphaTest.w * fragFogDepth), 0.0, 1.0);
		}
		else
		{
			// GL_LINEAR: f = (end - z) / (end - start)
			fogFactor = clamp((pc.fogParams.z - fragFogDepth) / max(pc.fogParams.z - pc.fogParams.y, 0.001), 0.0, 1.0);
		}

		color.rgb = mix(pc.fogColor.rgb, color.rgb, fogFactor);
	}

	outColor = color;
}
