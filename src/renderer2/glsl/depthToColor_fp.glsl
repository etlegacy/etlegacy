/* depthToColor_fp.glsl */

void main()
{
	// compute depth instead of world vertex position in a [0..1] range
	float depth = gl_FragCoord.z;

	// 32 bit precision
	const vec4 bitSh  = vec4(256 * 256 * 256, 256 * 256, 256, 1);
	const vec4 bitMsk = vec4(0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);

	vec4 comp;
	comp         = depth * bitSh;
	comp         = fract(comp);
	comp        -= comp.xxyz * bitMsk;
	gl_FragColor = comp;
/*
    // 24 bit precision
    const vec3 bitSh = vec3(256 * 256,			256,		1);
    const vec3 bitMsk = vec3(		0,	1.0 / 256.0,		1.0 / 256.0);

    vec3 comp;
    comp = depth * bitSh;
    comp = fract(comp);
    comp -= comp.xxy * bitMsk;
    gl_FragColor = vec4(comp, 0.0);
*/
}
