
textures/liquids_sd/sea_bright_na
{
    qer_editorimage textures/liquids_sd/sea_bright_na.tga
	{ 
		stage liquidmap
		map textures/liquids_sd/sea_bright_na.tga
//		blendFunc blend
//		alphaFunc GE128
//		depthWrite
		rgbgen identity
	}
}

textures/liquids_sd/seawall_specular
{
    qer_editorimage textures/liquids_sd/seawall_specular.tga
	{ 
		stage liquidmap
		map textures/liquids_sd/seawall_specular.tga
//		blendFunc blend
//		alphaFunc GE128
//		depthWrite
		rgbgen identity
	}
}

textures/liquids_sd/seawall_foam
{
    qer_editorimage textures/liquids_sd/seawall_foam.tga
	{
		stage liquidmap
		map textures/liquids_sd/seawall_foam_n.tga
//		blendFunc blend
//		alphaFunc GE128
//		depthWrite
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
	surfaceparm fog
}

// nonsolid terrain shader
textures/battery/ocean_terrain
{
	qer_editorimage textures/common/terrain_nonsolid.tga
	
//	q3map_terrain
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_lightmapAxis z
	q3map_nonplanar
	q3map_shadeAngle 170

	qer_trans .5
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	nopicmip
}

// abstract shader for subclassed shaders
textures/battery/ocean_base
{
	qer_editorimage textures/liquids_sd/seawall_ocean.tga
	qer_trans 0.75
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_lightmapAxis z
	q3map_nonplanar
	q3map_shadeAngle 170
	q3map_globalTexture
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm lightfilter
	surfaceparm pointlight
	surfaceparm water
	surfaceparm fog
	nopicmip
}

// subclassed ocean shaders
textures/battery/ocean_0
{
	q3map_baseshader textures/battery/ocean_base
	qer_editorimage textures/liquids_sd/seawall_ocean.tga
	deformVertexes wave 1317 sin 0 2.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	cull none
	sort underwater

	// collapsed layer 1 : ST_BUNDLE_WDB,  liquid/water + diffuse + bump
	// Note: this liquid stage does not have a lightmap rendered (nor will it have light/shadows).
	bumpmap textures/liquids_sd/sea_bright_na_n.tga
	{ 
		stage liquidmap
		refractionIndex 1.3
		// (because of current render issues with this Battery water, we do not use refraction)
		normalScale 0.00 // this is the scale of the refraction
		// fresnel determines the ratio reflection/refraction.
		// At higher angles of incedent, water becomes more reflective than refractive.
		// In other words: the further you look at a watersurface, the more reflective it is.
		// You see refraction at max, when you look straight down at the watersurface (angle if incedent is 0).
		fresnelPower 1.0 
		fresnelScale 2.0
		fresnelBias 0.1
		alphaGen const 0.2
		// 'color' is the fog-color you see on the watersurface only.
		// It does not render any fog on objects other than the watersurface.
		// Example: In Oasis, if you are above the watersurface, looking into the underwater tunnel,
		// it looks like the tunnel is fogged (you see that on the watersurface).
		// But if you go underwater, and look into that tunnel, you notice the tunnel has no fog at all.
		// When you are underwater, and look at the world above the water, you see the world fogged with the same 'color'.
		// Also here, it's just fog rendered on the watersurface.
		// Extra note: If you make the 4th parameter have a value of 1.0, then you will not see reflections on the watersurface
		fog on
		color 0.4, 0.4, 0.4, 0.4
	}
	{
		stage diffusemap
		map textures/liquids_sd/sea_bright_na.tga
//		alphaGen const 0.2 // this will do nothing: the alphagen of the liquid stage is used
		// Apply any tcMod to only the diffuse texture (of the ST_BUNDLE_WDB stage)
		tcmod scroll -0.01 0.06		// the bumps will go along with the diffuse "automagically"
		tcmod turb 0 0.05 0 0.12	// If desired, you can apply >1 tcMod.
	}

	// layer 2 : a "dummy" diffusemap that will receive the lightmap
	// but we don't want this to be displayed..
	// Note this is a workaround /todo
	{
		stage diffusemap
		map $blackimage
		blendfunc GL_ZERO GL_ONE
	}
/*
	// the glsl liquid shader renders the calculated specular highlights
	// and the cubeProbes reflections on the watersurface.
	// That's why i disabled the next stage.
	{
		map textures/liquids_sd/seawall_specular.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
	}
*/
}


textures/battery/ocean_0to1
{
	q3map_baseshader textures/battery/ocean_base
	qer_editorimage textures/liquids_sd/seawall_ocean.tga
	deformVertexes wave 1317 sin 0 2.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30 	
	cull none
	sort underwater

	// collapsed layer 1 : ST_BUNDLE_WDB,  liquid/water + diffuse + bump
	bumpmap textures/liquids_sd/sea_bright_na_n.tga
	{ 
		stage liquidmap
		refractionIndex 1.3
		normalScale 0.00
		fresnelPower 1.0 
		fresnelScale 2.0
		fresnelBias 0.1
		alphaGen const 0.2
		fog on
		color 0.4, 0.4, 0.4, 0.4
	}
	{ 
		stage diffusemap
		map textures/liquids_sd/sea_bright_na.tga
//		blendFunc blend
//		alphaGen const 0.2
//		rgbGen identity
		tcmod scroll -0.01 0.06
		tcMod turb 0 0.05 0 0.12
	}

	// collapsed layer 2 : ST_BUNDLE_DB,  diffuse + bump
	{
//		stage diffusemap
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
//blendFunc blend
		alphaGen vertex
		rgbGen wave sin 0.45 0.1 0.3 0.16
		tcmod scroll -0.01 0.17
		tcMod turb 0 0.05 0 0.38
	}
	{
		stage bumpmap
		map textures/liquids_sd/seawall_foam_n.tga
	}

	// collapsed layer 3 : ST_BUNDLE_DB,  diffuse + bump
	{
		stage bumpmap
		map textures/liquids_sd/seawall_foam_n.tga
	}
	{ 
//		stage diffusemap
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
//blendFunc blend
		alphaGen vertex
		rgbGen wave sin 0.25 0.1 0 0.1
		tcmod scroll -0.01 0.09
		tcMod turb 0 0.16 0.3 0.38
	}

/*
	{
		map textures/liquids_sd/seawall_specular.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
	}
*/
}

textures/battery/ocean_1
{
	q3map_baseshader textures/battery/ocean_base
	qer_editorimage textures/liquids_sd/seawall_ocean.tga
	deformVertexes wave 1317 sin 0 2.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	cull none
	sort underwater

	// collapsed layer 1 : ST_BUNDLE_WDB,  liquid/water + diffuse + bump
	bumpmap textures/liquids_sd/sea_bright_na_n.tga
	{ 
		stage liquidmap
		refractionIndex 1.3
		normalScale 0.00
		fresnelPower 1.0 
		fresnelScale 2.0
		fresnelBias 0.1
		alphaGen const 0.2
		fog on
		color 0.4, 0.4, 0.4, 0.4
	}
	{ 
		stage diffusemap
		map textures/liquids_sd/sea_bright_na.tga
//		blendFunc blend
//		alphaGen const 0.2
//		rgbGen identity
		tcmod scroll -0.01 0.06
		tcMod turb 0 0.05 0 0.12
	}

	// collapsed layer 2 : ST_BUNDLE_DB,  diffuse + bump
	// If you use tcMod, you need to add it for all specific stages that should be transformed by a tcMod.
	// For example, you could make the bumps move while the diffuse stays still.. or vice versa.
	{
		stage diffusemap
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
//blendFunc blend
		alphaGen vertex
		rgbGen wave sin 0.45 0.1 0.3 0.16
		tcmod scroll -0.01 0.17
		tcMod turb 0 0.05 0 0.38
	}
	{
		stage bumpmap
		map textures/liquids_sd/seawall_foam_n.tga
	}

	// collapsed layer 3 : ST_BUNDLE_DB,  diffuse + bump
	{
		stage bumpmap
		map textures/liquids_sd/seawall_foam_n.tga
	}
	{ 
		stage diffusemap
		map textures/liquids_sd/seawall_foam.tga
		blendFunc GL_SRC_ALPHA GL_ONE
//blendFunc blend
		alphaGen vertex
		rgbGen wave sin 0.25 0.1 0 0.1
		tcmod scroll -0.01 0.09
		tcMod turb 0 0.16 0.3 0.38
	}
/*
	{
		map textures/liquids_sd/seawall_specular.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
	}
*/
}

textures/battery/rock_graynoise
{
    qer_editorimage textures/temperate_sd/rock_grayvar.tga
    q3map_nonplanar
    q3map_shadeangle 180
    //q3map_tcmodscale 1.5 1.5
	diffusemap textures/temperate_sd/rock_grayvar.tga
	bumpmap textures/temperate_sd/rock_grayvar_n.tga
	specularmap textures/temperate_sd/rock_grayvar_r.tga
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/sand_disturb
{
    qer_editorimage textures/temperate_sd/sand_disturb_bright.tga
    q3map_nonplanar
    q3map_shadeangle 180
    //q3map_tcmodscale 1.5 1.5
    surfaceparm landmine
    surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
    {
        map $lightmap
		blendFunc filter
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
	qer_editorimage textures/temperate_sd/sand_disturb_bright.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_disturb_bright.tga
		rgbgen identity
		blendFunc blend
		alphaGen vertex
	}
	{
	   	stage bumpmap
	   	map textures/temperate_sd/sand_disturb_bright_n.tga
	   	rgbgen identity
		blendFunc blend
		alphaGen vertex
	}
	{
	   	stage specularmap
	   	map textures/temperate_sd/sand_disturb_bright_r.tga
	   	rgbgen identity
		blendFunc blend
		alphaGen vertex
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_1
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/sand_wave_bright.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_wave_bright.tga
	bumpmap textures/temperate_sd/sand_wave_bright_n.tga
	specularmap textures/temperate_sd/sand_wave_bright_r.tga
	{
		stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
	    map textures/temperate_sd/sand_wave_bright_r.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_2
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/rocky_sand.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/rocky_sand.tga
	bumpmap textures/temperate_sd/rocky_sand_n.tga
	specularmap textures/temperate_sd/rocky_sand_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap 
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rocky_sand_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_3
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/rock_graynoise.tga
	diffusemap textures/temperate_sd/rock_graynoise.tga
	bumpmap textures/temperate_sd/rock_graynoise_n.tga
	specularmap textures/temperate_sd/rock_graynoise_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_graynoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_4
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/rock_grayvar.tga
	diffusemap textures/temperate_sd/rock_grayvar.tga
	bumpmap textures/temperate_sd/rock_grayvar_n.tga
	specularmap textures/temperate_sd/rock_grayvar_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
	    map textures/temperate_sd/rock_grayvar_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap 
	    map textures/temperate_sd/rock_grayvar_r.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_5
{
	q3map_baseshader textures/battery/terrain_base
	qer_editorimage textures/temperate_sd/sand_patchnoise.tga
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_patchnoise.tga
	bumpmap textures/temperate_sd/sand_patchnoise_n.tga
	specularmap textures/temperate_sd/sand_patchnoise_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
	    map textures/temperate_sd/sand_patchnoise_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
	    map textures/temperate_sd/sand_patchnoise_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_0to1
{
	q3map_baseshader textures/battery/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
	{
		stage diffusemap
		map textures/temperate_sd/sand_wave_bright.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{ 
	    stage bumpmap
        map textures/temperate_sd/sand_wave_bright_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
	    map textures/temperate_sd/sand_wave_bright_r.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_0to2
{
	q3map_baseshader textures/battery/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
	{
		stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		stage bumpmap 
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		stage specularmap
		map textures/temperate_sd/rocky_sand_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_0to3
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_graynoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_0to4
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
	    map textures/temperate_sd/rock_grayvar_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap 
	    map textures/temperate_sd/rock_grayvar_r.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_0to5
{
	q3map_baseshader textures/battery/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_disturb_bright.tga
	bumpmap textures/temperate_sd/sand_disturb_bright_n.tga
	specularmap textures/temperate_sd/sand_disturb_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
	    map textures/temperate_sd/sand_patchnoise_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
	    map textures/temperate_sd/sand_patchnoise_n.tga
	    rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_1to2
{
	q3map_baseshader textures/battery/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_wave_bright.tga
	bumpmap textures/temperate_sd/sand_wave_bright_n.tga
	specularmap textures/temperate_sd/sand_wave_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rocky_sand.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rocky_sand_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rocky_sand_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_1to3
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/sand_wave_bright.tga
	bumpmap textures/temperate_sd/sand_wave_bright_n.tga
	specularmap textures/temperate_sd/sand_wave_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_graynoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_1to4
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/sand_wave_bright.tga
	bumpmap textures/temperate_sd/sand_wave_bright_n.tga
	specularmap textures/temperate_sd/sand_wave_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_grayvar_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_1to5
{
	q3map_baseshader textures/battery/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/sand_wave_bright.tga
	bumpmap textures/temperate_sd/sand_wave_bright_n.tga
	specularmap textures/temperate_sd/sand_wave_bright_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_patchnoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_2to3
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/rocky_sand.tga
	bumpmap textures/temperate_sd/rocky_sand_n.tga
	specularmap textures/temperate_sd/rocky_sand_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_graynoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_graynoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_graynoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_2to4
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/rocky_sand.tga
	bumpmap textures/temperate_sd/rocky_sand_n.tga
	specularmap textures/temperate_sd/rocky_sand_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_grayvar_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_2to5
{
	q3map_baseshader textures/battery/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	diffusemap textures/temperate_sd/rocky_sand.tga
	bumpmap textures/temperate_sd/rocky_sand_n.tga
	specularmap textures/temperate_sd/rocky_sand_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_patchnoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_3to4
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/rock_graynoise.tga
	bumpmap textures/temperate_sd/rock_graynoise_n.tga
	specularmap textures/temperate_sd/rock_graynoise_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/rock_grayvar.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/rock_grayvar_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/rock_grayvar_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_3to5
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/rock_graynoise.tga
	bumpmap textures/temperate_sd/rock_graynoise_n.tga
	specularmap textures/temperate_sd/rock_graynoise_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_patchnoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
	}
}

textures/battery/terrain_4to5
{
	q3map_baseshader textures/battery/terrain_base
	diffusemap textures/temperate_sd/rock_grayvar.tga
	bumpmap textures/temperate_sd/rock_grayvar_n.tga
	specularmap textures/temperate_sd/rock_grayvar_r.tga
	{
	    stage diffusemap
		map textures/temperate_sd/sand_patchnoise.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage bumpmap
		map textures/temperate_sd/sand_patchnoise_n.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
	    stage specularmap
		map textures/temperate_sd/sand_patchnoise_r.tga
		rgbgen identity
		alphaGen vertex
		blendFunc blend
	}
	{
		map $lightmap
		blendFunc filter
        rgbGen identity
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
