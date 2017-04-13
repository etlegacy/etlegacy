textures/alpha_sd/truss_m06grn
{
	qer_editorimage textures/seawall_wall/truss_m06
	diffuseMap textures/seawall_wall/truss_m06
	bumpMap textures/seawall_wall/truss_m06_n
	specularMap textures/seawall_wall/truss_m06
	surfaceparm alphashadow
	surfaceparm metalsteps
	nomipmaps
	nopicmip
	cull disable
	{
		map textures/seawall_wall/truss_m06
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

