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
	diffusemap textures/temperate_sd/sand_bubbles_bright
	bumpmap textures/temperate_sd/sand_bubbles_bright_n
	specularmap textures/temperate_sd/sand_bubbles_bright_s
	{
		map textures/temperate_sd/sand_bubbles_bright
		rgbgen identity
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
		VertexColor
		
	}
	{
	    stage bumpmap 
		map textures/temperate_sd/sand_bubbles_bright_n
		VertexColor
		blend blend
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	
}

textures/goldrush/lmterrain_0to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_bubbles_bright
		VertexColor
		
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_bubbles_bright_n
		VertexColor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
	    VertexColor
		blend blend
	}
		
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
		
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_tris_sandy_n
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_tris_sandy_s
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
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
        VertexColor
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_bubbles_bright_n
		VertexColor
		blend blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_bubbles_bright_s
		VertexColor
		blend blend
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
	    blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
	    blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	
}

textures/goldrush/lmterrain_1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/pavement_quad_sandy
	bumpmap textures/desert_sd/pavement_quad_sandy_n
	specularmap textures/desert_sd/pavement_quad_sandy_s
	{
	    
		map textures/desert_sd/pavement_quad_sandy
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
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		VertexColor
		blend blend
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		VertexColor
		blend blend
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/pavement_tris_sandy
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
        map textures/desert_sd/pavement_tris_sandy_n
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_tris_sandy_s
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
    }
}

textures/goldrush/lmterrain_1to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	{
	    stage diffusemap
		map textures/desert_sd/pavement_quad_sandy
		VertexColor
		tcmod scale 1.75 1.75
		
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_quad_sandy_n
		VertexColor
		blend blend
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/pavement_quad_sandy_s
		VertexColor
		blend blend
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	

}

textures/goldrush/lmterrain_2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/pavement_tris_sandy
	bumpmap textures/desert_sd/pavement_tris_sandy_n
	specularmap textures/desert_sd/pavement_tris_sandy_s
	{
		map textures/desert_sd/pavement_tris_sandy
		rgbgen identity
		tcmod scale 1.75 1.75
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
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage bumpmap
		map textures/desert_sd/pavement_tris_sandy_n
		VertexColor
		blend blend
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap 
		map textures/desert_sd/pavement_tris_sandy_s
		VertexColor
		blend blend
		tcmod scale 1.75 1.75
	}
	{
	    stage diffusemap
		map textures/desert_sd/road_dirty_gravel
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
		
	}
	{
	    stage bumpmap
		map textures/desert_sd/road_dirty_gravel_n
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	{
	    stage specularmap
		map textures/desert_sd/road_dirty_gravel_s
		blend blend
		VertexColor
		tcmod scale 1.75 1.75
	}
	
	
	
}

textures/goldrush/lmterrain_3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/road_dirty_gravel
	bumpmap textures/desert_sd/road_dirty_gravel_n
	specularmap textures/desert_sd/road_dirty_gravel_s
	surfaceparm gravelsteps
	surfaceparm landmine
	
	{
		map textures/desert_sd/road_dirty_gravel
		rgbgen identity
		tcmod scale 1.75 1.75
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	
}

textures/goldrush/pavement_quad
{
	qer_editorimage textures/desert_sd/pavement_quad_sandy
	diffusemap textures/desert_sd/pavement_quad_sandy
	bumpmap textures/desert_sd/pavement_quad_sandy_n
	specularmap textures/desert_sd/pavement_quad_sandy_s
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/desert_sd/pavement_quad_sandy
		blendFunc filter
		tcmod scale 1.75 1.75
		
		
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
  diffusemap  textures/desert_sd/road_dirty_gravel
  bumpmap  textures/desert_sd/road_dirty_gravel_n
  specularmap  textures/desert_sd/road_dirty_gravel_s
    {
		map $lightmap
		rgbGen identity
	}
	
	{
		map textures/desert_sd/road_dirty_gravel
		blendFunc filter
		
		
	}
}