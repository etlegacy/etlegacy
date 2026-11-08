#version 450

// tr3d.frag for soft particles (r_softParticles, VK_SoftParticleMode()):
// sprites and polys fade out where they come close to the scene behind them
// and close to the camera, instead of ending in a hard line where they cut
// into a wall or the floor. The scene's depth is a copy made before the first
// such draw of the view (VK_SoftDepthCopy()), multisampled with MSAA (this file, tr3d_soft.frag otherwise). Keep the rest in step with
// tr3d.frag.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest; // x: 0=off, 1=GT_0, 2=LT_80, 3=GE_80
	                // z: fog mode 0=linear, 1=exp (GL_EXP); w: fog density (exp)
	vec4 fogColor;
	vec4 fogParams; // x: fog enabled, y: start, z: end, w: clip plane enabled
	vec4 clipPlane; // portal/mirror eye-space clip plane (xyz=normal, w=dist)
	vec4 soft0;     // x, y: distance = x / (depth + y); z: 1 / fade distance; w: mode
	                // (0 scales alpha, 1 the whole color, 2 fades the color to white)
	vec4 soft1;     // x: distance from the camera where the fade in starts, y: 1 / its length
} pc;

layout(set = 0, binding = 0) uniform sampler2D uTexture;
layout(set = 1, binding = 0) uniform sampler2DMS uSceneDepth;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragFogDepth;
layout(location = 3) in vec3 fragEyePos;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 color = texture(uTexture, fragUV) * fragColor;

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

	// the distance of the scene behind this fragment and of the fragment
	// itself, from their reversed-Z depths (sample 0 with MSAA)
	float sceneDepth = texelFetch(uSceneDepth, ivec2(gl_FragCoord.xy), 0).r;
	float sceneDist  = pc.soft0.x / (sceneDepth + pc.soft0.y);
	float fragDist   = pc.soft0.x / (gl_FragCoord.z + pc.soft0.y);
	float fade       = clamp((sceneDist - fragDist) * pc.soft0.z, 0.0, 1.0) *
	                   clamp((fragDist - pc.soft1.x) * pc.soft1.y, 0.0, 1.0);

	if (pc.soft0.w < 0.5)
	{
		color.a *= fade;
	}
	else if (pc.soft0.w < 1.5)
	{
		color *= fade;
	}
	else
	{
		color.rgb = mix(vec3(1.0), color.rgb, fade);
	}

	outColor = color;
}
