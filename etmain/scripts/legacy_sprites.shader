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

sprites/objective_blue
{
	nopicmip
	nocompress
	nomipmaps
	{
		clampmap sprites/objective_blue.tga
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
		clampmap sprites/objective_red.tga
		depthFunc equal
		blendfunc blend
		rgbGen vertex
		alphaGen vertex
	}
}

//========================================//

//========================================//
// Clampmapped Player Float Sprites
//========================================//
sprites/legacy_balloon3
{
	nocompress
	nopicmip
	{
		clampmap sprites/balloon4.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/legacy_buddy
{
	nocompress
	nopicmip
	{
		clampmap sprites/buddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .75 .25
	}
}

sprites/legacy_construct
{
	nocompress
	nopicmip
	{
		clampmap sprites/construct.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//alphagen wave sin .6 .5 .5 .25
		rgbgen vertex
	}
}

sprites/legacy_destroy
{
	nocompress
	nopicmip
	{
		clampmap sprites/destroy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//alphagen wave sin .6 .5 0 .25
		rgbgen vertex
	}
}

sprites/legacy_escort
{
	nocompress
	nopicmip
	{
		clampmap sprites/escort.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//alphagen wave sin .6 .5 0 .25
		rgbgen vertex
	}
}

sprites/legacy_medic_revive
{
	nocompress
	nopicmip
	{
		clampmap sprites/medicrevive.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective_team
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_team.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective_dropped
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_dropped.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective_enemy
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_enemy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective_both_te
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_both_te.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective_both_td
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_both_td.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_objective_both_de
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_both_de.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_shield
{
	nocompress
	nopicmip
	{
		clampmap sprites/icon_shield.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/legacy_skull
{
	nocompress
	nomipmaps
	nopicmip
	{
		clampmap gfx/2d/multi_dead.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/legacy_uniform_allied
{
	nocompress
	nopicmip
	{
		clampmap sprites/active_uniform_allied.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/legacy_uniform_axis
{
	nocompress
	nopicmip
	{
		clampmap sprites/active_uniform_axis.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/legacy_voiceammo
{
	nocompress
	nopicmip
	{
		clampmap sprites/voiceammo.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/legacy_voicechat
{
	nocompress
	nopicmip
	{
		clampmap sprites/voicechat.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/legacy_voicechat_orange
{
	nocompress
	nopicmip
	{
		clampmap sprites/voicechat_orange.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/legacy_voicemedic
{
	nocompress
	nopicmip
	{
		clampmap sprites/voicemedic.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/legacy_waypoint_attack
{
	nocompress
	nopicmip
	{
		clampmap sprites/attack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/legacy_waypoint_attack_compass
{
	nocompress
	nopicmip
	{
		clampmap sprites/attack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}

sprites/legacy_waypoint_defend
{
	nocompress
	nopicmip
	{
		clampmap sprites/defend.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/legacy_waypoint_defend_compass
{
	nocompress
	nopicmip
	{
		clampmap sprites/defend.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}

sprites/legacy_waypoint_regroup
{
	nocompress
	nopicmip
	{
		clampmap sprites/regroup.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/legacy_waypoint_regroup_compass
{
	nocompress
	nopicmip
	{
		clampmap sprites/regroup.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}
//========================================//
