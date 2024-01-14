//========================================//
// legacy_sprites.shader
//========================================//


//========================================//
// Disguise shaders
//========================================//
// enemy disguised shader
sprites/undercover
{
	nocompress
	nopicmip
	{
		map sprites/undercover.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/uniform_allied_hud
{
	nocompress
	nopicmip
	{
		map sprites/active_uniform_allied.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/uniform_axis_hud
{
	nocompress
	nopicmip
	{
		map sprites/active_uniform_axis.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
//========================================//


//========================================//
// Fireteam
//========================================//
sprites/fireteam
{
	nocompress
	nopicmip
	{
		map sprites/fireteam.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
//========================================//


//========================================//
// Ready icon
//========================================//
sprites/ready
{
	nocompress
	nomipmaps
	nopicmip
	{
		map gfx/2d/ready.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

//========================================//
// Authenticated icon
//========================================//
sprites/authenticated
{
	nocompress
	nomipmaps
	nopicmip
	{
		map gfx/2d/authenticated.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
//========================================//


//========================================//
// Voice chat sprites
//========================================//
sprites/greentick
{
	nocompress
	nopicmip
	{
		map sprites/greentick.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/redcross
{
	nocompress
	nopicmip
	{
		map sprites/redcross.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
//========================================//


//========================================//
// Minimap sprites
//========================================//
sprites/landmine_allied
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/landmine_allied.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/landmine_axis
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/landmine_axis.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_medic_icon
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/voicemedic.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_ammo_icon
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/voiceammo.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_voicechat_icon
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/voicechat.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}
//========================================//
