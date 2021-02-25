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
 * @file cl_demo.c
 */

#include "client.h"

#ifdef ETLEGACY_DEBUG
#define DEMODEBUG(msg, ...) if (Cvar_VariableIntegerValue("demo_debug")) { Com_Printf("^1%s()^2: DEBUG " msg, __FUNCTION__, ## __VA_ARGS__); }
#define Com_FuncDPrinf(msg, ...) Com_DPrintf("%s(): " msg, __FUNCTION__, ## __VA_ARGS__)
#define Com_FuncPrinf(msg, ...) Com_Printf("%s(): " msg, __FUNCTION__, ## __VA_ARGS__)
#define Com_FuncError(msg, ...) Com_Error(ERR_FATAL, "%s(): " msg, __FUNCTION__, ## __VA_ARGS__)
#define Com_FuncDrop(msg, ...) Com_Error(ERR_DROP, "%s(): " msg, __FUNCTION__, ## __VA_ARGS__)
#else
#define DEMODEBUG(msg, ...)
#define Com_FuncDPrinf(msg, ...)
#define Com_FuncPrinf Com_Printf
#define Com_FuncError(...) Com_Error(ERR_FATAL, __VA_ARGS__)
#define Com_FuncDrop(...) Com_Error(ERR_DROP, __VA_ARGS__)
#endif

#define MEGABYTES(x) x / 1024.0 / 1024.0
#define MAX_REWIND_BACKUPS 20

#define NEW_DEMOFUNC 1

#if NEW_DEMOFUNC
typedef struct
{
	int numSnaps;
	int lastServerTime;
	int firstServerTime;
	int gameStartTime;
	int gameEndTime;
	//int serverFrameTime;

	//double wantedTime;
	int snapCount;

	int demoPos;
	int snapsInDemo;

	qboolean gotFirstSnap;
	qboolean skipSnap;

	//qboolean hasWarmup;
	qboolean seeking;

	double Overf;

	int firstNonDeltaMessageNumWritten;
} demoInfo_t;

typedef struct
{
	qboolean valid;
	int seekPoint;
	clientActive_t cl;
	clientConnection_t clc;
	clientStatic_t cls;
	int numSnaps;
} rewindBackups_t;

cvar_t *cl_maxRewindBackups;

demoInfo_t      di;
rewindBackups_t *rewindBackups   = NULL;
int             maxRewindBackups = 0;
#endif

demoPlayInfo_t dpi = { 0, 0 };

/**
 * @brief CL_WalkDemoExt
 * @param[in] arg
 * @param[in,out] name
 * @param[in,out] demofile
 * @return
 */
static int CL_WalkDemoExt(const char *arg, char *name, fileHandle_t *demofile)
{
	int i = 0;
	*demofile = 0;

	Com_sprintf(name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT, PROTOCOL_VERSION);
	FS_FOpenFileRead(name, demofile, qtrue);

	if (*demofile)
	{
		Com_FuncPrinf("Demo file: %s\n", name);
		return PROTOCOL_VERSION;
	}

	Com_FuncPrinf("Not found: %s\n", name);

	while (demo_protocols[i])
	{
		if (demo_protocols[i] == PROTOCOL_VERSION)
		{
			continue;
		}

		Com_sprintf(name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT, demo_protocols[i]);
		FS_FOpenFileRead(name, demofile, qtrue);
		if (*demofile)
		{
			Com_FuncPrinf("Demo file: %s\n", name);
			return demo_protocols[i];
		}
		else
		{
			Com_FuncPrinf("Not found: %s\n", name);
		}
		i++;
	}

	return -1;
}

// REWIND AND FASTFORWARD

#if NEW_DEMOFUNC
/**
 * @brief CL_PeekSnapshot
 * @param[in] snapshotNumber
 * @param[out] snapshot
 * @return
 */
