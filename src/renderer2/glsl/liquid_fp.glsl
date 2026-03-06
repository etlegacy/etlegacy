/* liquid_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

// fog
uniform sampler2D u_DepthMap;
uniform mat4      u_UnprojectMatrix;
uniform vec3      u_FogColor;
uniform float     u_FogDensity;
uniform float     u_NormalScale;
//
uniform sampler2D u_CurrentMap;
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D u_NormalMap;
// fresnel
uniform float     u_FresnelBias;
uniform float     u_FresnelPower;
uniform float     u_FresnelScale;
// refraction
uniform float     u_RefractionIndex;
// reflection
#if defined(USE_REFLECTIONS)
#if 1
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#else // 1
uniform samplerCube u_PortalMap;
#endif // 1
#endif // USE_REFLECTIONS
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_LightColor;
varying vec3 var_ViewOrigin; // position - vieworigin
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec2 var_TexNormal;
#if defined(USE_WATER)
varying vec2 var_TexNormal2;
#endif
varying vec3 var_LightDirection; 
varying vec3 var_ViewOrigin2; // vieworigin in worldspace
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S; // size and start position of search in texture space
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
#if defined(USE_PORTAL_CLIPPING)
	if (var_BackSide < 0.0)
	{
		discard;
		return;
	}
#endif // USE_PORTAL_CLIPPING


#if defined(USE_NORMAL_MAPPING)

	// light direction in world space
	vec3 L = var_LightDirection;

	// view direction
	vec3 V = var_ViewOrigin.xyz; // tangentspace
	vec3 Vw = var_ViewOrigin2.xyz; // worldspace

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
	vec2 texNormal = var_TexNormal;

#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction
	float depth = RayIntersectDisplaceMap(texNormal, var_S, u_NormalMap);
	// compute texcoords offset
	vec2 texOffset = var_S * depth;

	texScreen += texOffset;
	texNormal += texOffset;
#endif //USE_PARALLAX_MAPPING


	// normal
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0; // static bumpmap
#if defined(USE_WATER)
	vec3 Ntex2 = texture2D(u_NormalMap, var_TexNormal2).xyz * 2.0 - 1.0; // possibly moving bumpmap
	Ntex += Ntex2; // add the waves. interference will cause the final wave's amplitude to vary
#endif
	// transform normal from tangentspace to worldspace
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it


	// the final color
	vec4 color;

	// compute fresnel term
	// ratio reflection/refraction
	float dotNV = max(0.0, dot(N, V)); // float dotNV = dot(var_Normal, V);
	float fresnel = clamp(u_FresnelBias + pow(dotNV, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);

	// refraction
	vec3 T = refract(V, N, u_RefractionIndex);
	texScreen += (N.xy - T.xy) * u_NormalScale;
	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;

	// reflection
#if defined(USE_REFLECTIONS)
#if 1
	// use the cubeProbes
	vec3 reflectColor = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation);
#else // 1
	// use a portalmap
	vec3 reflectColor = textureCube(u_PortalMap, R).rgb;
#endif // 1
	color.rgb = mix(refractColor, reflectColor, fresnel);
#else // USE_REFLECTIONS
	// reflections disabled
	color.rgb = refractColor;
#endif // USE_REFLECTIONS

	color.a   = 1.0; // do not blend (it would blend the currentMap with the water-surface, and you'd see things double)


#else // USE_NORMAL_MAPPING


	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;

	vec4 color;
	color.rgb = texture2D(u_CurrentMap, texScreen).rgb;
	color.a   = 1.0;

#endif // USE_NORMAL_MAPPING


	if (u_FogDensity > 0.0)
	{
		// reconstruct vertex position in world space
		float depth = texture2D(u_DepthMap, texScreen).r;
		vec4  P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
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
	color.rgb += computeSpecular(V, N, L, vec3(1.0), 512.0);
#endif // USE_SPECULAR

	// compute the light term
	color.rgb *= var_LightColor.rgb;

	gl_FragColor = color;
}
