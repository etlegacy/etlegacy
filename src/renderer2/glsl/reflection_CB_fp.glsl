/* reflection_CB_fp.glsl */
#include "lib/normalMapping"

#if 1
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#else // 1
uniform samplerCube u_ColorMap;
#endif // 1
uniform mat4        u_ModelMatrix;
#if defined(USE_NORMAL_MAPPING)
uniform sampler2D   u_NormalMap;
#endif // USE_NORMAL_MAPPING

varying vec3 var_Position;
varying vec3 var_ViewOrigin; // position - vieworigin
varying vec4 var_Normal;
#if defined(USE_NORMAL_MAPPING)
varying mat3 var_tangentMatrix;
varying vec2 var_TexNormal;
#endif // USE_NORMAL_MAPPING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING


void main()
{
#if defined(USE_PORTAL_CLIPPING)
	if (var_BackSide < 0.0)
	{
		discard;
		return;
	}
#endif // USE_PORTAL_CLIPPING
	

	// compute incident ray
	vec3 V = var_ViewOrigin.xyz;


#if defined(USE_NORMAL_MAPPING)
	// normal
	vec3 Ntex = texture2D(u_NormalMap, var_TexNormal.st).xyz * 2.0 - 1.0;
	// transform normal from tangentspace to worldspace
	vec3 N = normalize(var_tangentMatrix * Ntex); // we must normalize to get a vector of unit-length..  reflect() needs it
#else // USE_NORMAL_MAPPING
	vec3 N = normalize(var_Normal.xyz);
#endif // USE_NORMAL_MAPPING


	// compute reflection ray
	vec3 R = reflect(V, N);
	R.x = -R.x;
	R.y = -R.y;
	//R.z = -R.z; // flip vertically

#if 1
	// This is the cubeProbes way of rendering reflections.
	vec4 envColor0 = textureCube(u_EnvironmentMap0, R).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, R).rgba;
	gl_FragColor = vec4(mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb * 0.1, 1.0); //* map.a
#else // 1
	gl_FragColor = textureCube(u_ColorMap, R).rgba;
#endif // 1

//#if 0
//	gl_FragColor = computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation); // this returns only .rgb (no alpha)
//#endif // 0
}
