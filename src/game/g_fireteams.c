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
 * @file g_fireteams.c
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

// What we need....
// invite <clientname|number>
// apply <fireteamname|number>
// create <name>
// leave

// player can only be on single fire team
// only leader can invite
// leaving a team causes the first person to join the team after the leader to become leader
// 32 char limit on fire team names, mebe reduce to 16..

// Application commad overview
//
// clientNum < 0 = special, otherwise is client that the command refers to
// -1 = Application sent
// -2 = Application Failed
// -3 = Application Approved
// -4 = Response sent

// Invitation commad overview
//
// clientNum < 0 = special, otherwise is client that the command refers to
// -1 = Invitation sent
// -2 = Invitation Rejected
// -3 = Invitation Accepted
// -4 = Response sent

// Proposition commad overview
//
// clientNum < 0 = special, otherwise is client that the command refers to
// -1 = Proposition sent
// -2 = Proposition Rejected
// -3 = Proposition Accepted
// -4 = Response sent

// Auto fireteam priv/pub
//
// -1 = ask
// -2 = confirmed
//

// Configstring for each fireteam "\\n\\%NAME%\\l\\%LEADERNUM%\\c\\%CLIENTS%"
// clients "compressed" using hex

#define G_ClientPrintAndReturn(entityNum, text) trap_SendServerCommand(entityNum, "cpm \"" text "\"\n"); return;

// Utility functions
fireteamData_t *G_FindFreeFireteam(void)
{
	int i;

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (!level.fireTeams[i].inuse)
		{
			return &level.fireTeams[i];
		}
	}

	return NULL;
}

team_t G_GetFireteamTeam(fireteamData_t *ft)
{
	if (!ft->inuse)
	{
		return -1;
	}

	if (ft->joinOrder[0] == -1 || !g_entities[(int)ft->joinOrder[0]].client)
	{
		G_Error("G_GetFireteamTeam: Fireteam leader is invalid\n");
	}

	return g_entities[(int)ft->joinOrder[0]].client->sess.sessionTeam;
}

int G_CountTeamFireteams(team_t team)
{
	int i, cnt = 0;

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (G_GetFireteamTeam(&level.fireTeams[i]) == team)
		{
			cnt++;
		}
	}

	return cnt;
}

void G_UpdateFireteamConfigString(fireteamData_t *ft)
{
	char buffer[128];
	int  clnts[2] = { 0, 0 };

	if (!ft->inuse)
	{
		Com_sprintf(buffer, 128, "\\id\\-1");
	}
	else
	{
		int i;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (ft->joinOrder[i] != -1)
			{
				COM_BitSet(clnts, ft->joinOrder[i]);
			}
		}

//		Com_sprintf(buffer, 128, "\\n\\%s\\l\\%i\\c\\%.8x%.8x", ft->name, ft->joinOrder[0], clnts[1], clnts[0]);
//		Com_sprintf(buffer, 128, "\\id\\%i\\l\\%i\\c\\%.8x%.8x", ft->ident - 1, ft->joinOrder[0], clnts[1], clnts[0]);
//		G_Printf(va("%s\n", buffer));
		Com_sprintf(buffer, 128, "\\id\\%i\\l\\%i\\p\\%i\\c\\%.8x%.8x", ft->ident - 1, ft->joinOrder[0], ft->priv, clnts[1], clnts[0]);
	}

	trap_SetConfigstring(CS_FIRETEAMS + (ft - level.fireTeams), buffer);
}

qboolean G_IsOnFireteam(int entityNum, fireteamData_t **teamNum)
{
	int i, j;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_IsOnFireteam: invalid client\n");
	}

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (!level.fireTeams[i].inuse)
		{
			continue;
		}

		for (j = 0; j < MAX_CLIENTS; j++)
		{
			if (level.fireTeams[i].joinOrder[j] == -1)
			{
				break;
			}

			if (level.fireTeams[i].joinOrder[j] == entityNum)
			{
				if (teamNum)
				{
					*teamNum = &level.fireTeams[i];
				}
				return qtrue;
			}
		}
	}

	if (teamNum)
	{
		*teamNum = NULL;
	}
	return qfalse;
}

