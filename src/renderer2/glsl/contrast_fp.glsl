/* contrast_fp.glsl */

uniform sampler2D u_ColorMap;

const vec4 LUMINANCE_VECTOR = vec4(0.2125, 0.7154, 0.0721, 0.0);

void main()
{
	vec2 st = gl_FragCoord.st;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	st *= r_FBufScale;

	// multiply with 4 because the FBO is only 1/4th of the screen resolution
	st *= vec2(4.0, 4.0);

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	// calculate contrast color
#if 0
	// perform a box filter for the downsample
	vec3 color = texture2D(u_ColorMap, st + vec2(-1.0, 1.0) * r_FBufScale).rgb;
	color += texture2D(u_ColorMap, st + vec2(1.0, 1.0) * r_FBufScale).rgb;
	color += texture2D(u_ColorMap, st + vec2(1.0, -1.0) * r_FBufScale).rgb;
	color += texture2D(u_ColorMap, st + vec2(-1.0, -1.0) * r_FBufScale).rgb;
	color *= 0.25;
#else
	vec4 color = texture2D(u_ColorMap, st);
#endif

    // adjust contrast
	float L = dot(LUMINANCE_VECTOR, color);
	L = max(L - 0.71, 0.0) * (1.0 / (1.0 - 0.71));
	
	return color * L;

	gl_FragColor = color;
}
