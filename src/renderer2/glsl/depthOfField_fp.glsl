/* depthOfField_fp.glsl */

uniform sampler2D u_CurrentMap;
uniform sampler2D u_DepthMap;

void main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	float focus  = 0.98;        // focal distance, normalized 0.8-0.999
	float radius = 0.5;     // 0 - 20.0

	vec4 sum = vec4(0.0, 0.0, 0.0, 0.0);

	// autofocus
	focus = texture2D(u_DepthMap, vec2(0.5, 0.5) * r_NPOTScale).r;

	const float tap  = 5.0;
	const float taps = tap * 2.0 + 1.0;

	float depth = texture2D(u_DepthMap, st).r;
	float delta = (abs(depth - focus) * abs(depth - focus)) / float(tap);
	delta *= radius;
	//delta = clamp(radius * delta, -max, max);

	for (float i = -tap; i < tap; i++)
	{
		for (float j = -tap; j < tap; j++)
		{
			sum += texture2D(u_CurrentMap, st + vec2(i, j) * delta);
		}
	}

	sum *= 1.0 / (taps * taps);

	gl_FragColor = sum;
}
