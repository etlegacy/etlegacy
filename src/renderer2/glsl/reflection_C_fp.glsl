/* reflection_C_fp.glsl */

uniform samplerCube u_ColorMap;
uniform vec3        u_ViewOrigin;

varying vec3 var_Position;
varying vec3 var_Normal;

void main()
{
	// compute incident ray
	vec3 I = normalize(var_Position - u_ViewOrigin);

	// compute normal
	vec3 N = normalize(var_Normal);

	// compute reflection ray
	vec3 R = reflect(I, N);

	// compute reflection color
	vec4 reflectColor = textureCube(u_ColorMap, R).rgba;

	// compute final color
	vec4 color = reflectColor;

	gl_FragColor = color;
}
