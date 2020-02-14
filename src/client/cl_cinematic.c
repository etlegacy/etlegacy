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
 * @file cl_cinematic.c
 * @brief Video and cinematic playback
 */

#include "client.h"

typedef struct videoDecode_s
{
	char fileExt[10];
	char fileExt2[10];

	// Init and Shutdown are not mandatory
	void (*Init)(void);
	void (*Shutdown)(void);

	qboolean (*Start)(cinematic_t *cin);
	void (*Update)(cinematic_t *cin, int time);
	void (*Reset)(cinematic_t *cin);
	void (*Stop)(cinematic_t *cin);
} videoDecode_t;

typedef enum
{
	VIDEO_NONE = 0,
	VIDEO_ROQ,
#ifdef FEATURE_THEORA
	VIDEO_OGV,
#endif
	//VIDEO_H264,

	VIDEO_NUM_CODECS
} cinType_t;

static videoDecode_t videoDecoders[] =
{
	{ "\0",  "\0",  NULL,     NULL, NULL,          NULL,                NULL,      NULL          },
	{ "roq", "\0",  ROQ_Init, NULL, ROQ_StartRead,  ROQ_UpdateCinematic,  ROQ_Reset, ROQ_StopVideo },
#ifdef FEATURE_THEORA
	{ "ogv", "ogm", NULL,     NULL, OGV_StartRead,  OGV_UpdateCinematic,  NULL,      OGV_StopVideo },
#endif
	//{ "h264", "\0", NULL,     NULL, H264_StartRead, H264_UpdateCinematic, NULL,      H264_StopVideo },
};

// ============================================================================

static cinematic_t cin_cinematics[MAX_CINEMATICS];

/**
 * @brief Finds a free cinHandle_t
 * @param[out] handle
 * @return
 */
static cinematic_t *CIN_HandleForCinematic(cinHandle_t *handle)
{
	cinematic_t *cin;
	int         i;

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
	{
		if (!cin->playing)
		{
			break;
		}
	}

	if (i == MAX_CINEMATICS)
	{
		Com_Error(ERR_DROP, "CIN_HandleForCinematic: none free");
	}

	*handle = i + 1;

	return cin;
}

/**
 * @brief CIN_HandleValid
 * @param[in] handle
 * @return
 */
static qboolean CIN_HandleValid(cinHandle_t handle)
{
	if (handle <= 0 || handle > MAX_CINEMATICS)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CIN_GetCinematicByHandle
 * @param[in] handle
 * @return A cinematic_t for the given cinHandle_t
 */
cinematic_t *CIN_GetCinematicByHandle(cinHandle_t handle)
{
	cinematic_t *cin;

	if (!CIN_HandleValid(handle))
	{
		Com_Error(ERR_DROP, "CIN_GetCinematicByHandle: handle out of range");
	}

	cin = &cin_cinematics[handle - 1];

	if (!cin->playing)
	{
		Com_Error(ERR_DROP, "CIN_GetCinematicByHandle: invalid handle");
	}

	return cin;
}

/**
 * @brief CIN_FreeCinematic
 * @param[in,out] cin
 * @param[in] runtime
 */
static void CIN_FreeCinematic(cinematic_t *cin, qboolean runtime)
{
	// Stop the cinematic
	if (runtime && (cin->flags & CIN_system))
	{
		Com_DPrintf("Stopped cinematic %s\n", cin->name);

		// Make sure sounds aren't playing
		S_StopAllSounds();
	}

	/*
	if (!cin->playing)
	{
	    return;
	}
	*/

	if (videoDecoders[cin->videoType].Stop)
	{
		videoDecoders[cin->videoType].Stop(cin);
	}

	// Free the frame buffers
	if (cin->frameBuffer[0])
	{
		Com_Dealloc(cin->frameBuffer[0]);
	}

	if (cin->frameBuffer[1])
	{
		Com_Dealloc(cin->frameBuffer[1]);
	}

	// Close the file
	if (cin->file)
	{
		FS_FCloseFile(cin->file);
	}

	Com_Memset(cin, 0, sizeof(cinematic_t));
}

/**
 * @brief CIN_PlayCinematic_f
 */
static void CIN_PlayCinematic_f(void)
{
	char name[MAX_OSPATH];
	char *vidName;
	char *vidArg;
	int  bits = CIN_system;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: playCinematic <name>\n");
		return;
	}

	// don't allow this while on server
	if (cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE)
	{
		return;
	}

	Com_DPrintf("CIN_PlayCinematic_f\n");
	if (cls.state == CA_CINEMATIC)
	{
		SCR_StopCinematic();
	}

	vidName = Cmd_Argv(1);
	vidArg  = Cmd_Argv(2);

	Com_sprintf(name, sizeof(name), "video/%s", vidName);
	COM_DefaultExtension(name, sizeof(name), ".roq");

	if ((vidArg && vidArg[0] == '1') || Q_stricmp(vidName, "demoend.roq") == 0 || Q_stricmp(vidName, "end.roq") == 0)
	{
		bits |= CIN_hold;
	}
	if (vidArg && vidArg[0] == '2')
	{
		bits |= CIN_loop;
	}

	// If running a local server, kill it
	SV_Shutdown("Server quit");

	// If connected to a server, disconnect
	CL_Disconnect(qtrue);

	// Play the cinematic
	cls.cinematicHandle = CIN_PlayCinematic(name, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits);
}

