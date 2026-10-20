#version 450

// GPU-skinned MDM vertex shader (single texture): skins the skin buffer's
// vertex with the surface's bones, as renderer1's RB_MDM_SurfaceAnim() does
// on the CPU (position: sum of weight * bone * offset; normal: rotated by
// the first weight's bone), then colors it like tr3d_diffuse.vert. Constant
// rgbGen stages come in with lightDir 0, so the table lookup yields
// floor(ambient) = that color; lightDir.w is the stage's constant alpha.
// Fog pass (alphaTest.y 2): fog texcoords from the parameters in the env
// slots, color from the constant color path.
// tcGen environment (alphaTest.y 1): RB_CalcEnvironmentTexCoords() with the
// batch's parameters from set 2.

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest; // y: 1 tcGen environment, 2 fog pass
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 lightDir;  // entity lightDir (model space), w = stage alpha
	vec4 ambient;   // entity ambientLight or constant color (0..255)
	vec4 directed;  // entity directedLight (0..255)
} pc;

// one surface's bones, indexed by the surface-local bone of each weight:
// rows[3 * b + r] = (matrix[r][0..2], translation[r])
layout(set = 1, binding = 0) uniform Bones
{
	vec4 rows[3 * 256];
} bones;

// view origin (model space) + sAdjust, axis row 1 * 0.5 + tAdjust, axis row 2 * 0.5
layout(set = 2, binding = 0) uniform EnvParams
{
	vec4 p[3];
} env;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uvec4 inBones;
layout(location = 3) in vec4 inWeights;
layout(location = 4) in vec3 inOffset0;
layout(location = 5) in vec3 inOffset1;
layout(location = 6) in vec3 inOffset2;
layout(location = 7) in vec3 inOffset3;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

float QRsqrt(float f)
{
	precise float x2 = f * 0.5;
	precise float y  = intBitsToFloat(0x5f3759df - (floatBitsToInt(f) >> 1));

	return y * (1.5 - (x2 * y * y));
}

float Dot3(vec3 a, vec3 b)
{
	precise float d = a.x * b.x + a.y * b.y + a.z * b.z;

	return d;
}

vec3 BonePoint(uint b, vec3 offset)
{
	vec4 p = vec4(offset, 1.0);

	return vec3(dot(bones.rows[3u * b], p), dot(bones.rows[3u * b + 1u], p), dot(bones.rows[3u * b + 2u], p));
}

void main()
{
	vec3 pos = inWeights.x * BonePoint(inBones.x, inOffset0) +
	           inWeights.y * BonePoint(inBones.y, inOffset1) +
	           inWeights.z * BonePoint(inBones.z, inOffset2) +
	           inWeights.w * BonePoint(inBones.w, inOffset3);
	uint b      = 3u * inBones.x;
	vec3 normal = vec3(dot(bones.rows[b].xyz, inNormal), dot(bones.rows[b + 1u].xyz, inNormal), dot(bones.rows[b + 2u].xyz, inNormal));

	int  dp    = clamp(int(64.0 * dot(normal, pc.lightDir.xyz)), 0, 63);
	vec3 light = min(floor(pc.ambient.rgb + (float(dp) / 63.0) * pc.directed.rgb), vec3(255.0));

	if (pc.alphaTest.y > 1.5)
	{
		// fog pass (RB_FogPass): RB_CalcFogTexCoords() from the fog volume's
		// s/t vectors and eye term (RB_CalcFogTexCoordParams) in the env slots
		precise float fogS = Dot3(pos, env.p[0].xyz) + env.p[0].w;
		precise float fogT = Dot3(pos, env.p[1].xyz) + env.p[1].w;

		fogT  += env.p[2].x;
		fragUV = vec2(fogS, fogT);
	}
	else if (pc.alphaTest.y > 0.5)
	{
		precise vec3  viewer = env.p[0].xyz - pos;
		precise float d2;
		precise vec3  reflected;

		viewer   *= QRsqrt(Dot3(viewer, viewer));
		d2        = 2.0 * Dot3(normal, viewer);
		reflected = normal * d2 - viewer;
		fragUV    = vec2(env.p[0].w + Dot3(reflected, env.p[1].xyz), env.p[1].w - Dot3(reflected, env.p[2].xyz));
	}
	else
	{
		fragUV = inUV;
	}

	gl_Position  = pc.mvp * vec4(pos, 1.0);
	fragColor    = vec4(light / 255.0, pc.lightDir.w);
	fragEyePos   = (pc.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
