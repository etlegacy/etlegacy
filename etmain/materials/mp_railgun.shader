// mp_railgun.shader

textures/mp_railgun/cable
{
	qer_editorimage textures/props/cable_m01
	surfaceparm metalsteps
	{
		stage diffusemap
		map textures/props/cable_m01.tga
		rgbGen vertex
	}
	{
		stage bumpmap
		map textures/props/cable_m01_n.tga
		rgbGen vertex
	}
	{
		stage specularmap
		map textures/props/cable_m01_r.tga
		rgbGen vertex
	}
}

textures/mp_railgun/fog
{
	qer_editorimage textures/sfx/fog_grey1.tga
	fogparms ( 0.6 0.6 0.6 ) 2560
	surfaceparm fog
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
}

textures/mp_railgun/lmterrain_base
{
	q3map_lightmapMergable
	q3map_lightmapaxis z
	q3map_lightmapsize 512 512
	q3map_normalimage textures/sd_bumpmaps/normalmap_terrain.tga
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_tcMod rotate 37
	q3map_tcMod scale 1 1
	surfaceparm landmine
	surfaceparm snowsteps
}

textures/mp_railgun/lmterrain_0
{
	q3map_baseshader textures/mp_railgun/lmterrain_base.tga
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_0to1
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_0to2
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_0to3
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_0to4
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_0to5
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_0to6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_dirt_m03i_2_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_dirt_m03i_2_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_dirt_m03i_2_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_1
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_1to2
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_1to3
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_1to4
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_1to5
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_1to6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/bigrock_rounded_faint.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/bigrock_rounded_faint_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/bigrock_rounded_faint_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_2
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_2to3
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_2to4
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_2to5
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_2to6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_var01_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_var01_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_var01_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_3
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_3to4
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_3to5
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_3to6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_muddy.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_muddy_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_muddy_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_4
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_4to5
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_4to6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_5
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_5to6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/snow_noisy.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/snow_noisy_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/snow_noisy_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/lmterrain_6
{
	q3map_baseshader textures/mp_railgun/lmterrain_base
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage diffusemap
		map textures/snow_sd/s_grass_ml03b_big.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/snow_sd/s_grass_ml03b_big_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		stage specularmap
		map textures/snow_sd/s_grass_ml03b_big_r.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/mp_railgun/sky
{
	qer_editorimage textures/skies/sky_6
	q3map_skylight 90 3
	q3map_sun 0.55 0.55 0.55 90 220 60
	skyparms - 200 -
	surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
}
