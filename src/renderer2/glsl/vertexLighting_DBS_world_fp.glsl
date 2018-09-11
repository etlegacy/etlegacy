/* vertexLighting_DBS_world_fp.glsl */
#include "lib/reliefMapping"

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;
uniform int       u_AlphaTest;
uniform vec3      u_ViewOrigin;
uniform float     u_DepthScale;
uniform vec4      u_PortalPlane;
uniform float     u_LightWrapAround;

varying vec3 var_Position;
varying vec4 var_TexDiffuseNormal;
varying vec2 var_TexSpecular;
varying vec4 var_LightColor;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
varying vec3 var_Normal;
//varying vec3 var_LightDirection;

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

#if defined(USE_NORMAL_MAPPING)

	// construct object-space-to-tangent-space 3x3 matrix
	mat3 objectToTangentMatrix;

#if defined(TWOSIDED)
	if (gl_FrontFacing)
	{
		objectToTangentMatrix = mat3(-var_Tangent.x, -var_Binormal.x, -var_Normal.x,
		                             -var_Tangent.y, -var_Binormal.y, -var_Normal.y,
		                             -var_Tangent.z, -var_Binormal.z, -var_Normal.z);
	}
	else
#endif
	{
		objectToTangentMatrix = mat3(var_Tangent.x, var_Binormal.x, var_Normal.x,
		                             var_Tangent.y, var_Binormal.y, var_Normal.y,
		                             var_Tangent.z, var_Binormal.z, var_Normal.z);
	}

	// compute view direction in tangent space
	vec3 Vts = normalize(objectToTangentMatrix * V);

	vec2 texDiffuse  = var_TexDiffuseNormal.st;
	vec2 texNormal   = var_TexDiffuseNormal.pq;
	vec2 texSpecular = var_TexSpecular.st;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

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
#else
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;
#endif

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

	// compute normal in tangent space from normalmap
	vec3 N = 2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5);

#if defined(r_NormalScale)
	N.z *= r_NormalScale;
#endif

	N = normalize(N);

	// compute light direction in tangent space
	// FIXME/ADD?: See ATTR_INDEX_LIGHTDIRECTION
	//vec3 L = normalize(objectToTangentMatrix * var_LightDirection);
	vec3 L = N; // fake bump mapping
		
	// compute half angle in tangent space
	vec3 H = normalize(L + Vts);

	vec3 R = reflect(-L, N);

	// compute the light term
#if defined(r_HalfLambertLighting)
	// http://developer.valvesoftware.com/wiki/Half_Lambert
	float NL = dot(N, L) * 0.5 + 0.5;
#elif defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif
	vec3 light = var_LightColor.rgb * NL;

	// compute the specular term
	//vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb * var_LightColor.rgb * pow(clamp(dot(N, H), 0.0, 1.0), r_SpecularExponent) * r_SpecularScale;
	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb * var_LightColor.rgb * pow(max(dot(Vts, R), 0.0), r_SpecularExponent) * r_SpecularScale;
	
	// compute final color
	vec4 color = vec4(diffuse.rgb, var_LightColor.a);
	color.rgb *= light;
	color.rgb += specular;

	gl_FragColor = color;

#else // no normal mapping

	vec3 N;

#if defined(TWOSIDED)
	if (gl_FrontFacing)
	{
		N = -normalize(var_Normal);
	}
	else
#endif
	{
		N = normalize(var_Normal);
	}

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

	//FIXME/ADD?: See ATTR_INDEX_LIGHTDIRECTION
	//vec3 L = normalize(var_LightDirection);

	// compute the light term
	//#if defined(r_WrapAroundLighting)
	//	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
	//#else
	//	float NL = clamp(dot(N, L), 0.0, 1.0);
	//#endif

	//vec3 light = var_LightColor.rgb * NL;
	//vec4 color = vec4(diffuse.rgb * light, var_LightColor.a);

	vec4 color = vec4(diffuse.rgb * var_LightColor.rgb, var_LightColor.a);

//defined(r_ShowTerrainBlends)
#if 0
	color = vec4(vec3(var_LightColor.a), 1.0);
#endif

	gl_FragColor = color;

#endif // USE_NORMAL_MAPPING
}
