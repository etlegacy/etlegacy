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
 * @file server.h
 * @brief Primary header for server
 */

#ifndef INCLUDE_SERVER_H
#define INCLUDE_SERVER_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../game/g_public.h"
#include "../game/bg_public.h"

#if defined(FEATURE_IRC_SERVER) && defined(DEDICATED)
	#include "../irc/irc_client.h"
#endif

#define PERS_SCORE              0   ///< !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!

// advert control
#define SVA_MASTER      0x0001      ///< 1  - master server
#define SVA_TRACKER     0x0002      ///< 2  - tracker

// server attack protection
#define SVP_IOQ3        0x0001      ///< 1  - ioQuake3 way
#define SVP_OWOLF       0x0002      ///< 2  - OpenWolf way
#define SVP_CONSOLE     0x0004      ///< 4  - console print

#define MAX_ENT_CLUSTERS    16

#define MAX_BPS_WINDOW      20      ///< net debugging

/**
 * @struct svEntity_s
 * @typedef svEntity_t
 * @brief
 */
typedef struct svEntity_s
{
	struct worldSector_s *worldSector;
	struct svEntity_s *nextEntityInWorldSector;

	entityState_t baseline;             ///< for delta compression of initial sighting
	int numClusters;                    ///< if -1, use headnode instead
	int clusternums[MAX_ENT_CLUSTERS];
	int lastCluster;                    ///< if all the clusters don't fit in clusternums
	int areanum, areanum2;
	int snapshotCounter;                ///< used to prevent double adding from portal views
	int originCluster;                  ///< calced upon linking, for origin only bmodel vis checks
} svEntity_t;

/**
 * @enum serverState_t
 */
typedef enum
{
	SS_DEAD,            ///< no map loaded
	SS_LOADING,         ///< spawning level entities
	SS_GAME             ///< actively running
} serverState_t;

/**
 * @struct server_t
 * @brief
 */
typedef struct
{
	serverState_t state;
	qboolean restarting;                ///< if true, send configstring changes during SS_LOADING
	int serverId;                       ///< changes each server start
	int restartedServerId;              ///< serverId before a map_restart
	int checksumFeed;                   ///< the feed key that we use to compute the pure checksum strings
	/// the serverId associated with the current checksumFeed (always <= serverId)
	int checksumFeedServerId;
	int snapshotCounter;                ///< incremented for each snapshot built
	int timeResidual;                   ///< <= 1000 / sv_frame->value
	int nextFrameTime;                  ///< when time > nextFrameTime, process world
	char *configstrings[MAX_CONFIGSTRINGS];
	qboolean configstringsmodified[MAX_CONFIGSTRINGS];
	svEntity_t svEntities[MAX_GENTITIES];

	char *entityParsePoint;             ///< used during game VM init

	// the game virtual machine will update these on init and changes
	sharedEntity_t *gentities;
	int gentitySize;
	int num_entities;                   ///< current number, <= MAX_GENTITIES

	playerState_t *gameClients;
	int gameClientSize;                 ///< will be > sizeof(playerState_t) due to game private data

	int restartTime;

	// net debugging
	int bpsWindow[MAX_BPS_WINDOW];
	int bpsWindowSteps;
	int bpsTotalBytes;
	int bpsMaxBytes;

	int ubpsWindow[MAX_BPS_WINDOW];
	int ubpsTotalBytes;
	int ubpsMaxBytes;

	float ucompAve;
	int ucompNum;

	md3Tag_t tags[MAX_SERVER_TAGS];
	tagHeaderExt_t tagHeadersExt[MAX_TAG_FILES];

	int num_tagheaders;
	int num_tags;

	// serverside demo recording
	fileHandle_t demoFile;
	demoState_t demoState;
	char demoName[MAX_QPATH];

	// serverside demo recording - previous frame for delta compression
	sharedEntity_t demoEntities[MAX_GENTITIES];
	playerState_t demoPlayerStates[MAX_CLIENTS];

} server_t;

