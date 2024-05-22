/* toneMapping_fp.glsl */

uniform sampler2D u_CurrentMap;
uniform float     u_HDRKey;
uniform float     u_HDRAverageLuminance;
uniform float     u_HDRMaxLuminance;

const vec4 LUMINANCE_VECTOR  = vec4(0.2125, 0.7154, 0.0721, 0.0);
const vec3 BLUE_SHIFT_VECTOR = vec3(1.05, 0.97, 1.27);

void main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

#if defined(BRIGHTPASS_FILTER)
	// multiply with 4 because the FBO is only 1/4th of the screen resolution
	st *= vec2(4.0, 4.0);
#endif

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	vec4 color = texture2D(u_CurrentMap, st);

	// see http://www.gamedev.net/reference/articles/article2208.asp
	// for Mathematics of Reinhard's Photographic Tone Reproduction Operator

	// get the luminance of the current pixel
	float Y = dot(LUMINANCE_VECTOR, color);

#if defined(BRIGHTPASS_FILTER)
	if (Y < 0.1)
	{
		discard;
		return;
	}
#endif

	// calculate the relative luminance
	float Yr = u_HDRKey * Y / u_HDRAverageLuminance;

	float Ymax = u_HDRMaxLuminance;

	// RGB -> XYZ conversion
	const mat3 RGB2XYZ = mat3(0.4124564, 0.2126729, 0.0193339,
	                          0.3575761, 0.7151522, 0.1191920,
	                          0.1804375, 0.0721750, 0.9503041);

	vec3 XYZ = RGB2XYZ * color.rgb;

	// XYZ -> Yxy conversion
	vec3 Yxy;

	// Y = Y luminance
	Yxy.r = XYZ.g;

	// x = X / (X + Y + Z)
	Yxy.g = XYZ.r / (XYZ.r + XYZ.g + XYZ.b);

	// y = Y / (X + Y + Z)
	Yxy.b = XYZ.g / (XYZ.r + XYZ.g + XYZ.b);

	// (Lp) map average luminance to the middlegrey zone by scaling pixel luminance
	float Lp = Yxy.r * u_HDRKey / u_HDRAverageLuminance;

	// (Ld) scale all luminance within a displayable range of 0 to 1

#if defined(r_HDRToneMappingOperator_1)
	Yxy.r = (Lp * (1.0 + Lp / (Ymax * Ymax))) / (1.0 + Lp);
#else
	Yxy.r = 1.0 - exp(-Lp);
#endif

	// Yxy -> XYZ conversion

	// X = Y * x / y
	XYZ.r = Yxy.r * Yxy.g / Yxy.b;

	// Y = Y
	XYZ.g = Yxy.r;

	// Z = Y * (1-x-y) / y  or  Z = (1 - x - y) * (Y / y)
	XYZ.b = (1.0 - Yxy.g - Yxy.b) * (Yxy.r / Yxy.b);

	// XYZ -> RGB conversion
	const mat3 XYZ2RGB = mat3(3.2404542, -0.9692660, 0.0556434,
	                          -1.5371385, 1.8760108, -0.2040259,
	                          -0.4985314, 0.0415560, 1.0572252);

	color.rgb = clamp(XYZ2RGB * XYZ, 0.0, 1.0);
	// color.rgb *= Yxy.r;

#if defined(BRIGHTPASS_FILTER)
#if defined(r_HDRRendering)
	// adjust contrast
	// L = pow(L, 1.32);

	float T = max(Lp - r_HDRContrastThreshold, 0.0);
	// float T = max(1.0 - exp(-Yr) - r_HDRContrastThreshold, 0.0);
	float B = T > 0.0 ? T / (r_HDRContrastOffset + T) : T;

	color.rgb *= clamp(B, 0.0, 1.0);
#endif
#endif

#if 0 // defined(r_HDRGamma)
	float gamma = 1.0 / r_HDRGamma;
	color.r = pow(color.r, gamma);
	color.g = pow(color.g, gamma);
	color.b = pow(color.b, gamma);
#endif

	gl_FragColor = color;

#if 0
	gl_FragColor = vec4(Lp, Lp, Lp, 1.0);
#endif
}
