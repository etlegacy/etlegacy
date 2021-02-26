/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cg_edv.c
 * @brief Extended demo viewer
 */

#include "cg_local.h"

#ifdef FEATURE_EDV

#define ADDLINE_WIDTH 1.5f

/**
 * @brief CG_RunBinding
 * @param[in] key
 * @param[in] down
 */
void CG_RunBinding(int key, qboolean down)
{
	char buf[MAX_STRING_TOKENS];

	// we don't want these duplicate keypresses
	if (key & K_CHAR_FLAG)
	{
		return;
	}

	trap_Key_GetBindingBuf(key, buf, sizeof(buf));

	if (!buf[0])
	{
		return;
	}

	CG_RunBindingBuf(key, down, buf);
}

/**
 * @brief CG_RunBindingBuf
 * @param[in] key
 * @param[in] down
 * @param[in,out] buf
 */
void CG_RunBindingBuf(int key, qboolean down, char *buf)
{
	if (!buf[0])
	{
		return;
	}

	// +commands need to be handled specially
	if (buf[0] == '+')
	{
		if (!down)
		{
			buf[0] = '-';
		}

		// pass the key number so that the engine can handle
		// keyups on its own if neccessary

		// the engine also tacks on the time so that sub-frame
		// key timing can be achieved
		trap_SendConsoleCommand(va("%s %d %d\n", buf, key, trap_Milliseconds()));

		// for cg_runinput so we can override client engine
		// hopefully this is ugly enough to make someone want to make it teh bettar
		if (cgs.demoCamera.renderingFreeCam)
		{
			if (!Q_stricmp(buf, "+moveright"))
			{
				cgs.demoCamera.move |= 0x04;
			}
			else if (!Q_stricmp(buf, "-moveright"))
			{
				cgs.demoCamera.move &= ~0x04;
			}
			else if (!Q_stricmp(buf, "+moveleft"))
			{
				cgs.demoCamera.move |= 0x08;
			}
			else if (!Q_stricmp(buf, "-moveleft"))
			{
				cgs.demoCamera.move &= ~0x08;
			}
			else if (!Q_stricmp(buf, "+forward"))
			{
				cgs.demoCamera.move |= 0x01;
			}
			else if (!Q_stricmp(buf, "-forward"))
			{
				cgs.demoCamera.move &= ~0x01;
			}
			else if (!Q_stricmp(buf, "+back"))
			{
				cgs.demoCamera.move |= 0x02;
			}
			else if (!Q_stricmp(buf, "-back"))
			{
				cgs.demoCamera.move &= ~0x02;
			}
			else if (!Q_stricmp(buf, "+moveup"))
			{
				cgs.demoCamera.move |= 0x10;
			}
			else if (!Q_stricmp(buf, "-moveup"))
			{
				cgs.demoCamera.move &= ~0x10;
			}
			else if (!Q_stricmp(buf, "+movedown"))
			{
				cgs.demoCamera.move |= 0x20;
			}
			else if (!Q_stricmp(buf, "-movedown"))
			{
				cgs.demoCamera.move &= ~0x20;
			}
		}
		else
		{
			cgs.demoCamera.move = 0;
		}

		return;
	}
	else if (down == qfalse)
	{
		return;         // we don't care about keyups otherwise

	}
	trap_SendConsoleCommand(va("%s\n", buf));
}

/**
 * @brief CG_DrawLine
 * @param[in] start
 * @param[in] end
 * @param[in] color
 * @param[in] shader
 */
