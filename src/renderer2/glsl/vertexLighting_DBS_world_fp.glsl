/* vertexLighting_DBS_world_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform float     u_DiffuseLighting;
uniform sampler2D u_DiffuseMap;
uniform float     u_LightWrapAround;
#if defined(USE_NORMAL_MAPPING)
uniform vec3      u_LightColor;
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
#endif // USE_NORMAL_MAPPING
#if defined(USE_ALPHA_TESTING)
uniform int u_AlphaTest;
#endif // USE_ALPHA_TESTING

varying vec3 var_Position;
varying vec4 var_Color;
varying vec4 var_LightColor;
varying vec2 var_TexDiffuse;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec3 var_LightDirection;
varying vec3 var_ViewOrigin;
#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S;
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
#endif


#if defined(USE_NORMAL_MAPPING)

	// texture coordinates
	vec2 texDiffuse = var_TexDiffuse;
#if defined(USE_PARALLAX_MAPPING)
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


	// view direction
	vec3 V = var_ViewOrigin;

	// light direction
	vec3 L = var_LightDirection;

	// normal
	vec3 Ntex = texture2D(u_NormalMap, texDiffuse).xyz * 2.0 - 1.0;
	// transform normal from tangentspace to worldspace
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	// the cosine of the angle N L (needs unit-vectors)
	float dotNL = dot(N, L);


	// compute the specular term (and reflections)
	//! https://en.wikipedia.org/wiki/Specular_highlight
#if defined(USE_SPECULAR)
	vec3 specular = computeSpecular2(dotNL, V, N, L, u_LightColor, u_SpecularExponent, u_SpecularScale);
	specular *= texture2D(u_SpecularMap, texDiffuse).rgb;
#endif // USE_SPECULAR
#if defined(USE_REFLECTIONS)
	vec3 reflections = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#if defined(USE_REFLECTIONMAP)
	reflections *= texture2D(u_ReflectionMap, texDiffuse).rgb;
#endif // USE_REFLECTIONMAP
#endif // USE_REFLECTIONS


	// compute the diffuse light term
	diffuse.rgb *= computeDiffuseLighting(dotNL, u_DiffuseLighting);



	// compute final color
	vec4 color = diffuse * var_LightColor;
#if defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_SPECULAR
#if defined(USE_REFLECTIONS)
	color.rgb += reflections;
#endif // USE_REFLECTIONS
	gl_FragColor = color;



#else // no normal mapping


	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, var_TexDiffuse);

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

	vec4 color = diffuse * var_LightColor;
	gl_FragColor = color;

#endif // USE_NORMAL_MAPPING
}
