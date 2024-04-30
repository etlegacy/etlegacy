/* fogQuake3_fp.glsl */

uniform sampler2D u_ColorMap;

varying vec2  var_Tex;
varying vec4  var_Color;
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
#endif // end USE_PORTAL_CLIPPING

	gl_FragColor = texture2D(u_ColorMap, var_Tex) * var_Color;
}
