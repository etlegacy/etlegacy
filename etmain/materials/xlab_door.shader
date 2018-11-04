// xlab_door.shader

textures/xlab_door/xdoor_m01
{
    qer_editorimage textures/xlab_door/xdoor_m01.tga
	diffusemap textures/xlab_door/xdoor_m01.tga
	specularmap textures/xlab_door/xdoor_m01_r.tga
	bumpmap textures/xlab_door/xdoor_m01_n.tga
	surfaceparm metalsteps
	{
		map textures/effects/tinfx.tga
		rgbGen identity
		tcGen environment
	}
	{
		map textures/xlab_door/xdoor_m01.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/xlab_door/xdoor_m01f
{
    qer_editorimage textures/xlab_door/xdoor_m01f.tga
	diffusemap textures/xlab_door/xdoor_m01f.tga
	specularmap textures/xlab_door/xdoor_m01f_r.tga
	bumpmap textures/xlab_door/xdoor_m01f_n.tga
	surfaceparm metalsteps
	{
		map textures/effects/tinfx.tga
		rgbGen identity
		tcGen environment
	}
	{
		map textures/xlab_door/xdoor_m01f.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}
