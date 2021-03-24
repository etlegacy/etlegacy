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
 * @file qcommon.h
 * @brief Definitions common between client and server, but not game or ref
 *        modules
 */

#ifndef INCLUDE_QCOMMON_H
#define INCLUDE_QCOMMON_H

#include "../qcommon/cm_public.h"

// msg.c

/**
 * @struct msg_s
 * @brief
 */
typedef struct
{
	qboolean allowoverflow;     ///< if false, do a Com_Error
	qboolean overflowed;        ///< set to true if the buffer size failed (with allowoverflow set)
	qboolean oob;               ///< set to true if the buffer size failed (with allowoverflow set)
	byte *data;                 ///< message content
	int maxsize;
	int cursize;
	int uncompsize;             ///< net debugging
	int readcount;
	int bit;                    ///< for bitwise reads and writes
	int strip;                  ///< strip >= 0x80 chars from message, old clients don't like them
} msg_t;

void MSG_Init(msg_t *buf, byte *data, int length);
void MSG_InitOOB(msg_t *buf, byte *data, int length);
void MSG_Clear(msg_t *buf);
void *MSG_GetSpace(msg_t *buf, int length);
void MSG_WriteData(msg_t *buf, const void *data, int length);
void MSG_Bitstream(msg_t *buf);
void MSG_Uncompressed(msg_t *buf);
#define MSG_EnableCharStrip(buf) (buf)->strip = 0x1;

/**
 * copy a msg_t in case we need to store it as is for a bit
 * (as I needed this to keep an msg_t from a static var for later use)
 * sets data buffer as MSG_Init does prior to do the copy
 */
void MSG_Copy(msg_t *buf, byte *data, int length, msg_t *src);

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void MSG_WriteBits(msg_t *msg, int value, int bits);

void MSG_WriteChar(msg_t *msg, int c);
void MSG_WriteByte(msg_t *msg, int c);
void MSG_WriteShort(msg_t *msg, int c);
void MSG_WriteLong(msg_t *msg, int c);
void MSG_WriteFloat(msg_t *msg, float f);
void MSG_WriteString(msg_t *msg, const char *s);
void MSG_WriteBigString(msg_t *msg, const char *s);
void MSG_WriteAngle16(msg_t *msg, float f);
int MSG_HashKey(const char *string, int maxlen, int strip);

void MSG_BeginReading(msg_t *msg);
void MSG_BeginReadingOOB(msg_t *msg);
void MSG_BeginReadingUncompressed(msg_t *msg);

int MSG_ReadBits(msg_t *msg, int bits);

int MSG_ReadChar(msg_t *msg);
int MSG_ReadByte(msg_t *msg);
int MSG_ReadShort(msg_t *msg);
int MSG_ReadLong(msg_t *msg);
float MSG_ReadFloat(msg_t *msg);
char *MSG_ReadString(msg_t *msg);
char *MSG_ReadBigString(msg_t *msg);
char *MSG_ReadStringLine(msg_t *msg);
float MSG_ReadAngle16(msg_t *msg);
void MSG_ReadData(msg_t *msg, void *data, int size);

void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);
void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);

void MSG_WriteDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to, qboolean force);
void MSG_ReadDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to, int number);

void MSG_WriteDeltaSharedEntity(msg_t *msg, void *from, void *to, qboolean force, int number);
void MSG_ReadDeltaSharedEntity(msg_t *msg, void *from, void *to, int number);

void MSG_WriteDeltaPlayerstate(msg_t *msg, struct playerState_s *from, struct playerState_s *to);
void MSG_ReadDeltaPlayerstate(msg_t *msg, struct playerState_s *from, struct playerState_s *to);

void MSG_ReportChangeVectors_f(void);

/**
==============================================================
NET
==============================================================
*/

#define NET_ENABLEV4            0x01
#define NET_ENABLEV6            0x02
/**
 * @def NET_PRIOV6
 * @brief if this flag is set, always attempt ipv6 connections
 * instead of ipv4 if a v6 address is found.
 */
#define NET_PRIOV6              0x04
/**
 * @def NET_PRIOV6
 * @brief disables ipv6 multicast support if set.
 */
#define NET_DISABLEMCAST        0x08

/**
 * @def PACKET_BACKUP
 * @brief number of old messages that must be kept on client and
 * server for delta comrpession and ping estimation
 */
#define PACKET_BACKUP   32
#define PACKET_MASK (PACKET_BACKUP - 1)

/**
 * @def MAX_PACKET_USERCMDS
 * @brief max number of usercmd_t in a packet
 */
#define MAX_PACKET_USERCMDS     32

#define PORT_ANY            -1

/**
 * @def MAX_RELIABLE_COMMANDS
 * @brief max string commands buffered for restransmit
 *
 * increased - seems to keep causing problems when set to 64
 */
#define MAX_RELIABLE_COMMANDS   256

/**
 * @enum netadrtype_t
 * @brief
 */
typedef enum
{
	NA_BAD = 0,                 ///< an address lookup failed
	NA_BOT,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IP6,
	NA_MULTICAST6,
	NA_UNSPEC
} netadrtype_t;

/**
 * @enum netsrc_t
 * @brief
 */
typedef enum
{
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

/**
 * @def NET_ADDRSTRMAXLEN
 * @brief maximum length of an IPv6 address string including trailing '\0'
 */
#define NET_ADDRSTRMAXLEN     48 /// why 48? IPv4-mapped IPv6 maximum is 45 .. + trailing 0 is 46
#define NET_ADDRSTRMAXLEN_EXT 56 /// NET_ADDRSTRMAXLEN + 8 (2xbrackets, colon, 5xport)

/**
 * @struct netadr_t
 * @brief
 */
typedef struct
{
	uint16_t type;
	byte ip[4];
	byte ip6[16];
	uint16_t port;
	uint64_t scope_id; ///< Needed for IPv6 link-local addresses
} netadr_t;

void NET_Init(void);
void NET_Shutdown(void);
void NET_Restart_f(void);
//void NET_Config(qboolean enableNetworking);

void NET_SendPacket(netsrc_t sock, int length, const void *data, netadr_t to);
void QDECL NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *format, ...);
void QDECL NET_OutOfBandData(netsrc_t sock, netadr_t adr, const char *format, int len);

