textures/metals_sd/grate_a
{
    qer_editorimage textures/metals_sd/grate_a
	diffusemap textures/metals_sd/grate_a
	bumpmap textures/metals_sd/grate_a_n
	specularmap textures/metals_sd/grate_a_s
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/grate_b
{
	qer_editorimage textures/metals_sd/grate_b
    surfaceparm alphashadow
	surfaceparm metalsteps
	nomipmaps
	nopicmip
	cull disable
	{
		stage diffusemap
	    map textures/metals_sd/grate_b
		alphaFunc GE128
		depthWrite
		rgbGen vertex
    }
	{
		stage bumpmap
	    map textures/metals_sd/grate_b_n
		alphaFunc GE128
		depthWrite
		rgbGen vertex
    }
	{
		stage specularmap
	    map textures/metals_sd/grate_b_s
		alphaFunc GE128
		depthWrite
		rgbGen vertex
    }
}

metals_sd/detail_b
{
    qer_editorimage metals_sd/detail_b
	diffusemap metals_sd/detail_b
	bumpmap metals_sd/detail_b_n
	specularmap metals_sd/detail_b_s
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/door_a
{
    qer_editorimage textures/metals_sd/door_a
	diffusemap textures/metals_sd/door_a
	bumpmap textures/metals_sd/door_a_n
	specularmap textures/metals_sd/door_a_s
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/duct_a2
{
    qer_editorimage textures/metals_sd/duct_a2
	diffusemap textures/metals_sd/duct_a2
	bumpmap textures/metals_sd/duct_a2_n
	specularmap textures/metals_sd/duct_a2_s
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/wall_b
{
    qer_editorimage textures/metals_sd/wall_b
	diffusemap textures/metals_sd/wall_b
	bumpmap textures/metals_sd/wall_b_n
	specularmap textures/metals_sd/wall_b_s
	surfaceparm metalsteps
	implicitMap -
}