/* vertexLighting_DBS_entity_fp.glsl */
#if defined(USE_NORMAL_MAPPING)
#include "lib/normalMapping"
#if defined(USE_PARALLAX_MAPPING)
#include "lib/reliefMapping"
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING

uniform sampler2D u_DiffuseMap;
uniform int   u_AlphaTest;
uniform vec3  u_LightColor;
uniform float u_LightWrapAround;
uniform vec3  u_AmbientColor;
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
varying vec2 var_TexDiffuse;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec2 var_TexNormal;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR


#if defined(USE_PARALLAX_MAPPING)
varying vec2 var_S; // size and start position of search in texture space
#endif // USE_PARALLAX_MAPPING
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING
varying vec3 var_LightDirection;
varying vec3 var_ViewOrigin; // position - vieworigin

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	if (var_BackSide < 0.0)
	{
		discard;
		return;
	}
#endif // end USE_PORTAL_CLIPPING

#if defined(USE_NORMAL_MAPPING)
     vec3 N;
	 // light direction
	vec3 L = var_LightDirection;
	vec3 V = var_ViewOrigin;
#else
 vec3 N = var_Normal;
	// view direction
	vec3 V = var_ViewOrigin;
	// light direction
	vec3 L = -var_LightDirection;
	
// the cosine of the angle N L
	float dotNL = dot(N, L);
#endif
  
	

	
	

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



#if defined(USE_NORMAL_MAPPING)

	// texture coordinates
	vec2 texNormal   = var_TexNormal.st; // the current texture coordinates st for the normalmap
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st; // the current texture coordinates st for the specularmap
#endif // USE_REFLECTIONS || USE_SPECULAR


#if defined(USE_PARALLAX_MAPPING)
	// ray intersect in view direction
	float depth = RayIntersectDisplaceMap(texNormal, var_S, u_NormalMap);
	// compute texcoords offset
	vec2 texOffset = var_S * depth;
	texDiffuse  += texOffset;
	texNormal   += texOffset;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	texSpecular += texOffset;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // end USE_PARALLAX_MAPPING


	

	// normal
	vec3 Ntex = texture2D(u_NormalMap, texNormal).xyz * 2.0 - 1.0;
	// transform normal from tangentspace to worldspace
	 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it

	float dotNL = dot(N, L);


	// compute the specular term (and reflections)
	//! https://en.wikipedia.org/wiki/Specular_highlight
#if defined(USE_SPECULAR) && !defined(USE_REFLECTIONS)
	vec4 map = texture2D(u_SpecularMap, texSpecular);
	vec3 specular = computeSpecular3(dotNL, V, N, L, u_LightColor, 32.0) * map.rgb;
#elif defined(USE_SPECULAR) && defined(USE_REFLECTIONS)
	vec4 map = texture2D(u_SpecularMap, texSpecular);
	vec3 specular = (computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation) * 0.07)
					+ (computeSpecular3(dotNL, V, N, L, u_LightColor, 32.0) * map.rgb * r_SpecularExponent2); // woot
#elif !defined(USE_SPECULAR) && defined(USE_REFLECTIONS)
	vec4 map = texture2D(u_SpecularMap, texSpecular);
	vec3 specular = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation) * map.rgb * 0.07);
#endif


#if defined(r_diffuseLighting)
	// compute the diffuse light term
	diffuse.rgb *= computeDiffuseLighting2(dotNL);
#endif // r_diffuseLighting


	// add Rim Lighting to highlight the edges
#if defined(r_rimLighting)
	vec3 R = reflect(-V, N);
	float rim = 1.0 - clamp(R, 0, 1);
	vec3  emission = r_rimColor.rgb * pow(rim, r_rimExponent);
#endif // end r_rimLighting



    // compute final color
	vec4 color = vec4(diffuse.rgb, 1.0);
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	color.rgb += specular;
#endif // end USE_REFLECTIONS || USE_SPECULAR
#if defined(r_rimLighting)
	color.rgb += emission;
#endif // end r_rimLighting

#else // USE_NORMAL_MAPPING


    vec4 color = vec4(diffuse.rgb, 1.0);
	
#endif // end USE_NORMAL_MAPPING

     //keep this from getting zero, black looks ugly in ET
	 //normalmapping can be closer as it also has specular
#if defined(USE_NORMAL_MAPPING)
   
	 //we need this for shadows on ents, see flag and stuff
    color.rgb *=(u_LightColor  + u_AmbientColor); 
#else
    
   
    color.rgb *= (u_LightColor + u_AmbientColor);
#endif

    
    //
	gl_FragColor = color;
}