qboolean NET_CompareAdr(netadr_t a, netadr_t b);
qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b);
qboolean NET_IsLocalAddress(netadr_t adr);
qboolean NET_IsLocalAddressString(const char *address);
qboolean NET_IsIPXAddress(const char *buf);
const char *NET_AdrToString(netadr_t a);
const char *NET_AdrToStringNoPort(netadr_t a);
int NET_StringToAdr(const char *s, netadr_t *a, netadrtype_t family);
qboolean NET_GetLoopPacket(netsrc_t sock, netadr_t *net_from, msg_t *net_message);
void NET_Sleep(int msec);

/**
 * @def MAX_MSGLEN
 * @brief max length of a message, which may be fragmented into multiple packets
 */
#define MAX_MSGLEN                  32768

/**
 * @def MAX_DOWNLOAD_WINDOW
 * @brief ACK window of 48 download chunks.Cannot set this higher, or clients will overflow the reliable commands buffer
 */
#define MAX_DOWNLOAD_WINDOW     48

/**
 * @def MAX_DOWNLOAD_BLKSIZE
 * @brief 896 byte block chunks
 */
#define MAX_DOWNLOAD_BLKSIZE        1024

/**
 * @def DLTYPE_WWW
 * @brief block -1 means www/ftp download
 */
#define DLTYPE_WWW -1

/**
 * @struct netchan_t
 *
 * @brief Netchan handles packet fragmentation and out of order / duplicate suppression
 */
typedef struct
{
	netsrc_t sock;

	int dropped;                    ///< between last packet and previous

	netadr_t remoteAddress;
	int qport;                      ///< qport value to write when transmitting

	// sequencing variables
	int incomingSequence;
	int outgoingSequence;

	// incoming fragment assembly buffer
	int fragmentSequence;
	int fragmentLength;
	byte fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean unsentFragments;
	int unsentFragmentStart;
	int unsentLength;
	byte unsentBuffer[MAX_MSGLEN];

	int lastSentTime;
	int lastSentSize;
} netchan_t;

