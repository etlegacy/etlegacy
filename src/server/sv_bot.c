/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file sv_bot.c
 */

#include "server.h"
#include "../botlib/botlib.h"

#ifdef FEATURE_TRACKER
#include "sv_tracker.h"
#endif

static bot_debugpoly_t debugpolygons[MAX_DEBUGPOLYS];

extern botlib_export_t *botlib_export;

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient(int clientNum)
{
	int      i;
	client_t *cl;

	// added possibility to request a clientnum
	if (clientNum > 0)
	{
		if (clientNum >= sv_maxclients->integer)
		{
			return -1;
		}

		cl = &svs.clients[clientNum];
		if (cl->state != CS_FREE)
		{
			return -1;
		}
		else
		{
			i = clientNum;
		}
	}
	else
	{
		// find a client slot
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
			if (i < 1)
			{
				continue;
			}

			if (cl->state == CS_FREE)
			{
				break; // done.
			}
		}
	}

	if (i == sv_maxclients->integer)
	{
		Com_DPrintf("SV_BotAllocateClient: can't allocate a bot client.\n");
		return -1;
	}

	cl->gentity                    = SV_GentityNum(i);
	cl->gentity->s.number          = i;
	cl->state                      = CS_ACTIVE;
	cl->lastPacketTime             = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate                       = 16384;

#ifdef FEATURE_TRACKER
	Tracker_catchBotConnect(cl - svs.clients);
#endif

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient(int clientNum)
{
	client_t *cl;

	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum);
	}
	cl          = &svs.clients[clientNum];
	cl->state   = CS_FREE;
	cl->name[0] = 0;
	if (cl->gentity)
	{
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
BotDrawDebugPolygons

TODO: remove in cm_patch.c
==================
*/
void BotDrawDebugPolygons(BotPolyFunc drawPoly, int value)
{
	return;
}

/*
==================
BotImport_Print
==================
*/
static __attribute__ ((format(printf, 2, 3))) void QDECL BotImport_Print(int type, char *fmt, ...)
{
	char    str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch (type)
	{
	case PRT_MESSAGE: {
		Com_Printf("%s", str);
		break;
	}
	case PRT_WARNING: {
		Com_Printf(S_COLOR_YELLOW "Warning: %s", str);
		break;
	}
	case PRT_ERROR: {
		Com_Printf(S_COLOR_RED "Error: %s", str);
		break;
	}
	case PRT_FATAL: {
		Com_Printf(S_COLOR_RED "Fatal: %s", str);
		break;
	}
	case PRT_EXIT: {
		Com_Error(ERR_DROP, S_COLOR_RED "Exit: %s", str);
		break;
	}
	default: {
		Com_Printf("unknown print type\n");
		break;
	}
	}
}

/*
==================
BotImport_Trace
==================
*/
void BotImport_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
	trace_t trace;

	// always use bounding box for bot stuff ?
	SV_Trace(&trace, start, mins, maxs, end, passent, contentmask, qfalse);
	// copy the trace information
	bsptrace->allsolid   = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction   = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type     = trace.plane.type;
	bsptrace->surface.value  = trace.surfaceFlags;
	bsptrace->ent            = trace.entityNum;
	bsptrace->exp_dist       = 0;
	bsptrace->sidenum        = 0;
	bsptrace->contents       = 0;
}

/*
==================
BotImport_EntityTrace
==================
*/
void BotImport_EntityTrace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask)
{
	trace_t trace;

	// always use bounding box for bot stuff ?
	SV_ClipToEntity(&trace, start, mins, maxs, end, entnum, contentmask, qfalse);
	// copy the trace information
	bsptrace->allsolid   = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction   = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type     = trace.plane.type;
	bsptrace->surface.value  = trace.surfaceFlags;
	bsptrace->ent            = trace.entityNum;
	bsptrace->exp_dist       = 0;
	bsptrace->sidenum        = 0;
	bsptrace->contents       = 0;
}

/*
==================
BotImport_PointContents
==================
*/
int BotImport_PointContents(vec3_t point)
{
	return SV_PointContents(point, -1);
}

