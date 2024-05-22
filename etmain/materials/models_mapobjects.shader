// models_mapobjects.shader

//archeology
models/mapobjects/archeology/jug
{
	diffuseMap models/mapobjects/archeology/jug.tga
	bumpMap models/mapobjects/archeology/jug_n.tga
	specularMap models/mapobjects/archeology/jug_r.tga
}

models/mapobjects/archeology/vase2
{
	diffuseMap models/mapobjects/archeology/vase2.tga
	bumpMap models/mapobjects/archeology/vase2_n.tga
	specularMap models/mapobjects/archeology/vase2_r.tga
}

models/mapobjects/archeology/vase3
{
	diffuseMap models/mapobjects/archeology/vase3.tga
	bumpMap models/mapobjects/archeology/vase3_n.tga
	specularMap models/mapobjects/archeology/vase3_r.tga
}

//**********************************************************
// barrel
models/mapobjects/barrel_sd/barrel_side
{
	qer_editorimage textures/props/barrel_m01.tga
	diffuseMap textures/props/barrel_m01.tga
	bumpMap textures/props/barrel_m01_n.tga
	specularMap textures/props/barrel_m01_r.tga
}

models/mapobjects/barrel_sd/barrel_top
{
	qer_editorimage textures/props/barrel_m02.tga
	diffuseMap textures/props/barrel_m02.tga
	bumpMap textures/props/barrel_m02_n.tga
	specularMap textures/props/barrel_m02_r.tga
}
//*****************************************************

// Blitz truck
// specular exception 

models/mapobjects/blitz_sd/blitz_sd_arches
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd.tga
	cull twosided
	diffuseMap models/mapobjects/blitz_sd/blitz_sd.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_arches_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd.tga
	cull twosided
	diffuseMap models/mapobjects/blitz_sd/blitz_sd.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_arches_s
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_s.tga
	cull twosided
	diffuseMap models/mapobjects/blitz_sd/blitz_sd_s.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_s_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_arches_s_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_s.tga
	cull twosided
	diffuseMap models/mapobjects/blitz_sd/blitz_sd_s.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_s_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_body
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd.tga
	diffuseMap models/mapobjects/blitz_sd/blitz_sd.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_body_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd.tga
	diffuseMap models/mapobjects/blitz_sd/blitz_sd.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_body_s
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_s.tga
	diffuseMap models/mapobjects/blitz_sd/blitz_sd_s.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_s_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_s_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_body_s_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_s.tga
	diffuseMap models/mapobjects/blitz_sd/blitz_sd_s.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_s_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_s_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_interior_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_interior02.tga
	cull twosided
	diffuseMap models/mapobjects/blitz_sd/blitz_sd_interior02.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_interior02_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_interior02_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_interior02_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_interior02.tga
	cull twosided
	diffuseMap models/mapobjects/blitz_sd/blitz_sd_interior02.tga
	bumpMap models/mapobjects/blitz_sd/blitz_sd_interior02_n.tga
	specularMap models/mapobjects/blitz_sd/blitz_sd_interior02_r.tga
}

models/mapobjects/blitz_sd/blitz_sd_windows
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd.tga
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/mapobjects/blitz_sd/blitz_sd.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbgen lightingDiffuse
	}
}

models/mapobjects/blitz_sd/blitz_sd_windows_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd.tga
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/mapobjects/blitz_sd/blitz_sd.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbgen lightingdiffuse
	}
}

models/mapobjects/blitz_sd/blitz_sd_windows_s
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_s.tga
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/mapobjects/blitz_sd/blitz_sd_s.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbgen lightingDiffuse
	}
}

models/mapobjects/blitz_sd/blitz_sd_windows_s_mm
{
	qer_editorimage models/mapobjects/blitz_sd/blitz_sd_s.tga
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/mapobjects/blitz_sd/blitz_sd_s.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbgen environment
	}
}

//*****************************************************
//book
models/mapobjects/book
{
	diffuseMap models/mapobjects/book.tga
	bumpMap models/mapobjects/book_n.tga
	specularMap models/mapobjects/book_r.tga
}
//*******************************************************

// Railgun tug and trailer
//**********************************************************************

models/mapobjects/cab_sd/wheels
{
	qer_editorimage models/mapobjects/cab_sd/wheels.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map models/mapobjects/cab_sd/wheels.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}

}

