//======================================================================
// Fueldump.shader
// Last edit: 04.03.2017 Thunder
//
//======================================================================
// q3map_sun <red> <green> <blue> <intensity> <degrees> <elevation>


textures/fueldump/fueldumpsky
{
	qer_editorimage textures/skies/fueldump_clouds.tga
	q3map_lightrgb 0.8 0.9 1.0
	q3map_skylight 85 3
	q3map_sun 1 .95 .9 200 210 28
	skyparms - 200 -
	surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	{
		map textures/skies/fueldump_clouds.tga
		rgbGen identity
	}
	{
		map textures/skies/fueldump_clouds.tga
		blendfunc blend
		rgbGen identity
		tcMod scroll 0.0005 0.00
		tcMod scale 2 1
	}
}



//======================================================================
// Base for metashaders
//======================================================================
textures/fueldump/terrain_base
{
	q3map_normalimage textures/sd_bumpmaps/normalmap_terrain
	q3map_lightmapMergable
	q3map_lightmapaxis z
	q3map_lightmapsize 512 512
	//q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_tcMod rotate 37
	q3map_tcMod scale 1 1
	surfaceparm landmine
	surfaceparm snowsteps
}

//======================================================================
// Metashader for both sides of the map
// _0: Slighty dirty snow
// _1: Mud/dirt snow
// _2: Clean white snow
// _3: Snow crispy rock
//======================================================================
textures/fueldump/terrain1_0
{
	q3map_baseshader textures/fueldump/terrain_base
	qer_editorimage textures/stone/mxsnow2.tga
	// TODO: it seems stupid to blend a texture to the same texture.
	// But we don't do it, this material's specular is incorrect.
	diffusemap textures/stone/mxsnow2.tga
	bumpmap textures/stone/mxsnow2_n.tga
	specularmap textures/stone/mxsnow2_r.tga
	{
		stage diffusemap
		map textures/stone/mxsnow2.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow2_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxsnow2_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_1
{
	q3map_baseshader textures/fueldump/terrain_base
    qer_editorimage textures/snow_sd/mxrock4b_snow.tga
	diffusemap textures/snow_sd/mxrock4b_snow.tga
	bumpmap textures/snow_sd/mxrock4b_snow_n.tga
	specularmap textures/snow_sd/mxrock4b_snow_r.tga
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_2
{
	q3map_baseshader textures/fueldump/terrain_base
    qer_editorimage textures/stone/mxsnow3.tga
	diffusemap textures/stone/mxsnow3.tga
	bumpmap textures/stone/mxsnow3_n.tga
	specularmap textures/stone/mxsnow3_r.tga
	{
		stage diffusemap
		map textures/stone/mxsnow3.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow3_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxsnow3_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_3
{
	q3map_baseshader textures/fueldump/terrain_base
    qer_editorimage textures/stone/mxrock3h_snow.tga
	diffusemap textures/stone/mxrock3h_snow.tga
	bumpmap textures/stone/mxrock3h_snow_n.tga
	specularmap textures/stone/mxrock3h_snow_r.tga
	{
		stage diffusemap
		map textures/stone/mxrock3h_snow.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_0to1
{
	q3map_baseshader textures/fueldump/terrain_base
	qer_editorimage textures/stone/mxsnow2.tga
	diffusemap textures/stone/mxsnow2.tga
	bumpmap textures/stone/mxsnow2_n.tga
	specularmap textures/stone/mxsnow2_r.tga
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/snow_sd/mxrock4b_snow_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_0to2
{
	q3map_baseshader textures/fueldump/terrain_base
    qer_editorimage textures/stone/mxsnow2.tga
	diffusemap textures/stone/mxsnow2.tga
	bumpmap textures/stone/mxsnow2_n.tga
	specularmap textures/stone/mxsnow2_r.tga
	{
		stage diffusemap
		map textures/stone/mxsnow3.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow3_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxsnow3_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_0to3
{
	q3map_baseshader textures/fueldump/terrain_base
	qer_editorimage textures/stone/mxsnow2.tga
	diffusemap textures/stone/mxsnow2.tga
	bumpmap textures/stone/mxsnow2_n.tga
	specularmap textures/stone/mxsnow2_r.tga
	{
	    stage diffusemap
		map textures/stone/mxrock3h_snow.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_1to2
{
	q3map_baseshader textures/fueldump/terrain_base
    qer_editorimage textures/snow_sd/mxrock4b_snow.tga
	diffusemap textures/snow_sd/mxrock4b_snow.tga
	bumpmap textures/snow_sd/mxrock4b_snow_n.tga
	specularmap textures/snow_sd/mxrock4b_snow_r.tga
	{
	    stage diffusemap
		map textures/stone/mxsnow3.tga
		blendFunc blend
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow3_n.tga
		blendFunc blend
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage specularmap
		map textures/stone/mxsnow3_r.tga
		blendFunc blend
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_1to3
{
	q3map_baseshader textures/fueldump/terrain_base
	qer_editorimage textures/snow_sd/mxrock4b_snow.tga
	diffusemap textures/snow_sd/mxrock4b_snow.tga
	bumpmap textures/snow_sd/mxrock4b_snow_n.tga
	specularmap textures/snow_sd/mxrock4b_snow_r.tga
	{
	    stage diffusemap
		map textures/stone/mxrock3h_snow.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/fueldump/terrain1_2to3
{
	q3map_baseshader textures/fueldump/terrain_base
	qer_editorimage textures/stone/mxsnow3.tga
	diffusemap textures/stone/mxsnow3.tga
	bumpmap textures/stone/mxsnow3_n.tga
	specularmap textures/stone/mxsnow3_r.tga
	{
	    stage diffusemap
		map textures/stone/mxrock3h_snow.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow_r.tga
		blendFunc blend
		alphaGen vertex
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

//===========================================================================
// Floor and wall textures for the cave system + hong phong goodness
//===========================================================================
textures/fueldump/cave_dark
{
	qer_editorimage textures/stone/mxrock3_a.tga
	q3map_nonplanar
	q3map_shadeangle 60
	diffusemap textures/stone/mxrock3_a.tga
	bumpmap textures/stone/mxrock3_a_n.tga
	specularmap textures/stone/mxrock3_a_r.tga
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/fueldump/cave_floor
{
	qer_editorimage textures/stone/mxrock1aa.tga
	q3map_nonplanar
	q3map_shadeangle 60
	surfaceparm gravelsteps
	diffusemap textures/stone/mxrock1aa.tga
	bumpmap textures/stone/mxrock1aa_n.tga
	specularmap textures/stone/mxrock1aa_r.tga
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/fueldump/snow_floor
{
	qer_editorimage textures/snow/s_dirt_m03i_2_phong.tga
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm gravelsteps
	diffusemap textures/snow/s_dirt_m03i_2.tga
	bumpmap textures/snow/s_dirt_m03i_2_n.tga
	specularmap textures/snow/s_dirt_m03i_2_r.tga
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/terrain/dirt_m03i
{
	diffusemap textures/terrain/dirt_m03i.tga
	bumpmap textures/terrain/dirt_m03i_n.tga
	specularmap textures/terrain/dirt_m03i_r.tga
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

//==========================================================================
// Terrain/tunnel blend textures
//==========================================================================
textures/snow_sd/snow_road01
{
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm snowsteps
	diffusemap textures/snow_sd/snow_road01.tga
	bumpmap textures/snow_sd/snow_road01_n.tga
	specularmap textures/snow_sd/snow_road01_r.tga
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

textures/snow_sd/snow_path01
{
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm snowsteps
	diffusemap textures/snow_sd/snow_path01.tga
	bumpmap textures/snow_sd/snow_path01_n.tga
	specularmap textures/snow_sd/snow_path01_r.tga
	{
		lightmap $lightmap
		blendFunc filter
		rgbgen identity
	}
}

//==========================================================================
// Misc stuff for the central building in the axis base
//==========================================================================
// Comms tower
textures/fueldump/atruss_m06a
{
	qer_editorimage textures/assault/atruss_m06a.tga
	qer_alphafunc gequal 0.5
	cull disable
	nomipmaps
	nopicmip
	surfaceparm trans
	surfaceparm alphashadow
	surfaceparm roofsteps
	diffusemap textures/assault/atruss_m06a.tga
	bumpmap textures/assault/atruss_m06a_n.tga
	specularmap textures/assault/atruss_m06a_r.tga
	implicitMask textures/assault/atruss_m06a.tga
}

textures/awf/awf_w_m11
{
	qer_editorimage textures/awf/awf_w_m11.tga
	q3map_lightimage textures/awf/awf_w_m11_g.tga
	q3map_surfacelight 200
	surfaceparm nomarks
	diffusemap textures/awf/awf_w_m11.tga
	bumpmap textures/awf/awf_w_m11_n.tga
	specularmap textures/awf/awf_w_m11_r.tga
	implicitMask textures/awf/awf_w_m11.tga
}

textures/awf/awf_w_m11_nlm
{
	qer_editorimage textures/awf/awf_w_m11_nlm.tga
	surfaceparm nomarks
	diffusemap textures/awf/awf_w_m11_nlm.tga
	bumpmap textures/awf/awf_w_m11_nlm_n.tga
	specularmap textures/awf/awf_w_m11_nlm_r.tga
	implicitMask textures/awf/awf_w_m11.tga
}

// This is more or less similiar to clipmissile
textures/alpha/fence_c11fd
{
	qer_editorimage textures/alpha/fence_c11.tga
	qer_trans 0.85
	cull disable
	nomipmaps
	nopicmip
	surfaceparm trans
	surfaceparm clipmissile
	surfaceparm nomarks
	surfaceparm alphashadow
	surfaceparm playerclip
	surfaceparm metalsteps
	surfaceparm pointlight
	diffusemap textures/alpha/fence_c11.tga
	bumpmap textures/alpha/fence_c11_n.tga
	specularmap textures/alpha/fence_c11_r.tga
	implicitMask textures/alpha/fence_c11.tga
}

//==========================================================================
// Various terrain decals textures
//==========================================================================

// ydnar: nuked unnecessary alphaGen vertex part & added surfaceparm trans & pointlight
textures/fueldump/cave_floorblend
{
	qer_editorimage textures/snow/s_dirt_m03i_alphadir.tga
	q3map_nonplanar
	q3map_shadeangle 60
	polygonOffset
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm gravelsteps
	surfaceparm pointlight
	{
		map textures/snow/s_dirt_m03i_alpha.tga
		blendFunc blend
		rgbGen vertex
	}
}

textures/fueldump/alphatree
{
	qer_editorimage textures/snow/s_dirt_m03i_alphatree.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	polygonOffset
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	{
		map textures/snow/s_dirt_m03i_alphatree.tga
		blendFunc blend
		rgbGen vertex
	}
}

//==========================================================================
// Various metal surfaceparm textures
//==========================================================================
textures/fueldump/door_m01asml
{
	qer_editorimage textures/fueldump_sd/door_m01asml.tga
	surfaceparm metalsteps
	diffusemap textures/fueldump_sd/door_m01asml.tga
	bumpmap textures/fueldump_sd/door_m01asml_n.tga
	specularmap textures/fueldump_sd/door_m01asml_r.tga
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/fueldump/door_m01asml_axis
{
	qer_editorimage textures/fueldump_sd/door_m01asml_axis.tga
	surfaceparm metalsteps
	diffusemap textures/fueldump_sd/door_m01asml_axis.tga
	bumpmap textures/fueldump_sd/door_m01asml_axis_n.tga
	specularmap textures/fueldump_sd/door_m01asml_axis_r.tga
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

//==========================================================================
// Ice Lake
//==========================================================================
textures/fueldump/icelake_top
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake3.tga
	tesssize 256
	cull disable
	sort underwater
	surfaceparm slick
	surfaceparm trans
	surfaceparm glass
	bumpmap textures/snow_sd/icelake3_n.tga
	specularmap textures/snow_sd/icelake3_r.tga
	{
		stage diffusemap
		map textures/snow_sd/icelake3.tga
		blendfunc blend
		alphaGen const 0.9
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
	}
}

textures/fueldump/icelake_bottom
{
	qer_editorimage textures/snow_sd/icelake3.tga
	qer_trans 0.7
	cull disable
	sort seethrough
	bumpmap textures/snow_sd/icelake3_n.tga
	specularmap textures/snow_sd/icelake3_r.tga
	{
		stage diffusemap
		map textures/snow_sd/icelake3.tga
		blendfunc filter
	}
	{
		map textures/effects/envmap_ice2
		tcgen environment
		blendfunc blend
		alphaGen const 0.5
	}
}

textures/fueldump/riverbed
{
	qer_editorimage textures/stone/mxdebri0_riverbed.tga
	{
		stage diffusemap
		map textures/stone/mxdebri0_riverbed.tga
		rgbGen vertex
	}	
	{
		stage bumpmap
		map textures/stone/mxdebri0_riverbed_n.tga
		rgbGen vertex
	}	
	{
		stage specularmap
		map textures/stone/mxdebri0_riverbed_r.tga
		rgbGen vertex
	}
}