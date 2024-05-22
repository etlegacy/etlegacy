// xlab_props.shader

textures/xlab_props/panel_m01
{
    qer_editorimage textures/xlab_props/panel_m01.tga
	diffusemap textures/xlab_props/panel_m01.tga
	specularmap textures/xlab_props/panel_m01_r.tga
	bumpmap textures/xlab_props/panel_m01_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_props/panel_m01_light
{
	qer_editorimage textures/xlab_props/panel_m01.tga
	q3map_lightimage textures/xlab_props/softblue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 1000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/xlab_props/panel_m01.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen wave sin 1 .1 0 7
	}
}

textures/xlab_props/xdrawers_c03
{
    qer_editorimage textures/xlab_props/xdrawers_c03.tga
	diffusemap textures/xlab_props/xdrawers_c03.tga
	specularmap textures/xlab_props/xdrawers_c03_r.tga
	bumpmap textures/xlab_props/xdrawers_c03_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_props/xgrid_c01
{
    qer_editorimage textures/xlab_props/xgrid_c01.tga
	diffusemap textures/xlab_props/xgrid_c01.tga
	specularmap textures/xlab_props/xgrid_c01_r.tga
	bumpmap textures/xlab_props/xgrid_c01_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_props/xpanel_c02dm
{
    qer_editorimage textures/xlab_props/xpanel_c02dm.tga
	diffusemap textures/xlab_props/xpanel_c02dm.tga
	specularmap textures/xlab_props/xpanel_c02dm_r.tga
	bumpmap textures/xlab_props/xpanel_c02dm_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_props/xpanel_c05_light_half
{
    qer_editorimage textures/xlab_props/xpanel_c05_light_half.tga
	diffusemap textures/xlab_props/xpanel_c05_light_half.tga
	specularmap textures/xlab_props/xpanel_c05_light_half_r.tga
	bumpmap textures/xlab_props/xpanel_c05_light_half_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_props/xpanel_c08
{
    qer_editorimage textures/xlab_props/xpanel_c08.tga
	diffusemap textures/xlab_props/xpanel_c08.tga
	specularmap textures/xlab_props/xpanel_c08_r.tga
	bumpmap textures/xlab_props/xpanel_c08_n.tga
	surfaceparm metalsteps
	implicitMap -
}

textures/xlab_props/xpanel_c10_light
{
    qer_editorimage textures/xlab_props/xpanel_c10_light.tga
	diffusemap textures/xlab_props/xpanel_c10_light.tga
	specularmap textures/xlab_props/xpanel_c10_light_r.tga
	bumpmap textures/xlab_props/xpanel_c10_light_n.tga
	surfaceparm metalsteps
	implicitMap -
}
