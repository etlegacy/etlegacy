/* deformVertexes_vp.glsl - Quake 3 deformVertexes semantic */

uniform float u_DeformParms[MAX_SHADER_DEFORM_PARMS];

float triangle(float x)
{
	return max(1.0 - abs(x), 0);
}

float sawtooth(float x)
{
	return x - floor(x);
}

float floatToRad(float f)
{
	return M_TAU * f;
}

vec4 DeformPosition(const int deformGen,
                    const vec4 wave,	// [base amplitude phase freq]
                    const vec3 bulge,	// [width height speed]
                    const float spread,
                    const float time,
                    const vec4 pos,
                    const vec3 normal,
                    const vec2 st)
{
	vec4 deformed = pos;

	switch(deformGen)
	{
		case DGEN_WAVE_SIN:
		{
			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = wave.x  + sin(floatToRad(off + wave.z + (time * wave.w))) * wave.y;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
		case DGEN_WAVE_SQUARE:
		{
			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = wave.x  + sign(sin(floatToRad(off + wave.z + (time * wave.w)))) * wave.y;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
		case DGEN_WAVE_TRIANGLE:
		{
			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = wave.x  + triangle(off + wave.z + (time * wave.w)) * wave.y;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
		case DGEN_WAVE_SAWTOOTH:
		{
			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = wave.x  + sawtooth(off + wave.z + (time * wave.w)) * wave.y;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
		case DGEN_WAVE_INVERSE_SAWTOOTH:
		{
			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = wave.x + (1.0 - sawtooth(off + wave.z + (time * wave.w))) * wave.y;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
		case DGEN_WAVE_NOISE:
		{
			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = wave.x  + noise1(off + wave.z + (time * wave.w)) * wave.y;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
		case DGEN_BULGE:
		{
			float bulgeWidth = bulge.x;
			float bulgeHeight = bulge.y;
			float bulgeSpeed = bulge.z;

			float now = time * bulgeSpeed;

			float off = (M_PI * 0.25) * st.x * bulgeWidth + now;
			float scale = sin(off) * bulgeHeight;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
			break;
		}
	}

	return deformed;
}

float WaveValue(float func, float base, float amplitude, float phase, float freq, float time)
{
	switch(int(func))
	{
		case GF_SIN:
			return base  + sin(floatToRad(phase + (time * freq))) * amplitude;
		case GF_SQUARE:
			return base  + sign(sin(floatToRad(phase + (time * freq)))) * amplitude;
		case GF_TRIANGLE:
			return base  + triangle(phase + (time * freq)) * amplitude;
		case GF_SAWTOOTH:
			return base  + sawtooth(phase + (time * freq)) * amplitude;
		case GF_INVERSE_SAWTOOTH:
			return base + (1.0 - sawtooth(phase + (time * freq))) * amplitude;
		case GF_NOISE:
			 return base + noise1((time + phase) * freq) * amplitude;
		case GF_NONE:
			break;
	}

	return 0.0;
}

vec4 DeformPosition2(const vec4 pos, const vec3 normal, const vec2 st, float time)
{
	int i, deformOfs = 0;
	int numDeforms = int(u_DeformParms[deformOfs++]);

	vec4 deformed = pos;

	for (i = 0; i < numDeforms; i++)
	{
		int deformGen = int(u_DeformParms[deformOfs++]);

		switch(deformGen)
		{
			case DEFORM_WAVE:
			{
				float func      = u_DeformParms[deformOfs++];
				float base      = u_DeformParms[deformOfs++];
				float amplitude = u_DeformParms[deformOfs++];
				float phase     = u_DeformParms[deformOfs++];
				float freq      = u_DeformParms[deformOfs++];

				float spread = u_DeformParms[deformOfs++];

				float off    = (pos.x + pos.y + pos.z) * spread;
				float scale  = WaveValue(func, base, amplitude, phase + off, freq, time);
				vec3  offset = normal * scale;

				deformed.xyz += offset;
				break;
			}
			case DEFORM_BULGE:
			{
				float bulgeWidth  = u_DeformParms[deformOfs++];
				float bulgeHeight = u_DeformParms[deformOfs++];
				float bulgeSpeed  = u_DeformParms[deformOfs++];

				float now = time * bulgeSpeed;

				float off    = (M_PI * 0.25) * st.x * bulgeWidth + now;
				float scale  = sin(off) * bulgeHeight;
				vec3  offset = normal * scale;

				deformed.xyz += offset;
				break;
			}
			case DEFORM_MOVE:
			{
				float func      = u_DeformParms[deformOfs++];
				float base      = u_DeformParms[deformOfs++];
				float amplitude = u_DeformParms[deformOfs++];
				float phase     = u_DeformParms[deformOfs++];
				float freq      = u_DeformParms[deformOfs++];

				vec3  move   = vec3(u_DeformParms[deformOfs++], u_DeformParms[deformOfs++], u_DeformParms[deformOfs++]);
				float scale  = WaveValue(func, base, amplitude, phase, freq, time);
				vec3  offset = move * scale;

				deformed.xyz += offset;
				break;
			}
		}
	}

	return deformed;
}
