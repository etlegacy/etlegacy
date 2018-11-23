/* vertexLighting_DBS_world_fp.glsl */
#include "lib/reliefMapping"
#include "lib/normalMapping"

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
#if defined(USE_REFLECTIONS)
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif // USE_REFLECTIONS
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec4 var_LightColor;
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
	if (dist < 0.0)
	{
		discard;
		return;
	}
#endif



#if defined(USE_NORMAL_MAPPING)

	// texture coordinates
	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st;
#endif // USE_REFLECTIONS || USE_SPECULAR


	// compute view direction in world space
	vec3 V = normalize(var_Position - u_ViewOrigin);

	// construct the matrix that transforms from tangentspace to worldspace
	mat3 tangentSpaceMatrix = tangetToWorldMatrix(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);


#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction

	// compute view direction (from tangentspace, to worldspace)
	vec3 Vts = normalize(tangentSpaceMatrix * V);

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



	// compute normal (to worldspace)
	vec3 N = computeNormal(texture2D(u_NormalMap, texNormal).xyz, tangentSpaceMatrix);

	// compute light direction (this is also in world space)
	vec3 L = -normalize(u_LightDir); // reverse the direction

	// the cosine of the angle N L (needs unit-vectors)
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
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	color.rgb += specular;
#endif // USE_REFLECTIONS || USE_SPECULAR
	color *= var_LightColor;
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

	vec4 color = diffuse * var_LightColor;
	gl_FragColor = color;

#endif // USE_NORMAL_MAPPING
}
