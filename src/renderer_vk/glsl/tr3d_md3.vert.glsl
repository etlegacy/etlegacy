#version 450

// GPU-animated MD3 vertex shader (single texture): interpolates the vertex
// between the entity's oldframe and frame like renderer1's
// LerpMeshVertexes_scalar() does on the CPU, then colors it like
// tr3d_skin.vert (lightingDiffuse, or a constant color passed as ambient
// with lightDir 0; lightDir.w is the stage's constant alpha).
//
// Position: old * oldXyzScale + new * newXyzScale (oldXyzScale 0 without
// interpolation). Normal: decoded from the packed lat/long through
// tr.sinTable (FUNCTABLE_SIZE 4096); without interpolation from the new
// frame, with interpolation from the old frame's lat/long scaled by
// newNormalScale + oldNormalScale and truncated, exactly as the CPU code
// (which reads only the old frame's normal there).

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
	mat4 modelView;
	vec4 alphaTest;
	vec4 fogColor;
	vec4 fogParams;
	vec4 clipPlane;
	vec4 lightDir;  // entity lightDir (model space), w = stage alpha
	vec4 ambient;   // entity ambientLight or constant color (0..255)
	vec4 directed;  // entity directedLight (0..255)
	vec4 lerp;      // newXyzScale, oldXyzScale, newNormalScale, oldNormalScale (< 0: no interpolation)
} pc;

layout(set = 1, binding = 0) uniform SinTable
{
	vec4 sinTable4[1024]; // tr.sinTable
} st;

layout(location = 0) in ivec4 inNew; // frame: xyz, packed lat/long normal
layout(location = 1) in ivec4 inOld; // oldframe
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogDepth;
layout(location = 3) out vec3 fragEyePos;

float SinT(int i)
{
	return st.sinTable4[i >> 2][i & 3];
}

void main()
{
	precise vec3 pos = vec3(inOld.xyz) * pc.lerp.y + vec3(inNew.xyz) * pc.lerp.x;
	int          lat, lng;
	vec3         normal;

	if (pc.lerp.w < 0.0)
	{
		lat = ((inNew.w >> 8) & 255) * 16;
		lng = (inNew.w & 255) * 16;
	}
	else
	{
		float a = float(((inOld.w >> 8) & 255) * 16);
		float b = float((inOld.w & 255) * 16);
		precise float lerpLat = a * pc.lerp.z + a * pc.lerp.w;
		precise float lerpLng = b * pc.lerp.z + b * pc.lerp.w;

		lat = int(lerpLat);
		lng = int(lerpLng);
	}

	normal = vec3(SinT((lat + 1024) & 4095) * SinT(lng), SinT(lat) * SinT(lng), SinT((lng + 1024) & 4095));

	{
		int  dp    = clamp(int(64.0 * dot(normal, pc.lightDir.xyz)), 0, 63);
		vec3 light = min(floor(pc.ambient.rgb + (float(dp) / 63.0) * pc.directed.rgb), vec3(255.0));

		fragColor = vec4(light / 255.0, pc.lightDir.w);
	}

	gl_Position  = pc.mvp * vec4(pos, 1.0);
	fragUV       = inUV;
	fragEyePos   = (pc.modelView * vec4(pos, 1.0)).xyz;
	fragFogDepth = -fragEyePos.z;
}