void CG_DrawLine(vec3_t start, vec3_t end, vec4_t color, qhandle_t shader)
{
	polyBuffer_t *pb;
	int          vert;
	byte         bcolor[4];

	vec3_t dir, diff, up;

	pb = CG_PB_FindFreePolyBuffer(shader, 4, 6);
	if (!pb)
	{
		return;
	}

	bcolor[0] = (byte)(color[0] * 255.f);
	bcolor[1] = (byte)(color[1] * 255.f);
	bcolor[2] = (byte)(color[2] * 255.f);
	bcolor[3] = (byte)(color[3] * 255.f);

	vert = pb->numVerts;

	VectorSubtract(start, end, dir);
	VectorNormalizeFast(dir);

	// start points
	VectorSubtract(start, cg.refdef_current->vieworg, diff);
	CrossProduct(dir, diff, up);
	VectorNormalizeFast(up);
	VectorScale(up, ADDLINE_WIDTH * .5f, up);

	VectorAdd(start, up, pb->xyz[vert + 0]);
	Vector2Set(pb->st[vert + 0], 0.0, 0.0);
	Com_Memcpy(pb->color[vert + 0], bcolor, sizeof(*pb->color));

	VectorSubtract(start, up, pb->xyz[vert + 1]);
	Vector2Set(pb->st[vert + 1], 0.0, 1.0);
	Com_Memcpy(pb->color[vert + 1], bcolor, sizeof(*pb->color));

	// end points
	VectorSubtract(end, cg.refdef_current->vieworg, diff);
	CrossProduct(dir, diff, up);
	VectorNormalizeFast(up);
	VectorScale(up, ADDLINE_WIDTH * .5f, up);

	VectorAdd(end, up, pb->xyz[vert + 2]);
	Vector2Set(pb->st[vert + 2], 1.0, 0.0);
	Com_Memcpy(pb->color[vert + 2], bcolor, sizeof(*pb->color));

	VectorSubtract(end, up, pb->xyz[vert + 3]);
	Vector2Set(pb->st[vert + 3], 1.0, 1.0);
	Com_Memcpy(pb->color[vert + 3], bcolor, sizeof(*pb->color));

	pb->numVerts = vert + 4;

	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 2);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 0);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 1);

	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 1);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 3);
	pb->indicies[pb->numIndicies++] = (unsigned int)(vert + 2);
}

#define ERROR_PREFIX "^1ERROR: "

/**
 * @brief CG_EDV_WeaponCam
 * @param[in] cent
 * @param[in] ent
 */
