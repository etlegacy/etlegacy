/* vertexLighting_DBS_entity_fp.glsl */
#include "lib/reliefMapping"

uniform sampler2D u_DiffuseMap;
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;
#if defined(USE_REFLECTIVE_SPECULAR)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif
#endif
uniform int   u_AlphaTest;
uniform vec3  u_ViewOrigin;
uniform vec3  u_AmbientColor;
uniform vec3  u_LightDir;
uniform vec3  u_LightColor;
//uniform float u_SpecularExponent;
uniform float u_DepthScale;
uniform vec4  u_PortalPlane;
uniform float u_LightWrapAround;

varying vec3 var_Position;
varying vec2 var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying vec2 var_TexNormal;
varying vec2 var_TexSpecular;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
#endif
varying vec3 var_Normal;
varying vec4 var_LightColor;

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	{
		float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if (dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif // end USE_PORTAL_CLIPPING



	// compute the diffuse term
	vec2 texDiffuse = var_TexDiffuse.st; // diffuse texture coordinates st
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse); // the color at coords(st) of the diffuse texture

	// alphaFunc
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
#endif // end USE_ALPHA_TESTING


	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);

	// compute light direction in world space
	// invert the direction, so we end up with a vector that is pointing away from the surface.
	// We must also normalize the vector, because the reflect() function needs it.
	vec3 L = -normalize(u_LightDir);

	// we start with a specular value of 0
	// Depending on the usage of reflections and/or specular highlights, we later only add to this specular.
	vec3 specular = vec3(0.0);



#if defined(USE_NORMAL_MAPPING)

	// texture coordinates
	vec2 texNormal   = var_TexNormal.st; // the current texture coordinates st for the normalmap
	vec2 texSpecular = var_TexSpecular.st; // the current texture coordinates st for the specularmap

	// invert tangent space for two sided surfaces which are looked at on the back side of that surface (surfaces which are backfacing).
	// Create the usual tangent space matrix in case we're dealing with a front-facing surface, or when the surface is not two-sided (the usual case).
	mat3 tangentSpaceMatrix;
#if defined(TWOSIDED)
	if (!gl_FrontFacing)
	{
		tangentSpaceMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif // end TWOSIDED
		tangentSpaceMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);


#if defined(USE_PARALLAX_MAPPING)
//!!!DEBUG!!! check this parallax stuff <-- todo
	// ray intersect in view direction

	// compute view direction in tangent space
	// We invert the view vector V, so we end up with a vector Vts that points away from the surface (to the camera/eye/view).. like the normal N
	vec3 Vts = normalize(tangentSpaceMatrix * -V);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;

	texDiffuse.st  += texOffset;
	texNormal.st   += texOffset;
	texSpecular.st += texOffset;
#endif // end USE_PARALLAX_MAPPING

	// compute normal (from normalmap texture) in tangent space
	// N is the normal (at every .st of the normalmap texture)
	// We convert normalmap texture values from range[0.0,1.0] to range[-1.0,1.0]
	// and then tranform N into tangent space.
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0;
	vec3 N = normalize(tangentSpaceMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	float dotNL = dot(N, L);
	vec3 R = reflect(L, N); // reflect() needs two unit-vectors, so we use reflect() first before possibly scaling the normal later on..

#if defined(r_rimLighting) || defined(USE_REFLECTIVE_SPECULAR)
	// this is the reflection vector used on the cubeMaps
	// it's also used for rimLighting.
	// Because we read pixels from a cubemap, the reflect() needs two vectors in world-space.
	// Also, the view-vector must be in the direction: from eye/camera to object. (don't use Vts).
	vec3 Renv = reflect(V, Ntex);
#endif // r_rimLighting or USE_REFLECTIVE_SPECULAR

#if defined(r_NormalScale)
	if (r_NormalScale != 1.0) N.z *= r_NormalScale;
#endif // end r_NormalScale


#if defined(USE_REFLECTIVE_SPECULAR)
	// This is the cubeProbes way of rendering reflections.
	vec3 envColor0 = textureCube(u_EnvironmentMap0, Renv).rgb; // old code used .rgba?  check<--
	vec3 envColor1 = textureCube(u_EnvironmentMap1, Renv).rgb;
	specular = mix(envColor0, envColor1, u_EnvironmentInterpolation);
//	specular *= pow(max(0.0, dot(R, Vts)), r_SpecularExponent) * r_SpecularScale; // shininess is dependent on R & V
//	specular *= texture2D(u_SpecularMap, texSpecular).rgb;
#endif // end USE_REFLECTIVE_SPECULAR

	// the specular highlights
	// These are only visible, at certain angles between normal, light-direction & view-direction.
	if (dotNL > 0.0) {
		// phong
		// we use world coords for reflections
		float reflectance = pow(max(0.0, dot(R, V)), 64.0); // r_SpecularExponent //) * r_SpecularScale;
//		specular += texture2D(u_SpecularMap, texSpecular).rgb * reflectance;
		specular = min(specular + reflectance, 1.0);
	}

	specular *= texture2D(u_SpecularMap, texSpecular).rgb;

#endif // end USE_NORMAL_MAPPING





	// compute the light term
 	// see https://learnopengl.com/Lighting/Basic-Lighting
	vec3 light = var_LightColor.rgb; //var_LightColor.rgb is the interpolated color (of this fragment/pixel)
#if defined(USE_NORMAL_MAPPING)
#if defined(r_HalfLambertLighting)
	// http://developer.valvesoftware.com/wiki/Half_Lambert
	float NL = dotNL * 0.5 + 0.5;
	NL *= NL;
#elif defined(r_WrapAroundLighting)
	float NL = clamp(dotNL + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else // else r_WrapAroundLighting
	float NL = clamp(dotNL, 0.0, 1.0);
#endif // end r_WrapAroundLighting. end r_HalfLambertLighting.
 	light *= NL;
	// add Rim Lighting to highlight the edges
#if defined(r_rimLighting)
	float rim = 1.0 - clamp(Renv, 0, 1);
	vec3  emission = r_rimColor.rgb * pow(rim, r_rimExponent);
#endif // end r_rimLighting
#endif // USE_NORMAL_MAPPING
	// add ambient color
	light += u_AmbientColor;



    // compute final color
    vec4 color = diffuse;
#if defined(USE_NORMAL_MAPPING)
	color.rgb += specular;
#if defined(r_rimLighting)
	color.rgb += emission;
#endif // end r_rimLighting
#endif // end USE_NORMAL_MAPPING
    color.rgb *= light;

	gl_FragColor = color;



#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else // else USE_NORMAL_MAPPING
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif // end USE_NORMAL_MAPPING. end USE_PARALLAX_MAPPING.
#endif // 0

}
