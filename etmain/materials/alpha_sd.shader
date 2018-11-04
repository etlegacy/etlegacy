// alpha_sd.shader

textures/alpha_sd/truss_m06grn
{
	qer_editorimage textures/seawall_wall/truss_m06.tga
	diffuseMap textures/seawall_wall/truss_m06.tga
	bumpMap textures/seawall_wall/truss_m06_n.tga
	specularMap textures/seawall_wall/truss_m06_r.tga
	surfaceparm alphashadow
	surfaceparm metalsteps
	nomipmaps
	nopicmip
	cull disable
	{
		map textures/seawall_wall/truss_m06.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}