models/mapobjects/cab_sd/part1
{
	qer_editorimage models/mapobjects/cab_sd/part1.tga
	//{
	//	map $lightmap
	//	rgbGen identity
	//}
	{
		stage diffuseMap
		map models/mapobjects/cab_sd/part1.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		stage bumpMap
		map models/mapobjects/cab_sd/part1_n.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		stage specularMap
		map models/mapobjects/cab_sd/part1_r.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

models/mapobjects/cab_sd/part2	
{
	qer_editorimage models/mapobjects/cab_sd/part2.tga
	//{
	//	map $lightmap
	//	rgbGen identity
	//}
	{
		stage diffuseMap
		map models/mapobjects/cab_sd/part2.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		stage bumpMap
		map models/mapobjects/cab_sd/part2_n.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		stage specularMap
		map models/mapobjects/cab_sd/part2_r.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

models/mapobjects/cab_sd/trailer
{
	qer_editorimage models/mapobjects/cab_sd/trailer.tga
	//{
	//	map $lightmap
	//	rgbGen identity
	//}
	{
		stage diffuseMap
		map models/mapobjects/cab_sd/trailer.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		stage bumpMap
		map models/mapobjects/cab_sd/trailer_n.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		stage specularMap
		map models/mapobjects/cab_sd/trailer_r.tga
		//blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}
//***************************************************************
// CMARKER
models/mapobjects/cmarker/c_box
{
	qer_editorimage models/mapobjects/cmarker/box_m05.tga
	diffuseMap models/mapobjects/cmarker/box_m05.tga
	bumpMap models/mapobjects/cmarker/box_m05_n.tga
	specularMap models/mapobjects/cmarker/box_m05_r.tga
}

models/mapobjects/cmarker/c_box_allied
{
	qer_editorimage models/mapobjects/cmarker/allied_crate.tga
	diffuseMap models/mapobjects/cmarker/allied_crate.tga
	bumpMap models/mapobjects/cmarker/allied_crate_n.tga
	specularMap models/mapobjects/cmarker/allied_crate_r.tga
}

models/mapobjects/cmarker/c_box_axis
{
	qer_editorimage models/mapobjects/cmarker/axis_crate.tga
	diffuseMap models/mapobjects/cmarker/axis_crate.tga
	bumpMap models/mapobjects/cmarker/axis_crate_n.tga
	specularMap models/mapobjects/cmarker/axis_crate_r.tga
}

models/mapobjects/cmarker/c_box_neutral
{
	qer_editorimage models/mapobjects/cmarker/neutral_crate.tga
	diffuseMap models/mapobjects/cmarker/neutral_crate.tga
	bumpMap models/mapobjects/cmarker/neutral_crate_n.tga
	specularMap models/mapobjects/cmarker/neutral_crate_r.tga
}

models/mapobjects/cmarker/c_sandbag_allied
{
	qer_editorimage models/mapobjects/cmarker/allied_sack.tga
	diffuseMap models/mapobjects/cmarker/allied_sack.tga
	bumpMap models/mapobjects/cmarker/allied_sack_n.tga
	   
}

models/mapobjects/cmarker/c_sandbag_axis
{
	qer_editorimage models/mapobjects/cmarker/axis_sack.tga
	diffuseMap models/mapobjects/cmarker/axis_sack.tga
	bumpMap models/mapobjects/cmarker/axis_sack_n.tga

}

models/mapobjects/cmarker/c_shovel
{
	qer_editorimage models/mapobjects/cmarker/shovel.tga
	diffuseMap models/mapobjects/cmarker/shovel.tga
	bumpMap models/mapobjects/cmarker/shovel_n.tga
	specularMap models/mapobjects/cmarker/shovel_r.tga
}
//saving 4 images here, using allied n and s for all
models/mapobjects/cmarker/cflag_allied
{
	qer_editorimage models/mapobjects/cmarker/cflagallied.tga
	cull twosided
	deformVertexes wave 194 sin 0 3 0 .4
	
     diffuseMap models/mapobjects/cmarker/cflagallied.tga
     bumpMap models/mapobjects/cmarker/cflagallied_n.tga
	 specularMap models/mapobjects/cmarker/cflagallied_r.tga
	 implicitMask models/mapobjects/cmarker/cflagallied.tga
	
	
}

models/mapobjects/cmarker/cflag_axis
{
	qer_editorimage models/mapobjects/cmarker/cflagaxis.tga
	cull twosided
	deformVertexes wave 194 sin 0 3 0 .4
	diffuseMap models/mapobjects/cmarker/cflagaxis.tga
	bumpMap models/mapobjects/cmarker/cflagallied_n.tga
	specularMap models/mapobjects/cmarker/cflagallied_r.tga
	implicitMask models/mapobjects/cmarker/cflagaxis.tga
}

models/mapobjects/cmarker/cflag_neutral
{
	qer_editorimage models/mapobjects/cmarker/cflagneutral.tga
	cull twosided
	deformVertexes wave 194 sin 0 3 0 .4
	diffuseMap models/mapobjects/cmarker/cflagneutral.tga
	bumpMap models/mapobjects/cmarker/cflagallied_n.tga
	specularMap models/mapobjects/cmarker/cflagallied_r.tga
	implicitMask models/mapobjects/cmarker/cflagneutral.tga
}
//*****************************************************************
//debris
models/mapobjects/debris/personaleffects
{
	diffuseMap models/mapobjects/debris/personaleffects.tga
	bumpMap models/mapobjects/debris/personaleffects_n.tga
	specularMap models/mapobjects/debris/personaleffects_r.tga
}

//********************************************************************

// dinghy_sd
models/mapobjects/dinghy_sd/dinghy
{
	diffuseMap models/mapobjects/dinghy_sd/dinghy.tga
	bumpMap models/mapobjects/dinghy_sd/dinghy_n.tga
	specularMap models/mapobjects/dinghy_sd/dinghy_r.tga
}
//*******************************************************************
//models\mapobjects\electronics

models/mapobjects/electronics/loudspeaker2
{
	diffuseMap models/mapobjects/electronics/loudspeaker2.tga
	bumpMap models/mapobjects/electronics/loudspeaker2_n.tga
	specularMap models/mapobjects/electronics/loudspeaker2_r.tga
}

models/mapobjects/electronics/radar_01
{
	diffuseMap models/mapobjects/electronics/radar_01.tga
	bumpMap models/mapobjects/electronics/radar_01_n.tga
	specularMap models/mapobjects/electronics/radar_01_r.tga
	//implicitMask models/mapobjects/electronics/radar_01
}

models/mapobjects/electronics/tele
{
	diffuseMap models/mapobjects/electronics/tele.tga
	bumpMap models/mapobjects/electronics/tele_n.tga
	specularMap models/mapobjects/electronics/tele_r.tga
}
//********************************************************************
//doesnt exist, yet......
//flags
//models/mapobjects/flag/flag_allied
//{
//	cull disable
//	nomipmaps
//	nopicmip
//	implicitMap -
//}

//models/mapobjects/flag/flag_axis
//{
//	cull disable
//	nomipmaps
//	nopicmip
//	implicitMap -
//}

models/mapobjects/flag/flag_dam
{
	diffuseMap models/mapobjects/flag/flag_dam.tga
	bumpMap models/mapobjects/flag/flag_dam_n.tga
	specularMap models/mapobjects/flag/flag_dam_r.tga
	cull disable
	nomipmaps
	nopicmip
	//implicitMap - drop in lightmap?
}

//***********************************************************

//furniture
//this is shadow and will not have bumps
models/mapobjects/furniture/sherman_s
{
	{
		map models/mapobjects/furniture/sherman_s.tga
		blendfunc blend
		rgbGen identity
	}
}

models/mapobjects/furniture/xsink
{
	diffuseMap  models/mapobjects/furniture/xsink.tga
	bumpMap models/mapobjects/furniture/xsink_n.tga
	specularMap models/mapobjects/furniture/xsink_r.tga
}

models/mapobjects/furniture/xsink_fac
{
	diffuseMap models/mapobjects/furniture/xsink_fac.tga
	bumpMap models/mapobjects/furniture/xsink_fac_n.tga
	specularMap models/mapobjects/furniture/xsink_fac_r.tga
}

models/mapobjects/furniture/chair1
{
	diffuseMap models/mapobjects/furniture/chair1.tga
	bumpMap models/mapobjects/furniture/chair1_n.tga
	specularMap models/mapobjects/furniture/chair1_r.tga
}
models/mapobjects/furniture/chairmetal
{
	diffuseMap models/mapobjects/furniture/chairmetal.tga
	bumpMap models/mapobjects/furniture/chairmetal_n.tga
	specularMap models/mapobjects/furniture/chairmetal_r.tga
}
models/mapobjects/furniture/clubchair
{
	diffuseMap models/mapobjects/furniture/clubchair.tga
	bumpMap models/mapobjects/furniture/clubchair_n.tga
	specularMap models/mapobjects/furniture/clubchair_r.tga
}
//fire image for furnace, no normalmaps
//models/mapobjects/furniture/fire

models/mapobjects/furniture/furnace
{
	diffuseMap models/mapobjects/furniture/furnace.tga
	bumpMap models/mapobjects/furniture/furnace_n.tga
	specularMap models/mapobjects/furniture/furnace_r.tga
}
models/mapobjects/furniture/hibackchair_a
{
	diffuseMap models/mapobjects/furniture/hibackchair_a.tga
	bumpMap models/mapobjects/furniture/hibackchair_a_n.tga
	specularMap models/mapobjects/furniture/hibackchair_a_r.tga
}
models/mapobjects/furniture/silverware
{
	diffuseMap models/mapobjects/furniture/silverware.tga
	bumpMap models/mapobjects/furniture/silverware_n.tga
	specularMap models/mapobjects/furniture/silverware_r.tga
}
models/mapobjects/furniture/trim_c01
{
	diffuseMap models/mapobjects/furniture/trim_c01.tga
	bumpMap models/mapobjects/furniture/trim_c01_n.tga
	specularMap models/mapobjects/furniture/trim_c01_r.tga
}
models/mapobjects/furniture/type
{
	diffuseMap models/mapobjects/furniture/type.tga
	bumpMap models/mapobjects/furniture/type_n.tga
	specularMap models/mapobjects/furniture/type_r.tga
}
models/mapobjects/furniture/wood_c05
{
	diffuseMap models/mapobjects/furniture/wood_c05.tga
	bumpMap models/mapobjects/furniture/wood_c05_n.tga
	specularMap models/mapobjects/furniture/wood_c05_r.tga
}
models/mapobjects/furniture/wood1
{
	diffuseMap models/mapobjects/furniture/wood1.tga
	bumpMap models/mapobjects/furniture/wood1_n.tga
	specularMap models/mapobjects/furniture/wood1_r.tga
}

//******************************************************************

//GOLDBOX_SD
models/mapobjects/goldbox_sd/goldbox
{
	diffuseMap models/mapobjects/goldbox_sd/goldbox.tga
	bumpMap models/mapobjects/goldbox_sd/goldbox_n.tga
	specularMap models/mapobjects/goldbox_sd/goldbox_r.tga
}

//******************************************************************

//lights, dont have bumps
models/mapobjects/xlab_props/light_1
{
	qer_editorimage models/mapobjects/xlab_props/light.tga
    surfaceparm nomarks
    surfaceparm alphashadow
 	surfaceparm nolightmap
	implicitMap models/mapobjects/xlab_props/light.tga
}

models/mapobjects/xlab_props/light_1_oasis
{
	qer_editorimage models/mapobjects/xlab_props/light.tga
    surfaceparm nomarks
    surfaceparm alphashadow
 	surfaceparm nolightmap
	surfaceparm trans
	{
		map models/mapobjects/xlab_props/light.tga
		rgbGen identity
	}
}

models/mapobjects/lamps/bel_lamp2k
{
	qer_editorimage models/mapobjects/light/bel_lamp.tga
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 1000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/lamps/bel_lamp2k_gm		// for misc_gamemodel
{
	qer_editorimage models/mapobjects/light/bel_lamp.tga
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 1000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbgen lightingDiffuse
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/bel_lamp
{
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 5000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/bel_lamp_gm		// for misc_gamemodels
{
	qer_editorimage models/mapobjects/light/bel_lamp.tga
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 5000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbgen lightingDiffuse
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/bel_lampb
{
	qer_editorimage models/mapobjects/light/bel_lamp.tga
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 10000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/bel_lampm
{
	qer_editorimage models/mapobjects/light/bel_lamp.tga
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 7000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/bel_lampm_gm		// for misc_gamemodels
{
	qer_editorimage models/mapobjects/light/bel_lamp.tga
	q3map_lightimage models/colors/amber.tga
	q3map_surfaceLight 7000
	surfaceparm nomarks
	{
		map models/mapobjects/light/bel_lamp.tga
		rgbgen lightingDiffuse
	}
	{
		map models/mapobjects/light/bel_lamp.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_lightn
{
	q3map_lightimage models/mapobjects/light/cage_lightn.tga
	q3map_lightrgb 0.80 0.72 0.60
	surfaceparm nomarks
	{
		map models/mapobjects/light/cage_lightn.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cage_light.blendn.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_lightna
{
	cull twosided
	surfaceparm nomarks
	implicitMask models/mapobjects/light/cage_lightna.tga
}

models/mapobjects/light/cage_light1k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 1000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light2k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light3k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 3000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light4k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 4000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light5k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 5000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light7k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 7000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light9k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 9000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cage_light12k
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 12000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cagelight_a
{
	q3map_lightimage models/mapobjects/light/cagelight_a.tga
	q3map_lightrgb 0.80 0.72 0.60
	q3map_surfacelight 7000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_a.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blenda.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/cagelight_r
{
	q3map_lightimage models/mapobjects/light/cagelight_r.tga
	q3map_lightrgb 1.0 0.1 0.1
	q3map_surfacelight 7000
	surfaceparm nomarks
	{
		map models/mapobjects/light/cagelight_r.tga
		rgbGen vertex
	}
	{
		map models/mapobjects/light/cagelight.blendr.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/chandlier4
{
	qer_alphafunc gequal 0.5
	cull disable
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/light/light_m4
{
	q3map_lightimage models/mapobjects/light/light_m4.tga
	q3map_surfacelight 5000
	surfaceparm nomarks
	{
		map models/mapobjects/light/light_m4.tga
		rgbGen vertex
	}
	{
		map textures/lights/light_m4.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

models/mapobjects/light/pendant10k
{
	qer_editorimage models/mapobjects/light/pendant_sd.tga
	implicitMap models/mapobjects/light/pendant_sd.tga
	q3map_lightimage models/mapobjects/light/pendant_sd.tga
	q3map_surfacelight 10000
	surfaceparm nomarks
}

models/mapobjects/light/sconce
{
	qer_alphafunc gequal 0.5
	qer_trans 0.5
	cull disable
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/light/sconce2
{
	q3map_lightimage models/mapobjects/light/sconce2.tga
	q3map_surfacelight 5000
	surfaceparm nomarks
	{
		map models/mapobjects/light/sconce2.tga
		rgbGen identity
	}
}

models/mapobjects/light/sd_sconce
{
	qer_alphafunc gequal 0.5
	cull disable
	surfaceparm nomarks
	implicitMask -
}


//**************************************************************

//logs_sd
models/mapobjects/logs_sd/log
{
   diffuseMap models/mapobjects/logs_sd/log.tga
   bumpMap models/mapobjects/logs_sd/log_n.tga
   specularMap models/mapobjects/logs_sd/log_r.tga
   implicitMap models/mapobjects/logs_sd/log.tga
}
models/mapobjects/logs_sd/ring
{
	diffuseMap models/mapobjects/logs_sd/ring.tga
	bumpMap models/mapobjects/logs_sd/ring_n.tga
	specularMap models/mapobjects/logs_sd/ring_r.tga
        implicitMap models/mapobjects/logs_sd/ring.tga
}

models/mapobjects/logs_sd/snow
{
	diffuseMap textures/snow_sd/snow_var01.tga
	bumpMap textures/snow_sd/snow_var01_n.tga
	specularMap textures/snow_sd/snow_var01_r.tga
        implicitMap textures/snow_sd/snow_var01.tga
}
models/mapobjects/logs_sd/trunk_cut_snow
{
	diffuseMap models/mapobjects/logs_sd/trunk_cut_snow.tga
	bumpMap models/mapobjects/logs_sd/trunk_cut_snow_n.tga
	specularMap models/mapobjects/logs_sd/trunk_cut_snow_r.tga
        implicitMap models/mapobjects/logs_sd/trunk_cut_snow.tga
}

models/mapobjects/logs_sd/trunk_snow
{
	diffuseMap models/mapobjects/logs_sd/trunk_snow.tga
	bumpMap models/mapobjects/logs_sd/trunk_snow_n.tga
	specularMap models/mapobjects/logs_sd/trunk_snow_r.tga
        implicitMap models/mapobjects/logs_sd/trunk_snow.tga
}

models/mapobjects/logs_sd/winterbranch01
{
	diffuseMap models/mapobjects/logs_sd/winterbranch01.tga
	bumpMap models/mapobjects/logs_sd/winterbranch01_n.tga
	specularMap models/mapobjects/logs_sd/winterbranch01_r.tga
        implicitMap models/mapobjects/logs_sd/winterbranch01.tga
}

models/mapobjects/logs_sd/wintertrunk01
{
	diffuseMap models/mapobjects/logs_sd/wintertrunk01.tga
	bumpMap models/mapobjects/logs_sd/wintertrunk01_n.tga
	specularMap models/mapobjects/logs_sd/wintertrunk01_r.tga
        implicitMap models/mapobjects/logs_sd/wintertrunk01.tga
}

//************************************************************************

//miltary_trim
models/mapobjects/miltary_trim/bags1_s2
{
	diffuseMap models/mapobjects/miltary_trim/bags1_s2.tga
	bumpMap models/mapobjects/miltary_trim/bags1_s2_n.tga
	specularMap models/mapobjects/miltary_trim/bags1_s2_r.tga
}

models/mapobjects/miltary_trim/metal_m05
{
	diffuseMap models/mapobjects/miltary_trim/metal_m05.tga
	bumpMap models/mapobjects/miltary_trim/metal_m05_n.tga
	specularMap models/mapobjects/miltary_trim/metal_m05_r.tga
	implicitMask -
}

models/mapobjects/miltary_trim/bags1_s_wils
{
	diffuseMap models/mapobjects/miltary_trim/bags1_s2.tga
	bumpMap models/mapobjects/miltary_trim/bags1_s2_n.tga
	specularMap models/mapobjects/miltary_trim/bags1_s2_r.tga
}

models/mapobjects/miltary_trim/metal_m05_wils
{
	qer_editorimage models/mapobjects/miltary_trim/metal_m05
	cull disable
	diffuseMap models/mapobjects/miltary_trim/metal_m05.tga
	bumpMap models/mapobjects/miltary_trim/metal_m05_n.tga
	specularMap models/mapobjects/miltary_trim/metal_m05_r.tga
	implicitMask -
}
//**************************************************************************

// Pak guns
// specular exception because of _s suffix

models/mapobjects/pak75_sd/pak75
{
	diffuseMap models/mapobjects/pak75_sd/pak75.tga
	bumpMap models/mapobjects/pak75_sd/pak75_n.tga
	specularMap models/mapobjects/pak75_sd/pak75_r.tga
}

models/mapobjects/pak75_sd/pak75_s
{
	diffuseMap models/mapobjects/pak75_sd/pak75_s.tga
	bumpMap models/mapobjects/pak75_sd/pak75_n.tga
	specularMap models/mapobjects/pak75_sd/pak75_r.tga
}

models/mapobjects/pak75_sd/pak75-a
{
	diffuseMap models/mapobjects/pak75_sd/pak75-a.tga
	bumpMap models/mapobjects/pak75_sd/pak75-a_n.tga
	specularMap models/mapobjects/pak75_sd/pak75-a_r.tga
	implicitMask -
}

models/mapobjects/pak75_sd/pak75-a_s
{
	diffuseMap models/mapobjects/pak75_sd/pak75-a_s.tga
	bumpMap models/mapobjects/pak75_sd/pak75-a_n.tga
	specularMap models/mapobjects/pak75_sd/pak75-a_r.tga
	implicitMask -
}

//**********************************************************************

// plants_sd
models/mapobjects/plants_sd/bush_desert1
{
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/bush_desert1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/bush_desert1.tga
	bumpMap models/mapobjects/plants_sd/bush_desert1_n.tga
	specularMap models/mapobjects/plants_sd/bush_desert1_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	nopicmip
	implicitMask -
}

models/mapobjects/plants_sd/bush_desert2
{ 
     qer_alphafunc greater 0.5 
     qer_editorimage models/mapobjects/plants_sd/bush_desert2.tga
     cull disable 
     q3map_bounceScale 0.25
     diffuseMap models/mapobjects/plants_sd/bush_desert2.tga
	 bumpMap models/mapobjects/plants_sd/bush_desert2_n.tga
	 specularMap models/mapobjects/plants_sd/bush_desert2_r.tga
     surfaceparm alphashadow 
     surfaceparm nomarks 
     surfaceparm pointlight 
     surfaceparm trans 
     nopicmip 
     implicitMask - 
}

models/mapobjects/plants_sd/bush_snow1
{
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/bush_snow1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/bush_snow1.tga
	bumpMap models/mapobjects/plants_sd/bush_snow1_n.tga
	specularMap models/mapobjects/plants_sd/bush_snow1_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	nopicmip
	implicitMask -
}

models/mapobjects/plants_sd/bush_snow2
{
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/bush_snow2.tga
	diffuseMap models/mapobjects/plants_sd/bush_snow2.tga
	bumpMap models/mapobjects/plants_sd/bush_snow2_n.tga
	specularMap models/mapobjects/plants_sd/bush_snow2_r.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	nopicmip
	implicitMask -
}

models/mapobjects/plants_sd/catail1
{
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/catail1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/catail1.tga
	bumpMap models/mapobjects/plants_sd/catail1_n.tga
	specularMap models/mapobjects/plants_sd/catail1_r.tga
	nopicmip
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	nopicmip
	implicitMask -
}

models/mapobjects/plants_sd/catail2
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/catail2.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/catail2.tga
	bumpMap models/mapobjects/plants_sd/catail2_n.tga
	specularMap models/mapobjects/plants_sd/catail2_r.tga
	nopicmip
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	deformVertexes wave 15 sin 0 1 0 0.25
	implicitMask -
}

models/mapobjects/plants_sd/catail3
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/catail3.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/catail3.tga
	bumpMap models/mapobjects/plants_sd/catail3_n.tga
	specularMap models/mapobjects/plants_sd/catail3_r.tga
	nopicmip
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	deformVertexes wave 15 sin 0 1 0 0.25
	implicitMask -
}

models/mapobjects/plants_sd/deadbranch1
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/deadbranch1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/deadbranch1.tga
	bumpMap models/mapobjects/plants_sd/deadbranch1_n.tga
	specularMap models/mapobjects/plants_sd/deadbranch1_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/deadbranch2
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/deadbranch2.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/deadbranch2.tga
	bumpMap models/mapobjects/plants_sd/deadbranch2_n.tga
	specularMap models/mapobjects/plants_sd/deadbranch2_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/deadbranch3
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/deadbranch3.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/deadbranch3.tga
	bumpMap models/mapobjects/plants_sd/deadbranch3_n.tga
	specularMap models/mapobjects/plants_sd/deadbranch3_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/deadbranch1_damp
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/deadbranch1_damp.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/deadbranch1_damp.tga
	bumpMap models/mapobjects/plants_sd/deadbranch1_damp_n.tga
	specularMap models/mapobjects/plants_sd/deadbranch1_damp_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/deadbranch2_damp
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/deadbranch2_damp.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/deadbranch2_damp.tga
	bumpMap models/mapobjects/plants_sd/deadbranch2_damp_n.tga
	specularMap models/mapobjects/plants_sd/deadbranch2_damp_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/deadbranch3_damp
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/deadbranch3_damp.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/deadbranch3_damp.tga
	bumpMap models/mapobjects/plants_sd/deadbranch3_damp_n.tga
	specularMap models/mapobjects/plants_sd/deadbranch3_damp_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

//models/mapobjects/plants_sd/shrub_green1
//{
//	nopicmip
//	qer_alphafunc greater 0.5
//	qer_editorimage models/mapobjects/plants_sd/shrub_green1.tga
//	cull disable
//	surfaceparm alphashadow
//	surfaceparm trans
//	surfaceparm nomarks
//	implicitMask -
//}


//models/mapobjects/plants_sd/shrub_green2
//{
//	nopicmip
//	qer_alphafunc greater 0.5
//	qer_editorimage models/mapobjects/plants_sd/shrub_green2.tga
//	cull disable
//	surfaceparm alphashadow
//	surfaceparm trans
//	surfaceparm nomarks
//	implicitMask -
//}



//models/mapobjects/plants_sd/leaf1
//{
//	nopicmip
//	qer_alphafunc greater 0.5
//	qer_editorimage models/mapobjects/plants_sd/leaf1.tga
//	cull disable
//	surfaceparm alphashadow
//	surfaceparm trans
//	surfaceparm nomarks
//	implicitMask -
//}

//models/mapobjects/plants_sd/leaf2
//{
//	nopicmip
//	qer_alphafunc greater 0.5
//	qer_editorimage models/mapobjects/plants_sd/leaf2.tga
//	cull disable
//	surfaceparm alphashadow
//	surfaceparm trans
//	surfaceparm nomarks
//	implicitMask -
//}

//models/mapobjects/plants_sd/leaf3
//{
//	nopicmip
//	qer_alphafunc greater 0.5
//	qer_editorimage models/mapobjects/plants_sd/leaf3.tga
//	cull disable
//	surfaceparm alphashadow
//	surfaceparm trans
//	surfaceparm nomarks
//	implicitMask -
//}

models/mapobjects/plants_sd/grass_dry1
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/grass_dry1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/grass_dry1.tga
	bumpMap models/mapobjects/plants_sd/grass_dry1_n.tga
	specularMap models/mapobjects/plants_sd/grass_dry1_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/grass_dry2
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/grass_dry2.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/grass_dry2.tga
	bumpMap models/mapobjects/plants_sd/grass_dry2_n.tga
	specularMap models/mapobjects/plants_sd/grass_dry2_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/grass_dry3
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/grass_dry3.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/grass_dry3.tga
	bumpMap models/mapobjects/plants_sd/grass_dry3_n.tga
	specularMap models/mapobjects/plants_sd/grass_dry3_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/grass_green1
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/grass_green1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/grass_green1.tga
	bumpMap models/mapobjects/plants_sd/grass_green1_n.tga
	specularMap models/mapobjects/plants_sd/grass_green1_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/grass_green2
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/grass_green2.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/grass_green2.tga
	bumpMap models/mapobjects/plants_sd/grass_green2_n.tga
	specularMap models/mapobjects/plants_sd/grass_green2_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/grass_green3
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/grass_green3.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/grass_green3.tga
	bumpMap models/mapobjects/plants_sd/grass_green3_n.tga
	specularMap models/mapobjects/plants_sd/grass_green3_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/mil1
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/mil1.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/mil1.tga
	bumpMap models/mapobjects/plants_sd/mil1_n.tga
	specularMap models/mapobjects/plants_sd/mil1_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/plants_sd/mil2
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/plants_sd/mil2.tga
	cull disable
	diffuseMap models/mapobjects/plants_sd/mil2.tga
	bumpMap models/mapobjects/plants_sd/mil2_n.tga
	specularMap models/mapobjects/plants_sd/mil2_r.tga
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

//models/mapobjects/plants_sd/grassfoliage1
//{
//	nopicmip
//	qer_alphafunc greater 0.5
//	qer_editorimage models/mapobjects/plants_sd/grassfoliage1.tga
//	cull disable
//	diffuseMap models/mapobjects/plants_sd/grassfoliage1.tga
//	bumpMap models/mapobjects/plants_sd/grassfoliage1_n.tga
//	specularMap models/mapobjects/plants_sd/grassfoliage1.tga
//	surfaceparm alphashadow
//	surfaceparm trans
//	surfaceparm nomarks
//	implicitMask -
//}

//********************************************************************************

// portable_radar_sd
models/mapobjects/portable_radar_sd/portable_radar_sd
{
	cull disable
	diffuseMap models/mapobjects/portable_radar_sd/portable_radar_sd.tga
	bumpMap models/mapobjects/portable_radar_sd/portable_radar_sd_n.tga
	specularMap models/mapobjects/portable_radar_sd/portable_radar_sd_r.tga
}

models/mapobjects/portable_radar_sd/portable_radar_t_sd
{
	diffuseMap models/mapobjects/portable_radar_sd/portable_radar_t_sd.tga
	bumpMap models/mapobjects/portable_radar_sd/portable_radar_t_sd_n.tga
	specularMap models/mapobjects/portable_radar_sd/portable_radar_t_sd_r.tga
	qer_alphafunc gequal 0.5
	cull disable
	surfaceparm nomarks
	implicitMask -
}

//******************************************************************************

//Props
models/mapobjects/props_sd/basket
{
	diffuseMap models/mapobjects/props_sd/basket.tga
	bumpMap models/mapobjects/props_sd/basket_n.tga
	specularMap models/mapobjects/props_sd/basket_r.tga
	surfaceparm nomarks
	implicitMap models/mapobjects/props_sd/basket
}

models/mapobjects/props_sd/basketsand
{
	qer_editorimage textures/props_sd/basketsand.tga
	diffuseMap textures/props_sd/basketsand.tga
	bumpMap textures/props_sd/basketsand_n.tga
	specularMap textures/props_sd/basketsand_r.tga
	implicitMap textures/props_sd/basketsand.tga
}

models/mapobjects/props_sd/sandlevel
{
	qer_editorimage textures/desert_sd/sand_disturb_desert.tga
	diffuseMap textures/desert_sd/sand_disturb_desert.tga
	bumpMap textures/desert_sd/sand_disturb_desert_n.tga
	
    implicitMap textures/desert_sd/sand_disturb_desert.tga
}

models/mapobjects/props_sd/lid
{
	surfaceparm nomarks
	implicitMap models/mapobjects/props_sd/lid.tga
	diffuseMap models/mapobjects/props_sd/lid.tga
	bumpMap models/mapobjects/props_sd/lid_n.tga
	specularMap models/mapobjects/props_sd/lid_r.tga
}

models/mapobjects/props_sd/bunk_sd1
{
	qer_editorimage textures/chat/bedlinen_c02.tga
	surfaceparm nomarks
	diffuseMap textures/chat/bedlinen_c02.tga
	bumpMap textures/chat/bedlinen_c02_n.tga
	implicitMap textures/chat/bedlinen_c02.tga
}

models/mapobjects/props_sd/bunk_sd2
{
	qer_editorimage models/mapobjects/furniture/wood1.tga
	surfaceparm nomarks
	diffuseMap models/mapobjects/furniture/wood1.tga
	bumpMap models/mapobjects/furniture/wood1_n.tga
	implicitMap models/mapobjects/furniture/wood1.tga
}

models/mapobjects/props_sd/bunk_sd3
{
	qer_editorimage textures/chat/bedlinenpillow_c02.tga
	surfaceparm nomarks
	diffuseMap textures/chat/bedlinenpillow_c02.tga
	bumpMap textures/chat/bedlinenpillow_c02_n.tga
	implicitMap textures/chat/bedlinenpillow_c02.tga
}

models/mapobjects/props_sd/drape_rug
{
	qer_editorimage textures/egypt_props_sd/siwa_carpet2.tga
	cull twosided
	surfaceparm nomarks
	diffuseMap textures/egypt_props_sd/siwa_carpet2.tga
	bumpMap textures/egypt_props_sd/siwa_carpet2_n.tga
	specularMap textures/egypt_props_sd/siwa_carpet2_r.tga
	implicitMap textures/egypt_props_sd/siwa_carpet2.tga
}

models/mapobjects/props_sd/drape_wood
{
	qer_editorimage textures/egypt_door_sd/siwa_door_neutral.tga
	surfaceparm nomarks
	diffuseMap textures/egypt_door_sd/siwa_door_neutral.tga
	bumpMap textures/egypt_door_sd/siwa_door_neutral_n.tga
	specularMap textures/egypt_door_sd/siwa_door_neutral_r.tga
	implicitMap textures/egypt_door_sd/siwa_door_neutral.tga
}

models/mapobjects/props_sd/vase
{
	surfaceparm nomarks
	diffuseMap models/mapobjects/props_sd/vase.tga
	bumpMap models/mapobjects/props_sd/vase_n.tga
	specularMap models/mapobjects/props_sd/vase_r.tga
	implicitMap models/mapobjects/props_sd/vase.tga
}

models/mapobjects/props_sd/fuel_can
{
	surfaceparm nomarks
	diffuseMap models/mapobjects/props_sd/fuel_can.tga
	bumpMap models/mapobjects/props_sd/fuel_can_n.tga
	specularMap models/mapobjects/props_sd/fuel_can_r.tga
	implicitMap models/mapobjects/props_sd/fuel_can.tga
}
//****************************************************************

// PUMP_SD
models/mapobjects/pump_sd/bottom
{
	 diffuseMap models/mapobjects/pump_sd/bottom.tga
	 specularMap models/mapobjects/pump_sd/bottom_r.tga
	 bumpMap models/mapobjects/pump_sd/bottom_n.tga		
}

models/mapobjects/pump_sd/top
{
	 diffuseMap models/mapobjects/pump_sd/top.tga
	 specularMap models/mapobjects/pump_sd/top_r.tga
	 bumpMap models/mapobjects/pump_sd/top_n.tga		
}

//********************************************************************

//radios_sd
// radio sign
models/mapobjects/radios_sd/sign
{
     implicitMask -
}

models/mapobjects/radios_sd/blue2
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_blue.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.8 9
	}
}

models/mapobjects/radios_sd/blue1
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_blue.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.2 12
	}
}

models/mapobjects/radios_sd/gold3
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_gold.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave sin 0 1 0 3
	}
}

models/mapobjects/radios_sd/gold2
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_gold.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.9 9
	}
}

models/mapobjects/radios_sd/gold1
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_gold.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0.2 0.8 0.5 13
	}
}

models/mapobjects/radios_sd/green5
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise  0.25 0.75 0 8
	}
}

models/mapobjects/radios_sd/green4
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0.2 0.8 0 10

	}
}

models/mapobjects/radios_sd/green3
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 9 0.66 12
	}
}

models/mapobjects/radios_sd/green2
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.33 5
	}
}

models/mapobjects/radios_sd/green1
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.15 20
	}
}

models/mapobjects/radios_sd/red4
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.8 9
	}
}

