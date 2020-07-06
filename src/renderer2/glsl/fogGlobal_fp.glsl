/* fogGlobal_fp.glsl */

uniform vec4      u_Color;
uniform sampler2D u_ColorMap;   // fog texture
uniform sampler2D u_DepthMap;
uniform vec3      u_ViewOrigin;
uniform vec4      u_FogDistanceVector;
uniform vec4      u_FogDepthVector;
uniform mat4      u_UnprojectMatrix;
uniform float     u_FogDensity;

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
//	depth *= depth; // hack to see further  (depth*depth will always yield a smaller value, except when depth equals 1)
//	depth *= depth;
	vec4  P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;

	// calculate the length in fog (t is always 0 if eye is in fog)      <--- is this line copy/paste woot?..
	st.s = dot(P.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	st.t = u_FogDensity; // * 0.125; // divided by 8

	gl_FragColor = u_Color * texture2D(u_ColorMap, st);
}
