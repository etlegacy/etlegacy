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
 * @file g_syscalls.c
 * @brief this file is only included when building a dll, g_syscalls.asm is
 * included instead when building a qvm
 */

#include "g_local.h"

static intptr_t (QDECL *syscall)(intptr_t arg, ...) = (intptr_t ( QDECL * )(intptr_t, ...)) - 1;

#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
void dllEntry(intptr_t (QDECL *syscallptr)(intptr_t arg, ...))
{
	syscall = syscallptr;
}
#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif

int PASSFLOAT(float x)
{
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void    trap_Printf(const char *fmt)
{
	syscall(G_PRINT, fmt);
}

void    trap_Error(const char *fmt)
{
	syscall(G_ERROR, fmt);
}

int     trap_Milliseconds(void)
{
	return syscall(G_MILLISECONDS);
}
int     trap_Argc(void)
{
	return syscall(G_ARGC);
}

void    trap_Argv(int n, char *buffer, int bufferLength)
{
	syscall(G_ARGV, n, buffer, bufferLength);
}

int     trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	return syscall(G_FS_FOPEN_FILE, qpath, f, mode);
}

void    trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	syscall(G_FS_READ, buffer, len, f);
}

int     trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	return syscall(G_FS_WRITE, buffer, len, f);
}

int     trap_FS_Rename(const char *from, const char *to)
{
	return syscall(G_FS_RENAME, from, to);
}

void    trap_FS_FCloseFile(fileHandle_t f)
{
	syscall(G_FS_FCLOSE_FILE, f);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return syscall(G_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

void    trap_SendConsoleCommand(int exec_when, const char *text)
{
	syscall(G_SEND_CONSOLE_COMMAND, exec_when, text);
}

void    trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags)
{
	syscall(G_CVAR_REGISTER, cvar, var_name, value, flags);
}

void    trap_Cvar_Update(vmCvar_t *cvar)
{
	syscall(G_CVAR_UPDATE, cvar);
}

void trap_Cvar_Set(const char *var_name, const char *value)
{
	syscall(G_CVAR_SET, var_name, value);
}

int trap_Cvar_VariableIntegerValue(const char *var_name)
{
	return syscall(G_CVAR_VARIABLE_INTEGER_VALUE, var_name);
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	syscall(G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}

void trap_Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	syscall(G_CVAR_LATCHEDVARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}

void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                         playerState_t *clients, int sizeofGClient)
{
	syscall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient);
}

void trap_DropClient(int clientNum, const char *reason, int length)
{
	syscall(G_DROP_CLIENT, clientNum, reason, length);
}

void trap_SendServerCommand(int clientNum, const char *text)
{
	// commands over 1022 chars will crash the client engine upon receipt,
	// so ignore them
	if (strlen(text) > 1022)
	{
		G_LogPrintf("%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAMEVERSION, clientNum);
		G_LogPrintf("%s: text [%s.950s]... truncated\n", GAMEVERSION, text);
		return;
	}
	syscall(G_SEND_SERVER_COMMAND, clientNum, text);
}

void trap_SetConfigstring(int num, const char *string)
{
	syscall(G_SET_CONFIGSTRING, num, string);
}

void trap_GetConfigstring(int num, char *buffer, int bufferSize)
{
	syscall(G_GET_CONFIGSTRING, num, buffer, bufferSize);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
	syscall(G_GET_USERINFO, num, buffer, bufferSize);
}

void trap_SetUserinfo(int num, const char *buffer)
{
	syscall(G_SET_USERINFO, num, buffer);
}

void trap_GetServerinfo(char *buffer, int bufferSize)
{
	syscall(G_GET_SERVERINFO, buffer, bufferSize);
}

void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	syscall(G_SET_BRUSH_MODEL, ent, name);
}

void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

void trap_TraceNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACE, results, start, mins, maxs, end, -2, contentmask);
}

void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, -2, contentmask);
}

int trap_PointContents(const vec3_t point, int passEntityNum)
{
	return syscall(G_POINT_CONTENTS, point, passEntityNum);
}


qboolean trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	return syscall(G_IN_PVS, p1, p2);
}

qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	return syscall(G_IN_PVS_IGNORE_PORTALS, p1, p2);
}

void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open)
{
	syscall(G_ADJUST_AREA_PORTAL_STATE, ent, open);
}

qboolean trap_AreasConnected(int area1, int area2)
{
	return syscall(G_AREAS_CONNECTED, area1, area2);
}

void trap_LinkEntity(gentity_t *ent)
{
	syscall(G_LINKENTITY, ent);
}

void trap_UnlinkEntity(gentity_t *ent)
{
	syscall(G_UNLINKENTITY, ent);
}


int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount)
{
	return syscall(G_ENTITIES_IN_BOX, mins, maxs, list, maxcount);
}

qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return syscall(G_ENTITY_CONTACT, mins, maxs, ent);
}

qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return syscall(G_ENTITY_CONTACTCAPSULE, mins, maxs, ent);
}

int trap_BotAllocateClient(int clientNum)
{
	return syscall(G_BOT_ALLOCATE_CLIENT, clientNum);
}

void trap_BotFreeClient(int clientNum)
{
}

int trap_GetSoundLength(sfxHandle_t sfxHandle)
{
	return syscall(G_GET_SOUND_LENGTH, sfxHandle);
}

sfxHandle_t trap_RegisterSound(const char *sample, qboolean compressed)
{
	return syscall(G_REGISTERSOUND, sample, compressed);
}

#ifdef DEBUG
//#define FAKELAG
#ifdef FAKELAG
#define MAX_USERCMD_BACKUP  256
#define MAX_USERCMD_MASK    (MAX_USERCMD_BACKUP - 1)

static usercmd_t cmds[MAX_CLIENTS][MAX_USERCMD_BACKUP];
static int       cmdNumber[MAX_CLIENTS];
#endif // FAKELAG
#endif // DEBUG

void trap_GetUsercmd(int clientNum, usercmd_t *cmd)
{
	syscall(G_GET_USERCMD, clientNum, cmd);

#ifdef FAKELAG
	{
		char s[MAX_STRING_CHARS];
		int  fakeLag;

		trap_Cvar_VariableStringBuffer("g_fakelag", s, sizeof(s));
		fakeLag = atoi(s);
		if (fakeLag < 0)
		{
			fakeLag = 0;
		}

		if (fakeLag)
		{
			int i;
			int realcmdtime, thiscmdtime;

			// store our newest usercmd
			cmdNumber[clientNum]++;
			memcpy(&cmds[clientNum][cmdNumber[clientNum] & MAX_USERCMD_MASK], cmd, sizeof(usercmd_t));

			// find a usercmd that is fakeLag msec behind
			i           = cmdNumber[clientNum] & MAX_USERCMD_MASK;
			realcmdtime = cmds[clientNum][i].serverTime;
			i--;
			do
			{
				thiscmdtime = cmds[clientNum][i & MAX_USERCMD_MASK].serverTime;

				if (realcmdtime - thiscmdtime > fakeLag)
				{
					// found the right one
					cmd = &cmds[clientNum][i & MAX_USERCMD_MASK];
					return;
				}

				i--;
			}
			while ((i & MAX_USERCMD_MASK) != (cmdNumber[clientNum] & MAX_USERCMD_MASK));

			// didn't find a proper one, just use the oldest one we have
			cmd = &cmds[clientNum][(cmdNumber[clientNum] - 1) & MAX_USERCMD_MASK];
			return;
		}
	}
#endif // FAKELAG
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	return syscall(G_GET_ENTITY_TOKEN, buffer, bufferSize);
}

int trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points)
{
	return syscall(G_DEBUG_POLYGON_CREATE, color, numPoints, points);
}

void trap_DebugPolygonDelete(int id)
{
	syscall(G_DEBUG_POLYGON_DELETE, id);
}

int trap_RealTime(qtime_t *qtime)
{
	return syscall(G_REAL_TIME, qtime);
}

void trap_SnapVector(float *v)
{
	syscall(G_SNAPVECTOR, v);
	return;
}

qboolean trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t *or)
{
	return syscall(G_GETTAG, clientNum, tagFileNumber, tagName, or);
}

qboolean trap_LoadTag(const char *filename)
{
	return syscall(G_REGISTERTAG, filename);
}

// BotLib traps start here
int trap_BotLibSetup(void)
{
	return 0;
}

int trap_BotLibShutdown(void)
{
	return 0;
}

int trap_BotLibVarSet(char *var_name, char *value)
{
	return 0;
}

int trap_BotLibVarGet(char *var_name, char *value, int size)
{
	return 0;
}

int trap_BotLibDefine(char *string)
{
	return 0;
}

int trap_PC_AddGlobalDefine(char *define)
{
	return 0;
}

int trap_PC_LoadSource(const char *filename)
{
	return syscall(BOTLIB_PC_LOAD_SOURCE, filename);
}

int trap_PC_FreeSource(int handle)
{
	return syscall(BOTLIB_PC_FREE_SOURCE, handle);
}

int trap_PC_ReadToken(int handle, pc_token_t *pc_token)
{
	return syscall(BOTLIB_PC_READ_TOKEN, handle, pc_token);
}

int trap_PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	return syscall(BOTLIB_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

int trap_PC_UnReadToken(int handle)
{
	return syscall(BOTLIB_PC_UNREAD_TOKEN, handle);
}

int trap_BotLibStartFrame(float time)
{
	return 0;
}

int trap_BotLibLoadMap(const char *mapname)
{
	return 0;
}

int trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue)
{
	return 0;
}

int trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3)
{
	return 0;
}

int trap_BotGetSnapshotEntity(int clientNum, int sequence)
{
	return 0;
}

int trap_BotGetServerCommand(int clientNum, char *message, int size)
{
	return syscall(BOTLIB_GET_CONSOLE_MESSAGE, clientNum, message, size);
}

void trap_BotUserCommand(int clientNum, usercmd_t *ucmd)
{
	syscall(BOTLIB_USER_COMMAND, clientNum, ucmd);
}

void trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info)
{
}

int trap_AAS_Initialized(void)
{
	return 0;
}

void trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs)
{
}

float trap_AAS_Time(void)
{
	return 0;
}

void trap_AAS_SetCurrentWorld(int index)
{
}

int trap_AAS_PointAreaNum(vec3_t point)
{
	return 0;
}

int trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas)
{
	return 0;
}

int trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas)
{
	return 0;
}

void trap_AAS_AreaCenter(int areanum, vec3_t center)
{
}

qboolean trap_AAS_AreaWaypoint(int areanum, vec3_t center)
{
	return qtrue;
}

int trap_AAS_PointContents(vec3_t point)
{
	return 0;
}

int trap_AAS_NextBSPEntity(int ent)
{
	return 0;
}

int trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size)
{
	return 0;
}

int trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v)
{
	return 0;
}

int trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value)
{
	return 0;
}

int trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value)
{
	return 0;
}

int trap_AAS_AreaReachability(int areanum)
{
	return 0;
}

int trap_AAS_AreaLadder(int areanum)
{
	return 0;
}

int trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags)
{
	return 0;
}

int trap_AAS_Swimming(vec3_t origin)
{
	return 0;
}

int trap_AAS_PredictClientMovement(void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize)
{
	return 0;
}

void trap_AAS_RT_ShowRoute(vec3_t srcpos, int srcnum, int destnum)
{
}

int trap_AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags, float maxdist, vec3_t distpos)
{
	return 0;
}

int trap_AAS_ListAreasInRange(vec3_t srcpos, int srcarea, float range, int travelflags, float **outareas, int maxareas)
{
	return 0;
}

int trap_AAS_AvoidDangerArea(vec3_t srcpos, int srcarea, vec3_t dangerpos, int dangerarea, float range, int travelflags)
{
	return 0;
}

int trap_AAS_Retreat
(
    // Locations of the danger spots (AAS area numbers)
    int *dangerSpots,
    // The number of danger spots
    int dangerSpotCount,
    vec3_t srcpos,
    int srcarea,
    vec3_t dangerpos,
    int dangerarea,
    // Min range from startpos
    float range,
    // Min range from danger
    float dangerRange,
    int travelflags
)
{
	return 0;
}

int trap_AAS_AlternativeRouteGoals(vec3_t start, vec3_t goal, int travelflags,
                                   aas_altroutegoal_t *altroutegoals, int maxaltroutegoals,
                                   int color)
{
	return 0;
}

void trap_AAS_SetAASBlockingEntity(vec3_t absmin, vec3_t absmax, int blocking)
{
}

void trap_AAS_RecordTeamDeathArea(vec3_t srcpos, int srcarea, int team, int teamCount, int travelflags)
{
}

void trap_EA_Say(int client, char *str)
{
}

void trap_EA_SayTeam(int client, char *str)
{
}

void trap_EA_UseItem(int client, char *it)
{
}

void trap_EA_DropItem(int client, char *it)
{
}

void trap_EA_UseInv(int client, char *inv)
{
}

void trap_EA_DropInv(int client, char *inv)
{
}

void trap_EA_Gesture(int client)
{
}

void trap_EA_Command(int client, char *command)
{
	syscall(BOTLIB_EA_COMMAND, client, command);
}

void trap_EA_SelectWeapon(int client, int weapon)
{
}

void trap_EA_Talk(int client)
{
}

void trap_EA_Attack(int client)
{
}

void trap_EA_Reload(int client)
{
}

void trap_EA_Activate(int client)
{
}

void trap_EA_Respawn(int client)
{
}

void trap_EA_Jump(int client)
{
}

void trap_EA_DelayedJump(int client)
{
}

void trap_EA_Crouch(int client)
{
}

void trap_EA_Walk(int client)
{
}

void trap_EA_MoveUp(int client)
{
}

void trap_EA_MoveDown(int client)
{
}

void trap_EA_MoveForward(int client)
{
}

void trap_EA_MoveBack(int client)
{
}

void trap_EA_MoveLeft(int client)
{
}

void trap_EA_MoveRight(int client)
{
}

void trap_EA_Move(int client, vec3_t dir, float speed)
{
}

void trap_EA_View(int client, vec3_t viewangles)
{
}

void trap_EA_EndRegular(int client, float thinktime)
{
}

void trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input)
{
}

void trap_EA_ResetInput(int client, void *init)
{
}

void trap_EA_Prone(int client)
{
}

int trap_BotLoadCharacter(char *charfile, int skill)
{
	return 0;
}

void trap_BotFreeCharacter(int character)
{
}

float trap_Characteristic_Float(int character, int index)
{
	return 0;
}

float trap_Characteristic_BFloat(int character, int index, float min, float max)
{
	return 0;
}

int trap_Characteristic_Integer(int character, int index)
{
	return 0;
}

int trap_Characteristic_BInteger(int character, int index, int min, int max)
{
	return 0;
}

void trap_Characteristic_String(int character, int index, char *buf, int size)
{
}

int trap_BotAllocChatState(void)
{
	return 0;
}

void trap_BotFreeChatState(int handle)
{
}

void trap_BotQueueConsoleMessage(int chatstate, int type, char *message)
{
}

void trap_BotRemoveConsoleMessage(int chatstate, int handle)
{
}

int trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm)
{
	return 0;
}

int trap_BotNumConsoleMessages(int chatstate)
{
	return 0;
}

void trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7)
{
}

int trap_BotNumInitialChats(int chatstate, char *type)
{
	return 0;
}

int trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7)
{
	return 0;
}

int trap_BotChatLength(int chatstate)
{
	return 0;
}

void trap_BotEnterChat(int chatstate, int client, int sendto)
{
}

void trap_BotGetChatMessage(int chatstate, char *buf, int size)
{
}

int trap_StringContains(char *str1, char *str2, int casesensitive)
{
	return 0;
}

int trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, unsigned long int context)
{
	return 0;
}

void trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size)
{
}

void trap_UnifyWhiteSpaces(char *string)
{
}

void trap_BotReplaceSynonyms(char *string, unsigned long int context)
{
}

int trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname)
{
	return 0;
}

void trap_BotSetChatGender(int chatstate, int gender)
{
}

void trap_BotSetChatName(int chatstate, char *name)
{
}

void trap_BotResetGoalState(int goalstate)
{
}

void trap_BotResetAvoidGoals(int goalstate)
{
}

void trap_BotRemoveFromAvoidGoals(int goalstate, int number)
{
}

void trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal)
{
}

void trap_BotPopGoal(int goalstate)
{
}

void trap_BotEmptyGoalStack(int goalstate)
{
}

void trap_BotDumpAvoidGoals(int goalstate)
{
}

void trap_BotDumpGoalStack(int goalstate)
{
}

void trap_BotGoalName(int number, char *name, int size)
{
}

int trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

int trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

int trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags)
{
	return 0;
}

int trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime)
{
	return 0;
}

int trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

int trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

int trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

int trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

int trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal)
{
	return 0;
}

float trap_BotAvoidGoalTime(int goalstate, int number)
{
	return 0;
}

void trap_BotInitLevelItems(void)
{
}

void trap_BotUpdateEntityItems(void)
{
}

int trap_BotLoadItemWeights(int goalstate, char *filename)
{
}

void trap_BotFreeItemWeights(int goalstate)
{
}

void trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child)
{
}

void trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename)
{
}

void trap_BotMutateGoalFuzzyLogic(int goalstate, float range)
{
}

int trap_BotAllocGoalState(int state)
{
}

void trap_BotFreeGoalState(int handle)
{
}

void trap_BotResetMoveState(int movestate)
{
}

void trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags)
{
}

int trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type)
{
}

void trap_BotResetAvoidReach(int movestate)
{
}

void trap_BotResetLastAvoidReach(int movestate)
{
}

int trap_BotReachabilityArea(vec3_t origin, int testground)
{
}

int trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target)
{
}

int trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target)
{
}

int trap_BotAllocMoveState(void)
{
}

void trap_BotFreeMoveState(int handle)
{
}

void trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove)
{
}

void trap_BotInitAvoidReach(int handle)
{
}

int trap_BotChooseBestFightWeapon(int weaponstate, int *inventory)
{
}

void trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo)
{
}

int trap_BotLoadWeaponWeights(int weaponstate, char *filename)
{
}

int trap_BotAllocWeaponState(void)
{
}

void trap_BotFreeWeaponState(int weaponstate)
{
}

void trap_BotResetWeaponState(int weaponstate)
{
}

int trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child)
{
}

void trap_PbStat(int clientNum, char *category, char *values)
{
	syscall(PB_STAT_REPORT, clientNum, category, values) ;
}

void trap_SendMessage(int clientNum, char *buf, int buflen)
{
	syscall(G_SENDMESSAGE, clientNum, buf, buflen);
}

messageStatus_t trap_MessageStatus(int clientNum)
{
	return syscall(G_MESSAGESTATUS, clientNum);
}
