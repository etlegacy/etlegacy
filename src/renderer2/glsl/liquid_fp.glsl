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
#if defined(USE_DIFFUSE)
uniform sampler2D u_DiffuseMap;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D u_NormalMap;
uniform vec3      u_LightColor;
// fresnel
uniform float     u_FresnelBias;
uniform float     u_FresnelPower;
uniform float     u_FresnelScale;
// refraction
uniform float     u_RefractionIndex;
// reflection
#if defined(USE_REFLECTIONS)
uniform float       u_ReflectionScale;
#if 1
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#else // 1
uniform samplerCube u_PortalMap;
#endif // 1
#endif // USE_REFLECTIONS
// specular
uniform float u_SpecularScale;
uniform float u_SpecularExponent;
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_LightColor;
varying vec3 var_Normal;
#if defined(USE_DIFFUSE)
varying vec2 var_TexDiffuse;
varying float var_alphaGen;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec2 var_TexNormal;
varying vec3 var_LightDirection; 
varying vec3 var_ViewDirW;          // view direction in world space
#if defined(USE_PARALLAX_MAPPING)
varying vec3 var_ViewDirT;          // view direction in tangentspace
varying float var_distanceToCam;    //
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


	vec4 color; // the final color

#if defined(USE_DIFFUSE)
	vec2 texDiffuse = var_TexDiffuse;
#endif


#if defined(USE_NORMAL_MAPPING)

	// calculate the screen texcoord in the 0.0 to 1.0 range
#if 0
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
#else
	vec2 texScreen = gl_FragCoord.st * r_FBufNPOTScale;
#endif
	vec2 texNormal = var_TexNormal;

#if defined(USE_PARALLAX_MAPPING)
	// compute texcoords offset
//	vec2 texOffset = RayIntersectDisplaceMap(texNormal, var_S, u_NormalMap);
//	texScreen  += texOffset;
//	texNormal  += texOffset;
	texScreen = parallax(u_NormalMap, texDiffuse, var_ViewDirT, u_DepthScale);
	texNormal = texDiffuse; // needs same resolution normalmap as diffusemap..
#if defined(USE_DIFFUSE)
	texDiffuse = texNormal;
#endif
#endif //USE_PARALLAX_MAPPING



	// light direction in world space
	vec3 L = var_LightDirection;

	// view direction
	vec3 V = var_ViewDirW;

	// normal
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0; // static bumpmap
#if defined(USE_WATER)
	vec3 Ntex2 = texture2D(u_NormalMap, texDiffuse).xyz * 2.0 - 1.0; // tcMod moving bumpmap
	Ntex += Ntex2; // add the waves. interference will cause the final wave's amplitude to vary
#endif
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	// refraction
	vec3 T = refract(V, N, u_RefractionIndex);
	texScreen += (N.xy - T.xy) * u_NormalScale;
	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;

	// reflection
#if defined(USE_REFLECTIONS)
	// compute fresnel term
	// ratio reflection/refraction
//!	float dotNV = max(0.0, dot(N, V));
float dotNL = max(0.0, dot(N, -L));
//!	float fresnel = clamp(u_FresnelBias + pow(dotNV, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
float fresnel = clamp(u_FresnelBias + pow(dotNL, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
#if 1
	// use the cubeProbes
	vec3 reflectColor = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#else // 1
	// use a portalmap
	vec3 R = reflect(V, N); // the reflection vector
	vec3 reflectColor = textureCube(u_PortalMap, R).rgb;
#endif // 1
	color.rgb = mix(reflectColor, refractColor, fresnel);
#else // USE_REFLECTIONS
	// reflections disabled
	color.rgb = refractColor;
#endif // USE_REFLECTIONS

	// diffuse lighting.. why not? :)
	color.rgb *= computeDiffuseLighting(dot(N,L), 0.4); // 0.4 value seems good for water(waves)

	// compute the specular term
	// We don't use u_SpecularExponent here, but instead a constant value, because water is always +- the same.
	// That's better than a wild value that a client could enter.
//	vec3 specular = computeSpecular(V, N, L, u_LightColor, 512.0, u_SpecularScale); // u_SpecularExponent
vec3 specular = computeSpecular(V, N, L, u_LightColor, 256.0, u_SpecularScale); // u_SpecularExponent


#else // USE_NORMAL_MAPPING


	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;

	color.rgb = texture2D(u_CurrentMap, texScreen).rgb;

#endif // USE_NORMAL_MAPPING



	// blend a diffuse texture?
#if defined(USE_DIFFUSE)
	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);
	color.rgb = mix(color.rgb, diffuse.rgb, var_alphaGen);
//color += diffuse;
#endif // USE_DIFFUSE



	// the water-surface fog
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

	// compute the light term
	color.rgb *= var_LightColor.rgb;


#if defined(USE_NORMAL_MAPPING)
	color.rgb += specular; // liquids need no specularmap. Liquids have the specular term calculated from any provided normalmap
#endif // USE_NORMAL_MAPPING

	color.a = 1.0; // do not blend (it would blend the currentMap with the water-surface, and you'd see things double (refracted and currentmap)

	gl_FragColor = color;
}
