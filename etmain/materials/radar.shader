textures/radar/dirt_m03icmp_brown
{
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	diffusemap textures/radar/dirt_m03icmp_brown
	bumpmap textures/radar/dirt_m03icmp_brown_n
	specularmap textures/radar/dirt_m03icmp_brown_s
	surfaceparm trans
	{
	    Map textures/temperate_sd/dirt_m03icmp_brown
		vertexcolor
		blend blend
	}
}    

textures/radar/dirt_m04cmp_brown
{
	qer_editorimage textures/temperate_sd/dirt_m04cmp_brown
	diffusemap textures/radar/dirt_m04cmp_brown
	bumpmap textures/radar/dirt_m04cmp_brown_n
	specularmap textures/radar/dirt_m04cmp_brown_s
	surfaceparm trans
	{
	    Map textures/temperate_sd/dirt_m04cmp_brown
		vertexcolor
		blend blend
	}
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
	diffusemap textures/temperate_sd/master_grass_dirt3
	bumpmap textures/temperate_sd/master_grass_dirt3_n
	specularmap textures/temperate_sd/master_grass_dirt3_s

	{
		map textures/temperate_sd/master_grass_dirt3
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
	diffusemap textures/temperate_sd/grass_path1
	bumpmap textures/temperate_sd/grass_path1_n
	specularmap textures/temperate_sd/grass_path1_s

	{
		map textures/temperate_sd/grass_path1
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
    diffusemap textures/temperate_sd/grass_dense1
	bumpmap textures/temperate_sd/grass_dense1_n
	specularmap textures/temperate_sd/grass_dense1_s
	{
		map textures/temperate_sd/grass_dense1
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
    diffusemap textures/temperate_sd/rock_ugly_brown
	bumpmap textures/temperate_sd/rock_ugly_brown_n
	specularmap textures/temperate_sd/rock_ugly_brown_s
	{
		map textures/temperate_sd/rock_ugly_brown
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
    diffusemap textures/temperate_sd/dirt3
	bumpmap textures/temperate_sd/dirt3_n
	specularmap textures/temperate_sd/dirt3_s
	{
		map textures/temperate_sd/dirt3
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
    diffusemap textures/temperate_sd/grass_ml03cmp
	bumpmap textures/temperate_sd/grass_ml03cmp_n
	specularmap textures/temperate_sd/grass_ml03cmp_s
	{
		map textures/temperate_sd/grass_ml03cmp
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
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		vertexcolor
		blend blend
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		vertexcolor
		blend blend
	}
	{
	   stage specularmap
	   map textures/temperate_sd/grass_path1_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_0to2
{
	q3map_baseshader textures/radar/lmterrain2_base

	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_dense1_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_n
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_0to3
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_ugly_brown_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_0to4
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		vertexcolor
		blend blend
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		vertexcolor
		blend blend
	}
}
textures/radar/lmterrain2_0to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/master_grass_dirt3
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/master_grass_dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/master_grass_dirt3_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_1to2
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		vertexcolor
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		vertexcolor
		blend blend
	}
	{
	   stage specularmap
	   map textures/temperate_sd/grass_path1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/grass_dense1_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_1to3
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		vertexcolor
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		vertexcolor
		blend blend
	}
	{
	   stage specularmap
	   map textures/temperate_sd/grass_path1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/rock_ugly_brown_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_1to4
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		vertexcolor
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		vertexcolor
		blend blend
	}
	{
	   stage specularmap
	   map textures/temperate_sd/grass_path1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		vertexcolor
		blend blend
	}
	
}

textures/radar/lmterrain2_1to5
{
	q3map_baseshader textures/radar/lmterrain2_foliage_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_path1
		vertexcolor
	}
	{   stage bumpmap
	    map textures/temperate_sd/grass_path1_n
		vertexcolor
		blend blend
	}
	{
	   stage specularmap
	   map textures/temperate_sd/grass_path1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_2to3
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		vertexcolor
		
	}
	{
	    stage normalmap
		map textures/temperate_sd/grass_dense1_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/rock_ugly_brown_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_2to4
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		vertexcolor
		
	}
	{
	    stage normalmap
		map textures/temperate_sd/grass_dense1_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_2to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/grass_dense1
		vertexcolor
		
	}
	{
	    stage normalmap
		map textures/temperate_sd/grass_dense1_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_dense1_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		vertexcolor
		blend blend
	}
	
}

textures/radar/lmterrain2_3to4
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		vertexcolor
	}
	{
	    stage normalmap
		map textures/temperate_sd/rock_ugly_brown_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		vertexcolor
		blend blend
	}
	{
	    stage normalmap
		map textures/temperate_sd/dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_3to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_ugly_brown
		vertexcolor
	}
	{
	    stage normalmap
		map textures/temperate_sd/rock_ugly_brown_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_ugly_brown_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		vertexcolor
		blend blend
	}
}

textures/radar/lmterrain2_4to5
{
	q3map_baseshader textures/radar/lmterrain2_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/dirt3
		vertexcolor
	}
	{
	    stage normalmap
		map textures/temperate_sd/dirt3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/dirt3_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/temperate_sd/grass_ml03cmp
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/grass_ml03cmp_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/grass_ml03cmp_s
		vertexcolor
		blend blend
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
	    Map textures/temperate_sd/dirt_m03icmp_brown
		rgbGen identity
		blend blend
	}
}

textures/radar/road
{
	
	qer_editorimage textures/temperate_sd/dirt_m03icmp_brown
	diffusemap textures/temperate_sd/dirt_m03icmp_brown
	bumpmap textures/temperate_sd/dirt_m03icmp_brown_n
	specularmap textures/temperate_sd/dirt_m03icmp_brown_s
	surfaceparm trans
	{
	    Map textures/temperate_sd/dirt_m03icmp_brown
		rgbGen identity
		blend blend
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
	diffusemap textures/temperate_sd/road_puddle1
	bumpmap textures/temperate_sd/road_puddle1_n
	specularmap textures/temperate_sd/road_puddle1
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
		map textures/temperate_sd/road_puddle1
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
	diffusemap textures/temperate_sd/road_bigpuddle
	bumpmap textures/temperate_sd/road_bigpuddle_n
	specularmap textures/temperate_sd/road_bigpuddle_s
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
		map textures/temperate_sd/road_bigpuddle
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
	{
	    Map textures/temperate_sd/dirt_m04cmp_brown
		vertexcolor
		blend blend
	}
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
	{
	    Map textures/wood/wood_m02
		vertexcolor
		blend blend
	}
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
	{
	    Map textures/graveyard/gy_ml03a
		vertexcolor
		blend blend
	}
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
	{
	    Map textures/rubble/debri_m05
		vertexcolor
		blend blend
	}
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
	{
	    Map textures/wood/wood_m16
		vertexcolor
		blend blend
	}
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
	{
	    Map textures/sleepy/wall_c01
		vertexcolor
		blend blend
	}
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
