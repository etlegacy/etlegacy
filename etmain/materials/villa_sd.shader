// villa_sd.shader

textures/villa_sd/villawindow_a
{
    qer_editorimage textures/villa_sd/villawindow_a.tga
	bumpMap textures/villa_sd/villawindow_a.tga
	specularMap textures/villa_sd/villawindow_a_r.tga
	bumpMap textures/villa_sd/villawindow_a_n.tga
	surfaceparm alphashadow
	surfaceparm glass
	surfaceparm pointlight
	surfaceparm trans
	{
		map textures/villa_sd/villawindow_a.tga
		blendfunc GL_ONE GL_ONE
		rgbGen vertex
	}
	{
		map textures/villa_sd/villawindow_a.tga
		alphaFunc GE128
		rgbGen vertex
	}
}
