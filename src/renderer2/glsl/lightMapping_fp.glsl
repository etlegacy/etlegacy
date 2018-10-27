/* lightMapping_fp.glsl */
#include "lib/reliefMapping"

uniform bool SHOW_LIGHTMAP;
uniform bool SHOW_DELUXEMAP;

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
#if defined(USE_DELUXE_MAPPING)
uniform sampler2D u_DeluxeMap;
#endif // USE_DELUXE_MAPPING
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


// We define a compiler directive if we want the faster transform code.
// If you comment the next line, a matrix is created and used.
#define transformFast


void main()
{
#if defined(USE_PORTAL_CLIPPING)
	float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
	if (dist < 0.0) {
		discard;
		return;
	}
#endif // USE_PORTAL_CLIPPING


	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);

	// get the color from the lightmap
	vec4 lightmapColor  = texture2D(u_LightMap, var_TexLight);

#if defined(USE_DELUXE_MAPPING)
	//fixme: must be done when there IS a deluxemap. not when we want-to-see (show) deluxemaps (and just pass -some- normalmap as a substitude :S)
	vec4 deluxemapColor = vec4(0.0, 0.0, 0.0, 1.0);
	if (SHOW_DELUXEMAP) { // should become something like:  if (USE_DELUXE_MAPPING)
		deluxemapColor = texture2D(u_DeluxeMap, var_TexLight);
	}
#endif // USE_DELUXE_MAPPING



#if defined(USE_NORMAL_MAPPING)

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st;
#endif // USE_REFLECTIONS || USE_SPECULAR


	// Create the matrix that will transform coordinates from world to tangent space.
	// invert tangent space for two sided surfaces (if that surface is backfaced).
#if defined(transformFast) // do not use a matrix, but do the necesarry calculations. it should be just a bit faster.
	// EEH.. no prep needed. 
#else // transformFast
	// construct object-space-to-tangent-space 3x3 matrix
	mat3 tangentSpaceMatrix;
#if defined(TWOSIDED)
	if (!gl_FrontFacing) {
		tangentSpaceMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	} else
#endif // TWOSIDED
		tangentSpaceMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
#endif // transformFast


#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction

	// We invert the view vector V, so we end up with a vector Vts that points away from the surface (to the camera/eye/view).. like the normal N
#if defined(transformFast) // do not use a matrix, but do the necesarry calculations. it should be just a bit faster.
	vec3 Vts;
#if defined(TWOSIDED)
	if (!gl_FrontFacing) {
		Vts.x = dot(-V, -var_Tangent);
		Vts.y = dot(-V, -var_Binormal);
		Vts.z = dot(-V, -var_Normal);
	} else
#endif// TWOSIDED
	{
		Vts.x = dot(-V, var_Tangent);
		Vts.y = dot(-V, var_Binormal);
		Vts.z = dot(-V, var_Normal);
	}
#else // transformFast
	vec3 Vts = -normalize(tangentSpaceMatrix * V);
#endif // transformFast


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


	// compute normal in tangent space from normalmap and multiply with tangenttoworldmatrix so it gets to world
	// each colour component is between 0 and 1, and each vector component is between -1 and 1,
	//so this simple mapping goes from the texel to the normal
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0;


	vec3 N;
#if defined(transformFast) // do not use a matrix, but do the necesarry calculations. it should be just a bit faster.
	// transform Ntex to tangent space
#if defined(TWOSIDED)
	if (!gl_FrontFacing) {
		N.x = dot(Ntex, -var_Tangent);
		N.y = dot(Ntex, -var_Binormal);
		N.z = dot(Ntex, -var_Normal);
	} else
#endif // TWOSIDED
	{
		N.x = dot(Ntex, var_Tangent);
		N.y = dot(Ntex, var_Binormal);
		N.z = dot(Ntex, var_Normal);
	}
	// we must normalize N because otherwise we see white artifacts visible in game
	N = normalize(N);
#else // transformFast
	N = normalize(tangentSpaceMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it
#endif // transformFast


#if defined(r_NormalScale)
	if (r_NormalScale != 1.0) N.z *= r_NormalScale;
#endif




#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	// compute light direction in world space
	vec3 L = -normalize(u_LightDir);
	// the angle between the normal- & light-directions (needs normalized vectors to return the cosine of the angle)
	float dotNL = dot(N, L);

	// compute the specular term
	// we start with a specular value of 0
	// Depending on the usage of reflections and/or specular highlights, we later only add to this specular.
	vec3 specular = vec3(0.0);

#if defined(USE_REFLECTIONS)
	// this is the reflection vector used on the cubeMaps
	// it's also used for rimLighting.
	// Because we read pixels from a cubemap, the reflect() needs two vectors in world-space.
	// Also, the view-vector must be in the direction: from eye/camera to object. (don't use Vts).
	vec3 Renv = reflect(V, Ntex);
	// This is the cubeProbes way of rendering reflections.
	vec4 envColor0 = textureCube(u_EnvironmentMap0, Renv).rgba; // old code used .rgba?  check<--
	vec4 envColor1 = textureCube(u_EnvironmentMap1, Renv).rgba;
	specular = mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;
//	specular *= pow(max(0.0, dot(R, Vts)), r_SpecularExponent) * r_SpecularScale; // shininess is dependent on R & V
#endif // end USE_REFLECTIONS

#if defined(USE_SPECULAR)
	// the specular highlights
	if (dotNL > 0.0) {
		// phong
		vec3 R = reflect(L, N);
		float reflectance = pow(max(0.0, dot(R, V)), 64.0); // r_SpecularExponent //) * r_SpecularScale;
		specular = min(specular + reflectance, 1.0); // specular highlights only add to the color
	}
#endif // USE_SPECULAR

	specular *= texture2D(u_SpecularMap, texSpecular).rgb; // shininess factor
#endif // USE_REFLECTIONS || USE_SPECULAR


	// light term
	vec3 light = lightmapColor.rgb;
// half-lambert on terrain??
//#if defined(r_HalfLambertLighting)	
//	// compute the light term (half lambert)
//	float halfLambert = dotNL * 0.5 + 0.5;
//	halfLambert *= halfLambert;
//	// compute light color from world space lightmap
//	light = lightmapColor.rgb * halfLambert;
//#endif


	// compute final color
	vec4 color = diffuse; //vec4 color = vec4(diffuse, var_Color.a);
#if defined(USE_NORMAL_MAPPING)
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING
	color.rgb *= light;

	// for smooth terrain blending else there is no blending of lightmapped
//	color.a *= var_Color.a; 





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
	color.rgb *= lightmapColor.rgb;
//	// for smooth terrain blending else there is no blending of lightmapped
//	color.a *= var_Color.a;

#endif // USE_NORMAL_MAPPING


	// show only the lightmap?
	if (SHOW_LIGHTMAP) {
		gl_FragColor = texture2D(u_LightMap, var_TexLight);
		return;
	}

#if defined(USE_DELUXE_MAPPING)
	// show only the deluxemap?
	if (SHOW_DELUXEMAP) {
		gl_FragColor = texture2D(u_DeluxeMap, var_TexLight);
		return;
	}
#endif // USE_DELUXE_MAPPING

	gl_FragColor = color;
}
