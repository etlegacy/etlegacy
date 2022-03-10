// sprites.shader

sprites/balloon3
{
	nocompress
	nopicmip
	{
		map sprites/balloon4.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/buddy
{
	nocompress
	nopicmip
	{
		map sprites/buddy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .75 .25
	}
}

sprites/construct
{
	nocompress
	nopicmip
	{
		map sprites/construct.tga
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
		map sprites/destroy.tga
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
		map sprites/escort.tga
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
		map sprites/medicrevive.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective
{
	nocompress
	nopicmip
	{
		map sprites/objective.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_team
{
	nocompress
	nopicmip
	{
		map sprites/objective_team.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_dropped
{
	nocompress
	nopicmip
	{
		map sprites/objective_dropped.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_enemy
{
	nocompress
	nopicmip
	{
		map sprites/objective_enemy.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_both_te
{
	nocompress
	nopicmip
	{
		map sprites/objective_both_te.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_both_td
{
	nocompress
	nopicmip
	{
		map sprites/objective_both_td.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/objective_both_de
{
	nocompress
	nopicmip
	{
		map sprites/objective_both_de.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

sprites/shield
{
	nocompress
	nopicmip
	{
		map sprites/icon_shield.tga
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
		map gfx/2d/multi_dead.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/uniform_allied
{
	nocompress
	nopicmip
	{
		map sprites/active_uniform_allied.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/uniform_axis
{
	nocompress
	nopicmip
	{
		map sprites/active_uniform_axis.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/voiceammo
{
	nocompress
	nopicmip
	{
		map sprites/voiceammo.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/voicechat
{
	nocompress
	nopicmip
	{
		map sprites/voicechat.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/voicemedic
{
	nocompress
	nopicmip
	{
		map sprites/voicemedic.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

sprites/waypoint_attack
{
	nocompress
	nopicmip
	{
		map sprites/attack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/waypoint_attack_compass
{
	nocompress
	nopicmip
	{
		map sprites/attack.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}

sprites/waypoint_defend
{
	nocompress
	nopicmip
	{
		map sprites/defend.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/waypoint_defend_compass
{
	nocompress
	nopicmip
	{
		map sprites/defend.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}

sprites/waypoint_regroup
{
	nocompress
	nopicmip
	{
		map sprites/regroup.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

sprites/waypoint_regroup_compass
{
	nocompress
	nopicmip
	{
		map sprites/regroup.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen wave sin .6 .5 .25 .25
	}
}
