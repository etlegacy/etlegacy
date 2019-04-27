// mp_siwa.shader

textures/mp_siwa/sky
{
	qer_editorimage textures/skies_sd/nero_bluelight.tga
	q3map_lightimage textures/skies_sd/siwa_clouds.tga
	q3map_skylight 75 3
	q3map_sun 0.75 0.70 0.6 135 199 49
	skyparms - 200 -
	surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	{
		map textures/skies_sd/siwa_clouds.tga
		tcMod scale 3 3
		tcMod scroll -0.001 -0.0025
	}
}

textures/mp_siwa/lmterrain_base
{   
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	q3map_lightmapsize 512 512
	q3map_lightmapMergable
	q3map_lightmapsamplesize 16
	q3map_lightmapaxis z
	q3map_shadeangle 179
}

textures/mp_siwa/lmterrain_0
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/sand_wave_desert.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
		stage diffuseMap
		map textures/desert_sd/sand_wave_desert.tga
		rgbGen identity
		alphaGen vertex
	}
	{
		stage bumpMap
		map textures/desert_sd/sand_wave_desert_n.tga
		rgbGen identity
		alphaGen vertex
	}
	{
		stage specularMap
		map textures/desert_sd/sand_wave_desert_r.tga
		rgbGen identity
		alphaGen vertex
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/grass_sand_flat.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_sand_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/grass_sand_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_sand_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_2
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/sand_patchy.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_patchy.tga
	bumpmap textures/desert_sd/sand_patchy_n.tga
	specularmap textures/desert_sd/sand_patchy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_patchy.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_patchy_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_patchy_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_3
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/sand_disturb_desert.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_disturb_desert.tga
	bumpmap textures/desert_sd/sand_disturb_desert_n.tga
	specularmap textures/desert_sd/sand_disturb_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_disturb_desert.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_disturb_desert_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_disturb_desert_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_4
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/grass_desert_flat.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_desert_flat.tga
	bumpmap textures/desert_sd/grass_desert_flat_n.tga
	specularmap textures/desert_sd/grass_desert_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_desert_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/grass_desert_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_desert_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_5
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/rock_edged_smooth.tga
	diffusemap textures/desert_sd/rock_edged_smooth.tga
	bumpmap textures/desert_sd/rock_edged_smooth_n.tga
	specularmap textures/desert_sd/rock_edged_smooth_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/rock_edged_smooth_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/sand_gravels_bright.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	
	// shader stage 1
	diffusemap textures/desert_sd/sand_gravels_bright.tga
	specularmap textures/desert_sd/sand_gravels_bright_r.tga
	bumpmap textures/desert_sd/sand_gravels_bright_n.tga
/*	
	// enable parallax relief mapping for this material.
	parallax
	// For parallax relief mapping to work, you need to provide a special kind of normalmap texture.
	// That parallax-normalmap contains in the alpha-channel the heightmap values. Shades of gray.
	// The 'displaceMap' function can make such a normal+height map, but it is not the best way for all textures.
	// But it is done on-the-fly, and is very easy to use and adjust.
	// Here's how it works:
	//     displaceMap(<map>, <map>)
	//     Sets the alpha channel to an average of the second image's RGB channels.
	// To create a displaceMap, the 2 provided images must be of the same resolution.
	//
	bumpmap displaceMap(textures/desert_sd/sand_gravels_bright_n.tga, textures/desert_sd/sand_gravels_bright_p.tga)
	//
	// You can also prepare an image with an appropriate alpha channel, a displacemap. In that case you just have to assign that map.
	//bumpmap textures/desert_sd/sand_gravels_bright_n.tga
	//
*/
	// shader stage 2
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	qer_editorimage textures/desert_sd/sand_dirt_medium.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_dirt_medium.tga
	bumpmap textures/desert_sd/sand_dirt_medium_n.tga
	specularmap textures/desert_sd/sand_dirt_medium_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to1
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
 	bumpmap textures/desert_sd/sand_wave_desert_n.tga
 	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_sand_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/grass_sand_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_sand_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to2
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_patchy.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_patchy_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_patchy_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to3
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_disturb_desert.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_disturb_desert_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_disturb_desert_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to4
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_desert_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/grass_desert_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_desert_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to5
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/rock_edged_smooth_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_0to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_wave_desert.tga
	bumpmap textures/desert_sd/sand_wave_desert_n.tga
	specularmap textures/desert_sd/sand_wave_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1to2
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_patchy.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_patchy_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_patchy_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1to3
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_disturb_desert.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_disturb_desert_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_disturb_desert_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1to4
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_desert_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/grass_desert_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_desert_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1to5
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/rock_edged_smooth_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1to6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_1to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_sand_flat.tga
	bumpmap textures/desert_sd/grass_sand_flat_n.tga
	specularmap textures/desert_sd/grass_sand_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_2to3
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_patchy.tga
	bumpmap textures/desert_sd/sand_patchy_n.tga
	specularmap textures/desert_sd/sand_patchy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_disturb_desert.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_disturb_desert_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_disturb_desert_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}
