// chat_window.shader

textures/chat_window/chwindow_c05
{
	qer_editorimage textures/chat_window/chwindow_c05.tga
	Q3map_lightimage textures/lightimage/ltblue_m1a.tga
	refractionIndex 1.5
	q3map_surfacelight 300
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chat_window/chwindow_c05.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chat_window/chwindow_c05.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/chat_window/chwindow_c06
{
	qer_editorimage textures/chat_window/chwindow_c06.tga
	Q3map_lightimage textures/lightimage/chwindow_c06.tga
	refractionIndex 1.5
	q3map_surfacelight 300
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/chat_window/chwindow_c06.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/chat_window/chwindow_c06.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}