/*
==================
BotImport_inPVS
==================
*/
int BotImport_inPVS(vec3_t p1, vec3_t p2)
{
	return SV_inPVS(p1, p2);
}

/*
==================
BotImport_BSPEntityData
==================
*/
char *BotImport_BSPEntityData(void)
{
	return CM_EntityString();
}

/*
==================
BotImport_BSPModelMinsMaxsOrigin
==================
*/
void BotImport_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin)
{
	clipHandle_t h = CM_InlineModel(modelnum);
	vec3_t       mins, maxs;

	CM_ModelBounds(h, mins, maxs);
	// if the model is rotated
	if ((angles[0] || angles[1] || angles[2]))
	{
		// expand for rotation
		float max = RadiusFromBounds(mins, maxs);
		int   i;

		for (i = 0; i < 3; i++)
		{
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if (outmins)
	{
		VectorCopy(mins, outmins);
	}
	if (outmaxs)
	{
		VectorCopy(maxs, outmaxs);
	}
	if (origin)
	{
		VectorClear(origin);
	}
}

/*
==================
BotImport_GetMemory
==================
*/
void *BotImport_GetMemory(int size)
{
	void *ptr;

	ptr = Z_TagMalloc(size, TAG_BOTLIB);
	return ptr;
}

/*
==================
BotImport_FreeMemory
==================
*/
void BotImport_FreeMemory(void *ptr)
{
	Z_Free(ptr);
}

/*
==================
BotImport_FreeZoneMemory
==================
*/
void BotImport_FreeZoneMemory(void)
{
	Z_FreeTags(TAG_BOTLIB);
}

/*
=================
BotImport_HunkAlloc
=================
*/
void *BotImport_HunkAlloc(int size)
{
	if (Hunk_CheckMark())
	{
		Com_Error(ERR_DROP, "SV_Bot_HunkAlloc: Alloc with marks already set");
	}
	return Hunk_Alloc(size, h_high);
}

/*
==================
BotImport_DebugPolygonCreate
==================
*/
int BotImport_DebugPolygonCreate(int color, int numPoints, vec3_t *points)
{
	bot_debugpoly_t *poly;
	int             i;

	for (i = 1; i < MAX_DEBUGPOLYS; i++)
	{
		if (!debugpolygons[i].inuse)
		{
			break;
		}
	}
	if (i >= MAX_DEBUGPOLYS)
	{
		return 0;
	}
	poly            = &debugpolygons[i];
	poly->inuse     = qtrue;
	poly->color     = color;
	poly->numPoints = numPoints;
	memcpy(poly->points, points, numPoints * sizeof(vec3_t));

	return i;
}

/*
==================
BotImport_DebugPolygonCreate
==================
*/
bot_debugpoly_t *BotImport_GetFreeDebugPolygon(void)
{
	int i;

	for (i = 1; i < MAX_DEBUGPOLYS; i++)
	{
		if (!debugpolygons[i].inuse)
		{
			return &debugpolygons[i];
		}
	}
	return NULL;
}

/*
==================
BotImport_DebugPolygonShow
==================
*/
void BotImport_DebugPolygonShow(int id, int color, int numPoints, vec3_t *points)
{
	bot_debugpoly_t *poly = &debugpolygons[id];

	poly->inuse     = qtrue;
	poly->color     = color;
	poly->numPoints = numPoints;
	memcpy(poly->points, points, numPoints * sizeof(vec3_t));
}

/*
==================
BotImport_DebugPolygonDelete
==================
*/
void BotImport_DebugPolygonDelete(int id)
{
	debugpolygons[id].inuse = qfalse;
}

/*
==================
BotImport_DebugPolygonDeletePointer
==================
*/
void BotImport_DebugPolygonDeletePointer(bot_debugpoly_t *pPoly)
{
	pPoly->inuse = qfalse;
}

/*
==================
BotImport_DebugLineCreate
==================
*/
int BotImport_DebugLineCreate(void)
{
	vec3_t points[1];
	return BotImport_DebugPolygonCreate(0, 0, points);
}

/*
==================
BotImport_DebugLineDelete
==================
*/
void BotImport_DebugLineDelete(int line)
{
	BotImport_DebugPolygonDelete(line);
}

/*
==================
BotImport_DebugLineShow
==================
*/
void BotImport_DebugLineShow(int line, vec3_t start, vec3_t end, int color)
{
	vec3_t points[4], dir, cross, up = { 0, 0, 1 };
	float  dot;

	VectorCopy(start, points[0]);
	VectorCopy(start, points[1]);
	//points[1][2] -= 2;
	VectorCopy(end, points[2]);
	//points[2][2] -= 2;
	VectorCopy(end, points[3]);

	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99 || dot < -0.99)
	{
		VectorSet(cross, 1, 0, 0);
	}
	else
	{
		CrossProduct(dir, up, cross);
	}

	VectorNormalize(cross);

	VectorMA(points[0], 2, cross, points[0]);
	VectorMA(points[1], -2, cross, points[1]);
	VectorMA(points[2], -2, cross, points[2]);
	VectorMA(points[3], 2, cross, points[3]);

	BotImport_DebugPolygonShow(line, color, 4, points);
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand(int client, char *command)
{
	SV_ExecuteClientCommand(&svs.clients[client], command, qtrue, qfalse);
}

#ifndef DEDICATED
void BotImport_DrawPolygon(int color, int numpoints, float *points);
#else
/*
==================
BotImport_DrawPolygon
==================
*/
void BotImport_DrawPolygon(int color, int numpoints, float *points)
{
	Com_DPrintf("BotImport_DrawPolygon stub\n");
}
#endif

/*
==================
SV_BotInitBotLib
==================
*/
void SV_BotInitBotLib(void)
{
	botlib_import_t botlib_import;

	botlib_import.Print                  = BotImport_Print;
	botlib_import.Trace                  = BotImport_Trace;
	botlib_import.EntityTrace            = BotImport_EntityTrace;
	botlib_import.PointContents          = BotImport_PointContents;
	botlib_import.inPVS                  = BotImport_inPVS;
	botlib_import.BSPEntityData          = BotImport_BSPEntityData;
	botlib_import.BSPModelMinsMaxsOrigin = BotImport_BSPModelMinsMaxsOrigin;
	botlib_import.BotClientCommand       = BotClientCommand;

	//memory management
	botlib_import.GetMemory      = BotImport_GetMemory;
	botlib_import.FreeMemory     = BotImport_FreeMemory;
	botlib_import.FreeZoneMemory = BotImport_FreeZoneMemory;
	botlib_import.HunkAlloc      = BotImport_HunkAlloc;

	// file system acess
	botlib_import.FS_FOpenFile  = FS_FOpenFileByMode;
	botlib_import.FS_Read       = FS_Read;
	botlib_import.FS_Write      = FS_Write;
	botlib_import.FS_FCloseFile = FS_FCloseFile;
	botlib_import.FS_Seek       = FS_Seek;

	// debug lines
	botlib_import.DebugLineCreate = BotImport_DebugLineCreate;
	botlib_import.DebugLineDelete = BotImport_DebugLineDelete;
	botlib_import.DebugLineShow   = BotImport_DebugLineShow;

	// debug polygons
	botlib_import.DebugPolygonCreate        = BotImport_DebugPolygonCreate;
	botlib_import.DebugPolygonGetFree       = BotImport_GetFreeDebugPolygon;
	botlib_import.DebugPolygonDelete        = BotImport_DebugPolygonDelete;
	botlib_import.DebugPolygonDeletePointer = BotImport_DebugPolygonDeletePointer;

	botlib_import.BotDrawPolygon = BotImport_DrawPolygon;

	botlib_export = (botlib_export_t *)GetBotLibAPI(BOTLIB_API_VERSION, &botlib_import);
}

//  * * * BOT AI CODE IS BELOW THIS POINT * * *

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage(int client, char *buf, int size)
{
	client_t *cl = &svs.clients[client];
	int      index;

	cl->lastPacketTime = svs.time;

	if (cl->reliableAcknowledge == cl->reliableSequence)
	{
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1);

	if (!cl->reliableCommands[index][0])
	{
		return qfalse;
	}

	//Q_strncpyz( buf, cl->reliableCommands[index], size );
	return qtrue;
}
