/* lightMapping_fp.glsl */
#include "lib/reliefMapping"
#include "lib/normalMapping"

uniform bool SHOW_LIGHTMAP;
//uniform bool SHOW_DELUXEMAP;

uniform vec3      u_ViewOrigin;
uniform int       u_AlphaTest;
uniform vec4      u_PortalPlane;
uniform vec3      u_LightDir;
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
varying vec3 var_Tangent;
varying vec3 var_Binormal;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
	if (dist < 0.0) {
		discard;
		return;
	}
#endif // USE_PORTAL_CLIPPING

	// get the color from the lightmap
	vec4 lightmapColor  = texture2D(u_LightMap, var_TexLight);

//#if defined(USE_DELUXE_MAPPING)
//	//fixme: must be done when there IS a deluxemap. not when we want-to-see (show) deluxemaps (and just pass -some- normalmap as a substitude :S)
//	vec4 deluxemapColor = vec4(0.0, 0.0, 0.0, 1.0);
//	if (SHOW_DELUXEMAP) {
//		deluxemapColor = texture2D(u_DeluxeMap, var_TexLight);
//	}
//#endif // USE_DELUXE_MAPPING



#if defined(USE_NORMAL_MAPPING)

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st;
#endif // USE_REFLECTIONS || USE_SPECULAR

	// compute view direction in world space
	vec3 V = normalize(var_Position - u_ViewOrigin);

	// Create the matrix that will transform coordinates to worldspace.
	mat3 tangentSpaceMatrix = tangetToWorldMatrix(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);


#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction

	// We invert the view vector V, so we end up with a vector Vts that points away from the surface (to the camera/eye/view).. like the normal N
	vec3 Vts = normalize(tangentSpaceMatrix * V);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;

	texDiffuse.st  += texOffset;
	texNormal.st   += texOffset;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	texSpecular.st += texOffset;
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


	// compute normal (from tangentspace, to worldspace)
	vec3 N = computeNormal(texture2D(u_NormalMap, texNormal).xyz, tangentSpaceMatrix);

	// compute light direction in world space. invert the direction so the vector points away from the surface.
	vec3 L = -normalize(u_LightDir);

	// the angle between the normal- & light-directions (needs normalized vectors to return the cosine of the angle)
	float dotNL = dot(N, L);


#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	// compute the specular term (and reflections)
	vec3 reflections = vec3(0.0);
	vec3 specular = vec3(0.0);
#if defined(USE_REFLECTIONS)
	// reflections
	reflections = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation);
#endif // end USE_REFLECTIONS
#if defined(USE_SPECULAR)
	// the specular highlights
	specular = computeSpecular(V, N, L, 64.0); //r_SpecularExponent
#endif // USE_SPECULAR
	specular += reflections;
	specular *= texture2D(u_SpecularMap, texSpecular).rgb; // shininess factor
#endif // USE_REFLECTIONS || USE_SPECULAR

	// compute the diffuse light term
	// https://en.wikipedia.org/wiki/Lambert%27s_cosine_law
	diffuse.rgb *= computeDiffuseLighting(N, L);

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
//	if (SHOW_DELUXEMAP) {
//		gl_FragColor = texture2D(u_DeluxeMap, var_TexLight);
//		return;
//	}
//#endif // USE_DELUXE_MAPPING

	gl_FragColor = color;
}
