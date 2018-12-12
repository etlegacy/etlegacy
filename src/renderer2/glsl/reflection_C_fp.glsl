/* reflection_C_fp.glsl */

#if 1
uniform samplerCube u_ColorMap;
#else
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#endif

varying vec3 var_Position;
varying vec3 var_Normal;
varying vec3 var_ViewOrigin; // position - vieworigin

void main()
{
	// compute incident ray
	//vec3 V = normalize(var_Position - u_ViewOrigin);
	//vec3 V = normalize(u_ViewOrigin - var_Position);
	vec3 V = var_ViewOrigin.xyz;

	// compute normal
	vec3 N = normalize(var_Normal);

	// compute reflection ray
	vec3 R = reflect(V, N);
	R.z = -R.z;

	// compute reflection color
#if 1
	// only 1 colormap is used (tcGen environment)
	gl_FragColor = textureCube(u_ColorMap, R).rgba;
#else
	// This is the cubeProbes way of rendering reflections.
	vec4 envColor0 = textureCube(u_EnvironmentMap0, R).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, R).rgba;
	gl_FragColor = mix(envColor0, envColor1, u_EnvironmentInterpolation).rgba;
#endif
}
