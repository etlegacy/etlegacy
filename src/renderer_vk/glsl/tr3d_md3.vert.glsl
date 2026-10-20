#version 450

// GPU-animated MD3/MDC vertex shader (single texture): interpolates the
// vertex between the entity's oldframe and frame like renderer1's
// LerpMeshVertexes_scalar() / LerpCMeshVertexes() do on the CPU, then colors
// it like tr3d_skin.vert (lightingDiffuse, or a constant color passed as
// ambient with lightDir 0; lightDir.w is the stage's constant alpha).
//
// MD3 position: old * oldXyzScale + new * newXyzScale (oldXyzScale 0
// without interpolation). Normal: decoded from the packed lat/long through
// tr.sinTable (FUNCTABLE_SIZE 4096); without interpolation from the new
// frame, with interpolation from the old frame's lat/long scaled by
// newNormalScale + oldNormalScale and truncated, exactly as the CPU code
// (which reads only the old frame's normal there).
//
// MDC: the frames are base frames (as MD3); a frame with a compressed frame
// adds its 8-bit offsets (scaled by newNormalScale / oldNormalScale) and
// takes its normal from r_anormals. Interpolated normals are blended and
// normalized with Q_rsqrt.
//
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
	vec4 ambient;   // entity ambientLight or constant color (0..255), w = flags
	vec4 directed;  // entity directedLight (0..255)
	vec4 lerp;      // newXyzScale, oldXyzScale, newNormalScale, oldNormalScale (< 0: no interpolation)
} pc;

// tr.sinTable[16 * k] and [16 * k - 1] as 256 pairs, then r_anormals
layout(set = 1, binding = 0) uniform MeshTables
{
	vec4 tables[128 + 256];
} mt;

// view origin (model space) + sAdjust, axis row 1 * 0.5 + tAdjust, axis row 2 * 0.5
layout(set = 2, binding = 0) uniform EnvParams
{
	vec4 p[3];
} env;

layout(location = 0) in ivec4 inNew;    // frame (MDC: base frame): xyz, packed lat/long normal
layout(location = 1) in ivec4 inOld;    // oldframe
layout(location = 2) in vec2 inUV;
layout(location = 3) in uint inNewComp; // MDC compressed frame: 3 x 8-bit offset, anormal index
layout(location = 4) in uint inOldComp;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

#define MESH_MDC      1
#define MESH_NEW_COMP 2
#define MESH_OLD_COMP 4

// tr.sinTable[i] for i = 16 * k or 16 * k - 1 (mod 4096)
float SinT(int i)
{
	int slot = ((i & 15) == 0) ? 2 * (i >> 4) : 2 * (((i + 1) & 4095) >> 4) + 1;

	return mt.tables[slot >> 2][slot & 3];
}

vec3 LatLongNormal(int lat, int lng)
{
	return vec3(SinT((lat + 1024) & 4095) * SinT(lng), SinT(lat) * SinT(lng), SinT((lng + 1024) & 4095));
}

vec3 CompOffset(uint v)
{
	return (vec3(float(v & 255u), float((v >> 8) & 255u), float((v >> 16) & 255u)) - 127.0) * 0.05;
}

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

void main()
{
	precise vec3 pos   = vec3(inOld.xyz) * pc.lerp.y + vec3(inNew.xyz) * pc.lerp.x;
	int          flags = int(pc.ambient.w);
	precise vec3 normal;

	if ((flags & MESH_MDC) != 0)
	{
		vec3 newNormal, oldNormal;

		if ((flags & MESH_NEW_COMP) != 0)
		{
			pos      += CompOffset(inNewComp) * pc.lerp.z;
			newNormal = mt.tables[128 + int(inNewComp >> 24)].xyz;
		}
		else
		{
			newNormal = LatLongNormal(((inNew.w >> 8) & 255) * 16, (inNew.w & 255) * 16);
		}

		if (pc.lerp.w < 0.0)
		{
			normal = newNormal;
		}
		else
		{
			if ((flags & MESH_OLD_COMP) != 0)
			{
				pos      += CompOffset(inOldComp) * pc.lerp.w;
				oldNormal = mt.tables[128 + int(inOldComp >> 24)].xyz;
			}
			else
			{
				oldNormal = LatLongNormal(((inOld.w >> 8) & 255) * 16, (inOld.w & 255) * 16);
			}

			normal  = oldNormal * pc.lerp.w + newNormal * pc.lerp.z;
			normal *= QRsqrt(Dot3(normal, normal));
		}
	}
	else if (pc.lerp.w < 0.0)
	{
		normal = LatLongNormal(((inNew.w >> 8) & 255) * 16, (inNew.w & 255) * 16);
	}
	else
	{
		float         a       = float(((inOld.w >> 8) & 255) * 16);
		float         b       = float((inOld.w & 255) * 16);
		precise float lerpLat = a * pc.lerp.z + a * pc.lerp.w;
		precise float lerpLng = b * pc.lerp.z + b * pc.lerp.w;

		normal = LatLongNormal(int(lerpLat), int(lerpLng));
	}

	{
		int  dp    = clamp(int(64.0 * dot(normal, pc.lightDir.xyz)), 0, 63);
		vec3 light = min(floor(pc.ambient.rgb + (float(dp) / 63.0) * pc.directed.rgb), vec3(255.0));

		fragColor = vec4(light / 255.0, pc.lightDir.w);
	}

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
	fragEyePos   = (pc.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
