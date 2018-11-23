/* vertexLighting_DBS_entity_fp.glsl */
#include "lib/reliefMapping"
#include "lib/normalMapping"

uniform sampler2D u_DiffuseMap;
uniform vec4  u_PortalPlane;
uniform int   u_AlphaTest;
uniform vec3  u_ViewOrigin;
uniform vec3  u_LightDir;
uniform vec3  u_LightColor;
uniform float u_LightWrapAround;
uniform vec3  u_AmbientColor;
uniform float u_DepthScale;
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
varying vec4 var_LightColor;
varying vec3 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying vec2 var_TexNormal;
varying vec3 var_Tangent;
varying vec3 var_Binormal;
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
varying vec2 var_TexSpecular;
#endif // USE_REFLECTIONS || USE_SPECULAR
#endif // USE_NORMAL_MAPPING


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



#if defined(USE_NORMAL_MAPPING)

	// texture coordinates
	vec2 texNormal   = var_TexNormal.st; // the current texture coordinates st for the normalmap
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	vec2 texSpecular = var_TexSpecular.st; // the current texture coordinates st for the specularmap
#endif // USE_REFLECTIONS || USE_SPECULAR

	// compute view direction in world space
	//vec3 V = normalize(u_ViewOrigin - var_Position);
	vec3 V = normalize(var_Position - u_ViewOrigin);

	// compute light direction in world space
	// invert the direction, so we end up with a vector that is pointing away from the surface.
	// We must also normalize the vector, because the reflect() function needs it.
	vec3 L = -normalize(u_LightDir);

	// Create the matrix that will transform coordinates to worldspace.
	mat3 tangentSpaceMatrix = tangetToWorldMatrix(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);


#if defined(USE_PARALLAX_MAPPING)
//!!!DEBUG!!! check this parallax stuff <-- todo
	// ray intersect in view direction

	// compute view direction in tangent space
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
#endif // end USE_PARALLAX_MAPPING



	// compute normal (from normalmap texture)
	// N is the normal (at every .st of the normalmap texture)
	// We convert normalmap texture values from range[0.0,1.0] to range[-1.0,1.0]
	// and then tranform N into worldspace.
	// compute normal (from tangentspace, to worldspace)
	vec3 N = computeNormal(texture2D(u_NormalMap, texNormal).xyz, tangentSpaceMatrix);

	// the cosine of the angle N L
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


	// add Rim Lighting to highlight the edges
#if defined(r_rimLighting)
	vec3 R = reflect(-V, N);
	float rim = 1.0 - clamp(R, 0, 1);
	vec3  emission = r_rimColor.rgb * pow(rim, r_rimExponent);
#endif // end r_rimLighting



    // compute final color
	vec4 color = vec4(diffuse.rgb, var_LightColor.a);
#if defined(USE_REFLECTIONS) || defined(USE_SPECULAR)
	color.rgb += specular;
#endif // end USE_REFLECTIONS || USE_SPECULAR
#if defined(r_rimLighting)
	color.rgb += emission;
#endif // end r_rimLighting

#else // USE_NORMAL_MAPPING

    vec4 color = vec4(diffuse.rgb, 1.0);

#endif // end USE_NORMAL_MAPPING

    //color.rgb *= (var_LightColor.rgb + u_AmbientColor); // i really think mapper's ambient values are too high..
    color.rgb *= (var_LightColor.rgb);
	gl_FragColor = color;
}
