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

	diffusemap textures/temperate_sd/sand_bubbles_bright.tga
	bumpmap textures/temperate_sd/sand_bubbles_bright_n.tga
	specularmap textures/temperate_sd/sand_bubbles_bright_r.tga
	{
	    stage diffuseMap
		map textures/temperate_sd/sand_bubbles_bright.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/temperate_sd/sand_bubbles_bright_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/temperate_sd/sand_bubbles_bright_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_0to1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/temperate_sd/sand_bubbles_bright.tga
	bumpmap textures/temperate_sd/sand_bubbles_bright_n.tga
	specularmap textures/temperate_sd/sand_bubbles_bright_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/pavement_quad_sandy_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_0to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/temperate_sd/sand_bubbles_bright.tga
	bumpmap textures/temperate_sd/sand_bubbles_bright_n.tga
	specularmap textures/temperate_sd/sand_bubbles_bright_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_tris_sandy_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/pavement_tris_sandy_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_0to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm gravelsteps
	surfaceparm landmine
	diffusemap textures/temperate_sd/sand_bubbles_bright.tga
	bumpmap textures/temperate_sd/sand_bubbles_bright_n.tga
	specularmap textures/temperate_sd/sand_bubbles_bright_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/road_dirty_gravel_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_1
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/pavement_quad_sandy.tga
	bumpmap textures/desert_sd/pavement_quad_sandy_n.tga
	specularmap textures/desert_sd/pavement_quad_sandy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/pavement_quad_sandy_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_1to2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/pavement_quad_sandy.tga
	bumpmap textures/desert_sd/pavement_quad_sandy_n.tga
	specularmap textures/desert_sd/pavement_quad_sandy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
        map textures/desert_sd/pavement_tris_sandy_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/pavement_tris_sandy_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
    }
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_1to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffuseMap textures/desert_sd/pavement_quad_sandy.tga
	bumpmap textures/desert_sd/pavement_quad_sandy_n.tga
	specularmap textures/desert_sd/pavement_quad_sandy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/road_dirty_gravel_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_2
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/pavement_tris_sandy.tga
	bumpmap textures/desert_sd/pavement_tris_sandy_n.tga
	specularmap textures/desert_sd/pavement_tris_sandy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_tris_sandy.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_tris_sandy_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/pavement_tris_sandy_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_2to3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	diffusemap textures/desert_sd/pavement_tris_sandy.tga
	bumpmap textures/desert_sd/pavement_tris_sandy_n.tga
	specularmap textures/desert_sd/pavement_tris_sandy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/road_dirty_gravel_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/lmterrain_3
{
	q3map_baseshader textures/goldrush/lmterrain_base
	surfaceparm gravelsteps
	surfaceparm landmine
	diffusemap textures/desert_sd/road_dirty_gravel.tga
	bumpmap textures/desert_sd/road_dirty_gravel_n.tga
	specularmap textures/desert_sd/road_dirty_gravel_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/road_dirty_gravel_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/pavement_quad
{
	qer_editorimage textures/desert_sd/pavement_quad_sandy
	diffusemap textures/desert_sd/pavement_quad_sandy.tga
	bumpmap textures/desert_sd/pavement_quad_sandy_n.tga
	specularmap textures/desert_sd/pavement_quad_sandy_r.tga
	{
	    stage diffuseMap
		map textures/desert_sd/pavement_quad_sandy.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage bumpMap
		map textures/desert_sd/pavement_quad_sandy_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
	    stage specularMap
		map textures/desert_sd/pavement_quad_sandy_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.75 1.75
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/goldrush/sandygrass_b_phong
{
	qer_editorimage textures/egypt_floor_sd/sandygrass_b.tga
	q3map_nonplanar
	q3map_shadeangle 135
	surfaceparm landmine
	surfaceparm grasssteps
	diffuseMap textures/egypt_floor_sd/sandygrass_b.tga
    bumpMap textures/egypt_floor_sd/sandygrass_b_n.tga
	specularMap textures/egypt_floor_sd/sandygrass_b_r.tga
	{
		map $lightmap
		blendFunc filter
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
	cull disable
	nofog
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	diffuseMap textures/egypt_props_sd/siwa_canvas1.tga
	bumpMap textures/egypt_props_sd/siwa_canvas1_n.tga
	specularMap textures/egypt_props_sd/siwa_canvas1_r.tga
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
	    stage specularMap
		map textures/desert_sd/road_dirty_gravel_r.tga
		rgbGen identity
		tcmod scale 1.2 1.2
		alphaGen vertex
	}
	{
	    stage diffuseMap
		map textures/desert_sd/road_dirty_gravel.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.2 1.2
		alphaGen vertex
	}
	{
	    stage bumpMap
		map textures/desert_sd/road_dirty_gravel_n.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.2 1.2
	}
	{
	    stage specularMap
		map textures/desert_sd/road_dirty_gravel_r.tga
		blendFunc blend
		alphaGen vertex
		tcMod scale 1.2 1.2
	}
	{
		map $lightmap
		blendFunc filter
		rgbgen identity
	}
}