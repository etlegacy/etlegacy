// tobruk_windows_sd.shader
//thunder 2017
// 5 total shaders

textures/tobruk_windows_sd/tobruk_moucha1
{
    qer_editorimage textures/tobruk_windows_sd/tobruk_moucha1
	diffusemap textures/tobruk_windows_sd/tobruk_moucha1
	specularmap textures/tobruk_windows_sd/tobruk_moucha1_s
	bumpmap textures/tobruk_windows_sd/tobruk_moucha1_n
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
    qer_editorimage textures/tobruk_windows_sd/tobruk_shutterbrown
	diffusemap textures/tobruk_windows_sd/tobruk_shutterbrown
	specularmap textures/tobruk_windows_sd/tobruk_shutterbrown_s
	bumpmap textures/tobruk_windows_sd/tobruk_shutterbrown_n
	surfaceparm woodsteps
	implicitMap -
}

textures/tobruk_windows_sd/tobruk_tobruk_lwind
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_lwind1
	q3map_lightimage textures/tobruk_windows_sd/tobruk_lwind2
	diffusemap textures/tobruk_windows_sd/tobruk_lwind1
	bumpmap textures/tobruk_windows_sd/tobruk_lwind1_n
	specularmap textures/tobruk_windows_sd/tobruk_lwind1_s
	q3map_surfacelight 175
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_lwind1
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_lwind2
		blendfunc GL_ONE GL_ONE
	}
}

textures/tobruk_windows_sd/tobruk_tobruk_mwind
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_mwind1
	q3map_lightimage textures/tobruk_windows_sd/tobruk_mwind2
	diffusemap textures/tobruk_windows_sd/tobruk_mwind1
	specularmap textures/tobruk_windows_sd/tobruk_mwind1_s
	bumpmap textures/tobruk_windows_sd/tobruk_mwind1_n
	q3map_surfacelight 175
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_mwind1
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_mwind2
		blendfunc GL_ONE GL_ONE
	}
}

textures/tobruk_windows_sd/tobruk_windows_bright
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_windows_on2
	diffusemap textures/tobruk_windows_sd/tobruk_windows_on2
	specularmap textures/tobruk_windows_sd/tobruk_windows_on2_s
	bumpmap textures/tobruk_windows_sd/tobruk_windows_on2_n
	q3map_surfacelight 120
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_on2
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_on1
		blendfunc GL_ONE GL_ONE
	}
}

textures/tobruk_windows_sd/tobruk_windows_medium
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_windows_on1
	diffusemap textures/tobruk_windows_sd/tobruk_windows_on1
	specularmap textures/tobruk_windows_sd/tobruk_windows_on1_s
	bumpmap textures/tobruk_windows_sd/tobruk_windows_on1_n
	q3map_surfacelight 45
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_off
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/tobruk_windows_sd/tobruk_windows_on1
		blendfunc GL_ONE GL_ONE
	}
}
