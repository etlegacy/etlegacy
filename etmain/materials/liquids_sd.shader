// liquids_sd.shader

textures/liquids_sd/siwa_water
{
	qer_editorimage textures/liquids_sd/siwa_water.tga
	qer_trans .5
	q3map_globaltexture
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nomarks
	surfaceparm lightfilter
	surfaceparm pointlight
	surfaceparm fog
	cull disable
	nopicmip
	// the next 2 lines are commented out, because waterfogvars is not functional at this moment.
	//nofog
	waterfogvars ( 0.4255 0.4804 0.6882 ) 256.0 // this needs all the spaces inside the ( x x x )
	sort underwater

	// collapsed layer 1 : ST_BUNDLE_WDB,  liquid/water + diffuse + bump
	bumpmap textures/liquids_sd/siwa_water_n.tga
	{ 
		stage liquidmap
		refractionIndex 1.3 // water
		normalScale 0.01 // the scale of the refraction deformations
		fresnelPower 1.0
		fresnelScale 2.0
		fresnelBias 0.5
		alphaGen const 0.2
		// 'color' is the fog-color you see on the watersurface only.
		// It does not render any fog on objects other than the watersurface.
		// Example: In Oasis, if you are above the watersurface, looking into the underwater tunnel,
		// it looks like the tunnel is fogged (you see that on the watersurface).
		// But if you go underwater, and look into that tunnel, you notice the tunnel has no fog at all.
		// When you are underwater, and look at the world above the water, you see the world fogged with the same 'color'.
		// Also here, it's just fog rendered on the watersurface.
		fog on
		color 0.1, 0.1, 0.1, 0.2
	}
	{
		stage diffusemap
		map textures/liquids_sd/siwa_water.tga
		// apply any tcMod to only the diffuse texture of a collapsed liquid stage
		// this will produce the bumpmapped, moving waves.
		tcmod scroll -.02 .001
		tcmod scale 0.5 0.5
	}

	// layer 2 : a "dummy" diffusemap that will receive the lightmap (and fancy light/shadows)
	// but we don't want this to be displayed, so we use a special blendfunc.
	// Note this is a workaround /todo
	{
		stage diffusemap
		map $blackimage
		blendfunc GL_ZERO GL_ONE
	}

	// the other stages are not collapsed
	// Do NOT make the next stages:	stage diffusemap, or else the light/shadow will be too bright
	{ 
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc blend
		alphaGen const 0.1
		rgbGen wave sin 0.3 0.02 0 0.25 
		tcmod scroll -.001 -.0002
		tcmod scale 0.01 0.01
	}

	{
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc add
		alphaGen const 0.1
		rgbGen wave sin 0.1 0.03 0 0.4
		tcmod scroll -.005 -.001
		tcmod scale 1 1
	}
	{
		map textures/liquids_sd/siwa_shimshim1.tga
		blendFunc add
		alphaGen const 0.1
		rgbGen wave sin 0.4 0.02 0 0.3
		tcmod scroll .005 -.001
		tcmod transform 0 1.5 1 1.5 2 1
	}

/*	// don't add a lightmap
	{
		map $lightmap
		rgbGen identity
		blendFunc filter
	}
*/
}

textures/liquids_sd/siwa_water_2
{
	qer_editorimage textures/liquids_sd/siwa_water.tga
	qer_trans .5
	q3map_globaltexture
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nomarks
	surfaceparm lightfilter
	surfaceparm pointlight
	surfaceparm fog
	cull disable
	nopicmip
	// the next 2 lines are commented out, because waterfogvars is not functional at this moment.
	//nofog
	waterfogvars ( 0.4255 0.4804 0.4882 ) 256.0 // this needs all the spaces inside the ( x x x )
	sort underwater

	// collapsed layer 1 : ST_BUNDLE_WDB,  water/liquid + diffuse + bump
	bumpmap textures/liquids_sd/siwa_water_n.tga
	{
		stage liquidmap
		fresnelPower 1.0 
		fresnelScale 2.0
		fresnelBias 0.5
		normalScale 0.01
		alphaGen const 0.2
		fog on
		color 0.1, 0.1, 0.1, 0.5
	}
	{
		stage diffusemap
		map textures/liquids_sd/siwa_water.tga
		tcmod scroll -.02 .001
		tcmod scale 0.5 0.5
	}
/*
	// stages 2 to 4 are not collapsed
	{
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc blend
		alphaGen const 0.1
		rgbGen wave sin 0.3 0.02 0 0.25
		tcmod scale 0.01 0.01
		tcmod scroll -.001 -.0002
	}
	{
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc add
		alphaGen const 0.001
//		rgbGen vertex
//		rgbGen wave sin 0.1 0.03 0 0.4
		tcmod scale 1 1
		tcmod scroll -.005 -.001
	}
	{
		map textures/liquids_sd/siwa_shimshim1.tga
		blendFunc add
		alphaGen const 0.001
//		rgbGen vertex
//		rgbGen wave sin 0.4 0.02 0 0.3
		tcmod transform 0 1.5 1 1.5 2 1
		tcmod scroll .005 -.001
	}
*/
/*
	{
		map $lightmap
		rgbGen identity
		blendFunc filter
	}
*/
}

textures/liquids_sd/siwa_waternodraw
{
	qer_editorimage textures/liquids_sd/siwa_waternodraw.tga
	qer_trans .75
	surfaceparm nodraw
	surfaceparm water
}