qboolean G_IsFireteamLeader(int entityNum, fireteamData_t **teamNum)
{
	int i;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_IsFireteamLeader: invalid client\n");
	}

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (!level.fireTeams[i].inuse)
		{
			continue;
		}

		if (level.fireTeams[i].joinOrder[0] == entityNum)
		{
			if (teamNum)
			{
				*teamNum = &level.fireTeams[i];
			}
			return qtrue;
		}
	}

	if (teamNum)
	{
		*teamNum = NULL;
	}
	return qfalse;
}

int G_FindFreeFireteamIdent(team_t team)
{
	qboolean freeIdent[MAX_FIRETEAMS / 2];
	int      i;

	memset(freeIdent, qtrue, sizeof(freeIdent));

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (!level.fireTeams[i].inuse)
		{
			continue;
		}

		if (g_entities[(int)level.fireTeams[i].joinOrder[0]].client->sess.sessionTeam == team)
		{
			freeIdent[level.fireTeams[i].ident - 1] = qfalse;
		}
	}

	for (i = 0; i < (MAX_FIRETEAMS / 2); i++)
	{
		if (freeIdent[i])
		{
			return i;
		}
	}

	// this should never happen
	return -1;
}

// Should be the only function that ever creates a fireteam
void G_RegisterFireteam(/*const char* name,*/ int entityNum)
{
	fireteamData_t *ft;
	gentity_t      *leader;
	int            count, ident;

	if (entityNum < 0 || entityNum >= MAX_CLIENTS)
	{
		G_Error("G_RegisterFireteam: invalid client\n");
	}

	leader = &g_entities[entityNum];
	if (!leader->client)
	{
		G_Error("G_RegisterFireteam: attempting to register a Fireteam to an entity with no client\n");
	}

	if (G_IsOnFireteam(entityNum, NULL))
	{
		G_ClientPrintAndReturn(entityNum, "You are already on a fireteam, leave it first");
	}

/*	if(!name || !*name) {
        G_ClientPrintAndReturn(entityNum, "You must choose a name for your fireteam");
    }*/

	if ((ft = G_FindFreeFireteam()) == NULL)
	{
		G_ClientPrintAndReturn(entityNum, "No free fireteams available");
	}

	if (leader->client->sess.sessionTeam != TEAM_AXIS && leader->client->sess.sessionTeam != TEAM_ALLIES)
	{
		G_ClientPrintAndReturn(entityNum, "Only players on a team can create a fireteam");
	}

	count = G_CountTeamFireteams(leader->client->sess.sessionTeam);
	if (count >= MAX_FIRETEAMS / 2)
	{
		G_ClientPrintAndReturn(entityNum, "Your team already has the maximum number of fireteams allowed");
	}

	ident = G_FindFreeFireteamIdent(leader->client->sess.sessionTeam) + 1;
	if (ident == 0)
	{
		G_ClientPrintAndReturn(entityNum, "Um, something is broken, spoink Gordon");
	}

	// good to go now, i hope!
	ft->inuse = qtrue;
	memset(ft->joinOrder, -1, sizeof(level.fireTeams[0].joinOrder));
	ft->joinOrder[0] = leader - g_entities;
	ft->ident        = ident;

	if (g_autoFireteams.integer)
	{
		ft->priv = qfalse;

		trap_SendServerCommand(entityNum, "aft -1");
		leader->client->pers.autofireteamEndTime = level.time + 20500;
	}
	else
	{
		ft->priv = qfalse;
	}

//	Q_strncpyz(ft->name, name, 32);

#ifdef FEATURE_OMNIBOT
	Bot_Event_FireTeamCreated(entityNum, ft->ident);
	Bot_Event_JoinedFireTeam(leader - g_entities, leader);
#endif

	G_UpdateFireteamConfigString(ft);
}

