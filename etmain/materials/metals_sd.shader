// metals_sd.shader

textures/metals_sd/grate_a
{
    qer_editorimage textures/metals_sd/grate_a.tga
	diffusemap textures/metals_sd/grate_a.tga
	bumpmap textures/metals_sd/grate_a_n.tga
	specularmap textures/metals_sd/grate_a_r.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/grate_b
{
	qer_editorimage textures/metals_sd/grate_b.tga
    surfaceparm alphashadow
	surfaceparm metalsteps
	nomipmaps
	nopicmip
	cull disable
	{
		stage diffusemap
	    map textures/metals_sd/grate_b.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
    }
	{
		stage bumpmap
	    map textures/metals_sd/grate_b_n.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
    }
	{
		stage specularmap
	    map textures/metals_sd/grate_b_r.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
    }
}

metals_sd/detail_b
{
    qer_editorimage metals_sd/detail_b.tga
	diffusemap metals_sd/detail_b.tga
	bumpmap metals_sd/detail_b_n.tga
	specularmap metals_sd/detail_b_r.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/door_a
{
    qer_editorimage textures/metals_sd/door_a.tga
	diffusemap textures/metals_sd/door_a.tga
	bumpmap textures/metals_sd/door_a_n.tga
	specularmap textures/metals_sd/door_a_r.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/duct_a2
{
    qer_editorimage textures/metals_sd/duct_a2.tga
	diffusemap textures/metals_sd/duct_a2.tga
	bumpmap textures/metals_sd/duct_a2_n.tga
	specularmap textures/metals_sd/duct_a2_r.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/metals_sd/wall_b
{
    qer_editorimage textures/metals_sd/wall_b.tga
	diffusemap textures/metals_sd/wall_b.tga
	bumpmap textures/metals_sd/wall_b_n.tga
	specularmap textures/metals_sd/wall_b_r.tga
	surfaceparm metalsteps
	implicitMap -
}