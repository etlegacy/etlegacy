// snow.shader

textures/snow/alpha_ice2
{		
    qer_editorimage textures/snow/alpha_ice2s
	surfaceparm alphashadow
	surfaceparm trans
	cull none
	{
	    stage diffusemap
		map textures/snow/alpha_ice2s.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
	{
	   stage bumpmap
	   map textures/snow/alpha_ice2s_n.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
	{
	   stage specularmap
	   map textures/snow/alpha_ice2s_s.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

textures/snow/s_bars_m01
{
    qer_editorimage textures/snow/s_bars_m01.tga
	diffusemap textures/snow/s_bars_m01.tga
	bumpmap textures/snow/s_bars_m01_n.tga
	specularmap textures/snow/s_bars_m01_s.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm trans
	implicitMask -
}

textures/snow/s_cashudder_c01
{
    qer_editorimage textures/snow/s_cashudder_c01.tga
	diffusemap textures/snow/s_cashudder_c01.tga
	bumpmap textures/snow/s_cashudder_c01_n.tga
	specularmap textures/snow/s_cashudder_c01_s.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/snow/s_castle_m03a_step
{
    qer_editorimage textures/snow/s_castle_m03a_step.tga
	diffusemap textures/snow/s_castle_m03a_step.tga
	bumpmap textures/snow/s_castle_m03a_step_n.tga
	specularmap textures/snow/s_castle_m03a_step_s.tga
	surfaceparm snowsteps
	implicitMap -
}

textures/snow/s_castle_c02
{
    diffusemap textures/snow/s_castle_c02.tga
	bumpmap textures/snow/s_castle_c02_n.tga
	specularmap textures/snow/s_castle_c02_s.tga
    implicitMap -
}

textures/snow/s_castle_c16d
{
    qer_editorimage textures/snow/s_castle_c16d.tga
	diffusemap textures/snow/s_castle_c16d.tga
	bumpmap textures/snow/s_castle_c16d_n.tga
	specularMap textures/snow/s_castle_c16d_s.tga
}

textures/snow/s_castle_c46_on_grid
{
    qer_editorimage textures/snow/s_castle_c46_on_grid.tga
    diffusemap textures/snow/s_castle_c46_on_grid.tga
    bumpmap textures/snow/s_castle_c46_on_grid_n.tga
	specularmap textures/snow/s_castle_c46_on_grid_s.tga
}

textures/snow/s_castle_m03a_step
{
    qer_editorimage textures/snow/s_castle_m03a_step.tga
	diffusemap textures/snow/s_castle_m03a_step.tga
	bumpmap textures/snow/s_castle_m03a_step_n.tga
	specularmap textures/snow/s_castle_m03a_step_s.tga
}

textures/snow/s_cathedrale_c06
{   
    qer_editorimage textures/snow/s_cathedrale_c06.tga
    diffusemap textures/snow/s_cathedrale_c06.tga
	bumpmap textures/snow/s_cathedrale_c06_n.tga
	specularmap textures/snow/s_cathedrale_c06_s.tga
}

textures/snow/s_cathedrale_c24b
{
    qer_editorimage textures/snow/s_cathedrale_c24b.tga
	diffusemap textures/snow/s_cathedrale_c24b.tga
	bumpmap textures/snow/s_cathedrale_c24b_n.tga
	specularmap textures/snow/s_cathedrale_c24b_s.tga
}


textures/snow/s_cwood_mo5c
{
    qer_editorimage textures/snow/s_cwood_mo5c.tga
	diffusemap textures/snow/s_cwood_mo5c.tga
	specularmap textures/snow/s_cwood_mo5c_s.tga
	bumpmap textures/snow/s_cwood_mo5c_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/snow/s_diamond_c01a
{
    qer_editorimage textures/snow/s_diamond_c01a.tga
	diffusemap textures/snow/s_diamond_c01a.tga
	specularmap textures/snow/s_diamond_c01a_s.tga
	bumpmap textures/snow/s_diamond_c01a_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/snow/s_dirt_m03i_2
{
    qer_editorimage textures/snow/s_dirt_m03i_2.tga
	diffusemap textures/snow/s_dirt_m03i_2.tga
	specularmap textures/snow/s_dirt_m03i_2_s.tga
	bumpmap textures/snow/s_dirt_m03i_2_n.tga
	surfaceparm snowsteps
	implicitMap -
}

textures/snow/s_fence_c07
{
    qer_editorimage textures/snow/s_fence_c07.tga
	diffusemap textures/snow/s_fence_c07.tga
	specularmap textures/snow/s_fence_c07_s.tga
	bumpmap textures/snow/s_fence_c07_n.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm trans
	implicitMask -
}

textures/snow/s_floor_c10_a2
{
    qer_editorimage textures/snow/s_floor_c10_a2.tga
	diffusemap textures/snow/s_floor_c10_a2.tga
	bumpmap textures/snow/s_floor_c10_a2_n.tga
	specularmap textures/snow/s_floor_c10_a2_s.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/snow/s_grass_ml03b
{
    qer_editorimage textures/snow/s_grass_ml03b.tga
	diffusemap textures/snow/s_grass_ml03b.tga
	bumpmap textures/snow/s_grass_ml03b_n.tga
	specularmap textures/snow/s_grass_ml03b_s.tga
	surfaceparm snowsteps
	implicitMap -
}

textures/snow/s_wood_c13a
{
    qer_editorimage textures/snow/s_wood_c13a.tga
	diffusemap textures/snow/s_wood_c13a.tga
	specularmap textures/snow/s_wood_c13a_s.tga
	bumpmap textures/snow/s_wood_c13a_n.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/snow/s_door_c10b_s
{
    qer_editorimage textures/snow/s_door_c10b_s
	diffusemap textures/snow/s_door_c10b_s
	bumpmap textures/snow/s_door_c10b_s_n
	specularmap textures/snow/s_door_c10b_s_s
	surfaceparm woodsteps
	implicitMap -
}

textures/snow/s_metal_m04dg2
{
    qer_editorimage textures/snow/s_metal_m04dg2.tga
	diffusemap textures/snow/s_metal_m04dg2.tga
	specularmap textures/snow/s_metal_m04dg2_s.tga
	bumpmap textures/snow/s_metal_m04dg2_n.tga
}

textures/snow/s_roof_c04dm
{
   qer_editorimage textures/snow/s_roof_c04dm.tga
   diffusemap textures/snow/s_roof_c04dm.tga
   specularmap textures/snow/s_roof_c04dm_s.tga
   bumpmap textures/snow/s_roof_c04dm_n.tga
   surfaceparm woodsteps
	implicitMap -
}

textures/snow/s_town_c91
{
    qer_editorimage textures/snow/s_town_c91.tga
	diffusemap textures/snow/s_town_c91.tga
	specularmap textures/snow/s_town_c91_s.tga
	bumpmap textures/snow/s_town_c91_n.tga
	implicitMap -
}

textures/snow/s_town_m_c01_trim
{
    qer_editorimage textures/snow/s_town_m_c01_trim.tga
	diffusemap textures/snow/s_town_m_c01_trim.tga
	specularmap textures/snow/s_town_m_c01_trim_s.tga
	bumpmap textures/snow/s_town_m_c01_trim_n.tga
	implicitMap -
}

textures/snow/s_window_c05a
{
    qer_editorimage textures/snow/s_window_c05a.tga
	diffusemap textures/snow/s_window_c05a.tga
	specularmap textures/snow/s_window_c05a_s.tga
	bumpmap textures/snow/s_window_c05a_n.tga
	implicitMap -
}


textures/snow/s_wood_c13a
{
    qer_editorimage textures/snow/s_wood_c13a.tga
	diffusemap textures/snow/s_wood_c13a.tga
	specularmap textures/snow/s_wood_c13a_s.tga
	bumpmap textures/snow/s_wood_c13a_n.tga
	surfaceparm woodsteps
	implicitMap -
}