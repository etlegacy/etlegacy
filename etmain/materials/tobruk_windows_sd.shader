// tobruk_windows_sd.shader

textures/tobruk_windows_sd/tobruk_moucha1
{
    qer_editorimage textures/tobruk_windows_sd/tobruk_moucha1.tga
	diffusemap textures/tobruk_windows_sd/tobruk_moucha1.tga
	specularmap textures/tobruk_windows_sd/tobruk_moucha1_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_moucha1_n.tga
	cull none
	nomipmaps
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/tobruk_windows_sd/tobruk_shutterbrown
{
    qer_editorimage textures/tobruk_windows_sd/tobruk_shutterbrown.tga
	diffusemap textures/tobruk_windows_sd/tobruk_shutterbrown.tga
	specularmap textures/tobruk_windows_sd/tobruk_shutterbrown_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_shutterbrown_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/tobruk_windows_sd/tobruk_tobruk_lwind
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_lwind1.tga
	q3map_lightimage textures/tobruk_windows_sd/tobruk_lwind2.tga
	diffusemap textures/tobruk_windows_sd/tobruk_lwind1.tga
	bumpmap textures/tobruk_windows_sd/tobruk_lwind1_n.tga
	specularmap textures/tobruk_windows_sd/tobruk_lwind1_r.tga
	q3map_surfacelight 175
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_lwind1.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_lwind2.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/tobruk_windows_sd/tobruk_tobruk_mwind
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_mwind1.tga
	q3map_lightimage textures/tobruk_windows_sd/tobruk_mwind2.tga
	diffusemap textures/tobruk_windows_sd/tobruk_mwind1.tga
	specularmap textures/tobruk_windows_sd/tobruk_mwind1_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_mwind1_n.tga
	q3map_surfacelight 175
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_mwind1.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_mwind2.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/tobruk_windows_sd/tobruk_windows_bright
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_windows_on2.tga
	diffusemap textures/tobruk_windows_sd/tobruk_windows_on2.tga
	specularmap textures/tobruk_windows_sd/tobruk_windows_on2_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_windows_on2_n.tga
	q3map_surfacelight 120
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_on2.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_on1.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/tobruk_windows_sd/tobruk_windows_medium
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_windows_on1.tga
	diffusemap textures/tobruk_windows_sd/tobruk_windows_on1.tga
	specularmap textures/tobruk_windows_sd/tobruk_windows_on1_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_windows_on1_n.tga
	q3map_surfacelight 45
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_off.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_on1.tga
		blendfunc GL_ONE GL_ONE
	}
}
