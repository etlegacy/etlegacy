// xlab_wall.shader

textures/xlab_wall/x_metalwall_m01
{
    qer_editorimage textures/xlab_wall/x_metalwall_m01.tga
	diffusemap textures/xlab_wall/x_metalwall_m01.tga
	specularmap textures/xlab_wall/x_metalwall_m01_r.tga
	bumpmap textures/xlab_wall/x_metalwall_m01_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xmetal_c03
{
    qer_editorimage textures/xlab_wall/xmetal_c03.tga
	diffusemap textures/xlab_wall/xmetal_c03.tga
	specularmap textures/xlab_wall/xmetal_c03_r.tga
	bumpmap textures/xlab_wall/xmetal_c03_n.tga
	surfaceparm metalsteps
	implicitMap -
}

//===========================================================================
// Metal phong for the brushwork only railings in fueldump
//===========================================================================
textures/xlab_wall/xmetal_c03_phong
{
    diffusemap textures/xlab_wall/xmetal_c03.tga
	specularmap textures/xlab_wall/xmetal_c03_r.tga
	bumpmap textures/xlab_wall/xmetal_c03_n.tga
	surfaceparm metalsteps
	q3map_nonplanar
	q3map_shadeangle 179
	qer_editorimage textures/xlab_wall/xmetal_c03_phong.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/xlab_wall/xmetal_c03.tga
		blendFunc filter
	}
}

textures/xlab_wall/xmetal_m02
{
    qer_editorimage textures/xlab_wall/xmetal_m02.tga
	diffusemap textures/xlab_wall/xmetal_m02.tga
	specularmap textures/xlab_wall/xmetal_m02_r.tga
	bumpmap textures/xlab_wall/xmetal_m02_n.tga
	surfaceparm metalsteps
	{
		map textures/effects/tinfx.tga
		rgbGen identity
		tcGen environment
	}
	{
		map textures/xlab_wall/xmetal_m02.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/xlab_wall/xmetal_m02a
{
    qer_editorimage textures/xlab_wall/xmetal_m02a.tga
	diffusemap textures/xlab_wall/xmetal_m02a.tga
	specularmap textures/xlab_wall/xmetal_m02a_r.tga
	bumpmap textures/xlab_wall/xmetal_m02a_n.tga
	surfaceparm metalsteps
	{
		map textures/effects/tinfx.tga
		rgbGen identity
		tcGen environment
	}
	{
		map textures/xlab_wall/xmetal_m02a.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/xlab_wall/xmetal_m02a_trim
{
    qer_editorimage textures/xlab_wall/xmetal_m02a_trim.tga
	diffusemap textures/xlab_wall/xmetal_m02a_trim.tga
	specularmap textures/xlab_wall/xmetal_m02a_trim_r.tga
	bumpmap textures/xlab_wall/xmetal_m02a_trim_n.tga
	surfaceparm metalsteps
	{
		map textures/effects/tinfx.tga
		rgbGen identity
		tcGen environment
	}
	{
		map textures/xlab_wall/xmetal_m02a_trim.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/xlab_wall/xmetal_m02f
{
	qer_editorimage textures/xlab_wall/xmetal_m02.tga
	diffusemap textures/xlab_wall/xmetal_m02f.tga
	specularmap textures/xlab_wall/xmetal_m02f_r.tga
	bumpmap textures/xlab_wall/xmetal_m02f_n.tga
	surfaceparm metalsteps
	implicitMap textures/xlab_wall/xmetal_m02.tga
}

textures/xlab_wall/xmetal_m03l
{
    qer_editorimage textures/xlab_wall/xmetal_m03l.tga
	diffusemap textures/xlab_wall/xmetal_m03l.tga
	specularmap textures/xlab_wall/xmetal_m03l_r.tga
	bumpmap textures/xlab_wall/xmetal_m03l_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xtrim_c02
{
    qer_editorimage textures/xlab_wall/xtrim_c02.tga
	diffusemap textures/xlab_wall/xtrim_c02.tga
	specularmap textures/xlab_wall/xtrim_c02_r.tga
	bumpmap textures/xlab_wall/xtrim_c02_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xtrim_c04
{
    qer_editorimage textures/xlab_wall/xtrim_c04.tga
	diffusemap textures/xlab_wall/xtrim_c04.tga
	specularmap textures/xlab_wall/xtrim_c04_r.tga
	bumpmap textures/xlab_wall/xtrim_c04_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xtrim_c05
{
    qer_editorimage textures/xlab_wall/xtrim_c05.tga
	diffusemap textures/xlab_wall/xtrim_c05.tga
	specularmap textures/xlab_wall/xtrim_c05_r.tga
	bumpmap textures/xlab_wall/xtrim_c05_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xtrim_c06
{
    qer_editorimage textures/xlab_wall/xtrim_c06.tga
	diffusemap textures/xlab_wall/xtrim_c06.tga
	specularmap textures/xlab_wall/xtrim_c06_r.tga
	bumpmap textures/xlab_wall/xtrim_c06_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xtrim_c07
{
    qer_editorimage textures/xlab_wall/xtrim_c07.tga
	diffusemap textures/xlab_wall/xtrim_c07.tga
	specularmap textures/xlab_wall/xtrim_c07_r.tga
	bumpmap textures/xlab_wall/xtrim_c07_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_wall/xwall_c07
{
    qer_editorimage textures/xlab_wall/xwall_c07.tga
	diffusemap textures/xlab_wall/xwall_c07.tga
	specularmap textures/xlab_wall/xwall_c07_r.tga
	bumpmap textures/xlab_wall/xwall_c07_n.tga
	surfaceparm metalsteps
	implicitMap -
}
