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
 * @file cl_main.c
 * @brief client main loop
 */

#include "client.h"
#include "snd_local.h"

#include <limits.h>

#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"

#ifdef USE_RENDERER_DLOPEN
cvar_t *cl_renderer;
#endif

cvar_t *cl_wavefilerecord;
cvar_t *cl_nodelta;
cvar_t *cl_debugMove;

cvar_t *cl_noprint;

cvar_t *rcon_client_password;
cvar_t *rconAddress;

cvar_t *cl_timeout;
cvar_t *cl_maxpackets;
cvar_t *cl_packetdup;
cvar_t *cl_timeNudge;
cvar_t *cl_showTimeDelta;
cvar_t *cl_freezeDemo;

cvar_t *cl_shownet = NULL;      // This is referenced in msg.c and we need to make sure it is NULL
cvar_t *cl_shownuments;
cvar_t *cl_showSend;
cvar_t *cl_showServerCommands;
cvar_t *cl_timedemo;
cvar_t *cl_avidemo;
cvar_t *cl_forceavidemo;
cvar_t *cl_avidemotype;
cvar_t *cl_aviMotionJpeg;

cvar_t *cl_freelook;
cvar_t *cl_sensitivity;

cvar_t *cl_mouseAccel;
cvar_t *cl_showMouseRate;

cvar_t *m_pitch;
cvar_t *m_yaw;
cvar_t *m_forward;
cvar_t *m_side;
cvar_t *m_filter;

cvar_t *cl_activeAction;

cvar_t *cl_autorecord;

cvar_t *cl_allowDownload;
cvar_t *cl_wwwDownload;
cvar_t *cl_conXOffset;
cvar_t *cl_inGameVideo;

cvar_t *cl_serverStatusResendTime;
cvar_t *cl_missionStats;

cvar_t *cl_profile;
cvar_t *cl_defaultProfile;

cvar_t *cl_demorecording;
cvar_t *cl_demofilename;
cvar_t *cl_demooffset;

cvar_t *cl_waverecording;
cvar_t *cl_wavefilename;
cvar_t *cl_waveoffset;

cvar_t *cl_packetloss;
cvar_t *cl_packetdelay;

cvar_t *cl_consoleKeys;

clientActive_t     cl;
clientConnection_t clc;
clientStatic_t     cls;
vm_t               *cgvm;
autoupdate_t       autoupdate;

netadr_t rcon_address;

// Structure containing functions exported from refresh DLL
refexport_t re;
#ifdef USE_RENDERER_DLOPEN
static void *rendererLib = NULL;
#endif

ping_t cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s
{
	char string[BIG_INFO_STRING];
	netadr_t address;
	int time, startTime;
	qboolean pending;
	qboolean print;
	qboolean retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int            serverStatusCount;

void CL_CheckForResend(void);
void CL_ShowIP_f(void);
void CL_ServerStatus_f(void);
void CL_ServerStatusResponse(netadr_t from, msg_t *msg);

void CL_WriteWaveClose(void);
void CL_WavStopRecord_f(void);

void CL_PurgeCache(void)
{
	cls.doCachePurge = qtrue;
}

void CL_DoPurgeCache(void)
{
	if (!cls.doCachePurge)
	{
		return;
	}

	cls.doCachePurge = qfalse;

	if (!com_cl_running)
	{
		return;
	}

	if (!com_cl_running->integer)
	{
		return;
	}

	if (!cls.rendererStarted)
	{
		return;
	}

	re.purgeCache();
}

/*
=======================================================================
CLIENT RELIABLE COMMAND COMMUNICATION
=======================================================================
*/

/**
 * @brief The given command will be transmitted to the server, and is guaranteed to
 * not have future usercmd_t executed before it is executed.
 */
void CL_AddReliableCommand(const char *cmd)
{
	int index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if (clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS)
	{
		Com_Error(ERR_DROP, "Client command overflow");
	}
	clc.reliableSequence++;
	index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
	Q_strncpyz(clc.reliableCommands[index], cmd, sizeof(clc.reliableCommands[index]));
}

/*
=======================================================================
CLIENT SIDE DEMO RECORDING
=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(msg_t *msg, int headerBytes)
{
	int len, swlen;

	// write the packet sequence
	len   = clc.serverMessageSequence;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len   = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);
	FS_Write(msg->data + headerBytes, len, clc.demofile);
}

/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f(void)
{
	int len;

	if (!clc.demorecording)
	{
		Com_Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	FS_Write(&len, 4, clc.demofile);
	FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;

	clc.demorecording = qfalse;
	Cvar_Set("cl_demorecording", "0");
	Cvar_Set("cl_demofilename", "");
	Cvar_Set("cl_demooffset", "0");
	Com_Printf("Stopped demo.\n");
}

/*
==================
CL_DemoFilename
==================
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

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/

static char demoName[MAX_QPATH];        // compiler bug workaround
void CL_Record_f(void)
{
	char name[MAX_OSPATH];
	char *s;

	if (Cmd_Argc() > 2)
	{
		Com_Printf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		Com_Printf("Already recording.\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		Com_Printf("You must be in a level to record.\n");
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
		for (number = 0 ; number <= 9999 ; number++)
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
	Com_Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Com_Printf("ERROR: couldn't open.\n");
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
	for (i = 0 ; i < MAX_CONFIGSTRINGS ; i++)
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
	memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_GENTITIES ; i++)
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
	FS_Write(&len, 4, clc.demofile);

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf.data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

/*
=======================================================================
CLIENT SIDE DEMO PLAYBACK
=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted(void)
{
	if (cl_timedemo && cl_timedemo->integer)
	{
		int time;

		time = Sys_Milliseconds() - clc.timeDemoStart;
		if (time > 0)
		{
			Com_Printf("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
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

/*
=================
CL_ReadDemoMessage
=================
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
		Com_Error(ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
	}
	r = FS_Read(buf.data, buf.cursize, clc.demofile);
	if (r != buf.cursize)
	{
		Com_Printf("Demo file was truncated.\n");
		CL_DemoCompleted();
		return;
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount      = 0;
	CL_ParseServerMessage(&buf);
}

/*
====================
  Wave file saving functions

  FIXME: make this actually work
====================
*/

/*
==================
CL_DemoFilename
==================
*/
void CL_WavFilename(int number, char *fileName)
{
	if (number < 0 || number > 9999)
	{
		Com_sprintf(fileName, MAX_OSPATH, "wav9999");
		return;
	}

	Com_sprintf(fileName, MAX_OSPATH, "wav%04i", number);
}

typedef struct wav_hdr_s
{
	unsigned int ChunkID;       // big endian
	unsigned int ChunkSize;     // little endian
	unsigned int Format;        // big endian

	unsigned int Subchunk1ID;   // big endian
	unsigned int Subchunk1Size; // little endian
	unsigned short AudioFormat; // little endian
	unsigned short NumChannels; // little endian
	unsigned int SampleRate;    // little endian
	unsigned int ByteRate;      // little endian
	unsigned short BlockAlign;  // little endian
	unsigned short BitsPerSample;   // little endian

	unsigned int Subchunk2ID;   // big endian
	unsigned int Subchunk2Size; // little indian ;)

	unsigned int NumSamples;
} wav_hdr_t;

wav_hdr_t hdr;

static void CL_WriteWaveHeader(void)
{
	memset(&hdr, 0, sizeof(hdr));

	hdr.ChunkID   = 0x46464952;     // "RIFF"
	hdr.ChunkSize = 0;              // total filesize - 8 bytes
	hdr.Format    = 0x45564157;     // "WAVE"

	hdr.Subchunk1ID   = 0x20746d66; // "fmt "
	hdr.Subchunk1Size = 16;         // 16 = pcm
	hdr.AudioFormat   = 1;          // 1 = linear quantization
	hdr.NumChannels   = 2;          // 2 = stereo

	hdr.SampleRate = dma.speed;

	hdr.BitsPerSample = 16;         // 16bits

	// SampleRate * NumChannels * BitsPerSample/8
	hdr.ByteRate = hdr.SampleRate * hdr.NumChannels * (hdr.BitsPerSample / 8);

	// NumChannels * BitsPerSample/8
	hdr.BlockAlign = hdr.NumChannels * (hdr.BitsPerSample / 8);

	hdr.Subchunk2ID = 0x61746164;       // "data"

	hdr.Subchunk2Size = 0;          // NumSamples * NumChannels * BitsPerSample/8

	// ...
	FS_Write(&hdr.ChunkID, 44, clc.wavefile);
}

static char wavName[MAX_QPATH];     // compiler bug workaround
void CL_WriteWaveOpen(void)
{
	// we will just save it as a 16bit stereo 22050kz pcm file

	char name[MAX_OSPATH];
	char *s;

	if (Cmd_Argc() > 2)
	{
		Com_Printf("wav_record <wavname>\n");
		return;
	}

	if (clc.waverecording)
	{
		Com_Printf("Already recording a wav file\n");
		return;
	}

	// yes ... no ? leave it up to them imo
	//if (cl_avidemo.integer)
	//  return;

	if (Cmd_Argc() == 2)
	{
		s = Cmd_Argv(1);
		Q_strncpyz(wavName, s, sizeof(wavName));
		Com_sprintf(name, sizeof(name), "wav/%s.wav", wavName);
	}
	else
	{
		int number, len;

		for (number = 0 ; number <= 9999 ; number++)
		{
			CL_WavFilename(number, wavName);
			Com_sprintf(name, sizeof(name), "wav/%s.wav", wavName);

			len = FS_FileExists(name);
			if (len <= 0)
			{
				break;  // file doesn't exist
			}
		}
	}

	Com_Printf("recording to %s.\n", name);
	clc.wavefile = FS_FOpenFileWrite(name);

	if (!clc.wavefile)
	{
		Com_Printf("ERROR: couldn't open %s for writing.\n", name);
		return;
	}

	CL_WriteWaveHeader();
	clc.wavetime = -1;

	clc.waverecording = qtrue;

	Cvar_Set("cl_waverecording", "1");
	Cvar_Set("cl_wavefilename", wavName);
	Cvar_Set("cl_waveoffset", "0");
}

void CL_WriteWaveClose()
{
	Com_Printf("Stopped recording\n");

	hdr.Subchunk2Size = hdr.NumSamples * hdr.NumChannels * (hdr.BitsPerSample / 8);
	hdr.ChunkSize     = 36 + hdr.Subchunk2Size;

	FS_Seek(clc.wavefile, 4, FS_SEEK_SET);
	FS_Write(&hdr.ChunkSize, 4, clc.wavefile);
	FS_Seek(clc.wavefile, 40, FS_SEEK_SET);
	FS_Write(&hdr.Subchunk2Size, 4, clc.wavefile);

	// and we're outta here
	FS_FCloseFile(clc.wavefile);
	clc.wavefile = 0;
}

/*
====================
CL_WalkDemoExt
====================
*/
static int CL_WalkDemoExt(char *arg, char *name, int *demofile)
{
	int i = 0;
	*demofile = 0;

	Com_sprintf(name, MAX_OSPATH, "demos/%s.%s%d", arg, DEMOEXT, PROTOCOL_VERSION);
	FS_FOpenFileRead(name, demofile, qtrue);

	if (*demofile)
	{
		Com_Printf("Demo file: %s\n", name);
		return PROTOCOL_VERSION;
	}

	Com_Printf("Not found: %s\n", name);

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
			Com_Printf("Demo file: %s\n", name);

			return demo_protocols[i];
		}
		else
		{
			Com_Printf("Not found: %s\n", name);
		}
		i++;
	}

	return -1;
}

/*
====================
CL_CompleteDemoName
====================
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
 * @brief Usage: demo <demoname>
 */
void CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH], retry[MAX_OSPATH];
	char *arg, *ext_test;
	int  protocol, i;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");

	CL_Disconnect(qtrue);

	// open the demo file
	arg = Cmd_Argv(1);
	// check for an extension .DEMOEXT_?? (?? is protocol)
	ext_test = strrchr(arg, '.');

	if (ext_test && !Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1))
	{
		protocol = atoi(ext_test + ARRAY_LEN(DEMOEXT));

		for (i = 0; demo_protocols[i]; i++)
		{
			if (demo_protocols[i] == protocol)
			{
				break;
			}
		}

		if (demo_protocols[i] || protocol == PROTOCOL_VERSION)
		{
			Com_sprintf(name, sizeof(name), "demos/%s", arg);
			FS_FOpenFileRead(name, &clc.demofile, qtrue);
		}
		else
		{
			int len;

			Com_Printf("Protocol %d not supported for demos\n", protocol);
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
		Com_Error(ERR_DROP, "couldn't open %s", name);
		return;
	}
	Q_strncpyz(clc.demoName, arg, sizeof(clc.demoName));

	Con_Close();

	cls.state       = CA_CONNECTED;
	clc.demoplaying = qtrue;

	if (Cvar_VariableValue("cl_wavefilerecord"))
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
	//if (clc.waverecording) {
	//  CL_WriteWaveClose();
	//  clc.waverecording = qfalse;
	//}
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo(void)
{
	char v[MAX_STRING_CHARS];

	Q_strncpyz(v, Cvar_VariableString("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS - 1] = 0;
	Com_DPrintf("CL_NextDemo: %s\n", v);
	if (!v[0])
	{
		return;
	}

	Cvar_Set("nextdemo", "");
	Cbuf_AddText(v);
	Cbuf_AddText("\n");
	Cbuf_Execute();
}

//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void)
{
	// clear sounds
	S_DisableSounds();
	// download subsystem
	DL_Shutdown();
	// shutdown CGame
	CL_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI();

	// shutdown the renderer
	if (re.Shutdown)
	{
		re.Shutdown(qfalse);        // don't destroy window or context
	}

	if (re.purgeCache)
	{
		CL_DoPurgeCache();
	}

	cls.uiStarted       = qfalse;
	cls.cgameStarted    = qfalse;
	cls.rendererStarted = qfalse;
	cls.soundRegistered = qfalse;

	// stop recording on map change etc, demos aren't valid over map changes anyway
	if (clc.demorecording)
	{
		CL_StopRecord_f();
	}

	if (clc.waverecording)
	{
		CL_WavStopRecord_f();
	}
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory(void)
{
	// shutdown all the client stuff
	CL_ShutdownAll();

	// if not running a server clear the whole hunk
	if (!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
		// clear collision map data
		CM_ClearMap();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading(void)
{
	if (!com_cl_running->integer)
	{
		return;
	}

	Con_Close();
	cls.keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if (cls.state >= CA_CONNECTED && !Q_stricmp(cls.servername, "localhost"))
	{
		cls.state = CA_CONNECTED;       // so the connect screen is drawn
		memset(cls.updateInfoString, 0, sizeof(cls.updateInfoString));
		memset(clc.serverMessage, 0, sizeof(clc.serverMessage));
		memset(&cl.gameState, 0, sizeof(cl.gameState));
		clc.lastPacketSentTime = -9999; // send first packet immediately
		SCR_UpdateScreen();
	}
	else
	{
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set("nextmap", "");
		CL_Disconnect(qtrue);
		Q_strncpyz(cls.servername, "localhost", sizeof(cls.servername));
		cls.state       = CA_CHALLENGING; // so the connect screen is drawn
		cls.keyCatchers = 0;
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr(cls.servername, &clc.serverAddress, NA_UNSPEC);
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

/**
 * @brief Update cl_guid using #ETKEY_FILE
 */
static void CL_UpdateGUID(void)
{
	fileHandle_t f;
	int          len;

	len = FS_SV_FOpenFileRead(BASEGAME "/" ETKEY_FILE, &f);
	FS_FCloseFile(f);

	if (len < ETKEY_SIZE)
	{
		Com_Printf(S_COLOR_RED "ERROR: Could not set etkey (size mismatch).\n");
		Cvar_Set("cl_guid", "");
	}
	else
	{
		char *guid = Com_MD5FileETCompat(ETKEY_FILE);

		if (guid)
		{
			Cvar_Set("cl_guid", guid);
		}
	}
}

/**
 * @brief Test for the existence of a valid #ETKEY_FILE and generate a new one
 * if it doesn't exist
 */
static void CL_GenerateETKey(void)
{
	int          len = 0;
	char         buff[ETKEY_SIZE];
	fileHandle_t f;

	len = FS_SV_FOpenFileRead(BASEGAME "/" ETKEY_FILE, &f);
	FS_FCloseFile(f);
	if (len > 0)
	{
		Com_Printf("ETKEY found.\n");
		return;
	}
	else
	{
		time_t    tt;
		struct tm *t;
		int       last;

		tt = time(NULL);
		t  = localtime(&tt);
		srand(Com_Milliseconds());
		last = rand() % 9999;

		Com_sprintf(buff, sizeof(buff), "0000001002%04i%02i%02i%02i%02i%02i%04i", t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, last);

		f = FS_SV_FOpenFileWrite(BASEGAME "/" ETKEY_FILE);
		if (!f)
		{
			Com_Printf(S_COLOR_RED "ERROR: Could not open %s for write\n", ETKEY_FILE);
			return;
		}
		FS_Write(buff, sizeof(buff), f);
		FS_FCloseFile(f);
		Com_Printf(S_COLOR_CYAN "ETKEY file generated.\n");
	}
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState(void)
{
	Com_Memset(&cl, 0, sizeof(cl));
}

/*
=====================
CL_ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void CL_ClearStaticDownload(void)
{
	assert(!cls.bWWWDlDisconnected);    // reset before calling
	cls.downloadRestart         = qfalse;
	cls.downloadTempName[0]     = '\0';
	cls.downloadName[0]         = '\0';
	cls.originalDownloadName[0] = '\0';
}

/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(qboolean showMainMenu)
{
	if (!com_cl_running || !com_cl_running->integer)
	{
		return;
	}

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if (clc.demorecording)
	{
		CL_StopRecord_f();
	}

	if (!cls.bWWWDlDisconnected)
	{
		if (clc.download)
		{
			FS_FCloseFile(clc.download);
			clc.download = 0;
		}
		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set("cl_downloadName", "");

		autoupdate.updateStarted = qfalse;
		// TODO: clean autoupdate cvars
	}

	if (clc.demofile)
	{
		FS_FCloseFile(clc.demofile);
		clc.demofile = 0;
	}

	if (uivm && showMainMenu)
	{
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE);
	}

	SCR_StopCinematic();
	S_ClearSoundBuffer(qtrue);    // modified

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if (cls.state >= CA_CONNECTED)
	{
		CL_AddReliableCommand("disconnect");
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}

	CL_ClearState();

	// wipe the client connection
	Com_Memset(&clc, 0, sizeof(clc));

	if (!cls.bWWWDlDisconnected)
	{
		CL_ClearStaticDownload();
	}

	// allow cheats locally
	Cvar_Set("sv_cheats", "1");

	// not connected to a pure server anymore
	cl_connectedToPureServer = qfalse;

	// don't try a restart if uivm is NULL, as we might be in the middle of a restart already
	if (uivm && cls.state > CA_DISCONNECTED)
	{
		// restart the UI
		cls.state = CA_DISCONNECTED;

		// shutdown the UI
		CL_ShutdownUI();

		// init the UI
		CL_InitUI();
	}
	else
	{
		cls.state = CA_DISCONNECTED;
	}
}

/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer(const char *string)
{
	char *cmd;

	cmd = Cmd_Argv(0);

	// ignore key up commands
	if (cmd[0] == '-')
	{
		return;
	}

	if (clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+')
	{
		Com_Printf("Unknown command \"%s\"\n", rc(cmd));
		return;
	}

	if (Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(string);
	}
	else
	{
		CL_AddReliableCommand(cmd);
	}
}

/**
 * @brief Send motd request to the #MOTD_SERVER_NAME
 */
void CL_RequestMotd(void)
{
	char info[MAX_INFO_STRING];

	if (!com_motd->integer)
	{
		return;
	}

	Com_Printf("MOTD: resolving %s... ", MOTD_SERVER_NAME);

	if (!NET_StringToAdr(va("%s:%i", MOTD_SERVER_NAME, PORT_MOTD), &autoupdate.motdServer, NA_UNSPEC))
	{
		Com_Printf("couldn't resolve address\n");
		return;
	}
	else
	{
		Com_Printf("resolved to %s\n", NET_AdrToString(autoupdate.motdServer));
	}

	Com_sprintf(autoupdate.motdChallenge, sizeof(autoupdate.motdChallenge), "%i", rand());

	info[0] = 0;
	Info_SetValueForKey(info, "challenge", autoupdate.motdChallenge);
	Info_SetValueForKey(info, "version", ETLEGACY_VERSION_SHORT);
	Info_SetValueForKey(info, "platform", CPUSTRING);

	NET_OutOfBandPrint(NS_CLIENT, autoupdate.motdServer, "getmotd \"%s\"", info);
}

/*
======================================================================
CONSOLE COMMANDS
======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f(void)
{
	if (cls.state != CA_ACTIVE || clc.demoplaying)
	{
		Com_Printf("Not connected to a server.\n");
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(Cmd_Args());
	}
}

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f(void)
{
	int argc = Cmd_Argc();

	if (argc > 2)
	{
		char buffer[1024];
		int  i;

		strcpy(buffer, Cmd_Argv(1));
		strcat(buffer, "=");

		for (i = 2; i < argc; i++)
		{
			strcat(buffer, Cmd_Argv(i));
			strcat(buffer, " ");
		}

		Q_putenv(buffer);
	}
	else if (argc == 2)
	{
		char *env = getenv(Cmd_Argv(1));

		if (env)
		{
			Com_Printf("%s=%s\n", Cmd_Argv(1), env);
		}
		else
		{
			Com_Printf("%s undefined\n", Cmd_Argv(1));
		}
	}
}

/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f(void)
{
	SCR_StopCinematic();

	if (cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC)
	{
		Com_Error(ERR_DISCONNECT, "Disconnected from server");
	}
}

/*
================
CL_Reconnect_f
================
*/
void CL_Reconnect_f(void)
{
	if (!strlen(cls.servername) || !strcmp(cls.servername, "localhost"))
	{
		Com_Printf("Can't reconnect to localhost.\n");
		return;
	}
	Cbuf_AddText(va("connect %s\n", cls.servername));
}

/*
================
CL_Connect_f
================
*/
void CL_Connect_f(void)
{
	char         *server;
	const char   *ip_port;
	int          argc   = Cmd_Argc();
	netadrtype_t family = NA_UNSPEC;

	if (argc != 2 && argc != 3)
	{
		Com_Printf("usage: connect [-4|-6] server\n");
		return;
	}

	if (argc == 2)
	{
		server = Cmd_Argv(1);
	}
	else
	{
		if (!strcmp(Cmd_Argv(1), "-4"))
		{
			family = NA_IP;
		}
		else if (!strcmp(Cmd_Argv(1), "-6"))
		{
			family = NA_IP6;
		}
		else
		{
			Com_Printf("warning: only -4 or -6 as address type understood.\n");
		}

		server = Cmd_Argv(2);
	}

	// Game started as a custom protocol handler for et://
	if (!Q_stricmpn(server, "et://", 5))
	{
		Q_strncpyz(server, server + 5, strlen(server));
	}

	S_StopAllSounds();

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");
	Cvar_Set("ui_connecting", "1");

	// fire a message off to the motd server and check for update
	CL_RequestMotd();
	CL_CheckAutoUpdate();

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	if (com_sv_running->integer && !strcmp(server, "localhost"))
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	CL_Disconnect(qtrue);
	Con_Close();

	Q_strncpyz(cls.servername, server, sizeof(cls.servername));

	if (!NET_StringToAdr(cls.servername, &clc.serverAddress, family))
	{
		Com_Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		Cvar_Set("ui_connecting", "0");
		return;
	}
	if (clc.serverAddress.port == 0)
	{
		clc.serverAddress.port = BigShort(PORT_SERVER);
	}

	ip_port = NET_AdrToString(clc.serverAddress);

	Com_Printf("%s resolved to %s\n", cls.servername, ip_port);

	// if we aren't playing on a lan, we need to request a challenge
	if (NET_IsLocalAddress(clc.serverAddress))
	{
		cls.state = CA_CHALLENGING;
	}
	else
	{
		cls.state = CA_CONNECTING;
	}

	Cvar_Set("cl_avidemo", "0");

	// prepare to catch a connection process that would turn bad
	Cvar_Set("com_errorDiagnoseIP", NET_AdrToString(clc.serverAddress));

	// we need to setup a correct default for this, otherwise the first val we set might reappear
	Cvar_Set("com_errorMessage", "");

	cls.keyCatchers        = 0;
	clc.connectTime        = -99999; // CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", server);
	Cvar_Set("cl_currentServerIP", ip_port);
}

#define MAX_RCON_MESSAGE 1024

/*
==================
CL_CompleteRcon
==================
*/
static void CL_CompleteRcon(char *args, int argNum)
{
	if (argNum == 2)
	{
		// Skip "rcon "
		char *p = Com_SkipTokens(args, 1, " ");

		if (p > args)
		{
			Field_CompleteCommand(p, qtrue, qtrue);
		}
	}
}

/**
 * @brief Send the rest of the command line over as an unconnected command.
 */
void CL_Rcon_f(void)
{
	char message[MAX_RCON_MESSAGE];

	if (!rcon_client_password->string)
	{
		Com_Printf("You must set 'rconpassword' before\n"
		           "issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	Q_strcat(message, MAX_RCON_MESSAGE, "rcon ");

	Q_strcat(message, MAX_RCON_MESSAGE, rcon_client_password->string);
	Q_strcat(message, MAX_RCON_MESSAGE, " ");

	Q_strcat(message, MAX_RCON_MESSAGE, Cmd_Cmd() + 5);

	if (cls.state >= CA_CONNECTED)
	{
		rcon_address = clc.netchan.remoteAddress;
	}
	else
	{
		if (!strlen(rconAddress->string))
		{
			Com_Printf("You must either be connected,\n"
			           "or set the 'rconAddress' cvar\n"
			           "to issue rcon commands\n");
			return;
		}
		NET_StringToAdr(rconAddress->string, &rcon_address, NA_UNSPEC);
		if (rcon_address.port == 0)
		{
			rcon_address.port = BigShort(PORT_SERVER);
		}
	}

	NET_SendPacket(NS_CLIENT, strlen(message) + 1, message, rcon_address);
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums(void)
{
	const char *pChecksums;
	char       cMsg[MAX_INFO_VALUE];

	// if we are pure we need to send back a command with our referenced pk3 checksums
	pChecksums = FS_ReferencedPakPureChecksums();

	Com_sprintf(cMsg, sizeof(cMsg), "cp %d %s", cl.serverId, pChecksums);

	CL_AddReliableCommand(cMsg);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer(void)
{
	CL_AddReliableCommand(va("vdr"));
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/

#ifdef _WIN32
extern void Sys_In_Restart_f(void);
#endif

void CL_Vid_Restart_f(void)
{
	// don't show percent bar, since the memory usage will just sit at the same level anyway
	// - so keep the value - feels like a bug for users
	//com_expectedhunkusage = -1;

	// Settings may have changed so stop recording now
	if (CL_VideoRecording())
	{
		//Stop recording and close the avi file before vid_restart
		Cmd_ExecuteString("stopvideo");
	}

	// Do we want to stop the recording of demos on vid_restart?
	//if(clc.demorecording)
	//  CL_StopRecord_f();

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI();
	// shutdown the CGame
	CL_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart(clc.checksumFeed);

	S_BeginRegistration();  // all sound handles are now invalid

	cls.rendererStarted      = qfalse;
	cls.uiStarted            = qfalse;
	cls.cgameStarted         = qfalse;
	cls.soundRegistered      = qfalse;
	autoupdate.updateChecked = qfalse;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");

	// if not running a server clear the whole hunk
	if (!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CL_StartHunkUsers();

#ifdef _WIN32
	Sys_In_Restart_f();
#endif
	// start the cgame if connected
	if (cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.cgameStarted = qtrue;
		CL_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
}

/*
=================
CL_UI_Restart_f

Restart the ui subsystem
=================
*/
void CL_UI_Restart_f(void) // shutdown the UI
{
	CL_ShutdownUI();

	autoupdate.updateChecked = qfalse;

	// init the UI
	CL_InitUI();
}

/*
=================
CL_Snd_Reload_f

Reloads sounddata from disk, retains soundhandles.
=================
*/
void CL_Snd_Reload_f(void)
{
	S_Reload();
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f(void)
{
	S_Shutdown();
	S_Init();

	CL_Vid_Restart_f();
}

/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f(void)
{
	Com_Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f(void)
{
	Com_Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f

Note: It may occure some CONFIGSTRINGS are not printed (see print buffer)
==================
*/
void CL_Configstrings_f(void)
{
	int i;
	int ofs;

	if (cls.state != CA_ACTIVE)
	{
		Com_Printf("Not connected to a server.\n");
		return;
	}

	for (i = 0 ; i < MAX_CONFIGSTRINGS ; i++)
	{
		ofs = cl.gameState.stringOffsets[i];
		if (!ofs)
		{
			continue;
		}
		Com_Printf("%4i: %s\n", i, cl.gameState.stringData + ofs);
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f(void)
{
	Com_Printf("--------- Client Information ---------\n");
	Com_Printf("state: %i\n", cls.state);
	Com_Printf("Server: %s\n", cls.servername);
	Com_Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO));
	Com_Printf("--------------------------------------\n");
}

/**
 * @brief Eat misc console commands to prevent exploits
 */
void CL_EatMe_f(void)
{
	// do nothing kthxbye
}

/*
==============
CL_WavRecord_f
==============
*/
void CL_WavRecord_f(void)
{
	if (clc.wavefile)
	{
		Com_Printf("Already recording a wav file\n");
		return;
	}

	CL_WriteWaveOpen();
}

/*
==============
CL_WavStopRecord_f
==============
*/
void CL_WavStopRecord_f(void)
{
	if (!clc.wavefile)
	{
		Com_Printf("Not recording a wav file\n");
		return;
	}

	CL_WriteWaveClose();
	Cvar_Set("cl_waverecording", "0");
	Cvar_Set("cl_wavefilename", "");
	Cvar_Set("cl_waveoffset", "0");
	clc.waverecording = qfalse;
}

//====================================================================

/**
 * @brief Called when all downloading has been completed
 * Initiates update process after an update has been downloaded.
 */
void CL_DownloadsComplete(void)
{
#ifdef FEATURE_AUTOUPDATE
	// Auto-update
	if (autoupdate.updateStarted)
	{
		if (strlen(com_updatefiles->string) > 4)
		{
			CL_InitDownloads();
			return;
		}

		/*
		char *fn;
		fn = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, com_updatefiles->string);
		#ifndef _WIN32
		Sys_Chmod(fn, S_IXUSR);
		#endif
		// will either exit with a successful process spawn, or will Com_Error ERR_DROP
		// so we need to clear the disconnected download data if needed
		if (cls.bWWWDlDisconnected)
		{
		    cls.bWWWDlDisconnected = qfalse;
		    CL_ClearStaticDownload();
		}

		Sys_StartProcess(fn, qtrue);

		// reinitialize the filesystem if the game directory or checksum has changed
		// - after Legacy mod update
		FS_ConditionalRestart(clc.checksumFeed);
		*/

		autoupdate.updateStarted = qfalse;

		CL_Disconnect(qtrue);

		// we can reset that now
		cls.bWWWDlDisconnected = qfalse;
		CL_ClearStaticDownload();

		return;
	}
#endif // FEATURE_AUTOUPDATE

	// if we downloaded files we need to restart the file system
	if (cls.downloadRestart)
	{
		cls.downloadRestart = qfalse;

		FS_Restart(clc.checksumFeed);    // We possibly downloaded a pak, restart the file system to load it

		if (!cls.bWWWDlDisconnected)
		{
			// inform the server so we get new gamestate info
			CL_AddReliableCommand("donedl");
		}
		// we can reset that now
		cls.bWWWDlDisconnected = qfalse;
		CL_ClearStaticDownload();

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// I wonder if that happens - it should not but I suspect it could happen if a download fails in the middle or is aborted
	assert(!cls.bWWWDlDisconnected);

	// let the client game init and load data
	cls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if (cls.state != CA_LOADING)
	{
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CL_FlushMemory();

	// initialize the CGame
	cls.cgameStarted = qtrue;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void CL_BeginDownload(const char *localName, const char *remoteName)
{

	Com_DPrintf("***** CL_BeginDownload *****\n"
	            "Localname: %s\n"
	            "Remotename: %s\n"
	            "****************************\n", localName, remoteName);

	Q_strncpyz(cls.downloadName, localName, sizeof(cls.downloadName));
	Com_sprintf(cls.downloadTempName, sizeof(cls.downloadTempName), "%s.tmp", localName);

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0; // Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand(va("download %s", remoteName));
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void)
{
	char *s;
	char *remoteName, *localName;

	// We are looking to start a download here
	if (*clc.downloadList)
	{
		s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
		{
			s++;
		}
		remoteName = s;

		if ((s = strchr(s, '@')) == NULL)
		{
			CL_DownloadsComplete();
			return;
		}

		*s++      = 0;
		localName = s;
		if ((s = strchr(s, '@')) != NULL)
		{
			*s++ = 0;
		}
		else
		{
			s = localName + strlen(localName);    // point at the nul byte

		}
		CL_BeginDownload(localName, remoteName);

		cls.downloadRestart = qtrue;

		// move over the rest
		memmove(clc.downloadList, s, strlen(s) + 1);

		return;
	}

	CL_DownloadsComplete();
}

/**
 * @brief After receiving a valid game state, we validate the cgame and
 * local zip files here and determine if we need to download them
 */
void CL_InitDownloads(void)
{
	char missingfiles[1024];

	// init some of the www dl data
	clc.bWWWDl             = qfalse;
	clc.bWWWDlAborting     = qfalse;
	cls.bWWWDlDisconnected = qfalse;
	CL_ClearStaticDownload();

#ifdef FEATURE_AUTOUPDATE
	if (autoupdate.updateStarted && NET_CompareAdr(autoupdate.autoupdateServer, clc.serverAddress))
	{
		if (strlen(com_updatefiles->string) > 4)
		{
			char *updateFile;
			char updateFilesRemaining[MAX_TOKEN_CHARS] = "";

			clc.bWWWDl             = qtrue;
			cls.bWWWDlDisconnected = qtrue;

			updateFile = strtok(com_updatefiles->string, ";");

			if (updateFile == NULL)
			{
				Com_Error(ERR_AUTOUPDATE, "Could not parse update string.");
			}
			else
			{
				// download format: @remotename@localname
				Q_strncpyz(clc.downloadList, va("@%s@%s", updateFile, updateFile), MAX_INFO_STRING);
				Q_strncpyz(cls.originalDownloadName, updateFile, sizeof(cls.originalDownloadName));
				Q_strncpyz(cls.downloadName, va("%s/%s", UPDATE_SERVER_NAME, updateFile), sizeof(cls.downloadName));
				Q_strncpyz(cls.downloadTempName,
				           FS_BuildOSPath(Cvar_VariableString("fs_homepath"), AUTOUPDATE_DIR, va("%s.tmp", cls.originalDownloadName)),
				           sizeof(cls.downloadTempName));
				// TODO: add file size, so UI can show progress bar
				//Cvar_SetValue("cl_downloadSize", clc.downloadSize);

				if (!DL_BeginDownload(cls.downloadTempName, cls.downloadName))
				{
					Com_Error(ERR_AUTOUPDATE, "Could not download an update file: \"%s\"", cls.downloadName);
					clc.bWWWDlAborting = qtrue;
				}

				while (1)
				{
					updateFile = strtok(NULL, ";");

					if (updateFile == NULL)
					{
						break;
					}

					Q_strcat(updateFilesRemaining, sizeof(updateFilesRemaining), va("%s;", updateFile));
				}

				if (strlen(updateFilesRemaining) > 4)
				{
					Cvar_Set("com_updatefiles", updateFilesRemaining);
				}
				else
				{
					Cvar_Set("com_updatefiles", "");
				}
				return;
			}
		}
	}
	else
#endif // FEATURE_AUTOUPDATE
	{
		// whatever autodownlad configuration, store missing files in a cvar, use later in the ui maybe
		if (FS_ComparePaks(missingfiles, sizeof(missingfiles), qfalse))
		{
			Cvar_Set("com_missingFiles", missingfiles);
		}
		else
		{
			Cvar_Set("com_missingFiles", "");
		}

		// reset the redirect checksum tracking
		clc.redirectedList[0] = '\0';

		if (cl_allowDownload->integer && FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), qtrue))
		{
			// this gets printed to UI, i18n
			Com_Printf(CL_TranslateStringBuf("Need paks: %s\n"), clc.downloadList);

			if (*clc.downloadList)
			{
				// if autodownloading is not enabled on the server
				cls.state = CA_CONNECTED;
				CL_NextDownload();
				return;
			}
		}
	}

	CL_DownloadsComplete();
}

/**
 * @brief Resend a connect message if the last one has timed out
 */
void CL_CheckForResend(void)
{
	int  port, i;
	char info[MAX_INFO_STRING];
	char data[MAX_INFO_STRING];
	char pkt[1024 + 1];
	int  pktlen;

	// don't send anything if playing back a demo
	if (clc.demoplaying)
	{
		return;
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING)
	{
		return;
	}

	if (cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT)
	{
		return;
	}

	clc.connectTime = cls.realtime; // for retransmit requests
	clc.connectPacketCount++;

	switch (cls.state)
	{
	case CA_CONNECTING:
		strcpy(pkt, "getchallenge");
		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, pkt);
		break;

	case CA_CHALLENGING:
		// received and confirmed the challenge, now responding with a connect packet
		port = Cvar_VariableValue("net_qport");

		Q_strncpyz(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));
		Info_SetValueForKey(info, "protocol", va("%i", PROTOCOL_VERSION));
		Info_SetValueForKey(info, "qport", va("%i", port));
		Info_SetValueForKey(info, "challenge", va("%i", clc.challenge));

		strcpy(data, "connect ");

		data[8] = '\"'; // quote the string because it may contain spaces

		for (i = 0; i < strlen(info); i++)
		{
			data[9 + i] = info[i];
		}

		data[9 + i]  = '\"'; // ending quote
		data[10 + i] = 0;

		pktlen = i + 10 ;
		memcpy(pkt, &data[0], pktlen);

		NET_OutOfBandData(NS_CLIENT, clc.serverAddress, pkt, pktlen);
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		Com_Error(ERR_FATAL, "CL_CheckForResend: bad cls.state");
	}
}

/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket(netadr_t from)
{
	const char *message;

	if (cls.state < CA_AUTHORIZING)
	{
		return;
	}

	// if not from our server, ignore it
	if (!NET_CompareAdr(from, clc.netchan.remoteAddress))
	{
		return;
	}

	// if we have received packets within three seconds, ignore (it might be a malicious spoof)
	// NOTE: there used to be a  clc.lastPacketTime = cls.realtime; line in CL_PacketEvent before calling CL_ConnectionLessPacket
	// therefore .. packets never got through this check, clients never disconnected
	// switched the clc.lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
	// you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
	if (cls.realtime - clc.lastPacketTime < 3000)
	{
		return;
	}

	// if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
	if (!cls.bWWWDlDisconnected)
	{
		// drop the connection
		message = "Server disconnected for unknown reason";
		Com_Printf("%s\n", message);
		Cvar_Set("com_errorMessage", message);
		CL_Disconnect(qtrue);
	}
	else
	{
		CL_Disconnect(qfalse);
		Cvar_Set("ui_connecting", "1");
		Cvar_Set("ui_dl_running", "1");
	}
}

/**
 * @brief Client received 'motd' connectionless packet
 */
void CL_MotdPacket(netadr_t from)
{
	char *challenge;
	char *info;

	// if not from our server, ignore it
	if (!NET_CompareAdr(from, autoupdate.motdServer))
	{
		return;
	}

	info = Cmd_Argv(1);

	// check challenge
	challenge = Info_ValueForKey(info, "challenge");
	if (strcmp(challenge, autoupdate.motdChallenge))
	{
		return;
	}

	challenge = Info_ValueForKey(info, "motd");

	Q_strncpyz(cls.updateInfoString, info, sizeof(cls.updateInfoString));
	Cvar_Set("com_motdString", challenge);
}

/**
 * @brief an OOB message from server, with potential markups.
 * Print OOB are the only messages we handle markups in.
 *
 * [err_dialog]: Used to indicate that the connection should be aborted. No
 *               further information, just do an error diagnostic screen afterwards.
 * [err_prot]:   This is a protocol error. The client uses a custom protocol error
 *               message (client sided) in the diagnostic window.
 * ET://         Redirects client to another server.
 * The space for the error message on the connection screen is limited to 256 chars.
 */
void CL_PrintPacket(msg_t *msg)
{
	char *s;
	s = MSG_ReadBigString(msg);
	if (!Q_stricmpn(s, "[err_dialog]", 12))
	{
		Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
		Com_Error(ERR_DROP, "%s", clc.serverMessage);
	}
	else if (!Q_stricmpn(s, "[err_prot]", 10))
	{
		Q_strncpyz(clc.serverMessage, s + 10, sizeof(clc.serverMessage));
		Com_Error(ERR_DROP, "%s", CL_TranslateStringBuf(PROTOCOL_MISMATCH_ERROR_LONG));
	}
	else if (!Q_stricmpn(s, "[err_update]", 12))
	{
		Q_strncpyz(clc.serverMessage, s + 12, sizeof(clc.serverMessage));
		Com_Error(ERR_AUTOUPDATE, "%s", clc.serverMessage);
	}
	else if (!Q_stricmpn(s, "ET://", 5))
	{
		Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
		Cvar_Set("com_errorMessage", clc.serverMessage);
		Com_Error(ERR_DROP, "%s", clc.serverMessage);
	}
	else
	{
		Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
	}
	Com_Printf("%s", clc.serverMessage);
}

/*
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo(serverInfo_t *server, netadr_t *address)
{
	server->adr            = *address;
	server->clients        = 0;
	server->hostName[0]    = '\0';
	server->mapName[0]     = '\0';
	server->maxClients     = 0;
	server->maxPing        = 0;
	server->minPing        = 0;
	server->ping           = -1;
	server->game[0]        = '\0';
	server->gameType       = 0;
	server->netType        = 0;
	server->allowAnonymous = 0;
}

#define MAX_SERVERSPERPACKET    256

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket(const netadr_t *from, msg_t *msg, qboolean extended)
{
	int      i, count, total;
	netadr_t addresses[MAX_SERVERSPERPACKET];
	int      numservers;
	byte     *buffptr;
	byte     *buffend;

	Com_Printf("CL_ServersResponsePacket\n");

	if (cls.numglobalservers == -1)
	{
		// state to detect lack of servers or lack of response
		cls.numglobalservers         = 0;
		cls.numGlobalServerAddresses = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr    = msg->data;
	buffend    = buffptr + msg->cursize;

	// advance to initial token
	do
	{
		if (*buffptr == '\\' || (extended && *buffptr == '/'))
		{
			break;
		}

		buffptr++;
	}
	while (buffptr < buffend);

	while (buffptr + 1 < buffend)
	{
		// IPv4 address
		if (*buffptr == '\\')
		{
			buffptr++;

			if (buffend - buffptr < sizeof(addresses[numservers].ip) + sizeof(addresses[numservers].port) + 1)
			{
				break;
			}

			for (i = 0; i < sizeof(addresses[numservers].ip); i++)
				addresses[numservers].ip[i] = *buffptr++;

			addresses[numservers].type = NA_IP;
		}
		// IPv6 address, if it's an extended response
		else if (extended && *buffptr == '/')
		{
			buffptr++;

			if (buffend - buffptr < sizeof(addresses[numservers].ip6) + sizeof(addresses[numservers].port) + 1)
			{
				break;
			}

			for (i = 0; i < sizeof(addresses[numservers].ip6); i++)
				addresses[numservers].ip6[i] = *buffptr++;

			addresses[numservers].type     = NA_IP6;
			addresses[numservers].scope_id = from->scope_id;
		}
		else
		{
			// syntax error!
			break;
		}

		// parse out port
		addresses[numservers].port  = (*buffptr++) << 8;
		addresses[numservers].port += *buffptr++;
		addresses[numservers].port  = BigShort(addresses[numservers].port);

		// syntax check
		if (*buffptr != '\\' && *buffptr != '/')
		{
			break;
		}

		numservers++;
		if (numservers >= MAX_SERVERSPERPACKET)
		{
			break;
		}
	}

	count = cls.numglobalservers;

	for (i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++)
	{
		// build net address
		serverInfo_t *server = &cls.globalServers[count];

		CL_InitServerInfo(server, &addresses[i]);
		// advance to next slot
		count++;
	}

	// if getting the global list
	if (count >= MAX_GLOBAL_SERVERS && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS)
	{
		// if we couldn't store the servers in the main list anymore
		for ( ; i < numservers && cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS; i++)
		{
			// just store the addresses in an additional list
			cls.globalServerAddresses[cls.numGlobalServerAddresses++] = addresses[i];
		}
	}

	cls.numglobalservers = count;
	total                = count + cls.numGlobalServerAddresses;

	Com_Printf("%d servers parsed (total %d)\n", numservers, total);
}

/**
 * @brief Responses to broadcasts, etc
 *
 * Compare first n chars so it doesn’t bail if token is parsed incorrectly.
 */
void CL_ConnectionlessPacket(netadr_t from, msg_t *msg)
{
	char *s;
	char *c;

	MSG_BeginReadingOOB(msg);
	MSG_ReadLong(msg);      // skip the -1

	s = MSG_ReadStringLine(msg);

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);

	Com_DPrintf("CL packet %s: %s\n", NET_AdrToString(from), c);

	// challenge from the server we are connecting to
	if (!Q_stricmp(c, "challengeResponse"))
	{
		if (cls.state != CA_CONNECTING)
		{
			Com_Printf("Unwanted challenge response received.  Ignored.\n");
		}
		else
		{
			// start sending challenge response instead of challenge request packets
			clc.challenge = atoi(Cmd_Argv(1));
			if (Cmd_Argc() > 2)
			{
				clc.onlyVisibleClients = atoi(Cmd_Argv(2));
			}
			else
			{
				clc.onlyVisibleClients = 0;
			}
			cls.state              = CA_CHALLENGING;
			clc.connectPacketCount = 0;
			clc.connectTime        = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.serverAddress = from;
			Com_DPrintf("challenge: %d\n", clc.challenge);
		}
		return;
	}

	// server connection
	if (!Q_stricmp(c, "connectResponse"))
	{
		if (cls.state >= CA_CONNECTED)
		{
			Com_Printf("Dup connect received.  Ignored.\n");
			return;
		}
		if (cls.state != CA_CHALLENGING)
		{
			Com_Printf("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if (!NET_CompareAdr(from, clc.serverAddress))
		{
			Com_Printf("connectResponse from a different address.  Ignored.\n");
			Com_Printf("%s should have been %s\n", NET_AdrToString(from),
			           NET_AdrToString(clc.serverAddress));
			return;
		}

		// If we have completed a connection to the Auto-Update server...
		if (autoupdate.updateChecked && NET_CompareAdr(autoupdate.autoupdateServer, clc.serverAddress))
		{
			// Mark the client as being in the process of getting an update
			if (com_updateavailable->integer)
			{
				autoupdate.updateStarted = qtrue;
			}
		}

		Netchan_Setup(NS_CLIENT, &clc.netchan, from, Cvar_VariableValue("net_qport"));
		cls.state              = CA_CONNECTED;
		clc.lastPacketSentTime = -9999;     // send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if (!Q_stricmp(c, "infoResponse"))
	{
		CL_ServerInfoPacket(from, msg);
		return;
	}

	// server responding to a get playerlist
	if (!Q_stricmp(c, "statusResponse"))
	{
		CL_ServerStatusResponse(from, msg);
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect"))
	{
		CL_DisconnectPacket(from);
		return;
	}

	// echo request from server
	if (!Q_stricmp(c, "echo"))
	{
		NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
		return;
	}

	// cd check
	if (!Q_stricmp(c, "keyAuthorize"))
	{
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD
	if (!Q_stricmp(c, "motd"))
	{
		CL_MotdPacket(from);
		return;
	}

	// echo request from server
	if (!Q_stricmp(c, "print"))
	{
		// NOTE: we may have to add exceptions for auth and update servers
		if (NET_CompareAdr(from, clc.serverAddress) || NET_CompareAdr(from, rcon_address))
		{
			CL_PrintPacket(msg);
		}
		return;
	}

	// Update server response message
	if (!Q_stricmp(c, "updateResponse"))
	{
		CL_UpdateInfoPacket(from);
		return;
	}

	// list of servers sent back by a master server
	if (!Q_strncmp(c, "getserversResponse", 18))
	{
		CL_ServersResponsePacket(&from, msg, qfalse);
		return;
	}

	// list of servers sent back by a master server (extended)
	if (!Q_strncmp(c, "getserversExtResponse", 21))
	{
		CL_ServersResponsePacket(&from, msg, qtrue);
		return;
	}

	Com_DPrintf("Unknown connectionless packet command.\n");
}

/**
 * @brief A packet has arrived from the main event loop
 */
void CL_PacketEvent(netadr_t from, msg_t *msg)
{
	int headerBytes;

#ifdef FEATURE_AUTOUPDATE
	static qboolean autoupdateRedirected = qfalse;

	// Update server doesn't understand netchan packets
	if (NET_CompareAdr(autoupdate.autoupdateServer, from)
	    && autoupdate.updateStarted && !autoupdateRedirected)
	{
		autoupdateRedirected = qtrue;
		CL_InitDownloads();
		return;
	}
#endif /* FEATURE_AUTOUPDATE */

	if (msg->cursize >= 4 && *( int * ) msg->data == -1)
	{
		CL_ConnectionlessPacket(from, msg);
		return;
	}

	clc.lastPacketTime = cls.realtime;

	if (cls.state < CA_CONNECTED)
	{
		return;     // can't be a valid sequenced packet
	}

	if (msg->cursize < 4)
	{
		Com_Printf("%s: Runt packet\n", NET_AdrToString(from));
		return;
	}

	// packet from server
	if (!NET_CompareAdr(from, clc.netchan.remoteAddress))
	{
		Com_DPrintf("%s:sequenced packet without connection\n"
		            , NET_AdrToString(from));
		// FIXME: send a client disconnect?
		return;
	}

	if (!CL_Netchan_Process(&clc.netchan, msg))
	{
		return;     // out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.serverMessageSequence = LittleLong(*( int * ) msg->data);

	clc.lastPacketTime = cls.realtime;
	CL_ParseServerMessage(msg);

	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	if (clc.demorecording && !clc.demowaiting)
	{
		CL_WriteDemoMessage(msg, headerBytes);
	}
}

/*
==================
CL_CheckTimeout
==================
*/
void CL_CheckTimeout(void)
{
	// check timeout
	if ((!cl_paused->integer || !sv_paused->integer)
	    && cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC
	    && cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000)
	{
		if (++cl.timeoutcount > 5)        // timeoutcount saves debugger
		{
			Cvar_Set("com_errorMessage", "Server connection timed out.");
			CL_Disconnect(qtrue);
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}

//============================================================================

/*
==================
CL_CheckUserinfo
==================
*/
void CL_CheckUserinfo(void)
{
	// don't add reliable commands when not yet connected
	if (cls.state < CA_CHALLENGING)
	{
		return;
	}
	// don't overflow the reliable command buffer when paused
	if (cl_paused->integer)
	{
		return;
	}
	// send a reliable userinfo update if needed
	if (cvar_modifiedFlags & CVAR_USERINFO)
	{
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString(CVAR_USERINFO)));
	}
}

/*
==================
CL_WWWDownload
==================
*/
void CL_WWWDownload(void)
{
	char            *to_ospath;
	dlStatus_t      ret;
	static qboolean bAbort = qfalse;

	if (clc.bWWWDlAborting)
	{
		if (!bAbort)
		{
			Com_DPrintf("CL_WWWDownload: WWWDlAborting\n");
			bAbort = qtrue;
		}
		return;
	}
	if (bAbort)
	{
		Com_DPrintf("CL_WWWDownload: WWWDlAborting done\n");
		bAbort = qfalse;
	}

	ret = DL_DownloadLoop();

	if (ret == DL_CONTINUE)
	{
		return;
	}

	if (ret == DL_DONE)
	{
		// taken from CL_ParseDownload
		// we work with OS paths
		clc.download                     = 0;
		to_ospath                        = FS_BuildOSPath(Cvar_VariableString("fs_homepath"), cls.originalDownloadName, "");
		to_ospath[strlen(to_ospath) - 1] = '\0';
		if (rename(cls.downloadTempName, to_ospath))
		{
			FS_CopyFile(cls.downloadTempName, to_ospath);
			remove(cls.downloadTempName);
		}
		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set("cl_downloadName", "");
		if (cls.bWWWDlDisconnected)
		{
			// for an auto-update in disconnected mode, we'll be spawning the setup in CL_DownloadsComplete
			if (!autoupdate.updateStarted)
			{
				// reconnect to the server, which might send us to a new disconnected download
				Cbuf_ExecuteText(EXEC_APPEND, "reconnect\n");
			}
		}
		else
		{
			CL_AddReliableCommand("wwwdl done");
			// tracking potential web redirects leading us to wrong checksum - only works in connected mode
			if (strlen(clc.redirectedList) + strlen(cls.originalDownloadName) + 1 >= sizeof(clc.redirectedList))
			{
				// just to be safe
				Com_Printf("ERROR: redirectedList overflow (%s)\n", clc.redirectedList);
			}
			else
			{
				strcat(clc.redirectedList, "@");
				strcat(clc.redirectedList, cls.originalDownloadName);
			}
		}
	}
	else
	{
		if (cls.bWWWDlDisconnected)
		{
			// in a connected download, we'd tell the server about failure and wait for a reply
			// but in this case we can't get anything from server
			// if we just reconnect it's likely we'll get the same disconnected download message, and error out again
			// this may happen for a regular dl or an auto update
			const char *error = va("Download failure while getting '%s'\n", cls.downloadName);    // get the msg before clearing structs

			cls.bWWWDlDisconnected = qfalse; // need clearing structs before ERR_DROP, or it goes into endless reload
			CL_ClearStaticDownload();
			Com_Error(ERR_DROP, "%s", error);
		}
		else
		{
			// see CL_ParseDownload, same abort strategy
			Com_Printf("Download failure while getting '%s'\n", cls.downloadName);
			CL_AddReliableCommand("wwwdl fail");
			clc.bWWWDlAborting = qtrue;
		}
		return;
	}

	clc.bWWWDl = qfalse;
	CL_NextDownload();
}

/*
==================
CL_WWWBadChecksum

FS code calls this when doing FS_ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
qboolean CL_WWWBadChecksum(const char *pakname)
{
	if (strstr(clc.redirectedList, va("@%s", pakname)))
	{
		Com_Printf("WARNING: file %s obtained through download redirect has wrong checksum\n", pakname);
		Com_Printf("         this likely means the server configuration is broken\n");
		if (strlen(clc.badChecksumList) + strlen(pakname) + 1 >= sizeof(clc.badChecksumList))
		{
			Com_Printf("ERROR: badChecksumList overflowed (%s)\n", clc.badChecksumList);
			return qfalse;
		}
		strcat(clc.badChecksumList, "@");
		strcat(clc.badChecksumList, pakname);
		Com_DPrintf("bad checksums: %s\n", clc.badChecksumList);
		return qtrue;
	}
	return qfalse;
}

/*
==================
CL_StartVideoRecording

This function will be called when the AVI recording will start either by video or cl_avidemo commands
==================
*/
void CL_StartVideoRecording(const char *aviname)
{
	char filename[MAX_OSPATH];

	if (!aviname || aviname == '\0')
	{
		int i, last;
		int a, b, c, d;

		for (i = 0; i <= 9999; i++)
		{
			last = i;

			a     = last / 1000;
			last -= a * 1000;
			b     = last / 100;
			last -= b * 100;
			c     = last / 10;
			last -= c * 10;
			d     = last;
			Com_Printf("videos/%s%d%d%d%d.avi", clc.demoName, a, b, c, d);
			Com_sprintf(filename, MAX_OSPATH, "videos/%s%d%d%d%d.avi", clc.demoName, a, b, c, d);

			if (!FS_FileExists(filename))
			{
				break; // file doesn't exist
			}
		}

		if (i > 9999)
		{
			Com_Printf(S_COLOR_RED "ERROR: no free file names to create video\n");
			return;
		}
		CL_OpenAVIForWriting(filename);
	}
	else
	{
		CL_OpenAVIForWriting(aviname);
	}
}

/*
===============
CL_Video_f

video
video [filename]
===============
*/
void CL_Video_f(void)
{
	char filename[MAX_OSPATH];

	if (!clc.demoplaying)
	{
		Com_Printf("The video command can only be used when playing back demos\n");
		return;
	}

	cl_avidemotype->integer = 2;
	if (cl_avidemo->integer == 0)
	{
		cl_avidemo->integer = 30;
	}

	if (Cmd_Argc() > 1)
	{
		// explicit filename
		Com_sprintf(filename, MAX_OSPATH, "videos/%s.avi", Cmd_Argv(1));
		if (Cmd_Argc() == 3)
		{
			Cvar_Set("cl_avidemo", Cmd_Argv(2));
		}
		CL_StartVideoRecording(filename);
	}
	else
	{
		CL_StartVideoRecording(NULL);
	}
}

/*
===============
CL_StopVideo_f
===============
*/
void CL_StopVideo_f(void)
{
	if (CL_VideoRecording())
	{
		//cl_avidemo->integer = 0;
		Cvar_Set("cl_avidemo", "0");

		// We need to call something like S_Base_StopAllSounds();
		// here to stop the stuttering. Something it crashes the game.
		Cmd_ExecuteString("s_stop");
		S_StopAllSounds();
		CL_CloseAVI();
	}
}

/*
==================
CL_Frame
==================
*/
void CL_Frame(int msec)
{
	if (!com_cl_running->integer)
	{
		return;
	}

	if (cls.state == CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_UI)
	    && !com_sv_running->integer)
	{
		// if disconnected, bring up the menu
		S_StopAllSounds();
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
	}

	// if recording an avi, lock to a fixed fps
	if (cl_avidemo->integer && msec)
	{
		// save the current screen
		if (cls.state == CA_ACTIVE || cl_forceavidemo->integer)
		{
			switch (cl_avidemotype->integer)
			{
			case 1:
				Cbuf_ExecuteText(EXEC_NOW, "screenshotJPEG silent\n");
				break;
			case 2:
				if (CL_VideoRecording())
				{
					CL_TakeVideoFrame();
				}
				else
				{
					CL_StartVideoRecording(NULL);
					CL_TakeVideoFrame();
					//Com_Printf("Error while recording avi, the file is not open.\n");
				}
				break;
			default:
				Cbuf_ExecuteText(EXEC_NOW, "screenshot silent\n");
			}
		}
		// fixed time for next frame
		msec = (1000 / cl_avidemo->integer) * com_timescale->value;
		if (msec == 0)
		{
			msec = 1;
		}
	}
	else if (cl_avidemo->integer == 0 && CL_VideoRecording())
	{
		CL_StopVideo_f();
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	if (cl_timegraph->integer)
	{
		SCR_DebugGraph(cls.realFrametime * 0.25);
	}

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// wwwdl download may survive a server disconnect
	if ((cls.state == CA_CONNECTED && clc.bWWWDl) || cls.bWWWDlDisconnected)
	{
		CL_WWWDownload();
	}

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update the sound
	S_Update();

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}

//============================================================================

// startup-caching system
typedef struct
{
	char name[MAX_QPATH];
	int hits;
	int lastSetIndex;
} cacheItem_t;
typedef enum
{
	CACHE_SOUNDS,
	CACHE_MODELS,
	CACHE_IMAGES,

	CACHE_NUMGROUPS
} cacheGroup_t;
static cacheItem_t cacheGroups[CACHE_NUMGROUPS] =
{
	{ { 's', 'o', 'u', 'n', 'd', 0 }, CACHE_SOUNDS },
	{ { 'm', 'o', 'd', 'e', 'l', 0 }, CACHE_MODELS },
	{ { 'i', 'm', 'a', 'g', 'e', 0 }, CACHE_IMAGES },
};
#define MAX_CACHE_ITEMS     4096
#define CACHE_HIT_RATIO     0.75        // if hit on this percentage of maps, it'll get cached

static int         cacheIndex;
static cacheItem_t cacheItems[CACHE_NUMGROUPS][MAX_CACHE_ITEMS];

static void CL_Cache_StartGather_f(void)
{
	cacheIndex = 0;
	memset(cacheItems, 0, sizeof(cacheItems));

	Cvar_Set("cl_cacheGathering", "1");
}

static void CL_Cache_UsedFile_f(void)
{
	char        groupStr[MAX_QPATH];
	char        itemStr[MAX_QPATH];
	int         i, group;
	cacheItem_t *item;

	if (Cmd_Argc() < 2)
	{
		Com_Error(ERR_DROP, "usedfile without enough parameters");
		return;
	}

	strcpy(groupStr, Cmd_Argv(1));

	strcpy(itemStr, Cmd_Argv(2));
	for (i = 3; i < Cmd_Argc(); i++)
	{
		strcat(itemStr, " ");
		strcat(itemStr, Cmd_Argv(i));
	}
	Q_strlwr(itemStr);

	// find the cache group
	for (i = 0; i < CACHE_NUMGROUPS; i++)
	{
		if (!Q_strncmp(groupStr, cacheGroups[i].name, MAX_QPATH))
		{
			break;
		}
	}
	if (i == CACHE_NUMGROUPS)
	{
		Com_Error(ERR_DROP, "usedfile without a valid cache group");
		return;
	}

	// see if it's already there
	group = i;
	for (i = 0, item = cacheItems[group]; i < MAX_CACHE_ITEMS; i++, item++)
	{
		if (!item->name[0])
		{
			// didn't find it, so add it here
			Q_strncpyz(item->name, itemStr, MAX_QPATH);
			if (cacheIndex > 9999)     // hack, but yeh
			{
				item->hits = cacheIndex;
			}
			else
			{
				item->hits++;
			}
			item->lastSetIndex = cacheIndex;
			break;
		}
		if (item->name[0] == itemStr[0] && !Q_strncmp(item->name, itemStr, MAX_QPATH))
		{
			if (item->lastSetIndex != cacheIndex)
			{
				item->hits++;
				item->lastSetIndex = cacheIndex;
			}
			break;
		}
	}
}

static void CL_Cache_SetIndex_f(void)
{
	if (Cmd_Argc() < 2)
	{
		Com_Error(ERR_DROP, "setindex needs an index");
		return;
	}

	cacheIndex = atoi(Cmd_Argv(1));
}

static void CL_Cache_MapChange_f(void)
{
	cacheIndex++;
}

static void CL_Cache_EndGather_f(void)
{
	// save the frequently used files to the cache list file
	int  i, j, handle, cachePass;
	char filename[MAX_QPATH];

	cachePass = ( int ) floor(( float ) cacheIndex * CACHE_HIT_RATIO);

	for (i = 0; i < CACHE_NUMGROUPS; i++)
	{
		Q_strncpyz(filename, cacheGroups[i].name, MAX_QPATH);
		Q_strcat(filename, MAX_QPATH, ".cache");

		handle = FS_FOpenFileWrite(filename);

		for (j = 0; j < MAX_CACHE_ITEMS; j++)
		{
			// if it's a valid filename, and it's been hit enough times, cache it
			if (cacheItems[i][j].hits >= cachePass && strstr(cacheItems[i][j].name, "/"))
			{
				FS_Write(cacheItems[i][j].name, strlen(cacheItems[i][j].name), handle);
				FS_Write("\n", 1, handle);
			}
		}

		FS_FCloseFile(handle);
	}

	Cvar_Set("cl_cacheGathering", "0");
}

//============================================================================

/*
================
CL_SetRecommended_f
================
*/
void CL_SetRecommended_f(void)
{
	Com_SetRecommended();
}

/*
================
CL_RefPrintf

DLL glue
================
*/
static __attribute__ ((format(printf, 2, 3))) void QDECL CL_RefPrintf(int print_level, const char *fmt, ...)
{
	va_list argptr;
	char    msg[MAXPRINTMSG];

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (print_level == PRINT_ALL)
	{
		Com_Printf("%s", msg);
	}
	else if (print_level == PRINT_WARNING)
	{
		Com_Printf(S_COLOR_YELLOW "%s", msg); // yellow
	}
	else if (print_level == PRINT_DEVELOPER)
	{
		Com_DPrintf(S_COLOR_RED "%s", msg); // red
	}
}

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef(void)
{
	if (!re.Shutdown)
	{
		return;
	}
	re.Shutdown(qtrue);
	memset(&re, 0, sizeof(re));
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer(void)
{
	// this sets up the renderer and calls R_Init
	re.BeginRegistration(&cls.glconfig);

	// load character sets
	cls.charSetShader = re.RegisterShader("gfx/2d/consolechars");
	cls.whiteShader   = re.RegisterShader("white");

	cls.consoleShader  = re.RegisterShader("console-16bit");   // shader works with 16bit
	cls.consoleShader2 = re.RegisterShader("console2-16bit");    // shader works with 16bit

	g_console_field_width       = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
	g_consoleField.widthInChars = g_console_field_width;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers(void)
{
	if (!com_cl_running)
	{
		return;
	}

	if (!com_cl_running->integer)
	{
		return;
	}

	if (!cls.rendererStarted)
	{
		cls.rendererStarted = qtrue;
		CL_InitRenderer();
	}

	if (!cls.soundStarted)
	{
		cls.soundStarted = qtrue;
		S_Init();
	}

	if (!cls.soundRegistered)
	{
		cls.soundRegistered = qtrue;
		S_BeginRegistration();
	}

	if (!cls.uiStarted)
	{
		cls.uiStarted = qtrue;
		CL_InitUI();
	}
}

void CL_CheckAutoUpdate(void)
{
	char info[MAX_INFO_STRING];

	if (!com_autoupdate->integer)
	{
		Com_DPrintf("Updater is disabled by com_autoupdate 0.\n");
		return;
	}

	// Only check once per session
	if (autoupdate.updateChecked)
	{
		return;
	}

	// Resolve update server
	Com_Printf("Updater: resolving %s... ", UPDATE_SERVER_NAME);

	if (!NET_StringToAdr(va("%s:%i", UPDATE_SERVER_NAME, PORT_UPDATE), &autoupdate.autoupdateServer, NA_UNSPEC))
	{
		Com_Printf("couldn't resolve address\n");

		autoupdate.updateChecked = qtrue;
		return;
	}
	else
	{
		Com_Printf("resolved to %s\n", NET_AdrToString(autoupdate.autoupdateServer));
	}

	info[0] = 0;
	Info_SetValueForKey(info, "version", ETLEGACY_VERSION_SHORT);
	Info_SetValueForKey(info, "platform", CPUSTRING);
	Info_SetValueForKey(info, va("etl_bin_%s.pk3", ETLEGACY_VERSION_SHORT),
	                    Com_MD5File(va("legacy/etl_bin_%s.pk3", ETLEGACY_VERSION_SHORT), 0, NULL, 0));
	Info_SetValueForKey(info, va("pak3_%s.pk3", ETLEGACY_VERSION_SHORT),
	                    Com_MD5File(va("legacy/pak3_%s.pk3", ETLEGACY_VERSION_SHORT), 0, NULL, 0));

	NET_OutOfBandPrint(NS_CLIENT, autoupdate.autoupdateServer, "getUpdateInfo \"%s\"", info);

	autoupdate.updateChecked = qtrue;
}

#ifdef FEATURE_AUTOUPDATE
void CL_GetAutoUpdate(void)
{
	// Don't try and get an update if we haven't checked for one
	if (!autoupdate.updateChecked)
	{
		return;
	}

	// Make sure there's a valid update file to request
	if (strlen(com_updatefiles->string) < 5)
	{
		return;
	}

	Com_DPrintf("Connecting to auto-update server...\n");

	S_StopAllSounds();

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// toggle on all the download related cvars
	Cvar_Set("cl_allowDownload", "1");  // general flag
	Cvar_Set("cl_wwwDownload", "1");    // ftp/http support

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	if (com_sv_running->integer)
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit\n");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	CL_Disconnect(qtrue);
	Con_Close();

	Q_strncpyz(cls.servername, "ET:L Update Server", sizeof(cls.servername));

	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_Printf("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		Cvar_Set("ui_connecting", "0");
		return;
	}

	// Copy auto-update server address to Server connect address
	memcpy(&clc.serverAddress, &autoupdate.autoupdateServer, sizeof(netadr_t));

	Com_DPrintf("%s resolved to %s\n", cls.servername,
	            NET_AdrToString(clc.serverAddress));

	cls.state = CA_CONNECTING;

	cls.keyCatchers        = 0;
	clc.connectTime        = -99999; // CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set("cl_currentServerAddress", "ET:L Update Server");
}
#endif /* FEATURE_AUTOUPDATE */

/*
============
CL_RefMalloc
============
*/
#ifdef ZONE_DEBUG
void *CL_RefMallocDebug(int size, char *label, char *file, int line)
{
	return Z_TagMallocDebug(size, TAG_RENDERER, label, file, line);
}
#else
void *CL_RefMalloc(int size)
{
	return Z_TagMalloc(size, TAG_RENDERER);
}
#endif

/*
============
CL_RefTagFree
============
*/
void CL_RefTagFree(void)
{
	Z_FreeTags(TAG_RENDERER);
	return;
}

int CL_ScaledMilliseconds(void)
{
	return Sys_Milliseconds() * com_timescale->value;
}
#ifndef USE_RENDERER_DLOPEN
extern refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp);
#endif

/*
============
CL_InitRef
============
*/
void CL_InitRef(void)
{
	refimport_t ri;
	refexport_t *ret;
#ifdef USE_RENDERER_DLOPEN
	GetRefAPI_t GetRefAPI;
	char        dllName[MAX_OSPATH];
#endif

	Com_Printf("----- Initializing Renderer ----\n");

#ifdef USE_RENDERER_DLOPEN
	cl_renderer = Cvar_Get("cl_renderer", "opengl1", CVAR_ARCHIVE | CVAR_LATCH);


#ifdef _WIN32
	Com_sprintf(dllName, sizeof(dllName), "renderer_%s_" ARCH_STRING DLL_EXT, cl_renderer->string);
#else // *nix
	Com_sprintf(dllName, sizeof(dllName), "librenderer_%s_" ARCH_STRING DLL_EXT, cl_renderer->string);
#endif
	if (!(rendererLib = Sys_LoadDll(dllName, qfalse)) && strcmp(cl_renderer->string, cl_renderer->resetString))
	{
		Cvar_ForceReset("cl_renderer");
#ifdef _WIN32
		Com_sprintf(dllName, sizeof(dllName), "renderer_opengl1_" ARCH_STRING DLL_EXT);
#else // *nix
		Com_sprintf(dllName, sizeof(dllName), "librenderer_opengl1_" ARCH_STRING DLL_EXT);
#endif
		rendererLib = Sys_LoadLibrary(dllName);
	}

	if (!rendererLib)
	{
		Com_Printf("failed:\n\"%s\"\n", Sys_LibraryError());
		Com_Error(ERR_FATAL, "Failed to load renderer lib");
	}

	GetRefAPI = Sys_LoadFunction(rendererLib, "GetRefAPI");
	if (!GetRefAPI)
	{
		Com_Error(ERR_FATAL, "Can't load symbol GetRefAPI: '%s'", Sys_LibraryError());
	}
#endif

	ri.Cmd_AddCommand    = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc          = Cmd_Argc;
	ri.Cmd_Argv          = Cmd_Argv;
	ri.Cmd_ExecuteText   = Cbuf_ExecuteText;
	ri.Printf            = CL_RefPrintf;
	ri.Error             = Com_Error;
	ri.Milliseconds      = CL_ScaledMilliseconds;
	ri.RealTime          = Com_RealTime;
#ifdef ZONE_DEBUG
	ri.Z_MallocDebug = CL_RefMallocDebug;
#else
	ri.Z_Malloc = CL_RefMalloc;
#endif
	ri.Free       = Z_Free;
	ri.Tag_Free   = CL_RefTagFree;
	ri.Hunk_Clear = Hunk_ClearToMark;
#ifdef HUNK_DEBUG
	ri.Hunk_AllocDebug = Hunk_AllocDebug;
#else
	ri.Hunk_Alloc = Hunk_Alloc;
#endif
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory     = Hunk_FreeTempMemory;

	//ri.CM_ClusterPVS = CM_ClusterPVS;
	ri.CM_PointContents    = CM_PointContents;
	ri.CM_DrawDebugSurface = CM_DrawDebugSurface;

	ri.FS_ReadFile     = FS_ReadFile;
	ri.FS_FreeFile     = FS_FreeFile;
	ri.FS_WriteFile    = FS_WriteFile;
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_ListFiles    = FS_ListFiles;
	ri.FS_FileIsInPAK  = FS_FileIsInPAK;
	ri.FS_FileExists   = FS_FileExists;

	ri.FS_FOpenFileRead = FS_FOpenFileRead;
	ri.FS_Read          = FS_Read;

	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	//ri.Cvar_SetValue = Cvar_SetValue;
	//ri.Cvar_CheckRange = Cvar_CheckRange;
	//ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;

	// cinematic stuff
	ri.CIN_UploadCinematic = CIN_UploadCinematic;
	ri.CIN_PlayCinematic   = CIN_PlayCinematic;
	ri.CIN_RunCinematic    = CIN_RunCinematic;

	ri.CL_VideoRecording     = CL_VideoRecording;
	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

	ri.Sys_GLimpSafeInit = Sys_GLimpSafeInit;
	ri.Sys_GLimpInit     = Sys_GLimpInit;
	ri.Sys_SetEnv        = Sys_SetEnv;

	ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;

	ri.IN_Init     = IN_Init;
	ri.IN_Shutdown = IN_Shutdown;
	ri.IN_Restart  = IN_Restart;

	//ri.ftol = Q_ftol;

	//ri.Sys_SetEnv = Sys_SetEnv;
	//ri.Sys_LowPhysicalMemory = Sys_LowPhysicalMemory;

	ret = GetRefAPI(REF_API_VERSION, &ri);

	Com_Printf("-------------------------------\n");

	if (!ret)
	{
		Com_Error(ERR_FATAL, "Couldn't initialize refresh");
	}

	re = *ret;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");
}

//===========================================================================================

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	Com_Printf("----- Client Initialization -----\n");

	Con_Init();

	CL_ClearState();

	cls.state = CA_DISCONNECTED;    // no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput();

#ifdef FEATURE_IRC_CLIENT
	CL_OW_IRCSetup();
#endif

	// register our variables
	cl_noprint = Cvar_Get("cl_noprint", "0", 0);

	cl_timeout = Cvar_Get("cl_timeout", "30", 0);

	cl_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP);

	cl_timeNudge          = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	cl_shownet            = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
	cl_shownuments        = Cvar_Get("cl_shownuments", "0", CVAR_TEMP);
	cl_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	cl_showSend           = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	cl_showTimeDelta      = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo         = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password  = Cvar_Get("rconPassword", "", CVAR_TEMP);
	cl_activeAction       = Cvar_Get("activeAction", "", CVAR_TEMP);
	cl_autorecord         = Cvar_Get("cl_autorecord", "0", CVAR_TEMP);

	cl_timedemo      = Cvar_Get("timedemo", "0", 0);
	cl_avidemo       = Cvar_Get("cl_avidemo", "0", 0);
	cl_forceavidemo  = Cvar_Get("cl_forceavidemo", "0", 0);
	cl_avidemotype   = Cvar_Get("cl_avidemotype", "0", CVAR_ARCHIVE);
	cl_aviMotionJpeg = Cvar_Get("cl_avimotionjpeg", "0", CVAR_TEMP);

	rconAddress = Cvar_Get("rconAddress", "", 0);

	cl_yawspeed      = Cvar_Get("cl_yawspeed", "140", CVAR_ARCHIVE);
	cl_pitchspeed    = Cvar_Get("cl_pitchspeed", "140", CVAR_ARCHIVE);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);

	cl_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	cl_packetdup  = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

	cl_run         = Cvar_Get("cl_run", "1", CVAR_ARCHIVE);
	cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE);
	cl_mouseAccel  = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE);
	cl_freelook    = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE);

	cl_showMouseRate = Cvar_Get("cl_showmouserate", "0", 0);

	cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE);
	cl_wwwDownload   = Cvar_Get("cl_wwwDownload", "1", CVAR_USERINFO | CVAR_ARCHIVE);

	cl_profile        = Cvar_Get("cl_profile", "", CVAR_ROM);
	cl_defaultProfile = Cvar_Get("cl_defaultProfile", "", CVAR_ROM);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// disabled autoswitch by default
	Cvar_Get("cg_autoswitch", "0", CVAR_ARCHIVE);

	// particle switch
	Cvar_Get("cg_wolfparticles", "1", CVAR_ARCHIVE);

	cl_conXOffset  = Cvar_Get("cl_conXOffset", "0", 0);
	cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);

	cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);

	cl_recoilPitch = Cvar_Get("cg_recoilPitch", "0", CVAR_ROM);

	cl_bypassMouseInput = Cvar_Get("cl_bypassMouseInput", "0", 0);    //CVAR_ROM );

	cl_doubletapdelay = Cvar_Get("cl_doubletapdelay", "350", CVAR_ARCHIVE);    // double tap

	m_pitch   = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw     = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE);
	m_side    = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE);
	m_filter  = Cvar_Get("m_filter", "0", CVAR_ARCHIVE);

	// make these cvars visible to cgame
	cl_demorecording = Cvar_Get("cl_demorecording", "0", CVAR_ROM);
	cl_demofilename  = Cvar_Get("cl_demofilename", "", CVAR_ROM);
	cl_demooffset    = Cvar_Get("cl_demooffset", "0", CVAR_ROM);
	cl_waverecording = Cvar_Get("cl_waverecording", "0", CVAR_ROM);
	cl_wavefilename  = Cvar_Get("cl_wavefilename", "", CVAR_ROM);
	cl_waveoffset    = Cvar_Get("cl_waveoffset", "0", CVAR_ROM);

	cl_packetloss  = Cvar_Get("cl_packetloss", "0", CVAR_CHEAT);
	cl_packetdelay = Cvar_Get("cl_packetdelay", "0", CVAR_CHEAT);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);

	// ~ and `, as keys and characters
	cl_consoleKeys = Cvar_Get("cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE);

	Cvar_Get("cg_drawCompass", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawNotifyText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_quickMessageAlt", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_popupLimboMenu", "1", CVAR_ARCHIVE);  // not used in legacy mod, kept for compatibility
	Cvar_Get("cg_descriptiveText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_drawTeamOverlay", "2", CVAR_ARCHIVE); // not used in legacy mod, kept for compatibility
	Cvar_Get("cg_drawGun", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_cursorHints", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_voiceSpriteTime", "6000", CVAR_ARCHIVE);
	Cvar_Get("cg_crosshairSize", "48", CVAR_ARCHIVE);
	Cvar_Get("cg_drawCrosshair", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomDefaultSniper", "20", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomStepSniper", "2", CVAR_ARCHIVE);

	// userinfo
	Cvar_Get("name", "ETLegacyPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "5000", CVAR_USERINFO | CVAR_ARCHIVE);       // changed from 3000
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_ARCHIVE);

	Cvar_Get("cg_autoactivate", "1", CVAR_ARCHIVE);

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE);

	cl_missionStats = Cvar_Get("g_missionStats", "0", CVAR_ROM);

	// Auto-update
	com_updateavailable = Cvar_Get("com_updateavailable", "0", CVAR_ROM);
	com_updatefiles     = Cvar_Get("com_updatefiles", "", CVAR_ROM);

	// register our commands
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand("configstrings", CL_Configstrings_f);
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand("snd_reload", CL_Snd_Reload_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand("ui_restart", CL_UI_Restart_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("demo", CL_PlayDemo_f);
	Cmd_SetCommandCompletionFunc("demo", CL_CompleteDemoName);
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand("connect", CL_Connect_f);
	Cmd_AddCommand("reconnect", CL_Reconnect_f);
	Cmd_AddCommand("localservers", CL_LocalServers_f);
	Cmd_AddCommand("globalservers", CL_GlobalServers_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_SetCommandCompletionFunc("rcon", CL_CompleteRcon);
	Cmd_AddCommand("setenv", CL_Setenv_f);
	Cmd_AddCommand("ping", CL_Ping_f);
	Cmd_AddCommand("serverstatus", CL_ServerStatus_f);
	Cmd_AddCommand("showip", CL_ShowIP_f);
	Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f);
	Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f);

#ifdef FEATURE_IRC_CLIENT
	Cmd_AddCommand("irc_connect", CL_OW_InitIRC);
	Cmd_AddCommand("irc_quit", CL_OW_IRCInitiateShutdown);
	Cmd_AddCommand("irc_say", CL_OW_IRCSay);
#endif

	// startup-caching system
	Cmd_AddCommand("cache_startgather", CL_Cache_StartGather_f);
	Cmd_AddCommand("cache_usedfile", CL_Cache_UsedFile_f);
	Cmd_AddCommand("cache_setindex", CL_Cache_SetIndex_f);
	Cmd_AddCommand("cache_mapchange", CL_Cache_MapChange_f);
	Cmd_AddCommand("cache_endgather", CL_Cache_EndGather_f);

	Cmd_AddCommand("updatehunkusage", CL_UpdateLevelHunkUsage);
	Cmd_AddCommand("updatescreen", SCR_UpdateScreen);

	Cmd_AddCommand("setRecommended", CL_SetRecommended_f);

	// we eat these commands to prevent exploits
	Cmd_AddCommand("userinfo", CL_EatMe_f);

	Cmd_AddCommand("wav_record", CL_WavRecord_f);
	Cmd_AddCommand("wav_stoprecord", CL_WavStopRecord_f);

	// Avi recording
	Cmd_AddCommand("video", CL_Video_f);
	Cmd_AddCommand("stopvideo", CL_StopVideo_f);

	CL_InitRef();

	SCR_Init();

	Cbuf_Execute();

	Cvar_Set("cl_running", "1");

	CL_GenerateETKey();
	Cvar_Get("cl_guid", "", CVAR_USERINFO | CVAR_ROM);
	CL_UpdateGUID();

	autoupdate.updateChecked = qfalse;
	autoupdate.updateStarted = qfalse;

#ifdef FEATURE_GETTEXT
	I18N_Init();
#endif

	// Initialize random number to be used in pairing of messages with replies
	srand(Com_Milliseconds());

	Com_Printf("----- Client Initialization Complete -----\n");
}

/*
===============
CL_Shutdown
===============
*/
void CL_Shutdown(void)
{
	static qboolean recursive = qfalse;

	Com_Printf("----- CL_Shutdown -----\n");

	if (recursive)
	{
		printf("recursive shutdown\n");
		return;
	}
	recursive = qtrue;

	if (clc.waverecording)     // write wav header when we quit
	{
		CL_WavStopRecord_f();
	}

	CL_Disconnect(qtrue);

	S_Shutdown();
	DL_Shutdown();
	CL_ShutdownRef();

#ifdef FEATURE_IRC_CLIENT
	CL_OW_IRCInitiateShutdown();
#endif

	CL_ShutdownUI();

	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("snd_reload");
	Cmd_RemoveCommand("snd_restart");
	Cmd_RemoveCommand("vid_restart");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("demo");
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("stoprecord");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("localservers");
	Cmd_RemoveCommand("globalservers");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("setenv");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("serverstatus");
	Cmd_RemoveCommand("showip");
	Cmd_RemoveCommand("model");

	// startup-caching system
	Cmd_RemoveCommand("cache_startgather");
	Cmd_RemoveCommand("cache_usedfile");
	Cmd_RemoveCommand("cache_setindex");
	Cmd_RemoveCommand("cache_mapchange");
	Cmd_RemoveCommand("cache_endgather");

	Cmd_RemoveCommand("updatehunkusage");
	Cmd_RemoveCommand("wav_record");
	Cmd_RemoveCommand("wav_stoprecord");

#ifdef FEATURE_IRC_CLIENT
	CL_OW_IRCWaitShutdown();
#endif

	Cvar_Set("cl_running", "0");

	recursive = qfalse;

	memset(&cls, 0, sizeof(cls));

	Com_Printf("-----------------------\n");
}

static void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping)
{
	if (server)
	{
		if (info)
		{
			server->clients = atoi(Info_ValueForKey(info, "clients"));
			Q_strncpyz(server->hostName, Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH);
			server->load = atoi(Info_ValueForKey(info, "serverload"));
			Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH);
			server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
			Q_strncpyz(server->game, Info_ValueForKey(info, "game"), MAX_NAME_LENGTH);
			server->gameType       = atoi(Info_ValueForKey(info, "gametype"));
			server->netType        = atoi(Info_ValueForKey(info, "nettype"));
			server->minPing        = atoi(Info_ValueForKey(info, "minping"));
			server->maxPing        = atoi(Info_ValueForKey(info, "maxping"));
			server->allowAnonymous = atoi(Info_ValueForKey(info, "sv_allowAnonymous"));
			server->friendlyFire   = atoi(Info_ValueForKey(info, "friendlyFire"));
			server->maxlives       = atoi(Info_ValueForKey(info, "maxlives"));
			server->needpass       = atoi(Info_ValueForKey(info, "needpass"));
			server->punkbuster     = atoi(Info_ValueForKey(info, "punkbuster"));
			Q_strncpyz(server->gameName, Info_ValueForKey(info, "gamename"), MAX_NAME_LENGTH);
			server->antilag       = atoi(Info_ValueForKey(info, "g_antilag"));
			server->weaprestrict  = atoi(Info_ValueForKey(info, "weaprestrict"));
			server->balancedteams = atoi(Info_ValueForKey(info, "balancedteams"));
		}
		server->ping = ping;
	}
}

static void CL_SetServerInfoByAddress(netadr_t from, const char *info, int ping)
{
	int i;

	for (i = 0; i < MAX_OTHER_SERVERS; i++)
	{
		if (NET_CompareAdr(from, cls.localServers[i].adr))
		{
			CL_SetServerInfo(&cls.localServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_GLOBAL_SERVERS; i++)
	{
		if (NET_CompareAdr(from, cls.globalServers[i].adr))
		{
			CL_SetServerInfo(&cls.globalServers[i], info, ping);
		}
	}

	for (i = 0; i < MAX_OTHER_SERVERS; i++)
	{
		if (NET_CompareAdr(from, cls.favoriteServers[i].adr))
		{
			CL_SetServerInfo(&cls.favoriteServers[i], info, ping);
		}
	}
}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket(netadr_t from, msg_t *msg)
{
	int  i, type;
	char info[MAX_INFO_STRING];
	char *infoString;
	int  prot;
	char *gameName;
	int  debug_protocol;
	int  protocol = PROTOCOL_VERSION;

	debug_protocol = Cvar_VariableIntegerValue("debug_protocol");
	if (debug_protocol)
	{
		protocol = debug_protocol;
	}

	infoString = MSG_ReadString(msg);

	// if this isn't the correct protocol version, ignore it
	prot = atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != protocol)
	{
		Com_DPrintf("Different protocol info packet: %s\n", infoString);
		return;
	}

	// if this isn't the correct game, ignore it
	gameName = Info_ValueForKey(infoString, "gamename");
	if (!gameName[0] || Q_stricmp(gameName, GAMENAME_STRING))
	{
		Com_DPrintf("Different game info packet: %s\n", infoString);
		return;
	}

	// iterate servers waiting for ping response
	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if (cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr(from, cl_pinglist[i].adr))
		{
			// calc ping time
			cl_pinglist[i].time = cls.realtime - cl_pinglist[i].start + 1;
			Com_DPrintf("ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString(from));

			// save of info
			Q_strncpyz(cl_pinglist[i].info, infoString, sizeof(cl_pinglist[i].info));

			// tack on the net type
			// NOTE: make sure these types are in sync with the netnames strings in the UI
			switch (from.type)
			{
			case NA_BROADCAST:
			case NA_IP:
				type = 1;
				break;
			case NA_IP6:
				type = 2;
				break;

			default:
				type = 0;
				break;
			}
			Info_SetValueForKey(cl_pinglist[i].info, "nettype", va("%d", type));
			CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if (cls.pingUpdateSource != AS_LOCAL)
	{
		return;
	}

	for (i = 0 ; i < MAX_OTHER_SERVERS ; i++)
	{
		// empty slot
		if (cls.localServers[i].adr.port == 0)
		{
			break;
		}

		// avoid duplicate
		if (NET_CompareAdr(from, cls.localServers[i].adr))
		{
			return;
		}
	}

	if (i == MAX_OTHER_SERVERS)
	{
		Com_DPrintf("MAX_OTHER_SERVERS hit, dropping infoResponse\n");
		return;
	}

	// add this to the list
	cls.numlocalservers                = i + 1;
	cls.localServers[i].adr            = from;
	cls.localServers[i].clients        = 0;
	cls.localServers[i].hostName[0]    = '\0';
	cls.localServers[i].load           = -1;
	cls.localServers[i].mapName[0]     = '\0';
	cls.localServers[i].maxClients     = 0;
	cls.localServers[i].maxPing        = 0;
	cls.localServers[i].minPing        = 0;
	cls.localServers[i].ping           = -1;
	cls.localServers[i].game[0]        = '\0';
	cls.localServers[i].gameType       = 0;
	cls.localServers[i].netType        = from.type;
	cls.localServers[i].allowAnonymous = 0;
	cls.localServers[i].friendlyFire   = 0;
	cls.localServers[i].maxlives       = 0;
	cls.localServers[i].needpass       = 0;
	cls.localServers[i].punkbuster     = 0;
	cls.localServers[i].antilag        = 0;
	cls.localServers[i].weaprestrict   = 0;
	cls.localServers[i].balancedteams  = 0;
	cls.localServers[i].gameName[0]    = '\0';

	Q_strncpyz(info, MSG_ReadString(msg), MAX_INFO_STRING);
	if (strlen(info))
	{
		if (info[strlen(info) - 1] != '\n')
		{
			strncat(info, "\n", sizeof(info) - 1);
		}
		Com_Printf("%s: %s", NET_AdrToString(from), info);
	}
}

/*
===================
CL_UpdateInfoPacket
===================
*/
void CL_UpdateInfoPacket(netadr_t from)
{
	if (autoupdate.autoupdateServer.type == NA_BAD)
	{
		Com_DPrintf("CL_UpdateInfoPacket: Update server has bad address\n");
		return;
	}

	Com_DPrintf("Update server resolved to %s\n",
	            NET_AdrToString(autoupdate.autoupdateServer));

	if (!NET_CompareAdr(from, autoupdate.autoupdateServer))
	{
		// TODO: when the updater is server-side as well, write this message to the Attack log
		Com_DPrintf("CL_UpdateInfoPacket: Ignoring packet from %s, because the update server is located at %s\n",
		            NET_AdrToString(from), NET_AdrToString(autoupdate.autoupdateServer));
		return;
	}

	Cvar_Set("com_updateavailable", Cmd_Argv(1));

	if (!Q_stricmp(com_updateavailable->string, "1"))
	{
		Cvar_Set("com_updatefiles", Cmd_Argv(2));
#ifdef FEATURE_AUTOUPDATE
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_AUTOUPDATE);
#endif /* FEATURE_AUTOUPDATE */
	}
}

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t *CL_GetServerStatus(netadr_t from)
{
	int i, oldest, oldestTime;

	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (NET_CompareAdr(from, cl_serverStatusList[i].address))
		{
			return &cl_serverStatusList[i];
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (cl_serverStatusList[i].retrieved)
		{
			return &cl_serverStatusList[i];
		}
	}
	oldest     = -1;
	oldestTime = 0;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime)
		{
			oldest     = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}
	if (oldest != -1)
	{
		return &cl_serverStatusList[oldest];
	}
	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS - 1)];
}

/*
===================
CL_ServerStatus
===================
*/
int CL_ServerStatus(char *serverAddress, char *serverStatusString, int maxLen)
{
	netadr_t       to;
	serverStatus_t *serverStatus;

	// if no server address then reset all server status requests
	if (!serverAddress)
	{
		int i;

		for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
		{
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved    = qtrue;
		}
		return qfalse;
	}
	// get the address
	if (!NET_StringToAdr(serverAddress, &to, NA_UNSPEC))
	{
		return qfalse;
	}
	serverStatus = CL_GetServerStatus(to);
	// if no server status string then reset the server status request for this address
	if (!serverStatusString)
	{
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	// if this server status request has the same address
	if (NET_CompareAdr(to, serverStatus->address))
	{
		// if we recieved an response for this server status request
		if (!serverStatus->pending)
		{
			Q_strncpyz(serverStatusString, serverStatus->string, maxLen);
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		// resend the request regularly
		else if (serverStatus->startTime < Sys_Milliseconds() - cl_serverStatusResendTime->integer)
		{
			serverStatus->print     = qfalse;
			serverStatus->pending   = qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time      = 0;
			serverStatus->startTime = Sys_Milliseconds();
			NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
			return qfalse;
		}
	}
	// if retrieved
	else if (serverStatus->retrieved)
	{
		serverStatus->address   = to;
		serverStatus->print     = qfalse;
		serverStatus->pending   = qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = Sys_Milliseconds();
		serverStatus->time      = 0;
		NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");
		return qfalse;
	}
	return qfalse;
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse(netadr_t from, msg_t *msg)
{
	char           *s;
	char           info[MAX_INFO_STRING];
	int            i, l, score, ping;
	int            len;
	serverStatus_t *serverStatus;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++)
	{
		if (NET_CompareAdr(from, cl_serverStatusList[i].address))
		{
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}
	// if we didn't request this server status
	if (!serverStatus)
	{
		return;
	}

	s = MSG_ReadStringLine(msg);

	len = 0;
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "%s", s);

	if (serverStatus->print)
	{
		Com_Printf("Server settings:\n");
		// print cvars
		while (*s)
		{
			for (i = 0; i < 2 && *s; i++)
			{
				if (*s == '\\')
				{
					s++;
				}
				l = 0;
				while (*s)
				{
					info[l++] = *s;
					if (l >= MAX_INFO_STRING - 1)
					{
						break;
					}
					s++;
					if (*s == '\\')
					{
						break;
					}
				}
				info[l] = '\0';
				if (i)
				{
					Com_Printf("%s\n", info);
				}
				else
				{
					Com_Printf("%-24s", info);
				}
			}
		}
	}

	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	if (serverStatus->print)
	{
		Com_Printf("\nPlayers:\n");
		Com_Printf("num: score: ping: name:\n");
	}
	for (i = 0, s = MSG_ReadStringLine(msg); *s; s = MSG_ReadStringLine(msg), i++)
	{

		len = strlen(serverStatus->string);
		Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\%s", s);

		if (serverStatus->print)
		{
			score = ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if (s)
			{
				s = strchr(s + 1, ' ');
			}
			if (s)
			{
				s++;
			}
			else
			{
				s = "unknown";
			}
			Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s);
		}
	}
	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

	serverStatus->time    = Sys_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = qfalse;
	if (serverStatus->print)
	{
		serverStatus->retrieved = qtrue;
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f(void)
{
	char     *message;
	int      i, j;
	netadr_t to;

	Com_Printf("Scanning for servers on the local network...\n");

	// reset the list, waiting for response
	cls.numlocalservers  = 0;
	cls.pingUpdateSource = AS_LOCAL;

	for (i = 0; i < MAX_OTHER_SERVERS; i++)
	{
		qboolean b = cls.localServers[i].visible;
		Com_Memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
		cls.localServers[i].visible = b;
	}
	Com_Memset(&to, 0, sizeof(to));

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for (i = 0 ; i < 2 ; i++)
	{
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for (j = 0 ; j < NUM_SERVER_PORTS ; j++)
		{
			to.port = BigShort(( short ) (PORT_SERVER + j));

			to.type = NA_BROADCAST;
			NET_SendPacket(NS_CLIENT, strlen(message), message, to);

#ifdef FEATURE_IPV6
			if (Cvar_VariableIntegerValue("net_enabled") & NET_ENABLEV6)
			{
				to.type = NA_MULTICAST6;
				NET_SendPacket(NS_CLIENT, strlen(message), message, to);
			}
#endif
		}
	}
}

/*
==================
CL_GlobalServers_f

FIXME/TODO: ADD MULTIPLE MASTER SUPPORT
==================
*/
void CL_GlobalServers_f(void)
{
	netadr_t to;
	int      i;
	int      count;
	char     command[1024];

	if ((count = Cmd_Argc()) < 3 || (cls.masterNum = atoi(Cmd_Argv(1))) < 0 || cls.masterNum > 1)
	{
		Com_Printf("usage: globalservers <master# 0-1> <protocol> [keywords]\n");
		return;
	}

	// reset the list, waiting for response
	// -1 is used to distinguish a "no response"

	i = NET_StringToAdr(MASTER_SERVER_NAME, &to, NA_UNSPEC);

	if (!i)
	{
		Com_Printf("CL_GlobalServers_f: Error: could not resolve address of master %s\n", MASTER_SERVER_NAME);
		return;
	}
	else if (i == 2)
	{
		to.port = BigShort(PORT_MASTER);
	}

	// FIXME @IP6 NET_AdrToString doesn't deal with port
	Com_Printf("Requesting servers from the master %s (%s)...\n", MASTER_SERVER_NAME, NET_AdrToString(to));

	cls.numglobalservers = -1;
	cls.pingUpdateSource = AS_GLOBAL;

#ifdef FEATURE_IPV6
	// Use the extended query for IPv6 masters
	if (to.type == NA_IP6 || to.type == NA_MULTICAST6)
	{
		Com_sprintf(command, sizeof(command), "getserversExt %s %s", GAMENAME_FOR_MASTER, Cmd_Argv(2));

		// TODO: test if we only have an IPv6 connection. If it's the case,
		//       request IPv6 servers only by appending " ipv6" to the command
	}
	else
#endif
	{
		Com_sprintf(command, sizeof(command), "getservers %s", Cmd_Argv(2));
	}

	// tack on keywords
	for (i = 3; i < count; i++)
	{
		Q_strcat(command, sizeof(command), " ");
		Q_strcat(command, sizeof(command), Cmd_Argv(i));
	}

	NET_OutOfBandPrint(NS_SERVER, to, command);
}

/*
==================
CL_GetPing
==================
*/
void CL_GetPing(int n, char *buf, int buflen, int *pingtime)
{
	const char *str;
	int        time;

	if (n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
	{
		// empty slot
		buf[0]    = '\0';
		*pingtime = 0;
		return;
	}

	str = NET_AdrToString(cl_pinglist[n].adr);
	Q_strncpyz(buf, str, buflen);

	time = cl_pinglist[n].time;
	if (!time)
	{
		int maxPing = Cvar_VariableIntegerValue("cl_maxPing");

		// check for timeout
		time = cls.realtime - cl_pinglist[n].start;

		if (maxPing < 100)
		{
			maxPing = 100;
		}
		if (time < maxPing)
		{
			// not timed out yet
			time = 0;
		}
	}

	CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

	*pingtime = time;
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo(int n, char *buf, int buflen)
{
	if (n < 0 || n >= MAX_PINGREQUESTS || !cl_pinglist[n].adr.port)
	{
		// empty slot
		if (buflen)
		{
			buf[0] = '\0';
		}
		return;
	}

	Q_strncpyz(buf, cl_pinglist[n].info, buflen);
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing(int n)
{
	if (n < 0 || n >= MAX_PINGREQUESTS)
	{
		return;
	}

	cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int CL_GetPingQueueCount(void)
{
	int    i;
	int    count    = 0;
	ping_t *pingptr = cl_pinglist;

	for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		if (pingptr->adr.port)
		{
			count++;
		}
	}

	return count;
}

/*
==================
CL_GetFreePing
==================
*/
ping_t *CL_GetFreePing(void)
{
	ping_t *pingptr = cl_pinglist;
	ping_t *best;
	int    oldest;
	int    i;
	int    time;

	for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// find free ping slot
		if (pingptr->adr.port)
		{
			if (!pingptr->time)
			{
				if (cls.realtime - pingptr->start < 500)
				{
					// still waiting for response
					continue;
				}
			}
			else if (pingptr->time < 500)
			{
				// results have not been queried
				continue;
			}
		}

		// clear it
		pingptr->adr.port = 0;
		return pingptr;
	}

	// use oldest entry
	pingptr = cl_pinglist;
	best    = cl_pinglist;
	oldest  = INT_MIN;
	for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++)
	{
		// scan for oldest
		time = cls.realtime - pingptr->start;
		if (time > oldest)
		{
			oldest = time;
			best   = pingptr;
		}
	}

	return best;
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f(void)
{
	netadr_t     to;
	ping_t       *pingptr;
	char         *server;
	int          argc;
	netadrtype_t family = NA_UNSPEC;

	argc = Cmd_Argc();

	if (argc != 2 && argc != 3)
	{
		Com_Printf("usage: ping [-4|-6] server\n");
		return;
	}

	if (argc == 2)
	{
		server = Cmd_Argv(1);
	}
	else
	{
		if (!strcmp(Cmd_Argv(1), "-4"))
		{
			family = NA_IP;
		}
		else if (!strcmp(Cmd_Argv(1), "-6"))
		{
			family = NA_IP6;
		}
		else
		{
			Com_Printf("warning: only -4 or -6 as address type understood.\n");
		}

		server = Cmd_Argv(2);
	}

	Com_Memset(&to, 0, sizeof(netadr_t));

	if (!NET_StringToAdr(server, &to, family))
	{
		return;
	}

	pingptr = CL_GetFreePing();

	memcpy(&pingptr->adr, &to, sizeof(netadr_t));
	pingptr->start = cls.realtime;
	pingptr->time  = 0;

	CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

	NET_OutOfBandPrint(NS_CLIENT, to, "getinfo xxx");
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
qboolean CL_UpdateVisiblePings_f(int source)
{
	int      slots, i;
	char     buff[MAX_STRING_CHARS];
	int      pingTime;
	int      max;
	qboolean status = qfalse;

	if (source < 0 || source > AS_FAVORITES)
	{
		return qfalse;
	}

	cls.pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if (slots < MAX_PINGREQUESTS)
	{
		serverInfo_t *server = NULL;

		switch (source)
		{
		case AS_LOCAL:
			server = &cls.localServers[0];
			max    = cls.numlocalservers;
			break;
		case AS_GLOBAL:
			server = &cls.globalServers[0];
			max    = cls.numglobalservers;
			break;
		case AS_FAVORITES:
			server = &cls.favoriteServers[0];
			max    = cls.numfavoriteservers;
			break;
		}

		for (i = 0; i < max; i++)
		{
			if (server[i].visible)
			{
				if (server[i].ping == -1)
				{
					int j;

					if (slots >= MAX_PINGREQUESTS)
					{
						break;
					}

					for (j = 0; j < MAX_PINGREQUESTS; j++)
					{
						if (!cl_pinglist[j].adr.port)
						{
							continue;
						}
						if (NET_CompareAdr(cl_pinglist[j].adr, server[i].adr))
						{
							// already on the list
							break;
						}
					}

					if (j >= MAX_PINGREQUESTS)
					{
						status = qtrue;
						for (j = 0; j < MAX_PINGREQUESTS; j++)
						{
							if (!cl_pinglist[j].adr.port)
							{
								break;
							}
						}
						memcpy(&cl_pinglist[j].adr, &server[i].adr, sizeof(netadr_t));
						cl_pinglist[j].start = cls.realtime;
						cl_pinglist[j].time  = 0;
						NET_OutOfBandPrint(NS_CLIENT, cl_pinglist[j].adr, "getinfo xxx");
						slots++;
					}
				}
				// if the server has a ping higher than cl_maxPing or
				// the ping packet got lost
				else if (server[i].ping == 0)
				{
					// if we are updating global servers
					if (source == AS_GLOBAL)
					{
						//
						if (cls.numGlobalServerAddresses > 0)
						{
							// overwrite this server with one from the additional global servers
							cls.numGlobalServerAddresses--;
							CL_InitServerInfo(&server[i], &cls.globalServerAddresses[cls.numGlobalServerAddresses]);
							// NOTE: the server[i].visible flag stays untouched
						}
					}
				}
			}
		}
	}

	if (slots)
	{
		status = qtrue;
	}
	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		if (!cl_pinglist[i].adr.port)
		{
			continue;
		}
		CL_GetPing(i, buff, MAX_STRING_CHARS, &pingTime);
		if (pingTime != 0)
		{
			CL_ClearPing(i);
			status = qtrue;
		}
	}

	return status;
}

/*
==================
CL_ServerStatus_f
==================
*/
void CL_ServerStatus_f(void)
{
	netadr_t       to, *toptr = NULL;
	char           *server;
	serverStatus_t *serverStatus;
	int            argc;

	argc = Cmd_Argc();

	if (argc != 2 && argc != 3)
	{
		if (cls.state != CA_ACTIVE || clc.demoplaying)
		{
			Com_Printf("Not connected to a server.\nusage: serverstatus [-4|-6] server\n");
			return;
		}

		toptr = &clc.serverAddress;
	}

	if (!toptr)
	{
		netadrtype_t family = NA_UNSPEC;

		Com_Memset(&to, 0, sizeof(netadr_t));

		if (argc == 2)
		{
			server = Cmd_Argv(1);
		}
		else
		{
			if (!strcmp(Cmd_Argv(1), "-4"))
			{
				family = NA_IP;
			}
			else if (!strcmp(Cmd_Argv(1), "-6"))
			{
				family = NA_IP6;
			}
			else
			{
				Com_Printf("warning: only -4 or -6 as address type understood.\n");
			}

			server = Cmd_Argv(2);
		}

		toptr = &to;
		if (!NET_StringToAdr(server, toptr, family))
		{
			return;
		}
	}

	NET_OutOfBandPrint(NS_CLIENT, *toptr, "getstatus");

	serverStatus          = CL_GetServerStatus(*toptr);
	serverStatus->address = *toptr;
	serverStatus->print   = qtrue;
	serverStatus->pending = qtrue;
}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void)
{
	Sys_ShowIP();
}

/*
=======================
CL_AddToLimboChat
=======================
*/
void CL_AddToLimboChat(const char *str)
{
	int  len = 0;
	char *p;
	int  i;

	cl.limboChatPos = LIMBOCHAT_HEIGHT - 1;

	// copy old strings
	for (i = cl.limboChatPos; i > 0; i--)
	{
		strcpy(cl.limboChatMsgs[i], cl.limboChatMsgs[i - 1]);
	}

	// copy new string
	p  = cl.limboChatMsgs[0];
	*p = 0;

	while (*str)
	{
		if (len > LIMBOCHAT_WIDTH - 1)
		{
			break;
		}

		if (Q_IsColorString(str))
		{
			*p++ = *str++;
			*p++ = *str++;
			continue;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;
}

/*
=======================
CL_GetLimboString
=======================
*/
qboolean CL_GetLimboString(int index, char *buf)
{
	if (index >= LIMBOCHAT_HEIGHT)
	{
		return qfalse;
	}

	strncpy(buf, cl.limboChatMsgs[index], 140);
	return qtrue;
}

/**
 * @brief Called by mod libs to handle translation.
 *
 * @param[in] string to be translated
 * @param[out] dest_buffer translated text
 * @note for client translations use _(x) macro instead.
 */
void CL_TranslateString(const char *string, char *dest_buffer)
{
	Com_sprintf(dest_buffer, MAX_STRING_CHARS, "%s", __(string));
}

/**
 * @brief Stores in a static buf, converts \n to chr(13)
 * @todo Replace / remove.
 */
const char *CL_TranslateStringBuf(const char *string)
{
	char        *p;
	int         i, l;
	static char buf[MAX_VA_STRING];

	CL_TranslateString(string, buf);
	while ((p = strstr(buf, "\\n")) != NULL)
	{
		*p = '\n';
		p++;

		l = strlen(p);
		for (i = 0; i < l; i++)
		{
			*p = *(p + 1);
			p++;
		}
	}
	return buf;
}

/*
=======================
CL_OpenURL
=======================
*/
void CL_OpenURL(const char *url)
{
	if (!url || !strlen(url))
	{
		Com_Printf("%s", CL_TranslateStringBuf("invalid/empty URL\n"));
		return;
	}
	Sys_OpenURL(url, qfalse);
}

void BotImport_DrawPolygon(int color, int numpoints, float *points)
{
	re.DrawDebugPolygon(color, numpoints, points);
}
