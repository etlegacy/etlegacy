//======================================================================
// snow_sd.shader
// Last edit: 20/04/03 Sock
//
//======================================================================
textures/snow_sd/alphatree_snow
{
    qer_editorimage textures/snow_sd/alphatree_snow
	diffusemap textures/snow_sd/alphatree_snow
	specularmap textures/snow_sd/alphatree_snow_s
	bumpmap textures/snow_sd/alphatree_snow_n
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatree_snow2
{
    qer_editorimage textures/snow_sd/alphatree_snow2
	diffusemap textures/snow_sd/alphatree_snow2
	specularmap textures/snow_sd/alphatree_snow2_s
	bumpmap textures/snow_sd/alphatree_snow2_n
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatree_snow3
{
    qer_editorimage textures/snow_sd/alphatree_snow3
	diffusemap textures/snow_sd/alphatree_snow3
	specularmap textures/snow_sd/alphatree_snow3_s
	bumpmap textures/snow_sd/alphatree_snow3_n
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatree_snow4
{
    qer_editorimage textures/snow_sd/alphatree_snow4
	diffusemap textures/snow_sd/alphatree_snow4
	specularmap textures/snow_sd/alphatree_snow4_s
	bumpmap textures/snow_sd/alphatree_snow4_n
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/alphatreeline_snow
{
    qer_editorimage textures/snow_sd/alphatreeline_snow
	diffusemap textures/snow_sd/alphatreeline_snow
	specularmap textures/snow_sd/alphatreeline_snow_s
	bumpmap textures/snow_sd/alphatreeline_snow_n
	qer_alphafunc gequal 0.5
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/ametal_m03_snow
{
    qer_editorimage textures/snow_sd/ametal_m03_snow
	diffusemap textures/snow_sd/ametal_m03_snow
	specularmap textures/snow_sd/ametal_m03_snow_s
	bumpmap textures/snow_sd/ametal_m03_snow_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/ametal_m04a_snow
{
    qer_editorimage textures/snow_sd/ametal_m04a_snow
	diffusemap textures/snow_sd/ametal_m04a_snow
	specularmap textures/snow_sd/ametal_m04a_snow_s
	bumpmap textures/snow_sd/ametal_m04a_snow_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/ametal_m04a_snowfade
{
    qer_editorimage textures/snow_sd/ametal_m04a_snowfade
	diffusemap textures/snow_sd/ametal_m04a_snowfade
	specularmap textures/snow_sd/ametal_m04a_snowfade_s
	bumpmap textures/snow_sd/ametal_m04a_snowfade_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/bunkertrim_snow
{
    diffusemap textures/snow_sd/bunkertrim_snow
	bumpmap textures/snow_sd/bunkertrim_snow_n
	specularmap textures/snow_sd/bunkertrim_snow_s
	qer_editorimage textures/snow_sd/bunkertrim_snow
	q3map_nonplanar
	q3map_shadeangle 160
	implicitMap -
}

//==========================================================================
// Corner/edges of axis fueldump bunker buildings, needs phong goodness.
//==========================================================================
textures/snow_sd/bunkertrim3_snow
{
	q3map_nonplanar
	q3map_shadeangle 179
	qer_editorimage textures/snow_sd/bunkertrim3_snow
	diffusemap textures/snow_sd/bunkertrim3_snow
	specularmap textures/snow_sd/bunkertrim3_snow_s
	bumpmap textures/snow_sd/bunkertrim3_snow_n
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/snow_sd/bunkertrim3_snow
		blendFunc filter
	}
}

textures/snow_sd/bunkerwall_lrg02
{
	qer_editorimage textures/snow_sd/bunkerwall_lrg02
	diffusemap textures/snow_sd/bunkerwall_lrg02
	specularmap textures/snow_sd/bunkerwall_lrg02_s
	bumpmap textures/snow_sd/bunkerwall_lrg02_n
	q3map_nonplanar
	q3map_shadeangle 80
	implicitMap -
}

textures/snow_sd/icey_lake
{
    diffusemap textures/snow_sd/icelake
	specularmap textures/snow_sd/icelake_s
	bumpmap textures/snow_sd/icelake_n
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake
	surfaceparm slick
	{
		map textures/effects/envmap_ice
		tcgen environment
	}
	{
		map textures/snow_sd/icelake
		blendfunc blend
		tcmod scale 0.35 0.35
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
	}
}

// Used in fueldump on the icy river.
// Note: Apply this at a scale of 2.0x2.0 so it aligns correctly
// ydnar: added depthwrite and sort key so it dlights correctly
textures/snow_sd/icelake2
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake2
	sort seethrough
	surfaceparm slick
	surfaceparm trans
	tesssize 256
	diffusemap textures/snow_sd/icelake2
	bumpmap textures/snow_sd/icelake2_n
	specularmap textures/snow_sd/icelake2_s

	{
		map textures/effects/envmap_ice2
		tcgen environment
		blendfunc blend
	}
	{
		map textures/snow_sd/icelake2
		blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
		depthWrite
	}
	
}

// Note: Apply this at a scale of 2.0x2.0 so it aligns correctly
textures/snow_sd/icelake2_opaque
{
	qer_editorimage textures/snow_sd/icelake2
	diffusemap textures/snow_sd/icelake2
	bumpmap textures/snow_sd/icelake2_n
	specularmap textures/snow_sd/icelake2_s
	surfaceparm slick
	tesssize 256

	{
		map textures/effects/envmap_ice2
		tcgen environment
		rgbGen identity
	}
	{
		map textures/snow_sd/icelake2
		blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
	}
	
}

textures/snow_sd/mesh_c03_snow
{
    qer_editorimage textures/snow_sd/mesh_c03_snow
	diffusemap textures/snow_sd/mesh_c03_snow
	specularmap textures/snow_sd/mesh_c03_snow_s
	bumpmap textures/snow_sd/mesh_c03_snow_n
	qer_alphafunc gequal 0.5
	cull none
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/metal_m04g2_snow
{
    qer_editorimage textures/snow_sd/metal_m04g2_snow
	diffusemap textures/snow_sd/metal_m04g2_snow
	specularmap textures/snow_sd/metal_m04g2_snow_s
	bumpmap textures/snow_sd/metal_m04g2_snow_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/mroof_snow
{
    qer_editorimage textures/snow_sd/mroof_snow
	diffusemap textures/snow_sd/mroof_snow
	bumpmap textures/snow_sd/mroof_snow_n
	specularmap textures/snow_sd/mroof_snow_s
	surfaceparm roofsteps
	implicitMap -
}

textures/snow_sd/sub1_snow
{
    qer_Editorimage textures/snow_sd/sub1_snow
	diffusemap textures/snow_sd/sub1_snow
	specularmap textures/snow_sd/sub1_snow_s
	bumpmap textures/snow_sd/sub1_snow_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/sub1_snow2
{
    qer_editorimage textures/snow_sd/sub1_snow2
	diffusemap textures/snow_sd/sub1_snow2
	specularmap textures/snow_sd/sub1_snow2_s
	bumpmap textures/snow_sd/sub1_snow2_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/wirefence01_snow
{
    qer_editorimage textures/snow_sd/wirefence01_snow
	diffusemap textures/snow_sd/wirefence01_snow
	specularmap textures/snow_sd/wirefence01_snow_s
	bumpmap textures/snow_sd/wirefence01_snow_n
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/wood_m05a_snow
{
    qer_editorimage textures/snow_sd/wood_m05a_snow
	diffusemap textures/snow_sd/wood_m05a_snow
	specularmap textures/snow_sd/wood_m05a_snow_s
	bumpmap textures/snow_sd/wood_m05a_snow_n
	surfaceparm woodsteps
	implicitMap -
}

textures/snow_sd/wood_m06b_snow
{
    qer_editorimage textures/snow_sd/wood_m06b_snow
	diffusemap textures/snow_sd/wood_m06b_snow
	specularmap textures/snow_sd/wood_m06b_snow_s
	bumpmap textures/snow_sd/wood_m06b_snow_n
	surfaceparm woodsteps
	implicitMap -
}

textures/snow_sd/fuse_box_snow
{
    qer_editorimage textures/snow_sd/fuse_box_snow
	diffusemap textures/snow_sd/fuse_box_snow
	specularmap textures/snow_sd/fuse_box_snow_s
	bumpmap textures/snow_sd/fuse_box_snow_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/xmetal_m02_snow
{
    qer_editorimage textures/snow_sd/xmetal_m02_snow
	diffusemap textures/snow_sd/xmetal_m02_snow
	specularmap textures/snow_sd/xmetal_m02_snow_s
	bumpmap textures/snow_sd/xmetal_m02_snow_n
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/xmetal_m02t_snow
{
    qer_editorimage textures/snow_sd/xmetal_m02t_snow
	diffusemap textures/snow_sd/xmetal_m02t_snow
	specularmap textures/snow_sd/xmetal_m02t_snow_s
	bumpmap textures/snow_sd/xmetal_m02t_snow_n
	surfaceparm metalsteps
	implicitMap -
}

//==========================================================================
// Various terrain decals textures
//==========================================================================

// ydnar: added "sort banner" and changed blendFunc so they fog correctly
// note: the textures were inverted and Brightness/Contrast applied so they
// will work properly with the new blendFunc (this is REQUIRED!)

textures/snow_sd/snow_track03 
{ 
	qer_editorimage textures/snow_sd/snow_track03
	//diffusemap textures/snow_sd/snow_track03
	//specularmap textures/snow_sd/snow_track03_s
	//bumpmap textures/snow_sd/snow_track03_n
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03
       	blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}

textures/snow_sd/snow_track03_faint
{ 
	qer_editorimage textures/snow_sd/snow_track03
	//diffusemap textures/snow_sd/snow_track03
	//specularmap textures/snow_sd/snow_track03_s
	//bumpmap textures/snow_sd/snow_track03_n
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03
       		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen const ( 0.5 0.5 0.5 )
	}
}

textures/snow_sd/snow_track03_end 
{ 
	qer_editorimage textures/snow_sd/snow_track03_end
	//diffusemap textures/snow_sd/snow_track03_end
	//specularmap textures/snow_sd/snow_track03_end_s
	//bumpmap textures/snow_sd/snow_track03_end_n
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03_end
        	blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}

textures/snow_sd/snow_track03_end_faint 
{ 
	qer_editorimage textures/snow_sd/snow_track03_end
	//diffusemap textures/snow_sd/snow_track03_end
	//specularmap textures/snow_sd/snow_track03_end_s
	//bumpmap textures/snow_sd/snow_track03_end_n
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03_end
        	blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen const ( 0.5 0.5 0.5 )
	}
}

textures/snow_sd/river_edge_snowy 
{ 
	qer_editorimage textures/snow_sd/river_edge_snowy
	//diffusemap textures/snow_sd/river_edge_snowy
	//specularmap textures/snow_sd/river_edge_snowy_s
	//bumpmap textures/snow_sd/river_edge_snowy_n
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	{
		map textures/snow_sd/river_edge_snowy
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}