// only way a client should ever join a fireteam, other than creating one
void G_AddClientToFireteam(int entityNum, int leaderNum)
{
	fireteamData_t *ft;
	int            i;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_AddClientToFireteam: invalid client\n");
	}

	if ((leaderNum < 0 || leaderNum >= MAX_CLIENTS) || !g_entities[leaderNum].client)
	{
		G_Error("G_AddClientToFireteam: invalid client\n");
	}

	if (g_entities[leaderNum].client->sess.sessionTeam != g_entities[entityNum].client->sess.sessionTeam)
	{
		G_ClientPrintAndReturn(entityNum, "You are not on the same team as that fireteam");
	}

	if (!G_IsFireteamLeader(leaderNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "The leader has now left the Fireteam you applied to");
	}

	if (G_IsOnFireteam(entityNum, NULL))
	{
		G_ClientPrintAndReturn(entityNum, "You are already on a fireteam");
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{

		if (i >= MAX_FIRETEAM_MEMBERS)
		{
			G_ClientPrintAndReturn(entityNum, "Too many players already on this Fireteam");
			return;
		}

		if (ft->joinOrder[i] == -1)
		{
			// found a free position
			ft->joinOrder[i] = entityNum;

#ifdef FEATURE_OMNIBOT
			Bot_Event_JoinedFireTeam(entityNum, &g_entities[leaderNum]);
#endif

			G_UpdateFireteamConfigString(ft);

			return;
		}
	}
}

// core: Check if the fireteam contains only bots..
// excludeEntityNum will be excluded from the test (if -1, it has no effect)
// firstHuman returns the joinOrder of the first human found in the fireteam (-1 if none is found)
qboolean G_OnlyBotsInFireteam(fireteamData_t *ft, int excludeEntityNum, int *firstHuman)
{
	int      i;
	qboolean botFound = qfalse;

	*firstHuman = -1;
	if (!ft || !ft->inuse)
	{
		return qfalse;
	}

	for (i = 0; i < MAX_FIRETEAM_MEMBERS && i < g_maxclients.integer; i++)
	{
		if (ft->joinOrder[i] == excludeEntityNum)
		{
			continue;
		}
		if (ft->joinOrder[i] == -1)
		{
			return botFound;
		}
		if (g_entities[(int)(ft->joinOrder[i])].r.svFlags & SVF_BOT)
		{
			botFound = qtrue;
		}
		else
		{
			if (*firstHuman == -1)
			{
				*firstHuman = i;
			}
			return qfalse;
		}
	}
	return botFound;
}

// The only way a client should be removed from a fireteam
void G_RemoveClientFromFireteams(int entityNum, qboolean update, qboolean print)
{
	fireteamData_t *ft;
	int            i, firstHuman;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_RemoveClientFromFireteams: invalid client\n");
	}

	if (G_IsOnFireteam(entityNum, &ft))
	{
		int j;
		for (i = 0; i < MAX_FIRETEAM_MEMBERS && i < g_maxclients.integer; ++i)
		{
			if (ft->joinOrder[i] == entityNum)
			{
				if (i == 0)
				{
					if (ft->joinOrder[1] == -1)
					{
						ft->inuse = qfalse;
						ft->ident = -1;
					}
					else
					{
						// core: only bots left in the fireteam? we disband the fireteam..
						if (G_OnlyBotsInFireteam(ft, entityNum, &firstHuman))
						{
							// empty the fireteam
							for (j = 0; j < g_maxclients.integer - 1; j++)
							{
#ifdef FEATURE_OMNIBOT
								Bot_Event_LeftFireTeam(ft->joinOrder[j]);
#endif
								ft->joinOrder[j] = -1;
							}
							ft->inuse = qfalse;
							ft->ident = -1;
							G_UpdateFireteamConfigString(ft);
							return;
						}
						else
						{
							// core: a bot as FT-leader? we try to pick a human..
							if (g_entities[(int)(ft->joinOrder[1])].r.svFlags & SVF_BOT)
							{
								if (firstHuman != -1)
								{
									// Swap first human with first bot..
									int tmp = ft->joinOrder[1];

									ft->joinOrder[1]          = ft->joinOrder[firstHuman];
									ft->joinOrder[firstHuman] = tmp;
								}
							}
							else
							{
								firstHuman = 1;
							}
							// Inform client of promotion to leader
							if (firstHuman != -1)
							{
								trap_SendServerCommand(ft->joinOrder[firstHuman], "cpm \"You are now the leader of your fireteam\"\n");
							}
						}
					}
				}
				for (j = i; j < g_maxclients.integer - 1; j++)
				{
					ft->joinOrder[j] = ft->joinOrder[j + 1];
				}
				ft->joinOrder[g_maxclients.integer - 1] = -1;

				break;
			}
		}
	}
	else
	{
		return;
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_LeftFireTeam(entityNum);
#endif

	if (print)
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (ft->joinOrder[i] == -1)
			{
				break;
			}

			trap_SendServerCommand(ft->joinOrder[i], va("cpm \"%s has left the Fireteam\"\n", level.clients[entityNum].pers.netname));
		}
	}

	if (update)
	{
		G_UpdateFireteamConfigString(ft);
	}
}

