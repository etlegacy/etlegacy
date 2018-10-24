//======================================================================
// decals.shader
// Last edit: 26.02.2017
//!NOTE! just edit what we have! -Thunder
//======================================================================
/*
textures/decals/blood1
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/blood1_drag
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/blood1_red
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/blood2
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/blood3
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/blood4
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/blood5
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/burnm_01a
{
	qer_editorimage textures/decals/burnm_01
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/burnm_01
}

textures/decals/candle1
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}
*/
textures/decals/corrosive_sign
{
	qer_editorimage textures/xlab_props/sign_c17
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/xlab_props/sign_c17
}
/*
textures/decals/drain
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}
*/
textures/decals/hay
{
	qer_editorimage textures/props/hay
	diffusemap textures/props/hay
	bumpmap textures/props/hay_n
	specularmap textures/props/hay_s
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/light_c01_d
{
	qer_editorimage textures/lights/light_c01
	q3map_lightimage textures/lightimage/light_c01_color
	q3map_surfacelight 5000
	
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm nomarks
	{
		map textures/lights/light_c01
		rgbGen vertex
	}
	{
		map textures/lights/light_c01.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/decals/light_c01_10k
{
	qer_editorimage textures/lights/light_c01
	q3map_lightimage textures/lightimage/light_c01.blend.tga
	q3map_surfacelight 10000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_c01
		rgbGen vertex
	}
	{
		map textures/lights/light_c01.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/decals/light_m01b
{
	qer_editorimage textures/lights/light_m01b
	q3map_lightimage textures/lightimage/blue
	q3map_lightsubdivide 128
	q3map_surfacelight 5000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m01b
		rgbGen vertex
	}
	{
		map textures/lights/light_m01b.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/decals/light_m11_redd
{
	qer_editorimage textures/lights/light_m11
	q3map_lightrgb 1.0 0.0 0.0
	q3map_surfacelight 5000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m11
		rgbGen vertex
	}
	{
		map textures/lights/light_m11.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/decals/light_m16_10kd
{
	qer_editorimage textures/lights/light_m16
	q3map_lightrgb 1.000000 0.866667 0.733333
	q3map_surfacelight 10000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m16
		rgbGen vertex
	}
	{
		map textures/lights/light_m16.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/decals/light_m32
{
	qer_editorimage textures/lights/light_m32
	q3map_lightrgb 1.000000 0.866667 0.733333
	q3map_surfacelight 10000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m32
		rgbGen vertex
	}
	{
		map textures/lights/light_m32.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}
/*
textures/decals/maps
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}
*/
textures/decals/oil_slick
{
    qer_editorimage textures/decals/oil_slick
	diffusemap textures/decals/oil_slick
	bumpmap textures/decals/oil_slick_n
	specularmap textures/decals/oil_slick_s
	surfaceparm slick
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset

	surfaceparm slick
	{
		map textures/decals/oil_slick
		alphaFunc GE128
		rgbGen vertex
	}
}
/*
textures/decals/one
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/paper_c01
{
	
	polygonOffset
	surfaceparm pointlight
	implicitMap -
}

textures/decals/paper_c10
{
	
	polygonOffset
	surfaceparm pointlight
	implicitMap -
}

textures/decals/s_attention
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_ATTENTION
}

textures/decals/s_danger
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_DANGER
}

textures/decals/s_donot_enter
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_DNOT_ENTER
}
*/
textures/decals/s_ident_required_2r
{
    qer_editorimage textures/decals/s_ident_required_2r
	diffusemap textures/decals/s_ident_required_2r
	bumpmap textures/decals/s_ident_required_2r_n
	specularmap textures/decals/s_ident_required_2r_s
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_IDENT_REQUIRED_2R
}
/*
textures/decals/s_ident_required_2w
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_IDENT_REQUIRED_2W
}

textures/decals/skidmarks
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/stripe_blue
{
	qer_editorimage textures/decals/stripe_blue_guide
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/stripe_haz
{
	qer_editorimage textures/decals/stripe_haz_guide
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/stripe_wht
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/stripe_wht_dot
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/stripe_ylw
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/three
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}
*/
textures/decals/trim_m01
{
    qer_editorimage textures/decals/trim_m01
	diffusemap textures/decals/trim_m01
	bumpmap textures/decals/trim_m01_n
	specularmap textures/decals/trim_m01_s
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

/*
textures/decals/two
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}
*/
textures/decals/vent
{
	qer_editorimage textures/decals/vent
	diffusemap textures/decals/vent
	bumpmap textures/decals/vent_n
	specularmap textures/decals/vent_s
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/vent2
{
	qer_editorimage textures/miltary_wall/miltary_m04
	diffusemap textures/miltary_wall/miltary_m04
	bumpmap textures/miltary_wall/miltary_m04_n
	specularmap textures/miltary_wall/miltary_m04
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/miltary_wall/miltary_m04
}

/*textures/decals/water_stain
{
	
	nofog
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/water_stain_100
{
	qer_editorimage textures/decals/water_stain
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/water_stain
}
*/