#version 450

// Two-stage 3D fragment shader (VK_Draw3DTwoStage): renderer1 draws stage 0
// (opaque, depth writing, no alpha test) and then stage 1 blended over it
// with its blendFunc. Here stage 0's result is the destination color, so
// stage 1's blend equation is evaluated in the shader and the draw uses
// stage 0's (opaque) state. Each stage's color is fogged before blending, as
// GL's fixed-function fog does per drawn fragment; the destination is
// clamped like the 8-bit framebuffer. uTexture0 is set 0, uTexture1 set 1.

layout(set = 0, binding = 0) uniform sampler2D uTexture0;
layout(set = 1, binding = 0) uniform sampler2D uTexture1;

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest; // z: fog mode 0=linear, 1=exp (GL_EXP); w: fog density (exp)
	vec4 fogColor;
	vec4 fogParams; // x: fog enabled, y: start, z: end, w: clip plane enabled
	vec4 clipPlane;
	vec4 blend1;    // stage 1 blendFunc: x = GLS_SRCBLEND_*, y = GLS_DSTBLEND_* >> 4
} pc;

layout(location = 0) in vec2 fragUV0;
layout(location = 1) in vec2 fragUV1;
layout(location = 2) in vec4 fragColor0;
layout(location = 3) in vec4 fragColor1;
layout(location = 4) in float fragFogDepth;
layout(location = 5) in vec3 fragEyePos;

layout(location = 0) out vec4 outColor;

vec4 Fog(vec4 color)
{
	float fogFactor;

	if (pc.fogParams.x < 0.5)
	{
		return color;
	}

	if (pc.alphaTest.z > 0.5)
	{
		fogFactor = clamp(exp(-pc.alphaTest.w * fragFogDepth), 0.0, 1.0);
	}
	else
	{
		fogFactor = clamp((pc.fogParams.z - fragFogDepth) / max(pc.fogParams.z - pc.fogParams.y, 0.001), 0.0, 1.0);
	}

	return vec4(mix(pc.fogColor.rgb, color.rgb, fogFactor), color.a);
}

// GLS_SRCBLEND_* (1 ZERO, 2 ONE, 3 DST_COLOR, 4 ONE_MINUS_DST_COLOR,
// 5 SRC_ALPHA, 6 ONE_MINUS_SRC_ALPHA, 7 DST_ALPHA, 8 ONE_MINUS_DST_ALPHA,
// 9 ALPHA_SATURATE)
vec4 SrcFactor(int f, vec4 src, vec4 dst)
{
	switch (f)
	{
	case 1: return vec4(0.0);
	case 3: return dst;
	case 4: return vec4(1.0) - dst;
	case 5: return vec4(src.a);
	case 6: return vec4(1.0 - src.a);
	case 7: return vec4(dst.a);
	case 8: return vec4(1.0 - dst.a);
	case 9: return vec4(vec3(min(src.a, 1.0 - dst.a)), 1.0);
	default: return vec4(1.0);
	}
}

// GLS_DSTBLEND_* >> 4 (1 ZERO, 2 ONE, 3 SRC_COLOR, 4 ONE_MINUS_SRC_COLOR,
// 5 SRC_ALPHA, 6 ONE_MINUS_SRC_ALPHA, 7 DST_ALPHA, 8 ONE_MINUS_DST_ALPHA)
vec4 DstFactor(int f, vec4 src, vec4 dst)
{
	switch (f)
	{
	case 2: return vec4(1.0);
	case 3: return src;
	case 4: return vec4(1.0) - src;
	case 5: return vec4(src.a);
	case 6: return vec4(1.0 - src.a);
	case 7: return vec4(dst.a);
	case 8: return vec4(1.0 - dst.a);
	default: return vec4(0.0);
	}
}

void main()
{
	vec4 dst = clamp(Fog(texture(uTexture0, fragUV0) * fragColor0), 0.0, 1.0);
	vec4 src = clamp(Fog(texture(uTexture1, fragUV1) * fragColor1), 0.0, 1.0); // clamped before blending, as for UNORM targets

	if (pc.fogParams.w > 0.5)
	{
		if (dot(fragEyePos, pc.clipPlane.xyz) + pc.clipPlane.w < 0.0)
		{
			discard;
		}
	}

	outColor = src * SrcFactor(int(pc.blend1.x), src, dst) + dst * DstFactor(int(pc.blend1.y), src, dst);
}