// The only way a client should ever be invitied to join a team
void G_InviteToFireTeam(int entityNum, int otherEntityNum)
{
	fireteamData_t *ft;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_InviteToFireTeam: invalid client\n");
	}

	if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) || !g_entities[otherEntityNum].client)
	{
		G_Error("G_InviteToFireTeam: invalid client\n");
	}

	if (!G_IsFireteamLeader(entityNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
	}

	if (g_entities[entityNum].client->sess.sessionTeam != g_entities[otherEntityNum].client->sess.sessionTeam)
	{
		G_ClientPrintAndReturn(entityNum, "You are not on the same team as the other player");
	}

	if (G_IsOnFireteam(otherEntityNum, NULL))
	{
		G_ClientPrintAndReturn(entityNum, "The other player is already on a fireteam");
	}

	if (g_entities[otherEntityNum].r.svFlags & SVF_BOT)
	{
		// bots auto join
		G_AddClientToFireteam(otherEntityNum, entityNum);
	}
	else
	{
		trap_SendServerCommand(entityNum, va("invitation -1"));
		trap_SendServerCommand(otherEntityNum, va("invitation %i", entityNum));
		g_entities[otherEntityNum].client->pers.invitationClient  = entityNum;
		g_entities[otherEntityNum].client->pers.invitationEndTime = level.time + 20500;
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_InviteFireTeam(entityNum, otherEntityNum);
#endif
}

void G_DestroyFireteam(int entityNum)
{
	fireteamData_t *ft;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_DestroyFireteam: invalid client\n");
	}

	if (!G_IsFireteamLeader(entityNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam\n");
	}

	while (ft->joinOrder[0] != -1)
	{
		if (ft->joinOrder[0] != entityNum)
		{
#ifdef FEATURE_OMNIBOT
			Bot_Event_FireTeamDestroyed(ft->joinOrder[0]);
#endif
			trap_SendServerCommand(ft->joinOrder[0], "cpm \"The Fireteam you are on has been disbanded\"\n");
		}

		G_RemoveClientFromFireteams(ft->joinOrder[0], qfalse, qfalse);
	}

	G_UpdateFireteamConfigString(ft);
}

void G_WarnFireTeamPlayer(int entityNum, int otherEntityNum)
{
	fireteamData_t *ft, *ft2;

	if (entityNum == otherEntityNum)
	{
		return; // ok, stop being silly :p
	}

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_WarnFireTeamPlayer: invalid client\n");
	}

	if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) || !g_entities[otherEntityNum].client)
	{
		G_Error("G_WarnFireTeamPlayer: invalid client\n");
	}

	if (!G_IsFireteamLeader(entityNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
	}

	if ((!G_IsOnFireteam(otherEntityNum, &ft2)) || ft != ft2)
	{
		G_ClientPrintAndReturn(entityNum, "You are not on the same Fireteam as the other player");
	}

	trap_SendServerCommand(otherEntityNum, "cpm \"You have been warned by your Fireteam Commander\n\"");

#ifdef FEATURE_OMNIBOT
	Bot_Event_FireTeam_Warn(entityNum, otherEntityNum);
#endif
}

