// village.shader

textures/village/villwin_c12m_glass
{
	qer_editorimage textures/village/villwin_c12m.tga
	qer_trans 0.7
	cull disable
	surfaceparm glass
	surfaceparm pointlight
	surfaceparm trans
	bumpmap textures/village/villwin_c12m_n.tga
	specularmap textures/village/villwin_c12m_r.tga
	{
	    stage diffusemap
		map textures/village/villwin_c12m.tga
		blendfunc blend
		rgbgen vertex
	}
}

textures/village/villwin_c15
{
	qer_editorimage textures/village/villwin_c15.tga
	q3map_surfacelight 300
	surfaceparm nomarks
	diffusemap textures/village/villwin_c15.tga
	bumpmap textures/village/villwin_c15_n.tga
	specularmap textures/village/villwin_c15_r.tga
	{
		map textures/village/villwin_c15.blend.tga
		blendfunc add
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/village/villwin_c18
{
	qer_editorimage textures/village/villwin_c18.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 200
	surfaceparm nomarks
	diffusemap textures/village/villwin_c18.tga
	bumpmap textures/village/villwin_c18_n.tga
	specularmap textures/village/villwin_c18_r.tga
	{
		map textures/village/villwin_c18.blend.tga
		blendfunc add
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/village/vill2_win_m2
{
	qer_editorimage textures/village/vill2_win_m2.tga
	surfaceparm glass
	surfaceparm pointlight
	diffusemap textures/village/vill2_win_m2.tga
	bumpmap textures/village/vill2_win_m2_n.tga
	specularmap textures/village/vill2_win_m2_r.tga
}

textures/village/villwin_c08dm
{
	qer_editorimage textures/village/villwin_c08dm.tga
	surfaceparm glass
	surfaceparm pointlight
	diffusemap textures/village/villwin_c08dm.tga
	bumpmap textures/village/villwin_c08dm_n.tga
	specularmap textures/village/villwin_c08dm_r.tga
}
