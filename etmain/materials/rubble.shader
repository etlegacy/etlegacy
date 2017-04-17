// rubble.shader
// modified 06.03.2017 Thunder

textures/rubble/burn_flr_m01
{
    qer_editorimage textures/rubble/burn_flr_m01
	diffusemap textures/rubble/burn_flr_m01
	bumpmap textures/rubble/burn_flr_m01_n
	specularmap textures/rubble/burn_flr_m01_s
	surfaceparm woodsteps
	implicitMap -
}

textures/rubble/burn_flr_m01a
{
	qer_editorimage textures/rubble/burn_flr_m01a
	diffusemap textures/rubble/burn_flr_m01a
	bumpmap textures/rubble/burn_flr_m01a_n
	specularmap textures/rubble/burn_flr_m01a_s
	surfaceparm pointlight
	surfaceparm woodsteps
	{
		map textures/props/ember1a
		rgbGen wave sin 1 0.5 0 0.5
		tcmod rotate 3
	}
	{
		map textures/rubble/burn_flr_m01a
		blendfunc blend
		rgbGen vertex
	}
}

textures/rubble/burn_flr_m01b
{
	qer_editorimage textures/rubble/burn_flr_m01a
	diffusemap textures/rubble/burn_flr_m01a
	bumpmap textures/rubble/burn_flr_m01a_n
	specularmap textures/rubble/burn_flr_m01a_s
	surfaceparm pointlight
	surfaceparm woodsteps
	{
		map textures/props/ember1a
		rgbGen wave sin 1 0.45 0 0.5
		tcmod rotate -3
	}
	{
		map textures/rubble/burn_flr_m01a
		blendfunc blend
		rgbGen vertex
	}
}

textures/rubble/burn_flr_m01c
{
	qer_editorimage textures/rubble/burn_flr_m01a
	diffusemap textures/rubble/burn_flr_m01a
	bumpmap textures/rubble/burn_flr_m01a_n
	specularmap textures/rubble/burn_flr_m01a_s
	surfaceparm pointlight
	surfaceparm woodsteps
	{
		map textures/props/ember1a
		rgbGen wave sin 1 0.55 0 0.5
		tcmod rotate 3.2
	}
	{
		map textures/rubble/burn_flr_m01a
		blendfunc blend
		rgbGen vertex
	}
}

textures/rubble/rebar_m01
{
    qer_editorimage textures/rubble/rebar_m01
	diffusemap textures/rubble/rebar_m01
	bumpmap textures/rubble/rebar_m01_n
	specularmap textures/rubble/rebar_m01_s
	cull none
	surfaceparm metalsteps
	implicitMask -
}

// one more
textures/rubble/debri_m01
{
    qer_editorimage textures/rubble/debri_m01
	diffusemap textures/rubble/debri_m01
	bumpmap textures/rubble/debri_m01_n
	specularmap textures/rubble/debri_m01_s
	implicitmask -
}
