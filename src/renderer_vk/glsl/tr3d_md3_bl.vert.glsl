#version 450

// tr3d_md3.vert for merged model batches (bindless textures, multi-draw
// indirect, tr3d_bl.frag): the vertices are read from the mesh buffer at
// the draw's offsets (one draw's streams can't be bound per draw inside a
// multi-draw), the per-draw values come from the draw's ModelDraw entry.

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
	uvec4 ofs;       // mesh buffer word offsets: frame, oldframe, texcoords, compressed frame
	uvec4 ofs2;      // x: compressed oldframe, y: bindless texture slot
};

layout(std430, set = 1, binding = 0) readonly buffer ModelDraws
{
	ModelDraw draws[];
};

// the mesh buffer (VK_WriteMeshData) as 32-bit words
layout(std430, set = 1, binding = 1) readonly buffer MeshData
{
	uint mesh[];
};

// tr.sinTable[16 * k] and [16 * k - 1] as 256 pairs, then r_anormals
layout(set = 2, binding = 0) uniform MeshTables
{
	vec4 tables[128 + 256];
} mt;



layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;
layout(location = 4) flat out uint fragTexture;

// md3XyzNormal_t at a word offset: short xyz[3], short latlong normal
ivec4 FrameVertex(uint ofs, uint v)
{
	uint a = mesh[ofs + 2u * v], b = mesh[ofs + 2u * v + 1u];

	return ivec4(int(a << 16) >> 16, int(a) >> 16, int(b << 16) >> 16, int(b) >> 16);
}

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
	ModelDraw d         = draws[gl_InstanceIndex];
	uint      v         = uint(gl_VertexIndex);
	ivec4     inNew     = FrameVertex(d.ofs.x, v);
	ivec4     inOld     = FrameVertex(d.ofs.y, v);
	vec2      inUV      = vec2(uintBitsToFloat(mesh[d.ofs.z + 2u * v]), uintBitsToFloat(mesh[d.ofs.z + 2u * v + 1u]));
	uint      inNewComp = mesh[d.ofs.w + v];
	uint      inOldComp = mesh[d.ofs2.x + v];
	precise vec3 pos   = vec3(inOld.xyz) * d.lerp.y + vec3(inNew.xyz) * d.lerp.x;
	int          flags = int(d.ambient.w);
	precise vec3 normal;

	if ((flags & MESH_MDC) != 0)
	{
		vec3 newNormal, oldNormal;

		if ((flags & MESH_NEW_COMP) != 0)
		{
			pos      += CompOffset(inNewComp) * d.lerp.z;
			newNormal = mt.tables[128 + int(inNewComp >> 24)].xyz;
		}
		else
		{
			newNormal = LatLongNormal(((inNew.w >> 8) & 255) * 16, (inNew.w & 255) * 16);
		}

		if (d.lerp.w < 0.0)
		{
			normal = newNormal;
		}
		else
		{
			if ((flags & MESH_OLD_COMP) != 0)
			{
				pos      += CompOffset(inOldComp) * d.lerp.w;
				oldNormal = mt.tables[128 + int(inOldComp >> 24)].xyz;
			}
			else
			{
				oldNormal = LatLongNormal(((inOld.w >> 8) & 255) * 16, (inOld.w & 255) * 16);
			}

			normal  = oldNormal * d.lerp.w + newNormal * d.lerp.z;
			normal *= QRsqrt(Dot3(normal, normal));
		}
	}
	else if (d.lerp.w < 0.0)
	{
		normal = LatLongNormal(((inNew.w >> 8) & 255) * 16, (inNew.w & 255) * 16);
	}
	else
	{
		float         a       = float(((inOld.w >> 8) & 255) * 16);
		float         b       = float((inOld.w & 255) * 16);
		precise float lerpLat = a * d.lerp.z + a * d.lerp.w;
		precise float lerpLng = b * d.lerp.z + b * d.lerp.w;

		normal = LatLongNormal(int(lerpLat), int(lerpLng));
	}

	{
		int  dp    = clamp(int(64.0 * dot(normal, d.lightDir.xyz)), 0, 63);
		vec3 light = min(floor(d.ambient.rgb + (float(dp) / 63.0) * d.directed.rgb), vec3(255.0));

		fragColor = vec4(light / 255.0, d.lightDir.w);
	}

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
	fragEyePos   = (d.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
