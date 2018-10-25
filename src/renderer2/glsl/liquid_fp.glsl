/* liquid_fp.glsl */

uniform sampler2D u_CurrentMap;
uniform samplerCube u_PortalMap;
uniform sampler2D u_DepthMap;
uniform sampler2D u_NormalMap;
uniform vec3      u_ViewOrigin;
uniform float     u_FogDensity;
uniform vec3      u_FogColor;
uniform float     u_RefractionIndex;
uniform float     u_FresnelPower;
uniform float     u_FresnelScale;
uniform float     u_FresnelBias;
uniform float     u_NormalScale;
uniform mat4      u_ModelMatrix;
uniform mat4      u_UnprojectMatrix;
uniform vec3      u_LightDir;
uniform vec3      u_LightColor;
uniform vec4      u_PortalPlane;
uniform float     u_DepthScale;

varying vec3 var_Position;
varying vec2 var_TexNormal;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;

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
#endif

	// compute view direction in world space/incident ray
	vec3 V = normalize(u_ViewOrigin - var_Position);

	// invert tangent space for twosided surfaces
	mat3 tangentToWorldMatrix;
#if defined(TWOSIDED)
	if(!gl_FrontFacing)
	{
		tangentToWorldMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif
	{
		tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	}

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
	vec2 texNormal = var_TexNormal.st;

#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction

	mat3 worldToTangentMatrix = transpose(tangentToWorldMatrix);

	// compute view direction in tangent space
	vec3 Vts = normalize(worldToTangentMatrix * V);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S);

	// compute texcoords offset
	vec2 texOffset = S * depth;

	texScreen.st += texOffset;
	texNormal.st += texOffset;
	//texSpecular.st += texOffset; FIXME?!
#endif //USE_PARALLAX_MAPPING

	// compute normals
	vec3 N = var_Normal.xyz;

#if defined(r_NormalScale)
	if (r_NormalScale != 1.0) N.z *= r_NormalScale;
#endif

	vec3 N2 = 2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5); // FIXME: normalize?
	N2 = normalize(tangentToWorldMatrix * N2);

	vec3 N3 = normalize(N + V);

	// compute fresnel term
	float fresnel = clamp(u_FresnelBias + pow(1.0 - dot(N2, N3), u_FresnelPower) * u_FresnelScale, 0.0, 1.0);

	texScreen += u_NormalScale * N2.xy;

	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;

	// compute reflection ray
	vec3 R = reflect(V, N2);
	vec3 reflectColor = textureCube(u_PortalMap, R).rgb;

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

	// compute light direction in world space
	vec3 L = normalize(u_LightDir); // don't do -L

	// compute half angle in world space
	vec3 H = normalize(L + V);

	// compute the light term
	vec3 light = u_LightColor * clamp(dot(N2, L), 0.0, 1.0);

	// compute the specular term
	vec3 specular = reflectColor * light * pow(clamp(dot(N2, H), 0.0, 1.0), r_SpecularExponent) * r_SpecularScale;
	color.rgb += specular;

	gl_FragColor = color;
}