void Netchan_Init(int port);
void Netchan_Setup(netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

void Netchan_Transmit(netchan_t *chan, int length, const byte *data);
void Netchan_TransmitNextFragment(netchan_t *chan);

qboolean Netchan_Process(netchan_t *chan, msg_t *msg);

/**
 * @brief download_t
 */
typedef struct
{
	// file transfer from server
	fileHandle_t download;
	int downloadNumber;
	int downloadBlock;                          ///< block we are waiting for
	int downloadCount;                          ///< how many bytes we got
	int downloadSize;                           ///< how many bytes we got
	int downloadFlags;                          ///< misc download behaviour flags sent by the server
	char downloadList[MAX_INFO_STRING];         ///< list of paks we need to download

	// www downloading
	qboolean bWWWDl;                            ///< we have a www download going
	qboolean bWWWDlAborting;                    ///< disable the CL_WWWDownload until server gets us a gamestate (used for aborts)
	char redirectedList[MAX_INFO_STRING];       ///< list of files that we downloaded through a redirect since last FS_ComparePaks
	char badChecksumList[MAX_INFO_STRING];      ///< list of files for which wwwdl redirect is broken (wrong checksum)

	// www downloading from static
	// in the static stuff since this may have to survive server disconnects
	// if new stuff gets added, CL_ClearStaticDownload code needs to be updated for clear up
	qboolean bWWWDlDisconnected;                ///< keep going with the download after server disconnect
	qboolean noReconnect;                       ///< do not try to reconnect when the dowload is ready
	char downloadName[MAX_OSPATH];
	char downloadTempName[MAX_OSPATH];          ///< in wwwdl mode, this is OS path (it's a qpath otherwise)
	char originalDownloadName[MAX_QPATH];       ///< if we get a redirect, keep a copy of the original file path
	qboolean downloadRestart;                   ///< if true, we need to do another FS_Restart because we downloaded a pak
	qboolean systemDownload;                    ///< if true, we are running an system inited download and we do not force fs_game or isolation paths
} download_t;

// update and motd server info

#define AUTOUPDATE_DIR "update"

/**
 * @struct autoupdate_t
 * @brief
 */
typedef struct
{
	netadr_t autoupdateServer;
	netadr_t authorizeServer;               ///< Unused.
	netadr_t motdServer;

	qboolean updateChecked;                 ///< Have we heard from the auto-update server this session?
	qboolean updateStarted;

	qboolean forceUpdate;

	char motdChallenge[MAX_TOKEN_CHARS];

	int masterDataChecked;
} autoupdate_t;

extern autoupdate_t autoupdate;

/*
==============================================================
PROTOCOL
==============================================================
*/

/**
 * @def PROTOCOL_MISMATCH_ERROR
 * @brief sent by the server, printed on connection screen, works for all clients
 * (restrictions: does not handle \n, no more than 256 chars)
 */
#define PROTOCOL_MISMATCH_ERROR "ERROR: Protocol Mismatch Between Client and Server.\
The server you are attempting to join is running an incompatible version of the game."

/**
 * @def PROTOCOL_MISMATCH_ERROR_LONG
 * @brief long version used by the client in diagnostic window
 */
#define PROTOCOL_MISMATCH_ERROR_LONG "ERROR: Protocol Mismatch Between Client and Server.\n\n\
The server you attempted to join is running an incompatible version of the game.\n\
You or the server may be running older versions of the game. Press the auto-update\
 button if it appears on the Main Menu screen."

#define GAMENAME_STRING     "et"
#define PROTOCOL_VERSION    84

/**
 * @var demo_protocols
 * @brief Maintain a list of compatible protocols for demo playing
 * @note That stuff only works with two digits protocols
 */
extern int demo_protocols[];

#define MASTER_SERVER_NAME  "master.etlegacy.com"                ///< location of the master server
#define MOTD_SERVER_NAME    "motd.etlegacy.com"                  ///< location of the message of the day server
#define UPDATE_SERVER_NAME  "update.etlegacy.com"                ///< location of the update server

#ifdef FEATURE_SSL
#define MIRROR_SERVER_URL "https://mirror.etlegacy.com" ///< location of the download server
#else
#define MIRROR_SERVER_URL "http://mirror.etlegacy.com"  ///< location of the download server
#endif

#define DOWNLOAD_SERVER_URL MIRROR_SERVER_URL "/etmain"

#define PORT_MASTER         27950
#define PORT_MOTD           27951
#define PORT_UPDATE         27951
#define PORT_SERVER         27960

/**
 * @def NUM_SERVER_PORTS
 *
 * Broadcast scan this many ports after PORT_SERVER
 * so a single machine can run multiple servers
 */
#define NUM_SERVER_PORTS    4

// the svc_strings[] array in cl_parse.c should mirror this

/**
 * @enum svc_ops_e
 * @brief Server to client
 */
enum svc_ops_e
{
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,           ///< [short] [string] only in gamestate messages
	svc_baseline,               ///< only in gamestate messages
	svc_serverCommand,          ///< [string] to be executed by client game module
	svc_download,               ///< [short] size [size bytes]
	svc_snapshot,
	svc_EOF
};

/**
 * @enum clc_ops_e
 * @brief Client to server
 */
enum clc_ops_e
{
	clc_bad,
	clc_nop,
	clc_move,                   ///< [[usercmd_t]
	clc_moveNoDelta,            ///< [[usercmd_t]
	clc_clientCommand,          ///< [string] message
	clc_EOF
};

/*
==============================================================
VIRTUAL MACHINE
==============================================================
*/

typedef struct vm_s vm_t;

/**
 * @enum vmInterpret_t
 * @brief
 */
typedef enum
{
	VMI_NATIVE,
	VMI_BYTECODE,
	VMI_COMPILED
} vmInterpret_t;

/**
 * @enum vmSlots_e
 * @typedef vmSlots_t
 * @brief
 */
typedef enum vmSlots_e
{
	VM_GAME = 0,
	VM_CGAME,
	VM_UI,
	MAX_VM
} vmSlots_t;

extern const char *vmStrs[MAX_VM];

/**
 * @enum sharedTraps_t
 * @brief
 */
typedef enum
{
	TRAP_MEMSET = 100,
	TRAP_MEMCPY,
	TRAP_STRNCPY,
	TRAP_SIN,
	TRAP_COS,
	TRAP_ATAN2,
	TRAP_SQRT,
	TRAP_MATRIXMULTIPLY,
	TRAP_ANGLEVECTORS,
	TRAP_PERPENDICULARVECTOR,
	TRAP_FLOOR,
	TRAP_CEIL,

	TRAP_TESTPRINTINT,
	TRAP_TESTPRINTFLOAT
} sharedTraps_t;

void VM_Init(void);
vm_t *VM_Create(const char *module, qboolean extract, intptr_t (*systemCalls)(intptr_t *), vmInterpret_t interpret);
// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"

void VM_Free(vm_t *vm);
void VM_Clear(void);
vm_t *VM_Restart(vm_t *vm);

intptr_t QDECL VM_CallFunc(vm_t *vm, int callNum, ...);
#define VM_Call(...) VM_CallFunc(__VA_ARGS__, VM_CALL_END)

void VM_Debug(int level);

void *VM_ArgPtr(intptr_t intValue);
void *VM_ExplicitArgPtr(vm_t *vm, intptr_t intValue);

#define VMA(x) VM_ArgPtr(args[x])
/**
 * @brief _vmf
 * @param[in] x
 */
static ID_INLINE float _vmf(intptr_t x)
{
	floatint_t fi;
	fi.i = (int) x;
	return fi.f;
}
#define VMF(x)  _vmf(args[x])

/*
==============================================================
CMD - Command text buffering and command execution
==============================================================
*/

/*
Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but entire text
files can be execed.
*/

void Cbuf_Init(void); ///< allocates an initial text buffer that will grow as needed

void Cbuf_AddText(const char *text); ///< Adds command text at the end of the buffer, does NOT add a final \n

void Cbuf_ExecuteText(int exec_when, const char *text); ///< this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_Execute(void);    ///< Pulls off \n terminated lines of text from the command buffer and sends
                            ///< them through Cmd_ExecuteString.  Stops when the buffer is empty.
                            ///< Normally called once per frame, but may be explicitly invoked.
                            ///< Do not call inside a command function, or current args will be destroyed.

//===========================================================================

/*
Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.
*/

typedef void (*xcommand_t)(void);
typedef void (*completionFunc_t)(char *args, int argNum);

void Cmd_Init(void);

// We need to use EXPAND because the Microsoft MSVC preprocessor does not expand the va_args the same way as other preprocessors
// http://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define EXPAND(x) x
#define GET_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define Cmd_AddCommand1(x) Cmd_AddSystemCommand(x, NULL, NULL, NULL)
#define Cmd_AddCommand2(x, y) Cmd_AddSystemCommand(x, y, NULL, NULL)
#define Cmd_AddCommand3(x, y, z) Cmd_AddSystemCommand(x, y, z, NULL)
#define Cmd_AddCommand4(x, y, z, i) Cmd_AddSystemCommand(x, y, z, i)
#define Cmd_AddCommand(...) EXPAND(GET_MACRO(__VA_ARGS__, Cmd_AddCommand4, Cmd_AddCommand3, Cmd_AddCommand2, Cmd_AddCommand1) (__VA_ARGS__))

/**
 * Called by the init functions of other parts of the program to
 * register commands and functions to call for them.
 * The cmd_name is referenced later, so it should not be in temp memory
 * if function is NULL, the command will be forwarded to the server
 * as a clc_clientCommand instead of executed locally
 */
void Cmd_AddSystemCommand(const char *cmd_name, xcommand_t function, const char *description, completionFunc_t complete);

void Cmd_RemoveCommand(const char *cmd_name);
void Cmd_RemoveCommandSafe(const char *cmd_name);

void Cmd_CommandCompletion(void (*callback)(const char *s)); ///< callback with each valid string

void Cmd_SetCommandCompletionFunc(const char *command, completionFunc_t complete);
void Cmd_SetCommandDescription(const char *command, const char *description);
void Cmd_CompleteArgument(const char *command, char *args, int argNum);

void Cmd_SaveCmdContext(void);
void Cmd_RestoreCmdContext(void);

int Cmd_Argc(void);
char *Cmd_Argv(int arg);
void Cmd_ArgvBuffer(int arg, char *buffer, size_t bufferLength);
char *Cmd_Args(void);
char *Cmd_ArgsFrom(int arg);
char *Cmd_ArgsFromTo(int arg, int max);
void Cmd_ArgsBuffer(char *buffer, size_t bufferLength);
char *Cmd_Cmd(void);

/**
 * The functions that execute commands get their parameters with these
 * functions. Cmd_Argv () will return an empty string, not a NULL
 * if arg > argc, so string operations are allways safe.
 */
void Cmd_Args_Sanitize(void);

void Cmd_TokenizeString(const char *text);

/// Takes a null terminated string.  Does not need to be /n terminated.
/// breaks the string up into arg tokens.
void Cmd_TokenizeStringIgnoreQuotes(const char *text_in);

/// Parses a single line of text into arguments and tries to execute it
/// as if it was typed at the console
void Cmd_ExecuteString(const char *text);

/*
==============================================================
CVAR
==============================================================
*/

/*
cvar_t variables are used to hold scalar or string variables that can be changed
or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder         prints the current value
r_draworder 0       sets the current value to 0
set r_draworder 0   as above, but creates the cvar if not present

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

The are also occasionally used to communicated information between different
modules of the program.
*/

cvar_t *Cvar_Get(const char *varName, const char *value, int flags);
cvar_t *Cvar_GetAndDescribe(const char *varName, const char *value, int flags, const char *description);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags
// if value is "", the value will not override a previously set value.

void Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
// basically a slightly modified Cvar_Get for the interpreted modules

void Cvar_Update(vmCvar_t *vmCvar);
// updates an interpreted modules' version of a cvar

void Cvar_Set(const char *varName, const char *value);
// will create the variable with no flags if it doesn't exist

cvar_t *Cvar_Set2(const char *var_name, const char *value, qboolean force);
// same as Cvar_Set, but allows more control over setting of cvar

void Cvar_SetSafe(const char *var_name, const char *value);
// sometimes we set variables from an untrusted source: fail if flags & CVAR_PROTECTED

void Cvar_SetLatched(const char *var_name, const char *value);
// don't set the cvar immediately

void Cvar_SetValue(const char *var_name, float value);
void Cvar_SetValueSafe(const char *var_name, float value);
// expands value to a string and calls Cvar_Set/Cvar_SetSafe

float Cvar_VariableValue(const char *var_name);
int Cvar_VariableIntegerValue(const char *var_name);
// returns 0 if not defined or non numeric

char *Cvar_VariableString(const char *var_name);
void Cvar_VariableStringBuffer(const char *var_name, char *buffer, size_t bufsize);
// returns an empty string if not defined
void Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer, size_t bufsize);
// returns the latched value if there is one, else the normal one, empty string if not defined as usual

