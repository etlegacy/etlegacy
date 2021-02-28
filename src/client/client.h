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
 * @file client.h
 * @brief Primary header for client
 */

#ifndef INCLUDE_CLIENT_H
#define INCLUDE_CLIENT_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"
#include "../ui/ui_public.h"
#include "keys.h"
#include "snd_public.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"
#ifdef FEATURE_IRC_CLIENT
	#include "../irc/irc_client.h"
#endif

#define RETRANSMIT_TIMEOUT  3000    ///< time between connection packet retransmits

#define LIMBOCHAT_WIDTH     140     ///< NOTE: buffer size indicator, not related to screen bbox
#define LIMBOCHAT_HEIGHT    7

#define ETKEY_FILE "etkey"
#define ETKEY_SIZE 28

/**
 * @struct clSnapshot_t
 * @brief Snapshots are a view of the server at a given time
 */
typedef struct
{
	qboolean valid;                     ///< cleared if delta parsing was invalid
	int snapFlags;                      ///< rate delayed and dropped commands

	int serverTime;                     ///< server time the message is valid for (in msec)

	int messageNum;                     ///< copied from netchan->incoming_sequence
	int deltaNum;                       ///< messageNum the delta is from
	int ping;                           ///< time from when cmdNum-1 was sent to time packet was reeceived
	byte areamask[MAX_MAP_AREA_BYTES];  ///< portalarea visibility bits

	int cmdNum;                         ///< the next cmdNum the server is expecting
	playerState_t ps;                   ///< complete information about the current player at this time

	int numEntities;                    ///< all of the entities that need to be presented
	int parseEntitiesNum;               ///< at the time of this snapshot

	int serverCommandNum;               ///< execute all commands up to this before
	                                    ///< making the snapshot current
} clSnapshot_t;

/**
 * @struct clSnapshot_t
 * @brief For double tapping
 */
typedef struct
{
	int pressedTime[DT_NUM];
	int releasedTime[DT_NUM];

	int lastdoubleTap;
} doubleTap_t;

/**
 * @struct outPacket_t
 * @brief
 */
typedef struct
{
	int p_cmdNumber;            ///< cl.cmdNumber when packet was sent
	int p_serverTime;           ///< usercmd->serverTime when packet was sent
	int p_realtime;             ///< cls.realtime when packet was sent
} outPacket_t;

/// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
/// entities, so that when a delta compressed message arives from the server
/// it can be un-deltad from the original
#define MAX_PARSE_ENTITIES  2048

extern int g_console_field_width;

/**
 * @struct clientActive_t
 * @brief The clientActive_t structure is wiped completely at every
 * new gamestate_t, potentially several times during an established connection
 */
