/* reflection_C_fp.glsl */
// even while this is the version that does not use any normalmap, we need a function from the lib
#include "lib/normalMapping"


uniform float       u_ReflectionScale;
#if 1
uniform samplerCube u_EnvironmentMap0;
uniform samplerCube u_EnvironmentMap1;
uniform float       u_EnvironmentInterpolation;
#else
uniform samplerCube u_ColorMap;
#endif

varying vec3 var_Position;
varying vec3 var_Normal;
varying vec3 var_ViewOrigin; // position - vieworigin

void main()
{
	// incident ray
	vec3 V = var_ViewOrigin.xyz;

	// compute normal
	vec3 N = normalize(var_Normal.xyz);

	// compute reflection color
	// this is the no-normals version: just flat shading using the surface-normal
#if 1
	// This is the cubeProbes way of rendering reflections.
	gl_FragColor = vec4(computeReflections(V, N, u_EnvironmentMap0, u_EnvironmentMap1, u_EnvironmentInterpolation, u_ReflectionScale), 1.0);
#else
	// only 1 colormap is used
	vec3 R = reflect(V, N); // compute reflection ray
	gl_FragColor = textureCube(u_ColorMap, R).rgba;
#endif
}
