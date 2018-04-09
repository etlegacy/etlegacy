/* skybox_fp.glsl */

uniform samplerCube u_ColorMap;
uniform vec3        u_ViewOrigin;
uniform vec4        u_PortalPlane;

varying vec3 var_Position;

void main()
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

	gl_FragColor = color;
}