void CG_EDV_WeaponCam(centity_t *cent, refEntity_t *ent)
{

	if (!cg.demoPlayback || cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}

	if (!demo_weaponcam.integer)
	{
		return;
	}

	if (cent->currentState.teamNum != cg.snap->ps.teamNum && demo_teamonlymissilecam.integer)
	{
		return;
	}

	if ((demo_weaponcam.integer & DWC_PANZER) && (GetWeaponTableData(cent->currentState.weapon)->type & WEAPON_TYPE_PANZER))
	{
		vec3_t delta;

		cgs.demoCamera.renderingWeaponCam = qtrue;

		// point camera in direction of travel
		VectorCopy(cent->currentState.pos.trDelta, delta);
		VectorNormalize(delta);
		vectoangles(delta, cgs.demoCamera.camAngle);

		if (demo_autotimescaleweapons.integer & ATSW_PANZER)
		{
			trap_Cvar_Set("timescale", demo_autotimescale.string);
		}
	}
	else if ((demo_weaponcam.integer & DWC_MORTAR) && CHECKBITWISE(GetWeaponTableData(cent->currentState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
	{
		cgs.demoCamera.renderingWeaponCam = qtrue;

		//point camera in direction of travel (saved from cg_ents)
		VectorCopy(cent->rawAngles, cgs.demoCamera.camAngle);

		if (demo_autotimescaleweapons.integer & ATSW_MORTAR)
		{
			trap_Cvar_Set("timescale", demo_autotimescale.string);
		}

	}
	else if ((demo_weaponcam.integer & DWC_GRENADE) && (GetWeaponTableData(cent->currentState.weapon)->type & (WEAPON_TYPE_GRENADE | WEAPON_TYPE_RIFLENADE)))
	{
		cgs.demoCamera.renderingWeaponCam = qtrue;
		// point camera in direction of travel (saved from cg_ents)
		VectorCopy(cent->rawAngles, cgs.demoCamera.camAngle);
		if (demo_autotimescaleweapons.integer & ATSW_GRENADE)
		{
			trap_Cvar_Set("timescale", demo_autotimescale.string);
		}
	}
	else if ((demo_weaponcam.integer & DWC_SMOKE) && (cent->currentState.weapon == WP_SMOKE_MARKER || cent->currentState.weapon == WP_SMOKE_BOMB))
	{
		cgs.demoCamera.renderingWeaponCam = qtrue;
		// point camera in direction of travel (saved from cg_ents)
		VectorCopy(cent->rawAngles, cgs.demoCamera.camAngle);
		if (demo_autotimescaleweapons.integer & ATSW_SMOKE)
		{
			trap_Cvar_Set("timescale", demo_autotimescale.string);
		}
	}
	else if ((demo_weaponcam.integer & DWC_DYNAMITE) && cent->currentState.weapon == WP_DYNAMITE)
	{
		cgs.demoCamera.renderingWeaponCam = qtrue;
		// point camera in direction of travel (saved from cg_ents)
		VectorCopy(cent->rawAngles, cgs.demoCamera.camAngle);
		if (demo_autotimescaleweapons.integer & ATSW_DYNAMITE)
		{
			trap_Cvar_Set("timescale", demo_autotimescale.string);
		}
	}

	if (cgs.demoCamera.renderingWeaponCam)
	{
		char distance[MAX_CVAR_VALUE_STRING];
		char *disValue;
		int  dis[3] = { -99999, -99999, -99999 };
		int  count;

		VectorCopy(ent->origin, cg.refdef.vieworg);
		VectorCopy(ent->oldorigin, cg.refdef.vieworg);
		VectorCopy(cent->lerpAngles, cg.refdefViewAngles);

		// store camera position for locked view
		VectorCopy(cg.refdef.vieworg, cgs.demoCamera.camOrigin);

		// extract x,y and z view distance of demo_followDistance cvar
		Q_strncpyz(distance, demo_followDistance.string, sizeof(demo_followDistance.string));

		disValue = strtok(distance, " ");
		for (count = 0; count < 3 && disValue; ++count)
		{
			dis[count] = Q_atoi(disValue);
			disValue   = strtok(NULL, " ,");
		}

		if (dis[0] == -99999)
		{
			CG_Printf("Warning: demo_followDistance cvar is missing the x value ('%s') - set to default 50\n", demo_followDistance.string);
			dis[0] = 50;
		}

		if (dis[1] == -99999)
		{
			CG_Printf("Warning: demo_followDistance cvar is missing the y value ('%s') - set to default 0\n", demo_followDistance.string);
			dis[1] = 0;
		}

		if (dis[2] == -99999)
		{
			CG_Printf("Warning: demo_followDistance cvar is missing the z value ('%s') - set to default 20\n", demo_followDistance.string);
			dis[2] = 20;
		}

		//CG_Printf("'%s'-> -%i %i %i\n", demo_followDistance.string, dis[0], dis[1], dis[2]);

		VectorMA(cg.refdef.vieworg, -dis[0], cg.refdef.viewaxis[0], cg.refdef.vieworg);
		VectorMA(cg.refdef.vieworg, dis[1], cg.refdef.viewaxis[1], cg.refdef.vieworg);
		VectorMA(cg.refdef.vieworg, dis[2], cg.refdef.viewaxis[2], cg.refdef.vieworg);

		VectorCopy(cent->rawAngles, cgs.demoCamera.camAngle);
	}
}

extern pmove_t cg_pmove; // cg_predict.c
extern void CG_TraceCapsule_World(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);   // cg_predict.c

/**
 * @brief CG_EDV_RunInput
 */
void CG_EDV_RunInput(void)
{
	playerState_t edv_ps;
	pmoveExt_t    edv_pmext;
	static int    lasttime = 0;
	int           i, delta, count;
	vec_t         frametime;
	char          speedValues[MAX_CVAR_VALUE_STRING];
	char          *speedValue;
	float         speed[3] = { -99999, -99999, -99999 };

	static vec3_t mins = { -6, -6, -6 };
	static vec3_t maxs = { 6, 6, 6 };

	trap_GetUserCmd(trap_GetCurrentCmdNumber(), &cg_pmove.cmd);

	// anti cl_freezedemo
	cg_pmove.cmd.serverTime = trap_Milliseconds();

	// calculate time so we can hack in angles
	delta    = cg.time - lasttime;
	lasttime = cg.time;

	// semi-arbitrary - matches min/max values in pmove
	if (delta < 1)
	{
		delta = 1;
	}
	else if (delta > 200)
	{
		delta = 200;
	}

	frametime = delta / 1000.0f;

	// client engine insists on "helping" cgame if PERS_HWEAPON_USE is set.
	// so we substitute saved cmd from etpro_RunBindingBuf() and tell client to FOAD
	if (cg.snap->ps.persistant[PERS_HWEAPON_USE] && cgs.demoCamera.renderingFreeCam)
	{
		//CG_Printf( "%d %d %d\n", edv_rightmove, edv_forwardmove, edv_upmove );
		// I still don't like this
		cg_pmove.cmd.forwardmove  = (cgs.demoCamera.move & 0x01) ? 127 : 0;
		cg_pmove.cmd.forwardmove += (cgs.demoCamera.move & 0x02) ? -127 : 0;
		cg_pmove.cmd.rightmove    = (cgs.demoCamera.move & 0x04) ? 127 : 0;
		cg_pmove.cmd.rightmove   += (cgs.demoCamera.move & 0x08) ? -127 : 0;
		cg_pmove.cmd.upmove       = (cgs.demoCamera.move & 0x10) ? 127 : 0;
		cg_pmove.cmd.upmove      += (cgs.demoCamera.move & 0x20) ? -127 : 0;
	}

	// extract yawturn-, pitchturn-. and rollspeed values of demo_yawPitchRollSpeed
	Q_strncpyz(speedValues, demo_yawPitchRollSpeed.string, sizeof(demo_yawPitchRollSpeed.string));

	speedValue = strtok(speedValues, " ");
	for (count = 0; count < 3 && speedValue; ++count)
	{
		speed[count] = atof(speedValue);
		speedValue   = strtok(NULL, " ,"); // note: speed values are float - fear the ','
	}

	if (speed[0] == -99999) // yawturnspeed
	{
		CG_Printf("Warning: demo_yawPitchRollSpeed cvar is missing the yawturnspeed value ('%s') - set to default 140\n", demo_yawPitchRollSpeed.string);
		speed[0] = 140;
	}

	if (speed[1] == -99999) // pitchturnspeed
	{
		CG_Printf("Warning: demo_yawPitchRollSpeed cvar is missing the pitchturnspeed value ('%s') - set to default 140\n", demo_yawPitchRollSpeed.string);
		speed[1] = 140;
	}

	if (speed[2] == -99999) // rollspeed
	{
		CG_Printf("Warning: demo_yawPitchRollSpeed cvar is missing the rollspeed value ('%s') - set to default 140\n", demo_yawPitchRollSpeed.string);
		speed[2] = 140;
	}

	// run turns, I still don't like this
	cg.refdefViewAngles[YAW]   += (cgs.demoCamera.turn & 0x01) ? speed[0] * frametime : 0; // yawturnspeed
	cg.refdefViewAngles[YAW]   += (cgs.demoCamera.turn & 0x02) ? -speed[0] * frametime : 0;
	cg.refdefViewAngles[PITCH] += (cgs.demoCamera.turn & 0x04) ? speed[1] * frametime : 0; // pitchturnspeed
	cg.refdefViewAngles[PITCH] += (cgs.demoCamera.turn & 0x08) ? -speed[1] * frametime : 0;
	cg.refdefViewAngles[ROLL]  += (cgs.demoCamera.turn & 0x10) ? speed[2] * frametime : 0; // rollspeed
	cg.refdefViewAngles[ROLL]  += (cgs.demoCamera.turn & 0x20) ? -speed[2] * frametime : 0;

	// Use current viewangles instead of the command angles;
	// looking is handled elsewhere (where, i do not know)
	for (i = 0; i < 3; i++)
	{
		cg_pmove.cmd.angles[i] = ANGLE2SHORT(cg.refdefViewAngles[i]);
	}

	cg_pmove.cmd.buttons &= ~BUTTON_TALK; // FIXME: Why is the engine talking?

	// Create a playerState for cam movement
	Com_Memset(&edv_ps, 0, sizeof(edv_ps));
	edv_ps.commandTime = cgs.demoCamera.commandTime;
	if (cgs.demoCamera.noclip)
	{
		edv_ps.pm_type = PM_NOCLIP;
	}
	else
	{
		edv_ps.pm_type = PM_SPECTATOR;
	}
	edv_ps.pm_flags        = 0;
	edv_ps.gravity         = 0;
	edv_ps.friction        = 5.0f;
	edv_ps.groundEntityNum = ENTITYNUM_NONE;
	edv_ps.clientNum       = cg.predictedPlayerState.clientNum;
	edv_ps.eFlags          = 0;
	VectorCopy(mins, edv_ps.mins);
	VectorCopy(maxs, edv_ps.maxs);

	// added speed cvar
	edv_ps.speed            = demo_freecamspeed.integer;
	edv_ps.runSpeedScale    = 0.8f;
	edv_ps.sprintSpeedScale = 1.1f;
	edv_ps.crouchSpeedScale = 0.25;

	VectorSet(edv_ps.delta_angles, 0, 0, 0);
	VectorCopy(cg.refdefViewAngles, edv_ps.viewangles);
	VectorCopy(cgs.demoCamera.camOrigin, edv_ps.origin);
	VectorCopy(cgs.demoCamera.velocity, edv_ps.velocity);

	edv_ps.crouchMaxZ = edv_ps.maxs[2] - (edv_ps.standViewHeight - edv_ps.crouchViewHeight);

	// Create pmext for cam movement
	Com_Memset(&edv_pmext, 0, sizeof(edv_pmext));
	edv_pmext.sprintTime = SPRINTTIME;

	// Fill in pmove stuff
	cg_pmove.ps        = &edv_ps;
	cg_pmove.pmext     = &edv_pmext;
	cg_pmove.character = CG_CharacterForClientinfo(&cgs.clientinfo[cg.snap->ps.clientNum], &cg_entities[cg.snap->ps.clientNum]);
	cg_pmove.skill     = cgs.clientinfo[cg.snap->ps.clientNum].skill;

	cg_pmove.trace         = CG_TraceCapsule_World;
	cg_pmove.tracemask     = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	cg_pmove.pointcontents = CG_PointContents;
	cg_pmove.noFootsteps   = qtrue;
	cg_pmove.noWeapClips   = qtrue;

	// Do the move
	Pmove(&cg_pmove);

	// Update cam
	cgs.demoCamera.commandTime = edv_ps.commandTime;
	VectorCopy(edv_ps.origin, cgs.demoCamera.camOrigin);
	VectorCopy(edv_ps.velocity, cgs.demoCamera.velocity);
	// angles, too
	VectorCopy(edv_ps.viewangles, cg.refdefViewAngles);
}

/**
 * @brief CG_DrawPVShint
 * @details Draws a tracer from the demo-player to the camera
 * so you always know in which direction you have to
 * travel to be back in the pvs
 * set cg_railtrailtime to 10 or smth -> no uses a hardcoded "10" value
 * so the dumb users don't have to mess with the railtrailtime
 */
void CG_DrawPVShint(void)
{
	//vec4_t color; // static green
	vec3_t origin;

	//BG_setCrosshair(b_tjl_color.string, color, 1.0, "b_tjl_color");

	VectorCopy(cgs.demoCamera.camOrigin, origin);

	// move the origin to the bottom of the screen
	origin[2] -= 6;

	CG_DrawLine(origin, cg.snap->ps.origin, colorGreen, cgs.media.railCoreShader);
}
#endif