/**
 * @struct clientSnapshot_t
 * @brief
 */
typedef struct
{
	int areabytes;
	byte areabits[MAX_MAP_AREA_BYTES];  ///< portalarea visibility bits
	playerState_t ps;
	int num_entities;
	int first_entity;                   ///< into the circular sv_packet_entities[]
	                                    ///< the entities MUST be in increasing state number
	                                    ///< order, otherwise the delta compression will fail
	int messageSent;                    ///< time the message was transmitted
	int messageAcked;                   ///< time the message was acked
	int messageSize;                    ///< used to rate drop packets
} clientSnapshot_t;

/**
 * @enum clientState_t
 * @brief
 */
typedef enum
{
	CS_FREE = 0,    ///< can be reused for a new connection
	CS_ZOMBIE,      ///< client has been disconnected, but don't reuse connection for a couple seconds
	CS_CONNECTED,   ///< has been assigned to a client_t, but no gamestate yet
	CS_PRIMED,      ///< gamestate has been sent, but client hasn't sent a usercmd
	CS_ACTIVE       ///< client is fully in game
} clientState_t;

/**
 * @struct netchan_buffer_s
 * @typedef netchan_buffer_t
 * @brief
 */
typedef struct netchan_buffer_s
{
	msg_t msg;
	byte msgBuffer[MAX_MSGLEN];
	char lastClientCommandString[MAX_STRING_CHARS];
	struct netchan_buffer_s *next;
} netchan_buffer_t;

/**
 * @struct client_s
 * @typedef client_t
 * @brief
 */
typedef struct client_s
{
	clientState_t state;
	char userinfo[MAX_INFO_STRING];         ///< name, etc
	char userinfobuffer[MAX_INFO_STRING];   ///< used for buffering of user info

	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	int reliableSequence;                   ///< last added reliable message, not necesarily sent or acknowledged yet
	int reliableAcknowledge;                ///< last acknowledged reliable message
	int reliableSent;                       ///< last sent reliable message, not necesarily acknowledged yet
	int messageAcknowledge;

	int binaryMessageLength;
	char binaryMessage[MAX_BINARY_MESSAGE];
	qboolean binaryMessageOverflowed;

	int gamestateMessageNum;                ///< netchan->outgoingSequence of gamestate
	int challenge;

	usercmd_t lastUsercmd;
	int lastMessageNum;                     ///< for delta compression
	int lastClientCommand;                  ///< reliable client message sequence
	char lastClientCommandString[MAX_STRING_CHARS];
	sharedEntity_t *gentity;                ///< SV_GentityNum(clientnum)
	char name[MAX_NAME_LENGTH];             ///< extracted from userinfo, high bits masked
	char guid[MAX_GUID_LENGTH + 1];         ///< extracted from userinfo

	// downloading
	char downloadName[MAX_QPATH];           ///< if not empty string, we are downloading
	fileHandle_t download;                  ///< file being downloaded by game server DL - see qboolean bWWWing for http DL
	int downloadSize;                       ///< total bytes (can't use EOF because of paks)
	int downloadCount;                      ///< bytes sent
	int downloadClientBlock;                ///< last block we sent to the client, awaiting ack
	int downloadCurrentBlock;               ///< current block number
	int downloadXmitBlock;                  ///< last block we xmited
	unsigned char *downloadBlocks[MAX_DOWNLOAD_WINDOW];     ///< the buffers for the download blocks
	int downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	qboolean downloadEOF;                   ///< We have sent the EOF block
	int downloadSendTime;                   ///< time we last sent a package
	int downloadAckTime;                    ///< time we last got an ack from the client

	// www downloading
	qboolean bDlOK;                         ///< passed from cl_wwwDownload CVAR_USERINFO, wether this client supports www dl
	char downloadURL[MAX_OSPATH];           ///< the URL we redirected the client to
	qboolean bWWWDl;                        ///< we have a www download going
	qboolean bWWWing;                       ///< the client is doing an ftp/http download - see fileHandle_t download for game server DL
	qboolean bFallback;                     ///< last www download attempt failed, fallback to regular download
	// NOTE: this is one-shot, multiple downloads would cause a www download to be attempted again

	int deltaMessage;                       ///< frame last client usercmd message
	int nextReliableTime;                   ///< svs.time when another reliable command will be allowed
	int nextReliableUserTime;               ///< svs.time when another userinfo change will be allowed
	int lastPacketTime;                     ///< svs.time when packet was last received
	int lastConnectTime;                    ///< svs.time when connection started
	int lastValidGamestate;                 ///< svs.time when active in game
	int lastSnapshotTime;                   ///< svs.time of last sent snapshot
	qboolean rateDelayed;                   ///< true if lastSnapshotTime was set based on rate instead of snapshotMsec
	int timeoutCount;                       ///< must timeout a few frames in a row so debugging doesn't break
	clientSnapshot_t frames[PACKET_BACKUP]; ///< updates can be delta'd from here
	int ping;
	int rate;                               ///< bytes / second
	int snapshotMsec;                       ///< requests a snapshot every snapshotMsec unless rate choked
	int pureAuthentic;
	qboolean gotCP;                         ///< additional flag to distinguish between a bad pure checksum, and no cp command at all
	netchan_t netchan;
	// queuing outgoing fragmented messages to send them properly, without udp packet bursts
	// in case large fragmented messages are stacking up
	// buffer them into this queue, and hand them out to netchan as needed
	netchan_buffer_t *netchan_start_queue;
	netchan_buffer_t **netchan_end_queue;

	int downloadnotify;

	int protocol; ///< We can access clients protocol any time

	qboolean demoClient; ///< is this a demoClient?

	userAgent_t agent;
} client_t;

