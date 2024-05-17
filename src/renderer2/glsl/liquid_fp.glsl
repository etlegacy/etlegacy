/* liquid_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform bool SHOW_LIGHTMAP;
//uniform bool SHOW_DELUXEMAP;
uniform bool UNDERWATER;

uniform vec4 u_Color;
uniform sampler2D u_DepthMap;
uniform mat4      u_UnprojectMatrix;
uniform vec3      u_FogColor;
uniform float     u_FogDensity;
uniform float     u_NormalScale;
uniform sampler2D u_CurrentMap;
#if defined(USE_LIGHT_MAPPING)
	uniform sampler2D u_LightMap;
#endif // USE_LIGHT_MAPPING
#if defined(USE_DIFFUSE)
	uniform sampler2D u_DiffuseMap;
#endif // USE_DIFFUSE
#if defined(USE_NORMAL_MAPPING)
	uniform sampler2D u_NormalMap;
	// sunlight
	//uniform vec3      u_LightDir;
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
varying vec4 var_Color;
#if defined(USE_LIGHT_MAPPING)
	varying vec2 var_TexLight;
	varying vec4 var_LightmapColor;
#endif // USE_LIGHT_MAPPING
#if defined(USE_WATER) || defined(USE_DIFFUSE)
	varying vec2 var_TexDiffuse;        // possibly moving coords
#endif
#if defined(USE_DIFFUSE)
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
	vec4 lightmapColor;

#if defined(USE_LIGHT_MAPPING)
	// get the color from the lightmap
	lightmapColor = texture2D(u_LightMap, var_TexLight);
#else
	lightmapColor = vec4(1.0, 1.0, 1.0, 1.0);
	//lightmapColor = var_LightmapColor;
#endif // USE_LIGHT_MAPPING

	// override showing only the lightmap?
	if (SHOW_LIGHTMAP)
	{
		gl_FragColor = lightmapColor;
		return;
	}


#if defined(USE_WATER) || defined(USE_DIFFUSE)
	vec2 texDiffuse = var_TexDiffuse;
#endif


#if !defined(USE_NORMAL_MAPPING)
	// normal mapping is disabled.
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
	color.rgb = texture2D(u_CurrentMap, texScreen).rgb;
#else // USE_NORMAL_MAPPING
	// the view direction in tangentspace
	vec3 V = normalize(var_ViewDirT);

	// the light direction
	vec3 L = var_LightDirT;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufNPOTScale;
	vec2 texNormal = var_TexNormal;

#if defined(USE_PARALLAX_MAPPING)
	// compute texcoords offset
	vec3 parallaxResult = parallaxAndShadow(u_NormalMap, texDiffuse, V, L, u_DepthScale, var_distanceToCam, u_ParallaxShadow, lightmapColor.rgb);
	texScreen = parallaxResult.xy;
	float parallaxShadow = parallaxResult.z;
	texNormal = texDiffuse; // needs same resolution normalmap as diffusemap..
#endif //USE_PARALLAX_MAPPING


	// pixel normal
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


	// set the initial color to the refracted scene
	color.rgb = refractColor;



	// reflection
#if defined(USE_REFLECTIONS)
	vec3 reflectColor;
	float dotNV = dot(N, V);
#if 0
	// Always use the cubeProbes, if above or below the watersurface.
	// compute fresnel term. This is the ratio reflection/refraction.
	// Value 1.0 = only refraction, no reflection.   0.0 = only reflection, no refraction.
	float fresnel = 1.0 - clamp(u_FresnelBias + pow(abs(dotNV), u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
	reflectColor = computeReflectionsW(V, N, var_worldMatrix, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#else
	// surface reflections above/under water are different
	float fresnel = clamp(u_FresnelBias + pow(1.0 + dotNV, u_FresnelPower) * u_FresnelScale, 0.0, 1.0);
	if (!UNDERWATER) {
		// Above surface: use the cubeProbes
		reflectColor = computeReflectionsW(V, N, var_worldMatrix, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
	} else {
#if 1
		// Below surface: use the currentmap
				const vec3 surfaceNormalT = vec3(0.0, 0.0, 1.0); // the tangentspace surface normal is just a constant
		vec3 R = reflect(V, -surfaceNormalT); // the reflection vector
		vec2 texScreen2 = texScreen + ((N.xy - R.xy) * u_NormalScale);
		reflectColor = texture2D(u_CurrentMap, texScreen2.st).rgb;
#else
		// Below surface: no reflections
		reflectColor = refractColor;
#endif
	}
#endif
#endif // USE_REFLECTIONS


	// diffuse lighting..
//	color.rgb *= computeDiffuseLighting(N, L, 0.2);


	// compute the specular term.
	// Liquids need no specularmap. Liquids have the specular term calculated from any provided normalmap.
	// We don't use u_SpecularExponent here, but instead a constant value.
	vec3 specular = computeSpecular(V, N, L, u_LightColor, 64.0, u_SpecularScale); // u_SpecularExponent


#endif // USE_NORMAL_MAPPING



	// blend a diffuse texture?
#if defined(USE_DIFFUSE)
	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);
	color.rgb = mix(color.rgb, diffuse.rgb, var_alphaGen);
//color.rgb = mix(color.rgb, diffuse.rgb, var_Color.a);
	
#endif // USE_DIFFUSE

	
	// the water-surface fog.
	if (!UNDERWATER && (u_FogDensity > 0.0)) {
		// reconstruct vertex position in world space
		float depth = texture2D(u_DepthMap, texScreen).r;
//?		// scale to Normalized Device Coordinates
//?		vec4  P = vec4(gl_FragCoord.xy, depth, 1.0) * 2.0 - 1.0;
		vec4  P = vec4(gl_FragCoord.xy, depth, 1.0);
		// unproject to get into viewspace
		P = u_UnprojectMatrix * P;
		// normalize to homogeneous coordinates
		P.xyz /= P.w;
		// calculate fog distance
		float fogDistance = distance(P.xyz, var_Position);
		// calculate fog exponent
		float fogExponent = fogDistance * u_FogDensity;
		// calculate fog factor
		float fogFactor = 1.0 - clamp(exp2(-fogExponent * fogExponent), 0, 1);
//!		float fogFactor = 1.0 - clamp(exp(-fogExponent * fogExponent), 0, 1);
		// set the color
		color.rgb = mix(color.rgb, u_FogColor, fogFactor);
	}


#if defined(USE_REFLECTIONS)
	color.rgb = mix(color.rgb, reflectColor, fresnel);
#endif

#if defined(USE_NORMAL_MAPPING)
	color.rgb += specular;
#endif // USE_NORMAL_MAPPING

#if defined(USE_LIGHT_MAPPING)
	// lightmap
	color.rgb *= lightmapColor.rgb;
#endif

#if defined(USE_PARALLAX_MAPPING)
	color.rgb *= parallaxShadow;
#endif
	color.a = 1.0; // do not blend (it would blend the currentMap with the water-surface, and you'd see things double (refracted and currentmap)

	gl_FragColor = color;
}
