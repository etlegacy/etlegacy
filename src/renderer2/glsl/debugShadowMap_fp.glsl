/* debugShadowMap_fp.glsl */

uniform sampler2D u_ShadowMap;

varying vec2 var_TexCoord;

void main()
{
#if defined(EVSM)
	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);
	float shadowDistance = shadowMoments.r;
	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);
#else
	discard;
#endif
}
