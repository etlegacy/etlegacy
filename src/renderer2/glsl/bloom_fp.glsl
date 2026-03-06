/* bloom_fp.glsl */

uniform sampler2D u_ColorMap;
uniform sampler2D u_ContrastMap;
uniform float     u_BlurMagnitude;

void main()
{
	vec2 st = gl_FragCoord.st;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	st *= r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	// we use the Normal-gauss distribution formula
	// f(x) being the formula, we used f(0.5)-f(-0.5); f(1.5)-f(0.5)...

	#if 0
	float gaussFact[3] = float[3](1.0, 2.0, 1.0);
	#elif 1
	float gaussFact[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);
	float gaussSum     = 4096.0; // = 64.0^2 = result of sumWeights;
	#elif 0
	float gaussFact[7] = float[7](1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0);
	float gaussSum     = 4096.0; // = 64.0^2 = result of sumWeights;
	#else
	float gaussFact[11] = float[11](
	    0.0222244, 0.0378346, 0.0755906, 0.1309775, 0.1756663,
	    0.1974126,
	    0.1756663, 0.1309775, 0.0755906, 0.0378346, 0.0222244
	    );
	#endif

	// do a full gaussian blur
	vec4  sumColors  = vec4(0.0);
	float sumWeights = 0.0;

	int tap = 2;
	for (int i = -tap; i < tap; i++)
	{
		for (int j = -tap; j < tap; j++)
		{
			float weight = gaussFact[i + 2] * gaussFact[j + 2];
			vec4  color  = texture2D(u_ContrastMap, st + vec2(i, j) * u_BlurMagnitude * r_FBufScale) * weight;

			sumColors  += color;
			sumWeights += weight;
		}
	}

	//gl_FragColor = texture2D(u_ColorMap, st) + sumColors / gaussSum;
	//gl_FragColor = sumColors / gaussSum;
	gl_FragColor = sumColors / sumWeights;
}