int Cvar_Flags(const char *var_name);
// returns CVAR_NONEXISTENT if cvar doesn't exist or the flags of that particular CVAR.

void Cvar_CommandCompletion(void (*callback)(const char *s));
// callback with each valid string

void Cvar_Reset(const char *var_name);
void Cvar_ForceReset(const char *var_name);

void Cvar_SetCheatState(void);
// reset all testing vars to a safe value

qboolean Cvar_Command(void);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables(fileHandle_t f);
// writes lines containing "set variable value" for all variables
// with the archive flag set to true.

void Cvar_Init(void);

char *Cvar_InfoString(int bit);
char *Cvar_InfoString_Big(int bit);
// returns an info string containing all the cvars that have the given bit set
// in their flags ( CVAR_USERINFO, CVAR_SERVERINFO, CVAR_SYSTEMINFO, etc )
void Cvar_InfoStringBuffer(int bit, char *buff, size_t buffsize);
void Cvar_CheckRange(cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral);
void Cvar_SetDescription(cvar_t *cv, const char *varDescription);

void Cvar_Restart(qboolean unsetVM);
void Cvar_Restart_f(void);

void Cvar_CompleteCvarName(char *args, int argNum);

extern int cvar_modifiedFlags;
// whenever a cvar is modifed, its flags will be OR'd into this, so
// a single check can determine if any CVAR_USERINFO, CVAR_SERVERINFO,
// etc, variables have been modified since the last check.  The bit
// can then be cleared to allow another change detection.

/*
==============================================================
FILESYSTEM
No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

// ET: Legacy specific - used by engine code
#define BASEGAME        "etmain"
#define DEFAULT_MODGAME MODNAME

/**
 * @struct modHash
 * @brief
 */
typedef struct
{
	unsigned int defaultMod;
	unsigned int currentMod;
} modHash;
extern modHash modHashes;

//#define IS_DEFAULT_MOD (Q_stricmp(Cvar_VariableString("fs_game"), DEFAULT_MODGAME) == 0)
#define IS_DEFAULT_MOD (modHashes.defaultMod == modHashes.currentMod)

// referenced flags
// these are in loop specific order so don't change the order
#define FS_GENERAL_REF  0x01
#define FS_UI_REF       0x02
#define FS_CGAME_REF    0x04
#define FS_QAGAME_REF   0x08

/**
 * @def NUM_ID_PAKS
 * @brief number of id paks that will never be autodownloaded from etmain
 */
#define NUM_ID_PAKS     9

/**
 * @def MAX_FILE_HANDLES
 * @brief
 */
#define MAX_FILE_HANDLES    64

/**
 * @def MAX_FOUND_FILES
 * @brief Max return of listing files
 */
#define MAX_FOUND_FILES 0x1000

qboolean FS_Initialized(void);

void FS_InitFilesystem(void);
void FS_Shutdown(qboolean closemfp);