//=============================================================================

#define STATFRAMES 100 ///< 5 seconds - assumed we run 20 fps

/**
 * @struct svstats_t
 * @brief
 */
typedef struct
{
	double active;
	double idle;
	int count;

	double latched_active;
	double latched_idle;

	float cpu;
	float avg;
} svstats_t;

/**
 * @def MAX_CHALLENGES
 * @brief Made large to prevent a DoS attack that could
 * cycle all of them out before legitimate users connected
 */
#define MAX_CHALLENGES  2048

/**
 * @struct challenge_t
 * @brief
 */
typedef struct
{
	netadr_t adr;
	int challenge;
	int clientChallenge;    ///< challenge number coming from the client
	int time;               ///< time the last packet was sent to the autherize server
	int pingTime;           ///< time the challenge response was sent to client
	int firstTime;          ///< time the adr was first used
	int firstPing;          ///< Used for min and max ping checks
	qboolean wasrefused;
	qboolean connected;
} challenge_t;

/**
 * @struct receipt_t
 * @brief
 */
typedef struct
{
	netadr_t adr;
	int time;
} receipt_t;

/**
 * @def MAX_INFO_RECEIPTS
 * @brief the maximum number of getstatus+getinfo responses that we send in
 * a two second time period.
 */
#define MAX_INFO_RECEIPTS  48

/**
 * @struct tempBan_s
 * @typedef tempBan_t
 * @brief
 */
typedef struct tempBan_s
{
	netadr_t adr;
	int endtime;
} tempBan_t;

#define MAX_TEMPBAN_ADDRESSES               MAX_CLIENTS

#define SERVER_PERFORMANCECOUNTER_FRAMES    600
#define SERVER_PERFORMANCECOUNTER_SAMPLES   6

/**
 * @struct serverStatic_t
 * @brief This structure will be cleared only when the game dll changes
 */