qboolean CL_PeekSnapshot(int snapshotNumber, snapshot_t *snapshot)
{
	clSnapshot_t *clSnap;
	clSnapshot_t csn;
	int          i, count;
	int          origPosition;
	int          cmd;
	//char         *s;
	char     buffer[16];
	qboolean success = qfalse;
	int      r;
	msg_t    buf;
	byte     bufData[MAX_MSGLEN];
	int      j;
	int      lastPacketTimeOrig;
	int      parseEntitiesNumOrig;
	int      currentSnapNum;
	//int      serverMessageSequence;

	clSnap = &csn;

	if (!clc.demoplaying)
	{
		return qfalse;
	}

	if (snapshotNumber <= cl.snap.messageNum)
	{
		success = CL_GetSnapshot(snapshotNumber, snapshot);
		if (!success)
		{
			Com_FuncPrinf("snapshot number outside of backup buffer\n");
			return qfalse;
		}
		return qtrue;
	}

	if (snapshotNumber > cl.snap.messageNum + 1)
	{
		Com_FuncPrinf("FIXME CL_PeekSnapshot  %d >  cl.snap.messageNum + 1 (%d)\n", snapshotNumber, cl.snap.messageNum);
		return qfalse; // FIXME:
	}

	parseEntitiesNumOrig = cl.parseEntitiesNum;
	lastPacketTimeOrig   = clc.lastPacketTime;
	// CL_ReadDemoMessage()
	origPosition = FS_FTell(clc.demofile);

	if (origPosition < 0)
	{
		// FS_FTell prints the warning ...
		return qfalse;
	}

	currentSnapNum = cl.snap.messageNum;

	for (j = 0; j < snapshotNumber - currentSnapNum; j++)
	{
		// get the sequence number
		Com_Memset(buffer, 0, sizeof(buffer));
		r = FS_Read(&buffer, 4, clc.demofile);
		if (r != 4)
		{
			Com_FuncPrinf("couldn't read sequence number\n");
			(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime  = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}
		//serverMessageSequence = LittleLong(*((int *)buffer));

		// init the message
		Com_Memset(&buf, 0, sizeof(msg_t));
		MSG_Init(&buf, bufData, sizeof(bufData));

		// get the length
		r = FS_Read(&buf.cursize, 4, clc.demofile);
		if (r != 4)
		{
			Com_FuncPrinf("couldn't get length\n");
			(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime  = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}

		buf.cursize = LittleLong(buf.cursize);
		if (buf.cursize == -1)
		{
			Com_FuncPrinf("buf.cursize == -1\n");
			(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime  = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}

		if (buf.cursize > buf.maxsize)
		{
			Com_FuncDrop("demoMsglen > MAX_MSGLEN");
		}

		r = FS_Read(buf.data, buf.cursize, clc.demofile);
		if (r != buf.cursize)
		{
			Com_FuncPrinf("Demo file was truncated.\n");
			(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime  = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qfalse;
		}

		clc.lastPacketTime = cls.realtime;
		buf.readcount      = 0;

		MSG_Bitstream(&buf);
		// get the reliable sequence acknowledge number
		MSG_ReadLong(&buf);

		// parse the message
		while (qtrue)
		{
			if (buf.readcount > buf.cursize)
			{
				Com_FuncDrop("read past end of server message");
			}

			cmd = MSG_ReadByte(&buf);

			if (cmd == svc_EOF)
			{
				break;
			}
			success = qfalse;

			switch (cmd)
			{
			default:
				Com_FuncDrop("Illegible server message");
			case svc_nop:
				break;
			case svc_serverCommand:
				MSG_ReadLong(&buf);  // seq
				//s = MSG_ReadString(&buf);
				MSG_ReadString(&buf);
				break;
			case svc_gamestate:
				Com_FuncPrinf("FIXME gamestate\n");
				goto alldone;
			case svc_snapshot:
				// TODO: changed this check if it works
				CL_ParseSnapshot(&buf);
				if (cl.snap.valid)
				{
					success = qtrue;
				}
				break;
			case svc_download:
				Com_FuncPrinf("FIXME download\n");
				goto alldone;
			}
		}

alldone:

		if (!success)
		{
			Com_FuncPrinf("failed\n");
			(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime  = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return success;
		}

		// FIXME other ents not supported yet

		// if the entities in the frame have fallen out of their
		// circular buffer, we can't return it
		if (cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES)
		{
			Com_FuncPrinf("cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES");
			(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
			clc.lastPacketTime  = lastPacketTimeOrig;
			cl.parseEntitiesNum = parseEntitiesNumOrig;
			return qtrue;  // FIXME if you fix other ents
		}

		// write the snapshot
		snapshot->snapFlags             = clSnap->snapFlags;
		snapshot->serverCommandSequence = clSnap->serverCommandNum;
		snapshot->ping                  = clSnap->ping;
		snapshot->serverTime            = clSnap->serverTime;
		Com_Memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
		snapshot->ps = clSnap->ps;
		count        = clSnap->numEntities;

		if (count > MAX_ENTITIES_IN_SNAPSHOT)
		{
			Com_FuncPrinf("truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT);
			count = MAX_ENTITIES_IN_SNAPSHOT;
		}

		snapshot->numEntities = count;
		for (i = 0; i < count; i++)
		{
			snapshot->entities[i] = cl.parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
		}
	}

	(void) FS_Seek(clc.demofile, origPosition, FS_SEEK_SET);
	clc.lastPacketTime  = lastPacketTimeOrig;
	cl.parseEntitiesNum = parseEntitiesNumOrig;
	// TODO: configstring changes and server commands!!!

	return qtrue;
}

/**
 * @brief CL_DemoFastForward
 * @param[in] wantedTime
 */
static void CL_DemoFastForward(double wantedTime)
{
	int loopCount;

	if (cls.state < CA_CONNECTED)
	{
		return;
	}

	if (wantedTime >= di.lastServerTime)
	{
		return;
	}

	DEMODEBUG("fast_forward %f\n", wantedTime);

	if (wantedTime >= (double)cl.serverTime  &&  wantedTime < (double)cl.snap.serverTime)
	{
		cl.serverTime      = (int)(floor(wantedTime));
		cls.realtime       = (int)(floor(wantedTime));
		di.Overf           = wantedTime - floor(wantedTime);
		cl.serverTimeDelta = 0;
		return;
	}

	if (wantedTime < (double)cl.snap.serverTime)
	{
		Com_FuncPrinf("This should not happen %f < %d\n", wantedTime, cl.snap.serverTime);
		return;
	}

	if (wantedTime > (double)di.lastServerTime)
	{
		Com_FuncPrinf("would seek past end of demo (%f > %d)\n", wantedTime, di.lastServerTime);
		return;
	}

	DEMODEBUG("fastfowarding from %f to %f\n", (double)cl.serverTime + di.Overf, wantedTime);

	loopCount = 0;
	while ((double)cl.snap.serverTime <= wantedTime)
	{
		DEMODEBUG("Servertime: %d wanted time %lf\n", cl.snap.serverTime, wantedTime);
		CL_ReadDemoMessage();
		while (clc.lastExecutedServerCommand < clc.serverCommandSequence)
		{
			if (clc.lastExecutedServerCommand + 1 <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS)
			{
				if (cl.snap.serverTime <= di.firstServerTime)
				{
					clc.lastExecutedServerCommand = clc.serverCommandSequence - 1;
					Com_FuncPrinf("setting clc.lastExecutedServerCommand %d (%d)\n", clc.lastExecutedServerCommand, loopCount);
				}
				else
				{
					Com_FuncDPrinf("FIXME %i  (%i) + 1 <= (%i) - MAX_RELIABLE_COMMANDS (%i)\n", clc.lastExecutedServerCommand, clc.serverCommandSequence, cl.snap.serverTime, di.firstServerTime);
					break;
				}
			}
			CL_GetServerCommand(clc.lastExecutedServerCommand + 1);
		}
		loopCount++;
	}

	DEMODEBUG("read %d demo messages, cl.snap.serverTime %d, wantedTime %f\n", loopCount, cl.snap.serverTime, wantedTime);

	cl.serverTime      = (int)(floor(wantedTime));
	cls.realtime       = (int)(floor(wantedTime));
	di.Overf           = wantedTime - floor(wantedTime);
	cl.serverTimeDelta = 0;

	// TODO: fix this
	//VM_Call(cgvm, CG_TIME_CHANGE, cl.serverTime, (int)(Overf * SUBTIME_RESOLUTION));

	di.seeking                        = qtrue;
	di.firstNonDeltaMessageNumWritten = -1;
}

/**
 * @brief CL_RewindDemo
 * @param[in] wantedTime
 */
static void CL_RewindDemo(double wantedTime)
{
	int             i;
	rewindBackups_t *rb;

	if (!IS_DEFAULT_MOD)
	{
		Com_FuncPrinf("Rewind is only supported on %s mod, sorry\n", DEFAULT_MODGAME);
		return;
	}

	if (wantedTime < (double)di.firstServerTime)
	{
		wantedTime = di.firstServerTime;
	}

	if (!rewindBackups[0].valid || di.snapCount == 0)
	{
		CL_DemoFastForward(wantedTime);
		return;
	}

	rb = NULL;
	for (i = di.snapCount - 1; i >= 0; i--)
	{
		rb = &rewindBackups[i];
		// go back a second before wanted time in order to have snapshot backups available for screen matching
		if ((double)rb->cl.snap.serverTime < wantedTime - 1000.0)
		{
			break;
		}
	}
	if (rb == NULL || i < 0)
	{
		if (rb == NULL)
		{
			Com_FuncPrinf("FIXME rewind couldn't find valid snap  rb:%p  i:%d  rb serverTime %d   wanted %f\n", (void *)rb, i, rewindBackups[0].cl.snap.serverTime, wantedTime);
		}
		rb = &rewindBackups[0];
		i  = 0;
	}

	DEMODEBUG("seeking to index %d %d   cl.serverTime:%d  cl.snap.serverTime:%d, new clc.lastExecutedServercommand %d  clc.serverCommandSequence %d\n", i, rb->seekPoint, cl.serverTime, cl.snap.serverTime, rb->clc.lastExecutedServerCommand, rb->clc.serverCommandSequence);
	(void) FS_Seek(clc.demofile, rb->seekPoint, FS_SEEK_SET);

	// TODO: take a look at these hacks
	di.numSnaps  = rb->numSnaps;
	di.snapCount = i + 1;

	Com_Memcpy(&cl, &rb->cl, sizeof(clientActive_t));
	Com_Memcpy(&clc, &rb->clc, sizeof(clientConnection_t));
	Com_Memcpy(&cls, &rb->cls, sizeof(clientStatic_t));
	di.Overf = 0;

	// TODO: this is a hack to set the state to something valid
	cls.state        = CA_ACTIVE;
	cls.keyCatchers |= KEYCATCH_CGAME;

	CL_DemoFastForward(wantedTime);
}

/**
 * @brief CL_DemoSeekMs
 * @param[in] ms
 * @param[in] exactServerTime
 */
static void CL_DemoSeekMs(double ms, int exactServerTime)  // server time in milliseconds
{
	double wantedTime;

	if (!clc.demoplaying)
	{
		Com_FuncPrinf("not playing demo can't seek\n");
		return;
	}

	wantedTime = ms;

	if (exactServerTime > 0)
	{
		wantedTime = exactServerTime;
	}

	DEMODEBUG("seek want %f\n", wantedTime);

	if (wantedTime > (double)cl.serverTime + di.Overf)
	{
		CL_DemoFastForward(wantedTime);
	}
	else
	{
		CL_RewindDemo(wantedTime);
	}
}

/**
 * @brief CL_ParseDemoSnapShotSimple
 * @param[in] msg
 */
static void CL_ParseDemoSnapShotSimple(msg_t *msg)
{
	int          len;
	clSnapshot_t *old;
	clSnapshot_t newSnap;
	int          deltaNum;
	int          oldMessageNum;
	int          i, packetNum;

	Com_Memset(&newSnap, 0, sizeof(newSnap));
	newSnap.serverCommandNum = clc.serverCommandSequence;
	newSnap.serverTime       = MSG_ReadLong(msg);
	newSnap.messageNum       = clc.serverMessageSequence;
	deltaNum                 = MSG_ReadByte(msg);
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte(msg);

	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = qtrue;      // uncompressed frame
		old           = NULL;
	}
	else
	{
		old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if (!old->valid)
		{
			// should never happen
			Com_FuncPrinf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_FuncPrinf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - 128)
		{
			Com_FuncPrinf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = qtrue;  // valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte(msg);

	if (len > sizeof(newSnap.areamask))
	{
		Com_FuncDrop("Invalid size %d for areamask.", len);
	}

	MSG_ReadData(msg, &newSnap.areamask, len);

	if (old)
	{
		MSG_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	CL_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	cl.snap      = newSnap;
	cl.snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK;
		if (cl.snap.ps.commandTime >= cl.outPackets[packetNum].p_serverTime)
		{
			cl.snap.ping = cls.realtime - cl.outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[cl.snap.messageNum & PACKET_MASK] = cl.snap;
	cl.newSnapshots                                = qtrue;
}

/**
 * @brief Do very shallow parse of the demo (could be extended) just to get times and snapshot count
 */
static void CL_ParseDemo(void)
{
	int tstart   = 0;
	int demofile = 0;

	// Reset our demo data
	Com_Memset(&di, 0, sizeof(di));

	// Parse start
	di.gameStartTime = -1;
	di.gameEndTime   = -1;
	(void) FS_Seek(clc.demofile, 0, FS_SEEK_SET);
	tstart = Sys_Milliseconds();

	while (qtrue)
	{
		int   r;
		msg_t buf;
		msg_t *msg;
		byte  bufData[MAX_MSGLEN];
		int   s;
		int   cmd;

		di.demoPos = FS_FTell(clc.demofile);

		// get the sequence number
		r = FS_Read(&s, 4, clc.demofile);
		if (r != 4)
		{
			CL_DemoCompleted();
			return;
		}

		clc.serverMessageSequence = LittleLong(s);

		// init the message
		MSG_Init(&buf, bufData, sizeof(bufData));

		// get the length
		r = FS_Read(&buf.cursize, 4, clc.demofile);

		if (r != 4)
		{
			break;
		}

		buf.cursize = LittleLong(buf.cursize);
		if (buf.cursize == -1)
		{
			break;
		}

		if (buf.cursize > buf.maxsize)
		{
			Com_FuncPrinf("demoMsglen > MAX_MSGLEN");
			break;
		}

		r = FS_Read(buf.data, buf.cursize, clc.demofile);
		if (r != buf.cursize)
		{
			Com_FuncPrinf("Demo file was truncated.\n");
			break;
		}

		clc.lastPacketTime = cls.realtime;
		buf.readcount      = 0;

		// parse
		msg = &buf;
		MSG_Bitstream(msg);

		// get the reliable sequence acknowledge number
		clc.reliableAcknowledge = MSG_ReadLong(msg);

		if (clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS)
		{
			clc.reliableAcknowledge = clc.reliableSequence;
		}

		// parse the message
		while (qtrue)
		{
			if (msg->readcount > msg->cursize)
			{
				Com_FuncDrop("read past end of server message");
			}

			cmd = MSG_ReadByte(msg);

			if (cmd == svc_EOF)
			{
				break;
			}

			// other commands
			switch (cmd)
			{
			default:
				Com_FuncDrop("Illegible server message %d", cmd);
			case svc_nop:
				break;
			case svc_serverCommand:
				MSG_ReadLong(msg);
				MSG_ReadString(msg);
				break;
			case svc_gamestate:
				clc.serverCommandSequence = MSG_ReadLong(msg);
				cl.gameState.dataCount    = 1;
				while (qtrue)
				{
					int cmd2 = MSG_ReadByte(msg);
					if (cmd2 == svc_EOF)
					{
						break;
					}
					if (cmd2 == svc_configstring)
					{
						MSG_ReadShort(msg);
						MSG_ReadBigString(msg);
					}
					else if (cmd2 == svc_baseline)
					{
						entityState_t s1, s2;
						Com_Memset(&s1, 0, sizeof(s1));
						Com_Memset(&s2, 0, sizeof(s2));
						MSG_ReadBits(msg, GENTITYNUM_BITS);
						MSG_ReadDeltaEntity(msg, &s1, &s2, 0);
					}
					else
					{
						Com_FuncDrop("bad command byte");
					}
				}
				MSG_ReadLong(msg);
				MSG_ReadLong(msg);
				break;
			case svc_snapshot:
				CL_ParseDemoSnapShotSimple(msg);
				break;
			case svc_download:
				MSG_ReadShort(msg);

				break;
			}
		}

		if (!cl.snap.valid)
		{
			Com_FuncPrinf("!cl.snap.valid\n");
			continue;
		}

		if (cl.snap.serverTime < cl.oldFrameServerTime)
		{
			// ignore snapshots that don't have entities
			if (cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE)
			{
				continue;
			}
			cls.state = CA_ACTIVE;

			// set the timedelta so we are exactly on this first frame
			cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
			cl.oldServerTime   = cl.snap.serverTime;

			clc.timeDemoBaseTime = cl.snap.serverTime;
		}
		cl.oldFrameServerTime = cl.snap.serverTime;

		if (cl.newSnapshots)
		{
			CL_AdjustTimeDelta();
		}

		di.lastServerTime = cl.snap.serverTime;
		if (di.firstServerTime == 0)
		{
			di.firstServerTime = cl.snap.serverTime;
			Com_FuncPrinf("firstServerTime %d\n", di.firstServerTime);
		}

		di.snapsInDemo++;
	}

	Com_FuncPrinf("Snaps in demo: %i\n", di.snapsInDemo);
	Com_FuncPrinf("last serverTime %d   total %f minutes\n", cl.snap.serverTime, (cl.snap.serverTime - di.firstServerTime) / 1000.0 / 60.0);
	Com_FuncPrinf("parse time %f seconds\n", (double)(Sys_Milliseconds() - tstart) / 1000.0);
	(void) FS_Seek(clc.demofile, 0, FS_SEEK_SET);
	clc.demoplaying = qfalse;
	demofile        = clc.demofile;
	CL_ClearState();
	Com_Memset(&clc, 0, sizeof(clc));
	Com_ClearDownload();
	clc.demofile             = demofile;
	cls.state                = CA_DISCONNECTED;
	cl_connectedToPureServer = qfalse;

	dpi.firstTime = di.firstServerTime;
	dpi.lastTime  = di.lastServerTime;
}

/**
 * @brief CL_FreeDemoPoints
 */
void CL_FreeDemoPoints(void)
{
	if (rewindBackups)
	{
		Com_Dealloc(rewindBackups);
		rewindBackups = NULL;
	}
}

/**
 * @brief CL_AllocateDemoPoints
 */
void CL_AllocateDemoPoints(void)
{
	CL_FreeDemoPoints();

	maxRewindBackups = cl_maxRewindBackups->integer;
	if (maxRewindBackups <= 0)
	{
		maxRewindBackups = MAX_REWIND_BACKUPS;
	}

	rewindBackups = (rewindBackups_t *)Com_Allocate(sizeof(rewindBackups_t) * maxRewindBackups);
	if (!rewindBackups)
	{
		Com_FuncError("couldn't allocate %.2f MB for rewind backups\n", MEGABYTES(sizeof(rewindBackups_t) * maxRewindBackups));
	}
	Com_FuncPrinf("allocated %.2f MB for rewind backups\n", MEGABYTES(sizeof(rewindBackups_t) * maxRewindBackups));
	Com_Memset(rewindBackups, 0, sizeof(rewindBackups_t) * maxRewindBackups);
}

#endif

/*
=======================================================================
CLIENT SIDE DEMO RECORDING
=======================================================================
*/

/**
 * @brief Dumps the current net message, prefixed by the length
 * @param[in] msg
 * @param[in] headerBytes
 */
void CL_WriteDemoMessage(msg_t *msg, int headerBytes)
{
	int len, swlen;

	// write the packet sequence
	len   = clc.serverMessageSequence;
	swlen = LittleLong(len);
	(void) FS_Write(&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len   = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	(void) FS_Write(&swlen, 4, clc.demofile);
	(void) FS_Write(msg->data + headerBytes, len, clc.demofile);
}

/**
 * @brief Stop recording a demo
 */
void CL_StopRecord_f(void)
{
	int len;

	if (!clc.demorecording)
	{
		Com_FuncPrinf("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	(void) FS_Write(&len, 4, clc.demofile);
	(void) FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;

	clc.demorecording = qfalse;
	Cvar_Set("cl_demorecording", "0");
	Cvar_Set("cl_demofilename", "");
	Cvar_Set("cl_demooffset", "0");
	Com_FuncPrinf("Stopped demo.\n");
}

/**
 * @brief CL_DemoFilename
 * @param[in] number
 * @param[in] fileName
 */
void CL_DemoFilename(int number, char *fileName)
{
	if (number < 0 || number > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "demo9999");
		return;
	}

	Com_sprintf(fileName, MAX_OSPATH, "demo%04i", number);
}

static char demoName[MAX_OSPATH];        // compiler bug workaround

/**
 * @brief Begins recording a demo from the current position
 */
void CL_Record_f(void)
{
	char name[MAX_OSPATH];
	char *s;

	if (Cmd_Argc() > 2)
	{
		Com_FuncPrinf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		Com_FuncPrinf("Already recording.\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		Com_FuncPrinf("You must be in a level to record.\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		Q_strncpyz(demoName, s, sizeof(demoName));
		Com_sprintf(name, sizeof(name), "demos/%s.%s%d", demoName, DEMOEXT, PROTOCOL_VERSION);
	}
	else
	{
		int number, len;

		// scan for a free demo name
		for (number = 0; number <= 9999; number++)
		{
			CL_DemoFilename(number, demoName);
			Com_sprintf(name, sizeof(name), "demos/%s.%s%d", demoName, DEMOEXT, PROTOCOL_VERSION);

			len = FS_ReadFile(name, NULL);
			if (len <= 0)
			{
				break;  // file doesn't exist
			}
		}
	}

	CL_Record(name);
}

/**
 * @brief CL_Record
 * @param[in] name
 */
void CL_Record(const char *name)
{
	int           i;
	msg_t         buf;
	byte          bufData[MAX_MSGLEN];
	entityState_t *ent;
	entityState_t nullstate;
	char          *s;
	int           len;

	// open the demo file
	Com_FuncPrinf("Recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Com_FuncPrinf("ERROR: couldn't open.\n");
		return;
	}

	clc.demorecording = qtrue;
	Cvar_Set("cl_demorecording", "1");    // fretn
	Q_strncpyz(clc.demoName, demoName, sizeof(clc.demoName));
	Cvar_Set("cl_demofilename", clc.demoName);    // bani
	Cvar_Set("cl_demooffset", "0");    // bani

	// don't start saving messages until a non-delta compressed message is received
	clc.demowaiting = qtrue;

	// write out the gamestate message
	MSG_Init(&buf, bufData, sizeof(bufData));
	MSG_Bitstream(&buf);

	// NOTE: all server->client messages now acknowledge
	MSG_WriteLong(&buf, clc.reliableSequence);

	MSG_WriteByte(&buf, svc_gamestate);
	MSG_WriteLong(&buf, clc.serverCommandSequence);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (!cl.gameState.stringOffsets[i])
		{
			continue;
		}
		s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		MSG_WriteByte(&buf, svc_configstring);
		MSG_WriteShort(&buf, i);
		MSG_WriteBigString(&buf, s);
	}

	// baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		ent = &cl.entityBaselines[i];
		if (!ent->number)
		{
			continue;
		}
		MSG_WriteByte(&buf, svc_baseline);
		MSG_WriteDeltaEntity(&buf, &nullstate, ent, qtrue);
	}

	MSG_WriteByte(&buf, svc_EOF);

	// finished writing the gamestate stuff

	// write the client num
	MSG_WriteLong(&buf, clc.clientNum);
	// write the checksum feed
	MSG_WriteLong(&buf, clc.checksumFeed);

	// finished writing the client packet
	MSG_WriteByte(&buf, svc_EOF);

	// write it to the demo file
	len = LittleLong(clc.serverMessageSequence - 1);
	(void) FS_Write(&len, 4, clc.demofile);

	len = LittleLong(buf.cursize);
	(void) FS_Write(&len, 4, clc.demofile);
	(void) FS_Write(buf.data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

/*
=======================================================================
CLIENT SIDE DEMO PLAYBACK
=======================================================================
*/

/**
 * @brief CL_DemoCleanUp
 */
void CL_DemoCleanUp(void)
{
	if (clc.demofile)
	{
		FS_FCloseFile(clc.demofile);
		clc.demofile = 0;
	}

#if NEW_DEMOFUNC
	CL_FreeDemoPoints();
#endif
}


/**
 * @brief CL_DemoCompleted
 */
void CL_DemoCompleted(void)
{
#if NEW_DEMOFUNC
	CL_FreeDemoPoints();
#endif

	if (cl_timedemo && cl_timedemo->integer)
	{
		int time;

		time = Sys_Milliseconds() - clc.timeDemoStart;
		if (time > 0)
		{
			Com_FuncPrinf("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
			              time / 1000.0, clc.timeDemoFrames * 1000.0 / time);
		}
	}

	if (CL_VideoRecording())
	{
		Cmd_ExecuteString("stopvideo");
	}

	if (clc.waverecording)
	{
		CL_WriteWaveClose();
		clc.waverecording = qfalse;
	}

	CL_Disconnect(qtrue);
	CL_NextDemo();
}

/**
 * @brief CL_DemoRun
 */
void CL_DemoRun(void)
{
#if NEW_DEMOFUNC
	int        loopCount = -1;
	int        startTime;
	static int lastTime = -1;
#endif

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if (cl_timedemo->integer)
	{
		if (!clc.timeDemoStart)
		{
			clc.timeDemoStart = Sys_Milliseconds();
		}
		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	if (cl_freezeDemo->integer)
	{
		return;
	}

#if NEW_DEMOFUNC
	startTime = cl.snap.serverTime;
#endif  // NEW_DEMOFUNC

	while (cl.serverTime >= cl.snap.serverTime)
	{
		// feed another message, which should change the contents of cl.snap
		CL_ReadDemoMessage();

#if NEW_DEMOFUNC
		loopCount++;
		DEMODEBUG("cl.serverTime >= cl.snap.serverTime   %d  %d  %d\n", cl.serverTime, cl.snap.serverTime, loopCount);

		if (cls.state == CA_ACTIVE  &&  cl.serverTime > cl.snap.serverTime)
		{
			if (com_timescale->value > 1.0f)
			{
				if (startTime == cl.snap.serverTime || lastTime == cl.snap.serverTime)
				{  // offline demo
					lastTime         = cl.snap.serverTime;
					cl.oldServerTime = lastTime;
					continue;
				}
				else
				{
					if (cl.serverTime > cl.snap.serverTime)
					{
						cl.serverTime      = cl.snap.serverTime;
						cls.realtime       = cl.snap.serverTime;
						cl.serverTimeDelta = 0;
						cl.oldServerTime   = lastTime;
					}
				}
			}
		}
#endif  // NEW_DEMOFUNC
		if (cls.state != CA_ACTIVE)
		{
			Cvar_Set("timescale", "1");
			return;     // end of demo
		}
	}
}

/**
 * @brief CL_ReadDemoMessage
 */
void CL_ReadDemoMessage(void)
{
	int   r;
	msg_t buf;
	byte  bufData[MAX_MSGLEN];
	int   s;

	if (!clc.demofile)
	{
		CL_DemoCompleted();
		return;
	}

#if NEW_DEMOFUNC
	di.numSnaps++;

	if (di.snapCount < maxRewindBackups &&
	    ((!di.gotFirstSnap  &&  !(cls.state >= CA_CONNECTED && cls.state < CA_PRIMED))
	     || (di.gotFirstSnap  &&  di.numSnaps % (di.snapsInDemo / maxRewindBackups) == 0)))
	{
		rewindBackups_t *rb;

		if (!di.skipSnap)
		{
			// first snap triggers loading screen when rewinding
			di.skipSnap = qtrue;
			goto keep_reading;
		}

		di.gotFirstSnap = qtrue;
		rb              = &rewindBackups[di.snapCount];

		if (!rb->valid)
		{
			rb->valid     = qtrue;
			rb->numSnaps  = di.numSnaps;
			rb->seekPoint = FS_FTell(clc.demofile);

			Com_Memcpy(&rb->cl, &cl, sizeof(clientActive_t));
			Com_Memcpy(&rb->clc, &clc, sizeof(clientConnection_t));
			Com_Memcpy(&rb->cls, &cls, sizeof(clientStatic_t));
		}
		di.snapCount++;
	}

keep_reading:

#endif

	// get the sequence number
	r = FS_Read(&s, 4, clc.demofile);
	if (r != 4)
	{
		CL_DemoCompleted();
		return;
	}

	clc.serverMessageSequence = LittleLong(s);

	// init the message
	MSG_Init(&buf, bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, clc.demofile);

	if (r != 4)
	{
		CL_DemoCompleted();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if (buf.cursize == -1)
	{
		CL_DemoCompleted();
		return;
	}

	if (buf.cursize > buf.maxsize)
	{
		Com_FuncDrop("demoMsglen > MAX_MSGLEN");
	}

	r = FS_Read(buf.data, buf.cursize, clc.demofile);
	if (r != buf.cursize)
	{
		Com_FuncPrinf("Demo file was truncated.\n");
		CL_DemoCompleted();
		return;
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount      = 0;
	CL_ParseServerMessage(&buf);
}

/**
 * @brief CL_CompleteDemoName
 * @param args - unused
 * @param[in] argNum
 */
static void CL_CompleteDemoName(char *args, int argNum)
{
	if (argNum == 2)
	{
		char demoExt[16];

		Com_sprintf(demoExt, sizeof(demoExt), ".%s%d", DEMOEXT, PROTOCOL_VERSION);
		Field_CompleteFilename("demos", demoExt, qtrue, qtrue);
	}
}

/**
 * @brief Usage: demo \<demoname\>
 */
void CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH], retry[MAX_OSPATH];
	char *arg, *ext_test;
	int  protocol, i;

	if (Cmd_Argc() != 2)
	{
		Com_FuncPrinf("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");

	CL_Disconnect(qtrue);

	Cvar_Set("cl_autorecord", "0");

	// open the demo file
	arg = Cmd_Argv(1);
	// check for an extension .DEMOEXT_?? (?? is protocol)
	ext_test = strrchr(arg, '.');

	if (ext_test && !Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1))
	{
		protocol = Q_atoi(ext_test + ARRAY_LEN(DEMOEXT));

		for (i = 0; demo_protocols[i]; i++)
		{
			if (demo_protocols[i] == protocol)
			{
				break;
			}
		}

		if (demo_protocols[i] || protocol == PROTOCOL_VERSION)
		{
			if (Sys_PathAbsolute(name))
			{
				FS_FOpenFileReadFullDir(name, &clc.demofile);
			}
			else
			{
				Com_sprintf(name, sizeof(name), "demos/%s", arg);
				FS_FOpenFileRead(name, &clc.demofile, qtrue);
			}
		}
		else
		{
			size_t len;

			Com_FuncPrinf("Protocol %d not supported for demos\n", protocol);
			len = ext_test - arg;

			if (len >= ARRAY_LEN(retry))
			{
				len = ARRAY_LEN(retry) - 1;
			}

			Q_strncpyz(retry, arg, len + 1);
			retry[len] = '\0';
			protocol   = CL_WalkDemoExt(retry, name, &clc.demofile);
		}
	}
	else
	{
		protocol = CL_WalkDemoExt(arg, name, &clc.demofile);
	}

	if (!clc.demofile)
	{
		Com_FuncDrop("couldn't open %s", name);
	}
	Q_strncpyz(clc.demoName, arg, sizeof(clc.demoName));

	Con_Close();

#if NEW_DEMOFUNC
	CL_AllocateDemoPoints();
	CL_ParseDemo();
#endif

	cls.state       = CA_CONNECTED;
	clc.demoplaying = qtrue;

	if (Cvar_VariableValue("cl_wavefilerecord") != 0.f)
	{
		CL_WriteWaveOpen();
	}

	Q_strncpyz(cls.servername, arg, sizeof(cls.servername));

	// read demo messages until connected
	while (cls.state >= CA_CONNECTED && cls.state < CA_PRIMED)
	{
		CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.firstDemoFrameSkipped = qfalse;
}

/**
 * @brief Called when a demo or cinematic finishes
 * If the "nextdemo" cvar is set, that command will be issued
 */
void CL_NextDemo(void)
{
	char v[MAX_STRING_CHARS];

	Q_strncpyz(v, Cvar_VariableString("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS - 1] = 0;
	Com_FuncDPrinf("CL_NextDemo: %s\n", v);
	if (!v[0])
	{
		return;
	}

	Cvar_Set("nextdemo", "");
	Cbuf_AddText(v);
	Cbuf_AddText("\n");
	Cbuf_Execute();
}

#if NEW_DEMOFUNC
/**
 * @brief CL_Rewind_f
 */
void CL_Rewind_f(void)
{
	double t;

	if (!clc.demoplaying)
	{
		Com_FuncPrinf("not playing demo can't rewind\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Com_FuncPrinf("usage:  rewind <time in seconds>\n");
		return;
	}

	if (!Q_isnumeric(Cmd_Argv(1)[0]))
	{
		t = (double)Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	}
	else
	{
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	if (cl.serverTime <= 0)
	{
		Com_FuncDrop("Servertime was unacceptable: %i\n", cl.serverTime);
	}

	DEMODEBUG("Servertime: %d snaptime: %d\n", cl.serverTime, cl.snap.serverTime);

	CL_RewindDemo((double)cl.serverTime + di.Overf - t);
}

/**
 * @brief CL_FastForward_f
 */
void CL_FastForward_f(void)
{
	double t;
	double wantedTime;

	if (!clc.demoplaying)
	{
		Com_FuncPrinf("not playing demo can't fast forward\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Com_FuncPrinf("usage:  fastforward <time in seconds>\n");
		return;
	}

	if (!Q_isnumeric(Cmd_Argv(1)[0]))
	{
		t = (double)Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	}
	else
	{
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	if (cl.snap.serverTime)
	{
		wantedTime = (double)cl.serverTime + di.Overf + t;
	}
	else
	{
		wantedTime = (double)di.firstServerTime + t;
	}

	CL_DemoFastForward(wantedTime);
}

/**
 * @brief CL_SeekServerTime_f
 */
void CL_SeekServerTime_f(void)
{
	double f;

	if (!clc.demoplaying)
	{
		Com_FuncPrinf("not playing demo can't seek\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Com_FuncPrinf("usage:  seekservertime <time in milliseconds>\n");
		return;
	}

	f = atof(Cmd_Argv(1));
	DEMODEBUG("%f\n", f);

	CL_DemoSeekMs(f, -1);
}

/**
 * @brief CL_Seek_f
 */
void CL_Seek_f(void)
{
	double t;

	if (!clc.demoplaying)
	{
		Com_FuncPrinf("not playing demo can't seek\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Com_FuncPrinf("usage:  seek <time in seconds>\n");
		return;
	}

	if (!Q_isnumeric(Cmd_Argv(1)[0]))
	{
		t = (double)Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	}
	else
	{
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	CL_DemoSeekMs((double)di.firstServerTime + t, -1);
}

/**
 * @brief CL_SeekEnd_f
 */
void CL_SeekEnd_f(void)
{
	double t;

	if (!clc.demoplaying)
	{
		Com_FuncPrinf("not playing demo can't seek\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Com_FuncPrinf("usage:  seek <time in seconds>\n");
		return;
	}

	if (!Q_isnumeric(Cmd_Argv(1)[0]))
	{
		t = (double)Cvar_VariableValue(Cmd_Argv(1)) * 1000.0;
	}
	else
	{
		t = atof(Cmd_Argv(1)) * 1000.0;
	}

	CL_DemoSeekMs((double)di.lastServerTime - t, -1);
}

/**
 * @brief CL_SeekNext_f
 */
void CL_SeekNext_f(void)
{
	snapshot_t snapshot;
	qboolean   r;
	int        i;

	if (cl.snap.serverTime == di.lastServerTime)
	{
		Com_FuncDPrinf("at last snap\n");
		return;
	}

	// takes into account offline demos with snaps with same server time
	if (cl.serverTime < cl.snap.serverTime)
	{
		CL_DemoSeekMs(0, cl.snap.serverTime);
		return;
	}

	i = 1;
	while (1)
	{
		r = CL_PeekSnapshot(cl.snap.messageNum + i, &snapshot);
		if (!r)
		{
			Com_FuncPrinf("couldn't get next snapshot\n");
			return;
		}

		if (snapshot.serverTime > cl.serverTime)
		{
			break;
		}
		i++;
	}

	CL_DemoSeekMs(0, snapshot.serverTime);
}

/**
 * @brief CL_SeekPrev_f
 */
void CL_SeekPrev_f(void)
{
	clSnapshot_t *clSnap;
	int          i = 0;

	if (cl.snap.serverTime == di.firstServerTime)
	{
		Com_FuncPrinf("at first snap\n");
		return;
	}

	// takes into account offline demos with snapshots having same server time
	while (qtrue)
	{
		clSnap = &cl.snapshots[(cl.snap.messageNum - i) & PACKET_MASK];
		if (clSnap->serverTime < cl.serverTime)
		{
			break;
		}
		i++;
	}

	CL_DemoSeekMs(0, clSnap->serverTime);
}
#endif

/**
 * @brief CL_PauseDemo_f
 */
void CL_PauseDemo_f(void)
{
#if NEW_DEMOFUNC
	static int pauseTime = 0;
#endif

	if (!clc.demoplaying)
	{
		return;
	}

	if (!IS_DEFAULT_MOD)
	{
		Com_FuncPrinf("Demo pausing is only supported on %s mod, sorry\n", DEFAULT_MODGAME);
		return;
	}

#if NEW_DEMOFUNC
	if (!cl_freezeDemo->integer)
	{
		pauseTime = cl.serverTime;
	}
	else
	{
		// TODO: this is just a hack, actually fix the cl_freezeDemo instead of this
		CL_DemoSeekMs(0, pauseTime);
	}
#endif

	Cvar_SetValue("cl_freezeDemo", !cl_freezeDemo->integer);
}

/**
 * @brief CL_DemoInit
 */
void CL_DemoInit(void)
{
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand("demo", CL_PlayDemo_f);
	Cmd_SetCommandCompletionFunc("demo", CL_CompleteDemoName);
	Cmd_AddCommand("pausedemo", CL_PauseDemo_f);

#if NEW_DEMOFUNC
	Cmd_AddCommand("rewind", CL_Rewind_f);
	Cmd_AddCommand("fastforward", CL_FastForward_f);
	Cmd_AddCommand("seekservertime", CL_SeekServerTime_f);
	Cmd_AddCommand("seek", CL_Seek_f);
	Cmd_AddCommand("seekend", CL_SeekEnd_f);
	Cmd_AddCommand("seeknext", CL_SeekNext_f);
	Cmd_AddCommand("seekprev", CL_SeekPrev_f);

	cl_maxRewindBackups = Cvar_Get("cl_maxRewindBackups", va("%i", MAX_REWIND_BACKUPS), CVAR_ARCHIVE | CVAR_LATCH);
#endif
}