qboolean FS_ConditionalRestart(int checksumFeed);
void FS_Restart(int checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

char **FS_ListFiles(const char *path, const char *extension, int *numfiles);
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void FS_FreeFileList(char **list);

qboolean FS_FileExists(const char *file);
qboolean FS_SV_FileExists(const char *file, qboolean checkBase);

qboolean FS_IsSamePath(const char *s1, const char *s2);
qboolean FS_CreatePath(const char *OSPath);
void FS_CheckFilenameIsNotExecutable(const char *fileName, const char *function);
//void FS_CheckFilenameIsMutable(const char *filename, const char *function);
void FS_Remove(const char *osPath);

char *FS_NormalizePath(const char *path);
char *FS_BuildOSPath(const char *base, const char *game, const char *qpath);

int FS_LoadStack(void);

int FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int FS_GetModList(char *listbuf, int bufsize);

fileHandle_t FS_FOpenFileWrite(const char *fileName);
// will properly create any needed paths and deal with seperater character issues

long FS_filelength(fileHandle_t f);
fileHandle_t FS_SV_FOpenFileWrite(const char *fileName);
long FS_SV_FOpenFileRead(const char *fileName, fileHandle_t *fp);
void FS_SV_Rename(const char *from, const char *to);
long FS_FOpenFileRead(const char *fileName, fileHandle_t *file, qboolean uniqueFILE);
long FS_FOpenFileReadFullDir(const char *fullFileName, fileHandle_t *file);

/*
if uniqueFILE is true, then a new FILE will be fopened even if the file
is found in an already open pak file.  If uniqueFILE is false, you must call
FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
It is generally safe to always set uniqueFILE to true, because the majority of
file IO goes through FS_ReadFile, which Does The Right Thing already.

added exclude flag to filter out regular dirs or pack files on demand
would rather have used FS_FOpenFileRead(..., int filter_flag = 0)
but that's a C++ construct ..
*/
#define FS_EXCLUDE_DIR 0x1
#define FS_EXCLUDE_PK3 0x2
long FS_FOpenFileRead_Filtered(const char *qpath, fileHandle_t *file, qboolean uniqueFILE, int filter_flag);

// returns 1 if a file is in the PAK file, otherwise -1
int FS_FileIsInPAK(const char *fileName, int *pChecksum);

int FS_Delete(const char *fileName);

int FS_Write(const void *buffer, int len, fileHandle_t h);

int FS_OSStatFile(const char *ospath);

long FS_FileAge(const char *ospath);

int FS_Read(void *buffer, int len, fileHandle_t f);
// properly handles partial reads and reads from other dlls

void FS_FCloseFile(fileHandle_t f);
// note: you can't just fclose from another DLL, due to MS libc issues

int FS_ReadFile(const char *qpath, void **buffer);
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void FS_ForceFlush(fileHandle_t f);
// forces flush on files we're writing to.

void FS_FreeFile(void *buffer);
// frees the memory returned by FS_ReadFile

void FS_WriteFile(const char *qpath, const void *buffer, int size);
// writes a complete file, creating any subdirectories needed

long FS_filelength(fileHandle_t f);
// doesn't work for files that are opened from a pack file

int FS_FTell(fileHandle_t f);
// where are we?

void FS_Flush(fileHandle_t f);

void QDECL FS_Printf(fileHandle_t h, const char *fmt, ...);
// like fprintf

int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode);
// opens a file for reading, writing, or appending depending on the value of mode

int FS_Seek(fileHandle_t f, long offset, int origin);
// seek on a file

qboolean FS_FilenameCompare(const char *s1, const char *s2);
qboolean FS_IsDemoExt(const char *fileName, int namelen);

const char *FS_LoadedPakNames(void);
const char *FS_LoadedPakChecksums(void);
const char *FS_LoadedPakPureChecksums(void);
// Returns a space separated string containing the checksums of all loaded pk3 files.
// Servers with sv_pure set will get this string and pass it to clients.

const char *FS_ReferencedPakNames(void);
const char *FS_ReferencedPakChecksums(void);
const char *FS_ReferencedPakPureChecksums(void);
// Returns a space separated string containing the checksums of all loaded
// AND referenced pk3 files. Servers with sv_pure set will get this string
// back from clients for pure validation

void FS_ClearPakReferences(int flags);
// clears referenced booleans on loaded pk3s
void FS_ClearPureServerPacks(void);
void FS_PureServerSetReferencedPaks(const char *pakSums, const char *pakNames);
void FS_PureServerSetLoadedPaks(const char *pakSums, const char *pakNames);
// If the string is empty, all data sources will be allowed.
// If not empty, only pk3 files that match one of the space
// separated checksums will be checked for files, with the
// sole exception of .cfg files.

qboolean FS_CheckDirTraversal(const char *checkdir);
qboolean FS_VerifyOfficialPaks(void);
qboolean FS_idPak(const char *pak, const char *base);
qboolean FS_ComparePaks(char *neededpaks, size_t len, qboolean dlstring);
qboolean FS_InvalidGameDir(const char *gamedir);

void FS_Rename(const char *from, const char *to);

void FS_FilenameCompletion(const char *dir, int numext, const char **ext,
                           qboolean stripExt, void (*callback)(const char *s), qboolean allowNonPureFilesOnDisk);

#if !defined(DEDICATED)
extern int cl_connectedToPureServer;
qboolean FS_CL_ExtractFromPakFile(const char *base, const char *gamedir, const char *fileName);
#endif

void FS_CopyFile(const char *fromOSPath, const char *toOSPath);

qboolean FS_VerifyPak(const char *pak);

qboolean FS_UnzipTo(const char *fileName, const char *outpath, qboolean quiet);
qboolean FS_Unzip(const char *fileName, qboolean quiet);

void FS_HomeRemove(const char *homePath);

qboolean FS_FileInPathExists(const char *ospath);
int FS_CalculateFileSHA1(const char *path, char *hash);
const char *FS_Dirpath(const char *path);
const char *FS_Basename(const char *path);
qboolean FS_MatchFileInPak(const char *filepath, const char *match);
#define IsPathSep(X) ((X) == '\\' || (X) == '/' || (X) == PATH_SEP)

