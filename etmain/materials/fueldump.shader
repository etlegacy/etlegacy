//======================================================================
// Fueldump.shader
// Last edit: 04.03.2017 Thunder
//
//======================================================================
// q3map_sun <red> <green> <blue> <intensity> <degrees> <elevation>

textures/skies/fueldump_clouds
{
    diffusemap textures/skies/fueldump_clouds
	bumpmap textures/skies/fueldump_clouds_n
	//specularmap textures/skies/fueldump_clouds_s
}
textures/fueldump/fueldumpsky
{
	qer_editorimage textures/skies/fueldump_clouds
	q3map_lightrgb 0.8 0.9 1.0
	q3map_skylight 85 3
	q3map_sun 1 .95 .9 200 210 28
	skyparms - 200 -
	//surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	{
	    stage diffuseMap
		map textures/skies/fueldump_clouds
		rgbGen identity
		//vertexcolor
	}
	{
	    stage bumpmap
		map textures/skies/fueldump_clouds_n
		vertexcolor
		blend blend
	}
/*	{
	    stage specularmap
		map textures/skies/fueldump_clouds_s
		blend blend
	}*/
	{
		map textures/skies/fueldump_clouds
		blendfunc blend
		tcMod scroll 0.0015 0.00
		//tcMod scale 2 1
	}
}



//======================================================================
// Base for metashaders
//======================================================================
textures/fueldump/terrain_base
{
	q3map_lightmapMergable
	q3map_lightmapaxis z
	q3map_lightmapsize 512 512
	q3map_normalimage textures/sd_bumpmaps/normalmap_terrain
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
	qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	diffusemap textures/stone/mxsnow2
	bumpmap textures/stone/mxsnow2_n
	specularmap textures/stone/mxsnow2
	{
	   map textures/stone/mxsnow2
	   rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	
}

textures/fueldump/terrain1_1
{
    qer_editorimage textures/snow_sd/mxrock4b_snow
	q3map_baseshader textures/fueldump/terrain_base
	diffusemap textures/snow_sd/mxrock4b_snow
	bumpmap textures/snow_sd/mxrock4b_snow_n
	specularmap textures/snow_sd/mxrock4b_snow_s
	
	{
		map textures/snow_sd/mxrock4b_snow
		rgbgen identity
	}
	
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/fueldump/terrain1_2
{
    qer_editorimage textures/stone/mxsnow3
	q3map_baseshader textures/fueldump/terrain_base
	diffusemap textures/stone/mxsnow3
	bumpmap textures/stone/mxsnow3_n
	specularmap textures/stone/mxsnow3_s
	{
		map textures/stone/mxsnow3
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	
}

textures/fueldump/terrain1_3
{
    qer_editorimage textures/stone/mxrock3h_snow
	q3map_baseshader textures/fueldump/terrain_base
	diffusemap textures/stone/mxrock3h_snow
	bumpmap textures/stone/mxrock3h_snow_n
	specularmap textures/stone/mxrock3h_snow_S
	{   
		map textures/stone/mxrock3h_snow
		rgbgen identity
	}

	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
}

textures/fueldump/terrain1_0to1
{
	qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
	    stage diffusemap
		map textures/stone/mxsnow2
		vertexcolor
	}
	{
	   stage bumpmap
	   map textures/stone/mxsnow2_n
	   blend blend
	   vertexcolor
		
	}
	{
	    stage specularmap
		map textures/stone/mxsnow2
		blend blend
		vertexcolor
	}
	
	{
		stage diffusemap
		map textures/snow_sd/mxrock4b_snow
		vertexcolor
		blend blend
		
	}
	{
	    stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n
		vertexcolor
		blend blend
		
	}
	{
	    stage specularmap
		map textures/snow_sd/mxrock4b_snow_s
		vertexcolor
		blend blend
	}
	
}

textures/fueldump/terrain1_0to2
{
    qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
	    stage diffusemap
		map textures/stone/mxsnow2
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow2_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxsnow2_s
		vertexcolor
		blend blend
	}
	{
		map textures/stone/mxsnow3
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxsnow3_s
		vertexcolor
		blend blend
	}
}

textures/fueldump/terrain1_0to3
{
	qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
	    stage diffusemap
		map textures/stone/mxsnow2
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow2_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxsnow2_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/stone/mxrock3h_snow
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow
		vertexcolor
		blend blend
	}
}

