/* lightVolume_omni_fp.glsl */

uniform sampler2D   u_DepthMap;
uniform sampler2D   u_AttenuationMapXY;
uniform sampler2D   u_AttenuationMapZ;
uniform samplerCube u_ShadowMap;
uniform vec3        u_ViewOrigin;
uniform vec3        u_LightOrigin;
uniform vec3        u_LightColor;
uniform float       u_LightRadius;
uniform float       u_LightScale;
uniform mat4        u_LightAttenuationMatrix;
uniform int         u_ShadowCompare;
uniform mat4        u_UnprojectMatrix;

varying vec2 var_TexDiffuse;
varying vec3 var_TexAttenXYZ;

void main()
{
#if 0
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;
#else
	vec2 st = gl_FragCoord.st * r_FBufNPOTScale;
#endif

	// reconstruct vertex position in world space
	float depth = texture2D(u_DepthMap, st).r;
	vec4  P     = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;

#if 0
	#if defined(USE_PORTAL_CLIPPING)
		float dist = dot(P.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if (dist < 0.0)
		{
			discard;
			return;
		}
	#endif
#endif

	// compute incident ray in world space
	vec3 R = normalize(P.xyz - u_ViewOrigin);
	//vec3 R = normalize(u_ViewOrigin - P.xyz);

	//float traceDistance = dot(P.xyz - (u_ViewOrigin.xyz + R * u_ZNear ), forward);
	//traceDistance = clamp(traceDistance, 0.0, 2500.0 ); // Far trace distance

	float traceDistance = distance(P.xyz, u_ViewOrigin);
	traceDistance = clamp(traceDistance, 0.0, 2500.0);

	// TODO move to front clipping plane

	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

	//int steps = 40;
	//float stepSize = traceDistance / float(steps);

	int   steps    = int(min(traceDistance, 2000.0)); // TODO r_MaxSteps
	float stepSize = 64.0;

	for (float tracedDistance = 0.0; tracedDistance < traceDistance; tracedDistance += stepSize)
	{
		//vec3 T = P.xyz + (R * stepSize * float(i));
		//vec3 T = u_ViewOrigin + (R * stepSize * float(i));
		vec3 T = u_ViewOrigin + (R * tracedDistance);

		// compute attenuation
		vec3 texAttenXYZ   = (u_LightAttenuationMatrix * vec4(T, 1.0)).xyz;
		vec3 attenuationXY = texture2D(u_AttenuationMapXY, texAttenXYZ.xy).rgb;
		vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(texAttenXYZ.z, 0)).rgb;

		float shadow = 1.0;

		color.rgb += attenuationXY * attenuationZ;
	}

	color.rgb /= float(steps);
	color.rgb *= u_LightColor;
	//color.rgb *= u_LightScale;

	gl_FragColor = color;
}