#if defined(FEATURE_PAKISOLATION) && !defined(DEDICATED)
const char* DL_ContainerizePath(const char *temp, const char *dest);
void FS_InitWhitelist(void);
qboolean FS_IsWhitelisted(const char *pakName, const char *hash);
#define FS_CONTAINER "dlcache"
#endif


/*
==============================================================
DOWNLOAD
==============================================================
*/

#include "dl_public.h"

/*
==============================================================
Edit fields and command line history/completion
==============================================================
*/

#define MAX_EDIT_LINE   512

/**
 * @struct field_t
 * @brief
 */
typedef struct
{
	int cursor;
	int scroll;
	int widthInChars;
	char buffer[MAX_EDIT_LINE];
} field_t;

void Console_RemoveHighlighted(field_t *field, int *completionOffset);
void Console_AutoComplete(field_t *field, int *completionOffset);
void Field_Clear(field_t *field);
void Field_AutoComplete(field_t *field);
void Field_CompleteKeyname(void);
void Field_CompleteFilenameMultiple(const char *dir, int numext, const char **ext, qboolean allowNonPureFilesOnDisk);
void Field_CompleteFilename(const char *dir, const char *ext, qboolean stripExt, qboolean allowNonPureFilesOnDisk);
void Field_CompleteCommand(char *cmd, qboolean doCommands, qboolean doCvars);

/*
==============================================================
MISC
==============================================================
*/

char *CopyString(const char *in);
void Info_Print(const char *s);

void Com_BeginRedirect(char *buffer, size_t buffersize, void (*flush)(char *));
void Com_EndRedirect(void);
void QDECL Com_Printf(const char *fmt, ...) _attribute ((format(printf, 1, 2)));
void QDECL Com_DPrintf(const char *fmt, ...) _attribute ((format(printf, 1, 2)));
void QDECL Com_Error(int code, const char *fmt, ...) _attribute ((noreturn, format(printf, 2, 3)));
void Com_Quit_f(void) _attribute ((noreturn));

int Com_Milliseconds(void);     // will be journaled properly
unsigned int Com_BlockChecksum(const void *buffer, size_t length);
unsigned int Com_BlockChecksumKey(void *buffer, int length, int key);
char *Com_MD5FileETCompat(const char *fileName);
int Com_HashKey(char *string, int maxlen);
int Com_Filter(char *filter, char *name, int casesensitive);
int Com_FilterPath(const char *filter, const char *name, int casesensitive);
int Com_RealTime(qtime_t *qtime);
qboolean Com_SafeMode(void);
void Com_RandomBytes(byte *string, int len);

char *Com_MD5File(const char *fileName, int length, const char *prefix, int prefix_len);

void Com_StartupVariable(const char *match);
void Com_SetRecommended(void);
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

// profile functions
void Com_TrackProfile(const char *profile_path);
qboolean Com_CheckProfile(void);

extern cvar_t *com_crashed;
extern cvar_t *com_ignorecrash;

extern cvar_t *com_pid;
extern cvar_t *com_pidfile;

extern cvar_t *com_developer;
extern cvar_t *com_dedicated;
extern cvar_t *com_speeds;
extern cvar_t *com_timescale;
extern cvar_t *com_sv_running;
extern cvar_t *com_cl_running;
extern cvar_t *com_viewlog;             // 0 = hidden, 1 = visible, 2 = minimized
extern cvar_t *com_version;
extern cvar_t *com_buildScript;         // for building release pak files
extern cvar_t *com_journal;
extern cvar_t *com_ansiColor;
extern cvar_t *com_unfocused;
extern cvar_t *com_minimized;
#if idppc
extern cvar_t *com_altivec;
#endif

// updater and motd
extern cvar_t *com_updateavailable;
extern cvar_t *com_updatemessage;
extern cvar_t *com_updatefiles;
extern cvar_t *com_motd;
extern cvar_t *com_motdString;

// watchdog
extern cvar_t *com_watchdog;
extern cvar_t *com_watchdog_cmd;

// both client and server must agree to pause
extern cvar_t *cl_paused;
extern cvar_t *sv_paused;

extern cvar_t *cl_packetdelay;
extern cvar_t *sv_packetdelay;

// com_speeds times
extern int time_game;
extern int time_frontend;
extern int time_backend;            // renderer backend time

extern int com_frameTime;
extern int com_expectedhunkusage;
extern int com_hunkusedvalue;

extern qboolean com_errorEntered;

extern cvar_t *com_masterServer;
extern cvar_t *com_motdServer;
extern cvar_t *com_updateServer;
extern cvar_t *com_downloadURL;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

/**
 * @enum memtag_t
 * @brief
 */
typedef enum
{
	TAG_FREE,
	TAG_GENERAL,
	TAG_BOTLIB,
	TAG_RENDERER,
	TAG_SMALL,
	TAG_STATIC
} memtag_t;

/*
--- low memory ----
server vm
server clipmap
---mark---
renderer initialization (shaders, etc)
UI vm
cgame vm
renderer map
renderer models

---free---

temp file loading
--- high memory ---
*/

#ifdef ETLEGACY_DEBUG
#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define Z_TagMalloc(size, tag)          Z_TagMallocDebug(size, tag, # size, __FILE__, __LINE__)
#define Z_Malloc(size)                  Z_MallocDebug(size, # size, __FILE__, __LINE__)
#define S_Malloc(size)                  S_MallocDebug(size, # size, __FILE__, __LINE__)
void *Z_TagMallocDebug(size_t size, int tag, char *label, char *file, int line);   // NOT 0 filled memory
void *Z_MallocDebug(size_t size, char *label, char *file, int line);               // returns 0 filled memory
void *S_MallocDebug(size_t size, char *label, char *file, int line);               // returns 0 filled memory
#else
void *Z_TagMalloc(size_t size, int tag);   // NOT 0 filled memory
void *Z_Malloc(size_t size);               // returns 0 filled memory
void *S_Malloc(size_t size);               // NOT 0 filled memory only for small allocations
#endif
void Z_Free(void *ptr);
void Z_FreeTags(int tag);
void Z_LogHeap(void);

