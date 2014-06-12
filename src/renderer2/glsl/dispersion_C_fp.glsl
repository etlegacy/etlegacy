/* dispersion_C_fp.glsl */

uniform samplerCube u_ColorMap;
uniform vec3        u_ViewOrigin;
uniform vec3        u_EtaRatio;
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
	vec3 N = normalize(var_Normal); // FIXME normalize?

	// compute reflection ray
	vec3 R = reflect(I, N);

	// compute fresnel term
	float fresnel = u_FresnelBias + pow(1.0 - dot(I, N), u_FresnelPower) * u_FresnelScale;

	// compute reflection color
	vec3 reflectColor = textureCube(u_ColorMap, R).rgb;

	// compute refraction color using a refraction ray for each channel
	vec3 refractColor;

	refractColor.r = textureCube(u_ColorMap, refract(I, N, u_EtaRatio.x)).r;
	refractColor.g = textureCube(u_ColorMap, refract(I, N, u_EtaRatio.y)).g;
	refractColor.b = textureCube(u_ColorMap, refract(I, N, u_EtaRatio.z)).b;

	// compute final color
	vec4 color;
	color.r = (1.0 - fresnel) * refractColor.r + reflectColor.r * fresnel;
	color.g = (1.0 - fresnel) * refractColor.g + reflectColor.g * fresnel;
	color.b = (1.0 - fresnel) * refractColor.b + reflectColor.b * fresnel;
	color.a = 1.0;

	gl_FragColor = color;
}
