/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file sv_cl_demo.c
 */

#include "server.h"

/**
  * @brief Dumps the current net message, prefixed by the length
  * @param[in] msg
  * @param[in] headerBytes
  */
void SV_CL_WriteDemoMessage(msg_t *msg, int headerBytes)
{
	int len, swlen;

	// write the packet sequence
	len   = svclc.serverMessageSequence;
	swlen = LittleLong(len);
	(void)FS_Write(&swlen, 4, svclc.demo.file);

	// skip the packet sequencing information
	len   = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	(void)FS_Write(&swlen, 4, svclc.demo.file);
	(void)FS_Write(msg->data + headerBytes, len, svclc.demo.file);
}

/**
  * @brief SV_CL_DemoFilename
  * @param[in] number
  * @param[in] fileName
  */
void SV_CL_DemoFilename(int number, char *fileName)
{
	if (number < 0 || number > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "demo9999");
		return;
	}

	Com_sprintf(fileName, MAX_OSPATH, "demo%04i", number);
}

static char demoName[MAX_OSPATH];        // compiler bug workaround

void SV_CL_Record_f(void)
{
	char name[MAX_OSPATH];
	char *s;

	if (Cmd_Argc() > 2)
	{
		Com_Printf("record <demoname>\n");
		return;
	}

	if (svclc.demo.recording)
	{
		Com_Printf("Already recording.\n");
		return;
	}

	if (svcls.state != CA_ACTIVE)
	{
		Com_Printf("You must be in a level to record.\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		Q_strncpyz(demoName, s, sizeof(demoName));
		Com_sprintf(name, sizeof(name), "svdemos/%s.%s%d", demoName, SVCLDEMOEXT, ETTV_PROTOCOL_VERSION);
	}
	else
	{
		int number, len;

		// scan for a free demo name
		for (number = 0; number <= 9999; number++)
		{
			SV_CL_DemoFilename(number, demoName);
			Com_sprintf(name, sizeof(name), "svdemos/%s.%s%d", demoName, SVCLDEMOEXT, ETTV_PROTOCOL_VERSION);

			len = FS_ReadFile(name, NULL);
			if (len <= 0)
			{
				break;  // file doesn't exist
			}
		}
	}

	SV_CL_Record(name);
}

/**
 * @brief SV_CL_Record
 * @param[in] name
 */
void SV_CL_Record(const char *name)
{
	int            i;
	msg_t          buf;
	byte           bufData[MAX_MSGLEN];
	entityState_t  *ent;
	entityState_t  nullstate;
	entityShared_t *entShared;
	entityShared_t nullstateShared;
	sharedEntity_t *svent;
	char           *s;
	int            len;

	// open the demo file
	Com_Printf("Recording to %s.\n", name);
	svclc.demo.file = FS_FOpenFileWrite(name);
	if (!svclc.demo.file)
	{
		Com_Printf("ERROR: couldn't open.\n");
		return;
	}

	svclc.demo.recording = qtrue;
	Q_strncpyz(svclc.demo.demoName, demoName, sizeof(svclc.demo.demoName));

	// don't start saving messages until a non-delta compressed message is received
	svclc.demo.waiting = qtrue;

	// write out the gamestate message
	MSG_Init(&buf, bufData, sizeof(bufData));
	MSG_Bitstream(&buf);

	// NOTE: all server->client messages now acknowledge
	MSG_WriteLong(&buf, svclc.reliableSequence);

	MSG_WriteByte(&buf, svc_gamestate);
	MSG_WriteLong(&buf, svclc.serverCommandSequence);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (!svcl.gameState.stringOffsets[i])
		{
			continue;
		}
		s = svcl.gameState.stringData + svcl.gameState.stringOffsets[i];
		MSG_WriteByte(&buf, svc_configstring);
		MSG_WriteShort(&buf, i);
		MSG_WriteBigString(&buf, s);
	}

	// baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	Com_Memset(&nullstateShared, 0, sizeof(nullstateShared));
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		ent       = &svcl.entityBaselines[i];
		entShared = &svcl.entitySharedBaselines[i];
		if (!ent->number)
		{
			continue;
		}
		MSG_WriteByte(&buf, svc_baseline);
		MSG_WriteDeltaEntity(&buf, &nullstate, ent, qtrue);
		MSG_ETTV_WriteDeltaSharedEntity(&buf, &nullstateShared, entShared, qtrue);
	}

	// current states
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		svent = SV_GentityNum(i);

		if (svent->r.linked)
		{
			MSG_WriteByte(&buf, svc_ettv_currentstate);
			MSG_WriteDeltaEntity(&buf, &svcl.entityBaselines[i], &svent->s, qtrue);
			MSG_ETTV_WriteDeltaSharedEntity(&buf, &svcl.entitySharedBaselines[i], &svent->r, qtrue);
		}
	}

	MSG_WriteByte(&buf, svc_EOF);

	// finished writing the gamestate stuff

	// write the client num
	MSG_WriteLong(&buf, svclc.clientNum);
	// write the checksum feed
	MSG_WriteLong(&buf, svclc.checksumFeed);

	// finished writing the client packet
	MSG_WriteByte(&buf, svc_EOF);

	// write it to the demo file
	len = LittleLong(svclc.serverMessageSequence - 1);
	(void)FS_Write(&len, 4, svclc.demo.file);

	len = LittleLong(buf.cursize);
	(void)FS_Write(&len, 4, svclc.demo.file);
	(void)FS_Write(buf.data, buf.cursize, svclc.demo.file);

	// the rest of the demo file will be copied from net messages
}