/**
 * @brief CIN_ListCinematics_f
 */
static void CIN_ListCinematics_f(void)
{
	cinematic_t *cin;
	int         count = 0, bytes = 0;
	int         i;

	Com_Printf("\n");
	Com_Printf("    -w-- -h-- -size- fps -name-----------\n");

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
	{
		if (!cin->playing)
		{
			continue;
		}

		count++;
		bytes += cin->frameWidth * cin->frameHeight * 8;

		Com_Printf("%2i: ", i);

		Com_Printf("%4i %4i ", cin->frameWidth, cin->frameHeight);

		Com_Printf("%5ik ", SIZE_KB(cin->frameWidth * cin->frameHeight * 8));

		Com_Printf("%3i ", cin->frameRate);

		Com_Printf("%s\n", cin->name);
	}

	Com_Printf("-----------------------------------------\n");
	Com_Printf("%i total cinematics\n", count);
	Com_Printf("%.2f MB of cinematic data\n", (double)(SIZE_MB_FLOAT(bytes)));
	Com_Printf("\n");
}

/**
 * @brief CIN_Completion_VideoName
 * @param args - unused
 * @param[in] argNum
 */
static void CIN_Completion_VideoName(char *args, int argNum)
{
	if (argNum == 2)
	{
#ifdef FEATURE_THEORA
		const char *extensions[] = { "roq", "ogv", "ogm" };
		Field_CompleteFilenameMultiple("video", 3, extensions, qtrue);
#else
		Field_CompleteFilename("video", "roq", qtrue, qtrue);
#endif
	}
}

/**
 * @brief CIN_PlayCinematic
 * @param[in] name
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] flags
 * @return
 */
