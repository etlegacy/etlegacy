/* normalMapping_fp.glsl - Normalmapping helper functions */

#if defined(USE_NORMAL_MAPPING)

	// Construct the 3x3 matrix that transforms from tangentspace to worldspace
	// The matrix depends on how the surface is faced (front or back), and if the surface is rendered two-sided.
	// In case the surface is two-sided, and we view the back-side of that surface, we flip tangentspace axis..
	// ..that way, we handle the back-facing-side as if it's the front we're viewing (so the surface gets drawn, and not culled)
	mat3 tangetToWorldMatrix(vec3 tangent, vec3 binormal, vec3 normal)
	{
		#if defined(TWOSIDED)
			if (!gl_FrontFacing)
				return mat3(-tangent.xyz, -binormal.xyz, -normal.xyz);
			else
		#endif // TWOSIDED
				return mat3(tangent.xyz, binormal.xyz, normal.xyz);
	}


	// Compute normal: from texture values to normal vector values. from tangentspace, to worldspace.
	vec3 computeNormal(vec3 textureNormal, mat3 tangentToWorldMatrix)
	{
		// texture values are in the range 0.0 to 1.0. We convert them into the range -1.0 to 1.0
		vec3 N = textureNormal * 2.0 - 1.0;

		// scaling the normal is for debugging only !!   this should be removed in a final version. (possible exploit)
		#if defined(r_NormalScale)
			if (r_NormalScale != 1.0)
			{
				N.z *= r_NormalScale;
			}
		#endif

		N = normalize(tangentToWorldMatrix * N); // we must normalize to get a vector of unit-length..  reflect() needs it

		return N;
	}

#if defined(USE_REFLECTIONS)
	// Compute reflections
	// This function returns the color that's reflected.
	// Note that the cubeProbe's textures of the cubemap are drawn on the inside of a cubeProbe.
	// The reflection-vector is positioned at the center of a cubeProbe, and points to a pixel on the inside of the cubemap
	vec3 computeReflections(vec3 viewDir, vec3 normal, samplerCube envmap0, samplerCube envmap1, float interpolate)
	{
		vec3 R = reflect(viewDir, normal); // the reflection vector
		R.x = -R.x;  //todo:check
		R.y = -R.y;
		//R.z = -R.z; // flip vertically
		vec4 envColor0 = textureCube(envmap0, R).rgba;
		vec4 envColor1 = textureCube(envmap1, R).rgba;
		return mix(envColor0, envColor1, interpolate).rgb;
	}
#endif // end USE_REFLECTIONS

#if defined(USE_SPECULAR)
	// Compute the specular lighting
	// Specular highlights are only visible if you look into the light.
	vec3 computeSpecular(vec3 viewDir, vec3 normal, vec3 lightDir, float exponent)
	{
		float dotNL = dot(normal, lightDir);
		if (dotNL > 0.0)
		{
			vec3 H = normalize(lightDir + viewDir); // the half-vector
			float dotNH = max(0.0, dot(normal, H));
			float intensity = pow(dotNH, exponent);
			return vec3(intensity);
		}
		return vec3(0.0);
	}
#endif // USE_SPECULAR

	// compute the diffuse light term
	// It renders surfaces darker when they are less facing the light.
	// Diffuse lighting is very much visible: If the diffuse light value is 0, the surface will be rendered black.
	// If we don't want pitch black surfaces, we must take care that this value is always >0.
	// The half-Lambert method keeps the diffuse lighting value in the range 0.5 to 1.0
	// You don't have to stick to half-Lambert; You can choose how dark your world should be..
	// ..Just keep the value in a range 0.0 to 1.0 (0=black and dark, higher values = less dark, 1.0 = effectively disabling Lambertian.. the dark is gone)
	float computeDiffuseLighting(vec3 normal, vec3 lightDir)
	{
		float dotNL = dot(normal, lightDir);
		#if defined(r_HalfLambertLighting)
			// half-Lambert starts at 0.5, and fills the range 0.5 to 1.0
			// We want it a bit brighter, so we start at 0.875 (to 1.0)
			float lambert = dotNL * 0.125 + 0.875;
			return lambert*lambert; // square the result (this also makes the result always >0)
		#elif defined(r_WrapAroundLighting)
			return clamp(dotNL + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
		#else // r_WrapAroundLighting
			return clamp(dotNL, 0.0, 1.0); // maximum Lambertian applied
		#endif // r_WrapAroundLighting, r_HalfLambertLighting
	}

#endif