typedef struct
{
	int timeoutcount;                       ///< it requres several frames in a timeout condition
	                                        ///< to disconnect, preventing debugging breaks from
	                                        ///< causing immediate disconnects on continue
	clSnapshot_t snap;                      ///< latest received from server

	int serverTime;                         ///< may be paused during play
	int oldServerTime;                      ///< to prevent time from flowing bakcwards
	int oldFrameServerTime;                 ///< to check tournament restarts
	int serverTimeDelta;                    ///< cl.serverTime = cls.realtime + cl.serverTimeDelta
	                                        ///< this value changes as net lag varies
	qboolean extrapolatedSnapshot;          ///< set if any cgame frame has been forced to extrapolate
	                                        ///< cleared when CL_AdjustTimeDelta looks at it
	qboolean newSnapshots;                  ///< set on parse of any valid packet

	gameState_t gameState;                  ///< configstrings
	char mapname[MAX_QPATH];                ///< extracted from CS_SERVERINFO

	int parseEntitiesNum;                   ///< index (not anded off) into cl_parse_entities[]

	int mouseDx[2], mouseDy[2];             ///< added to by mouse events
	int mouseIndex;
	int joystickAxis[MAX_JOYSTICK_AXIS];    ///< set by joystick events

	// cgame communicates a few values to the client system
	int cgameUserCmdValue;                  ///< current weapon to add to usercmd_t
	int cgameFlags;                         ///< flags that can be set by the gamecode
	float cgameSensitivity;
	int cgameMpIdentClient;
	vec3_t cgameClientLerpOrigin;

	/// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	/// properly generated command
	usercmd_t cmds[CMD_BACKUP];             ///< each mesage will send several old cmds
	int cmdNumber;                          ///< incremented each frame, because multiple
	                                        ///< frames may need to be packed into a single packet

	/// double tapping
	doubleTap_t doubleTap;

	outPacket_t outPackets[PACKET_BACKUP];  ///< information about each packet we have sent out

	/// the client maintains its own idea of view angles, which are
	/// sent to the server each frame.  It is cleared to 0 upon entering each level.
	/// the server sends a delta each frame which is added to the locally
	/// tracked view angles to account for standing on rotating objects,
	/// and teleport direction changes
	vec3_t viewangles;

	int serverId;                           ///< included in each client message so the server
	                                        ///< can tell if it is for a prior map_restart
	                                        ///< big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t snapshots[PACKET_BACKUP];

	entityState_t entityBaselines[MAX_GENTITIES];   ///< for delta compression when not in previous frame

	entityState_t parseEntities[MAX_PARSE_ENTITIES];

	// NOTE - UI uses LIMBOCHAT_WIDTH strings (140),
	// but for the processing in CL_AddToLimboChat we need some safe room
	char limboChatMsgs[LIMBOCHAT_HEIGHT][LIMBOCHAT_WIDTH * 3 + 1];
	int limboChatPos;

	qboolean corruptedTranslationFile;
	char translationVersion[MAX_STRING_TOKENS];

} clientActive_t;

extern clientActive_t cl;

//==================================================================

/**
 * @struct clientConnection_t
 * @brief The clientConnection_t structure is wiped when disconnecting from a server,
 * either to go to a full screen console, play a demo, or connect to a different server
 *
 * A connection can be to either a server through the network layer or a
 * demo through a file.
 */
typedef struct
{
	connstate_t state;                          ///< connection status

	int clientNum;
	int lastPacketSentTime;                     ///< for retransmits during connection
	int lastPacketTime;                         ///< for timeouts

	netadr_t serverAddress;
	int connectTime;                            ///< for connection retransmits
	int connectPacketCount;                     ///< for display on connection dialog
	char serverMessage[MAX_STRING_TOKENS];      ///< for display on connection dialog

	int challenge;                              ///< from the server to use for connecting
	int checksumFeed;                           ///< from the server for checksum calculations

	int onlyVisibleClients;

	// these are our reliable messages that go to the server
	int reliableSequence;
	int reliableAcknowledge;                    ///< the last one the server has executed
	/// NOTE: incidentally, reliableCommands[0] is never used (always start at reliableAcknowledge+1)
	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];

	// unreliable binary data to send to server
	int binaryMessageLength;
	char binaryMessage[MAX_BINARY_MESSAGE];
	qboolean binaryMessageOverflowed;

	/// server message (unreliable) and command (reliable) sequence
	/// numbers are NOT cleared at level changes, but continue to
	/// increase as long as the connection is valid

	/// message sequence is used by both the network layer and the
	/// delta compression layer
	int serverMessageSequence;

	// reliable messages received from server
	int serverCommandSequence;
	int lastExecutedServerCommand;              ///< last server command grabbed or executed with CL_GetServerCommand
	char serverCommands[MAX_RELIABLE_COMMANDS][MAX_TOKEN_CHARS];

	// demo information
	char demoName[MAX_QPATH];
	qboolean demorecording;
	qboolean demoplaying;
	qboolean demowaiting;                       ///< don't record until a non-delta message is received
	qboolean firstDemoFrameSkipped;
	fileHandle_t demofile;

	qboolean waverecording;
	fileHandle_t wavefile;
	int wavetime;

	int timeDemoFrames;                         ///< counter of rendered frames
	int timeDemoStart;                          ///< cls.realtime before first frame
	int timeDemoBaseTime;                       ///< each frame will be at this time + frameNum * 50

	//float aviVideoFrameRemainder;
	//float aviSoundFrameRemainder;
	userAgent_t agent;                          ///< holds server engine information

	/// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t netchan;

} clientConnection_t;

