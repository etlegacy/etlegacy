textures/props_sd/board_cl01m
{
    qer_editorimage textures/props_sd/board_cl01m
	diffusemap textures/props_sd/board_cl01m
	bumpmap textures/props_sd/board_cl01m_n
	specularmap textures/props_sd/board_cl01m_s
	surfaceparm woodsteps
	implicitMap -
}

textures/props_sd/board_cl02m
{
    qer_editorimage textures/props_sd/board_cl02m
	diffusemap textures/props_sd/board_cl02m
	bumpmap textures/props_sd/board_cl02m_n
	specularmap textures/props_sd/board_cl02m_s
	surfaceparm woodsteps
	implicitMap -
}

textures/props_sd/s_ammo01
{
    qer_editorimage textures/props_sd/s_ammo01
	diffusemap textures/props_sd/s_ammo01
	bumpmap textures/props_sd/s_ammo01_n
	specularmap textures/props_sd/s_ammo01_S
	surfaceparm metalsteps
	implicitMap -
}

textures/props_sd/s_casemate01
{
    qer_editorimage textures/props_sd/s_casemate01
	diffusemap textures/props_sd/s_casemate01
	bumpmap textures/props_sd/s_casemate01_n
	specularmap textures/props_sd/s_casemate01_s
	surfaceparm metalsteps
	implicitMap -
}

textures/props_sd/s_casemate02
{
    qer_editorimage textures/props_sd/s_casemate02
	diffusemap textures/props_sd/s_casemate02
	bumpmap textures/props_sd/s_casemate02_n
	specularmap textures/props_sd/s_casemate02_s
	surfaceparm metalsteps
	implicitMap -
}

textures/props_sd/s_generator01
{
    qer_editorimage textures/props_sd/s_generator01
	diffusemap textures/props_sd/s_generator01
	bumpmap textures/props_sd/s_generator01_n
	specularmap textures/props_sd/s_generator01_s
	surfaceparm metalsteps
	implicitMap -
}

textures/props_sd/s_gun01
{
    qer_editorimage textures/props_sd/s_gun01
	diffusemap textures/props_sd/s_gun01
	bumpmap textures/props_sd/s_gun01_n
	specularmap textures/props_sd/s_gun01_s
	surfaceparm metalsteps
	implicitMap -
}



textures/props_sd/barrel_m01_rednwhite
{
    qer_editorimage textures/props_sd/barrel_m01_rednwhite
	diffusemap textures/props_sd/barrel_m01_rednwhite
	bumpmap textures/props_sd/barrel_m01_rednwhite_n
	specularmap textures/props_sd/barrel_m01_rednwhite_s
	surfaceparm metalsteps
	implicitMap -
}
// norm and spec doesnt seem to work here
textures/props_sd/wires
{
    qer_editorimage textures/props_sd/wires
	//diffusemap textures/props_sd/wires
	//bumpmap textures/props_sd/wires_n
	//specularmap textures/props_sd/wires_s
	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans
	surfaceparm nonsolid
	{
		map textures/props_sd/wires
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

textures/props_sd/wires01
{
    qer_editorimage textures/props_sd/wires01
	//diffusemap textures/props_sd/wires01
	//bumpmap textures/props_sd/wires01_n
	//specularmap textures/props_sd/wires01_s
	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans
	surfaceparm nonsolid
	{
		map textures/props_sd/wires01
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

//New shaders!
textures/props_sd/sign_radar
{
    qer_editorimage textures/props_sd/sign_radar
	diffusemap textures/props_sd/sign_radar
	bumpmap textures/props_sd/sign_radar_n
	specularmap textures/props_sd/sign_radar
	surfaceparm metalsteps
	implicitMap -
}

textures/props_sd/trim_c01w
{
    qer_editorimage textures/props_sd/trim_c01w
	diffusemap textures/props_sd/trim_c01w
	bumpmap textures/props_sd/trim_c01w_n
	specularmap textures/props_sd/trim_c01w_s
    implicitMap -
}
	
textures/props_sd/trim_c03w
{
    qer_editorimage textures/props_sd/trim_c03w
	diffusemap textures/props_sd/trim_c03w
	bumpmap textures/props_sd/trim_c03w_n
	specularmap textures/props_sd/trim_c03w_s
    implicitMap -
}

textures/props_sd/trim_c10w
{
    qer_editorimage textures/props_sd/trim_c10w
	diffusemap textures/props_sd/trim_c10w
	bumpmap textures/props_sd/trim_c10w_n
	specularmap textures/props_sd/trim_c10w_s
    implicitMap -
}

textures/props_sd/basketsand
{
    qer_editorimage textures/props_sd/basketsand
	diffusemap textures/props_sd/basketsand
	bumpmap textures/props_sd/basketsand_n
	specularmap textures/props_sd/basketsand_s
	surfaceparm gravelsteps
    implicitMap -
}