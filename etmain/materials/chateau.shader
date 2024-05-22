// chateau.shader

textures/chateau/desk_c03
{
    qer_editorimage textures/chateau/desk_c03.tga
	diffusemap textures/chateau/desk_c03.tga
	bumpmap textures/chateau/desk_c03_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/desk_c04
{
    qer_editorimage textures/chateau/desk_c04.tga
	diffusemap textures/chateau/desk_c04.tga
	bumpmap textures/chateau/desk_c04_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/door_c01
{
    qer_editorimage textures/chateau/door_c01.tga
	diffusemap textures/chateau/door_c01.tga
	bumpmap textures/chateau/door_c01_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/fireplace_01
{
    qer_editorimage textures/chateau/fireplace_01.tga
	diffusemap textures/chateau/fireplace_01.tga
	bumpmap textures/chateau/fireplace_01_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/floor_c04
{
    qer_editorimage textures/chateau/floor_c04.tga
	diffusemap textures/chateau/floor_c04.tga
	bumpmap textures/chateau/floor_c04_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/floor_c05
{
    qer_editorimage textures/chateau/floor_c05.tga
	diffusemap textures/chateau/floor_c05.tga
	bumpmap textures/chateau/floor_c05_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/floor_c06
{
    qer_editorimage textures/chateau/floor_c06.tga
	diffusemap textures/chateau/floor_c06.tga
	bumpmap textures/chateau/floor_c06_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/floor_c10
{
    qer_editorimage textures/chateau/floor_c10.tga
	diffusemap textures/chateau/floor_c10.tga
	bumpmap textures/chateau/floor_c10_n.tga
	surfaceparm carpetsteps
	implicitMap -
}

textures/chateau/floor_c10a
{
    qer_editorimage textures/chateau/floor_c10a.tga
	diffusemap textures/chateau/floor_c10a.tga
	bumpmap textures/chateau/floor_c10a_n.tga
	surfaceparm carpetsteps
	implicitMap -
}

textures/chateau/pillar_temp_rf
{
    qer_editorimage textures/chateau/pillar_temp_rf.tga
	diffusemap textures/chateau/pillar_temp_rf.tga
	bumpmap textures/chateau/pillar_temp_rf_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/stair_c01
{
    qer_editorimage textures/chateau/stair_c01.tga
	diffusemap textures/chateau/stair_c01.tga
	bumpmap textures/chateau/stair_c01_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/stair_c03
{
    qer_editormap textures/chateau/stair_c03.tga
	diffusemap textures/chateau/stair_c03.tga
	bumpmap textures/chateau/stair_c03_n.tga
	surfaceparm carpetsteps
	implicitMap -
}

textures/chateau/trim_c01
{
    qer_editorimage textures/chateau/trim_c01.tga
	diffusemap textures/chateau/trim_c01.tga
	bumpmap textures/chateau/trim_c01_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/trim_c03
{
    qer_editorimage textures/chateau/trim_c03.tga
	diffusemap textures/chateau/trim_c03.tga
	bumpmap textures/chateau/trim_c03_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/trim_c08
{
    qer_editorimage textures/chateau/trim_c08.tga
	diffusemap textures/chateau/trim_c08.tga
	bumpmap textures/chateau/trim_c08_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/trim_c09
{
    qer_editorimage textures/chateau/trim_c09.tga
	diffusemap textures/chateau/trim_c09.tga
	bumpmap textures/chateau/trim_c09_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/trim_c10
{
    qer_editorimage textures/chateau/trim_c10.tga
	diffusemap textures/chateau/trim_c10.tga
	bumpmap textures/chateau/trim_c10_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/trim_c14
{
    qer_editorimage textures/chateau/trim_c14.tga
	diffusemap textures/chateau/trim_c14.tga
	bumpmap textures/chateau/trim_c14_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/walltest_c07a_vertex
{
	qer_editorimage chateau/walltest_c07a
	{
		map textures/chateau/walltest_c07a.tga
		rgbGen vertex
		tcMod Scale .25 .25
	}
}

textures/chateau/window_c03a
{
	qer_editorimage textures/chateau/window_c03a.tga
	q3map_lightimage textures/lightimage/soft_blue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 300
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c03a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c01.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/chateau/window_c03a_nolight
{
	qer_editorimage textures/chateau/window_c03a.tga
	implicitMap textures/chateau/window_c03a.tga
}

textures/chateau/window_c03a_750
{
	qer_editorimage textures/chateau/window_c03a.tga
	q3map_lightimage textures/lightimage/soft_blue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 750
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c03a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c01.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chateau/window_c03a_1250
{
	qer_editorimage textures/chateau/window_c03a.tga
	q3map_lightimage textures/lightimage/soft_blue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 1250
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c03a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c01.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chateau/window_c03a_2500
{
	qer_editorimage textures/chateau/window_c03a.tga
	q3map_lightimage textures/lightimage/soft_blue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 2500
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c03a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c01.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chateau/window_c04a
{
	qer_editorimage textures/chateau/window_c04a.tga
	q3map_lightimage textures/lightimage/soft_blue2.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 300
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c04a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c02.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chateau/window_c07a
{
	qer_editorimage textures/chateau/window_c07a.tga
	q3map_lightimage textures/lightimage/soft_warm.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 80
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c07a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c05.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chateau/window_director
{
	qer_editorimage textures/chateau/window_c03a.tga
	q3map_lightimage textures/lightimage/soft_blue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 3000
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chateau/window_c03a.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chateau/window_c01.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chateau/wood_c05
{
    qer_editorimage textures/chateau/wood_c05.tga
	diffusemap textures/chateau/wood_c05.tga
	bumpmap textures/chateau/wood_c05_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/wood_c06
{
    qer_editorimage textures/chateau/wood_c06.tga
	diffusemap textures/chateau/wood_c06.tga
	bumpmap textures/chateau/wood_c06_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/wood_c07
{
    qer_editorimage textures/chateau/wood_c07.tga
	diffusemap textures/chateau/wood_c07.tga
	bumpmap textures/chateau/wood_c07_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/wood_c09
{
    qer_editorimage textures/chateau/wood_c09.tga
	diffusemap textures/chateau/wood_c09.tga
	bumpmap textures/chateau/wood_c09_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/wood_c20
{
    qer_editorimage textures/chateau/wood_c20.tga
	diffusemap textures/chateau/wood_c20.tga
	bumpmap textures/chateau/wood_c20_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/chateau/wood_test
{
    qer_editorimage textures/chateau/wood_test.tga
	diffusemap textures/chateau/wood_test.tga
	bumpmap textures/chateau/wood_test_n.tga
	surfaceparm woodsteps
	implicitMap -
}