extern clientConnection_t clc;

//==================================================================

/**
 * @struct ping_t
 * @brief
 */
typedef struct
{
	netadr_t adr;
	int start;
	int time;
	char info[MAX_INFO_STRING];
} ping_t;

/**
 * @struct serverInfo_t
 * @brief
 */
typedef struct
{
	netadr_t adr;
	char version[MAX_NAME_LENGTH];
	char hostName[MAX_SERVER_NAME_LENGTH];
	int32_t load;
	char mapName[MAX_NAME_LENGTH];
	char game[MAX_NAME_LENGTH];
	int32_t netType;
	int32_t gameType;
	int32_t clients;
	int32_t humans;
	int32_t maxClients;
	int32_t privateClients;
	int32_t minPing;
	int32_t maxPing;
	int32_t ping;
	int32_t visible;
	int32_t friendlyFire;
	int32_t maxlives;
	int32_t needpass;
	int32_t punkbuster;
	int32_t antilag;
	int32_t weaprestrict;
	int32_t balancedteams;
	char gameName[MAX_NAME_LENGTH];
} serverInfo_t;

typedef struct
{
	char *buffer;
	size_t bufferSize;
} clipboardCapture_t;

/**
 * @struct clientStatic_t
 * @brief the clientStatic_t structure is never wiped, and is used even when
 * no client connection is active at all
 *
 * A connection can be to either a server through the network layer or a
 * demo through a file.
 */
typedef struct
{
	connstate_t state;               ///< connection status
	challengeState_t challengeState; ///< challenge status

	int keyCatchers;                 ///< bit flags

	qboolean doCachePurge;           ///< empty the renderer cache as soon as possible

	char servername[MAX_OSPATH];     ///< name of server from original connect (used by reconnect)

	// when the server clears the hunk, all of these must be restarted
	qboolean rendererStarted;
	qboolean soundStarted;
	qboolean soundRegistered;
	qboolean uiStarted;
	qboolean cgameStarted;

	int framecount;
	int frametime;                  ///< msec since last frame

	int realtime;                   ///< ignores pause
	int realFrametime;              ///< ignoring pause, so console always works

	int numlocalservers;
	serverInfo_t localServers[MAX_OTHER_SERVERS];

	int numglobalservers;
	serverInfo_t globalServers[MAX_GLOBAL_SERVERS];
	// additional global servers
	int numGlobalServerAddresses;
	netadr_t globalServerAddresses[MAX_GLOBAL_SERVERS];

	int32_t numfavoriteservers;
	serverInfo_t favoriteServers[MAX_FAVOURITE_SERVERS];

	int pingUpdateSource;           ///< source currently pinging or updating

	char updateInfoString[MAX_INFO_STRING];

	// rendering info
	glconfig_t glconfig;
	qhandle_t charSetShader;
	qhandle_t whiteShader;
	qhandle_t consoleShader;
	//qhandle_t consoleShader2;

	download_t download;

	clipboardCapture_t clipboard;

	int cinematicHandle;
} clientStatic_t;

extern clientStatic_t cls;

//=============================================================================

extern vm_t        *cgvm;       ///< interface to cgame dll or vm
extern vm_t        *uivm;       ///< interface to ui dll or vm
extern refexport_t re;          ///< interface to refresh .dll

// cvars

extern cvar_t *cl_nodelta;
extern cvar_t *cl_debugMove;
extern cvar_t *cl_noprint;
extern cvar_t *cl_timegraph;
extern cvar_t *cl_maxpackets;
extern cvar_t *cl_packetdup;
extern cvar_t *cl_shownet;
extern cvar_t *cl_shownuments;
extern cvar_t *cl_showSend;
extern cvar_t *cl_showServerCommands;
extern cvar_t *cl_timeNudge;
extern cvar_t *cl_showTimeDelta;
extern cvar_t *cl_freezeDemo;

extern cvar_t *cl_yawspeed;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_run;
extern cvar_t *cl_anglespeedkey;

extern cvar_t *cl_recoilPitch;