models/mapobjects/radios_sd/red3
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0 1 0.2 12
	}
}

models/mapobjects/radios_sd/red2
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave sin 0 1 0 3
	}
}

models/mapobjects/radios_sd/red1
{
	polygonOffset
	surfaceparm nomarks
	{
		map models/mapobjects/radios_sd/beep_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave noise 0.5 0.5 0.5 10
	}
}

models/mapobjects/radios_sd/screen_broken
{
	polygonOffset
	surfaceparm nomarks
	{
		map gfx/damage/glass_mrk.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/mapobjects/radios_sd/screen_circle_fx
{
    qer_editorimage models/mapobjects/radios_sd/screen_circle.tga
	surfaceparm nomarks
	polygonOffset

	{
		map models/mapobjects/radios_sd/radio_scroll1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.3 0.7 0 20
		tcMod scale  1 1.25
	}	
	{
		map models/mapobjects/radios_sd/radio_scroll2.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.3 0.7 0 20
		tcMod scale  3.25 1.25
		tcMod scroll 2 0
	}	
	{
		map models/mapobjects/radios_sd/screen_circle.tga
		blendfunc blend
		rgbGen lightingdiffuse
	}
}

models/mapobjects/radios_sd/screen_square_fx
{
    qer_editorimage models/mapobjects/radios_sd/screen_square.tga
	surfaceparm nomarks
	polygonOffset

	{
		map models/mapobjects/radios_sd/radio_scroll1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.3 0.7 0 20
	}		
	{
		map models/mapobjects/radios_sd/radio_scroll2.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.3 0.7 0 20
		tcMod scale  2 1
		tcMod scroll 3 0
	}	
	{
		map models/mapobjects/radios_sd/screen_square.tga
		blendfunc blend
		rgbGen lightingdiffuse
	}
}

models/mapobjects/radios_sd/grid
{
	qer_editorimage models/mapobjects/radios_sd/grid.tga
    cull twosided
	diffuseMap models/mapobjects/radios_sd/grid.tga
	bumpMap models/mapobjects/radios_sd/grid_n.tga
    implicitMask models/mapobjects/radios_sd/grid.tga
	surfaceparm nomarks
}

models/mapobjects/radios_sd/iron
{
    	qer_editorimage models/mapobjects/radios_sd/iron.tga
    	cull twosided
		diffuseMap models/mapobjects/radios_sd/iron.tga
		bumpMap models/mapobjects/radios_sd/iron_n.tga
		specularMap models/mapobjects/radios_sd/iron_r.tga
}

models/mapobjects/radios_sd/neutralcabinet
{
    	qer_editorimage models/mapobjects/radios_sd/neutralcabinet.tga
    	surfaceparm nomarks
    	implicitMap models/mapobjects/radios_sd/neutralcabinet.tga
}

models/mapobjects/radios_sd/axiscabinet
{
    	qer_editorimage models/mapobjects/radios_sd/axiscabinet.tga
    	surfaceparm nomarks
    	implicitMap models/mapobjects/radios_sd/axiscabinet.tga
}

models/mapobjects/radios_sd/crate1
{
     qer_editorimage models/mapobjects/radios_sd/crate.tga
     diffuseMap models/mapobjects/radios_sd/crate.tga
	 bumpMap models/mapobjects/radios_sd/crate_n.tga
	 specularMap models/mapobjects/radios_sd/crate_r.tga
     surfaceparm nomarks
}

models/mapobjects/radios_sd/command7a
{
     qer_editorimage models/mapobjects/radios_sd/command7a.tga
     surfaceparm nomarks
	 diffuseMap models/mapobjects/radios_sd/command7a.tga
	 bumpMap models/mapobjects/radios_sd/command7a_n.tga
	 specularMap models/mapobjects/radios_sd/command7a_r.tga
}

models/mapobjects/radios_sd/command7
{
    qer_editorimage models/mapobjects/radios_sd/command7.tga
    surfaceparm nomarks
	diffuseMap models/mapobjects/radios_sd/command7.tga
	bumpMap models/mapobjects/radios_sd/command7_n.tga
	specularMap models/mapobjects/radios_sd/command7_r.tga
}

models/mapobjects/radios_sd/command6a
{
    qer_editorimage models/mapobjects/radios_sd/command6a.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command6a.tga
	bumpMap models/mapobjects/radios_sd/command6a_n.tga
	specularMap models/mapobjects/radios_sd/command6a_r.tga
}

models/mapobjects/radios_sd/command6
{
    qer_editorimage models/mapobjects/radios_sd/command6.tga
    surfaceparm nomarks
	diffuseMap models/mapobjects/radios_sd/command6.tga
	bumpMap models/mapobjects/radios_sd/command6_n.tga
	specularMap models/mapobjects/radios_sd/command6_r.tga
}

models/mapobjects/radios_sd/command5a
{
    qer_editorimage models/mapobjects/radios_sd/command5a.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command5a.tga
	bumpMap models/mapobjects/radios_sd/command5a_n.tga
	specularMap models/mapobjects/radios_sd/command5a_r.tga
}

models/mapobjects/radios_sd/command5
{
    qer_editorimage models/mapobjects/radios_sd/command5.tga
    surfaceparm nomarks
	diffuseMap models/mapobjects/radios_sd/command5.tga
	bumpMap models/mapobjects/radios_sd/command5_n.tga
	specularMap models/mapobjects/radios_sd/command5_r.tga
}

models/mapobjects/radios_sd/command4a
{
    qer_editorimage models/mapobjects/radios_sd/command4a.tga
    surfaceparm nomarks
	diffuseMap models/mapobjects/radios_sd/command4a.tga
    bumpMap models/mapobjects/radios_sd/command4a_n.tga
	specularMap models/mapobjects/radios_sd/command4a_r.tga   
}

models/mapobjects/radios_sd/command4
{
    qer_editorimage models/mapobjects/radios_sd/command4.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command4.tga
	bumpMap models/mapobjects/radios_sd/command4_n.tga
	specularMap models/mapobjects/radios_sd/command4_r.tga
}

models/mapobjects/radios_sd/command3a
{
    qer_editorimage models/mapobjects/radios_sd/command3a.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command3a.tga
	bumpMap models/mapobjects/radios_sd/command3a_n.tga
	specularMap models/mapobjects/radios_sd/command3a_r.tga
}

models/mapobjects/radios_sd/command3
{
    qer_editorimage models/mapobjects/radios_sd/command3.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command3.tga
	bumpMap models/mapobjects/radios_sd/command3_n.tga
	specularMap models/mapobjects/radios_sd/command3_r.tga
}

models/mapobjects/radios_sd/command2a
{
    qer_editorimage models/mapobjects/radios_sd/command2a.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command2a.tga
	bumpMap models/mapobjects/radios_sd/command2a_n.tga
	specularMap models/mapobjects/radios_sd/command2a_r.tga
}

models/mapobjects/radios_sd/command2
{
    qer_editorimage models/mapobjects/radios_sd/command2.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command2.tga
	bumpMap models/mapobjects/radios_sd/command2_n.tga
	specularMap models/mapobjects/radios_sd/command2_r.tga
}

models/mapobjects/radios_sd/command1a
{
    qer_editorimage models/mapobjects/radios_sd/command1a.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/command1a.tga
	bumpMap models/mapobjects/radios_sd/command1a_n.tga
	specularMap models/mapobjects/radios_sd/command1a_r.tga
}

models/mapobjects/radios_sd/command1
{
    qer_editorimage models/mapobjects/radios_sd/command1.tga
    surfaceparm nomarks
	diffuseMap models/mapobjects/radios_sd/command1.tga
	bumpMap models/mapobjects/radios_sd/command1_n.tga
	specularMap models/mapobjects/radios_sd/command1_r.tga
}

models/mapobjects/radios_sd/allied_sign
{
    qer_editorimage models/mapobjects/radios_sd/allied_sign.tga
    surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/allied_sign.tga
	bumpMap models/mapobjects/radios_sd/allied_sign_n.tga
	specularMap models/mapobjects/radios_sd/allied_sign_r.tga
	implicitmask models/mapobjects/radios_sd/allied_sign.tga
}

models/mapobjects/radios_sd/neutral_sign
{
	qer_editorimage models/mapobjects/radios_sd/neutral_sign.tga
	surfaceparm nomarks
    diffuseMap models/mapobjects/radios_sd/neutral_sign.tga
	bumpMap models/mapobjects/radios_sd/neutral_sign_n.tga
	specularMap models/mapobjects/radios_sd/neutral_sign_r.tga
	implicitmask models/mapobjects/radios_sd/neutral_sign.tga
}

models/mapobjects/radios_sd/axis_sign
{
     qer_editorimage models/mapobjects/radios_sd/axis_sign.tga
     surfaceparm nomarks
     diffuseMap models/mapobjects/radios_sd/axis_sign.tga
	 bumpMap models/mapobjects/radios_sd/axis_sign_n.tga
	 specularMap models/mapobjects/radios_sd/axis_sign_r.tga
	 implicitmask models/mapobjects/radios_sd/axis_sign.tga
}
//***********************************************************************

//Raster (bmw motorcycle)
models/mapobjects/raster/moto
{
	cull twosided
	diffuseMap models/mapobjects/raster/moto.tga
	bumpMap models/mapobjects/raster/moto_n.tga
	specularMap models/mapobjects/raster/moto_r.tga
	implicitmask -
}

models/mapobjects/raster/moto_bag
{
	diffuseMap models/mapobjects/raster/moto_bag.tga
	bumpMap models/mapobjects/raster/moto_bag_n.tga
	specularMap models/mapobjects/raster/moto_bag_n.tga
}

//*************************************************************************

// rocks
models/mapobjects/rocks_sd/rock_desert
{
	qer_editorimage models/mapobjects/rocks_sd/rock_desert.tga
	diffuseMap models/mapobjects/rocks_sd/rock_desert.tga
	bumpMap models/mapobjects/rocks_sd/rock_desert_n.tga
}

models/mapobjects/rocks_sd/rock_desert_small
{
	qer_editorimage models/mapobjects/rocks_sd/rock_desert_small.tga
	diffuseMap models/mapobjects/rocks_sd/rock_desert_small.tga
	bumpMap  models/mapobjects/rocks_sd/rock_desert_small_n.tga
}

models/mapobjects/rocks_sd/rock_desert_big
{
	qer_editorimage models/mapobjects/rocks_sd/rock_desert_big.tga
	diffuseMap models/mapobjects/rocks_sd/rock_desert_big.tga
	bumpMap models/mapobjects/rocks_sd/rock_desert_big_n.tga
}

models/mapobjects/rocks_sd/rock_snow
{
	qer_editorimage models/mapobjects/rocks_sd/rock_snow.tga
	diffuseMap models/mapobjects/rocks_sd/rock_snow.tga
	bumpMap models/mapobjects/rocks_sd/rock_snow_n.tga
	specularMap models/mapobjects/rocks_sd/rock_snow_r.tga
	q3map_clipModel
}

models/mapobjects/rocks_sd/rock_snow_small
{
	qer_editorimage models/mapobjects/rocks_sd/rock_snow_small.tga
	diffuseMap models/mapobjects/rocks_sd/rock_snow_small.tga
	bumpMap models/mapobjects/rocks_sd/rock_snow_small_n.tga
	specularMap models/mapobjects/rocks_sd/rock_snow_small_r.tga
}

models/mapobjects/rocks_sd/rock_snow_big
{
	qer_editorimage models/mapobjects/rocks_sd/rock_snow_big.tga
	diffuseMap models/mapobjects/rocks_sd/rock_snow_big.tga
	bumpMap models/mapobjects/rocks_sd/rock_snow_big_n.tga
	specularMap models/mapobjects/rocks_sd/rock_snow_big_r.tga
	q3map_clipModel
}

models/mapobjects/rocks_sd/rock_temperate
{
	q3map_clipModel
	qer_editorimage models/mapobjects/rocks_sd/rock_temperate.tga
	diffuseMap models/mapobjects/rocks_sd/rock_temperate.tga
	bumpMap models/mapobjects/rocks_sd/rock_temperate_n.tga
}

models/mapobjects/rocks_sd/rock_temperate_small
{
	q3map_clipModel
	qer_editorimage models/mapobjects/rocks_sd/rock_temperate_small.tga
	diffuseMap models/mapobjects/rocks_sd/rock_temperate_small.tga
	bumpMap models/mapobjects/rocks_sd/rock_temperate_small_n.tga
}

models/mapobjects/rocks_sd/rock_temperate_big
{
	q3map_clipModel
	qer_editorimage models/mapobjects/rocks_sd/rock_temperate_big.tga
	diffuseMap models/mapobjects/rocks_sd/rock_temperate_big.tga
	bumpMap models/mapobjects/rocks_sd/rock_temperate_big_n.tga
}

models/mapobjects/rocks_sd/rock_temperate2
{
	q3map_clipModel
	qer_editorimage models/mapobjects/rocks_sd/rock_temperate2.tga
	diffuseMap models/mapobjects/rocks_sd/rock_temperate2.tga
	bumpMap models/mapobjects/rocks_sd/rock_temperate2_n.tga
}

models/mapobjects/rocks_sd/rock_temperate2_small
{
	q3map_clipModel
	qer_editorimage models/mapobjects/rocks_sd/rock_temperate2_small.tga
	diffuseMap models/mapobjects/rocks_sd/rock_temperate2_small.tga
	bumpMap models/mapobjects/rocks_sd/rock_temperate2_small_n.tga
}

models/mapobjects/rocks_sd/rock_temperate2_big
{
	q3map_clipModel
	qer_editorimage models/mapobjects/rocks_sd/rock_temperate2_big.tga
	diffuseMap models/mapobjects/rocks_sd/rock_temperate2_big.tga
	bumpMap models/mapobjects/rocks_sd/rock_temperate2_big_n.tga
}

//***********************************************************************

//siwa_props
models/mapobjects/siwa_props/siwa_pitcher1
{
	diffuseMap models/mapobjects/siwa_props/siwa_pitcher1.tga
	bumpMap models/mapobjects/siwa_props/siwa_pitcher1_n.tga
	specularMap models/mapobjects/siwa_props/siwa_pitcher1_r.tga
}
models/mapobjects/siwa_props/siwa_pitcher2
{
	diffuseMap models/mapobjects/siwa_props/siwa_pitcher2.tga
	bumpMap models/mapobjects/siwa_props/siwa_pitcher2_n.tga
	specularMap models/mapobjects/siwa_props/siwa_pitcher2_r.tga
}
models/mapobjects/siwa_props/siwa_pitcher3
{
	diffuseMap models/mapobjects/siwa_props/siwa_pitcher3.tga
	bumpMap models/mapobjects/siwa_props/siwa_pitcher3_n.tga
	specularMap models/mapobjects/siwa_props/siwa_pitcher3_r.tga
}

//*****************************************************************

//spool_Sd
models/mapobjects/spool_Sd/spool
{
	diffuseMap models/mapobjects/spool_Sd/spool.tga
	bumpMap models/mapobjects/spool_Sd/spool_n.tga
	specularMap models/mapobjects/spool_Sd/spool_r.tga
}

models/mapobjects/spool_Sd/wires
{
	diffuseMap models/mapobjects/spool_Sd/wires.tga
	bumpMap models/mapobjects/spool_Sd/wires_n.tga
	specularMap models/mapobjects/spool_Sd/wires_r.tga
}

//******************************************************************

models/mapobjects/supplystands/frame
{
	diffuseMap models/mapobjects/supplystands/frame.tga
	bumpMap models/mapobjects/supplystands/frame_n.tga
	specularMap models/mapobjects/supplystands/frame_r.tga
}

models/mapobjects/supplystands/metal_shelves
{
	diffuseMap models/mapobjects/supplystands/metal_shelves.tga
	bumpMap models/mapobjects/supplystands/metal_shelves_n.tga
	specularMap models/mapobjects/supplystands/metal_shelves_r.tga
}

//tanks_sd
models/mapobjects/tanks_sd/tracks
{
	diffuseMap models/mapobjects/tanks_sd/tracks.tga
	bumpMap models/mapobjects/tanks_sd/tracks_n.tga
	specularMap models/mapobjects/tanks_sd/tracks_r.tga
	implicitMask models/mapobjects/tanks_sd/tracks.tga
}
models/mapobjects/tanks_sd/tracks_a
{
	diffuseMap models/mapobjects/tanks_sd/tracks.tga
	bumpMap models/mapobjects/tanks_sd/tracks_n.tga
	specularMap models/mapobjects/tanks_sd/tracks_r.tga
	implicitMask models/mapobjects/tanks_sd/tracks.tga
}
models/mapobjects/tanks_sd/tracks_b
{
	diffuseMap models/mapobjects/tanks_sd/tracks_b.tga
	bumpMap models/mapobjects/tanks_sd/tracks_b_n.tga
	specularMap models/mapobjects/tanks_sd/tracks_b_r.tga
	implicitMask models/mapobjects/tanks_sd/tracks_b.tga
}
models/mapobjects/tanks_sd/wheel
{
	diffuseMap models/mapobjects/tanks_sd/wheel.tga
	bumpMap models/mapobjects/tanks_sd/wheel_n.tga
	specularMap models/mapobjects/tanks_sd/wheel_r.tga
	implicitmask models/mapobjects/tanks_sd/wheel.tga
}
models/mapobjects/tanks_sd/wheel_a
{
	diffuseMap models/mapobjects/tanks_sd/wheel_a.tga
	bumpMap models/mapobjects/tanks_sd/wheel_a_n.tga
	specularMap models/mapobjects/tanks_sd/wheel_a_r.tga
	implicitmask models/mapobjects/tanks_sd/wheel_a.tga
}
models/mapobjects/tanks_sd/wheel2_a
{
	diffuseMap models/mapobjects/tanks_sd/wheel2_a.tga
	bumpMap models/mapobjects/tanks_sd/wheel2_a_n.tga
	specularMap models/mapobjects/tanks_sd/wheel2_a_r.tga
	implicitmask models/mapobjects/tanks_sd/wheel2_a.tga
}

//let these use shaders
models/mapobjects/tanks_sd/bits_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod scroll 0 -2
	}
}

