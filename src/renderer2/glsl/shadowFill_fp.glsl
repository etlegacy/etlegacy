/* shadowFill_fp.glsl */

#if defined(USE_ALPHA_TESTING)
uniform int       u_AlphaTest;
uniform sampler2D u_ColorMap;
#endif // USE_ALPHA_TESTING
#if defined(USE_PORTAL_CLIPPING)
uniform vec4      u_PortalPlane;
#endif // USE_PORTAL_CLIPPING

#if defined(VSM) || defined(EVSM) || defined(ESM)
#if !defined(LIGHT_DIRECTIONAL)
uniform vec3  u_LightOrigin;
uniform float u_LightRadius;

varying float var_Distance;
#if defined(VSM)
varying float var_DistanceSquared;
#endif
#endif // LIGHT_DIRECTIONAL
#endif // VSM, EVSM, ESM

varying vec3 var_Position;
#if defined(USE_ALPHA_TESTING)
varying vec2 var_Tex;
#endif // USE_ALPHA_TESTING
#if !defined(LIGHT_DIRECTIONAL)
#if defined(EVSM)
#if !defined(r_EVSMPostProcess)
varying vec4 var_FragColorEVSM;
#endif // r_EVSMPostProcess
#endif // EVSM
#endif // LIGHT_DIRECTIONAL

#if defined(USE_PORTAL_CLIPPING)
varying float var_BackSide; // in front, or behind, the portalplane
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
#endif

#if defined(VSM)
	float distance_;
	float distanceSquared;
#if defined(LIGHT_DIRECTIONAL)
	distance_       = gl_FragCoord.z;
	distanceSquared = distance_ * distance_;
#else
	distance_       = var_Distance;
	distanceSquared = var_DistanceSquared;
#endif
	// shadowmap can be float RGBA or luminance alpha so store distanceSquared into alpha
#if defined(VSM_CLAMP)
	// convert to [0,1] color space
	gl_FragColor = vec4(distance_, 0.0, 0.0, distanceSquared) * 0.5 + 0.5;
#else
	gl_FragColor = vec4(distance_, 0.0, 0.0, distanceSquared);
#endif


#elif defined(EVSM) || defined(ESM)
	float distance_;
#if defined(LIGHT_DIRECTIONAL)
	distance_ = gl_FragCoord.z; // * r_ShadowMapDepthScale;
#else
	distance_ = var_Distance;
#endif
#if defined(EVSM)
	#if !defined(r_EVSMPostProcess)
		gl_FragColor = var_FragColorEVSM; //ShadowDepthToEVSM(distance_);
	#else
		gl_FragColor = vec4(0.0, 0.0, 0.0, distance_);
	#endif
#else
	gl_FragColor = vec4(0.0, 0.0, 0.0, distance_);
#endif // defined(EVSM)

#else
	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
#endif
}