extern cvar_t *cl_bypassMouseInput;

extern cvar_t *cl_doubletapdelay;

extern cvar_t *cl_sensitivity;
extern cvar_t *cl_freelook;

extern cvar_t *cl_mouseAccel;
extern cvar_t *cl_showMouseRate;

extern cvar_t *cl_avidemo;
extern cvar_t *cl_aviMotionJpeg;

extern cvar_t *m_pitch;
extern cvar_t *m_yaw;
extern cvar_t *m_forward;
extern cvar_t *m_side;
extern cvar_t *m_filter;

extern cvar_t *cl_timedemo;

extern cvar_t *cl_activeAction;
extern cvar_t *cl_autorecord;

extern cvar_t *cl_activatelean;

extern cvar_t *cl_allowDownload;
extern cvar_t *cl_conXOffset;

extern cvar_t *cl_missionStats;

extern cvar_t *cl_profile;
extern cvar_t *cl_defaultProfile;

extern cvar_t *cl_consoleKeys;

//=================================================

// cl_main
void CL_WriteWaveOpen(void);
void CL_WriteWaveClose(void);
void CL_Init(void);
void CL_FlushMemory(void);
void CL_ShutdownAll(void);
void CL_AddReliableCommand(const char *cmd);

void CL_StartHunkUsers(void);

void CL_RequestMasterData(qboolean force);

void CL_Disconnect_f(void);
void CL_GetChallengePacket(void);
void CL_Vid_Restart_f(void);
void CL_Snd_Restart_f(void);
void CL_NextDemo(void);
void CL_ReadDemoMessage(void);

void CL_DownloadsComplete(void);

void CL_GetPing(int n, char *buf, size_t buflen, int *pingtime);
void CL_GetPingInfo(int n, char *buf, size_t buflen);
void CL_ClearPing(int n);
int CL_GetPingQueueCount(void);

void CL_ShutdownRef(void);
void CL_InitRef(void);
int CL_ServerStatus(const char *serverAddress, char *serverStatusString, size_t maxLen);

void CL_AddToLimboChat(const char *str);
qboolean CL_GetLimboString(int index, char *buf);

void CL_TranslateString(const char *string, char *dest_buffer);
const char *CL_TranslateStringBuf(const char *string);
void CL_TranslateStringMod(const char *string, char *dest_buffer);

void CL_OpenURL(const char *url);

void CL_Record(const char *name);

// cl_avi

qboolean CL_OpenAVIForWriting(const char *fileName);
void CL_TakeVideoFrame(void);
void CL_WriteAVIVideoFrame(const byte *imageBuffer, int size);
void CL_WriteAVIAudioFrame(const byte *pcmBuffer, int size);
qboolean CL_CloseAVI(void);
qboolean CL_VideoRecording(void);

// cl_demo
extern demoPlayInfo_t dpi;

void CL_DemoCleanUp(void);
void CL_DemoCompleted(void);
void CL_WriteDemoMessage(msg_t *msg, int headerBytes);
void CL_StopRecord_f(void);
void CL_DemoRun(void);
void CL_DemoInit(void);

// cl_input

/**
 * @struct kbutton_t
 * @brief
 */
typedef struct
{
	int down[2];                ///< key nums holding it down
	unsigned downtime;          ///< msec timestamp
	unsigned msec;              ///< msec down this frame if both a down and up happened
	qboolean active;            ///< current state
	qboolean wasPressed;        ///< set when down, not cleared when up
} kbutton_t;

/**
 * @enum kbuttons_t
 * @brief
 */
typedef enum
{
	KB_NONE = -1,
	KB_LEFT,
	KB_RIGHT,
	KB_FORWARD,
	KB_BACK,
	KB_LOOKUP,
	KB_LOOKDOWN,
	KB_MOVELEFT,
	KB_MOVERIGHT,
	KB_STRAFE,
	KB_SPEED,
	KB_UP,
	KB_DOWN,
	KB_BUTTONS0,
	KB_BUTTONS1,
	KB_BUTTONS2,
	KB_BUTTONS3,
	KB_BUTTONS4,
	KB_BUTTONS5,
	KB_BUTTONS6,
	KB_BUTTONS7,                ///< unused
	KB_WBUTTONS0,
	KB_WBUTTONS1,
	KB_WBUTTONS2,
	KB_WBUTTONS3,
	KB_WBUTTONS4,
	KB_WBUTTONS5,
	KB_WBUTTONS6,
	KB_WBUTTONS7,
	KB_MLOOK,
	NUM_BUTTONS
} kbuttons_t;