void Hunk_Clear(void);
void Hunk_ClearToMark(void);
void Hunk_SetMark(void);
qboolean Hunk_CheckMark(void);
//void *Hunk_Alloc( int size );
// void *Hunk_Alloc( int size, ha_pref preference );
void Hunk_ClearTempMemory(void);
void *Hunk_AllocateTempMemory(size_t size);
void Hunk_FreeTempMemory(void *buf);
int Hunk_MemoryRemaining(void);
void Hunk_SmallLog(void);
void Hunk_Log(void);

void Com_TouchMemory(void);

// commandLine should not include the executable name (argv[0])
void Com_Init(char *commandLine);
char *Com_GetCommandLine(void);
void Com_Frame(void);
void Com_Shutdown(qboolean badProfile);

/*
==============================================================
CLIENT / SERVER SYSTEMS
==============================================================
*/

// client interface

void CL_InitKeyCommands(void);
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init(void);
void CL_Disconnect(qboolean showMainMenu);
void CL_Shutdown(void);
void CL_Frame(int msec);
qboolean CL_GameCommand(void);
void CL_KeyEvent(int key, qboolean down, unsigned time);

void CL_CharEvent(int key);
// char events are for field typing, not game control

void CL_MouseEvent(int dx, int dy, int time);

void CL_JoystickEvent(int axis, int value, int time);

void CL_PacketEvent(netadr_t from, msg_t *msg);

void CL_ConsolePrint(char *txt);

void CL_MapLoading(void);
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void CL_ForwardCommandToServer(const char *string);
// adds the current command line as a clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_ShutdownAll(void);
// shutdown all the client stuff

void CL_FlushMemory(void);
// dump all memory on an error

qboolean CL_ConnectedToServer(void);
// returns qtrue if connected to a server

void CL_StartHunkUsers(void);
// start all the client stuff using the hunk

void CL_Snd_Shutdown(void);
// Restart sound subsystem

// udpate.c

/**
 * @enum UPDATE_FLAGS
 * @brief
 */
enum UPDATE_FLAGS
{
	CLEAR_STATUS = 0,
	CLEAR_FLAGS,
	CLEAR_ALL,
};

void Com_CheckAutoUpdate(void);
void Com_GetAutoUpdate(void);
qboolean Com_CheckUpdateDownloads(void);
qboolean Com_InitUpdateDownloads(void);
qboolean Com_UpdatePacketEvent(netadr_t from);
void Com_UpdateInfoPacket(netadr_t from);
void Com_CheckUpdateStarted(void);
void Com_UpdateVarsClean(int flags);
void Com_Update_f(void);

// download.c
#define CA_CERT_FILE "cacert.pem"
#define TMP_FILE_EXTENSION ".tmp"
void Com_ClearDownload(void);
void Com_ClearStaticDownload(void);

void Com_NextDownload(void);
void Com_InitDownloads(void);
void Com_WWWDownload(void);
qboolean Com_WWWBadChecksum(const char *pakname);
void Com_Download_f(void);

#if defined(FEATURE_SSL)
void Com_CheckCaCertStatus(void);
#endif

void Key_KeynameCompletion(void (*callback)(const char *s));
// for keyname autocompletion

void Key_WriteBindings(fileHandle_t f);
// for writing the config files

void S_ClearSoundBuffer(qboolean killStreaming);
// call before filesystem access

void SCR_DebugGraph(float value);     // FIXME: move logging to common?

// AVI files have the start of pixel lines 4 byte-aligned
#define AVI_LINE_PADDING 4

// server interface

void SV_Init(void);
void SV_Shutdown(const char *finalmsg);
void SV_Frame(int msec);
void SV_PacketEvent(netadr_t from, msg_t *msg);
qboolean SV_GameCommand(void);
int SV_FrameMsec();
int SV_SendQueuedPackets();

// UI interface

qboolean UI_GameCommand(void);

/*
==============================================================
NON-PORTABLE SYSTEM SERVICES
==============================================================
*/

/**
 * @enum joystickAxis_t
 * @brief
 */
typedef enum
{
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

/**
 * @enum sysEventType_t
 * @brief
 * @note Make sure SE_NONE is zero
 */
typedef enum
{
	SE_NONE = 0,        ///< evTime is still valid
	SE_KEY,             ///< evValue is a key code, evValue2 is the down flag
	SE_CHAR,            ///< evValue is an ascii char
	SE_MOUSE,           ///< evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,   ///< evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,         ///< evPtr is a char*
} sysEventType_t;

/**
 * @struct sysEvent_t
 * @brief
 * @note Make sure SE_NONE is zero
 */
typedef struct
{
	int evTime;
	sysEventType_t evType;
	int evValue, evValue2;
	int evPtrLength;                ///< bytes of data pointed to by evPtr, for journaling
	void *evPtr;                    ///< this must be manually freed if not NULL
} sysEvent_t;

void Com_QueueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr);
int Com_EventLoop(void);
sysEvent_t Com_GetSystemEvent(void);
void Com_RunAndTimeServerPacket(netadr_t *evFrom, msg_t *buf);

void Sys_Init(void);
qboolean IN_IsNumLockDown(void);

#ifdef _WIN32
#define Sys_GetDLLName(x) x "_mp_" ARCH_STRING DLL_EXT
#elif __OpenBSD__ // TODO: detect the *BSD variant
#define Sys_GetDLLName(x) x ".mp.obsd." ARCH_STRING DLL_EXT
#elif __FreeBSD__ // TODO: detect the *BSD variant
#define Sys_GetDLLName(x) x ".mp.fbsd." ARCH_STRING DLL_EXT
#elif __NetBSD__ // TODO: detect the *BSD variant
#define Sys_GetDLLName(x) x ".mp.nbsd." ARCH_STRING DLL_EXT
#elif __APPLE__
#define Sys_GetDLLName(x) x DLL_EXT
#elif __ANDROID__
#define Sys_GetDLLName(x) "lib" x ".mp.android." ARCH_STRING DLL_EXT
#else
#define Sys_GetDLLName(x) x ".mp." ARCH_STRING DLL_EXT
#endif

qboolean Sys_DllExtension(const char *name);