models/mapobjects/tanks_sd/bits_backward_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod scroll 0 -2
	}
}

models/mapobjects/tanks_sd/bits_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod scroll 0 2
	}
}

models/mapobjects/tanks_sd/bits_forward_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod scroll 0 2
	}
}


models/mapobjects/tanks_sd/bits_forward_oasis
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod scroll 0 1.5
	}
}

models/mapobjects/tanks_sd/bits_l
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/bits_l_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks_a
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/bits_r
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/bits_r_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/bits_static
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/bits_static_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		map models/mapobjects/tanks_sd/tracks_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}
//end use shader
models/mapobjects/tanks_sd/churchill_flat_oasis
{
	diffuseMap models/mapobjects/tanks_sd/churchill_flat_oasis.tga
	bumpMap models/mapobjects/tanks_sd/churchill_flat_oasis_n.tga
	specularMap models/mapobjects/tanks_sd/churchill_flat_oasis_r.tga
}

//TODO: whats this??
//models/mapobjects/tanks_sd/explosive
//{
//	qer_editorimage models/mapobjects/tanks_sd/explosive.tga
//	surfaceparm metalsteps
//	{
//		map models/mapobjects/tanks_sd/explosive.tga
//		rgbGen lightingDiffuse
//	}
//}

//let these use shader
models/mapobjects/tanks_sd/jag_cogs_alt_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 75
	}
}

