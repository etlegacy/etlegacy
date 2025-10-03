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
 * @file g_systemmsg.c
 */

#include "g_local.h"


/**
 * @brief G_NeedEngineers
 * @param[in] team
 * @return
 *
 * @note Unused
 */
qboolean G_NeedEngineers(int team)
{
	int       i;
	gentity_t *e;

	for (i = MAX_CLIENTS, e = &g_entities[MAX_CLIENTS]; i < level.num_entities; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (e->s.eType == ET_CONSTRUCTIBLE_INDICATOR || e->s.eType == ET_EXPLOSIVE_INDICATOR || e->s.eType == ET_TANK_INDICATOR)
		{
			if (e->s.teamNum == 3)
			{
				return qtrue;
			}
			else if (team == TEAM_AXIS)
			{
				if (e->s.teamNum == 2)
				{
					return qtrue;
				}
			}
			else
			{
				if (e->s.teamNum == 1)
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/**
 * @brief G_GetSysMessageNumber
 * @param[in] sysMsg
 * @return
 */
int G_GetSysMessageNumber(const char *sysMsg)
{
	int i;

	for (i = 0; i < SM_NUM_SYS_MSGS; i++)
	{
		if (!Q_stricmp(HQMessages[i].codeString, sysMsg))
		{
			return i;
		}
	}

	return -1;
}

/**
 * @brief G_SendSystemMessage
 * @param[in] message
 * @param[in] team
 */
void G_SendSystemMessage(sysMsg_t message, int team)
{
	gentity_t *other;
	int       *time;
	int       j;

	time = team == TEAM_AXIS ? &level.lastSystemMsgTime[0] : &level.lastSystemMsgTime[1];

	if (*time && (level.time - *time) < 15000) // cvar?
	{
		return;
	}

	*time = level.time;

	for (j = 0; j < level.maxclients; j++)
	{
		other = &g_entities[j];

		if (!other->client || !other->inuse)
		{
			continue;
		}

		if (other->client->sess.sessionTeam != team)
		{
			continue;
		}

		trap_SendServerCommand((int)(other - g_entities), va("vschat %i %i", (int)(other - g_entities), message));
	}
}

/*
 * @brief G_CheckForNeededClasses
 * @note Unused
void G_CheckForNeededClasses(void)
{
    qboolean   playerClasses[NUM_PLAYER_CLASSES - 1][2];
    int        i, team, cnt;
    int        teamCounts[2];
    gentity_t  *ent;
    static int lastcheck;

    Com_Memset(playerClasses, 0, sizeof(playerClasses));
    Com_Memset(teamCounts, 0, sizeof(teamCounts));

    if (lastcheck && (level.time - lastcheck) < 60000)
    {
        return;
    }

    lastcheck = level.time;


    for (i = 0, ent = g_entities; i < level.maxclients; i++, ent++)
    {
        if (!ent->client || !ent->inuse)
        {
            break;
        }

        // don't want spectators
        if (ent->client->sess.sessionTeam < TEAM_AXIS ||
            ent->client->sess.sessionTeam > TEAM_ALLIES)
        {
            continue;
        }

        team = ent->client->sess.sessionTeam == TEAM_AXIS ? 0 : 1;

        if (ent->client->sess.playerType != PC_SOLDIER)
        {
            playerClasses[ent->client->sess.playerType - 1][team] = qtrue;
        }

        teamCounts[team]++;
    }

    // ALLIES
    if (teamCounts[1] > 3)
    {
        if (!playerClasses[PC_ENGINEER - 1])
        {
            playerClasses[PC_ENGINEER - 1][0] = G_NeedEngineers(TEAM_ALLIES) ? 0 : 1;
        }

        cnt = 0;
        for (i = 0; i < NUM_PLAYER_CLASSES; i++)
        {
            if (!playerClasses[i][0])
            {
                cnt++;
            }
        }

        if (cnt != 0)
        {
            cnt = rand() % cnt;

            for (i = 0; i < NUM_PLAYER_CLASSES; i++)
            {
                if (!playerClasses[i][0])
                {
                    if (cnt-- == 0)
                    {
                        G_SendSystemMessage(SM_NEED_MEDIC + i, TEAM_AXIS);
                    }
                }
            }
        }
    }

    // AXIS
    if (teamCounts[0] > 3)
    {
        if (!playerClasses[PC_ENGINEER - 1])
        {
            playerClasses[PC_ENGINEER - 1][1] = G_NeedEngineers(TEAM_AXIS) ? 0 : 1;
        }

        cnt = 0;
        for (i = 0; i < NUM_PLAYER_CLASSES; i++)
        {
            if (!playerClasses[i][1])
            {
                cnt++;
            }
        }

        if (cnt != 0)
        {
            cnt = rand() % cnt;

            for (i = 0; i < NUM_PLAYER_CLASSES; i++)
            {
                if (!playerClasses[i][1])
                {
                    if (cnt-- == 0)
                    {
                        G_SendSystemMessage(SM_NEED_MEDIC + i, TEAM_ALLIES);
                    }
                }
            }
        }
    }
}
*/

/*
 * @brief G_CheckMenDown
 * @note Unused
void G_CheckMenDown(void)
{
    int       alive[2], dead[2];
    gentity_t *ent;
    int       i, team;

    Com_Memset(dead, 0, sizeof(dead));
    Com_Memset(alive, 0, sizeof(alive));

    for (i = 0, ent = g_entities; i < level.maxclients; i++, ent++)
    {
        if (!ent->client || !ent->inuse)
        {
            break;
        }

        // don't want spectators
        if (ent->client->sess.sessionTeam < TEAM_AXIS ||
            ent->client->sess.sessionTeam > TEAM_ALLIES)
        {
            continue;
        }

        team = ent->client->sess.sessionTeam == TEAM_AXIS ? 0 : 1;

        if (ent->health <= 0)
        {
            dead[team]++;
        }
        else
        {
            alive[team]++;
        }
    }

    if (dead[0] + alive[0] >= 4 && (dead[0] >= ((dead[0] + alive[0]) * 0.75f)))
    {
        G_SendSystemMessage(SM_LOST_MEN, TEAM_AXIS);
    }

    if (dead[1] + alive[1] >= 4 && (dead[1] >= ((dead[1] + alive[1]) * 0.75f)))
    {
        G_SendSystemMessage(SM_LOST_MEN, TEAM_ALLIES);
    }
}
*/
