
textures/liquids_sd/seawall_specular
{
    qer_editorimage textures/liquids_sd/seawall_specular.tga

	{ 
		stage liquidmap
		map textures/liquids_sd/seawall_specular_n.tga
		blendFunc blend
		alphaFunc GE128
		depthWrite
		rgbgen identity
	}
}

textures/liquids_sd/sea_bright_na
{
    qer_editorimage textures/liquids_sd/sea_bright_na.tga
	
	{ 
		stage liquidmap
		map textures/liquids_sd/sea_bright_na_n.tga
		blendFunc blend
		alphaFunc GE128
		depthWrite
		rgbgen identity
	}
}

textures/liquids_sd/seawall_foam
{
    qer_editorimage textures/liquids_sd/seawall_foam.tga
    
	{
		stage liquidmap
		map textures/liquids_sd/seawall_foam_n.tga
		blendFunc blend
		alphaFunc GE128
		depthWrite
		rgbgen identity
	}
}

// ocean fog water
textures/battery/fog_water
{
	qer_editorimage textures/common/fog_water
	
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	surfaceparm water
	refractionIndex 1.3 // water
                fresnelPower 2.0
                fresnelScale 0.85       // + sinTable[time * 0.4] * 0.25
                fresnelBias  0.05
}

// abstract shader for subclassed shaders
textures/battery/ocean_base
{
	qer_editorimage textures/liquids_sd/seawall_ocean.tga
	
	diffusemap textures/liquids_sd/sea_bright_na.tga
	bumpmap textures/liquids_sd/sea_bright_na_n.tga
	specularmap textures/liquids_sd/sea_bright_na_r.tga
	
	qer_trans 0.75
	//q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_globalTexture
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm lightfilter
	surfaceparm pointlight
	nopicmip
}

// nonsolid terrain shader
textures/battery/ocean_terrain
{
	qer_editorimage textures/common/terrain_nonsolid.tga
	
	q3map_terrain
	qer_trans .5
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	nopicmip
}

// subclassed ocean shaders
textures/battery/ocean_0
{
	q3map_baseshader textures/battery/ocean_base
	
	cull none
	deformVertexes wave 1317 sin 0 2.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	
	{
		map textures/liquids_sd/seawall_specular.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
		depthWrite
	}
	{ 
		map textures/liquids_sd/sea_bright_na.tga
		blendFunc blend
		rgbGen identity
		alphaGen const 0.8
		tcmod scroll 0.005 0.03
	}
}

textures/battery/ocean_1
{
	q3map_baseshader textures/battery/ocean_base
	
	cull none
	deformVertexes wave 1317 sin 0 2.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	
	{
		map textures/liquids_sd/seawall_specular.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
		depthWrite
	}
	{ 
		map textures/liquids_sd/sea_bright_na.tga
		blendFunc blend
		rgbGen identity
		alphaGen const .8
		tcmod scroll 0.005 0.03
	}
	{
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.2 0.1 0 0.2
		alphaGen vertex
		tcMod turb 0 0.05 0 0.15
		tcmod scroll -0.01 0.08
	}
	{ 
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.15 0.1 0.1 0.15
		alphaGen vertex
		tcMod turb 0 0.05 0.5 0.15
		tcmod scroll 0.01 0.09
	}
}

textures/battery/ocean_0to1
{
	q3map_baseshader textures/battery/ocean_base
	
	cull none
	deformVertexes wave 1317 sin 0 2.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30 	
	
	{
		map textures/liquids_sd/seawall_specular.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
		depthWrite
	}
	
	{ 
		map textures/liquids_sd/sea_bright_na.tga
		blendFunc blend
		rgbGen identity
		alphaGen const .8
		tcmod scroll 0.005 0.03
	}
	{
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.2 0.1 0 0.2
		alphaGen vertex
		tcMod turb 0 0.05 0 0.15
		tcmod scroll -0.01 0.08
	}
	{ 
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.15 0.1 0.1 0.15
		alphaGen vertex
		tcMod turb 0 0.05 0.5 0.15
		tcmod scroll 0.01 0.09
	}
	
}

textures/battery/rock_graynoise
{
    q3map_nonplanar
    q3map_shadeangle 180
    //q3map_tcmodscale 1.5 1.5
    qer_editorimage textures/temperate_sd/rock_grayvar.tga
    
   
    {
	    stage diffusemap
        map textures/temperate_sd/rock_grayvar.tga
        rgbGen identity
    }
	{
	    stage bumpmap
        map textures/temperate_sd/rock_grayvar_n.tga
        rgbGen identity
    }
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/sand_disturb
{
    q3map_nonplanar
    q3map_shadeangle 180
    //q3map_tcmodscale 1.5 1.5
    qer_editorimage textures/temperate_sd/sand_disturb_bright.tga
    
    surfaceparm landmine
    surfaceparm gravelsteps
    
    {
        map $lightmap
        rgbGen identity
    }
    {
	    stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
        rgbGen identity
    }
	{
	    stage bumpmap
		map textures/temperate_sd/sand_disturb_bright_n.tga
        rgbGen identity
    }
}

textures/battery/terrain_base
{
	//q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	q3map_lightmapsize 512 512
	q3map_lightmapMergable
	q3map_lightmapsamplesize 16
	q3map_lightmapaxis z
	q3map_shadeangle 120
}

textures/battery/terrain_0
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	
	{
        stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
        stage bumpmap
		map textures/temperate_sd/sand_disturb_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/sand_disturb_bright_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_1
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_2
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_3
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/rock_grayvar.tga
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_4
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/rock_grayvar.tga
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_5
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/sand_patchnoise.tga
	
	surfaceparm landmine
	surfaceparm gravelsteps
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
		stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_0to1
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	
	{
		stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
		stage bumpmap
		map textures/temperate_sd/sand_disturb_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	
	{
		stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_0to2
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	{
	    stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex		
	}
	{
	    stage bumpmap 
		map textures/temperate_sd/sand_disturb_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap 
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_0to3
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	   	stage bumpmap
	   	map textures/temperate_sd/sand_disturb_bright_n.tga
	   	rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_0to4
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap 
		map textures/temperate_sd/sand_disturb_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
	    map textures/temperate_sd/rock_grayvar_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA	
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_0to5
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	{
	    stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_disturb_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
	    map textures/temperate_sd/sand_patchnoise_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_1to2
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_1to3
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_1to4
{
	q3map_baseshader textures/battery/terrain_base

	{
	    stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex	
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_1to5
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	{
	    stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_2to3
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_2to4
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex	
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rocky_sand_n.tga
	    rgbgen identity
		alphaGen vertex
	}
	
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA	
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHAd	
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_2to5
{
	q3map_baseshader textures/battery/terrain_base
	
	surfaceparm landmine
	surfaceparm gravelsteps
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_3to4
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_3to5
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

textures/battery/terrain_4to5
{
	q3map_baseshader textures/battery/terrain_base
	
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
	}
	
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}
// Leaving these - Thunder

textures/battery/water_nodraw
{
	qer_editorimage textures/common/nodraw
	
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	surfaceparm water
}

textures/battery/water_fog
{
	qer_editorimage textures/common/fog_water
	
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	surfaceparm fog
  	
  	fogparms ( 0.3137 0.36 0.4039 ) 256
}