cinHandle_t CIN_PlayCinematic(const char *name, int x, int y, int w, int h, int flags)
{
	cinematic_t  *cin;
	cinHandle_t  handle;
	fileHandle_t file;
	int          size;
	int          i;
	const char   *fileExt;

	fileExt = COM_GetExtension(name);

	// See if already playing
	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
	{
		if (!cin->playing)
		{
			continue;
		}

		if (!Q_stricmp(cin->name, name))
		{
			if (cin->flags != flags)
			{
				continue;
			}

			return i + 1;
		}
	}

	// Open the file
	size = FS_FOpenFileRead(name, &file, qtrue);

	if (size <= 0)
	{
		Com_Printf("Warning: Cinematic '%s' file not found or can't be read\n", name);
		return -1;
	}


	if (!file)
	{
		if (flags & CIN_system)
		{
			Com_Printf("Warning: Cinematic %s file not found\n", name);
		}

		return -1;
	}

	if (flags & CIN_system)
	{
		Com_Printf("Cinematic %s found\n", name);
	}

	// Play the cinematic
	cin = CIN_HandleForCinematic(&handle);

	// Fill it in
	cin->playing = qtrue;
	Q_strncpyz(cin->name, name, MAX_OSPATH);
	cin->flags          = flags;
	cin->file           = file;
	cin->size           = size;
	cin->startTime      = 0;
	cin->frameWidth     = 0;
	cin->frameHeight    = 0;
	cin->frameCount     = 0;
	cin->frameBuffer[0] = NULL;
	cin->frameBuffer[1] = NULL;
	cin->rectangle.x    = x;
	cin->rectangle.y    = y;
	cin->rectangle.w    = w;
	cin->rectangle.h    = h;

	for (i = 0; i < VIDEO_NUM_CODECS; i++)
	{
		if (!videoDecoders[i].fileExt[0])
		{
			continue;
		}

		if (COM_CompareExtension(fileExt, videoDecoders[i].fileExt) ||
		    (videoDecoders[i].fileExt2[0] && COM_CompareExtension(fileExt, videoDecoders[i].fileExt2)))
		{
			if (!videoDecoders[i].Start)
			{
				Com_Error(ERR_FATAL, "Cinematic %s cannot be run, there is no start method defined", name);
			}

			cin->videoType = i;

			if (!videoDecoders[i].Start(cin))
			{
				if (flags & CIN_system)
				{
					Com_Printf("Warning: Cinematic %s is not a valid %s file\n", name, videoDecoders[i].fileExt);
				}

				goto video_playback_failed;
			}
			goto codec_found_valid;
		}
	}
	Com_Printf("Could not find a codec for the video: %s\n", name);
video_playback_failed:
	CIN_FreeCinematic(cin, qfalse);
	return -1;

codec_found_valid:

	if (flags & CIN_system)
	{
		Com_Printf("Playing cinematic %s\n", name);

		cls.state = CA_CINEMATIC;

		// Force console and GUI off
		if (uivm)
		{
			VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE);
		}

		Con_Close();
	}

	// Make sure sounds aren't playing
	S_StopAllSounds();

	return handle;
}

/**
 * @brief CIN_ResetCinematic
 * @param[in] handle
 *
 * @note Unused
 */
void CIN_ResetCinematic(cinHandle_t handle)
{
	cinematic_t *cin;

	if (!CIN_HandleValid(handle))
	{
		return;
	}

	cin = CIN_GetCinematicByHandle(handle);

	// Reset the cinematic
	if (videoDecoders[cin->videoType].Reset)
	{
		videoDecoders[cin->videoType].Reset(cin);
	}
	else
	{
		Com_Printf("Warning: Cinematic decoder %s missing reset functionality\n", videoDecoders[cin->videoType].fileExt);
	}

	cin->startTime  = 0;
	cin->frameCount = 0;
}

/**
 * @brief CIN_StopCinematic
 * @param[in] handle
 * @return
 */
e_status CIN_StopCinematic(cinHandle_t handle)
{
	cinematic_t *cin;

	if (!CIN_HandleValid(handle))
	{
		return FMV_EOF;
	}

	cin = CIN_GetCinematicByHandle(handle);

	if (cin->flags & CIN_system)
	{
		const char *s;

		cls.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		s = Cvar_VariableString("nextmap");
		if (s[0])
		{
			Cbuf_ExecuteText(EXEC_APPEND, va("%s\n", s));
			Cvar_Set("nextmap", "");
		}
		cls.cinematicHandle = -1;
	}

	CIN_FreeCinematic(cin, qtrue);

	return FMV_EOF;
}

/**
 * @brief CIN_Init
 */
void CIN_Init(void)
{
	int i;

	// Add commands
	Cmd_AddCommand("cinematic", CIN_PlayCinematic_f, "Plays a cinematic.", CIN_Completion_VideoName);
	Cmd_AddCommand("listCinematics", CIN_ListCinematics_f, "Lists playing cinematics.", NULL);

	Com_Memset(cin_cinematics, 0, sizeof(cin_cinematics));

	for (i = 0; i < VIDEO_NUM_CODECS; i++)
	{
		if (videoDecoders[i].Init)
		{
			videoDecoders[i].Init();
		}
	}
}

