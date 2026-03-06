/* generic_fp.glsl */

uniform sampler2D u_ColorMap;
uniform int       u_AlphaTest;
uniform vec4      u_PortalPlane;

varying vec3 var_Position;
varying vec2 var_Tex;
varying vec4 var_Color;

void main()
{
#if defined(USE_PORTAL_CLIPPING)
	float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
	if (dist < 0.0)
	{
		discard;
		return;
	}
#endif

	vec4 color = texture2D(u_ColorMap, var_Tex);

#if defined(USE_ALPHA_TESTING)
	if (u_AlphaTest == ATEST_GT_0 && color.a <= 0.0)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_LT_128 && color.a >= 0.5)
	{
		discard;
		return;
	}
	else if (u_AlphaTest == ATEST_GE_128 && color.a < 0.5)
	{
		discard;
		return;
	}
#endif

	color *= var_Color;

	gl_FragColor = color;
}

