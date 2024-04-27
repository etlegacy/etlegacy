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
	// calculate the screen texcoord in the 0.0 to 1.0 range
	// and scale by the screen non-power-of-two-adjust
	vec2 st = gl_FragCoord.st * r_FBufNPOTScale;

	// reconstruct vertex position in world space
	float depth = texture2D(u_DepthMap, st).r;
	// scale to NDC (Normalized Device Coordinates) space
	vec4  P = vec4(gl_FragCoord.xy, depth, 1.0) * 2.0 - 1.0;
	// unproject to get into viewspace
	P = u_UnprojectMatrix * P;
	// normalize to homogeneous coordinates (where w is always 1)
	P.xyz /= P.w;

	st.s = dot(P.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	st.t = u_FogDensity;

	gl_FragColor = u_Color * texture2D(u_ColorMap, st);
}
