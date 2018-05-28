textures/radar/dirt_m03icmp_brown
{
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	diffusemap textures/radar/dirt_m03icmp_brown
	bumpmap textures/radar/dirt_m03icmp_brown_n
	specularmap textures/radar/dirt_m03icmp_brown_s
	surfaceparm trans
	rgbGen identity	
}    

textures/radar/dirt_m04cmp_brown
{
	qer_editorimage textures/temperate_sd/dirt_m04cmp_brown
	diffusemap textures/radar/dirt_m04cmp_brown
	bumpmap textures/radar/dirt_m04cmp_brown_n
	specularmap textures/radar/dirt_m04cmp_brown_s
	surfaceparm trans
	rgbGen identity	

}
// could be fun to make this move... thoughts for later
textures/radar/fog
{
	qer_editorimage textures/sfx/fog_grey1
	
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm fog
	fogparms ( 0.09411 0.09803 0.12549 ) 3192
}

textures/radar/lmterrain2_base
{
	q3map_normalimage textures/sd_bumpmaps/normalmap_terrain
	
	q3map_lightmapsize 512 512
	q3map_lightmapMergable
	q3map_lightmapaxis z
	
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_tcMod scale 2 2
	q3map_tcMod rotate 37
	
	surfaceparm grasssteps
	surfaceparm landmine	
}

textures/radar/lmterrain2_foliage_base
{
	q3map_baseShader textures/radar/lmterrain2_base

	q3map_foliage models/foliage/grassfoliage1.md3 1.25 48 0.1 2
	q3map_foliage models/foliage/grassfoliage2.md3 1.1 48 0.1 2
	q3map_foliage models/foliage/grassfoliage3.md3 1 48 0.1 2
}

textures/radar/lmterrain2_foliage_fade
{
	q3map_baseShader textures/radar/lmterrain2_base

	q3map_foliage models/foliage/grassfoliage1.md3 1.25 64 0.1 2
	q3map_foliage models/foliage/grassfoliage2.md3 1.1 64 0.1 2
	q3map_foliage models/foliage/grassfoliage3.md3 1 64 0.1 2
}

