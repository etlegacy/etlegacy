/* lightMapping_fp.glsl */
#include "lib/reliefMapping"

uniform bool SHOW_LIGHTMAP;
uniform bool SHOW_DELUXEMAP;

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;
uniform sampler2D u_LightMap;
uniform sampler2D u_DeluxeMap;
uniform int       u_AlphaTest;
uniform vec3      u_ViewOrigin;
uniform float     u_DepthScale;
uniform vec4      u_PortalPlane;
uniform vec3      u_LightDir;

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexSpecular;
varying vec2 var_TexLight;

varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;

varying vec4 var_Color;

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
	if (dist < 0.0)
	{
		discard;
		return;
	}
#endif

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);
	vec4 lightmapColor  = texture2D(u_LightMap, var_TexLight);

	//fixme: must be done when there IS a deluxemap. not when we want-to-see (show) deluxemaps (and just pass -some- normalmap as a substitude :S)
	vec4 deluxemapColor = vec4(0.0, 0.0, 0.0, 1.0);
	if (SHOW_DELUXEMAP) { // should become something like:  if (USE_DELUXE_MAPPING)
		deluxemapColor = texture2D(u_DeluxeMap, var_TexLight);
	}

#if defined(USE_NORMAL_MAPPING)

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
	vec2 texSpecular = var_TexSpecular.st;


	// Create the matrix that will transform coordinates from world to tangent space.
	// invert tangent space for two sided surfaces (if that surface is backfaced).
	mat3 tangentSpaceMatrix;
#if defined(TWOSIDED)
	if (!gl_FrontFacing) {
		tangentSpaceMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif
		tangentSpaceMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	// We invert the view vector V, so we end up with a vector Vts that points away from the surface (to the camera/eye/view).. like the normal N
	vec3 Vts = normalize(tangentSpaceMatrix * -V);


#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction

	// We invert the view vector V, so we end up with a vector Vts that points away from the surface (to the camera/eye/view).. like the normal N
	//vec3 Vts = normalize(tangentSpaceMatrix * -V);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;

	texDiffuse.st  += texOffset;
	texNormal.st   += texOffset;
	texSpecular.st += texOffset;
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
//	vec3 N = normalize(tangentSpaceMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it
vec3 N = Ntex;
#if defined(r_NormalScale)
	if (r_NormalScale != 1.0) N.z *= r_NormalScale;
#endif

	// compute light direction in world space
	vec3 L = -normalize(u_LightDir);

	float dotNL = dot(N, L);


	// the specular reflections
	vec3 specular;
	if (dotNL > 0.0)
	{
		// phong
		vec3 R = reflect(L, N);
		float shininess = pow(max(0.0, dot(R, Vts)), 64.0); // r_SpecularExponent //) * r_SpecularScale;
		specular = texture2D(u_SpecularMap, texSpecular).rgb * shininess;
	}
	else
	{
		specular = vec3(0.0);
	}


	// light term
	vec3 light = lightmapColor.rgb;
//#if defined(r_HalfLambertLighting)	
//	// compute the light term (half lambert)
//	float halfLambert = dotNL * 0.5 + 0.5;
//	halfLambert *= halfLambert;
//	// compute light color from world space lightmap
//	light = lightmapColor.rgb * halfLambert;
//#endif


	// compute final color
	vec4 color = diffuse;
//vec4 color = vec4(diffuse, var_Color.a);
	color.rgb += specular;
	color.rgb *= light;

	// for smooth terrain blending else there is no blending of lightmapped
//	color.a *= var_Color.a; 


#else // DO_NOT_NORMAL_MAPPING
																  
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
#endif

	// compute the final color
	vec4 color = diffuse;
	color.rgb *= lightmapColor.rgb;
//	// for smooth terrain blending else there is no blending of lightmapped
//	color.a *= var_Color.a;

#endif // DO_NOT_NORMAL_MAPPING

	// show only the lightmap?
	if (SHOW_LIGHTMAP)
	{
		gl_FragColor = texture2D(u_LightMap, var_TexLight);
		return;
	}

	// show only the deluxemap?
	if (SHOW_DELUXEMAP)
	{
		gl_FragColor = texture2D(u_DeluxeMap, var_TexLight);
		return;
	}

	gl_FragColor = color;
}
