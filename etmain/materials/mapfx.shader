// mapfx.shader

textures/mapfx/translucent
{
	qer_editorimage textures/common/dirtymirror.tga
	cull none
	surfaceparm glass
	surfaceparm pointlight
	surfaceparm trans
	{
		map textures/common/dirtymirror.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
		tcgen environment
	}
}

textures/mapfx/translucent_red
{
	qer_editorimage textures/common/dirtymirror.tga
	cull none
	surfaceparm glass
	surfaceparm pointlight
	surfaceparm trans
	{
		map textures/common/dirtymirror.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 0.5
		tcgen environment
	}
	{
		map $whiteimage
		blendfunc filter
		rgbGen const ( 1.0 0.6 0.6 )
	}
}