void G_KickFireTeamPlayer(int entityNum, int otherEntityNum)
{
	fireteamData_t *ft, *ft2;

	if (entityNum == otherEntityNum)
	{
		return; // ok, stop being silly :p
	}

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_KickFireTeamPlayer: invalid client\n");
	}

	if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) || !g_entities[otherEntityNum].client)
	{
		G_Error("G_KickFireTeamPlayer: invalid client\n");
	}

	if (!G_IsFireteamLeader(entityNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
	}

	if ((!G_IsOnFireteam(otherEntityNum, &ft2)) || ft != ft2)
	{
		G_ClientPrintAndReturn(entityNum, "You are not on the same Fireteam as the other player");
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_LeftFireTeam(otherEntityNum);
#endif

	G_RemoveClientFromFireteams(otherEntityNum, qtrue, qfalse);

	G_ClientPrintAndReturn(otherEntityNum, "You have been kicked from the fireteam");
}

// The only way a client should ever apply to join a team
void G_ApplyToFireTeam(int entityNum, int fireteamNum)
{
	gentity_t      *leader;
	fireteamData_t *ft;

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_AddClientToFireteam: invalid client\n");
	}

	if (G_IsOnFireteam(entityNum, NULL))
	{
		G_ClientPrintAndReturn(entityNum, "You are already on a fireteam");
	}

	ft = &level.fireTeams[fireteamNum];
	if (!ft->inuse)
	{
		G_ClientPrintAndReturn(entityNum, "The Fireteam you requested does not exist");
	}

	if (ft->joinOrder[0] < 0 || ft->joinOrder[0] >= MAX_CLIENTS)
	{
		G_Error("G_ApplyToFireTeam: Fireteam leader is invalid\n");
	}

	leader = &g_entities[(int)ft->joinOrder[0]];
	if (!leader->client)
	{
		G_Error("G_ApplyToFireTeam: Fireteam leader client is NULL\n");
	}

	// TEMP
//	G_AddClientToFireteam( entityNum, ft->joinOrder[0] );

	trap_SendServerCommand(entityNum, va("application -1"));
	trap_SendServerCommand(leader - g_entities, va("application %i", entityNum));
	leader->client->pers.applicationClient  = entityNum;
	leader->client->pers.applicationEndTime = level.time + 20000;
}

void G_ProposeFireTeamPlayer(int entityNum, int otherEntityNum)
{
	fireteamData_t *ft;
	gentity_t      *leader;

	if (entityNum == otherEntityNum)
	{
		return; // ok, stop being silly :p
	}

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_ProposeFireTeamPlayer: invalid client\n");
	}

	if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) || !g_entities[otherEntityNum].client)
	{
		G_Error("G_ProposeFireTeamPlayer: invalid client\n");
	}

	if (G_IsOnFireteam(otherEntityNum, NULL))
	{
		G_ClientPrintAndReturn(entityNum, "The other player is already on a fireteam");
	}

	if (!G_IsOnFireteam(entityNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "You are not on a fireteam");
	}

	if (ft->joinOrder[0] == entityNum)
	{
		// you are the leader so just invite them
		G_InviteToFireTeam(entityNum, otherEntityNum);
		return;
	}

	leader = &g_entities[(int)ft->joinOrder[0]];
	if (!leader->client)
	{
		G_Error("G_ProposeFireTeamPlayer: invalid client\n");
	}

	trap_SendServerCommand(entityNum, va("proposition -1"));
	trap_SendServerCommand(leader - g_entities, va("proposition %i %i", otherEntityNum, entityNum));
	leader->client->pers.propositionClient  = otherEntityNum;
	leader->client->pers.propositionClient2 = entityNum;
	leader->client->pers.propositionEndTime = level.time + 20000;

#ifdef FEATURE_OMNIBOT
	Bot_Event_FireTeam_Proposal(leader - g_entities, otherEntityNum);
#endif
}


int G_FireteamNumberForString(const char *name, team_t team)
{
	int i, fireteam = 0;

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (!level.fireTeams[i].inuse)
		{
			continue;
		}

		if (g_entities[(int)level.fireTeams[i].joinOrder[0]].client->sess.sessionTeam != team)
		{
			continue;
		}

		if (!Q_stricmp(bg_fireteamNames[level.fireTeams[i].ident - 1], name))
		{
			fireteam = i + 1;
		}

/*		if(!Q_stricmp(level.fireTeams[i].name, name)) {
            fireteam = i+1;
        }*/
	}

	if (fireteam <= 0)
	{
		fireteam = atoi(name);
	}

	return fireteam;
}

fireteamData_t *G_FindFreePublicFireteam(team_t team)
{
	int i, j;

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		if (!level.fireTeams[i].inuse)
		{
			continue;
		}

		if (g_entities[(int)level.fireTeams[i].joinOrder[0]].client->sess.sessionTeam != team)
		{
			continue;
		}

		if (level.fireTeams[i].priv)
		{
			continue;
		}

		for (j = 0; j < MAX_CLIENTS; j++)
		{
			if (j >= 6 || level.fireTeams[i].joinOrder[j] == -1)
			{
				break;
			}
		}
		if (j >= 6)
		{
			continue;
		}

		return &level.fireTeams[i];
	}

	return NULL;
}

