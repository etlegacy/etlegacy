/* debugShadowMap_fp.glsl */

uniform sampler2D u_ShadowMap;

varying vec2 var_TexCoord;

void main()
{
#if defined(ESM)

	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);

	float shadowDistance = shadowMoments.a;

	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);

#elif defined(VSM)
	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);

	float shadowDistance        = shadowMoments.r;
	float shadowDistanceSquared = shadowMoments.a;

	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);

#elif defined(EVSM)

	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);

#if defined(r_EVSMPostProcess)
	float shadowDistance = shadowMoments.r;
#else
	float shadowDistance = log(shadowMoments.b);
#endif

	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);
#else
	discard;
#endif
}
