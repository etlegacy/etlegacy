/* world_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform bool SHOW_LIGHTMAP;
//uniform bool SHOW_DELUXEMAP;

uniform float     u_DiffuseLighting;
#if defined(USE_LIGHT_MAPPING)
	uniform sampler2D u_LightMap;
#endif // USE_LIGHT_MAPPING
#if defined(USE_DIFFUSE)
	uniform sampler2D u_DiffuseMap;
	#if defined(USE_ALPHA_TESTING)
		uniform int       u_AlphaTest;
	#endif // USE_ALPHA_TESTING
	#if defined(USE_NORMAL_MAPPING)
		uniform vec3      u_LightColor;
		uniform sampler2D u_NormalMap;
		uniform float     u_BumpScale;
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
			uniform float u_ParallaxShadow;
		#endif // USE_PARALLAX_MAPPING
	#endif // USE_NORMAL_MAPPING
#endif // USE_DIFFUSE

varying vec3 var_Position;
varying vec4 var_Color;
varying vec3 var_Normal;
#if defined(USE_LIGHT_MAPPING)
	varying vec2 var_TexLight;
#endif // USE_LIGHT_MAPPING
#if defined(USE_DIFFUSE)
	varying vec2 var_TexDiffuse;
	#if defined(USE_NORMAL_MAPPING)
		varying mat3 var_tangentMatrix;
		varying vec3 var_LightDirW; 
		varying vec3 var_ViewDirW;               // view direction in world space
varying vec3 var_LightDirT;
varying vec3 var_ViewDirT;           // view direction in tangentspace
		#if defined(USE_PARALLAX_MAPPING)
///			varying vec3 var_ViewDirT;           // view direction in tangentspace
			varying float var_distanceToCam;     // in world units
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


#if defined(USE_LIGHT_MAPPING)
	// get the color from the lightmap
	vec4 lightmapColor = texture2D(u_LightMap, var_TexLight);
#else
	// get the color from the vertex
	vec4 lightmapColor = var_Color;
#endif // USE_LIGHT_MAPPING


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
	vec2 texDiffuse;

#if defined(USE_PARALLAX_MAPPING)
//!	float parallaxHeight; // needed for parallax self shadowing. set by the function parallax()
//!	texDiffuse = parallax(u_NormalMap, var_TexDiffuse, var_ViewDirT, u_DepthScale, var_distanceToCam, parallaxHeight);
//!	float parallaxShadow = parallaxSelfShadow(u_NormalMap, texDiffuse, -var_LightDirT, u_DepthScale, var_distanceToCam, parallaxHeight);

	vec3 parallaxResult = parallaxAndShadow(u_NormalMap, var_TexDiffuse, var_ViewDirT, -var_LightDirT, u_DepthScale, var_distanceToCam, u_ParallaxShadow);
	texDiffuse = parallaxResult.xy;
	float parallaxShadow = parallaxResult.z;
#else
	texDiffuse = var_TexDiffuse;
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
	vec3 V = var_ViewDirW;
//vec3 V = normalize(var_ViewDirT); // what we really want is all vectors in tangentspace (calculated in the vertexshader), so we don't have to transform every normal in the fp

	// light direction in world space
	vec3 L = var_LightDirW;
//vec3 L = var_LightDirT;

	// normal
	vec3 Ntex = texture2D(u_NormalMap, texDiffuse).xyz * 2.0 - 1.0;
	// the bump scale
	Ntex.y *= u_BumpScale;
	// the final normal in worldspace
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it
//vec3 N = normalize(Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	// the angle between the normal- & light-direction (needs normalized vectors to return the cosine of the angle)
	float dotNL = dot(N, L);

	// compute the diffuse light term
	diffuse.rgb *= computeDiffuseLighting(dotNL, u_DiffuseLighting);


	// compute the specular term (and reflections)
#if defined(USE_SPECULAR)
	vec3 specular = computeSpecular2(dotNL, V, N, L, u_LightColor, u_SpecularExponent, u_SpecularScale);
//vec3 specular = computeSpecular(V, N, L, u_LightColor, u_SpecularExponent, u_SpecularScale);
	specular *= texture2D(u_SpecularMap, texDiffuse).rgb;
#endif // USE_SPECULAR


// we only render reflections in the lightmap shader , when there's a reflectionmap.
#if defined(USE_REFLECTIONS)
	vec3 reflections = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale);
#if defined(USE_REFLECTIONMAP)
	reflections *= texture2D(u_ReflectionMap, texDiffuse).rgb;
#endif // USE_REFLECTIONMAP
#endif // USE_REFLECTIONS

#endif // USE_NORMAL_MAPPING




	// compute final color
	vec4 color = diffuse;
#if defined(USE_PARALLAX_MAPPING)
	color.rgb *= parallaxShadow; //pow(parallaxShadow, 2); //pow(parallaxShadow, 4);
#endif
#if defined(USE_NORMAL_MAPPING)
#if defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_SPECULAR
#if defined(USE_REFLECTIONS)
	color.rgb += reflections;
#endif // USE_REFLECTIONS
#endif // USE_NORMAL_MAPPING
	color *= lightmapColor; // lightmap or vertex color



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
