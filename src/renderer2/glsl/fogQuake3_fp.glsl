/* fogQuake3_fp.glsl */

uniform sampler2D u_ColorMap;
uniform vec4      u_PortalPlane;

varying vec3 var_Position;
varying vec2 var_Tex;
varying vec4 var_Color;

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

	vec4 color = texture2D(u_ColorMap, var_Tex);

	color       *= var_Color;
	gl_FragColor = color;

#if 0
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), color.a);
#endif
}
