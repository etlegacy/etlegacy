/* vertexLighting_DBS_entity_fp.glsl */
#include "lib/reliefMapping"

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;

uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;

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
#endif

	// compute light direction in world space
	vec3 L = u_LightDir;

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);

	vec2 texDiffuse = var_TexDiffuse.st;

#if defined(USE_NORMAL_MAPPING)
	// invert tangent space for two sided surfaces
	mat3 tangentToWorldMatrix;

#if defined(TWOSIDED)
	if (gl_FrontFacing)
	{
		tangentToWorldMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif
	{
		tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	}

	vec2 texNormal   = var_TexNormal.st;
	vec2 texSpecular = var_TexSpecular.st;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

	mat3 worldToTangentMatrix = transpose(tangentToWorldMatrix);

	// compute view direction in tangent space
	vec3 Vts = worldToTangentMatrix * (u_ViewOrigin - var_Position.xyz);
	Vts = normalize(Vts);

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
#endif // end USE_PARALLAX_MAPPING

	// compute normal in world space from normalmap
	vec3 N = normalize(tangentToWorldMatrix * (2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5)));

	// compute half angle in world space
	vec3 H = normalize(L + V);

	// compute specular reflection
	vec3 R =  reflect(-L, N); 

	// compute the specular term
#if defined(USE_REFLECTIVE_SPECULAR)

	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb;

	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-V, N)).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-V, N)).rgba;

	specular *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;

	// Blinn-Phong
	float NH = clamp(dot(N, H), 0, 1);
	specular *= u_LightColor * pow(NH, r_SpecularExponent2) * r_SpecularScale;
	// FIXME?
	//specular *= u_LightColor * pow(max(dot(V, R), 0.0), r_SpecularExponent) * r_SpecularScale;

#if 0
	gl_FragColor = vec4(specular, 1.0);
	// gl_FragColor = vec4(u_EnvironmentInterpolation, u_EnvironmentInterpolation, u_EnvironmentInterpolation, 1.0);
	// gl_FragColor = envColor0;
	return;
#endif

#else

	// simple Blinn-Phong
	float NH       = clamp(dot(N, H), 0, 1);
	//vec3  specular = texture2D(u_SpecularMap, texSpecular).rgb * u_LightColor * pow(NH, r_SpecularExponent) * r_SpecularScale;
	vec3  specular = texture2D(u_SpecularMap, texSpecular).rgb * u_LightColor * pow(max(dot(V, R), 0.0), r_SpecularExponent) * r_SpecularScale;

#endif // end USE_REFLECTIVE_SPECULAR


#else // else USE_NORMAL_MAPPING

	vec3 N;

#if defined(TWOSIDED)
	if (gl_FrontFacing)
	{
		N = -normalize(var_Normal);
		// gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		// return;
	}
	else
#endif
	{
		N = normalize(var_Normal);
	}

#endif // end USE_NORMAL_MAPPING


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


// add Rim Lighting to highlight the edges
#if defined(r_rimLighting)
	float rim      = 1.0 - clamp(dot(N, V), 0, 1);
	vec3  emission = r_rimColor.rgb * pow(rim, r_rimExponent);

	// gl_FragColor = vec4(emission, 1.0);
	// return;
#endif

	// compute the light term
#if defined(r_HalfLambertLighting)
	// http://developer.valvesoftware.com/wiki/Half_Lambert
	float NL = dot(N, L) * 0.5 + 0.5;
	NL *= NL;
#elif defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif

 	vec3 light = (u_AmbientColor + u_LightColor) * NL;
 	// FIXME? specular? see https://learnopengl.com/Lighting/Basic-Lighting
 	//vec3 light = (u_AmbientColor + u_LightColor + specular) * NL;
	//clamp(light, 0.0, 1.0);

    // compute final color
    vec4 color = diffuse;
    color.rgb *= light;
#if defined(USE_NORMAL_MAPPING)
	color.rgb += specular; // FIXME?
#endif
#if defined(r_rimLighting)
	color.rgb += emission;
#endif

	gl_FragColor = color;

	//gl_FragColor = vec4(vec3(NL, NL, NL), diffuse.a);

#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif
}