models/mapobjects/tanks_sd/jag_cogs_alt_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag_cogs_left
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_cogs_left_s
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen vertex
	}
}

models/mapobjects/tanks_sd/jag_cogs_right
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_cogs_right_s
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen vertex
	}
}

models/mapobjects/tanks_sd/jag_cogs_snow_alt_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 75
	}
}

models/mapobjects/tanks_sd/jag_cogs_snow_alt_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag_cogs_snow_left
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a_r.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_cogs_snow_right
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a_r.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_tracks_alt_backward
{
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
		tcMod scroll -1 0
	}
}

models/mapobjects/tanks_sd/jag_tracks_alt_forward
{
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
		tcMod scroll 1 0
	}
}

models/mapobjects/tanks_sd/jag_tracks_left
{
	qer_editorimage models/mapobjects/tanks_sd/tracks_b.tga
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_tracks_left_s
{
	qer_editorimage models/mapobjects/tanks_sd/tracks_b.tga
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen vertex
	}
}

models/mapobjects/tanks_sd/jag_tracks_right
{
	qer_editorimage models/mapobjects/tanks_sd/tracks_b.tga
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_tracks_right_s
{
	qer_editorimage models/mapobjects/tanks_sd/tracks_b.tga
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen vertex
	}
}

models/mapobjects/tanks_sd/jag_wheels_alt_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag_wheels_alt_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag_wheels_left
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel2_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_wheels_left_s
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel2_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen vertex
	}
}

