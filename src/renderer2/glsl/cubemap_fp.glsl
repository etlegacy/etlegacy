/* cubemap_fp.glsl */

uniform samplerCube u_ColorMap;

varying vec3 var_ViewDirW;

void main()
{
	gl_FragColor = textureCube(u_ColorMap, varViewDirW).rgba;
}
