// snow_sd.shader

textures/snow_sd/alphatree_snow
{
    qer_editorimage textures/snow_sd/alphatree_snow.tga
	diffusemap textures/snow_sd/alphatree_snow.tga
	specularmap textures/snow_sd/alphatree_snow_r.tga
	bumpmap textures/snow_sd/alphatree_snow_n.tga
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
    qer_editorimage textures/snow_sd/alphatree_snow2.tga
	diffusemap textures/snow_sd/alphatree_snow2.tga
	specularmap textures/snow_sd/alphatree_snow2_r.tga
	bumpmap textures/snow_sd/alphatree_snow2_n.tga
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
    qer_editorimage textures/snow_sd/alphatree_snow3.tga
	diffusemap textures/snow_sd/alphatree_snow3.tga
	specularmap textures/snow_sd/alphatree_snow3_r.tga
	bumpmap textures/snow_sd/alphatree_snow3_n.tga
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
    qer_editorimage textures/snow_sd/alphatree_snow4.tga
	diffusemap textures/snow_sd/alphatree_snow4.tga
	specularmap textures/snow_sd/alphatree_snow4_r.tga
	bumpmap textures/snow_sd/alphatree_snow4_n.tga
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
    qer_editorimage textures/snow_sd/alphatreeline_snow.tga
	diffusemap textures/snow_sd/alphatreeline_snow.tga
	specularmap textures/snow_sd/alphatreeline_snow_r.tga
	bumpmap textures/snow_sd/alphatreeline_snow_n.tga
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
    qer_editorimage textures/snow_sd/ametal_m03_snow.tga
	diffusemap textures/snow_sd/ametal_m03_snow.tga
	specularmap textures/snow_sd/ametal_m03_snow_r.tga
	bumpmap textures/snow_sd/ametal_m03_snow_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/ametal_m04a_snow
{
    qer_editorimage textures/snow_sd/ametal_m04a_snow.tga
	diffusemap textures/snow_sd/ametal_m04a_snow.tga
	specularmap textures/snow_sd/ametal_m04a_snow_r.tga
	bumpmap textures/snow_sd/ametal_m04a_snow_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/ametal_m04a_snowfade
{
    qer_editorimage textures/snow_sd/ametal_m04a_snowfade.tga
	diffusemap textures/snow_sd/ametal_m04a_snowfade.tga
	specularmap textures/snow_sd/ametal_m04a_snowfade_r.tga
	bumpmap textures/snow_sd/ametal_m04a_snowfade_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/bunkertrim_snow
{
    diffusemap textures/snow_sd/bunkertrim_snow.tga
	bumpmap textures/snow_sd/bunkertrim_snow_n.tga
	specularmap textures/snow_sd/bunkertrim_snow_r.tga
	qer_editorimage textures/snow_sd/bunkertrim_snow.tga
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
	qer_editorimage textures/snow_sd/bunkertrim3_snow.tga
	diffusemap textures/snow_sd/bunkertrim3_snow.tga
	specularmap textures/snow_sd/bunkertrim3_snow_r.tga
	bumpmap textures/snow_sd/bunkertrim3_snow_n.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/snow_sd/bunkertrim3_snow.tga
		blendFunc filter
	}
}

textures/snow_sd/bunkerwall_lrg02
{
	qer_editorimage textures/snow_sd/bunkerwall_lrg02.tga
	diffusemap textures/snow_sd/bunkerwall_lrg02.tga
	specularmap textures/snow_sd/bunkerwall_lrg02_r.tga
	bumpmap textures/snow_sd/bunkerwall_lrg02_n.tga
	q3map_nonplanar
	q3map_shadeangle 80
	implicitMap -
}

textures/snow_sd/icey_lake
{
    diffusemap textures/snow_sd/icelake.tga
	specularmap textures/snow_sd/icelake_r.tga
	bumpmap textures/snow_sd/icelake_n.tga
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake.tga
	surfaceparm slick
	{
		map textures/effects/envmap_ice.tga
		tcgen environment
	}
	{
		map textures/snow_sd/icelake.tga
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
	qer_editorimage textures/snow_sd/icelake2.tga
	sort seethrough
	surfaceparm slick
	surfaceparm trans
	tesssize 256
	diffusemap textures/snow_sd/icelake2.tga
	bumpmap textures/snow_sd/icelake2_n.tga
	specularmap textures/snow_sd/icelake2_r.tga

	{
		map textures/effects/envmap_ice2.tga
		tcgen environment
		blendfunc blend
	}
	{
		map textures/snow_sd/icelake2.tga
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
	qer_editorimage textures/snow_sd/icelake2.tga
	diffusemap textures/snow_sd/icelake2.tga
	bumpmap textures/snow_sd/icelake2_n.tga
	specularmap textures/snow_sd/icelake2_r.tga
	surfaceparm slick
	tesssize 256

	{
		map textures/effects/envmap_ice2.tga
		tcgen environment
		rgbGen identity
	}
	{
		map textures/snow_sd/icelake2.tga
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
    qer_editorimage textures/snow_sd/mesh_c03_snow.tga
	diffusemap textures/snow_sd/mesh_c03_snow.tga
	specularmap textures/snow_sd/mesh_c03_snow_r.tga
	bumpmap textures/snow_sd/mesh_c03_snow_n.tga
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
    qer_editorimage textures/snow_sd/metal_m04g2_snow.tga
	diffusemap textures/snow_sd/metal_m04g2_snow.tga
	specularmap textures/snow_sd/metal_m04g2_snow_r.tga
	bumpmap textures/snow_sd/metal_m04g2_snow_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/mroof_snow
{
    qer_editorimage textures/snow_sd/mroof_snow.tga
	diffusemap textures/snow_sd/mroof_snow.tga
	bumpmap textures/snow_sd/mroof_snow_n.tga
	specularmap textures/snow_sd/mroof_snow_r.tga
	surfaceparm roofsteps
	implicitMap -
}

textures/snow_sd/sub1_snow
{
    qer_Editorimage textures/snow_sd/sub1_snow.tga
	diffusemap textures/snow_sd/sub1_snow.tga
	specularmap textures/snow_sd/sub1_snow_r.tga
	bumpmap textures/snow_sd/sub1_snow_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/sub1_snow2
{
    qer_editorimage textures/snow_sd/sub1_snow2.tga
	diffusemap textures/snow_sd/sub1_snow2.tga
	specularmap textures/snow_sd/sub1_snow2_r.tga
	bumpmap textures/snow_sd/sub1_snow2_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/wirefence01_snow
{
    qer_editorimage textures/snow_sd/wirefence01_snow.tga
	diffusemap textures/snow_sd/wirefence01_snow.tga
	specularmap textures/snow_sd/wirefence01_snow_r.tga
	bumpmap textures/snow_sd/wirefence01_snow_n.tga
	cull none
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	implicitMask -
}

textures/snow_sd/wood_m05a_snow
{
    qer_editorimage textures/snow_sd/wood_m05a_snow.tga
	diffusemap textures/snow_sd/wood_m05a_snow.tga
	specularmap textures/snow_sd/wood_m05a_snow_r.tga
	bumpmap textures/snow_sd/wood_m05a_snow_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/snow_sd/wood_m06b_snow
{
    qer_editorimage textures/snow_sd/wood_m06b_snow.tga
	diffusemap textures/snow_sd/wood_m06b_snow.tga
	specularmap textures/snow_sd/wood_m06b_snow_r.tga
	bumpmap textures/snow_sd/wood_m06b_snow_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/snow_sd/fuse_box_snow
{
    qer_editorimage textures/snow_sd/fuse_box_snow.tga
	diffusemap textures/snow_sd/fuse_box_snow.tga
	specularmap textures/snow_sd/fuse_box_snow_r.tga
	bumpmap textures/snow_sd/fuse_box_snow_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/xmetal_m02_snow
{
    qer_editorimage textures/snow_sd/xmetal_m02_snow.tga
	diffusemap textures/snow_sd/xmetal_m02_snow.tga
	specularmap textures/snow_sd/xmetal_m02_snow_r.tga
	bumpmap textures/snow_sd/xmetal_m02_snow_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow_sd/xmetal_m02t_snow
{
    qer_editorimage textures/snow_sd/xmetal_m02t_snow.tga
	diffusemap textures/snow_sd/xmetal_m02t_snow.tga
	specularmap textures/snow_sd/xmetal_m02t_snow_r.tga
	bumpmap textures/snow_sd/xmetal_m02t_snow_n.tga
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
	qer_editorimage textures/snow_sd/snow_track03.tga
	//diffusemap textures/snow_sd/snow_track03.tga
	//specularmap textures/snow_sd/snow_track03_r.tga
	//bumpmap textures/snow_sd/snow_track03_n.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03.tga
       	blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}

textures/snow_sd/snow_track03_faint
{ 
	qer_editorimage textures/snow_sd/snow_track03.tga
	//diffusemap textures/snow_sd/snow_track03.tga
	//specularmap textures/snow_sd/snow_track03_r.tga
	//bumpmap textures/snow_sd/snow_track03_n.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03.tga
       		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen const ( 0.5 0.5 0.5 )
	}
}

textures/snow_sd/snow_track03_end 
{ 
	qer_editorimage textures/snow_sd/snow_track03_end.tga
	//diffusemap textures/snow_sd/snow_track03_end.tga
	//specularmap textures/snow_sd/snow_track03_end_r.tga
	//bumpmap textures/snow_sd/snow_track03_end_n.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03_end.tga
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}

textures/snow_sd/snow_track03_end_faint 
{ 
	qer_editorimage textures/snow_sd/snow_track03_end.tga
	//diffusemap textures/snow_sd/snow_track03_end.tga
	//specularmap textures/snow_sd/snow_track03_end_r.tga
	//bumpmap textures/snow_sd/snow_track03_end_n.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	
	sort decal
	
	{
		map textures/snow_sd/snow_track03_end.tga
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen const ( 0.5 0.5 0.5 )
	}
}

textures/snow_sd/river_edge_snowy 
{ 
	qer_editorimage textures/snow_sd/river_edge_snowy.tga
	//diffusemap textures/snow_sd/river_edge_snowy.tga
	//specularmap textures/snow_sd/river_edge_snowy_r.tga
	//bumpmap textures/snow_sd/river_edge_snowy_n.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	{
		map textures/snow_sd/river_edge_snowy.tga
        blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen identity
	}
}