typedef struct
{
	qboolean initialized;                       ///< sv_init has completed

	int time;                                   ///< will be strictly increasing across level changes

	int snapFlagServerBit;                      ///< ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	client_t *clients;                          ///< [sv_maxclients->integer];
	int numSnapshotEntities;                    ///< sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES
	int nextSnapshotEntities;                   ///< next snapshotEntities to use
	entityState_t *snapshotEntities;            ///< [numSnapshotEntities]
	int nextHeartbeatTime;
	challenge_t challenges[MAX_CHALLENGES];     ///< to prevent invalid IPs from connecting
	receipt_t infoReceipts[MAX_INFO_RECEIPTS];
	netadr_t redirectAddress;                   ///< for rcon return messages
	tempBan_t tempBanAddresses[MAX_TEMPBAN_ADDRESSES];

	int sampleTimes[SERVER_PERFORMANCECOUNTER_SAMPLES];
	int currentSampleIndex;
	int totalFrameTime;
	int currentFrameIndex;
	int serverLoad;
	svstats_t stats;

	download_t download;
} serverStatic_t;

//=============================================================================

extern serverStatic_t svs;                  ///< persistant server info across maps
extern server_t       sv;                   ///< cleared each map
extern vm_t           *gvm;                 ///< game virtual machine

extern cvar_t *sv_fps;
extern cvar_t *sv_timeout;
extern cvar_t *sv_zombietime;
extern cvar_t *sv_rconPassword;
extern cvar_t *sv_privatePassword;
extern cvar_t *sv_hidden;
extern cvar_t *sv_allowDownload;
extern cvar_t *sv_friendlyFire;
extern cvar_t *sv_maxlives;
extern cvar_t *sv_maxclients;
extern cvar_t *sv_needpass;
extern cvar_t *sv_democlients; ///< number of democlients: this should always be set to 0,
                               ///< and will be automatically adjusted when needed by the demo facility.
                               ///< ATTENTION: if sv_maxclients = sv_democlients then server will be full!
                               ///< sv_democlients consume clients slots even if there are no democlients recorded nor replaying for this slot!

extern cvar_t *sv_privateClients;
extern cvar_t *sv_hostname;
extern cvar_t *sv_master[MAX_MASTER_SERVERS];

extern cvar_t *sv_reconnectlimit;
extern cvar_t *sv_tempbanmessage;

extern cvar_t *sv_padPackets;
extern cvar_t *sv_killserver;
extern cvar_t *sv_mapname;
extern cvar_t *sv_mapChecksum;
extern cvar_t *sv_serverid;
extern cvar_t *sv_minRate;
extern cvar_t *sv_maxRate;
extern cvar_t *sv_minPing;
extern cvar_t *sv_maxPing;
extern cvar_t *sv_gametype;
extern cvar_t *sv_pure;
extern cvar_t *sv_floodProtect;
extern cvar_t *sv_lanForceRate;
extern cvar_t *sv_onlyVisibleClients;

extern cvar_t *sv_showAverageBPS;           ///< net debugging

/// autodl
extern cvar_t *sv_dl_timeout;

extern cvar_t *sv_wwwDownload; ///< general flag to enable/disable www download redirects
extern cvar_t *sv_wwwBaseURL;  ///< the base URL of all the files
/// tell clients to perform their downloads while disconnected from the server
/// this gets you a better throughput, but you loose the ability to control the download usage
extern cvar_t *sv_wwwDlDisconnected;
extern cvar_t *sv_wwwFallbackURL;

extern cvar_t *sv_cheats;
extern cvar_t *sv_packetloss;
extern cvar_t *sv_packetdelay;

extern cvar_t *sv_dlRate;

extern cvar_t *sv_fullmsg;

extern cvar_t *sv_advert;

extern cvar_t *sv_protect;
extern cvar_t *sv_protectLog;

#ifdef FEATURE_ANTICHEAT
extern cvar_t *sv_wh_active;
extern cvar_t *sv_wh_bbox_horz;
extern cvar_t *sv_wh_bbox_vert;
extern cvar_t *sv_wh_check_fov;
#endif

