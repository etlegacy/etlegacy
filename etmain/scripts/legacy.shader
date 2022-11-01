//========================================//
// legacy.shader
//========================================//

images/blackmask
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/2d/mapmask.tga
		depthwrite
		blendfunc blend
		rgbGen identity
		alphaGen vertex
	}
}

textures/sfx/shoutcast_landmine
{
	cull none
	deformVertexes wave 1 sin -0.5 0 0 1
	noPicmip
	surfaceparm trans
	{
		map textures/sfx/construction.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
		tcGen environment
		tcMod scroll 0.025 -0.07625
	}
}

textures/sfx/spawnpoint_marker
{
	cull none
	noPicmip
	{
		clampMap ui/assets/gradientround.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
