#version 450

// tr3d_skin.vert for merged model batches (bindless textures, multi-draw
// indirect, tr3d_bl.frag): the bones are read from the bone arena at the
// draw's first bone row, the per-draw values come from its ModelDraw entry.

// GPU-skinned MDM vertex shader (single texture): skins the skin buffer's
// vertex with the surface's bones, as renderer1's RB_MDM_SurfaceAnim() does
// on the CPU (position: sum of weight * bone * offset; normal: rotated by
// the first weight's bone), then colors it like tr3d_diffuse.vert. Constant
// rgbGen stages come in with lightDir 0, so the table lookup yields
// floor(ambient) = that color; lightDir.w is the stage's constant alpha.
// tcGen environment (alphaTest.y): RB_CalcEnvironmentTexCoords() with the
// batch's parameters from set 2.

// per-draw data of a merged model batch (VK_FlushModelBatch), indexed by
// the draw's firstInstance: what the single-draw path pushes as constants,
// plus the frame offsets the vertex streams would be bound at
struct ModelDraw
{
	mat4  mvp;
	mat4  modelView;
	vec4  alphaTest; // y: tcGen environment
	vec4  lightDir;  // entity lightDir (model space), w = stage alpha
	vec4  ambient;   // entity ambientLight or constant color (0..255), w = flags
	vec4  directed;  // entity directedLight (0..255)
	vec4  lerp;      // newXyzScale, oldXyzScale, newNormalScale, oldNormalScale (< 0: no interpolation)
	vec4  env[3];    // tcGen environment parameters (RB_VkSetupEnvParams)
	uvec4 ofs;       // x: first bone row (vec4) in the bone arena
	uvec4 ofs2;      // y: bindless texture slot
};

layout(std430, set = 1, binding = 0) readonly buffer ModelDraws
{
	ModelDraw draws[];
};


// the bone arena (VK_AllocSkinBones) as vec4 rows: a draw's bone b is
// rows[ofs.x + 3 * b + r] = (matrix[r][0..2], translation[r])
layout(std430, set = 1, binding = 2) readonly buffer Bones
{
	vec4 rows[];
} bones;

uint boneBase;


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
layout(location = 4) flat out uint fragTexture;

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

	return vec3(dot(bones.rows[boneBase + 3u * b], p), dot(bones.rows[boneBase + 3u * b + 1u], p), dot(bones.rows[boneBase + 3u * b + 2u], p));
}

void main()
{
	ModelDraw d = draws[gl_InstanceIndex];

	boneBase = d.ofs.x;

	vec3 pos = inWeights.x * BonePoint(inBones.x, inOffset0) +
	           inWeights.y * BonePoint(inBones.y, inOffset1) +
	           inWeights.z * BonePoint(inBones.z, inOffset2) +
	           inWeights.w * BonePoint(inBones.w, inOffset3);
	uint b      = 3u * inBones.x;
	vec3 normal = vec3(dot(bones.rows[boneBase + b].xyz, inNormal), dot(bones.rows[boneBase + b + 1u].xyz, inNormal), dot(bones.rows[boneBase + b + 2u].xyz, inNormal));

	int  dp    = clamp(int(64.0 * dot(normal, d.lightDir.xyz)), 0, 63);
	vec3 light = min(floor(d.ambient.rgb + (float(dp) / 63.0) * d.directed.rgb), vec3(255.0));

	if (d.alphaTest.y > 0.5)
	{
		precise vec3  viewer = d.env[0].xyz - pos;
		precise float d2;
		precise vec3  reflected;

		viewer   *= QRsqrt(Dot3(viewer, viewer));
		d2        = 2.0 * Dot3(normal, viewer);
		reflected = normal * d2 - viewer;
		fragUV    = vec2(d.env[0].w + Dot3(reflected, d.env[1].xyz), d.env[1].w - Dot3(reflected, d.env[2].xyz));
	}
	else
	{
		fragUV = inUV;
	}

	gl_Position  = d.mvp * vec4(pos, 1.0);
	fragTexture  = d.ofs2.y;
	fragColor    = vec4(light / 255.0, d.lightDir.w);
	fragEyePos   = (d.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
