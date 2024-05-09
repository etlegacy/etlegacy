/* entity_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform sampler2D u_DiffuseMap;
uniform vec3      u_LightColor;
uniform float     u_LightWrapAround;
uniform vec3      u_AmbientColor;
#if defined(USE_NORMAL_MAPPING)
uniform float     u_DiffuseLighting;
uniform sampler2D u_NormalMap;
	#if defined(USE_SPECULAR)
uniform sampler2D u_SpecularMap;
uniform float     u_SpecularScale;
uniform float     u_SpecularExponent;
	#endif // USE_SPECULAR
	#if defined(USE_REFLECTIONS)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
uniform float       u_ReflectionScale;
		#if defined(USE_REFLECTIONMAP)
uniform sampler2D u_ReflectionMap;
		#endif // USE_REFLECTIONMAP
	#endif // USE_REFLECTIONS
	#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
uniform float u_ParallaxShadow;
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_ALPHA_TESTING)
uniform int u_AlphaTest;
#endif // USE_ALPHA_TESTING

varying vec3 var_Position;
varying vec2 var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying mat3 var_worldMatrix;
varying vec2 var_TexNormal;
varying vec3 var_LightDirT;                 // light direction in tangentspace
varying vec3 var_ViewDirT;                  // view direction in tangentspace
	#if defined(USE_PARALLAX_MAPPING)
varying float var_distanceToCam;            //
	#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide;     // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
#if defined(USE_PORTAL_CLIPPING)
	if (var_BackSide < 0.0)
	{
		discard;
		return;
	}
#endif // end USE_PORTAL_CLIPPING


#if defined(USE_NORMAL_MAPPING)
	// the view direction in tangentspace
	vec3 V = normalize(var_ViewDirT);

	// the light direction
	vec3 L = var_LightDirT;

#if defined(USE_PARALLAX_MAPPING)
	const vec3 lightmapColor  = vec3(1.0, 1.0, 1.0);
	vec3       parallaxResult = parallaxAndShadow(u_NormalMap, var_TexDiffuse, V, L, u_DepthScale, var_distanceToCam, u_ParallaxShadow, lightmapColor);
	vec2       texDiffuse     = parallaxResult.xy;
	float      parallaxShadow = parallaxResult.z;
#else
	vec2 texDiffuse = var_TexDiffuse;
#endif // USE_PARALLAX_MAPPING
#else
	vec2 texDiffuse = var_TexDiffuse; // diffuse texture coordinates st
#endif // USE_NORMAL_MAPPING


	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse); // the color at coords(st) of the diffuse texture

	// alphaFunc
#if defined(USE_ALPHA_TESTING)
	if (u_AlphaTest == ATEST_GE_128 && diffuse.a < 0.5)
	{ // this is the one used when implicitMask is used (possibly refered to the most.. we check this one first)
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_GT_0 && diffuse.a <= 0.0)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_LT_128 && diffuse.a >= 0.5)
	{
		discard;
		return;
	}
#endif // end USE_ALPHA_TESTING



#if defined(USE_NORMAL_MAPPING)
	// normal
	vec3 Ntex = texture2D(u_NormalMap, texDiffuse).xyz * 2.0 - 1.0;
	vec3 N    = normalize(Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	// compute the diffuse light term
	diffuse.rgb *= computeDiffuseLighting(N, L, u_DiffuseLighting);


	// add Rim Lighting to highlight the edges
#if defined(r_rimLighting)
	float rim      = 1.0 - clamp(dot(N, V), 0, 1);
	vec3  emission = r_rimColor.rgb * pow(rim, r_rimExponent);
#endif // end r_rimLighting


	// compute the specular term (and reflections)
	//! https://en.wikipedia.org/wiki/Specular_highlight
#if defined(USE_SPECULAR)
	vec3 specular = computeSpecular(V, N, L, u_LightColor, u_SpecularExponent, u_SpecularScale);
	specular *= texture2D(u_SpecularMap, texDiffuse).rgb;
#endif // USE_SPECULAR
#if defined(USE_REFLECTIONS)
	vec3 reflections = computeReflectionsW(V, N, var_worldMatrix, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#if defined(USE_REFLECTIONMAP)
	reflections *= texture2D(u_ReflectionMap, texDiffuse).rgb;
#endif // USE_REFLECTIONMAP
#endif // USE_REFLECTIONS



	// compute final color
	vec4 color = vec4(diffuse.rgb, 1.0);
#if defined(USE_PARALLAX_MAPPING)
	color.rgb *= parallaxShadow;
#endif
#if defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_SPECULAR
#if defined(USE_REFLECTIONS)
	color.rgb += reflections;
#endif // USE_REFLECTIONS
#if defined(r_rimLighting)
	color.rgb += emission;
#endif // end r_rimLighting

#else // USE_NORMAL_MAPPING

	vec4 color = vec4(diffuse.rgb, 1.0);

#endif // end USE_NORMAL_MAPPING

	//u_AmbientColor // i really think mapper's ambient values are too high..use this if you want "glowing" entities :P
	color.rgb   *= u_LightColor;
	gl_FragColor = color;
}
