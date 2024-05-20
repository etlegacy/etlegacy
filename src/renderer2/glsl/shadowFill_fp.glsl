/* shadowFill_fp.glsl */

#if defined(USE_ALPHA_TESTING)
uniform int       u_AlphaTest;
uniform sampler2D u_ColorMap;
#endif // USE_ALPHA_TESTING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4 u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

#if defined(EVSM)
#if LIGHT_DIRECTIONAL == 0
uniform vec3  u_LightOrigin;
uniform float u_LightRadius;

varying vec3  var_Position;
varying float var_Distance;
#endif // LIGHT_DIRECTIONAL
#endif // EVSM
#if defined(USE_ALPHA_TESTING)
varying vec2 var_Tex;
#endif // USE_ALPHA_TESTING
#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide;     // in front, or behind, the portalplane
#endif // USE_PORTAL_CLIPPING



void main()
{

#if defined(USE_PORTAL_CLIPPING)
	if (var_BackSide < 0.0)
	{
		discard;
		return;
	}
#endif


#if defined(USE_ALPHA_TESTING)
	vec4 color = texture2D(u_ColorMap, var_Tex);
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
#endif // USE_ALPHA_TESTING


#if defined(EVSM)
#if LIGHT_DIRECTIONAL == 1
	gl_FragColor = vec4(0.0, 0.0, 0.0, gl_FragCoord.z); // * r_ShadowMapDepthScale;
#else // LIGHT_DIRECTIONAL
	gl_FragColor = vec4(0.0, 0.0, 0.0, var_Distance);
#endif // LIGHT_DIRECTIONAL

#else // EVSM
	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
#endif // EVSM



}
