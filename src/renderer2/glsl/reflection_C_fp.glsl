/* reflection_C_fp.glsl */

#if 1
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#else
uniform samplerCube u_ColorMap;
#endif
uniform vec3        u_ViewOrigin;

varying vec3 var_Position;
varying vec3 var_Normal;

void main()
{
	// compute incident ray
	//vec3 I = normalize(var_Position - u_ViewOrigin);
	vec3 I = normalize(u_ViewOrigin - var_Position);

	// compute normal
	vec3 N = normalize(var_Normal);

	// compute reflection ray
	vec3 R = reflect(I, N);
	R.z = -R.z;

	// compute reflection color
#if 1
	// This is the cubeProbes way of rendering reflections.
	vec4 envColor0 = textureCube(u_EnvironmentMap0, R).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, R).rgba;
	gl_FragColor = mix(envColor0, envColor1, u_EnvironmentInterpolation).rgba;
#else
	gl_FragColor = textureCube(u_ColorMap, R).rgba;
#endif
}