models/mapobjects/tanks_sd/jag_wheels_right
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel2_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_wheels_right_s
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel2_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen vertex
	}
}

models/mapobjects/tanks_sd/jag_wheels_snow_alt_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag_wheels_snow_alt_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag_wheels_snow_left
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel2_a_r.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag_wheels_snow_right
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel2_a_r.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a_r.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}
// end use shader

models/mapobjects/tanks_sd/jagdpanther
{
	qer_editorimage models/mapobjects/tanks_sd/jagdpanther.tga
	
	diffuseMap models/mapobjects/tanks_sd/jagdpanther.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_r.tga
}

models/mapobjects/tanks_sd/jagdpanther_additions_des_s
{
	qer_editorimage models/mapobjects/tanks_sd/jagdpanther_additions_desert.tga
	
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_additions_desert.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_additions_desert_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_additions_desert_r.tga
}

models/mapobjects/tanks_sd/jagdpanther_additions_desert
{
	
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_additions_desert.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_additions_desert_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_additions_desert_r.tga
}

models/mapobjects/tanks_sd/jagdpanther_additions_temperate
{
	
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_additions_temperate.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_additions_temperate_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_additions_temperate_r.tga
}

models/mapobjects/tanks_sd/jagdpanther_additions_snow
{
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_additions_snow.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_additions_snow_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_additions_snow_r.tga
}

