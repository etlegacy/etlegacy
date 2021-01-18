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
 * @file g_antilag.c
 */

#include "g_local.h"

/**
 * @brief Used below to interpolate between two previous vectors
 * @param[in] start start vector
 * @param[in] end end vector
 * @param[in] frac fraction with which to interpolate
 * @param[out] result A vector "frac" times the distance between "start" and "end"
 */
static void TimeShiftLerp(const vec3_t start, const vec3_t end, float frac, vec3_t result)
{
	result[0] = start[0] + frac * (end[0] - start[0]);
	result[1] = start[1] + frac * (end[1] - start[1]);
	result[2] = start[2] + frac * (end[2] - start[2]);
}

/**
 * @brief Check if this entity can be antilagged safely
 * @param[in] ent which to check
 * @return if it is safe to antilag this entity
 */
static qboolean G_AntilagSafe(gentity_t *ent)
{
	if (!ent)
	{
		return qfalse;
	}

	if (!ent->inuse)
	{
		return qfalse;
	}

	if (!ent->r.linked)
	{
		return qfalse;
	}

	if (!ent->client)
	{
		return qfalse;
	}

	// No bots allowed
	//if (ent->r.svFlags & SVF_BOT)
	//{
	//	return qfalse;
	//}

	if (ent->client->sess.sessionTeam != TEAM_AXIS && ent->client->sess.sessionTeam != TEAM_ALLIES)
	{
		return qfalse;
	}

	if ((ent->client->ps.pm_flags & PMF_LIMBO))
	{
		return qfalse;
	}

	// realhead support
	// restore players who have just died to keep the corpse head box in sync.
	if (ent->client->backupMarker.time == level.time && ent->client->ps.pm_type == PM_DEAD && g_realHead.integer)
	{
		return qtrue;
	}

	if (ent->health <= 0)
	{
		return qfalse;
	}

	// don't store clientMarkers for corpses, etc
	if (!(ent->client->ps.pm_type == PM_NORMAL))
	{
		return qfalse;
	}

	// don't store clientMarkers for the player on the tank
	if ((ent->client->ps.eFlags & EF_MOUNTEDTANK))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Store client entity's position and other related data which is required to shift time (B2TF)
 * @param[in,out] ent target client entity
 */
void G_StoreClientPosition(gentity_t *ent)
{
	int top;

	if (!G_AntilagSafe(ent))
	{
		return;
	}

	ent->client->topMarker++;
	if (ent->client->topMarker >= MAX_CLIENT_MARKERS)
	{
		ent->client->topMarker = 0;
	}

	top = ent->client->topMarker;

	VectorCopy(ent->r.mins, ent->client->clientMarkers[top].mins);
	VectorCopy(ent->r.maxs, ent->client->clientMarkers[top].maxs);
	VectorCopy(ent->s.pos.trBase, ent->client->clientMarkers[top].origin);
	ent->client->clientMarkers[top].time = level.time;

	// store all angles & frame info

	VectorCopy(ent->s.apos.trBase, ent->client->clientMarkers[top].viewangles);

	ent->client->clientMarkers[top].eFlags     = ent->s.eFlags;
	ent->client->clientMarkers[top].pm_flags   = ent->client->ps.pm_flags;
	ent->client->clientMarkers[top].viewheight = ent->client->ps.viewheight;

	// Torso Markers
	ent->client->clientMarkers[top].torsoOldFrameModel = ent->torsoFrame.oldFrameModel;
	ent->client->clientMarkers[top].torsoFrameModel    = ent->torsoFrame.frameModel;
	ent->client->clientMarkers[top].torsoOldFrame      = ent->torsoFrame.oldFrame;
	ent->client->clientMarkers[top].torsoFrame         = ent->torsoFrame.frame;
	ent->client->clientMarkers[top].torsoOldFrameTime  = ent->torsoFrame.oldFrameTime;
	ent->client->clientMarkers[top].torsoFrameTime     = ent->torsoFrame.frameTime;
	ent->client->clientMarkers[top].torsoYawAngle      = ent->torsoFrame.yawAngle;
	ent->client->clientMarkers[top].torsoPitchAngle    = ent->torsoFrame.pitchAngle;
	ent->client->clientMarkers[top].torsoYawing        = ent->torsoFrame.yawing;
	ent->client->clientMarkers[top].torsoPitching      = ent->torsoFrame.pitching;

	// Legs Markers
	ent->client->clientMarkers[top].legsOldFrameModel = ent->legsFrame.oldFrameModel;
	ent->client->clientMarkers[top].legsFrameModel    = ent->legsFrame.frameModel;
	ent->client->clientMarkers[top].legsOldFrame      = ent->legsFrame.oldFrame;
	ent->client->clientMarkers[top].legsFrame         = ent->legsFrame.frame;
	ent->client->clientMarkers[top].legsOldFrameTime  = ent->legsFrame.oldFrameTime;
	ent->client->clientMarkers[top].legsFrameTime     = ent->legsFrame.frameTime;
	ent->client->clientMarkers[top].legsYawAngle      = ent->legsFrame.yawAngle;
	ent->client->clientMarkers[top].legsPitchAngle    = ent->legsFrame.pitchAngle;
	ent->client->clientMarkers[top].legsYawing        = ent->legsFrame.yawing;
	ent->client->clientMarkers[top].legsPitching      = ent->legsFrame.pitching;
}

/**
 * @brief Move a client back to where he was at the specified "time"
 * @param[in,out] ent client entity which to shift
 * @param[in] time timestamp which to use
 * @return true if adjusted, otherwise false
 */
static qboolean G_AdjustSingleClientPosition(gentity_t *ent, int time)
{
	int i, j;

	if (time > level.time)
	{
		time = level.time;
	} // no lerping forward....

	if (!G_AntilagSafe(ent))
	{
		return qfalse;
	}

	// find a pair of markers which bound the requested time
	i = j = ent->client->topMarker;
	do
	{
		if (ent->client->clientMarkers[i].time <= time)
		{
			break;
		}

		j = i;
		i--;
		if (i < 0)
		{
			i = MAX_CLIENT_MARKERS - 1;
		}
	}
	while (i != ent->client->topMarker);

	if (i == j)     // oops, no valid stored markers
	{
		return qfalse;
	}

	// save current position to backup if we have not already done so ( no need to re-save )
	if (ent->client->backupMarker.time != level.time)
	{
		VectorCopy(ent->r.currentOrigin, ent->client->backupMarker.origin);
		VectorCopy(ent->r.mins, ent->client->backupMarker.mins);
		VectorCopy(ent->r.maxs, ent->client->backupMarker.maxs);

		// Head, Legs
		VectorCopy(ent->client->ps.viewangles, ent->client->backupMarker.viewangles);
		ent->client->backupMarker.eFlags     = ent->client->ps.eFlags;
		ent->client->backupMarker.pm_flags   = ent->client->ps.pm_flags;
		ent->client->backupMarker.viewheight = ent->client->ps.viewheight;

		ent->client->backupMarker.time = level.time;

		// Torso Markers
		ent->client->backupMarker.torsoOldFrameModel = ent->torsoFrame.oldFrameModel;
		ent->client->backupMarker.torsoFrameModel    = ent->torsoFrame.frameModel;
		ent->client->backupMarker.torsoOldFrame      = ent->torsoFrame.oldFrame;
		ent->client->backupMarker.torsoFrame         = ent->torsoFrame.frame;
		ent->client->backupMarker.torsoOldFrameTime  = ent->torsoFrame.oldFrameTime;
		ent->client->backupMarker.torsoFrameTime     = ent->torsoFrame.frameTime;
		ent->client->backupMarker.torsoYawAngle      = ent->torsoFrame.yawAngle;
		ent->client->backupMarker.torsoPitchAngle    = ent->torsoFrame.pitchAngle;
		ent->client->backupMarker.torsoYawing        = ent->torsoFrame.yawing;
		ent->client->backupMarker.torsoPitching      = ent->torsoFrame.pitching;

		// Legs Markers
		ent->client->backupMarker.legsOldFrameModel = ent->legsFrame.oldFrameModel;
		ent->client->backupMarker.legsFrameModel    = ent->legsFrame.frameModel;
		ent->client->backupMarker.legsOldFrame      = ent->legsFrame.oldFrame;
		ent->client->backupMarker.legsFrame         = ent->legsFrame.frame;
		ent->client->backupMarker.legsOldFrameTime  = ent->legsFrame.oldFrameTime;
		ent->client->backupMarker.legsFrameTime     = ent->legsFrame.frameTime;
		ent->client->backupMarker.legsYawAngle      = ent->legsFrame.yawAngle;
		ent->client->backupMarker.legsPitchAngle    = ent->legsFrame.pitchAngle;
		ent->client->backupMarker.legsYawing        = ent->legsFrame.yawing;
		ent->client->backupMarker.legsPitching      = ent->legsFrame.pitching;
	}

	// if we haven't wrapped back to the head, we've sandwiched, so
	// we shift the client's position back to where he was at "time"
	if (i != ent->client->topMarker)
	{
		float frac = (float)(time - ent->client->clientMarkers[i].time) /
		             (float)(ent->client->clientMarkers[j].time - ent->client->clientMarkers[i].time);
		// Using TimeShiftLerp since it follows the client exactly meaning less roundoff error instead of LerpPosition()
		TimeShiftLerp(
			ent->client->clientMarkers[i].origin,
			ent->client->clientMarkers[j].origin,
			frac,
			ent->r.currentOrigin);
		TimeShiftLerp(
			ent->client->clientMarkers[i].mins,
			ent->client->clientMarkers[j].mins,
			frac,
			ent->r.mins);
		TimeShiftLerp(
			ent->client->clientMarkers[i].maxs,
			ent->client->clientMarkers[j].maxs,
			frac,
			ent->r.maxs);

		// These are for Head / Legs
		ent->client->ps.viewangles[0] = LerpAngle(
			ent->client->clientMarkers[i].viewangles[0],
			ent->client->clientMarkers[j].viewangles[0],
			frac);
		ent->client->ps.viewangles[1] = LerpAngle(
			ent->client->clientMarkers[i].viewangles[1],
			ent->client->clientMarkers[j].viewangles[1],
			frac);
		ent->client->ps.viewangles[2] = LerpAngle(
			ent->client->clientMarkers[i].viewangles[2],
			ent->client->clientMarkers[j].viewangles[2],
			frac);
		// Set the ints to the closest ones in time since you can't lerp them.
		if ((ent->client->clientMarkers[j].time - time) < (time - ent->client->clientMarkers[i].time))
		{
			ent->client->ps.eFlags     = ent->client->clientMarkers[j].eFlags;
			ent->client->ps.pm_flags   = ent->client->clientMarkers[j].pm_flags;
			ent->client->ps.viewheight = ent->client->clientMarkers[j].viewheight;

			// Torso Markers
			ent->torsoFrame.oldFrameModel = ent->client->clientMarkers[j].torsoOldFrameModel;
			ent->torsoFrame.frameModel    = ent->client->clientMarkers[j].torsoFrameModel;
			ent->torsoFrame.oldFrame      = ent->client->clientMarkers[j].torsoOldFrame;
			ent->torsoFrame.frame         = ent->client->clientMarkers[j].torsoFrame;
			ent->torsoFrame.oldFrameTime  = ent->client->clientMarkers[j].torsoOldFrameTime;
			ent->torsoFrame.frameTime     = ent->client->clientMarkers[j].torsoFrameTime;
			ent->torsoFrame.yawAngle      = ent->client->clientMarkers[j].torsoYawAngle;
			ent->torsoFrame.pitchAngle    = ent->client->clientMarkers[j].torsoPitchAngle;
			ent->torsoFrame.yawing        = ent->client->clientMarkers[j].torsoYawing;
			ent->torsoFrame.pitching      = ent->client->clientMarkers[j].torsoPitching;

			// Legs Markers
			ent->legsFrame.oldFrameModel = ent->client->clientMarkers[j].legsOldFrameModel;
			ent->legsFrame.frameModel    = ent->client->clientMarkers[j].legsFrameModel;
			ent->legsFrame.oldFrame      = ent->client->clientMarkers[j].legsOldFrame;
			ent->legsFrame.frame         = ent->client->clientMarkers[j].legsFrame;
			ent->legsFrame.oldFrameTime  = ent->client->clientMarkers[j].legsOldFrameTime;
			ent->legsFrame.frameTime     = ent->client->clientMarkers[j].legsFrameTime;
			ent->legsFrame.yawAngle      = ent->client->clientMarkers[j].legsYawAngle;
			ent->legsFrame.pitchAngle    = ent->client->clientMarkers[j].legsPitchAngle;
			ent->legsFrame.yawing        = ent->client->clientMarkers[j].legsYawing;
			ent->legsFrame.pitching      = ent->client->clientMarkers[j].legsPitching;

			// time stamp for BuildHead/Leg
			ent->timeShiftTime = ent->client->clientMarkers[j].time;

		}
		else
		{
			ent->client->ps.eFlags     = ent->client->clientMarkers[i].eFlags;
			ent->client->ps.pm_flags   = ent->client->clientMarkers[i].pm_flags;
			ent->client->ps.viewheight = ent->client->clientMarkers[i].viewheight;

			// Torso Markers
			ent->torsoFrame.oldFrameModel = ent->client->clientMarkers[i].torsoOldFrameModel;
			ent->torsoFrame.frameModel    = ent->client->clientMarkers[i].torsoFrameModel;
			ent->torsoFrame.oldFrame      = ent->client->clientMarkers[i].torsoOldFrame;
			ent->torsoFrame.frame         = ent->client->clientMarkers[i].torsoFrame;
			ent->torsoFrame.oldFrameTime  = ent->client->clientMarkers[i].torsoOldFrameTime;
			ent->torsoFrame.frameTime     = ent->client->clientMarkers[i].torsoFrameTime;
			ent->torsoFrame.yawAngle      = ent->client->clientMarkers[i].torsoYawAngle;
			ent->torsoFrame.pitchAngle    = ent->client->clientMarkers[i].torsoPitchAngle;
			ent->torsoFrame.yawing        = ent->client->clientMarkers[i].torsoYawing;
			ent->torsoFrame.pitching      = ent->client->clientMarkers[i].torsoPitching;

			// Legs Markers
			ent->legsFrame.oldFrameModel = ent->client->clientMarkers[i].legsOldFrameModel;
			ent->legsFrame.frameModel    = ent->client->clientMarkers[i].legsFrameModel;
			ent->legsFrame.oldFrame      = ent->client->clientMarkers[i].legsOldFrame;
			ent->legsFrame.frame         = ent->client->clientMarkers[i].legsFrame;
			ent->legsFrame.oldFrameTime  = ent->client->clientMarkers[i].legsOldFrameTime;
			ent->legsFrame.frameTime     = ent->client->clientMarkers[i].legsFrameTime;
			ent->legsFrame.yawAngle      = ent->client->clientMarkers[i].legsYawAngle;
			ent->legsFrame.pitchAngle    = ent->client->clientMarkers[i].legsPitchAngle;
			ent->legsFrame.yawing        = ent->client->clientMarkers[i].legsYawing;
			ent->legsFrame.pitching      = ent->client->clientMarkers[i].legsPitching;

			// time stamp for BuildHead/Leg
			ent->timeShiftTime = ent->client->clientMarkers[i].time;
		}

		/* FIXME
		if ( debugger && debugger->client) {
		    // print some debugging stuff exactly like what the client does
		    // it starts with "Rec:" to let you know it backward-reconciled
		    char msg[2048];
		    Com_sprintf( msg, sizeof(msg),
		        "print \"^1Rec: time: %d, j: %d, k: %d, origin: %0.2f %0.2f %0.2f\n"
		        "^2frac: %0.4f, origin1: %0.2f %0.2f %0.2f, origin2: %0.2f %0.2f %0.2f\n"
		        "^7level.time: %d, est time: %d, level.time delta: %d, est real ping: %d\n\"",
		        time, ent->client->clientMarkers[i].time, ent->client->clientMarkers[j].time,
		        ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2],
		        frac,
		        ent->client->clientMarkers[i].origin[0],
		        ent->client->clientMarkers[i].origin[1],
		        ent->client->clientMarkers[i].origin[2],
		        ent->client->clientMarkers[j].origin[0],
		        ent->client->clientMarkers[j].origin[1],
		        ent->client->clientMarkers[j].origin[2],
		        level.time, level.time + debugger->client->frameOffset,
		        level.time - time, level.time + debugger->client->frameOffset - time);

		    trap_SendServerCommand( debugger - g_entities, msg );
		}
		*/
	}
	else
	{
		VectorCopy(ent->client->clientMarkers[j].origin, ent->r.currentOrigin);
		VectorCopy(ent->client->clientMarkers[j].mins, ent->r.mins);
		VectorCopy(ent->client->clientMarkers[j].maxs, ent->r.maxs);

		// BuildHead/Legs uses these
		VectorCopy(ent->client->clientMarkers[j].viewangles, ent->client->ps.viewangles);
		ent->client->ps.eFlags     = ent->client->clientMarkers[j].eFlags;
		ent->client->ps.pm_flags   = ent->client->clientMarkers[j].pm_flags;
		ent->client->ps.viewheight = ent->client->clientMarkers[j].viewheight;

		// Torso Markers
		ent->torsoFrame.oldFrameModel = ent->client->clientMarkers[j].torsoOldFrameModel;
		ent->torsoFrame.frameModel    = ent->client->clientMarkers[j].torsoFrameModel;
		ent->torsoFrame.oldFrame      = ent->client->clientMarkers[j].torsoOldFrame;
		ent->torsoFrame.frame         = ent->client->clientMarkers[j].torsoFrame;
		ent->torsoFrame.oldFrameTime  = ent->client->clientMarkers[j].torsoOldFrameTime;
		ent->torsoFrame.frameTime     = ent->client->clientMarkers[j].torsoFrameTime;
		ent->torsoFrame.yawAngle      = ent->client->clientMarkers[j].torsoYawAngle;
		ent->torsoFrame.pitchAngle    = ent->client->clientMarkers[j].torsoPitchAngle;
		ent->torsoFrame.yawing        = ent->client->clientMarkers[j].torsoYawing;
		ent->torsoFrame.pitching      = ent->client->clientMarkers[j].torsoPitching;

		// Legs Markers
		ent->legsFrame.oldFrameModel = ent->client->clientMarkers[j].legsOldFrameModel;
		ent->legsFrame.frameModel    = ent->client->clientMarkers[j].legsFrameModel;
		ent->legsFrame.oldFrame      = ent->client->clientMarkers[j].legsOldFrame;
		ent->legsFrame.frame         = ent->client->clientMarkers[j].legsFrame;
		ent->legsFrame.oldFrameTime  = ent->client->clientMarkers[j].legsOldFrameTime;
		ent->legsFrame.frameTime     = ent->client->clientMarkers[j].legsFrameTime;
		ent->legsFrame.yawAngle      = ent->client->clientMarkers[j].legsYawAngle;
		ent->legsFrame.pitchAngle    = ent->client->clientMarkers[j].legsPitchAngle;
		ent->legsFrame.yawing        = ent->client->clientMarkers[j].legsYawing;
		ent->legsFrame.pitching      = ent->client->clientMarkers[j].legsPitching;

		// time stamp for BuildHead/Leg
		ent->timeShiftTime = ent->client->clientMarkers[j].time;
	}

	trap_LinkEntity(ent);

	return qtrue;
}

/**
 * @brief Move a client back to where he was before the time shift
 * @param[in,out] ent client entity which to shift
 * @return true if re-adjusted, otherwise false
 */
qboolean G_ReAdjustSingleClientPosition(gentity_t *ent)
{
	if (!G_AntilagSafe(ent))
	{
		return qfalse;
	}

	// restore from backup
	if (ent->client->backupMarker.time == level.time)
	{
		VectorCopy(ent->client->backupMarker.origin, ent->r.currentOrigin);
		VectorCopy(ent->client->backupMarker.mins, ent->r.mins);
		VectorCopy(ent->client->backupMarker.maxs, ent->r.maxs);

		// Head, Legs stuff
		VectorCopy(ent->client->backupMarker.viewangles, ent->client->ps.viewangles);

		ent->client->ps.eFlags     = ent->client->backupMarker.eFlags;
		ent->client->ps.pm_flags   = ent->client->backupMarker.pm_flags;
		ent->client->ps.viewheight = ent->client->backupMarker.viewheight;


		ent->client->backupMarker.time = 0;

		// Torso Markers
		ent->torsoFrame.oldFrameModel = ent->client->backupMarker.torsoOldFrameModel;
		ent->torsoFrame.frameModel    = ent->client->backupMarker.torsoFrameModel;
		ent->torsoFrame.oldFrame      = ent->client->backupMarker.torsoOldFrame;
		ent->torsoFrame.frame         = ent->client->backupMarker.torsoFrame;
		ent->torsoFrame.oldFrameTime  = ent->client->backupMarker.torsoOldFrameTime;
		ent->torsoFrame.frameTime     = ent->client->backupMarker.torsoFrameTime;
		ent->torsoFrame.yawAngle      = ent->client->backupMarker.torsoYawAngle;
		ent->torsoFrame.pitchAngle    = ent->client->backupMarker.torsoPitchAngle;
		ent->torsoFrame.yawing        = ent->client->backupMarker.torsoYawing;
		ent->torsoFrame.pitching      = ent->client->backupMarker.torsoPitching;

		// Legs Markers
		ent->legsFrame.oldFrameModel = ent->client->backupMarker.legsOldFrameModel;
		ent->legsFrame.frameModel    = ent->client->backupMarker.legsFrameModel;
		ent->legsFrame.oldFrame      = ent->client->backupMarker.legsOldFrame;
		ent->legsFrame.frame         = ent->client->backupMarker.legsFrame;
		ent->legsFrame.oldFrameTime  = ent->client->backupMarker.legsOldFrameTime;
		ent->legsFrame.frameTime     = ent->client->backupMarker.legsFrameTime;
		ent->legsFrame.yawAngle      = ent->client->backupMarker.legsYawAngle;
		ent->legsFrame.pitchAngle    = ent->client->backupMarker.legsPitchAngle;

		// time stamp for BuildHead/Leg
		ent->timeShiftTime = 0;

		trap_LinkEntity(ent);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Move ALL clients back to where they were at the specified "time", except for "skip"
 * @param[in] skip Client to skip (the one shooting currently)
 * @param[in] time timestamp which to use
 * @param[in] backwards are we going back or forward in time (are we restoring the original location)
 */
static void G_AdjustClientPositions(gentity_t *skip, int time, qboolean backwards)
{
	int       i;
	gentity_t *list;

	for (i = 0; i < level.numConnectedClients; i++, list++)
	{
		list = g_entities + level.sortedClients[i];

		// dont adjust the firing client entity
		if (list == skip)
		{
			continue;
		}

		if (backwards)
		{
			G_AdjustSingleClientPosition(list, time);
		}
		else
		{
			G_ReAdjustSingleClientPosition(list);
		}
	}
}

/**
 * @brief Clear out the given client's history (should be called on client spawn or teleport)
 * @param[in,out] ent client entity which to reset history
 */
void G_ResetMarkers(gentity_t *ent)
{
	int   i, time;
	float period = sv_fps.value;
	int   eFlags;

	if (period <= 0.f)
	{
		period = 50;
	}
	else
	{
		period = 1000.f / period;
	}

	eFlags = ent->client->ps.eFlags;
	// don't save entity flags that are not allowed for clientMarkers
	if (eFlags & EF_MOUNTEDTANK)
	{
		eFlags &= ~EF_MOUNTEDTANK;
	}

	ent->client->topMarker = MAX_CLIENT_MARKERS - 1;
	for (i = MAX_CLIENT_MARKERS - 1, time = level.time; i >= 0; i--, time -= period)
	{
		VectorCopy(ent->r.mins, ent->client->clientMarkers[i].mins);
		VectorCopy(ent->r.maxs, ent->client->clientMarkers[i].maxs);
		VectorCopy(ent->r.currentOrigin, ent->client->clientMarkers[i].origin);
		ent->client->clientMarkers[i].time = time;
		VectorCopy(ent->client->ps.viewangles, ent->client->clientMarkers[i].viewangles);
		ent->client->clientMarkers[i].eFlags     = eFlags;
		ent->client->clientMarkers[i].pm_flags   = ent->client->ps.pm_flags;
		ent->client->clientMarkers[i].viewheight = ent->client->ps.viewheight;

		// Torso Markers
		ent->client->clientMarkers[i].torsoOldFrameModel = ent->torsoFrame.oldFrameModel;
		ent->client->clientMarkers[i].torsoFrameModel    = ent->torsoFrame.frameModel;
		ent->client->clientMarkers[i].torsoOldFrame      = ent->torsoFrame.oldFrame;
		ent->client->clientMarkers[i].torsoFrame         = ent->torsoFrame.frame;
		ent->client->clientMarkers[i].torsoOldFrameTime  = ent->torsoFrame.oldFrameTime;
		ent->client->clientMarkers[i].torsoFrameTime     = ent->torsoFrame.frameTime;
		ent->client->clientMarkers[i].torsoYawAngle      = ent->torsoFrame.yawAngle;
		ent->client->clientMarkers[i].torsoPitchAngle    = ent->torsoFrame.pitchAngle;
		ent->client->clientMarkers[i].torsoYawing        = ent->torsoFrame.yawing;
		ent->client->clientMarkers[i].torsoPitching      = ent->torsoFrame.pitching;

		// Legs Markers
		ent->client->clientMarkers[i].legsOldFrameModel = ent->legsFrame.oldFrameModel;
		ent->client->clientMarkers[i].legsFrameModel    = ent->legsFrame.frameModel;
		ent->client->clientMarkers[i].legsOldFrame      = ent->legsFrame.oldFrame;
		ent->client->clientMarkers[i].legsFrame         = ent->legsFrame.frame;
		ent->client->clientMarkers[i].legsOldFrameTime  = ent->legsFrame.oldFrameTime;
		ent->client->clientMarkers[i].legsFrameTime     = ent->legsFrame.frameTime;
		ent->client->clientMarkers[i].legsYawAngle      = ent->legsFrame.yawAngle;
		ent->client->clientMarkers[i].legsPitchAngle    = ent->legsFrame.pitchAngle;
		ent->client->clientMarkers[i].legsYawing        = ent->legsFrame.yawing;
		ent->client->clientMarkers[i].legsPitching      = ent->legsFrame.pitching;
	}
	// time stamp for BuildHead/Leg
	ent->timeShiftTime = 0;
}

// This variable needs to be here in order for G_BuildLeg() to access it..
static grefEntity_t refent;

/**
 * @brief G_AttachBodyParts
 * @param[in] ent
 */
static void G_AttachBodyParts(gentity_t *ent)
{
	int       i;
	gentity_t *list;

	for (i = 0; i < level.numConnectedClients; i++, list++)
	{
		list = g_entities + level.sortedClients[i];
		// ok lets test everything under the sun
		if (list->inuse &&
		    (list->client->sess.sessionTeam == TEAM_AXIS || list->client->sess.sessionTeam == TEAM_ALLIES) &&
		    (list != ent) &&
		    list->r.linked &&
		    !(list->client->ps.pm_flags & PMF_LIMBO) &&
		    (list->client->ps.pm_type == PM_NORMAL || list->client->ps.pm_type == PM_DEAD)
		    )
		{
			list->client->tempHead = G_BuildHead(list, &refent, qtrue);
			list->client->tempLeg  = G_BuildLeg(list, &refent, qfalse);
		}
		else
		{
			list->client->tempHead = NULL;
			list->client->tempLeg  = NULL;
		}
	}
}

/**
 * @brief G_DettachBodyParts
 */
static void G_DettachBodyParts(void)
{
	int       i;
	gentity_t *list;

	for (i = 0; i < level.numConnectedClients; i++, list++)
	{
		list = g_entities + level.sortedClients[i];
		if (list->client->tempHead)
		{
			G_FreeEntity(list->client->tempHead);
		}
		if (list->client->tempLeg)
		{
			G_FreeEntity(list->client->tempLeg);
		}
	}
}

/**
 * @brief G_SwitchBodyPartEntity
 * @param[in] ent
 * @return
 */
static int G_SwitchBodyPartEntity(gentity_t *ent)
{
	if (ent->s.eType == ET_TEMPHEAD)
	{
		return ent->parent - g_entities;
	}
	if (ent->s.eType == ET_TEMPLEGS)
	{
		return ent->parent - g_entities;
	}
	return ent - g_entities;
}

#define POSITION_READJUST                       \
	if (res != results->entityNum) {               \
		VectorSubtract(end, start, dir);          \
		VectorNormalizeFast(dir);             \
                                    \
		VectorMA(results->endpos, -1, dir, results->endpos);  \
		results->entityNum = res;               \
	}

/**
 * @brief Run a trace with players in historical positions.
 * @param[in] ent
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param[in] passEntityNum
 * @param[in] contentmask
 */
void G_HistoricalTrace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	if (!g_antilag.integer || !ent->client)
	{
		G_Trace(ent, results, start, mins, maxs, end, passEntityNum, contentmask);
		return;
	}

	G_AdjustClientPositions(ent, ent->client->pers.cmd.serverTime, qtrue);

	G_Trace(ent, results, start, mins, maxs, end, passEntityNum, contentmask);

	G_AdjustClientPositions(ent, 0, qfalse);
}

/**
 * @brief G_HistoricalTraceBegin
 * @param[in] ent
 */
void G_HistoricalTraceBegin(gentity_t *ent)
{
	// don't do this with antilag off
	if (!g_antilag.integer)
	{
		return;
	}
	G_AdjustClientPositions(ent, ent->client->pers.cmd.serverTime, qtrue);
}

/**
 * @brief G_HistoricalTraceEnd
 * @param[in] ent
 */
void G_HistoricalTraceEnd(gentity_t *ent)
{
	// don't do this with antilag off
	if (!g_antilag.integer)
	{
		return;
	}
	G_AdjustClientPositions(ent, 0, qfalse);
}

/**
 * @brief Run a trace without fixups (historical fixups will be done externally)
 * @param[in] ent
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param[in] passEntityNum
 * @param[in] contentmask
 */
void G_Trace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	vec3_t dir;
	int    res;

	float maxsBackup[MAX_CLIENTS] ;
	int   i, clientNum;

	G_AttachBodyParts(ent);

	for (i = 0; i < level.numConnectedClients; ++i)
	{
		clientNum             = level.sortedClients[i];
		maxsBackup[clientNum] = g_entities[clientNum].r.maxs[2];

		// use higher hitbox for syringe only
		if (ent->s.weapon == WP_MEDIC_SYRINGE && (g_entities[clientNum].s.eFlags & (EF_DEAD | EF_PRONE)))
		{
			g_entities[clientNum].r.maxs[2] = CROUCH_BODYHEIGHT;
		}
		else
		{
			g_entities[clientNum].r.maxs[2] = ClientHitboxMaxZ(&g_entities[clientNum]);
		}
	}

	trap_Trace(results, start, mins, maxs, end, passEntityNum, contentmask);

	// backup hitbox height
	for (i = 0; i < level.numConnectedClients; ++i)
	{
		clientNum                       = level.sortedClients[i];
		g_entities[clientNum].r.maxs[2] = maxsBackup[clientNum];
	}

	res = G_SwitchBodyPartEntity(&g_entities[results->entityNum]);
	POSITION_READJUST

	G_DettachBodyParts();
}

/**
 * @brief G_SkipCorrectionSafe
 * @param[in] ent
 * @return
 */
static qboolean G_SkipCorrectionSafe(gentity_t *ent)
{
	if (!ent)
	{
		return qfalse;
	}

	if (!ent->inuse)
	{
		return qfalse;
	}

	if (!ent->r.linked)
	{
		return qfalse;
	}

	if (!ent->client)
	{
		return qfalse;
	}

	if (ent->client->sess.sessionTeam != TEAM_AXIS
	    && ent->client->sess.sessionTeam != TEAM_ALLIES)
	{
		return qfalse;
	}

	if ((ent->client->ps.pm_flags & PMF_LIMBO) ||
	    (ent->client->ps.pm_flags & PMF_TIME_LOCKPLAYER))
	{
		return qfalse;
	}

	if (ent->health <= 0)
	{
		return qfalse;
	}

	if (ent->client->ps.pm_type != PM_NORMAL)
	{
		return qfalse;
	}

	// this causes segfault with new PM_TraceLegs() from 2.60 because it wants to update pm.pmext which
	// we don't bother passing.
	if ((ent->client->ps.eFlags & EF_PRONE))
	{
		return qfalse;
	}

	if ((ent->client->ps.eFlags & EF_MOUNTEDTANK))
	{
		return qfalse;
	}

	// perhaps 2 would be OK too?
	if (ent->waterlevel > 1)
	{
		return qfalse;
	}

	// don't bother with skip correction if the player isn't moving horizontally
	if (ent->client->ps.velocity[0] == 0.f && ent->client->ps.velocity[1] == 0.f)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Make use of the 'Pmove' functions to figure out where a player
 * would have moved in the short amount of time indicated by frametime.
 *
 * After the Pmove is complete, copy the values to the player's entity
 * state, but then copy the original player state values back to the
 * player state so that the player's movements aren't effected in any way.
 *
 * The only Pmove functions used are PM_StepSlideMove() and
 * PM_GroundTrace() since these are all that is needed for this
 * short period of time.  This means that a lot of movement types
 * cannot be predicted and no prediction should be done for them
 * See G_SkipCorrectionSafe()
 *
 * @param[in,out] ent
 * @param[in] frametime
 */
void G_PredictPmove(gentity_t *ent, float frametime)
{
	pmove_t   pm;
	vec3_t    origin;
	vec3_t    velocity;
	int       groundEntityNum;
	int       pm_flags;
	int       pm_time;
	int       eFlags;
	gclient_t *client;

	if (!G_SkipCorrectionSafe(ent))
	{
		return;
	}

	client = ent->client;

	// backup all the playerState values that may be altered by PmovePredict()
	VectorCopy(client->ps.origin, origin);
	VectorCopy(client->ps.velocity, velocity);
	groundEntityNum = client->ps.groundEntityNum;
	pm_flags        = client->ps.pm_flags;
	pm_time         = client->ps.pm_time;
	eFlags          = client->ps.eFlags;

	Com_Memset(&pm, 0, sizeof(pm));
	pm.ps            = &client->ps;
	pm.pmext         = &client->pmext;
	pm.character     = client->pers.character;
	pm.trace         = trap_TraceCapsuleNoEnts;
	pm.pointcontents = trap_PointContents;
	pm.tracemask     = MASK_PLAYERSOLID;
	VectorCopy(ent->r.mins, pm.mins);
	VectorCopy(ent->r.maxs, pm.maxs);
	pm.predict = qtrue;
	//pm.debugLevel = 1;

	PmovePredict(&pm, frametime);

	// update entity state with the resulting player state
	VectorCopy(client->ps.origin, ent->s.pos.trBase);
	VectorCopy(client->ps.velocity, ent->s.pos.trDelta);
	ent->s.groundEntityNum = client->ps.groundEntityNum;
	ent->s.eFlags          = client->ps.eFlags;

	// this needs to be updated in case g_antilag is not enabled
	VectorCopy(client->ps.origin, ent->r.currentOrigin);
	// link them to stop them from skipping over mines and being unhittable when antilag is 0
	trap_LinkEntity(ent);

	// restore player state so that their next command will process as if nothing has happened.
	VectorCopy(origin, client->ps.origin);
	VectorCopy(velocity, client->ps.velocity);
	client->ps.groundEntityNum = groundEntityNum;
	client->ps.pm_flags        = pm_flags;
	client->ps.pm_time         = pm_time;
	client->ps.eFlags          = eFlags;
}
