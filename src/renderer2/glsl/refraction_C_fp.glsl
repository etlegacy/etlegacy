/* refraction_C_fp.glsl */

uniform samplerCube u_ColorMap;
uniform vec3        u_ViewOrigin;
uniform float       u_RefractionIndex;
uniform float       u_FresnelPower;
uniform float       u_FresnelScale;
uniform float       u_FresnelBias;

varying vec3 var_Position;
varying vec3 var_Normal;

void main()
{
	// compute incident ray
	vec3 I = normalize(var_Position - u_ViewOrigin);

	// compute normal
	vec3 N = normalize(var_Normal.xyz);

	// compute reflection ray
	vec3 R = reflect(I, N);

	// compute refraction ray
	vec3 T = refract(I, N, u_RefractionIndex);

	// compute fresnel term
	float fresnel = u_FresnelBias + pow(1.0 - dot(I, N), u_FresnelPower) * u_FresnelScale;

	vec3 reflectColor = textureCube(u_ColorMap, R).rgb;
	vec3 refractColor = textureCube(u_ColorMap, T).rgb;

	// compute final color
	vec4 color;
	color.r = (1.0 - fresnel) * refractColor.r + reflectColor.r * fresnel;
	color.g = (1.0 - fresnel) * refractColor.g + reflectColor.g * fresnel;
	color.b = (1.0 - fresnel) * refractColor.b + reflectColor.b * fresnel;
	color.a = 1.0;

	gl_FragColor = color;
}
