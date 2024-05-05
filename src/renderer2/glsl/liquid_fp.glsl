/* liquid_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform vec4 u_Color;
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
		uniform samplerCube u_EnvironmentMap0;
		uniform samplerCube u_EnvironmentMap1;
		uniform float       u_EnvironmentInterpolation;
	#endif // USE_REFLECTIONS
	// specular
	uniform float u_SpecularScale;
	uniform float u_SpecularExponent;
	#if defined(USE_PARALLAX_MAPPING)
		uniform float u_DepthScale;
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec3 var_Normal;
//varying vec4 var_LightColor;
#if defined(USE_DIFFUSE)
	varying vec2 var_TexDiffuse;
	varying float var_alphaGen;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
	varying mat3 var_tangentMatrix;
	varying mat3 var_worldMatrix;
	varying vec2 var_TexNormal;
	varying vec3 var_LightDirT;         // light direction in tangentspace
	varying vec3 var_ViewDirT;          // view direction in tangentspace
	#if defined(USE_PARALLAX_MAPPING)
		varying float var_distanceToCam;    //
		uniform float u_ParallaxShadow;
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
	varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
#if defined(USE_PORTAL_CLIPPING)
	if (var_BackSide < 0.0)	{
		discard;
		return;
	}
#endif // USE_PORTAL_CLIPPING


	vec4 color; // the final color


#if defined(USE_DIFFUSE)
	vec2 texDiffuse = var_TexDiffuse;
#endif


#if defined(USE_NORMAL_MAPPING)
	// the view direction in tangentspace
	vec3 V = normalize(var_ViewDirT);

	// the light direction
	vec3 L = var_LightDirT;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufNPOTScale;
	vec2 texNormal = var_TexNormal;

#if defined(USE_PARALLAX_MAPPING)
	// compute texcoords offset
	const vec3 lightmapColor = vec3(1.0, 1.0, 1.0);
	vec3 parallaxResult = parallaxAndShadow(u_NormalMap, texDiffuse, V, L, u_DepthScale, var_distanceToCam, u_ParallaxShadow, lightmapColor);
	texScreen = parallaxResult.xy;
	float parallaxShadow = parallaxResult.z;
	texNormal = texDiffuse; // needs same resolution normalmap as diffusemap..
#endif //USE_PARALLAX_MAPPING


	// normal
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0; // static bumpmap
#if defined(USE_WATER)
	vec3 Ntex2 = texture2D(u_NormalMap, texDiffuse).xyz * 2.0 - 1.0; // tcMod moving bumpmap
	Ntex += Ntex2; // add the waves. interference will cause the final wave's amplitude to vary
#endif
	vec3 N = normalize(Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it


	// refraction
	vec3 T = refract(V, N, u_RefractionIndex);
	texScreen += (N.xy - T.xy) * u_NormalScale;
	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;


	// set the initial color to the refracted underwater scene
	color.rgb = refractColor;


	// reflection
#if defined(USE_REFLECTIONS)
	vec3 reflectColor;
	// compute fresnel term
	// ratio reflection/refraction.  Value 1.0 = only refraction, no reflection.   0.0 = only reflection, no refraction.
	float dotNV = dot(N, V);
	float dotAbsNV = abs(dotNV);
#if 1
	float fresnel = clamp(u_FresnelBias + pow(dotAbsNV, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
	// use the cubeProbes
	reflectColor = computeReflectionsW(V, N, var_worldMatrix, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#else
	float fresnel = clamp(u_FresnelBias + pow(1.0 + dotNV, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
	// test surface reflections above/under water are different
	if (dotNV >= 0) {
		// Above surface: use the cubeProbes
		reflectColor = computeReflectionsW(V, N, var_worldMatrix, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
	} else {
		// Below surface: use the currentmap
		vec3 R = reflect(V, N); // the reflection vector
		reflectColor = texture2D(u_CurrentMap, R.st).rgb; // it a test..
	}
#endif
#endif // USE_REFLECTIONS


	// diffuse lighting..
//	color.rgb *= computeDiffuseLighting(N, L, 0.2);


	// compute the specular term
	// We don't use u_SpecularExponent here, but instead a constant value.
	vec3 specular = computeSpecular(V, N, L, u_LightColor, 64.0, u_SpecularScale); // u_SpecularExponent


#else // USE_NORMAL_MAPPING


	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;

	color.rgb = texture2D(u_CurrentMap, texScreen).rgb;

#endif // USE_NORMAL_MAPPING



	// blend a diffuse texture?
#if defined(USE_DIFFUSE)
	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);
	color.rgb = mix(color.rgb, diffuse.rgb, var_alphaGen); // !! var_alphaGen = the alpha value from "color r, g, b, alpha"
#endif // USE_DIFFUSE


	// the water-surface fog
	// Note: if this value is too high, all the translucency of the water gets lost. Use small values..
	// TODO: Before release, normalize this scale, so people can use more normal values in their materials/shaders.
	if (u_FogDensity > 0.0)
	{
		// reconstruct vertex position in world space
		float depth = texture2D(u_DepthMap, texScreen).r;
		// scale to NDC (Normalized Device Coordinates) space
		vec4  P = vec4(gl_FragCoord.xy, depth, 1.0) * 2.0 - 1.0;
		// unproject to get into viewspace
		P = u_UnprojectMatrix * P;
		// normalize to homogeneous coordinates (where w is always 1)
		P.xyz /= P.w;

		// calculate fog distance
		float fogDistance = distance(P.xyz, var_Position);

		// calculate fog exponent
		float fogExponent = fogDistance * u_FogDensity;

		// calculate fog factor
		float fogFactor = 1.0 - clamp(exp2(-fogExponent * fogExponent), 0, 1);

//		color.rgb = mix(color.rgb, u_FogColor, fogFactor);
color.rgb = mix(color.rgb, u_Color.rgb, fogFactor); // test
	}



#if defined(USE_REFLECTIONS)
	color.rgb = mix(reflectColor, color.rgb, fresnel);
#endif


	// compute the light term
//	color.rgb *= var_LightColor.rgb;


#if defined(USE_NORMAL_MAPPING)
	color.rgb += specular; // liquids need no specularmap. Liquids have the specular term calculated from any provided normalmap
#endif // USE_NORMAL_MAPPING
#if defined(USE_PARALLAX_MAPPING)
	color.rgb *= parallaxShadow;
#endif
	color.a = 1.0; // do not blend (it would blend the currentMap with the water-surface, and you'd see things double (refracted and currentmap)

	gl_FragColor = color;
}