textures/radar/lmterrain2_0
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_1
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_path1_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_path1_s
		rgbgen identity
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_path1_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_path1_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_2
{
	q3map_baseshader textures/radar/lmterrain2_base
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		rgbgen identity
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_3
{
	q3map_baseshader textures/radar/lmterrain2_base
    {
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_4
{
	q3map_baseshader textures/radar/lmterrain2_base
   
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/dirt3_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/dirt3_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_5
{
	q3map_baseshader textures/radar/lmterrain2_base
    {
	   stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		rgbgen identity
	}
	{   stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		rgbgen identity
	}
	{   stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_ml03cmp_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/grass_ml03cmp_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_0to1
{
	q3map_baseshader textures/radar/lmterrain2_foliage_fade

	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/grass_path1_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_0to2
{
	q3map_baseshader textures/radar/lmterrain2_base

	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_0to3
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_0to4
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/dirt3_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}
textures/radar/lmterrain2_0to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_1to2
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/grass_path1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_1to3
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/grass_path1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_1to4
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/grass_path1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/dirt3_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_1to5
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		rgbgen identity
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		rgbgen identity
	}
	{
	    stage specularmap
	    map textures/temperate_sd/grass_path1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_2to3
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		rgbgen identity
		
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_2to4
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/dirt3_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_2to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		rgbgen identity
		
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_3to4
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/dirt3_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_3to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/radar/lmterrain2_4to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/dirt3_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

//**********************************************
// rain FX
//base shader has problem with texture scaling
//should be fixed by me or ydnar later
//back to the old school way - Chris-
//**********************************************

textures/radar/road_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	
	surfaceparm trans
	{
	    stage diffusemap
	    Map textures/temperate_sd/dirt_m03icmp_brown
		rgbGen identity
	}
	{
	    stage bumpmap
	    Map textures/temperate_sd/dirt_m03icmp_brown
		rgbGen identity
	}
	{
	    stage specularmap
	    Map textures/temperate_sd/dirt_m03icmp_brown
		rgbGen identity
	}
}

textures/radar/road
{
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown

	surfaceparm trans
	
	{
		stage diffusemap 
		map textures/temperate_sd/dirt_m03icmp_brown
		rgbGen identity	
	}
	{	
		stage bumpmap
		map textures/temperate_sd/dirt_m03icmp_brown_n
		rgbGen identity	
	}
	{
		stage specularmap
		map textures/temperate_sd/dirt_m03icmp_brown_s
		rgbGen identity	
	}
}


textures/radar/road_puddle1
{
	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/temperate_sd/road_puddle1
	surfaceparm trans
	surfaceparm splash
		
	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
	    stage diffusemap
		map textures/temperate_sd/road_puddle1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/road_puddle1_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{   
	    stage specularmap
		map textures/temperate_sd/road_puddle1_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
}

textures/radar/road_bigpuddle
{
	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/temperate_sd/road_bigpuddle
	
	surfaceparm trans
	surfaceparm splash
	
	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
	    blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
	    stage diffusemap
		map textures/temperate_sd/road_bigpuddle
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/road_bigpuddle_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/road_bigpuddle_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
}


textures/radar/borderroad
{
	qer_editorimage textures/temperate_sd/dirt_m04cmp_brown
	
	diffusemap textures/temperate_sd/dirt_m04cmp_brown
	bumpmap textures/temperate_sd/dirt_m04cmp_brown_n
	specularmap textures/temperate_sd/dirt_m04cmp_brown_s
	surfaceparm trans
}

textures/radar/wood_m02_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/wood/wood_m02
	
	diffusemap textures/wood/wood_m02
	bumpmap textures/wood/wood_m02_n
	specularmap textures/wood/wood_m02_s
	
}

textures/radar/gy_ml03a_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/graveyard/gy_ml03a
	
	diffusemap textures/graveyard/gy_ml03a
	bumpmap textures/graveyard/gy_ml03a_n
	specularmap textures/graveyard/gy_ml03a_s

}

textures/radar/debri_m05_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/rubble/debri_m05
	
	diffusemap textures/rubble/debri_m05
	bumpmap textures/rubble/debri_m05_n
	specularmap textures/rubble/debri_m05_s
}

textures/radar/wood_m16_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/wood/wood_m16
	
	diffusemap textures/wood/wood_m16
	bumpmap textures/wood/wood_m16_n
	specularmap textures/wood/wood_m16_s
}

textures/radar/wall_c01_wet
{
//	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
//	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
//	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
//	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
//	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
//	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/sleepy/wall_c01

	diffusemap textures/sleepy/wall_c01
	bumpmap textures/sleepy/wall_c01_n
	specularmap textures/sleepy/wall_c01_s
}

textures/radar/metal_wet1
{
	qer_editorimage textures/metals_sd/metal_ref1

	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
		blendFunc GL_SRC_ALPHA GL_ONE
		tcGen environment
	}
	{
		map textures/metals_sd/metal_ref1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		
		rgbGen identity
	}
}

textures/radar/metal_wet2
{
	q3map_foliage models/foliage/raincircle0.md3 1 64 0.1 2
	q3map_foliage models/foliage/raincircle1.md3 0.8 64 0.1 2
	q3map_foliage models/foliage/raincircle2.md3 0.6 64 0.1 2
	q3map_foliage models/foliage/raincircle3.md3 0.9 64 0.1 2
	q3map_foliage models/foliage/raincircle4.md3 0.7 64 0.1 2
	q3map_foliage models/foliage/raincircle5.md3 0.5 64 0.1 2
	qer_editorimage textures/metals_sd/metal_ref1

	{
		map textures/effects/envmap_radar
		rgbGen identity
		tcMod scale 0.5 0.5
		tcGen environment
	}
	{
		map textures/liquids_sd/puddle_specular
		rgbGen identity
		tcMod scale 2 2
		blendFunc  GL_SRC_ALPHA GL_SRC_COLOR
		tcGen environment
	}
	{
		map textures/metals_sd/metal_ref1
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen identity
	}
}
