/* cameraEffects_fp.glsl */
uniform sampler2D u_CurrentMap;
uniform sampler2D u_GrainMap;
uniform sampler2D u_VignetteMap;

varying vec2 var_Tex;

void main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 stClamped = gl_FragCoord.st * r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	vec2 st = stClamped * r_NPOTScale;

	vec4 original = clamp(texture2D(u_CurrentMap, st), 0.0, 1.0);

	vec4 color = original;

	// calculate chromatic aberration
#if 0
	vec2 redOffset   = vec2(0.5, 0.25);
	vec2 greenOffset = vec2(0.0, 0.0);
	vec2 blueOffset  = vec2(-0.5, -0.25);

	color.r = texture2D(u_CurrentMap, st + redOffset * r_FBufScale).r;
	color.g = texture2D(u_CurrentMap, st + greenOffset * r_FBufScale).g;
	color.b = texture2D(u_CurrentMap, st + blueOffset * r_FBufScale).b;
#endif

	// blend the vignette
	vec4 vignette = texture2D(u_VignetteMap, stClamped);
	color.rgb *= vignette.rgb;

	// add grain
	vec4 grain = texture2D(u_GrainMap, var_Tex);
	color.rgb = (color.rgb + (grain.rgb * vec3(0.035, 0.065, 0.09))) + (color.rgb * (grain.rgb * vec3(0.035, 0.065, 0.09)));
	//color.rgb = grain.rgb;

	gl_FragColor = color;
}