void G_GiveAdminOfFireTeam(int entityNum, int otherEntityNum)
{
	fireteamData_t *ft, *ft2;
	char           tempArray[MAX_CLIENTS];
	int            i, x;

	if (entityNum == otherEntityNum)
	{
		return;
	}

	if ((entityNum < 0 || entityNum >= MAX_CLIENTS) || !g_entities[entityNum].client)
	{
		G_Error("G_KickFireTeamPlayer: invalid client\n");
	}

	if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) || !g_entities[otherEntityNum].client)
	{
		G_Error("G_KickFireTeamPlayer: invalid client\n");
	}

	if (!G_IsFireteamLeader(entityNum, &ft))
	{
		G_ClientPrintAndReturn(entityNum, "You must be a fireteam admin to give admin rights to someone else");
	}

	if ((!G_IsOnFireteam(otherEntityNum, &ft2)) || ft != ft2)
	{
		G_ClientPrintAndReturn(entityNum, "The other player must be on the same fireteam for you to give admin rights to");
	}

	tempArray[0] = otherEntityNum;
	tempArray[1] = entityNum;
	x            = 2;
	for (i = 1; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		if (ft->joinOrder[i] != otherEntityNum || ft->joinOrder[i] == -1)
		{
			tempArray[x++] = ft->joinOrder[i];
			continue;
		}
	}

	for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
	{
		ft->joinOrder[i] = tempArray[i];
	}

	ft->leader = otherEntityNum;

	G_UpdateFireteamConfigString(ft);
	G_ClientPrintAndReturn(otherEntityNum, "You have been given fireteam admin rights");
	G_ClientPrintAndReturn(entityNum, "You have been been stripped of fireteam admin rights");
}


