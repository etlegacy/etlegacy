//========================================//
// legacy_models_weapons2.shader
//========================================//


//========================================//
// KA-BAR
//========================================//
models/weapons2/knife_kbar/knife_yd
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/weapons2/knife_kbar/knife_yd_alpha.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/weapons2/knife_kbar/knife_yd.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen lightingdiffuse
	}
}
//========================================//


//========================================//
// Mobile Browning .30 cal MG
//========================================//
models/weapons2/browning/browning
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/browning/browning.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/weapons2/browning/biped
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/weapons2/browning/biped.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/weapons2/browning/barrel
{
	cull none
	{
		map models/multiplayer/browning/barrel.tga
		rgbGen const ( 0 0 0 )
		alphaFunc GE128
		depthWrite
	}
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
		depthFunc equal
	}
	{
		map models/multiplayer/browning/barrel.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
		depthFunc equal
	}
}
//========================================//


//========================================//
// Bazooka
//========================================//
models/weapons2/bazooka/bazooka
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/weapons2/bazooka/bazooka_alpha.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/weapons2/bazooka/bazooka.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen lightingdiffuse
	}
}

models/weapons2/bazooka/bazookaTube
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/weapons2/bazooka/bazookaTube_alpha.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/weapons2/bazooka/bazookaTube.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen lightingdiffuse
	}
}
//========================================//


//========================================//
// Axis Granatwefer 34
//========================================//
models/multiplayer/mortar/mortar_ax
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/mortar/mortar_ax.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/multiplayer/mortar/mortar_shell_ax
{
	{
		map models/multiplayer/mortar/mortar_shell_ax.jpg
		rgbGen lightingdiffuse
	}
}

models/multiplayer/mortar/mortar_sd
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/multiplayer/mortar/mortar_sd.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}
//========================================//


//========================================//
// MP34
//========================================//
models/weapons2/mp34/mp34
{
	cull disable
	{
		map models/weapons2/mp34/mp34.tga
		rgbGen const ( 0 0 0 )
		alphaFunc GE128
	}
	{
		map models/weapons2/mp34/mp34.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/weapons2/mp34/mp34stock
{
	{
		map textures/effects/envmap_slate.tga
		rgbGen lightingDiffuse
		tcGen environment
	}
	{
		map models/weapons2/mp34/mp34stock_alpha.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/weapons2/mp34/mp34stock.jpg
		blendFunc GL_ONE GL_ONE
		rgbGen lightingdiffuse
	}
}
//========================================//


//========================================//
// Medium brass model
// Used by MGs and rifles
//========================================//
models/weapons2/shells/m_shell
{
	{
		map models/weapons2/shells/M_shell.jpg
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/shells/M_shell.jpg
		blendFunc GL_ZERO GL_ONE
		rgbGen lightingDiffuse
	}
}
//========================================//

//========================================//
// Empty syringe model
//========================================//
models/multiplayer/syringe/0percent
{
	cull disable
	{
		map models/multiplayer/syringe/syringe.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
	{
		map models/multiplayer/syringe/syringe_reflections.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen lightingdiffuse
	}
}
