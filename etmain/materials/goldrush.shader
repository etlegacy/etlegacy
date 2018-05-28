textures/goldrush/lmterrain_base
{
	q3map_lightmapaxis z
	q3map_lightmapmergable
	q3map_lightmapsize 512 512
	//q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	q3map_shadeangle 120
}

textures/goldrush/lmterrain_0
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm landmine
	surfaceparm gravelsteps

	{
	    stage diffusemap
		map textures/temperate_sd/sand_bubbles_bright
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_bubbles_bright_n
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
		rgbgen identity
	}
	{
	    stage diffusemap
		map textures/temperate_sd/sand_bubbles_bright
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_bubbles_bright_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_0to1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/temperate_sd/sand_bubbles_bright
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap 
		map textures/temperate_sd/sand_bubbles_bright_n
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
		tcMod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_0to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_bubbles_bright
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_bubbles_bright_n
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
	    tcMod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_tris_sandy_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
		
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_tris_sandy_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
		
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_0to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm gravelsteps
	surfaceparm landmine
	{
	    stage diffusemap
		map textures/temperate_sd/sand_bubbles_bright
        tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_bubbles_bright_n
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
		tcMod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
	    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
	    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		rgbgen identity
		tcmod scale 1.75 1.75
	}

	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_1to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		tcMod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
        map textures/desert_sd/pavement_tris_sandy_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_tris_sandy_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
    }
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_1to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_tris_sandy_n
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_tris_sandy_s
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_tris_sandy_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_tris_sandy_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_2to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_tris_sandy_n
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap 
		map textures/desert_sd/pavement_tris_sandy_s
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/lmterrain_3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm gravelsteps
	surfaceparm landmine
	
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/goldrush/pavement_quad
{
	qer_editorimage textures/desert_sd/pavement_quad_sandy
	{
		map $lightmap
		rgbGen identity
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
}

textures/goldrush/sandygrass_b_phong
{
	qer_editorimage textures/egypt_floor_sd/sandygrass_b
	diffusemap textures/egypt_floor_sd/sandygrass_b
    bumpmap textures/egypt_floor_sd/sandygrass_b_n
	specularmap textures/egypt_floor_sd/sandygrass_b_s
	q3map_nonplanar
	q3map_shadeangle 135
	surfaceparm landmine
	surfaceparm grasssteps
	implicitMap textures/egypt_floor_sd/sandygrass_b
}

textures/goldrush/camp_map
{
	qer_editorimage gfx/loading/camp_map.tga
	surfaceparm woodsteps
	
	implicitMap gfx/loading/camp_map.tga
}

textures/goldrush/canvas_nondeform
{
	qer_editorimage textures/egypt_props_sd/siwa_canvas1
	diffusemap textures/egypt_props_sd/siwa_canvas1
	bumpmap textures/egypt_props_sd/siwa_canvas1_n
	specularmap textures/egypt_props_sd/siwa_canvas1_s
	cull disable
	nofog
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	implicitMap textures/egypt_props_sd/siwa_canvas1
}

//Needs to be there for bridge...

textures/desert_sd/road_dirty_gravel
{
    qer_editorimage  textures/desert_sd/road_dirty_gravel
    
    {
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		rgbGen identity
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		rgbGen identity
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		rgbGen identity
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}