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
	    stage diffuseMap
		map textures/temperate_sd/sand_bubbles_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/temperate_sd/sand_bubbles_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/temperate_sd/sand_bubbles_bright.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/temperate_sd/sand_bubbles_bright_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_0to1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffuseMap
		map textures/temperate_sd/sand_bubbles_bright.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap 
		map textures/temperate_sd/sand_bubbles_bright_n.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_0to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	
	{
	    stage diffuseMap
		map textures/temperate_sd/sand_bubbles_bright.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/temperate_sd/sand_bubbles_bright_n.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_tris_sandy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_0to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm gravelsteps
	surfaceparm landmine
	{
	    stage diffuseMap
		map textures/temperate_sd/sand_bubbles_bright.tga
        tcMod scale 1.75 1.75
        alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/temperate_sd/sand_bubbles_bright_n.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
	    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		rgbgen identity
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		rgbgen identity
		tcmod scale 1.75 1.75
		alphaGen vertex
	}

	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
	    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_1to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		tcMod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
        map textures/desert_sd/pavement_tris_sandy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_1to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		rgbgen identity
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_tris_sandy_n.tga
		rgbgen identity
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_tris_sandy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_2to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_tris_sandy_n.tga
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/lmterrain_3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm gravelsteps
	surfaceparm landmine
	
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		rgbgen identity
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		rgbgen identity
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/pavement_quad
{
	qer_editorimage textures/desert_sd/pavement_quad_sandy
	
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		tcmod scale 1.75 1.75
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/sandygrass_b_phong
{
	qer_editorimage textures/egypt_floor_sd/sandygrass_b.tga
	q3map_nonplanar
	q3map_shadeangle 135
	
	diffuseMap textures/egypt_floor_sd/sandygrass_b.tga
    bumpMap textures/egypt_floor_sd/sandygrass_b_n.tga
	
	surfaceparm landmine
	surfaceparm grasssteps
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/goldrush/camp_map
{
	qer_editorimage gfx/loading/camp_map.tga
	surfaceparm woodsteps
	
	implicitMap gfx/loading/camp_map.tga
}

textures/goldrush/canvas_nondeform
{
	qer_editorimage textures/egypt_props_sd/siwa_canvas1.tga
	
	diffuseMap textures/egypt_props_sd/siwa_canvas1.tga
	bumpMap textures/egypt_props_sd/siwa_canvas1_n.tga
	cull disable
	nofog
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	implicitMap textures/egypt_props_sd/siwa_canvas1.tga
}

//Needs to be there for bridge...
//adjusted tcmod scale to fit better
textures/desert_sd/road_dirty_gravel
{
    qer_editorimage  textures/desert_sd/road_dirty_gravel.tga
    
    {
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		rgbGen identity
		tcmod scale 1.2 1.2
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		rgbGen identity
		tcmod scale 1.2 1.2
		alphaGen vertex
	}
	
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.2 1.2
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		tcMod scale 1.2 1.2
	}
	
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}