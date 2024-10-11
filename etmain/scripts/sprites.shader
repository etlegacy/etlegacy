//========================================//
// Clampmapped Player Float Sprites
//========================================//
sprites/balloon3
{
	nocompress
	nopicmip
	{
		clampmap sprites/balloon4.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/buddy
{
	nocompress
	nopicmip
	{
		clampmap sprites/buddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .75 .25
	}
}

sprites/construct
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

sprites/destroy
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

sprites/escort
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

sprites/medic_revive
{
	nocompress
	nopicmip
	{
		clampmap sprites/medicrevive.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_team
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_team.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_dropped
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_dropped.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_enemy
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_enemy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_both_te
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_both_te.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_both_td
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_both_td.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_both_de
{
	nocompress
	nopicmip
	{
		clampmap sprites/objective_both_de.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/shield
{
	nocompress
	nopicmip
	{
		clampmap sprites/icon_shield.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/skull
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

sprites/uniform_allied
{
	nocompress
	nopicmip
	{
		clampmap sprites/active_uniform_allied.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/uniform_axis
{
	nocompress
	nopicmip
	{
		clampmap sprites/active_uniform_axis.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/voiceammo
{
	nocompress
	nopicmip
	{
		clampmap sprites/voiceammo.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/voicechat
{
	nocompress
	nopicmip
	{
		clampmap sprites/voicechat.tga
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

sprites/voicemedic
{
	nocompress
	nopicmip
	{
		clampmap sprites/voicemedic.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/waypoint_attack
{
	nocompress
	nopicmip
	{
		clampmap sprites/attack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/waypoint_attack_compass
{
	nocompress
	nopicmip
	{
		clampmap sprites/attack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}

sprites/waypoint_defend
{
	nocompress
	nopicmip
	{
		clampmap sprites/defend.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/waypoint_defend_compass
{
	nocompress
	nopicmip
	{
		clampmap sprites/defend.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}

sprites/waypoint_regroup
{
	nocompress
	nopicmip
	{
		clampmap sprites/regroup.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/waypoint_regroup_compass
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