textures/fueldump/terrain1_1to2
{
    qer_editorimage textures/snow_sd/mxrock4b_snow
	q3map_baseshader textures/fueldump/terrain_base
	{
	    stage diffusemap
		map textures/snow_sd/mxrock4b_snow
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n
		vertexcolor
		blend blend
    }
	{
	   stage specularmap
	   map textures/snow_sd/mxrock4b_snow_s
	   vertexcolor
	   blend blend
	}
	{
	    stage diffusemap
		map textures/stone/mxsnow3
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxsnow3_s
		vertexcolor
		blend blend
	}
}

textures/fueldump/terrain1_1to3
{
	qer_editorimage textures/snow_sd/mxrock4b_snow
	q3map_baseshader textures/fueldump/terrain_base
	{
	    stage diffusemap
		map textures/snow_sd/mxrock4b_snow
		vertexcolor
	}
	{
	    stage bumpmap
		map textures/snow_sd/mxrock4b_snow_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/snow_sd/mxrock4b_snow_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/stone/mxrock3h_snow
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow_s
		vertexcolor
		blend blend
	}
}

textures/fueldump/terrain1_2to3
{
	qer_editorimage textures/stone/mxsnow3
	q3map_baseshader textures/fueldump/terrain_base
	{
	    stage diffusemap
		map textures/stone/mxsnow3
		vertexcolor
		
	}
	{
	    stage bumpmap
		map textures/stone/mxsnow3_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxsnow3_s
		vertexcolor
		blend blend
	}
	{
	    stage diffusemap
		map textures/stone/mxrock3h_snow
		vertexcolor
		blend blend
	}
	{
	    stage bumpmap
		map textures/stone/mxrock3h_snow_n
		vertexcolor
		blend blend
	}
	{
	    stage specularmap
		map textures/stone/mxrock3h_snow_s
		vertexcolor
		blend blend
	}
}

//===========================================================================
// Floor and wall textures for the cave system + hong phong goodness
//===========================================================================
textures/fueldump/cave_dark
{
	q3map_nonplanar
	q3map_shadeangle 60
	qer_editorimage textures/stone/mxrock3_a
	diffusemap textures/stone/mxrock3_a
	bumpmap textures/stone/mxrock3_a_n
	specularmap textures/stone/mxrock3_a_s
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/stone/mxrock3_a
		blendFunc filter
	}
}

textures/fueldump/cave_floor
{
	surfaceparm gravelsteps
	q3map_nonplanar
	q3map_shadeangle 60
	qer_editorimage textures/stone/mxrock1aa
	diffusemap textures/stone/mxrock1aa
	bumpmap textures/stone/mxrock1aa_n
	specularmap textures/stone/mxrock1aa_s
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/stone/mxrock1aa
		blendFunc filter
	}
}

textures/fueldump/snow_floor
{
	surfaceparm gravelsteps
	q3map_nonplanar
	q3map_shadeangle 179
	qer_editorimage textures/snow/s_dirt_m03i_2_phong
	diffusemap textures/snow/s_dirt_m03i_2
	bumpmap textures/snow/s_dirt_m03i_2_n
	specularmap textures/snow/s_dirt_m03i_2_s
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/snow/s_dirt_m03i_2
		blendFunc filter
	}
}

textures/terrain/dirt_m03i
{
    diffusemap textures/terrain/dirt_m03i
	bumpmap textures/terrain/dirt_m03i_n
	specularmap textures/terrain/dirt_m03i_s
	{
		map textures/terrain/dirt_m03i
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}

}

