// rubble.shader
// modified 06.03.2017 Thunder

textures/rubble/burn_flr_m01
{
    qer_editorimage textures/rubble/burn_flr_m01.tga
	diffusemap textures/rubble/burn_flr_m01.tga
	bumpmap textures/rubble/burn_flr_m01_n.tga
	specularmap textures/rubble/burn_flr_m01_r.tga
	surfaceparm woodsteps
	implicitMap -
}

textures/rubble/burn_flr_m01a
{
	qer_editorimage textures/rubble/burn_flr_m01a.tga
	diffusemap textures/rubble/burn_flr_m01a.tga
	bumpmap textures/rubble/burn_flr_m01a_n.tga
	specularmap textures/rubble/burn_flr_m01a_r.tga
	surfaceparm pointlight
	surfaceparm woodsteps
	{
		map textures/props/ember1a.tga
		rgbGen wave sin 1 0.5 0 0.5
		tcmod rotate 3
	}
	{
		map textures/rubble/burn_flr_m01a.tga
		blendfunc blend
		rgbGen vertex
	}
}

textures/rubble/burn_flr_m01b
{
	qer_editorimage textures/rubble/burn_flr_m01a.tga
	diffusemap textures/rubble/burn_flr_m01a.tga
	bumpmap textures/rubble/burn_flr_m01a_n.tga
	specularmap textures/rubble/burn_flr_m01a_r.tga
	surfaceparm pointlight
	surfaceparm woodsteps
	{
		map textures/props/ember1a.tga
		rgbGen wave sin 1 0.45 0 0.5
		tcmod rotate -3
	}
	{
		map textures/rubble/burn_flr_m01a.tga
		blendfunc blend
		rgbGen vertex
	}
}

textures/rubble/burn_flr_m01c
{
	qer_editorimage textures/rubble/burn_flr_m01a.tga
	diffusemap textures/rubble/burn_flr_m01a.tga
	bumpmap textures/rubble/burn_flr_m01a_n.tga
	specularmap textures/rubble/burn_flr_m01a_r.tga
	surfaceparm pointlight
	surfaceparm woodsteps
	{
		map textures/props/ember1a.tga
		rgbGen wave sin 1 0.55 0 0.5
		tcmod rotate 3.2
	}
	{
		map textures/rubble/burn_flr_m01a.tga
		blendfunc blend
		rgbGen vertex
	}
}

textures/rubble/rebar_m01
{
    qer_editorimage textures/rubble/rebar_m01.tga
	diffusemap textures/rubble/rebar_m01.tga
	bumpmap textures/rubble/rebar_m01_n.tga
	specularmap textures/rubble/rebar_m01_r.tga
	cull none
	surfaceparm metalsteps
	implicitMask -
}

// one more
textures/rubble/debri_m01
{
    qer_editorimage textures/rubble/debri_m01.tga
	diffusemap textures/rubble/debri_m01.tga
	bumpmap textures/rubble/debri_m01_n.tga
	specularmap textures/rubble/debri_m01_r.tga
	implicitmask -
}
