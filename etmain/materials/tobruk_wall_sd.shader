// tobruk_wall_sd.shader

textures/tobruk_wall_base7_phong
{
	qer_editorimage textures/tobruk_wall_sd/tobruk_wall_base7_phong.tga
	q3map_nonplanar
	q3map_shadeangle 135
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/tobruk_wall_sd/tobruk_wall_base7.tga
		blendFunc filter
		rgbGen identity
	}
}
