/* cubemap_fp.glsl */

uniform samplerCube u_ColorMap;
//uniform sampler2D u_ColorMap;

varying vec3 var_Position;
varying vec3 var_Normal;
varying vec3 var_ViewDirW;

void main()
{
#if 0
	// make axis vector
	vec2 uv;
	vec3 absPos = abs(var_Position);
	if (absPos.x > absPos.y) {
		if (absPos.x > absPos.z) {
			// x is greatest
			if (absPos.x < 0) {
				// X-
				uv.x = ((-var_Position.z / absPos.x) + 1) * 0.5;
				uv.y = ((-var_Position.y / absPos.x) + 1) * 0.5;
			} else {
				// X+
				uv.x = ((-var_Position.z / absPos.x) + 1) * 0.5;
				uv.y = ((-var_Position.y / absPos.x) + 1) * 0.5;
			}
		} else {
			// z is greatest
			if (absPos.z < 0) {
				// Z-
				uv.x = ((-var_Position.y / absPos.z) + 1) * 0.5;
				uv.y = ((-var_Position.x / absPos.z) + 1) * 0.5;
			} else {
				// Z+
				uv.x = ((-var_Position.y / absPos.z) + 1) * 0.5;
				uv.y = ((-var_Position.x / absPos.z) + 1) * 0.5;
			}
		}
	} else
	if (absPos.y > absPos.z) {
		// y is greatest
			if (absPos.y < 0) {
				// Y-
				uv.x = ((-var_Position.z / absPos.y) + 1) * 0.5;
				uv.y = ((-var_Position.x / absPos.y) + 1) * 0.5;
			} else {
				// Y+
				uv.x = ((-var_Position.z / absPos.y) + 1) * 0.5;
				uv.y = ((-var_Position.x / absPos.y) + 1) * 0.5;
			}
	} else {
		// z is greatest
		if (absPos.z < 0) {
			// Z-
			uv.x = ((-var_Position.y / absPos.z) + 1) * 0.5;
			uv.y = ((-var_Position.x / absPos.z) + 1) * 0.5;
		} else {
			// Z+
			uv.x = ((-var_Position.y / absPos.z) + 1) * 0.5;
			uv.y = ((-var_Position.x / absPos.z) + 1) * 0.5;
		}
	}
	gl_FragColor = texture2D(u_ColorMap, uv).rgba;
#endif

	gl_FragColor = textureCube(u_ColorMap, var_ViewDirW).rgba;
}
