/* screenSpaceAmbientOcclusion_fp.glsl */

uniform sampler2D u_CurrentMap;
uniform sampler2D u_DepthMap;

float ReadDepth(vec2 st)
{
	vec2 camerarange = vec2(4.0, 4096.0);

	return (2.0 * camerarange.x) / (camerarange.y + camerarange.x - texture2D(u_DepthMap, st).x * (camerarange.y - camerarange.x));
}

void main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	float depth = ReadDepth(st);
	float d;

	float aoCap = 1.0;
	float ao    = 0.0;

	float aoMultiplier   = 1000.0;
	float depthTolerance = 0.0001;

	float tap  = 3.0;
	float taps = tap * 2.0 + 1.0;
	for (float i = -tap; i < tap; i++)
	{
		for (float j = -tap; j < tap; j++)
		{
			d   = ReadDepth(st + vec2(j, i) * r_FBufScale);
			ao += min(aoCap, max(0.0, depth - d - depthTolerance) * aoMultiplier);
		}
	}

	ao /= (taps * taps);

	gl_FragColor = vec4(1.0 - ao) * texture2D(u_CurrentMap, st);
}