void SV_CL_StopRecord_f(void)
{
	int len;

	if (!svclc.demo.recording)
	{
		Com_Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	(void)FS_Write(&len, 4, svclc.demo.file);
	(void)FS_Write(&len, 4, svclc.demo.file);
	FS_FCloseFile(svclc.demo.file);
	svclc.demo.file = 0;

	svclc.demo.recording = qfalse;
	Com_Printf("Stopped demo.\n");
}

/**
 * @brief Called when a demo or cinematic finishes
 * If the "nextdemo" cvar is set, that command will be issued
 */
void SV_CL_NextDemo(void)
{
	char v[MAX_STRING_CHARS];

	Q_strncpyz(v, Cvar_VariableString("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS - 1] = 0;
	Com_Printf("SV_CL_NextDemo: %s\n", v);
	if (!v[0])
	{
		return;
	}

	Cvar_Set("nextdemo", "");
	Cbuf_AddText(v);
	Cbuf_AddText("\n");
	Cbuf_Execute();
}

/**
 * @brief SV_CL_DemoCleanUp
 */
void SV_CL_DemoCleanUp(void)
{
	if (svclc.demo.file)
	{
		FS_FCloseFile(svclc.demo.file);
		svclc.demo.file = 0;
	}
}

/**
 * @brief SV_CL_DemoCompleted
 */
void SV_CL_DemoCompleted(void)
{
	//if (cl_timedemo && cl_timedemo->integer)
	//{
	//	int time;

	//	time = Sys_Milliseconds() - clc.demo.timeStart;
	//	if (time > 0)
	//	{
	//		Com_FuncPrinf("%i frames, %3.1f seconds: %3.1f fps\n", clc.demo.timeFrames,
	//			time / 1000.0, clc.demo.timeFrames * 1000.0 / time);
	//	}
	//}

	Com_Printf("etltv: demo completed\n");

	SV_CL_Disconnect();
	SV_CL_NextDemo();
}

/**
 * @brief SV_CL_ReadDemoMessage
 */
void SV_CL_ReadDemoMessage(void)
{
	int   r;
	msg_t buf;
	byte  bufData[MAX_MSGLEN];
	int   s;

	if (!svclc.demo.file)
	{
		SV_CL_DemoCompleted();
		return;
	}

	// get the sequence number
	r = FS_Read(&s, 4, svclc.demo.file);
	if (r != 4)
	{
		SV_CL_DemoCompleted();
		return;
	}

	svclc.serverMessageSequence = LittleLong(s);

	// init the message
	MSG_Init(&buf, bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, svclc.demo.file);

	if (r != 4)
	{
		SV_CL_DemoCompleted();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if (buf.cursize == -1)
	{
		SV_CL_DemoCompleted();
		return;
	}

	if (buf.cursize > buf.maxsize)
	{
		Com_Error(ERR_DROP, "demoMsglen > MAX_MSGLEN");
	}

	r = FS_Read(buf.data, buf.cursize, svclc.demo.file);
	if (r != buf.cursize)
	{
		Com_Printf("Demo file was truncated.\n");
		SV_CL_DemoCompleted();
		return;
	}

	svclc.lastPacketTime = svcls.realtime;
	buf.readcount        = 0;
	SV_CL_ParseServerMessage(&buf);
}

/**
 * @brief SV_CL_PlayDemo_f
 */
void SV_CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH];
	char *demoFile;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("demo <demoname>\n");
		return;
	}

	SV_CL_Disconnect();

	// open the demo file (should be the last arg)
	demoFile = Cmd_Argv(Cmd_Argc() - 1);
	Com_sprintf(name, MAX_OSPATH, "svdemos/%s.%s%d", demoFile, SVCLDEMOEXT, PROTOCOL_VERSION);
	FS_FOpenFileRead(name, &svclc.demo.file, qtrue);

	if (!svclc.demo.file)
	{
		Com_Printf("Couldn't open %s", name);
		return;
	}

	Q_strncpyz(svclc.demo.demoName, demoFile, sizeof(svclc.demo.demoName));

	if (Cmd_Argc() == 3)
	{
		char *param = Cmd_Argv(1);

		if (!Q_stricmp(param, "pure"))
		{
			svclc.demo.pure = qtrue;
		}
		else if (!Q_stricmp(param, "dirty"))
		{
			svclc.demo.pure = qfalse;
		}
	}
	else
	{
		svclc.demo.pure = Cvar_VariableIntegerValue("sv_pure") != 0;
	}

	svcls.state        = CA_CONNECTED;
	svclc.demo.playing = qtrue;

	Q_strncpyz(svcls.servername, demoFile, sizeof(svcls.servername));

	// read demo messages until connected
	while (svcls.state >= CA_CONNECTED && svcls.state < CA_PRIMED)
	{
		SV_CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	svclc.demo.firstFrameSkipped = qfalse;
}

/**
 * @brief SV_CL_CompleteDemoName
 * @param args - unused
 * @param[in] argNum
 */
static void SV_CL_CompleteDemoName(char *args, int argNum)
{
	if (argNum == 2)
	{
		char demoExt[16];

		Com_sprintf(demoExt, sizeof(demoExt), ".%s%d", SVCLDEMOEXT, PROTOCOL_VERSION);
		Field_CompleteFilename("svdemos", demoExt, qtrue, qtrue);
	}
}

/**
  * @brief SV_CL_DemoInit
  */
void SV_CL_DemoInit(void)
{
	Cmd_AddCommand("record", SV_CL_Record_f);
	Cmd_AddCommand("stoprecord", SV_CL_StopRecord_f);
	Cmd_AddCommand("demo", SV_CL_PlayDemo_f);
	Cmd_SetCommandCompletionFunc("demo", SV_CL_CompleteDemoName);
}