// Command handler
void Cmd_FireTeam_MP_f(gentity_t *ent)
{
	char command[32];
	int  i;

	if (trap_Argc() < 2)
	{
		G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam <create|leave|apply|invite>");
	}

	trap_Argv(1, command, 32);

	if (!Q_stricmp(command, "create"))
	{
		G_RegisterFireteam(ent - g_entities);
	}
	else if (!Q_stricmp(command, "disband"))
	{
		G_DestroyFireteam(ent - g_entities);
	}
	else if (!Q_stricmp(command, "leave"))
	{
		G_RemoveClientFromFireteams(ent - g_entities, qtrue, qtrue);
	}
	else if (!Q_stricmp(command, "apply"))
	{
		char namebuffer[32];
		int  fireteam;

		if (trap_Argc() < 3)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam apply <fireteamname|fireteamnumber>");
		}

		trap_Argv(2, namebuffer, 32);
		fireteam = G_FireteamNumberForString(namebuffer, ent->client->sess.sessionTeam);

		if (fireteam <= 0)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam apply <fireteamname|fireteamnumber>");
		}

		G_ApplyToFireTeam(ent - g_entities, fireteam - 1);
	}
	else if (!Q_stricmp(command, "invite"))
	{
		char namebuffer[32];
		int  clientnum = 0;

		if (trap_Argc() < 3)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam invite <clientname|clientnumber>");
		}

		trap_Argv(2, namebuffer, 32);
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!g_entities[i].inuse || !g_entities[i].client)
			{
				continue;
			}

			if (!Q_stricmp(g_entities[i].client->pers.netname, namebuffer))
			{
				clientnum = i + 1;
			}
		}

		if (clientnum <= 0)
		{
			clientnum = atoi(namebuffer);

			if ((clientnum <= 0 || clientnum > MAX_CLIENTS) || !g_entities[clientnum - 1].inuse || !g_entities[clientnum - 1].client)
			{
				G_ClientPrintAndReturn(ent - g_entities, "Invalid client selected");
			}
		}

		if (clientnum <= 0)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam invite <clientname|clientnumber>");
		}

		G_InviteToFireTeam(ent - g_entities, clientnum - 1);
	}
	else if (!Q_stricmp(command, "warn"))
	{
		char namebuffer[32];
		int  clientnum = 0;

		if (trap_Argc() < 3)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam warn <clientname|clientnumber>");
		}

		trap_Argv(2, namebuffer, 32);
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!g_entities[i].inuse || !g_entities[i].client)
			{
				continue;
			}

			if (!Q_stricmp(g_entities[i].client->pers.netname, namebuffer))
			{
				clientnum = i + 1;
			}
		}

		if (clientnum <= 0)
		{
			clientnum = atoi(namebuffer);

			if ((clientnum <= 0 || clientnum > MAX_CLIENTS) || !g_entities[clientnum - 1].inuse || !g_entities[clientnum - 1].client)
			{
				G_ClientPrintAndReturn(ent - g_entities, "Invalid client selected");
			}
		}

		if (clientnum <= 0)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam warn <clientname|clientnumber>");
		}

		G_WarnFireTeamPlayer(ent - g_entities, clientnum - 1);
	}
	else if (!Q_stricmp(command, "kick"))
	{
		char namebuffer[32];
		int  clientnum = 0;

		if (trap_Argc() < 3)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam kick <clientname|clientnumber>");
		}

		trap_Argv(2, namebuffer, 32);
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!g_entities[i].inuse || !g_entities[i].client)
			{
				continue;
			}

			if (!Q_stricmp(g_entities[i].client->pers.netname, namebuffer))
			{
				clientnum = i + 1;
			}
		}

		if (clientnum <= 0)
		{
			clientnum = atoi(namebuffer);

			if ((clientnum <= 0 || clientnum > MAX_CLIENTS) || !g_entities[clientnum - 1].inuse || !g_entities[clientnum - 1].client)
			{
				G_ClientPrintAndReturn(ent - g_entities, "Invalid client selected");
			}
		}

		if (clientnum <= 0)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam kick <clientname|clientnumber>");
		}

		G_KickFireTeamPlayer(ent - g_entities, clientnum - 1);
	}
	else if (!Q_stricmp(command, "propose"))
	{
		char namebuffer[32];
		int  clientnum = 0;

		if (trap_Argc() < 3)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam propose <clientname|clientnumber>");
		}

		trap_Argv(2, namebuffer, 32);
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!g_entities[i].inuse || !g_entities[i].client)
			{
				continue;
			}

			if (!Q_stricmp(g_entities[i].client->pers.netname, namebuffer))
			{
				clientnum = i + 1;
			}
		}

		if (clientnum <= 0)
		{
			clientnum = atoi(namebuffer);

			if ((clientnum <= 0 || clientnum > MAX_CLIENTS) || !g_entities[clientnum - 1].inuse || !g_entities[clientnum - 1].client)
			{
				G_ClientPrintAndReturn(ent - g_entities, "Invalid client selected");
			}
		}

		if (clientnum <= 0)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam propose <clientname|clientnumber>");
		}

		G_ProposeFireTeamPlayer(ent - g_entities, clientnum - 1);
	}
	else if (!Q_stricmp(command, "privacy"))
	{
		fireteamData_t *ft;
		if (G_IsFireteamLeader(ent - g_entities, &ft))
		{
			if (ft->priv)
			{
				ft->priv = qfalse;
				G_UpdateFireteamConfigString(ft);
				G_ClientPrintAndReturn(ent - g_entities, "Your fireteam is now public");
			}
			else
			{
				ft->priv = qtrue;
				G_UpdateFireteamConfigString(ft);
				G_ClientPrintAndReturn(ent - g_entities, "Your fireteam is now private");
			}
		}
		else
		{
			G_ClientPrintAndReturn(ent - g_entities, "You are not an fireteam admin");
		}
	}
	else if (!Q_stricmp(command, "admin"))
	{
		char namebuffer[32];
		int  clientnum = 0;

		if (trap_Argc() < 3)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam admin <clientname|clientnumber>");
		}

		trap_Argv(2, namebuffer, 32);
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!g_entities[i].inuse || !g_entities[i].client)
			{
				continue;
			}

			if (!Q_stricmp(g_entities[i].client->pers.netname, namebuffer))
			{
				clientnum = i + 1;
			}
		}

		if (clientnum <= 0)
		{
			clientnum = atoi(namebuffer);

			if ((clientnum <= 0 || clientnum > MAX_CLIENTS) || !g_entities[clientnum - 1].inuse || !g_entities[clientnum - 1].client)
			{
				G_ClientPrintAndReturn(ent - g_entities, "Invalid client selected");
			}
		}

		if (clientnum <= 0)
		{
			G_ClientPrintAndReturn(ent - g_entities, "usage: fireteam admin <clientname|clientnumber>");
		}

		G_GiveAdminOfFireTeam(ent - g_entities, clientnum - 1);
	}
}
