/* vertexLighting_DBS_world_fp.glsl */
#include "lib/reliefMapping"

uniform sampler2D u_DiffuseMap;
uniform int       u_AlphaTest;
uniform vec3      u_ViewOrigin;
uniform float     u_DepthScale;
uniform vec4      u_PortalPlane;
uniform float     u_LightWrapAround;
uniform vec3      u_LightDir;
uniform vec3      u_LightColor;
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D u_NormalMap;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
uniform sampler2D u_SpecularMap;
#endif // USE_REFLECTIONS || USE_SPECULAR
#if defined(USE_REFLECTIONS)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif // USE_REFLECTIONS
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec4 var_LightColor;
//varying vec3 var_LightDirection;
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
		if (dist < 0.0)
		{
			discard;
			return;
		}
#endif



#if defined(USE_NORMAL_MAPPING)

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st;
#endif // USE_REFLECTIONS || USE_SPECULAR



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

	// compute view direction in tangent space
	// we invert V so we end up with a vector that is pointing away from the surface, to the viewer.
	// transform V to tangent space
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
#if 0
	vec2 texOffset = vec2(0.0);
	for (int i = 0; i < 4; i++)
	{
		vec4  Normal = texture2D(u_NormalMap, texNormal.st + texOffset);
		float height = Normal.a * 0.2 - 0.0125;
		texOffset += height * Normal.z * S;
	}
#else // 0
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);
	// compute texcoords offset
	vec2 texOffset = S * depth;
#endif // 0

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
#endif // USE_ALPHA_TESTING



	// compute normal in tangent space from normalmap
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

	// compute light direction in tangent space
	vec3 L = -normalize(u_LightDir);

	float dotNL = dot(N, L);


#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
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
	// the specular highlights, only visible if you look into the light
	if (dotNL > 0.0) {
		vec3 R = reflect(L, N);
		float reflectance = pow(max(0.0, dot(R, V)), 64.0); // ,r_SpecularExponent) // * r_SpecularScale;
		specular = min(specular + reflectance, 1.0); // specular highlights only add to the color
	}
#endif // USE_SPECULAR

	specular *= texture2D(u_SpecularMap, texSpecular).rgb; // shininess factor

#if 0
//!!!DEBUG!!!  contrast test

//specular = min(0.0, dot(reflect(L, N), V)) * texture2D(u_SpecularMap, texSpecular).rgb; //dotNL * 
//specular = min(0.0, dot(reflect(L, N), V)) * texture2D(u_SpecularMap, texSpecular).rgb; //darken (only in shadow side)
//specular = dot(reflect(L, N), V) * texture2D(u_SpecularMap, texSpecular).rgb; // darker in shadow, lighter in sun (but too light. bumps are gone, img flat)
specular = -abs(dot(reflect(L, N), V)) * texture2D(u_SpecularMap, texSpecular).rgb; //darken (on shadow side AND on light side.. creates more contrast. bumps still visible)
#endif

#endif // USE_REFLECTIONS || USE_SPECULAR


	// compute the light term
#if defined(r_HalfLambertLighting)
	// http://developer.valvesoftware.com/wiki/Half_Lambert
	//
	float NL = dotNL * 0.5 + 0.5;  // 1/2

//		// r_HalfLambertLighting: as a percentage of howmuch the half-lambert effect should be applied.
//		// The higher the value, the more dark it will become in the shadow areas of the map.
//		//   0 : effect not visible / unused / disabled.
//		// > 0 : enabled
//		// 0 < HL < 100 : HL percentage. where 100% produces the most contrast light/dark, and 0% no effect at all.
//		float factorHL =  0.01 * float(r_HalfLambertLighting);
//		NL *= factorHL;
//		NL += (1.0 - factorHL);

	// to prevent pitch black surfaces (in shadow), we scale and shift the hlafLambert value
	NL *= 0.125; // scale 1/16
	NL += 0.875; // 1.0 - 0.125
//	NL *= 0.5; // scale & shift 1/4
//	NL += 0.5;
//	NL *= 0.5; // scale & shift 1/8
//	NL += 0.5;
//	NL *= 0.5; // scale & shift 1/16
//	NL += 0.5;

	NL *= NL; // square it, which makes NL always positive. btw..
#elif defined(r_WrapAroundLighting)
	float NL = clamp(dotNL + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else // r_WrapAroundLighting
	float NL = clamp(dotNL, 0.0, 1.0);
#endif // r_WrapAroundLighting, r_HalfLambertLighting

	vec3 light = var_LightColor.rgb * NL;



	// compute final color
	vec4 color = vec4(diffuse.rgb, var_LightColor.a);
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_REFLECTIONS || USE_SPECULAR
	color.rgb *= light;
	gl_FragColor = color;




#else // no normal mapping


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

	vec4 color = vec4(diffuse.rgb * var_LightColor.rgb, var_LightColor.a);

//defined(r_ShowTerrainBlends)
#if 0
	color = vec4(vec3(var_LightColor.a), 1.0);
#endif

	gl_FragColor = color;

#endif // USE_NORMAL_MAPPING
}