//==========================================================================
// Terrain/tunnel blend textures
//==========================================================================
textures/snow_sd/snow_road01
{
    diffusemap textures/snow_sd/snow_road01
	bumpmap textures/snow_sd/snow_road01_n
	specularmap textures/snow_sd/snow_road01_s
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm snowsteps
	{
		map textures/snow_sd/snow_road01
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	
}

textures/snow_sd/snow_path01
{
    diffusemap textures/snow_sd/snow_path01
	bumpmap textures/snow_sd/snow_path01_n
	specularmap textures/snow_sd/snow_path01_s
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm snowsteps
	{
		map textures/snow_sd/snow_path01
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	
}

//==========================================================================
// Misc stuff for the central building in the axis base
//==========================================================================
// Comms tower
textures/fueldump/atruss_m06a
{
	qer_alphafunc gequal 0.5
	qer_editorimage textures/assault/atruss_m06a
	diffusemap textures/assault/atruss_m06a
	bumpmap textures/assault/atruss_m06a_n
	specularmap textures/assault/atruss_m06a_s
	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm roofsteps
	surfaceparm trans
	implicitMask textures/assault/atruss_m06a
}

textures/awf/awf_w_m11
{
	qer_editorimage textures/awf/awf_w_m11
	diffusemap textures/awf/awf_w_m11
	bumpmap textures/awf/awf_w_m11_n
	specularmap textures/awf/awf_w_m11_s
	q3map_lightimage textures/awf/awf_w_m11_g
	q3map_surfacelight 200
	surfaceparm nomarks
	implicitMask textures/awf/awf_w_m11
}

textures/awf/awf_w_m11_nlm
{
	qer_editorimage textures/awf/awf_w_m11_nlm
	diffusemap textures/awf/awf_w_m11_nlm
	bumpmap textures/awf/awf_w_m11_nlm_n
	specularmap textures/awf/awf_w_m11_nlm_s
	surfaceparm nomarks
	implicitMask textures/awf/awf_w_m11
}

// This is more or less similiar to clipmissile
textures/alpha/fence_c11fd
{
	qer_trans 0.85
	qer_editorimage textures/alpha/fence_c11
	diffusemap textures/alpha/fence_c11
	bumpmap textures/alpha/fence_c11_n
	specularmap textures/alpha/fence_c11_s
	cull disable
	nomipmaps
	nopicmip

	surfaceparm clipmissile
	surfaceparm nomarks
	surfaceparm alphashadow
	surfaceparm playerclip
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans

	implicitMask textures/alpha/fence_c11
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
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm gravelsteps
	surfaceparm pointlight
	polygonOffset

	{
		map textures/snow/s_dirt_m03i_alpha.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}


textures/fueldump/alphatree
{
	qer_editorimage textures/snow/s_dirt_m03i_alphatree.tga
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	{
		map textures/snow/s_dirt_m03i_alphatree.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}


//==========================================================================
// Various metal surfaceparm textures
//==========================================================================
textures/fueldump/door_m01asml
{
	qer_editorimage textures/fueldump_sd/door_m01asml
//	diffusemap textures/fueldump/door_m01asml
//	bumpmap textures/fueldump/door_m01asml_n
//	specularmap textures/fueldump/door_m01asml
	surfaceparm metalsteps
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/fueldump_sd/door_m01asml
		blendFunc filter
	}
}

textures/fueldump/door_m01asml_axis
{
	qer_editorimage textures/fueldump_sd/door_m01asml_axis
	diffusemap textures/fueldump_sd/door_m01asml_axis
	bumpmap textures/fueldump_sd/door_m01asml_axis_n
	specularmap textures/fueldump_sd/door_m01asml_axis_s
	surfaceparm metalsteps
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/fueldump_sd/door_m01asml_axis
		blendFunc filter
	}
}

//==========================================================================
// Ice Lake
//==========================================================================
textures/fueldump/icelake_top
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake3
	diffusemap textures/snow_sd/icelake3
	bumpmap textures/snow_sd/icelake3_n
	specularmap textures/snow_sd/icelake3_s
	sort seethrough
	surfaceparm slick
	surfaceparm trans
	surfaceparm glass
	
	tesssize 256

	{
		map textures/effects/envmap_ice2
		tcgen environment
		blendfunc blend
	}
	{
		map textures/snow_sd/icelake3
		blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
		depthWrite
	}
	
}

textures/fueldump/icelake_bottom
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake3
	diffusemap textures/snow_sd/icelake3
	bumpmap textures/snow_sd/icelake3_n
	specularmap textures/snow_sd/icelake3_s
	sort seethrough
	surfaceparm trans
	cull disable
	
	{
		map textures/snow_sd/icelake3
		blendfunc filter
	}
}

textures/fueldump/riverbed
{
	qer_editorimage textures/stone/mxdebri0_riverbed
	diffusemap textures/stone/mxdebri0_riverbed
	bumpmap textures/stone/mxdebri0_riverbed_n
	specularmap textures/stone/mxdebri0_riverbed_s

	{
		map textures/stone/mxdebri0_riverbed
		rgbGen vertex
	}	
}