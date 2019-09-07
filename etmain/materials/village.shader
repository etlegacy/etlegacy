// village.shader

textures/village/villwin_c12m_glass
{
	qer_editorimage textures/village/villwin_c12m.tga
	qer_trans 0.7
	cull disable
	surfaceparm glass
	surfaceparm pointlight
	surfaceparm trans
	{
	    stage diffuseMap
		map textures/village/villwin_c12m.tga
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendfunc blend
		//alphaFunc GE128
		rgbgen vertex
		depthWrite
	}
	{
	   	stage bumpMap
	    map textures/village/villwin_c12m_n.tga
	    //blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendfunc blend
		//alphaFunc GE128
		rgbgen vertex
		depthWrite
	}
	{
	   stage specularMap
	   map textures/village/villwin_c12m_r.tga
       //blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendfunc blend
		//alphaFunc GE128
		rgbgen vertex
		depthWrite
	}
}

textures/village/villwin_c15
{
	qer_editorimage textures/village/villwin_c15.tga
	diffuseMap textures/village/villwin_c15.tga
	specularMap textures/village/villwin_c15_r.tga
	bumpMap textures/village/villwin_c15_n.tga
	q3map_surfacelight 300
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/village/villwin_c15.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/village/villwin_c15.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/village/villwin_c18
{
	qer_editorimage textures/village/villwin_c18.tga
	diffuseMap textures/village/villwin_c18.tga
	specularMap textures/village/villwin_c18_r.tga
	bumpMap textures/village/villwin_c18_n.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 200
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/village/villwin_c18.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/village/villwin_c18.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/village/vill2_win_m2
{
        qer_editorimage textures/village/vill2_win_m2.tga
		diffuseMap textures/village/vill2_win_m2.tga
		specularMap textures/village/vill2_win_m2_r.tga
		bumpMap textures/village/vill2_win_m2_n.tga
		surfaceparm glass
	    surfaceparm pointlight
}

textures/village/villwin_c08dm
{
        qer_editorimage textures/village/villwin_c08dm.tga
		diffuseMap textures/village/villwin_c08dm.tga
		specularMap textures/village/villwin_c08dm_r.tga
		bumpMap textures/village/villwin_c08dm_n.tga
		surfaceparm glass
	    surfaceparm pointlight
}