// server side demo recording
extern cvar_t *sv_demopath;
extern cvar_t *sv_demoState;
extern cvar_t *sv_autoDemo;
extern cvar_t *sv_freezeDemo;
extern cvar_t *sv_demoTolerant;

extern cvar_t *sv_ipMaxClients; ///< limit client connection

extern cvar_t *sv_guidCheck;    ///< enable check for client guid validity

//===========================================================

// sv_demo.c
void SV_DemoAutoDemoRecord(void);
void SV_DemoRestartPlayback(void);
void SV_DemoReadFrame(void);
void SV_DemoWriteFrame(void);
qboolean SV_DemoClientCommandCapture(client_t *client, const char *msg);
void SV_DemoWriteServerCommand(const char *cmd);
void SV_DemoWriteGameCommand(int clientNum, const char *cmd);
void SV_DemoWriteConfigString(int cs_index, const char *cs_string);
void SV_DemoWriteClientUserinfo(client_t *client, const char *userinfo);
qboolean SV_CheckLastCmd(const char *cmd, qboolean onlyStore);
void SV_DemoStopAll(void);
void SV_DemoInit(void);
void SV_DemoShutdown(void);

// sv_demo_ext.c
//int SV_GentityGetHealthField(sharedEntity_t *gent);   // Test purpose
//void SV_GentitySetHealthField(sharedEntity_t *gent, int value);   // Test purpose
void SV_GentityUpdateHealthField(sharedEntity_t *gent, playerState_t *player);

// sv_main.c
void SV_FinalCommand(const char *cmd, qboolean disconnect);   ///< added disconnect flag so map changes can use this function as well
void QDECL SV_SendServerCommand(client_t *cl, const char *fmt, ...) _attribute ((format(printf, 2, 3)));
void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);
void SV_MasterHeartbeat(const char *msg);
void SV_MasterShutdown(void);
void SV_MasterGameCompleteStatus(void);
int SV_RateMsec(client_t *client);

typedef struct leakyBucket_s leakyBucket_t;

/**
 * @struct leakyBucket_s
 * @typedef leakyBucket_t
 * @brief
 */
struct leakyBucket_s
{
	netadrtype_t type;

	union
	{
		byte _4[4];
		byte _6[16];
	} ipv;

	int lastTime;
	signed char burst;

	long hash;

	leakyBucket_t *prev, *next;
};

/// This is deliberately quite large to make it more of an effort to DoS
#define MAX_BUCKETS         16384
#define MAX_HASHES          1024

qboolean SVC_RateLimit(leakyBucket_t *bucket, int burst, int period);
qboolean SVC_RateLimitAddress(netadr_t from, int burst, int period);
extern leakyBucket_t outboundLeakyBucket;

// sv_init.c
void SV_SetConfigstringNoUpdate(int index, const char *val);
void SV_SetConfigstring(int index, const char *val);
void SV_UpdateConfigStrings(void);
void SV_GetConfigstring(int index, char *buffer, unsigned int bufferSize);
void SV_SetUserinfo(int index, const char *val);
void SV_GetUserinfo(int index, char *buffer, unsigned int bufferSize);
void SV_ChangeMaxClients(void);
void SV_SpawnServer(const char *server);
void SV_WriteAttackLog(const char *log);

#ifdef ETLEGACY_DEBUG
#define SV_WriteAttackLogD(x) SV_WriteAttackLog(x)
#else
#define SV_WriteAttackLogD(x)
#endif

