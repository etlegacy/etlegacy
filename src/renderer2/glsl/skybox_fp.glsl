/* skybox_fp.glsl */

uniform samplerCube u_ColorMap;
uniform vec3        u_ViewOrigin;
uniform vec4        u_PortalPlane;

varying vec3 var_Position;

void    main()
{
	#if defined(USE_PORTAL_CLIPPING)
	{
		float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if (dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif

	// compute incident ray
	vec3 I = normalize(var_Position - u_ViewOrigin);

	vec4 color = textureCube(u_ColorMap, I).rgba;

#if defined(r_DeferredShading)
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(0.0);
	gl_FragData[2] = vec4(0.0);
	gl_FragData[3] = vec4(0.0);
#else
	gl_FragColor = color;
#endif
}
