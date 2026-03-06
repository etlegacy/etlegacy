/* lightMapping_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform bool SHOW_LIGHTMAP;
//uniform bool SHOW_DELUXEMAP;

uniform int       u_AlphaTest;
uniform vec3      u_LightColor;
uniform sampler2D u_DiffuseMap;
uniform sampler2D u_LightMap;
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D u_NormalMap;
#if defined(USE_PARALLAX_MAPPING)
uniform float     u_DepthScale;
#endif // USE_PARALLAX_MAPPING
//#if defined(USE_DELUXE_MAPPING)
//uniform sampler2D u_DeluxeMap;
//#endif // USE_DELUXE_MAPPING
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
uniform sampler2D u_SpecularMap;
#if defined(USE_REFLECTIONS)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif // USE_REFLECTIONS
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_Color;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexLight;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
varying vec3 var_LightDirection; 
varying vec3 var_ViewOrigin; // position - vieworigin
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S; // size and start position of search in texture space
varying vec3 var_ViewOrigin2; // in worldspace
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


	// get the color from the lightmap
	vec4 lightmapColor  = texture2D(u_LightMap, var_TexLight);

//#if defined(USE_DELUXE_MAPPING)
//	//fixme: must be done when there IS a deluxemap. not when we want-to-see (show) deluxemaps (and just pass -some- normalmap as a substitude :S)
//	vec4 deluxemapColor = vec4(0.0, 0.0, 0.0, 1.0);
//	if (SHOW_DELUXEMAP)
//  {
//		deluxemapColor = texture2D(u_DeluxeMap, var_TexLight);
//	}
//#endif // USE_DELUXE_MAPPING



#if defined(USE_NORMAL_MAPPING)

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st;
#endif // USE_REFLECTIONS || USE_SPECULAR


#if defined(USE_PARALLAX_MAPPING)
#if 0
	// ray intersect in view direction
	float depth = RayIntersectDisplaceMap(texNormal, var_S, u_NormalMap);
//float depth = texture2D(u_NormalMap, texNormal).a;
	// compute texcoords offset
	vec2 texOffset = var_S * depth;
#endif
vec3 parallax = parallaxReliefMappingOffset(var_ViewOrigin, var_S, texNormal, u_NormalMap);
vec2 texOffset = parallax.st;
float parallaxHeight = parallax.z;

	texDiffuse  -= texOffset;
	texNormal   -= texOffset;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	texSpecular -= texOffset;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_PARALLAX_MAPPING


	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

#if defined(USE_ALPHA_TESTING)
	if (u_AlphaTest == ATEST_GT_0 && diffuse.a <= 0.0)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_LT_128 && diffuse.a >= 0.5)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_GE_128 && diffuse.a < 0.5)
	{
		discard;
		return;
	}
#endif


	// view direction
	vec3 V = var_ViewOrigin;

	// light direction in world space
	vec3 L = var_LightDirection;

	// normal
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0;
	// transform normal from tangentspace to worldspace
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it
//vec3 N = Ntex;

	// the angle between the normal- & light-directions (needs normalized vectors to return the cosine of the angle)
	float dotNL = dot(N, L);


	// compute the specular term (and reflections)
	//! https://en.wikipedia.org/wiki/Specular_highlight
#if defined(USE_SPECULAR) && !defined(USE_REFLECTIONS)
	vec4 map = texture2D(u_SpecularMap, texSpecular);
	vec3 specular = computeSpecular2(dotNL, V, N, L, u_LightColor, r_SpecularExponent) * map.rgb;
#elif defined(USE_SPECULAR) && defined(USE_REFLECTIONS)
	vec4 map = texture2D(u_SpecularMap, texSpecular);
	vec3 specular = (computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation) * 0.07)
					+ (computeSpecular2(dotNL, V, N, L, u_LightColor, r_SpecularExponent) * map.rgb);
#elif !defined(USE_SPECULAR) && defined(USE_REFLECTIONS)
	vec4 map = texture2D(u_SpecularMap, texSpecular);
	vec3 specular = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation) * map.rgb * 0.07;
#endif

#if defined(r_diffuseLighting)
	// compute the diffuse light term
	diffuse.rgb *= computeDiffuseLighting2(dotNL);
#endif // r_diffuseLighting

	// compute final color
	vec4 color = diffuse;
#if defined(USE_NORMAL_MAPPING)
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING
   
	color *= lightmapColor; // we must blend using the lightmap.a

#else // USE_NORMAL_MAPPING
																  
	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, var_TexDiffuseNormal.st);

#if defined(USE_ALPHA_TESTING)
	if (u_AlphaTest == ATEST_GT_0 && diffuse.a <= 0.0)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_LT_128 && diffuse.a >= 0.5)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_GE_128 && diffuse.a < 0.5)
	{
		discard;
		return;
	}
#endif // USE_ALPHA_TESTING

	// compute the final color
	vec4 color = diffuse;
	color *= lightmapColor;

#endif // USE_NORMAL_MAPPING


	// show only the lightmap?
	if (SHOW_LIGHTMAP)
	{
		gl_FragColor = texture2D(u_LightMap, var_TexLight);
		return;
	}

//#if defined(USE_DELUXE_MAPPING)
//	// show only the deluxemap?
//	if (SHOW_DELUXEMAP)
//  {
//		gl_FragColor = texture2D(u_DeluxeMap, var_TexLight);
//		return;
//	}
//#endif // USE_DELUXE_MAPPING

	gl_FragColor = color;
}
