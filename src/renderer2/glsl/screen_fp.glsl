/* screen_fp.glsl */

uniform sampler2D u_CurrentMap;

varying vec4 var_Color;

void main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	vec4 color = texture2D(u_CurrentMap, st);
	color *= var_Color;

	gl_FragColor = color;
}
