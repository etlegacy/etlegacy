/* liquid_fp.glsl */

uniform mat4      u_UnprojectMatrix;
uniform vec3      u_ViewOrigin;
uniform vec3      u_LightDir;
uniform vec4      u_PortalPlane;
uniform sampler2D u_CurrentMap;
uniform sampler2D u_DepthMap;
uniform float     u_FogDensity;
uniform vec3      u_FogColor;
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D u_NormalMap;
uniform float     u_NormalScale;
uniform float     u_RefractionIndex;
uniform float     u_FresnelPower;
uniform float     u_FresnelScale;
uniform float     u_FresnelBias;
#if defined(USE_PARALLAX_MAPPING)
uniform float     u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#if defined(USE_REFLECTIONS)
#if 0
uniform samplerCube u_PortalMap;
#else // 0
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif // 0
#endif // USE_REFLECTIONS
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_LightColor;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec2 var_TexNormal;
#if defined(USE_WATER)
varying vec2 var_TexNormal2;
#endif
#endif // USE_NORMAL_MAPPING


// We define a compiler directive if we want the faster transform code.
// If you comment the next line, a matrix is created and used.
#define transformFast


#if defined(USE_PARALLAX_MAPPING)
float RayIntersectDisplaceMap(vec2 dp, vec2 ds)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);

	// current size of search window
	float size = depthStep;

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for (int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;

		vec4 t = texture2D(u_NormalMap, dp + ds * depth);

		if (bestDepth > 0.996)       // if no depth found yet
		{
			if (depth >= t.w)
			{
				bestDepth = depth;  // store best depth
			}
		}
	}

	depth = bestDepth;

	// recurse around first point (depth) for closest match
	for (int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		vec4 t = texture2D(u_NormalMap, dp + ds * depth);

		if (depth >= t.w)
		#ifdef RM_DOUBLEDEPTH
			if (depth <= t.z)
		#endif
		{
			bestDepth = depth;
			depth    -= 2.0 * size;
		}

		depth += size;
	}

	return bestDepth;
}
#endif //USE_PARALLAX_MAPPING