char *Sys_GetCurrentUser(void);

void QDECL Sys_Error(const char *error, ...) _attribute ((noreturn, format(printf, 1, 2)));
void Sys_Quit(void) _attribute ((noreturn));
char *IN_GetClipboardData(void);       // note that this isn't journaled...
void IN_SetClipboardData(const char *text);

void Sys_Print(const char *msg);

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int Sys_Milliseconds(void);

int Sys_PID(void);
qboolean Sys_WritePIDFile(void);
qboolean Sys_PIDIsRunning(unsigned int pid);

void Sys_SnapVector(float *v);

// the system console is shown when a dedicated server is running
void Sys_DisplaySystemConsole(qboolean show);

void Sys_SendPacket(int length, const void *data, netadr_t to);

qboolean Sys_StringToAdr(const char *s, netadr_t *a, netadrtype_t family);
//Does NOT parse port numbers, only base addresses.

qboolean Sys_IsLANAddress(netadr_t adr);
void Sys_ShowIP(void);

qboolean Sys_CheckCD(void);

FILE *Sys_FOpen(const char *ospath, const char *mode);
qboolean Sys_Mkdir(const char *path);

#ifdef _WIN32
int Sys_Remove(const char *path);
int Sys_RemoveDir(const char *path);

#define sys_stat_t struct _stat
int Sys_Stat(const char *path, void *stat);
#define Sys_S_IsDir(m) (m & _S_IFDIR)

int Sys_Rename(const char *from, const char *to);
char *Sys_RealPath(const char *path);
#define Sys_PathAbsolute(name) (name && strlen(name) > 3 && name[1] == ':' && (name[2] == '\\' || name[2] == '/'))
#else
#include <unistd.h>
#define Sys_Remove(x) remove(x)
#define Sys_RemoveDir(x) rmdir(x)
#define Sys_Rename(from, to) rename(from, to)
#define Sys_PathAbsolute(name) (name && name[0] == '/')
// realpath() returns NULL if there are issues with the file
#define Sys_RealPath(path) realpath(path, NULL)

#define sys_stat_t struct stat
#define Sys_Stat(osPath, statBuf) stat(osPath, statBuf)
#define Sys_S_IsDir(m) S_ISDIR(m)
#endif

char *Sys_Cwd(void);
char *Sys_DefaultBasePath(void);
char *Sys_DefaultInstallPath(void);
char *Sys_DefaultHomePath(void);
const char *Sys_Basename(char *path);
const char *Sys_Dirname(char *path);
char *Sys_ConsoleInput(void);

qboolean Sys_RandomBytes(byte *string, int len);

char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs);
void Sys_FreeFileList(char **list);

qboolean Sys_LowPhysicalMemory(void);

void Sys_SetEnv(const char *name, const char *value);

/**
 * @enum dialogResult_t
 * @brief
 */
typedef enum
{
	DR_YES    = 0,
	DR_NO     = 1,
	DR_OK     = 0,
	DR_CANCEL = 1
} dialogResult_t;

/**
 * @enum dialogType_t
 * @brief
 */
typedef enum
{
	DT_INFO,
	DT_WARNING,
	DT_ERROR,
	DT_YES_NO,
	DT_OK_CANCEL
} dialogType_t;

dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title);

// NOTE: on win32 the cwd is prepended .. non portable behaviour
void Sys_StartProcess(char *cmdline, qboolean doexit);
void Sys_OpenURL(const char *url, qboolean doexit);

#ifndef _WIN32
void Sys_Chmod(const char *file, int mode);
#endif

// Console
void Hist_Add(field_t *field);
field_t *Hist_Prev(void);
field_t *Hist_Next(void);

/*
 * This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list
 */

#define NYT HMAX                    ///< NYT = Not Yet Transmitted
#define INTERNAL_NODE (HMAX + 1)

/**
 * @struct nodetype
 * @typedef node_t
 * @brief
 */
typedef struct nodetype
{
	struct  nodetype *left, *right, *parent;    ///< tree structure
	struct  nodetype *next, *prev;              ///< doubly-linked list
	struct  nodetype **head;                    ///< highest ranked node in block
	int weight;
	int symbol;

} node_t;

/**
 * @def HMAX
 * @brief Maximum symbol
 */
#define HMAX 256

/**
 * @struct huff_t
 * @brief
 */
typedef struct
{
	int blocNode;
	int blocPtrs;

	node_t *tree;
	node_t *lhead;
	node_t *ltail;
	node_t *loc[HMAX + 1];
	node_t **freelist;

	node_t nodeList[768];
	node_t *nodePtrs[768];
} huff_t;

/**
 * @struct huffman_t
 * @brief
 */
typedef struct
{
	huff_t compressor;
	huff_t decompressor;
} huffman_t;

void Huff_Compress(msg_t *mbuf, int offset);
void Huff_Decompress(msg_t *mbuf, int offset);
void Huff_Init(huffman_t *huff);
void Huff_addRef(huff_t *huff, byte ch);
int Huff_Receive(node_t *node, int *ch, byte *fin);
void Huff_transmit(huff_t *huff, int ch, byte *fout, int maxoffset);
void Huff_offsetReceive(node_t *node, int *ch, byte *fin, int *offset, int maxoffset);
void Huff_offsetTransmit(huff_t *huff, int ch, byte *fout, int *offset, int maxoffset);
void Huff_putBit(int bit, byte *fout, int *offset);
int Huff_getBit(byte *fin, int *offset);

extern huffman_t clientHuffTables;

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

void Com_GetHunkInfo(int *hunkused, int *hunkexpected);

/*
==============================================================
Native language support
==============================================================
*/
#ifdef FEATURE_GETTEXT

#define _(x) I18N_Translate(x)
#define __(x) I18N_TranslateMod(x)

void I18N_Init(void);
void I18N_SetLanguage(const char *language);
const char *I18N_Translate(const char *msgid);
const char *I18N_TranslateMod(const char *msgid);

extern qboolean doTranslateMod;

#else // FEATURE_GETTEXT
#define _(x) x
#define __(x) x
#endif

#endif // #ifndef INCLUDE_QCOMMON_H
