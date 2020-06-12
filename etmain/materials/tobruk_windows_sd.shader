// tobruk_windows_sd.shader

textures/tobruk_windows_sd/tobruk_moucha1
{
    qer_editorimage textures/tobruk_windows_sd/tobruk_moucha1.tga
	cull none
	nomipmaps
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
	diffusemap textures/tobruk_windows_sd/tobruk_moucha1.tga
	specularmap textures/tobruk_windows_sd/tobruk_moucha1_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_moucha1_n.tga
}

textures/tobruk_windows_sd/tobruk_shutterbrown
{
    qer_editorimage textures/tobruk_windows_sd/tobruk_shutterbrown.tga
	surfaceparm woodsteps
	diffusemap textures/tobruk_windows_sd/tobruk_shutterbrown.tga
	specularmap textures/tobruk_windows_sd/tobruk_shutterbrown_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_shutterbrown_n.tga
}

textures/tobruk_windows_sd/tobruk_tobruk_lwind
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_lwind1.tga
	q3map_lightimage textures/tobruk_windows_sd/tobruk_lwind2.tga
	q3map_surfacelight 175
	surfaceparm nomarks
	diffusemap textures/tobruk_windows_sd/tobruk_lwind1.tga
	bumpmap textures/tobruk_windows_sd/tobruk_lwind1_n.tga
	specularmap textures/tobruk_windows_sd/tobruk_lwind1_r.tga
	{
		map textures/tobruk_windows_sd/tobruk_lwind2.tga
		blendfunc add
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/tobruk_windows_sd/tobruk_tobruk_mwind
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_mwind1.tga
	q3map_lightimage textures/tobruk_windows_sd/tobruk_mwind2.tga
	q3map_surfacelight 175
	surfaceparm nomarks
	diffusemap textures/tobruk_windows_sd/tobruk_mwind1.tga
	specularmap textures/tobruk_windows_sd/tobruk_mwind1_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_mwind1_n.tga
	{
		map textures/tobruk_windows_sd/tobruk_mwind2.tga
		blendfunc add
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/tobruk_windows_sd/tobruk_windows_bright
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_windows_on2.tga
	q3map_surfacelight 120
	surfaceparm nomarks
	diffusemap textures/tobruk_windows_sd/tobruk_windows_on2.tga
	specularmap textures/tobruk_windows_sd/tobruk_windows_on2_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_windows_on2_n.tga
	{
		map textures/tobruk_windows_sd/tobruk_windows_on1.tga
		blendfunc add
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/tobruk_windows_sd/tobruk_windows_medium
{
	qer_editorimage textures/tobruk_windows_sd/tobruk_windows_on1.tga
	q3map_surfacelight 45
	surfaceparm nomarks
	diffusemap textures/tobruk_windows_sd/tobruk_windows_on1.tga
	specularmap textures/tobruk_windows_sd/tobruk_windows_on1_r.tga
	bumpmap textures/tobruk_windows_sd/tobruk_windows_on1_n.tga
	{
		map textures/tobruk_windows_sd/tobruk_windows_on1.tga
		blendfunc add
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}