void CL_ClearKeys(void);

void CL_InitInput(void);
void CL_SendCmd(void);
void CL_ClearState(void);
void CL_ReadPackets(void);

void CL_WritePacket(void);
void IN_Help(void);

float CL_KeyState(kbutton_t *key);
int Key_StringToKeynum(const char *str);
char *Key_KeynumToString(int keynum);

// cl_parse.c

extern int cl_connectedToPureServer;
extern int cl_connectedToCheatServer;

void CL_ParseSnapshot(msg_t *msg);
void CL_ParsePacketEntities(msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe);
void CL_SystemInfoChanged(void);
void CL_ParseServerMessage(msg_t *msg);

//====================================================================

void CL_ServerInfoPacket(netadr_t from, msg_t *msg);
void CL_ServerInfoPacketCheck(netadr_t from, msg_t *msg);
void CL_LocalServers_f(void);
void CL_GlobalServers_f(void);
void CL_Ping_f(void);
qboolean CL_UpdateVisiblePings_f(int source);

/// console
#define NUM_CON_TIMES   10

#define CON_TEXTSIZE    131072

/**
 * @struct console_t
 * @brief
 */
typedef struct
{
	qboolean initialized;

	unsigned int text[CON_TEXTSIZE];
	byte textColor[CON_TEXTSIZE];
	int current;                        ///< line where next message will be printed
	int x;                              ///< offset in current line for next print
	int scrollIndex;                    ///< bottom of console displays this line (current)
	int bottomDisplayedLine;            ///< bottom of console displays this line (final)

	int linewidth;                      ///< characters across screen
	int totalLines;                     ///< total text filled lines in console scrollback
	int maxTotalLines;                  ///< total lines in console scrollback

	float displayFrac;                  ///< aproaches finalFrac at con_openspeed
	float finalFrac;                    ///< 0.0 to 1.0 lines of console to display
	float desiredFrac;                  ///< for variable console heights

	int scanLines;                      ///< in scan lines
	int visibleLines;                   ///< amount of visible lines

	int times[NUM_CON_TIMES];           // cls.realtime time the line was generated
	                                    // for transparent notify lines
	vec4_t color;                       ///< for transparent lines

	int highlightOffset;                ///< highligting start offset (if == 0) then no hightlight
} console_t;

extern console_t con;

void Con_ToggleConsole_f(void);
void Con_DrawNotify(void);
void Con_ClearNotify(void);
void Con_Clear_f(void);
void Con_Dump_f(void);
void Con_CheckResize(void);
void Cmd_CompleteTxtName(char *args, int argNum);
void Con_Init(void);
void Con_Shutdown(void);
void Con_Linefeed(qboolean skipnotify);
void CL_ConsolePrint(char *txt);

void Con_DrawClock(void);
void Con_DrawVersion(void);
void Con_DrawInput(void);
void Con_DrawScrollbar(int length, float x, float y);
void Con_DrawSolidConsole(float frac);
void Con_DrawConsole(void);
void Con_RunConsole(void);

void Con_PageUp(void);
void Con_PageDown(void);
void Con_ScrollUp(int lines);
void Con_ScrollDown(int lines);
void Con_ScrollTop(void);
void Con_ScrollBottom(void);
void Con_Close(void);

// cl_scrn.c

void SCR_Init(void);
void SCR_UpdateScreen(void);

void SCR_DebugGraph(float value);

void SCR_AdjustFrom640(float *x, float *y, float *w, float *h);
void SCR_FillRect(float x, float y, float width, float height,
                  const float *color);
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void SCR_DrawChar(int x, int y, float w, float h, int ch, qboolean nativeResolution);
void SCR_DrawStringExt(int x, int y, float w, float h, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape, qboolean dropShadow, qboolean nativeResolution);

