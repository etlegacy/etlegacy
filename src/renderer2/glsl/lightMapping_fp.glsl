/* lightMapping_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform bool SHOW_LIGHTMAP;
//uniform bool SHOW_DELUXEMAP;

uniform float     u_DiffuseLighting;
uniform sampler2D u_LightMap;
#if defined(USE_DIFFUSE)
uniform sampler2D u_DiffuseMap;
#if defined(USE_ALPHA_TESTING)
uniform int       u_AlphaTest;
#endif // USE_ALPHA_TESTING
#if defined(USE_NORMAL_MAPPING)
uniform vec3      u_LightColor;
uniform sampler2D u_NormalMap;
//#if defined(USE_DELUXE_MAPPING)
//uniform sampler2D u_DeluxeMap;
//#endif // USE_DELUXE_MAPPING
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
uniform sampler2D   u_ReflectionMap;
#endif // USE_REFLECTIONMAP
#endif  // USE_REFLECTIONS
#if defined(USE_PARALLAX_MAPPING)
uniform float u_DepthScale;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#endif // USE_DIFFUSE

varying vec3 var_Position;
varying vec4 var_Color;
varying vec2 var_TexLight;
varying vec3 var_Normal;
#if defined(USE_DIFFUSE)
varying vec2 var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying vec3 var_Tangent;
varying vec3 var_biNormal;
varying mat3 var_tangentMatrix;
varying vec3 var_LightDirection; 
varying vec3 var_ViewOrigin;
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S;
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#endif // USE_DIFFUSE
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
	vec4 lightmapColor = texture2D(u_LightMap, var_TexLight);


//#if defined(USE_DELUXE_MAPPING)
//	//fixme: must be done when there IS a deluxemap. not when we want-to-see (show) deluxemaps (and just pass -some- normalmap as a substitude :S)
//	vec4 deluxemapColor = vec4(0.0, 0.0, 0.0, 1.0);
//	if (SHOW_DELUXEMAP)
//  {
//		deluxemapColor = texture2D(u_DeluxeMap, var_TexLight);
//	}
//#endif // USE_DELUXE_MAPPING


	// render a lightmapped face
#if defined(USE_DIFFUSE)
	vec2 texDiffuse = var_TexDiffuse;
#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction
	texDiffuse += RayIntersectDisplaceMap(texDiffuse, var_S, u_NormalMap);
#endif // USE_PARALLAX_MAPPING


	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

	// alpha masking
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

#else // USE_DIFFUSE
	vec4 diffuse = vec4(1.0);
#endif // USE_DIFFUSE


	// override showing only the lightmap?
	if (SHOW_LIGHTMAP)
	{
		gl_FragColor = lightmapColor;
		return;
	}


#if defined(USE_NORMAL_MAPPING)

	// view direction
	vec3 V = var_ViewOrigin;

	// light direction in world space
	vec3 L = var_LightDirection;

	// normal
	vec3 Ntex = texture2D(u_NormalMap, texDiffuse).xyz * 2.0 - 1.0;
	// transform normal from tangentspace to worldspace
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	// the angle between the normal- & light-direction (needs normalized vectors to return the cosine of the angle)
	float dotNL = dot(N, L);


	// compute the specular term (and reflections)
#if defined(USE_SPECULAR)
	vec3 specular = computeSpecular2(dotNL, V, N, L, u_LightColor, u_SpecularExponent, u_SpecularScale);
	specular *= texture2D(u_SpecularMap, texDiffuse).rgb;
#endif // USE_SPECULAR
// we only render reflections in the lightmap shader , when there's a reflectionmap.
#if defined(USE_REFLECTIONS)
	vec3 reflections = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#if defined(USE_REFLECTIONMAP)
	reflections *= texture2D(u_ReflectionMap, texDiffuse).rgb;
#endif // USE_REFLECTIONMAP
#endif // USE_REFLECTIONS


	// compute the diffuse light term
	diffuse.rgb *= computeDiffuseLighting(dotNL, u_DiffuseLighting);


	// compute final color
	vec4 color = diffuse;
#if defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_SPECULAR
#if defined(USE_REFLECTIONS)
	color.rgb += reflections;
#endif // USE_REFLECTIONS
	color *= lightmapColor; // we must blend using the lightmap.a

#else // USE_NORMAL_MAPPING


	// compute final color
	vec4 color = diffuse * lightmapColor; // we must blend using the lightmap.a

#endif // USE_NORMAL_MAPPING


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