models/mapobjects/tanks_sd/jagdpanther_full
{
	qer_editorimage models/mapobjects/tanks_sd/jagdpanther_full.tga
	
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_full.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_full_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_full_r.tga
}
models/mapobjects/tanks_sd/mg42turret_2
{
	diffuseMap models/mapobjects/tanks_sd/mg42turret_2.tga
	bumpMap models/mapobjects/tanks_sd/mg42turret_2_n.tga
	specularMap models/mapobjects/tanks_sd/mg42turret_2_r.tga
}
models/mapobjects/tanks_sd/jagdpanther_full_temperate
{
	qer_editorimage models/mapobjects/tanks_sd/jagdpanther_full_temperate.tga
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_full_temperate.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_full_temperate_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_full_temperate_r.tga
}

models/mapobjects/tanks_sd/jagdpanther_full_s
{
	qer_editorimage models/mapobjects/tanks_sd/jagdpanther_full.tga
	diffuseMap models/mapobjects/tanks_sd/jagdpanther_full.tga
	bumpMap models/mapobjects/tanks_sd/jagdpanther_full_n.tga
	specularMap models/mapobjects/tanks_sd/jagdpanther_full_r.tga
}

//more just shader
models/mapobjects/tanks_sd/jag2_cogs_alt_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 75
	}
}

models/mapobjects/tanks_sd/jag2_cogs_alt_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag2_cogs_left
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag2_cogs_right
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_a.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag2_tracks_alt_backward
{
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
		tcMod scroll -1 0
	}
}

models/mapobjects/tanks_sd/jag2_tracks_alt_forward
{
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
		tcMod scroll 1 0
	}
}

models/mapobjects/tanks_sd/jag2_tracks_left
{
	qer_editorimage models/mapobjects/tanks_sd/tracks_b.tga
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag2_tracks_right
{
	qer_editorimage models/mapobjects/tanks_sd/tracks_b.tga
	{
		map models/mapobjects/tanks_sd/tracks_b.tga
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag2_wheels_alt_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_b.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag2_wheels_alt_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_b.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -75
	}
}

models/mapobjects/tanks_sd/jag2_wheels_left
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_b.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_b.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/jag2_wheels_right
{
	qer_alphafunc gequal 0.5
	qer_editorimage models/mapobjects/tanks_sd/wheel_b.tga
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_b.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/wheel_backward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -60
	}
}

models/mapobjects/tanks_sd/wheel_backward_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -60
	}
}

models/mapobjects/tanks_sd/wheel_forward
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 60
	}
}

models/mapobjects/tanks_sd/wheel_forward_oasis
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 90
	}
}

models/mapobjects/tanks_sd/wheel_forward_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 60
	}
}

