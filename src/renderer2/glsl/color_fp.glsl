uniform sampler2D u_CurrentMap;
uniform float     u_gamma;

void main()
{
	vec2 uv    = gl_FragCoord.st * r_FBufScale;
	vec3 color = texture2D(u_CurrentMap, uv).rgb;

	gl_FragColor.a = 1.0;

	//Just to debug stuff for now
	if (false)
	{
		if (uv.x > 0.50)
		{
			gl_FragColor.rgb = color;
			return;
		}
	}

	// calculate gamma
	gl_FragColor.rgb = pow(color, vec3(1.0 / u_gamma));
}