#define SCR_DrawSmallChar(x, y, ch) SCR_DrawChar(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, ch, qtrue)
// ignores embedded color control characters
#define SCR_DrawSmallString(x, y, string, setColor, forceColor, noColorEscape) SCR_DrawStringExt(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, string, setColor, forceColor, noColorEscape, qfalse, qtrue)

// cl_cin.c

#define MAX_CINEMATICS 16

typedef int cinHandle_t;

/**
 * @struct cinData_t
 * @brief
 */
typedef struct
{
	const byte *image;
	qboolean dirty;

	int width;
	int height;
} cinData_t;

/**
 * @struct cinematic_t
 * @brief
 */
typedef struct
{
	qboolean playing;

	int videoType;

	char name[MAX_OSPATH];
	int flags;

	fileHandle_t file;
	int size;
	int offset;

	int startTime;

	int frameRate;
	int frameWidth;
	int frameHeight;
	int frameCount;
	int frameBufferSize;
	byte *frameBuffer[2];

	rectDef_t rectangle;

	cinData_t currentData;

	void *data;
} cinematic_t;

void SCR_DrawCinematic(void);
void SCR_RunCinematic(void);
void SCR_StopCinematic(void);
int CIN_PlayCinematic(const char *name, int x, int y, int w, int h, int flags);
e_status CIN_StopCinematic(int handle);
e_status CIN_RunCinematic(int handle);
void CIN_DrawCinematic(int handle);
void CIN_SetExtents(int handle, int x, int y, int w, int h);
void CIN_UploadCinematic(int handle);
void CIN_CloseAllVideos(void);

void CIN_Init(void);
void CIN_Shutdown(void);

cinematic_t *CIN_GetCinematicByHandle(cinHandle_t handle);

// cl_roq.c
void ROQ_UpdateCinematic(cinematic_t *cin, int time);
qboolean ROQ_StartRead(cinematic_t *cin);
void ROQ_StopVideo(cinematic_t *cin);
void ROQ_Reset(cinematic_t *cin);
void ROQ_Init(void);

// cl_ogv.c
#ifdef FEATURE_THEORA
void OGV_UpdateCinematic(cinematic_t *cin, int time);
qboolean OGV_StartRead(cinematic_t *cin);
void OGV_StopVideo(cinematic_t *cin);
#endif

// cl_cgame.c

void CL_InitCGame(void);
void CL_ShutdownCGame(void);
qboolean CL_GameCommand(void);
void CL_CGameRendering(void);
void CL_SetCGameTime(void);
void CL_FirstSnapshot(void);
void CL_ShaderStateChanged(void);
void CL_UpdateLevelHunkUsage(void);
void CL_CGameBinaryMessageReceived(const byte *buf, int buflen, int serverTime);
qboolean CL_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);
qboolean CL_GetServerCommand(int serverCommandNumber);
void CL_AdjustTimeDelta(void);

// cl_ui.c

void CL_InitUI(void);
void CL_ShutdownUI(void);
int Key_GetCatcher(void);
void Key_SetCatcher(int catcher);
void LAN_LoadCachedServers(void);
void LAN_SaveServersToFile(void);
//int LAN_AddServer(int source, const char *name, const char *address);

// cl_net_chan.c

void CL_Netchan_Transmit(netchan_t *chan, msg_t *msg);   //int length, const byte *data );
void CL_Netchan_TransmitNextFragment(netchan_t *chan);
qboolean CL_Netchan_Process(netchan_t *chan, msg_t *msg);

// cl_db.c
#ifdef FEATURE_DBMS
void DB_InsertFavorite(const char *profile, int source, const char *name, const char *address, const char *mod);
void DB_DeleteFavorite(const char *profile, const char *address);
void DB_UpdateFavorite(const char *profile, const char *address);
void DB_LoadFavorites(const char *profile);

void CL_InitServerInfo(serverInfo_t *server, netadr_t *address);
#endif

#endif // #ifndef INCLUDE_CLIENT_H