textures/mp_siwa/lmterrain_2to4
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_patchy.tga
	bumpmap textures/desert_sd/sand_patchy_n.tga
	specularmap textures/desert_sd/sand_patchy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_desert_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{   
	    stage bumpMap
		map textures/desert_sd/grass_desert_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_desert_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_2to5
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_patchy.tga
	bumpmap textures/desert_sd/sand_patchy_n.tga
	specularmap textures/desert_sd/sand_patchy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
    	map textures/desert_sd/rock_edged_smooth_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
    }
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_2to6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_patchy.tga
	bumpmap textures/desert_sd/sand_patchy_n.tga
	specularmap textures/desert_sd/sand_patchy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_2to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_patchy.tga
	bumpmap textures/desert_sd/sand_patchy_n.tga
	specularmap textures/desert_sd/sand_patchy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_3to4
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_disturb_desert.tga
	bumpmap textures/desert_sd/sand_disturb_desert_n.tga
	specularmap textures/desert_sd/sand_disturb_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/grass_desert_flat.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/grass_desert_flat_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/grass_desert_flat_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_3to5
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_disturb_desert.tga
	bumpmap textures/desert_sd/sand_disturb_desert_n.tga
	specularmap textures/desert_sd/sand_disturb_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/rock_edged_smooth_n.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
		rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_3to6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_disturb_desert.tga
	bumpmap textures/desert_sd/sand_disturb_desert_n.tga
	specularmap textures/desert_sd/sand_disturb_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_3to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_disturb_desert.tga
	bumpmap textures/desert_sd/sand_disturb_desert_n.tga
	specularmap textures/desert_sd/sand_disturb_desert_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}	
}

textures/mp_siwa/lmterrain_4to5
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_desert_flat.tga
	bumpmap textures/desert_sd/grass_desert_flat_n.tga
	specularmap textures/desert_sd/grass_desert_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/rock_edged_smooth_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_4to6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_desert_flat.tga
	bumpmap textures/desert_sd/grass_desert_flat_n.tga
	specularmap textures/desert_sd/grass_desert_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_4to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/grass_desert_flat.tga
	bumpmap textures/desert_sd/grass_desert_flat_n.tga
	specularmap textures/desert_sd/grass_desert_flat_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_5to6
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/rock_edged_smooth.tga
	bumpmap textures/desert_sd/rock_edged_smooth_n.tga
	specularmap textures/desert_sd/rock_edged_smooth_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_gravels_bright.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_gravels_bright_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_gravels_bright_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_5to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/rock_edged_smooth.tga
	bumpmap textures/desert_sd/rock_edged_smooth_n.tga
	specularmap textures/desert_sd/rock_edged_smooth_r.tga
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/lmterrain_6to7
{
	q3map_baseshader textures/mp_siwa/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_gravels_bright.tga
	bumpmap textures/desert_sd/sand_gravels_bright_n.tga
	specularmap textures/desert_sd/sand_gravels_bright_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/mp_siwa/sand_dirt
{
	q3map_nonplanar
	q3map_shadeangle 120
	qer_editorimage textures/desert_sd/sand_dirt_medium.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/desert_sd/sand_dirt_medium.tga
	bumpmap textures/desert_sd/sand_dirt_medium_n.tga
	specularmap textures/desert_sd/sand_dirt_medium_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/sand_dirt_medium.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/sand_dirt_medium_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/sand_dirt_medium_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc filter
	}
}

textures/mp_siwa/rock_edged
{
	q3map_nonplanar
	q3map_shadeangle 120
	qer_editorimage textures/desert_sd/rock_edged_smooth.tga
	surfaceparm landmine
	surfaceparm gravelsteps
    diffusemap textures/desert_sd/rock_edged_smooth.tga
    bumpmap textures/desert_sd/rock_edged_smooth_n.tga
	specularmap textures/desert_sd/rock_edged_smooth_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/rock_edged_smooth.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpMap
		map textures/desert_sd/rock_edged_smooth_n.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularMap
		map textures/desert_sd/rock_edged_smooth_r.tga
	    rgbGen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}
