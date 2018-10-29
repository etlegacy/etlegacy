// liquids_sd.shader

/*
// sinTable and cosTable must be defined for the rotate function to work
table sinTable { {
0.000000, 0.024541, 0.049068, 0.073565, 0.098017, 0.122411, 0.146730, 0.170962, 
0.195090, 0.219101, 0.242980, 0.266713, 0.290285, 0.313682, 0.336890, 0.359895, 
0.382683, 0.405241, 0.427555, 0.449611, 0.471397, 0.492898, 0.514103, 0.534998, 
0.555570, 0.575808, 0.595699, 0.615232, 0.634393, 0.653173, 0.671559, 0.689541, 
0.707107, 0.724247, 0.740951, 0.757209, 0.773010, 0.788346, 0.803208, 0.817585, 
0.831470, 0.844854, 0.857729, 0.870087, 0.881921, 0.893224, 0.903989, 0.914210, 
0.923880, 0.932993, 0.941544, 0.949528, 0.956940, 0.963776, 0.970031, 0.975702, 
0.980785, 0.985278, 0.989177, 0.992480, 0.995185, 0.997290, 0.998795, 0.999699, 
1.000000, 0.999699, 0.998795, 0.997290, 0.995185, 0.992480, 0.989177, 0.985278, 
0.980785, 0.975702, 0.970031, 0.963776, 0.956940, 0.949528, 0.941544, 0.932993, 
0.923880, 0.914210, 0.903989, 0.893224, 0.881921, 0.870087, 0.857729, 0.844854, 
0.831470, 0.817585, 0.803208, 0.788346, 0.773010, 0.757209, 0.740951, 0.724247, 
0.707107, 0.689541, 0.671559, 0.653173, 0.634393, 0.615232, 0.595699, 0.575808, 
0.555570, 0.534998, 0.514103, 0.492898, 0.471397, 0.449611, 0.427555, 0.405241, 
0.382683, 0.359895, 0.336890, 0.313682, 0.290285, 0.266713, 0.242980, 0.219101, 
0.195090, 0.170962, 0.146730, 0.122411, 0.098017, 0.073565, 0.049068, 0.024541, 
0.000000, -0.024541, -0.049068, -0.073565, -0.098017, -0.122411, -0.146730, -0.170962, 
-0.195090, -0.219101, -0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895, 
-0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898, -0.514103, -0.534998, 
-0.555570, -0.575808, -0.595699, -0.615232, -0.634393, -0.653173, -0.671559, -0.689541, 
-0.707107, -0.724247, -0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585, 
-0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224, -0.903989, -0.914210, 
-0.923880, -0.932993, -0.941544, -0.949528, -0.956940, -0.963776, -0.970031, -0.975702, 
-0.980785, -0.985278, -0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699, 
-1.000000, -0.999699, -0.998795, -0.997290, -0.995185, -0.992480, -0.989177, -0.985278, 
-0.980785, -0.975702, -0.970031, -0.963776, -0.956940, -0.949528, -0.941544, -0.932993, 
-0.923880, -0.914210, -0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844854, 
-0.831470, -0.817585, -0.803208, -0.788346, -0.773010, -0.757209, -0.740951, -0.724247, 
-0.707107, -0.689541, -0.671559, -0.653173, -0.634393, -0.615232, -0.595699, -0.575808, 
-0.555570, -0.534998, -0.514103, -0.492898, -0.471397, -0.449611, -0.427555, -0.405241, 
-0.382683, -0.359895, -0.336890, -0.313682, -0.290285, -0.266713, -0.242980, -0.219101, 
-0.195090, -0.170962, -0.146730, -0.122411, -0.098017, -0.073565, -0.049068, -0.024541 } }
*/

textures/liquids_sd/siwa_water
{
	nocompress
	qer_editorimage textures/liquids_sd/siwa_water.tga
	qer_trans .5
	q3map_globaltexture
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nomarks
	cull disable
	nopicmip
	nofog

	waterfogvars ( 0.11 0.13 0.15 ) 0.2

	bumpmap textures/liquids_sd/siwa_water_n.tga

	{ 
		stage liquidmap
		fog on
		map textures/liquids_sd/siwa_water_n.tga
		// these are new default values of Renderer_liquid
		//refractionIndex 1.3 // water
        //fresnelPower 2.0
        //fresnelScale 0.85       // + sinTable[time * 0.4] * 0.25
        //fresnelBias  0.05
		blendFunc blend
		alphaFunc GE128
		depthWrite
		rgbgen identity
		tcmod scale 0.5 0.5
		tcmod scroll -.02 .001
		color 0.1, 0.1, 0.1, 0.5 // underwater fog color
	}

	{ 
		fog on
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.3 0.02 0 0.25 
		tcmod scale 0.01 0.01
		tcmod scroll -.001 -.0002
	}
	
	{
		fog on
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.1 0.03 0 0.4
		tcmod scale 1 1
		tcmod scroll -.005 -.001
	}
	{
		fog on
		map textures/liquids_sd/siwa_shimshim1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.4 0.02 0 0.3
		tcmod transform 0 1.5 1 1.5 2 1
		tcmod scroll .005 -.001
	}
}

textures/liquids_sd/siwa_water_2
{
	qer_editorimage textures/liquids_sd/siwa_water.tga
	qer_trans .5
	q3map_globaltexture
	cull disable
	nocompress
	nofog
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm water
	waterfogvars ( 0.11 0.13 0.15 ) 0.2
	color 0.1, 0.1, 0.1, 0.5 // underwater fog color

	bumpmap textures/liquids_sd/siwa_water_n.tga

	nopicmip
	{
		stage liquidmap
		map textures/liquids_sd/siwa_water_n.tga
		blendFunc blend
		alphaFunc GE128
		rgbgen identity
		tcmod scale 0.5 0.5
		tcmod scroll -.02 .001
		fog on
	}
	{
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.3 0.02 0 0.25
		tcmod scale 0.01 0.01
		tcmod scroll -.001 -.0002
		fog on
	}
	{
		map textures/liquids_sd/seawall_ripple1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.1 0.03 0 0.4
		tcmod scale 1 1
		tcmod scroll -.005 -.001
		fog on
	}
	{
		map textures/liquids_sd/siwa_shimshim1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.4 0.02 0 0.3
		tcmod transform 0 1.5 1 1.5 2 1
		tcmod scroll .005 -.001
		fog on
	}
}

textures/liquids_sd/siwa_waternodraw
{
	qer_editorimage textures/liquids_sd/siwa_waternodraw.tga
	qer_trans .75
	surfaceparm nodraw
	surfaceparm water
}