models/mapobjects/tanks_sd/wheel_l
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/wheel_l_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/wheel_r
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/wheel_r_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/wheel2_backward_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate -60
	}
}

models/mapobjects/tanks_sd/wheel2_forward_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
		tcMod rotate 60
	}
}

models/mapobjects/tanks_sd/wheel2_l_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}

models/mapobjects/tanks_sd/wheel2_r_a
{
	qer_alphafunc gequal 0.5
	cull disable
	{
		clampmap models/mapobjects/tanks_sd/wheel2_a.tga
		alphaFunc GE128
		rgbGen lightingDiffuse
	}
}
//end use shader
models/mapobjects/tanks_sd/mg42turret
{
	diffuseMap models/mapobjects/tanks_sd/mg42turret.tga
	bumpMap models/mapobjects/tanks_sd/mg42turret_n.tga
	specularMap models/mapobjects/tanks_sd/mg42turret_r.tga
}

models/mapobjects/tanks_sd/churchill_flat
{
	
	diffuseMap models/mapobjects/tanks_sd/churchill_flat.tga
	bumpMap models/mapobjects/tanks_sd/churchill_flat_n.tga
	specularMap models/mapobjects/tanks_sd/churchill_flat_r.tga
}
//***************************************************************************



models/mapobjects/grass_sd/grass
{
	qer_editorimage models/mapobjects/grass_sd/grass.tga
	cull twosided
	surfaceparm nomarks
	surfaceparm nonsolid
	//surfaceparm alphashadow
	surfaceparm trans
	nopicmip
	{
		map models/mapobjects/grass_sd/grass.tga
		rgbGen const ( 0.3 0.3 0.3 )
		detail
	}
}

models/mapobjects/grass_sd/grass_spike
{
	qer_editorimage models/mapobjects/grass_sd/grass_spike.tga
	cull twosided
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nonsolid
	//surfaceparm alphashadow
	surfaceparm trans
	nopicmip
	{
		map models/mapobjects/grass_sd/grass_spike.tga
		alphaFunc GE128
		rgbGen const ( 0.4 0.4 0.4 )
	}
}

models/mapobjects/tree_temperate_sd/trunk_temperate
{
	surfaceparm nomarks
	implicitMap models/mapobjects/tree_temperate_sd/trunk_temperate.tga
}

models/mapobjects/tree_temperate_sd/trunk_cut
{
	surfaceparm nomarks
	implicitMap models/mapobjects/tree_temperate_sd/trunk_cut.tga
}


models/mapobjects/tree_temperate_sd/leaves_temperate1
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/tree_temperate_sd/leaves_temperate1.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/tree_temperate_sd/leaves_temperate2
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/tree_temperate_sd/leaves_temperate2.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/tree_temperate_sd/leaves_temperate3
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/tree_temperate_sd/leaves_temperate3.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

models/mapobjects/tree_temperate_sd/floor_leaf1
{
	nopicmip
	qer_alphafunc greater 0.5
	qer_editorimage models/mapobjects/tree_temperate_sd/floor_leaf1.tga
	cull disable
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nomarks
	implicitMask -
}

//**********************************************************************

models/mapobjects/rocks_sd/rock_tunnelsiwa
{
	qer_editorimage models/mapobjects/rocks_sd/rock_tunnelsiwa.tga
	implicitMap models/mapobjects/rocks_sd/rock_tunnelsiwa.tga
}

models/mapobjects/rocks_sd/rock_tunnelsiwa_small
{
	qer_editorimage models/mapobjects/rocks_sd/rock_tunnelsiwa_small.tga
	implicitMap models/mapobjects/rocks_sd/rock_tunnelsiwa_small.tga
}

//**********************************************************************
// remapped snow rocks in fueldump
//**********************************************************************

models/mapobjects/props_sd/snowrock_clip
{
	qer_editorimage models/mapobjects/props_sd/snowrock_clip.tga
	implicitMap models/mapobjects/rocks_sd/rock_snow_big.tga
	q3map_clipModel
}

//**********************************************************************

models/mapobjects/seawall_rocks/rocks
{
	qer_editorimage textures/temperate_sd/rock_grayvar.tga
	diffuseMap  textures/temperate_sd/rock_grayvar.tga
	bumpMap  textures/temperate_sd/rock_grayvar_n.tga
	q3map_forcemeta
	q3map_lightmapSampleOffset 8.0
	q3map_nonplanar
	q3map_clipModel
	//implicitMap textures/temperate_sd/rock_grayvar.tga
	surfaceparm pointlight
	q3map_shadeangle 180
 
}

models/mapobjects/siwa_tunnels_sd/siwa_tunnel
{
	qer_editorimage textures/desert_sd/rock_edged_smooth
	q3map_clipModel
	q3map_forcemeta
	q3map_lightmapSampleOffset 8.0
	q3map_nonplanar
	q3map_normalimage models/mapobjects/siwa_tunnels_sd/siwa_nm.tga
	q3map_shadeangle 180
	surfaceparm pointlight
	diffuseMap textures/desert_sd/rock_edged_smooth.tga
	bumpMap textures/desert_sd/rock_edged_smooth_n.tga
}
// just a clip texture, not visible
models/mapobjects/siwa_tunnels_sd/siwa_tunneliaclip
{
	qer_editorimage textures/common/clipmonster
	q3map_forcemeta
    	q3map_clipModel
    	surfaceparm nodraw
    	surfaceparm nomarks
    	surfaceparm nonsolid
    	surfaceparm monsterclip
}

models/mapobjects/tree/branch_slp1
{
	cull twosided
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	nopicmip
	implicitMask -
}

models/mapobjects/tree/branch_slp2
{
	cull twosided
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	nopicmip
	implicitMask -
}

models/mapobjects/tree/branch8
{
	cull twosided
	deformVertexes wave 194 sin 0 1 0 .4
	deformVertexes wave 194 sin 0 2 0 .1
	deformVertexes wave 30 sin 0 1 0 .3
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	nopicmip
	implicitMask -
}

models/mapobjects/tree_desert_sd/palm_trunk
{
	diffuseMap models/mapobjects/tree_desert_sd/palm_trunk.tga
	bumpMap models/mapobjects/tree_desert_sd/palm_trunk_n.tga
	specularMap models/mapobjects/tree_desert_sd/palm_trunk_r.tga
	surfaceparm woodsteps
}
models/mapobjects/tree_desert_sd/palm_leaf1
{
	diffuseMap models/mapobjects/tree_desert_sd/palm_leaf1.tga
	bumpMap models/mapobjects/tree_desert_sd/palm_leaf1_n.tga
	specularMap models/mapobjects/tree_desert_sd/palm_leaf1_r.tga
	qer_alphafunc gequal 0.5
	cull twosided
	deformVertexes wave 15 sin 0 1 0 0.25
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	nopicmip
	implicitMask -
}

//models/mapobjects/tree_desert_sd/palm_leaves
//{
//	qer_alphafunc gequal 0.5
//	cull twosided
//	deformVertexes wave 194 sin 0 1 0 .4
//	deformVertexes wave 194 sin 0 2 0 .1
//	deformVertexes wave 30 sin 0 1 0 .3
//	surfaceparm alphashadow
//	surfaceparm nomarks
//	surfaceparm trans
//	nopicmip
//	implicitMask -
//}

//models/mapobjects/tree_desert_sd/palm_leaves2
//{
//	cull twosided
//	deformVertexes wave 194 sin 0 1 0 .4
//	deformVertexes wave 194 sin 0 2 0 .1
//	deformVertexes wave 30 sin 0 1 0 .3
//	surfaceparm alphashadow
//	surfaceparm nomarks
//	surfaceparm trans
//	nopicmip
//	implicitMask -
//}

//models/mapobjects/tree_desert_sd/floorpalmleaf
//{
//	qer_editorimage models/mapobjects/tree_desert_sd/palm_leaf1.tga
//	qer_alphafunc gequal 0.5
//	cull twosided
//	surfaceparm alphashadow
//	surfaceparm nomarks
//	surfaceparm trans
//	nopicmip
//	implicitMask models/mapobjects/tree_desert_sd/palm_leaf1.tga
//}

//models/mapobjects/trees_sd/bush_s
//{
//	qer_alphafunc gequal 0.5
//	surfaceparm alphashadow
//	surfaceparm nomarks
//	surfaceparm nonsolid
//	surfaceparm trans
//	nopicmip
//	implicitMask -
//}

models/mapobjects/trees_sd/winterbranch01
{
	qer_alphafunc gequal 0.5
	cull twosided
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	nopicmip
	diffuseMap models/mapobjects/trees_sd/winterbranch01.tga
	bumpMap models/mapobjects/trees_sd/winterbranch01_n.tga
	specularMap models/mapobjects/trees_sd/winterbranch01_r.tga
	implicitMask -
}

models/mapobjects/trees_sd/wintertrunk01
{
	qer_alphafunc gequal 0.5
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm trans
	nopicmip
	diffuseMap models/mapobjects/trees_sd/wintertrunk01.tga
	bumpMap models/mapobjects/trees_sd/wintertrunk01_n.tga
	specularMap models/mapobjects/trees_sd/wintertrunk01_r.tga
	implicitMask -
}

models/mapobjects/vehicles/wagon/wag_whl
{
	cull twosided
	implicitMask -
}