/**
 * @brief CIN_Shutdown
 */
void CIN_Shutdown(void)
{
	int i;

	// Remove commands
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("listCinematics");

	CIN_CloseAllVideos();

	for (i = 0; i < VIDEO_NUM_CODECS; i++)
	{
		if (videoDecoders[i].Shutdown)
		{
			videoDecoders[i].Shutdown();
		}
	}
}

/**
 * @brief Stop all the cinematics
 */
void CIN_CloseAllVideos(void)
{
	cinematic_t *cin;
	int         i;

	cls.cinematicHandle = -1;

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
	{
		CIN_FreeCinematic(cin, qtrue);
	}

	// Clear cinematics list
	Com_Memset(cin_cinematics, 0, sizeof(cin_cinematics));
}

/**
 * @brief CIN_RunCinematic
 * @param[in] handle
 * @return
 */
e_status CIN_RunCinematic(int handle)
{
	cinematic_t *data;

	if (!CIN_HandleValid(handle))
	{
		return FMV_EOF;
	}

	data = CIN_GetCinematicByHandle(handle);

	if (videoDecoders[data->videoType].Update)
	{
		videoDecoders[data->videoType].Update(data, cls.realtime);
	}
	else
	{
		Com_Error(ERR_FATAL, "Cinematic decoder %s is missing the update function",
		          videoDecoders[data->videoType].fileExt);
	}

	if (!data->currentData.image)
	{
		CIN_StopCinematic(handle);

		return FMV_EOF;
	}

	return FMV_PLAY;
}

/**
 * @brief CIN_SetExtents
 * @param[in] handle
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 */
void CIN_SetExtents(int handle, int x, int y, int w, int h)
{
	if (CIN_HandleValid(handle))
	{
		cinematic_t *cin;

		cin = CIN_GetCinematicByHandle(handle);
		cin->rectangle.x = x;
		cin->rectangle.y = y;
		cin->rectangle.w = w;
		cin->rectangle.h = h;
	}
}

/**
 * @brief SCR_StopCinematic
 */
void SCR_StopCinematic(void)
{
	if (CIN_HandleValid(cls.cinematicHandle))
	{
		CIN_StopCinematic(cls.cinematicHandle);
		cls.cinematicHandle = -1;
	}
}

/**
 * @brief SCR_RunCinematic
 */
void SCR_RunCinematic(void)
{
	if (CIN_HandleValid(cls.cinematicHandle))
	{
		CIN_RunCinematic(cls.cinematicHandle);
	}
}

/**
 * @brief SCR_DrawCinematic
 */
void SCR_DrawCinematic(void)
{
	if (CIN_HandleValid(cls.cinematicHandle))
	{
		CIN_DrawCinematic(cls.cinematicHandle);
	}
}

/**
 * @brief CIN_DrawCinematic
 * @param[in] handle
 */
void CIN_DrawCinematic(int handle)
{
	float       x, y, w, h;
	cinematic_t *cin;

	if (!CIN_HandleValid(handle))
	{
		return;
	}

	cin = CIN_GetCinematicByHandle(handle);

	x = cin->rectangle.x;
	y = cin->rectangle.y;
	w = cin->rectangle.w;
	h = cin->rectangle.h;
	SCR_AdjustFrom640(&x, &y, &w, &h);

	if (!cin->currentData.image)
	{
		return;
	}

	re.DrawStretchRaw(x, y, w, h, cin->currentData.width, cin->currentData.height, cin->currentData.image, handle,
	                  cin->currentData.dirty);
}

/**
 * @brief CIN_UploadCinematic
 * @param handle
 */
void CIN_UploadCinematic(int handle)
{
	cinematic_t *cin;

	if (!CIN_HandleValid(handle))
	{
		Com_Printf("CIN_UploadCinematic Warning: invalid handle\n");
		return;
	}

	cin = CIN_GetCinematicByHandle(handle);

	if (!cin->currentData.image || !cin->currentData.image[0])
	{
		return;
	}

	re.UploadCinematic(256, 256, 256, 256, cin->currentData.image, handle, cin->currentData.dirty);
}