void main()
{
#if defined(USE_PORTAL_CLIPPING)
	float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
	if (dist < 0.0)
	{
		discard;
		return;
	}
#endif // USE_PORTAL_CLIPPING


#if defined(USE_NORMAL_MAPPING)

	// compute light direction in world space
	vec3 L = -normalize(u_LightDir);

	// compute view direction in world space/incident ray
	vec3 V = normalize(u_ViewOrigin - var_Position);


	// compute view direction in tangent space
#if defined(transformFast) // do not use a matrix, but do the necesarry calculations. it should be just a bit faster.
	vec3 Vts;
#if defined(TWOSIDED)
	if (!gl_FrontFacing)
	{
		Vts.x = -dot(V, -var_Tangent);
		Vts.y = -dot(V, -var_Binormal);
		Vts.z = -dot(V, -var_Normal);
	} 
	else
#endif// TWOSIDED
	{
		Vts.x = dot(V, var_Tangent);
		Vts.y = dot(V, var_Binormal);
		Vts.z = dot(V, var_Normal);
	}
#else // transformFast
	// invert tangent space for two sided surfaces which are looked at on the back side of that surface (surfaces which are backfacing).
	// Create the usual tangent space matrix in case we're dealing with a front-facing surface, or when the surface is not two-sided (the usual case).
	mat3 tangentSpaceMatrix;
#if defined(TWOSIDED)
	if (!gl_FrontFacing)
	{
		tangentSpaceMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif // end TWOSIDED
	{
		tangentSpaceMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	}
	vec3 Vts = normalize(tangentSpaceMatrix * V);
#endif // transformFast


	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
	vec2 texNormal = var_TexNormal.st;

#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction

	//mat3 worldToTangentMatrix = transpose(tangentSpaceMatrix);

	// compute view direction in tangent space
	//vec3 Vts = normalize(worldToTangentMatrix * V);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S);

	// compute texcoords offset
	vec2 texOffset = S * depth;

	texScreen.st += texOffset;
	texNormal.st += texOffset;
#endif //USE_PARALLAX_MAPPING

	// normal(s)
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0; // static bumpmap
#if defined(USE_WATER)
	vec3 Ntex2 = texture2D(u_NormalMap, var_TexNormal2).xyz * 2.0 - 1.0; // possibly moving bumpmap
	Ntex += Ntex2;
#endif

	vec3 N;
#if defined(transformFast) // do not use a matrix, but do the necesarry calculations. it should be just a bit faster.
	// transform Ntex to tangent space
#if defined(TWOSIDED)
	if (!gl_FrontFacing)
	{
		N.x = dot(Ntex, -var_Tangent);
		N.y = dot(Ntex, -var_Binormal);
		N.z = dot(Ntex, -var_Normal);
	} 
	else
#endif // TWOSIDED
	{
		N.x = dot(Ntex, var_Tangent);
		N.y = dot(Ntex, var_Binormal);
		N.z = dot(Ntex, var_Normal);
	}
	// we must normalize N because otherwise we see white artifacts visible in game
	N = normalize(N);
#else // transformFast
	N = normalize(tangentSpaceMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it
#endif // transformFast

	// compute fresnel term
	// (The fresnel values from cvars produce an opaque water surface..  default values need adjustment)
	float dotNV = dot(N, Vts);
	float amount;
	if (dotNV > 0)
	{
		amount = max(0.0, dotNV); // amount of reflection (distant water reflects more than water nearby, because angle of incedence is greater)
	} 
	else
	{
		amount = 1.0; // no reflections when underwater
	}
	//float fresnel = clamp(u_FresnelBias + pow(1.0 - dot(N, N3), u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
	//float fresnel = clamp(u_FresnelBias + pow(1.0 - amount, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
	float fresnel = 1.0 - amount;

	// refraction
	texScreen += u_NormalScale * N.xy;
	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;

#if defined(USE_REFLECTIONS)
	// reflection
	vec3 R = reflect(V, N);
#if 0
	vec3 reflectColor = textureCube(u_PortalMap, R).rgb;
#else // 0
	vec4 envColor0 = textureCube(u_EnvironmentMap0, R).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, R).rgba;
	vec3 reflectColor = mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;
#endif // 0
#else // USE_REFLECTIONS
	vec3 reflectColor = vec3(0.0);
#endif // USE_REFLECTIONS

	vec4 color;
	color.rgb = mix(refractColor, reflectColor, fresnel);
	color.a   = 1.0;

	if (u_FogDensity > 0.0)
	{
		// reconstruct vertex position in world space
		float depth = texture2D(u_DepthMap, texScreen).r;
		vec4  P     = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
		P.xyz /= P.w;

		// calculate fog distance
		float fogDistance = distance(P.xyz, var_Position);

		// calculate fog exponent
		float fogExponent = fogDistance * u_FogDensity;

		// calculate fog factor
		float fogFactor = exp2(-abs(fogExponent));

		color.rgb = mix(u_FogColor, color.rgb, fogFactor);
	}

#if defined(USE_SPECULAR)
	// compute the specular term
	vec3 specular = vec3(0.0);
	if (dot(-N, L) > 0.0)
	{
		vec3 R = reflect(L, -N);
		float reflectance = pow(max(0.0, dot(R, V)), 64.0); // ,r_SpecularExponent) // * r_SpecularScale;
		specular = vec3(reflectance);
	}
	color.rgb += specular;
#endif // USE_SPECULAR

	// compute the light term
	color.rgb *= var_LightColor.rgb; // * max(dot(-N2, L), 0.0);

	gl_FragColor = color;


#else // USE_NORMAL_MAPPING


	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;

	vec4 color;
	color.rgb = texture2D(u_CurrentMap, texScreen).rgb;
	color.a   = 1.0;


	if (u_FogDensity > 0.0)
	{
		// reconstruct vertex position in world space
		float depth = texture2D(u_DepthMap, texScreen).r;
		vec4  P     = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
		P.xyz /= P.w;

		// calculate fog distance
		float fogDistance = distance(P.xyz, var_Position);

		// calculate fog exponent
		float fogExponent = fogDistance * u_FogDensity;

		// calculate fog factor
		float fogFactor = exp2(-abs(fogExponent));

		color.rgb = mix(u_FogColor, color.rgb, fogFactor);
	}

	// compute the light term
	color.rgb *= var_LightColor.rgb; // * max(dot(-N2, L), 0.0);

	gl_FragColor = color;

#endif // USE_NORMAL_MAPPING
}
