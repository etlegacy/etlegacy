uniform sampler2D u_CurrentMap;
uniform float     u_gamma;

void main()
{
	vec2 uv    = gl_FragCoord.st * r_FBufScale;
	vec3 color = texture2D(u_CurrentMap, uv).rgb;

	//Just to debug stuff for now
	if (true)
	{
		if (uv.x < 0.50)
		{
			gl_FragColor.rgb = pow(color, vec3(1.0 / u_gamma));
		}
		else
		{
			gl_FragColor.rgb = color;
		}
	}
	else
	{
		gl_FragColor.rgb = pow(color, vec3(1.0 / u_gamma));
	}

	gl_FragColor.a = 1.0;

	gl_FragColor.rgb = vec3(1.0);
}