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
		clampmap sprites/undercover.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/uniform_allied_hud
{
	nocompress
	nopicmip
	{
		clampmap sprites/active_uniform_allied.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/uniform_axis_hud
{
	nocompress
	nopicmip
	{
		clampmap sprites/active_uniform_axis.tga
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
		clampmap sprites/fireteam.tga
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
		clampmap gfx/2d/ready.tga
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
		clampmap gfx/2d/authenticated.tga
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
		clampmap sprites/greentick.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/redcross
{
	nocompress
	nopicmip
	{
		clampmap sprites/redcross.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/voicechat_orange
{
	nocompress
	nopicmip
	{
		clampmap sprites/voicechat_orange.tga
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
		clampmap sprites/landmine_allied.tga
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
		clampmap sprites/landmine_axis.tga
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
		clampmap sprites/voicemedic.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_medic_revive
{
	nopicmip
	nocompress
	nomipmaps
	{
		clampmap sprites/medic_revive.tga
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
		clampmap sprites/voiceammo.tga
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
		clampmap sprites/voicechat.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_voicechat_orange_icon
{
	nopicmip
	nocompress
	nomipmaps
	{
		clampmap sprites/voicechat_orange.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_greentick
{
	nopicmip
	nocompress
	nomipmaps
	{
		clampmap sprites/greentick.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_redcross
{
	nopicmip
	nocompress
	nomipmaps
	{
		clampmap sprites/redcross.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/cm_friendlycross
{
	nopicmip
	nocompress
	nomipmaps
	{
		clampmap gfx/2d/friendlycross.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/objective_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/objective_blue.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

sprites/objective_red
{
	nopicmip
	nocompress
	nomipmaps
	{
		map sprites/objective_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_ic_soldier
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/ic_soldier.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_ic_medic
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/ic_medic.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_ic_engineer
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/ic_engineer.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_ic_fieldops
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/ic_fieldops.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/limbo/cm_ic_covertops
{
	nopicmip
	nocompress
	nomipmaps
	{
		map gfx/limbo/ic_covertops.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

//========================================//