// sv_client.c
void SV_GetChallenge(netadr_t from);
void SV_DirectConnect(netadr_t from);
void SV_ExecuteClientMessage(client_t *cl, msg_t *msg);
void SV_UserinfoChanged(client_t *cl);
void SV_UpdateUserinfo_f(client_t *cl);
void SV_ClientEnterWorld(client_t *client, usercmd_t *cmd);
qboolean SV_CheckForMsgOverflow(client_t *client, msg_t *msg);
void SV_DropClient(client_t *drop, const char *reason);
void SV_ExecuteClientCommand(client_t *cl, const char *s, qboolean clientOK, qboolean premaprestart);
void SV_ClientThink(client_t *cl, usercmd_t *cmd);
int SV_SendDownloadMessages(void);
int SV_SendQueuedMessages(void);

// sv_ccmds.c
void SV_Heartbeat_f(void);
qboolean SV_TempBanIsBanned(netadr_t address);
void SV_TempBanNetAddress(netadr_t address, int length);
void SV_UptimeReset(void);

// sv_snapshot.c
void SV_AddServerCommand(client_t *client, const char *cmd);
void SV_UpdateServerCommandsToClient(client_t *client, msg_t *msg);
void SV_SendMessageToClient(msg_t *msg, client_t *client);
void SV_SendClientMessages(void);
void SV_SendClientSnapshot(client_t *client);
void SV_CheckClientUserinfoTimer(void);
void SV_SendClientIdle(client_t *client);

// sv_game.c
int SV_NumForGentity(sharedEntity_t *ent);
sharedEntity_t *SV_GentityNum(int num);
playerState_t *SV_GameClientNum(int num);
svEntity_t *SV_SvEntityForGentity(sharedEntity_t *gEnt);
sharedEntity_t *SV_GEntityForSvEntity(svEntity_t *svEnt);
void SV_InitGameProgs(void);
void SV_ShutdownGameProgs(void);
void SV_RestartGameProgs(void);
qboolean SV_inPVS(const vec3_t p1, const vec3_t p2);
qboolean SV_GetTag(int clientNum, int tagFileNumber, char *tagname, orientation_t *orientation);
int SV_LoadTag(const char *mod_name);
void SV_GameSendServerCommand(int clientNum, const char *text);

void SV_GameBinaryMessageReceived(int cno, const char *buf, int buflen, int commandTime);

// sv_bot.c
int SV_BotAllocateClient(int clientNum);
void SV_BotFreeClient(int clientNum);
int SV_BotGetConsoleMessage(int client, char *buf, size_t size);
int BotImport_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void BotImport_DebugPolygonDelete(int id);

// sv_wallhack.c
#ifdef FEATURE_ANTICHEAT
void SV_RandomizePos(int player, int other);
void SV_InitWallhack(void);
void SV_RestorePos(int cli);
int SV_CanSee(int player, int other);
int SV_PositionChanged(int cli);
#endif

//============================================================

// high level object sorting to reduce interaction tests

void SV_ClearWorld(void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity(sharedEntity_t *gEnt);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity(sharedEntity_t *gEnt);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

clipHandle_t SV_ClipHandleForEntity(const sharedEntity_t *ent);

void SV_SectorList_f(void);

int SV_AreaEntities(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount);
// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact test.
// returns the number of pointers filled in
// The world entity is never returned in this list.

int SV_PointContents(const vec3_t p, int passEntityNum);
// returns the CONTENTS_* value from the world and all entities at the given point.

void SV_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, qboolean capsule);
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum is explicitly excluded from clipping checks (normally ENTITYNUM_NONE)

void SV_ClipToEntity(trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, qboolean capsule);
// clip to a specific entity

// sv_net_chan.c
void SV_Netchan_Transmit(client_t *client, msg_t *msg);
void SV_Netchan_ClearQueue(client_t *client);
int SV_Netchan_TransmitNextFragment(client_t *client);
qboolean SV_Netchan_Process(client_t *client, msg_t *msg);

// cl->downloadnotify
#define DLNOTIFY_REDIRECT   0x00000001  ///< "Redirecting client ..."
#define DLNOTIFY_BEGIN      0x00000002  ///< "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL        (DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN)

#endif // #ifndef INCLUDE_SERVER_H
