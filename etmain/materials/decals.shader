// decals.shader

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

// not in path
textures/decals/burnm_01a
{
	qer_editorimage textures/decals/burnm_01.tga
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/burnm_01.tga
}

textures/decals/candle1
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

// not in path
//textures/decals/corrosive_sign
//{
//	qer_editorimage textures/xlab_props/sign_c17.tga
	
//	polygonOffset
//	surfaceparm pointlight
//	implicitBlend textures/xlab_props/sign_c17.tga
//}

textures/decals/drain
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/hay
{
	qer_editorimage textures/props/hay.tga
	diffusemap textures/props/hay.tga
	bumpmap textures/props/hay_n.tga
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/light_c01_d
{
	qer_editorimage textures/lights/light_c01.tga
	q3map_lightimage textures/lightimage/light_c01_color.tga
	q3map_surfacelight 5000
	
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm nomarks
	{
		map textures/lights/light_c01.tga
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
	qer_editorimage textures/lights/light_c01.tga
	q3map_lightimage textures/lightimage/light_c01.blend.tga
	q3map_surfacelight 10000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_c01.tga
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
	qer_editorimage textures/lights/light_m01b.tga
	q3map_lightimage textures/lightimage/blue.tga
	q3map_lightsubdivide 128
	q3map_surfacelight 5000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m01b.tga
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
	qer_editorimage textures/lights/light_m11.tga
	q3map_lightrgb 1.0 0.0 0.0
	q3map_surfacelight 5000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m11.tga
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
	qer_editorimage textures/lights/light_m16.tga
	q3map_lightrgb 1.000000 0.866667 0.733333
	q3map_surfacelight 10000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m16.tga
		rgbGen vertex
	}
	{
		map textures/lights/light_m16.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

// not in path
textures/decals/light_m32
{
	qer_editorimage textures/lights/light_m32.tga
	q3map_lightrgb 1.000000 0.866667 0.733333
	q3map_surfacelight 10000
	
	polygonOffset
	surfaceparm nomarks
	surfaceparm pointlight
	{
		map textures/lights/light_m32.tga
		rgbGen vertex
	}
	{
		map textures/lights/light_m32.blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen identity
	}
}

textures/decals/maps
{
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/oil_slick
{
    qer_editorimage textures/decals/oil_slick.tga
	surfaceparm slick
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset

	surfaceparm slick
	{
		map textures/decals/oil_slick.tga
		alphaFunc GE128
		rgbGen vertex
	}
}

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

// not in path
textures/decals/s_attention
{
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_ATTENTION.tga
}

// not in path
textures/decals/s_danger
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_DANGER.tga
}

// not in path
textures/decals/s_donot_enter
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_DNOT_ENTER.tga
}

textures/decals/s_ident_required_2r
{
    qer_editorimage textures/decals/s_ident_required_2r.tga
	diffusemap textures/decals/S_IDENT_REQUIRED_2R.tga
	bumpmap textures/decals/S_IDENT_REQUIRED_2R_n.tga
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_IDENT_REQUIRED_2R.tga
}

// not in path
textures/decals/s_ident_required_2w
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/S_IDENT_REQUIRED_2W.tga
}

textures/decals/skidmarks
{
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

// not in path
textures/decals/stripe_blue
{
	qer_editorimage textures/decals/stripe_blue_guide.tga
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/stripe_haz
{
	qer_editorimage textures/decals/stripe_haz_guide.tga
	
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

textures/decals/trim_m01
{
    qer_editorimage textures/decals/trim_m01.tga
	diffusemap textures/decals/trim_m01.tga
	bumpmap textures/decals/trim_m01_n.tga
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

// not in path
textures/decals/vent
{
	qer_editorimage textures/decals/vent.tga
	diffusemap textures/decals/vent.tga
	bumpmap textures/decals/vent_n.tga
	specularmap textures/decals/vent_r.tga
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/vent2
{
	qer_editorimage textures/miltary_wall/miltary_m04.tga
	diffusemap textures/miltary_wall/miltary_m04.tga
	bumpmap textures/miltary_wall/miltary_m04_n.tga
	specularmap textures/miltary_wall/miltary_m04_r.tga
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/miltary_wall/miltary_m04.tga
}

// not in path
/*
textures/decals/water_stain
{
	
	nofog
	polygonOffset
	surfaceparm pointlight
	implicitBlend -
}

textures/decals/water_stain_100
{
	qer_editorimage textures/decals/water_stain.tga
	
	polygonOffset
	surfaceparm pointlight
	implicitBlend textures/decals/water_stain.tga
}
*/
