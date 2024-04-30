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
	cull disable
	nocompress
	nopicmip
	// the next 2 lines are commented out, because waterfogvars is not functional at this moment.
	//nofog
	//waterfogvars ( 1.0 0.0 0.0 ) 0.7 // this needs all the spaces inside the ( x x x )

	// collapsed layer 1 : ST_BUNDLE_WDB,  liquid/water + diffuse + bump
	bumpmap textures/liquids_sd/siwa_water_n.tga
	{ 
		stage liquidmap
		refractionIndex 1.3 // water
		normalScale 0.01 // the scale of the refraction deformations
		fresnelPower 2.0
		fresnelScale -0.15
		fresnelBias 0.35
		// 'color' is the fog-color you see on the watersurface only.
		// It does not render any fog on objects other than the watersurface.
		// Example: In Oasis, if you are above the watersurface, looking into the underwater tunnel,
		// it looks like the tunnel is fogged (you see that on the watersurface).
		// But if you go underwater, and look into that tunnel, you notice the tunnel has no fog at all.
		// When you are underwater, and look at the world above the water, you see the world fogged with the same 'color'.
		// Also here, it's just fog rendered on the watersurface.
		color 0.1, 0.1, 0.1, 0.2
	}
	{
		stage diffusemap
		map textures/liquids_sd/siwa_water.tga
		blendFunc blend
		alphaGen const 0.2
		rgbgen identity
		// apply any tcMod to only the diffuse texture
		tcmod scroll -.02 .001
		tcmod scale 0.5 0.5
		fog on
	}

	// the other stages are not collapsed
	{ 
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc add
		rgbGen wave sin 0.3 0.02 0 0.25 
		tcmod scroll -.001 -.0002
		tcmod scale 0.01 0.01
		fog on
	}
	{
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc add
		rgbGen wave sin 0.1 0.03 0 0.4
		tcmod scroll -.005 -.001
		tcmod scale 1 1
		fog on
	}
	{
		map textures/liquids_sd/siwa_shimshim1.tga
		blendFunc add
		rgbGen wave sin 0.4 0.02 0 0.3
		tcmod scroll .005 -.001
		tcmod transform 0 1.5 1 1.5 2 1
		fog on
	}
}

textures/liquids_sd/siwa_water_2
{
	qer_editorimage textures/liquids_sd/siwa_water.tga
	qer_trans .5
	q3map_globaltexture
	cull disable
	nocompress
	nopicmip
	nofog
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm water
	waterfogvars ( 0.11 0.13 0.15 ) 0.2

	// collapsed layer 1 : ST_BUNDLE_LDB,  liquid + diffuse + bump
	bumpmap textures/liquids_sd/siwa_water_n.tga
	{
		stage liquidmap
		fresnelPower 2.0 
		fresnelScale -0.15
		fresnelBias 0.35
		blendFunc blend
		alphaFunc GE128
		rgbgen identity
		fog on
		color 0.1, 0.1, 0.1, 0.5
	}
	{
		fog on
		map textures/liquids_sd/siwa_water.tga
		blendFunc blend
		alphaGen const 0.2
		depthWrite
		rgbgen identity
		tcmod scroll -.02 .001
		tcmod scale 0.5 0.5
	}

	// stages 2 to 4 are not collapsed
	{
		fog on
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc add
		rgbGen wave sin 0.3 0.02 0 0.25
		tcmod scale 0.01 0.01
		tcmod scroll -.001 -.0002
	}
	{
		fog on
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc add
		rgbGen wave sin 0.1 0.03 0 0.4
		tcmod scale 1 1
		tcmod scroll -.005 -.001
	}
	{
		fog on
		map textures/liquids_sd/siwa_shimshim1.tga
		blendFunc add
		rgbGen wave sin 0.4 0.02 0 0.3
		tcmod transform 0 1.5 1 1.5 2 1
		tcmod scroll .005 -.001
	}
}

textures/liquids_sd/siwa_waternodraw
{
	qer_editorimage textures/liquids_sd/siwa_waternodraw.tga
	qer_trans .75
	surfaceparm nodraw
	surfaceparm water
}
