/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		ai_team.c
 *
 * desc:		Wolf bot AI
 *
 *
 *****************************************************************************/

#include "../game/g_local.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"
#include "../botai/inv.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_cmd.h"
#include "ai_team.h"
//
#include "ai_dmnet_mp.h"

// Key AI constants
#include "ai_distances.h"

// Array contains all the info about the team VO and AI coordination
AI_Team_t g_aiTeam[TEAM_NUM_TEAMS];


/*
==================
BotValidTeamLeader
==================
*/
int BotValidTeamLeader( bot_state_t *bs ) {
	if ( !strlen( bs->teamleader ) ) {
		return qfalse;
	}
	if ( ClientFromName( bs->teamleader ) == -1 ) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
BotNumTeamMates
==================
*/
int BotNumTeamMates( bot_state_t *bs, int *list, int maxList ) {
	int i, j, numplayers;

	numplayers = 0;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];
		if ( bs->client == j ) {
			continue;
		}

		if ( !g_entities[j].inuse ) {
			continue;
		}

		if ( !BotSameTeam( bs, j ) ) {
			continue;
		}

		if ( list ) {
			if ( numplayers < maxList ) {
				list[numplayers++] = j;
			}
		} else {
			// calling without a list gives the full count no matter what maxList is set to
			numplayers++;
		}
	}
	return numplayers;
}

/*
==================
BotNumTeamMatesWithTarget
==================
*/
int BotNumTeamMatesWithTarget( bot_state_t *bs, int targetEntity, int *list, int maxList ) {
	int i, j, numplayers;

	numplayers = 0;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( bs->client == j ) {
			continue;
		}

		if ( !g_entities[j].inuse ) {
			continue;
		}

		if ( !BotSameTeam( bs, j ) ) {
			continue;
		}

		if ( g_entities[j].health <= 0 ) {
			continue;
		}

		if ( !botstates[j].inuse ) {
			continue;
		}

		if ( botstates[j].target_goal.entitynum != targetEntity ) {
			continue;
		}

		if ( list ) {
			if ( numplayers < maxList ) {
				list[numplayers++] = j;
			}
		} else {
			// calling without a list gives the full count no matter what maxList is set to
			numplayers++;
		}
	}
	return numplayers;
}

/*
==================
BotNumTeamMatesWithTargetByClass
==================
*/
int BotNumTeamMatesWithTargetByClass( bot_state_t *bs, int targetEntity, int *list, int maxList, int playerType ) {
	int i, j, numplayers;

	numplayers = 0;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];
		if ( bs->client == j ) {
			continue;
		}

		if ( !g_entities[j].inuse ) {
			continue;
		}

		if ( !BotSameTeam( bs, j ) ) {
			continue;
		}

		if ( g_entities[j].health <= 0 ) {
			continue;
		}

		if ( g_entities[j].client->sess.playerType != playerType ) {
			continue;
		}

		if ( !botstates[j].inuse ) {
			continue;
		}

		if ( botstates[j].target_goal.entitynum != targetEntity ) {
			continue;
		}

		if ( list ) {
			if ( numplayers < maxList ) {
				list[numplayers++] = j;
			}
		} else {
			// calling without a list gives the full count no matter what maxList is set to
			numplayers++;
		}
	}
	return numplayers;
}

/*
==================
BotNumTeamMatesWithTargetAndCloser
==================
*/
int BotNumTeamMatesWithTargetAndCloser( bot_state_t *bs, int targetEntity, int targetArea, int *list, int maxList, int playerType ) {
	int i, j, numplayers;
	int ourTime, t;

	numplayers = 0;

	ourTime = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, targetArea, bs->tfl );

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];
		if ( bs->client == j ) {
			continue;
		}

		if ( !g_entities[j].inuse ) {
			continue;
		}

		if ( !botstates[j].inuse ) {
			continue;
		}

		if ( !BotSameTeam( bs, j ) ) {
			continue;
		}

		if ( g_entities[j].health <= 0 ) {
			continue;
		}

		if ( playerType >= 0 && g_entities[j].client->sess.playerType != playerType ) {
			continue;
		}

		if ( botstates[j].target_goal.entitynum != targetEntity ) {
			continue;
		}

		t = botstates[j].inventory[GOAL_TRAVELTIME];
		// trap_AAS_AreaTravelTimeToGoalArea( botstates[j].target_goal.areanum, botstates[j].origin, targetArea, bs->tfl );
		if ( !t ) {
			continue;
		}
		if ( t > ourTime ) {
			continue;
		}

		if ( list ) {
			if ( numplayers < maxList ) {
				list[numplayers++] = j;
			}
		} else {
			// calling without a list gives the full count no matter what maxList is set to
			numplayers++;
		}
	}
	return numplayers;
}

/*
==================
BotSortPlayersByDistance

  returns the distances list
==================
*/
float* BotSortPlayersByDistance( vec3_t target, int *list, int numList ) {
	static float outDistances[MAX_CLIENTS];

	float distances[MAX_CLIENTS], bestDist;
	int i, j, outList[MAX_CLIENTS], best = 0;

	for ( i = 0; i < numList; i++ ) {
		distances[i] = VectorDistanceSquared( g_entities[list[i]].r.currentOrigin, target );
	}

	for ( j = 0; j < numList; j++ ) {
		// find the closest player
		bestDist = -1;
		for ( i = 0; i < numList; i++ ) {
			if ( bestDist < 0 || distances[i] <= bestDist ) {
				best = i;
				bestDist = distances[i];
			}
		}

		outDistances[j] = distances[best];
		distances[best] = -1;
		outList[j] = list[best];
	}

	return outDistances;
}

/*
==================
BotSortPlayersByTraveltime

  returns the distances list
==================
*/
float *BotSortPlayersByTraveltime( int areanum, int *list, int numList ) {
	static float outDistances[MAX_CLIENTS];

	float distances[MAX_CLIENTS], bestDist;
	int i, j, outList[MAX_CLIENTS], best = 0;
	bot_state_t *lbs;

	for ( i = 0; i < numList; i++ ) {
		lbs = &botstates[list[i]];
		if ( lbs && lbs->inuse ) {
			distances[i] = (float)trap_AAS_AreaTravelTimeToGoalArea( BotGetArea( list[i] ), BotGetOrigin( list[i] ), areanum, lbs->tfl );
		} else {
			distances[i] = 0;
		}
	}

	for ( j = 0; j < numList; j++ ) {
		// find the closest player
		bestDist = -1;
		for ( i = 0; i < numList; i++ ) {
			if ( distances[i] < 0 ) {
				continue;
			}
			if ( bestDist < 0 || distances[i] <= bestDist ) {
				best = i;
				bestDist = distances[i];
			}
		}

		outDistances[j] = distances[best];
		distances[best] = -1;
		outList[j] = list[best];
	}

	return outDistances;
}

/*
==================
BotGetEnemyFlagCarrier
==================
*/
gentity_t *BotGetEnemyFlagCarrier( bot_state_t *bs ) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = level.maxclients;
	}

	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		if ( !g_entities[i].inuse ) {
			continue;
		}
		if ( g_entities[i].health <= 0 ) {
			continue;
		}
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		if ( !BotSameTeam( bs, i ) ) {
			if ( g_entities[i].client->ps.powerups[PW_BLUEFLAG] || g_entities[i].client->ps.powerups[PW_REDFLAG] ) {
				return BotGetEntity( i );
			}
		}
	}
	return NULL;
}

/*
==================
BotGetTeamFlagCarrier
==================
*/
int BotGetTeamFlagCarrier( bot_state_t *bs ) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		if ( !g_entities[i].inuse ) {
			continue;
		}
		if ( g_entities[i].health <= 0 ) {
			continue;
		}
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		if ( BotSameTeam( bs, i ) ) {
			if ( g_entities[i].client->ps.powerups[PW_BLUEFLAG] || g_entities[i].client->ps.powerups[PW_REDFLAG] ) {
				return g_entities[i].s.number;
			}
		}
	}
	return -1;
}

/*
==================
BotFlagFlagAtBase

  returns -1 if there is no such flag, qfalse or qtrue otherwise.
==================
*/
int BotFlagAtBase( int team, gentity_t **returnEnt ) {
	gentity_t *ent;
	botStaticEntityEnum_t flags[2] = {BOTSTATICENTITY_CTF_REDFLAG, BOTSTATICENTITY_CTF_BLUEFLAG};
	//
	if ( team > TEAM_SPECTATOR ) {
		return qfalse;
	}
	//
	ent = NULL;
	if ( returnEnt ) {
		*returnEnt = NULL;
	}
	while ( ( ent = BotFindNextStaticEntity( ent, flags[team - 1] ) ) ) {
		if ( ent->flags & FL_DROPPED_ITEM ) {
			continue;
		}
		if ( returnEnt ) {
			*returnEnt = ent;
		}
		// this is the enemy flag, is it at base?
		if ( ent->r.svFlags & SVF_NOCLIENT ) {
			// not at base
			return qfalse;
		} else {
			// at base
			return qtrue;
		}
	}
	//
	return -1;
}

/*
==================
BotClientTravelTimeToGoal
==================
*/
int BotClientTravelTimeToGoal( int client, bot_goal_t *goal ) {
	playerState_t ps;
	int areanum;

	BotAI_GetClientState( client, &ps );
	areanum = BotPointAreaNum( client, ps.origin );
	if ( !areanum ) {
		return 1;
	}
	if ( !BotTravelFlagsForClient( client ) ) {
		return 1;
	}
	return trap_AAS_AreaTravelTimeToGoalArea( areanum, ps.origin, goal->areanum, BotTravelFlagsForClient( client ) );
}

/*
==================
BotSortTeamMatesByBaseTravelTime
==================
*/
int BotSortTeamMatesByBaseTravelTime( bot_state_t *bs, int *teammates, int maxteammates ) {
	return 0;
}

/*
==================
BotSayTeamOrders
==================
*/
void BotSayTeamOrder( bot_state_t *bs, int toclient ) {
	char teamchat[MAX_MESSAGE_SIZE];
	char buf[MAX_MESSAGE_SIZE];
	char name[MAX_NETNAME];

	//if the bot is talking to itself
	if ( bs->client == toclient ) {
		//don't show the message just put it in the console message queue
		trap_BotGetChatMessage( bs->cs, buf, sizeof( buf ) );
		ClientName( bs->client, name, sizeof( name ) );
		Com_sprintf( teamchat, sizeof( teamchat ), "(%s): %s", name, buf );
		trap_BotQueueConsoleMessage( bs->cs, CMS_CHAT, teamchat );
	} else {
		trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
	}
}

/*
==================
BotFindDroppedFlag
==================
*/
qboolean BotFindDroppedFlag( gentity_t **returnEnt ) {
	gentity_t *ent;
	char *flagStr[2] = {"team_CTF_redflag", "team_CTF_blueflag"};
	int i, j;
	//
	ent = BotGetEntity( level.maxclients );
	for ( j = 0; j < level.num_entities - level.maxclients; j++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( !( ent->flags & FL_DROPPED_ITEM ) ) {
			continue;
		}
		if ( ent->classname[0] != 't' || ent->classname[1] != 'e' ) {
			continue;
		}
		for ( i = 0; i < 2; i++ ) {
			if ( Q_stricmp( ent->classname, flagStr[i] ) ) {
				continue;
			}
			// this is a dropped flag
			if ( returnEnt ) {
				*returnEnt = ent;
			}
			return qtrue;
		}
	}
	//
	return qfalse;
}

/*
==================
BotFindSparseDefendArea
==================
*/
int BotFindSparseDefendArea( bot_state_t *bs, bot_goal_t *goal, qboolean force ) {
	vec3_t dSpot, targetPos;
	int numTeam, i, j, t, t2, teammates[64];
	int numareas, numTPos, area, bestArea = 0, bestTime = 0;
	vec3_t tPos[64], waypoints[64];
	bot_state_t *tbs;
	float dist, bestDist, closest, d, maxrange;
	int avoidEnts[] = {BOTSTATICENTITY_FUNC_DOOR, BOTSTATICENTITY_FUNC_DOOR_ROTATING, -1};
	vec3_t avoidPos[128];
	int numAvoidPos;
	gentity_t   *trav, *flagEnt;
	qboolean hasflag, getFurthestFromFlag;
	bot_goal_t flagGoal, flag;
	int flagDestTime = 0;
	bot_state_t *obs;
	vec3_t brushPos, center, vec;
	int list[20], numList;
	int oldestTime = -1, oldest = 0;
	//
	if ( !force && bs->last_SparseDefense > level.time - 400 - rand() % 200 ) {
		return -1;
	}
	bs->last_SparseDefense = level.time;
	//
	// find our team mates that are defending the same goal
	numTeam = BotNumTeamMates( bs, teammates, 64 );
	numTPos = 0;
	for ( i = 0; i < numTeam; i++ ) {
		if ( !g_entities[teammates[i]].inuse || !( g_entities[teammates[i]].r.svFlags & SVF_BOT ) || g_entities[teammates[i]].health <= 0 ) {
			continue;
		}
		if ( teammates[i] == bs->client ) {
			continue;
		}
		tbs = &botstates[teammates[i]];
		if ( tbs->target_goal.entitynum == goal->entitynum ) {
			// same goal
			VectorCopy( tbs->target_goal.origin, tPos[numTPos] );
			numTPos++;
		}
	}
	// get the location of the goal
	if ( goal->entitynum >= 0 ) {
		VectorAdd( g_entities[goal->entitynum].r.absmin, g_entities[goal->entitynum].r.absmax, targetPos );
		VectorScale( targetPos, 0.5, targetPos );
		//if (!trap_AAS_PointAreaNum( targetPos )) {
		//	VectorCopy( goal->origin, targetPos );
		//}
	} else {
		VectorCopy( goal->origin, targetPos );
	}
	//
	hasflag = qfalse;
	getFurthestFromFlag = qfalse;
	if ( !BotSinglePlayer() && !BotCoop() && goal->entitynum >= 0 && goal->entitynum < MAX_CLIENTS && BotCarryingFlag( goal->entitynum ) ) {
		if ( !( g_entities[goal->entitynum].r.svFlags & SVF_BOT ) ) {
			trav = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
			if ( !trav ) {
				trav = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
			}
			if ( trav ) {
				BotGoalForEntity( NULL, trav->s.number, &flagGoal, 0 );
				// find the flag itself, if we are closer the that, then pick a spot further away from the flag
				flagEnt = NULL;
				if ( bs->sess.sessionTeam == TEAM_AXIS ) {
					flagEnt = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_CTF_BLUEFLAG );
				} else {
					flagEnt = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_CTF_REDFLAG );
				}
				if ( flagEnt ) {
					BotGoalForEntity( NULL, flagEnt->s.number, &flag, 0 );
					t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, flag.areanum, bs->tfl );
					if ( t ) {
						t2 = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, flagGoal.areanum, bs->tfl );
						if ( t < t2 ) {
							// we should find a spot further away from the flag itself, rather than closer to the destination.
							// this takes into account the player taking an alternate route, so that the bots lead the player
							// still, rather than trailing behind while they think they are leading the player.
							getFurthestFromFlag = qtrue;
							BotGoalForEntity( NULL, flagEnt->s.number, &flagGoal, 0 );
							hasflag = qtrue;
							flagDestTime = t;
						}
					}
				} else {
					VectorAdd( trav->r.absmin, trav->r.absmax, brushPos );
					VectorScale( brushPos, 0.5, brushPos );
					// find the best goal area
					numList = trap_AAS_BBoxAreas( trav->r.absmin, trav->r.absmax, list, 20 );
					bestDist = -1;
					if ( numList ) {
						oldestTime = -1;
						for ( i = 0; i < numList; i++ ) {
							if ( !trap_AAS_AreaReachability( list[i] ) ) {
								continue;
							}
							t = trap_AAS_AreaTravelTimeToGoalArea( goal->areanum, goal->origin, list[i], bs->tfl );
							if ( t > 0 ) {
								// choose the reachable area closest to the center of the info_objective brush
								trap_AAS_AreaCenter( list[i], center );
								VectorSubtract( brushPos, center, vec );
								vec[2] = 0;
								dist = VectorLength( vec );
								if ( bestDist < 0 || dist < bestDist ) {
									oldestTime = t;
									oldest = list[i];
									bestDist = dist;
								}
							}
						}
					}
					if ( bestDist > -1 ) {
						hasflag = qtrue;
						flagDestTime = oldestTime;
						flagGoal.areanum = oldest;
					}
				}
			}
		} else {    // use the bot's current goal
			obs = &botstates[goal->entitynum];
			flagGoal = obs->target_goal;
			// do they have an alt_goal
			if ( obs->alt_goal.number && obs->alt_goal.entitynum == obs->target_goal.entitynum && obs->alt_goal.number == obs->target_goal.number ) {
				flagGoal = obs->target_goal;    // use the ultimate destination, not the altgoal
				t = trap_AAS_AreaTravelTimeToGoalArea( goal->areanum, goal->origin, flagGoal.areanum, bs->tfl );
				if ( t ) {
					hasflag = qtrue;
					flagDestTime = t;
				}
			}
		}
	}
	//
	if ( !BotSinglePlayer() && !BotCoop() ) {
		maxrange = 700;
		if ( !hasflag && numTPos < 4 ) {
			maxrange = 300.0 + ( ( maxrange - 300.0 ) * numTPos / 4.0 );
		} else if ( hasflag ) {
			maxrange = 800;
		}
	} else {
		// use autonomy range
		maxrange = BotGetMovementAutonomyRange( bs, goal );
		if ( maxrange > 700 ) {
			maxrange = 700;
		}
/*		// if we are following a leader, we need to work out where to position ourselves from them
		if (goal->entitynum >= 0 && goal->entitynum < MAX_CLIENTS) {
			useoffset = qtrue;
			//
			trav = BotGetEntity( goal->entitynum );
			// are they moving?
			if (VectorLengthSquared( trav->client->ps.velocity ) > SQR(10)) {
				VectorNormalize2( trav->client->ps.velocity, offsetVec );
				// low aggression, stay behind

			}
		}
*/
	}
	//
	numareas = trap_AAS_ListAreasInRange( targetPos, goal->areanum, maxrange, bs->tfl & ~( TFL_BARRIERJUMP | TFL_LADDER | TFL_JUMP ), (float **)waypoints, 64 );
	//
	if ( !numareas ) {
		return 0;
	}
	//
	// build a list of avoidEnts
	numAvoidPos = 0;
	for ( i = 0; avoidEnts[i] >= 0; i++ ) {
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, avoidEnts[i] ) ) ) {
			VectorAdd( trav->r.absmin, trav->r.absmax, dSpot );
			VectorScale( dSpot, 0.5, dSpot );
			VectorCopy( dSpot, avoidPos[numAvoidPos] );
			numAvoidPos++;
			if ( numAvoidPos == 128 ) {
				break;
			}
		}
		if ( numAvoidPos == 128 ) {
			break;
		}
	}
	//
	// find the area with the greatest distance from all other teammates, and the goal
	bestDist = -999999;
	closest = 99999;
	for ( i = 0; i < numareas; i++ ) {
		// if this point is outside movement autonomy, ignore it
		if ( !BotPointWithinMovementAutonomy( bs, goal, waypoints[i] ) ) {
			continue;
		}
		//
		dist = 0.5 * VectorDistance( targetPos, waypoints[i] );
		for ( j = 0; j < numTPos; j++ ) {
			d = VectorDistance( waypoints[i], tPos[j] );
			if ( !j || d < closest ) {
				closest = d;
			}
		}
		// add avoidEnts
		for ( j = 0; j < numAvoidPos; j++ ) {
			d = VectorDistance( avoidPos[j], bs->origin );
			if ( d < closest ) {
				closest = d;
			}
		}
		dist += closest * 6;  // weight this higher than the goal distance, since we really want to space each other out
		//
		// if they have the flag, select areas that are closer to their goal
		if ( hasflag ) {
			// get closer or further from goal depending on leader's status
			area = trap_AAS_PointAreaNum( waypoints[i] );
			t = trap_AAS_AreaTravelTimeToGoalArea( area, waypoints[i], flagGoal.areanum, bs->tfl );
			//
			if ( !t ) {
				continue;
			}
			//
			if ( BotCarryingFlag( goal->entitynum ) && !getFurthestFromFlag ) {
				dist += 14 * ( flagDestTime - t );    // closer
			} else {
				dist -= 12 * ( flagDestTime - t );    // further
			}
		}
		//
		if ( dist > bestDist ) {
			area = trap_AAS_PointAreaNum( waypoints[i] );
			if ( area ) {
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, area, bs->tfl );
				if ( t ) {
					bestDist = dist;
					VectorCopy( waypoints[i], dSpot );
					bestArea = area;
					bestTime = t;
				}
			}
		}
	}
	//
	if ( bestArea ) {
		// we have our spot, defend this area now
		goal->areanum = bestArea;
		VectorCopy( dSpot, goal->origin );
		goal->flags |= GFL_DEFEND_CLOSE;
	}
	//
	return bestTime;
}

#define GETTARGETEXPLOSIVE_CACHE_SIZE   32

typedef struct botExplosiveCache_s {
	int list[GETTARGETEXPLOSIVE_CACHE_SIZE];
	int count;
	int listSize;
	qboolean ignore;
	int time;
} botExplosiveCache_t;

botExplosiveCache_t g_botExplosiveCache[2];
botExplosiveCache_t g_botSatchelCache[2];

gentity_t* G_FindMissile( gentity_t* start, weapon_t weap ) {
	int i = start ? ( start - g_entities ) + 1 : 0;
	gentity_t* ent = &g_entities[i];

	for ( ; i < level.num_entities; i++, ent++ ) {
		if ( ent->s.eType != ET_MISSILE ) {
			continue;
		}

		if ( ent->s.weapon != weap ) {
			continue;
		}

		return ent;
	}

	return NULL;
}

gentity_t* G_FindDynamite( gentity_t* start ) {
	return G_FindMissile( start, WP_DYNAMITE );
}

gentity_t* G_FindSmokeBomb( gentity_t* start ) {
	return G_FindMissile( start, WP_SMOKE_BOMB );
}

gentity_t* G_FindLandmine( gentity_t* start ) {
	return G_FindMissile( start, WP_LANDMINE );
}

gentity_t* G_FindSatchels( gentity_t* start ) {
	return G_FindMissile( start, WP_SATCHEL );
}

// Gordon: adding some support functions
// returns qtrue if a construction is under way on this ent, even before it hits any stages
qboolean G_ConstructionBegun( gentity_t* ent ) {
	if ( G_ConstructionIsPartlyBuilt( ent ) ) {
		return qtrue;
	}

	if ( ent->s.angles2[0] ) {
		return qtrue;
	}

	return qfalse;
}

// returns qtrue if all stage are built
qboolean G_ConstructionIsFullyBuilt( gentity_t* ent ) {
	if ( ent->s.angles2[1] != 1 ) {
		return qfalse;
	}
	return qtrue;
}

// returns qtrue if 1 stage or more is built
qboolean G_ConstructionIsPartlyBuilt( gentity_t* ent ) {
	if ( G_ConstructionIsFullyBuilt( ent ) ) {
		return qtrue;
	}

	if ( ent->count2 ) {
		if ( !ent->grenadeFired ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	return qfalse;
}

qboolean G_ConstructionIsDestroyable( gentity_t* ent ) {
	if ( !G_ConstructionIsPartlyBuilt( ent ) ) {
		return qfalse;
	}

	if ( ent->s.angles2[0] ) {
		return qfalse;
	}

	return qtrue;
}

// returns the constructible for this team that is attached to this toi
gentity_t* G_ConstructionForTeam( gentity_t* toi, team_t team ) {
	gentity_t* targ = toi->target_ent;
	if ( !targ || targ->s.eType != ET_CONSTRUCTIBLE ) {
		return NULL;
	}

	if ( targ->spawnflags & 4 ) {
		if ( team == TEAM_ALLIES ) {
			return targ->chain;
		}
	} else if ( targ->spawnflags & 8 ) {
		if ( team == TEAM_AXIS ) {
			return targ->chain;
		}
	}

	return targ;
}

gentity_t* G_IsConstructible( team_t team, gentity_t* toi ) {
	gentity_t* ent;

	if ( !toi || toi->s.eType != ET_OID_TRIGGER ) {
		return NULL;
	}

	if ( !( ent = G_ConstructionForTeam( toi, team ) ) ) {
		return NULL;
	}

	if ( G_ConstructionIsFullyBuilt( ent ) ) {
		return NULL;
	}

	if ( ent->chain && G_ConstructionBegun( ent->chain ) ) {
		return NULL;
	}

	return ent;
}

gentity_t* G_FindDynamiteTargetForTeam( gentity_t* trav, team_t team ) {
	gentity_t* targ;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_OBJECTIVE_INFO ) ) ) {
		if ( !trav->r.linked ) {
			continue;
		}

		if ( ( targ = trav->target_ent ) ) {
			if ( targ->s.eType == ET_EXPLOSIVE ) {
				if ( !( targ->spawnflags & 64 ) ) {  // DY-NO-MITE
					continue;
				}

				if ( !targ->parent ) {
					continue;
				}

				if ( targ->aiInactive & ( 1 << team ) ) {
					continue;
				}

				// Gordon: dont wanna dynamite our own things
				if ( ( targ->parent->spawnflags & AXIS_OBJECTIVE ) && ( team == TEAM_AXIS ) ) {
					continue;
				} else if ( ( targ->parent->spawnflags & ALLIED_OBJECTIVE ) && ( team == TEAM_ALLIES ) ) {
					continue;
				}

				return targ;
			} else if ( targ->s.eType == ET_CONSTRUCTIBLE ) {
				targ = G_ConstructionForTeam( trav, team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS );
				// no constructible for the other team attached to this
				if ( !targ ) {
					continue;
				}

				// dynamite only
				if ( !( targ->spawnflags & 32 ) ) {
					continue;
				}

				// if it isn't built yet, there's nothing to blow up
				if ( !G_ConstructionIsDestroyable( targ ) ) {
					continue;
				}

				// not active from the script
				if ( targ->aiInactive & ( 1 << team ) ) {
					continue;
				}

				return targ;
			}
		}
	}

	return NULL;
}

gentity_t* G_FindSatchelChargeTargetForTeam( gentity_t* trav, team_t team ) {
	gentity_t* targ;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_OBJECTIVE_INFO ) ) ) {
		if ( !trav->r.linked ) {
			continue;
		}

		if ( ( targ = trav->target_ent ) ) {
			if ( targ->s.eType == ET_EXPLOSIVE ) {
				continue;
			} else if ( targ->s.eType == ET_CONSTRUCTIBLE ) {
				targ = G_ConstructionForTeam( trav, team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS );
				// no constructible for the other team attached to this
				if ( !targ ) {
					continue;
				}

				// can satchel charge it
				if ( !( targ->spawnflags & 256 ) ) {
					continue;
				}

				// if it isn't built yet, there's nothing to blow up
				if ( !G_ConstructionIsDestroyable( targ ) ) {
					continue;
				}

				// not active from the script
				if ( targ->aiInactive & ( 1 << team ) ) {
					continue;
				}

				return targ;
			}
		}
	}

	return NULL;
}



/*
==================
BotGetTargetsForSatchelCharge
==================
*/
int BotGetTargetsForSatchelCharge( team_t team, int *list, int listSize, qboolean ignoreSatchelCharge ) {
	int count;
	gentity_t *trav, *dyn;
	vec3_t pos, vec;
	botExplosiveCache_t* pCache = &g_botSatchelCache[team - TEAM_AXIS];

	if ( ( pCache->time == level.time ) && ( pCache->listSize == listSize ) && ( pCache->ignore == ignoreSatchelCharge ) ) {
		// cache hit
		if ( listSize <= GETTARGETEXPLOSIVE_CACHE_SIZE ) {
			memcpy( list, pCache->list, sizeof( int ) * listSize );
			return pCache->count;
		}
	}

	count = 0;
	trav = NULL;
	for ( trav = G_FindSatchelChargeTargetForTeam( NULL, team ); trav; trav = G_FindSatchelChargeTargetForTeam( trav->parent, team ) ) {
		if ( !ignoreSatchelCharge ) {
			// is there already some dynamite planted here?
			VectorAdd( trav->r.absmax, trav->r.absmin, pos );
			VectorScale( pos, 0.5, pos );

			// Gordon: could really do with just having some ref counting for this...
			for ( dyn = G_FindSatchels( NULL ); dyn; dyn = G_FindSatchels( dyn ) ) {
				G_AdjustedDamageVec( trav, dyn->r.currentOrigin, vec );
				if ( ( VectorLengthSquared( vec ) <= SQR( dyn->splashRadius ) ) && CanDamage( trav, dyn->r.currentOrigin ) ) {
					if ( listSize ) {
						if ( list && count >= listSize ) {
							continue;
						}
						continue;   // planted satchel charge was found
					}
				}
			}
		}

		if ( list ) {
			list[count] = trav->s.number;
		}

		count++;
		if ( list && count >= listSize ) {
			break;
		}
	}

	// set the cache items
	if ( list && listSize <= GETTARGETEXPLOSIVE_CACHE_SIZE ) {
		memcpy( pCache->list, list, sizeof( int ) * listSize );
		pCache->count =             count;
		pCache->ignore =            ignoreSatchelCharge;
		pCache->listSize =          listSize;
		pCache->time =              level.time;
	}

	return count;
}


/*
==================
BotGetTargetExplosives
==================
*/
int BotGetTargetExplosives( team_t team, int *list, int listSize, qboolean ignoreDynamite ) {
	int count;
	gentity_t *trav, *dyn;
	vec3_t pos, vec;
	botExplosiveCache_t* pCache = &g_botExplosiveCache[team - TEAM_AXIS];

	if ( ( pCache->time == level.time ) && ( pCache->listSize == listSize ) && ( pCache->ignore == ignoreDynamite ) ) {
		// cache hit
		if ( listSize <= GETTARGETEXPLOSIVE_CACHE_SIZE ) {
			memcpy( list, pCache->list, sizeof( int ) * listSize );
			return pCache->count;
		}
	}

	count = 0;
	trav = NULL;
	for ( trav = G_FindDynamiteTargetForTeam( NULL, team ); trav; trav = G_FindDynamiteTargetForTeam( trav->parent, team ) ) {
		if ( !ignoreDynamite ) {
			// is there already some dynamite planted here?
			VectorAdd( trav->r.absmax, trav->r.absmin, pos );
			VectorScale( pos, 0.5, pos );

			for ( dyn = G_FindDynamite( NULL ); dyn; dyn = G_FindDynamite( dyn ) ) {
				G_AdjustedDamageVec( trav, dyn->r.currentOrigin, vec );
				if ( ( VectorLengthSquared( vec ) <= SQR( dyn->splashRadius ) ) && CanDamage( trav, dyn->r.currentOrigin ) ) {
					// Gordon: checking damage makes no sense in this case, as these are one hit wonders
					if ( listSize ) {
						if ( list && count >= listSize ) {
							continue;
						}
						continue;   // planted dynamite was found
					}
				}
			}
		}

		if ( list ) {
			if ( trav->s.eType == ET_EXPLOSIVE ) {
				list[count] = trav->parent->s.number;
			} else {
				list[count] = trav->s.number;
			}
		}

		count++;
		if ( list && count >= listSize ) {
			break;
		}
	}

	// set the cache items
	if ( list && listSize <= GETTARGETEXPLOSIVE_CACHE_SIZE ) {
		memcpy( pCache->list, list, sizeof( int ) * listSize );
		pCache->count =             count;
		pCache->ignore =            ignoreDynamite;
		pCache->listSize =          listSize;
		pCache->time =              level.time;
	}

	return count;
}

int GetTargetExplosives( team_t team, qboolean ignoreDynamite ) {
	return BotGetTargetExplosives( team, NULL, 0, ignoreDynamite );
}

/*
==================
BotGetTargetDynamite
==================
*/
int BotGetTargetDynamite( int *list, int listSize, gentity_t* target ) {
	gentity_t *dyn, *trav;
	vec3_t vec;
	int count = 0;
	team_t team;

	for ( dyn = G_FindDynamite( NULL ); dyn; dyn = G_FindDynamite( dyn ) ) {
		// RF, if the dynamite is unarmed, ignore
		if ( dyn->s.teamNum >= 4 ) {
			continue;
		}
		for ( team = TEAM_AXIS; team <= TEAM_ALLIES; team++ ) {
			vec3_t mins, maxs;
			VectorAdd( dyn->r.currentOrigin, dyn->r.mins, mins );
			VectorAdd( dyn->r.currentOrigin, dyn->r.maxs, maxs );

			if ( target ) {
				if ( target->s.eType == ET_EXPLOSIVE ) {
					if ( !target->parent ) {
						continue;
					}

					if ( BG_BBoxCollision( dyn->r.absmin, dyn->r.absmax, target->parent->r.absmin, target->parent->r.absmax ) ) {
						if ( list ) {
							list[count] = dyn->s.number;
						}
						count++;
						break;
					}
				} else {
					G_AdjustedDamageVec( target, dyn->r.currentOrigin, vec );
					if ( ( VectorLengthSquared( vec ) <= SQR( dyn->splashRadius ) ) && CanDamage( target, dyn->r.currentOrigin ) ) {
						if ( list ) {
							list[count] = dyn->s.number;
						}
						count++;
						break;
					}
				}
			} else {
				for ( trav = G_FindDynamiteTargetForTeam( NULL, team ); trav; trav = G_FindDynamiteTargetForTeam( trav->parent, team ) ) {
					if ( trav->s.eType == ET_EXPLOSIVE ) {
						if ( !trav->parent ) {
							continue;
						}

						if ( BG_BBoxCollision( dyn->r.absmin, dyn->r.absmax, trav->parent->r.absmin, trav->parent->r.absmax ) ) {
							if ( list ) {
								list[count] = dyn->s.number;
							}
							count++;
							break;
						}
					} else {
						G_AdjustedDamageVec( trav, dyn->r.currentOrigin, vec );
						if ( ( VectorLengthSquared( vec ) <= SQR( dyn->splashRadius ) ) && CanDamage( trav, dyn->r.currentOrigin ) ) {
							if ( list ) {
								list[count] = dyn->s.number;
							}
							count++;
							break;
						}
					}
				}
			}

			if ( list && count >= listSize ) {
				break;
			}
		}
	}

	return count;
}

/*
==================
BotGetConstructibles
==================
*/
int BotGetConstructibles( team_t team, int *list, int listSize, qboolean ignoreBuilt ) {
	gentity_t *trav, *targ;
	int count = 0;

	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_OBJECTIVE_INFO ) ) ) {
		if ( !trav->r.linked ) {
			continue;
		}

		targ = G_ConstructionForTeam( trav, team );
		if ( !targ ) {
			continue;
		}

		if ( ignoreBuilt && G_ConstructionIsFullyBuilt( targ ) ) {
			continue;
		}


		if ( listSize >= 0 ) {
			if ( list ) {
				list[count] = targ->s.number;
			}
			count++;
		}

		if ( listSize > 0 && list && count >= listSize ) {
			break;
		}
	}

	return count;
}

/*
==================
BotNumTeamMembers
==================
*/
int BotNumTeamMembers( int team ) {
	gclient_t *cl;
	int i, cnt;

	cl = &level.clients[0];
	cnt = 0;
	for ( i = 0; i < level.maxclients; i++, cl++ ) {
		if ( !( cl->pers.connected == CON_CONNECTED ) ) {
			continue;
		}
		if ( cl->sess.sessionTeam == team ) {
			cnt++;
		}
	}
	//
	return cnt;
}

/*
==================
BotNumTeamClasses
==================
*/
int BotNumTeamClasses( team_t team, int mpClass, int ignore ) {
	gclient_t *cl;
	int i, cnt, j;

	cnt = 0;
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( j == ignore ) {
			continue;
		}

		if ( !g_entities[j].inuse ) {
			continue;
		}

		cl = &level.clients[j];
		if ( cl->sess.sessionTeam != team ) {
			continue;
		}

		if ( g_entities[j].r.svFlags & SVF_BOT ) {
			if ( BotIsDead( &botstates[j] ) ) {
				if ( botstates[j].mpClass != mpClass ) {
					continue;
				}
			} else {
				if ( cl->sess.playerType != mpClass ) {
					continue;
				}
			}
		} else {
			if ( cl->ps.pm_flags & PMF_LIMBO ) {
				if ( cl->sess.latchPlayerType != mpClass ) {
					continue;
				}
			} else {
				if ( cl->sess.playerType != mpClass ) {
					continue;
				}
			}
		}

		cnt++;
	}

	return cnt;
}

/*
==================
BotCheckNeedEngineer
==================
*/
qboolean BotCheckNeedEngineer( bot_state_t *bs, team_t team ) {
	static int teamLastTime[2] = { 0, 0 };
	int* lastTime = &teamLastTime[ team == TEAM_AXIS ? 0 : 1 ];

	// Gordon: want a couple at the start
	if ( level.time - level.startTime < 20000 ) {
		if ( BotNumTeamClasses( team, PC_ENGINEER, bs->client ) < 2 ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if ( *lastTime && *lastTime <= level.time && *lastTime > level.time - 10000 ) {
		return qfalse;  // only check every 10 seconds, to prevent everyone from changing class at once
	}
	*lastTime = level.time;

	if ( bs->last_fire > level.time - 10000 ) {
		return qfalse;
	}

	// flag checks
	if ( BotCarryingFlag( bs->client ) || ( bs->leader > -1 && BotCarryingFlag( bs->leader ) ) ) { // dont abandon a leader that needs us
		return qfalse;
	}

	if ( BotNumTeamClasses( team, PC_ENGINEER, bs->client ) > 0 ) {
		return qfalse;
	}

	if ( !BotGetTargetExplosives( team, NULL, 0, qfalse ) && !BotGetConstructibles( team, NULL, 0, qtrue ) ) {
		return qfalse;
	}

	//
	// we need engineer's
	return qtrue;
}

/*
==================
BotSuggestClass
==================
*/
int BotSuggestClass( bot_state_t *bs, team_t team ) {
	gclient_t *cl;
	int i;
	int numRequired[NUM_PLAYER_CLASSES];
//	int		list[10], numList;
	int numTeamMembers;

	int classPriority[NUM_PLAYER_CLASSES] = {PC_ENGINEER, PC_SOLDIER, PC_MEDIC, PC_FIELDOPS, PC_COVERTOPS};
//	char	userinfo[MAX_INFO_STRING], *str;
	float bestDiff, diff;
	int bestClass;
	qboolean needEngineers = qfalse;
	int lastMg42Death = ( team == TEAM_ALLIES ? level.alliesMG42Counter : level.axisMG42Counter );

	cl = &level.clients[bs->entitynum];

	// if we have a specified class, then use that
/*	trap_GetUserinfo( bs->client, userinfo, sizeof(userinfo) );
	if ((str = Info_ValueForKey( userinfo, "pClass" )) && strlen(str)) {
		i = atoi(str);
		if (i > 0) {
			return i-1;
		}
	}*/

	// quick test for engineers
	if ( BotCheckNeedEngineer( bs, team ) ) {
		return PC_ENGINEER;
	}

	memset( numRequired, 0, sizeof( numRequired ) );
	numTeamMembers = BotNumTeamMembers( team );

	// do we need to destroy barriers?
	if ( BotGetTargetExplosives( team, NULL, 0, qtrue ) || BotGetConstructibles( team, NULL, 0, qtrue ) ) {
		needEngineers = qtrue;
	} else {
		numRequired[PC_ENGINEER] = 0;
	}

	if ( needEngineers ) {
		if ( numTeamMembers <= 3 ) {
			numRequired[PC_ENGINEER] = 1;
		} else {
			numRequired[PC_ENGINEER] = (int)ceil( numTeamMembers / 3.f );
		}
	}

	// we should have at least one of each other class
	numTeamMembers -= numRequired[PC_ENGINEER];

	numRequired[PC_SOLDIER] =   numTeamMembers / 2.f > 1 ? numTeamMembers / 2.f : 1;
	numRequired[PC_COVERTOPS] = numRequired[PC_FIELDOPS] = numRequired[PC_MEDIC] = ( numTeamMembers / 6.f ) > 1 ? ( numTeamMembers / 6.f ) : 1;

	//
	// special cases
	if ( lastMg42Death && ( ( level.time - lastMg42Death ) < ( 30 * 1000 ) ) ) {
		// use panzers to clear mg42 nests
		numRequired[PC_SOLDIER] = ( numTeamMembers - 3 > numRequired[PC_SOLDIER] ? numTeamMembers - 3 : numRequired[PC_SOLDIER] );
		numRequired[PC_COVERTOPS] = numRequired[PC_FIELDOPS] = numRequired[PC_MEDIC] = 1;
	}

	// allocate classes in order of priority
	bestDiff = 1.0;
	bestClass = -1;
	for ( i = 0; i < NUM_PLAYER_CLASSES; i++ ) {
		if ( !numRequired[classPriority[i]] ) {
			continue;
		}
		diff = BotNumTeamClasses( team, classPriority[i], bs->client ) / (float)numRequired[classPriority[i]];
		if ( bestDiff > diff ) {
			// we need one of these
			bestDiff = diff;
			bestClass = classPriority[i];
		}
	}

	if ( bestClass >= 0 ) {
		return bestClass;
	}

	if ( level.time < level.startTime + 20000 ) {
		return PC_SOLDIER;
	}

	// Gordon: FIXME: balanace it? and why not engineer, they ARE useful outside of constructing stuff etc...
	// not important, so just pick at random
	while ( ( i = rand() % NUM_PLAYER_CLASSES ) == PC_ENGINEER ) ;

	return i;
}

/*
==================
BotSuggestWeapon
==================
*/
int BotSuggestWeapon( bot_state_t *bs, team_t team ) {
	int i, r;
	gentity_t *trav;
	qboolean noSniper = qfalse;
	int lastMg42Death = ( team == TEAM_ALLIES ? level.alliesMG42Counter : level.axisMG42Counter );

//	char	userinfo[MAX_INFO_STRING], *str;
	//
	// if we have a specified weapon, then use that
/*	trap_GetUserinfo( bs->client, userinfo, sizeof(userinfo) );
	if ((str = Info_ValueForKey( userinfo, "pWeapon" )) && Q_stricmp(str, "ANY") && (i = atoi(str))) {
		return i;
	}*/

	//
	// special cases
	if ( level.captureFlagMode ) {
		// if defending team, try to get some snipers at the start
		if ( ( bs->mpClass == PC_COVERTOPS ) && ( rand() % 3 ) && ( level.time < level.startTime + 120000 ) && ( bs->mpTeam != level.attackingTeam ) ) {
			r = rand() % 2;
			switch ( r ) {
			default:    return WP_FG42;
			case 1:
				switch ( bs->mpTeam ) {
				case TEAM_AXIS:
					return WP_K43;
				default:
					return WP_GARAND;
				}
			}
		}
		// if attacking, use panzers to clear defenses
		if ( ( bs->mpClass == PC_SOLDIER ) && ( rand() % 3 ) && ( level.time < level.startTime + 30000 ) && ( bs->mpTeam == level.attackingTeam ) ) {
			//return 4; // panzer
			return WP_PANZERFAUST;
		}
		// if GT_WOLF, and checkpoint is owned by opposition, then no point in having snipers
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_CHECKPOINT ) ) ) {
			if ( trav->count == level.attackingTeam ) {
				break;
			}
		}
		if ( trav ) {
			// checkpoints are not defended
			noSniper = qtrue;
		}
	}

	if ( !noSniper && !( rand() % 2 ) && ( bs->mpClass == PC_COVERTOPS ) ) {
		// are there sniper spots available?
		i = BotBestSniperSpot( bs );
		if ( i >= 0 ) {
			// sniper spots available!
			g_entities[i].missionLevel = 0; // dont avoid this spot

			switch ( bs->mpTeam ) {
			case TEAM_AXIS:
				return WP_K43;
			default:
				return WP_GARAND;
			}
		}
	}
	// else choose at random

	switch ( bs->mpClass ) {
	case PC_ENGINEER:
		r = rand() % 2;
		switch ( bs->mpTeam ) {
		case TEAM_AXIS:
			switch ( r ) {
			default:    return WP_KAR98;
			case 1:     return WP_MP40;
			}
		default:
			switch ( r ) {
			default:    return WP_CARBINE;
			case 1:     return WP_THOMPSON;
			}
		}
	case PC_SOLDIER:
		if ( lastMg42Death && ( level.time - lastMg42Death ) < ( 60 * 1000 ) ) {
			// Gordon: much greater chance of panzerfaust
			r = rand() % 12;
		} else {
			r = rand() % 3;
		}
		switch ( r ) {
		default:        return WP_PANZERFAUST;
		case 1:     return WP_FLAMETHROWER;
		case 2:     return WP_MOBILE_MG42;
			//case 3:	return WP_MORTAR;		// they dont understand this yet
		case 0:
			switch ( bs->mpTeam ) {
			case TEAM_AXIS:
				return WP_MP40;
			default:
				return WP_THOMPSON;
			}
		}
		break;
	case PC_COVERTOPS:
		r = rand() % 3;
		switch ( r ) {
		default:    return WP_FG42;
		case 1:
			switch ( bs->mpTeam ) {
			case TEAM_AXIS:
				return WP_K43;
			default:
				return WP_GARAND;
			}
		case 2:     return WP_STEN;
		}
		break;
	}

	switch ( bs->mpTeam ) {
	case TEAM_AXIS:
		return WP_MP40;
	default:
		return WP_THOMPSON;
	}
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_BothFlagsNotAtBase( bot_state_t *bs ) {
	int numteammates, defenders, attackers, i, other;
	int teammates[MAX_CLIENTS];
	char name[MAX_NETNAME], carriername[MAX_NETNAME];

	numteammates = BotSortTeamMatesByBaseTravelTime( bs, teammates, sizeof( teammates ) );
	//different orders based on the number of team mates
	switch ( bs->numteammates ) {
	case 1: break;
	case 2:
	{
		//tell the one not carrying the flag to attack the enemy base
		if ( teammates[0] != bs->flagcarrier ) {
			other = teammates[0];
		} else { other = teammates[1];}
		ClientName( other, name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, other );
		break;
	}
	case 3:
	{
		//tell the one closest to the base not carrying the flag to accompany the flag carrier
		if ( teammates[0] != bs->flagcarrier ) {
			other = teammates[0];
		} else { other = teammates[1];}
		ClientName( other, name, sizeof( name ) );
		ClientName( bs->flagcarrier, carriername, sizeof( carriername ) );
		if ( bs->flagcarrier == bs->client ) {
			BotAI_BotInitialChat( bs, "cmd_accompanyme", name, NULL );
		} else {
			BotAI_BotInitialChat( bs, "cmd_accompany", name, carriername, NULL );
		}
		BotSayTeamOrder( bs, other );
		//tell the one furthest from the the base not carrying the flag to get the enemy flag
		if ( teammates[2] != bs->flagcarrier ) {
			other = teammates[2];
		} else { other = teammates[1];}
		ClientName( other, name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, other );
		break;
	}
	default:
	{
		defenders = (int) ( float ) numteammates * 0.4 + 0.5;
		attackers = (int) ( float ) numteammates * 0.5 + 0.5;
		ClientName( bs->flagcarrier, carriername, sizeof( carriername ) );
		for ( i = 0; i < defenders; i++ ) {
			//
			if ( teammates[i] == bs->flagcarrier ) {
				continue;
			}
			//
			ClientName( teammates[i], name, sizeof( name ) );
			if ( bs->flagcarrier == bs->client ) {
				BotAI_BotInitialChat( bs, "cmd_accompanyme", name, NULL );
			} else {
				BotAI_BotInitialChat( bs, "cmd_accompany", name, carriername, NULL );
			}
			BotSayTeamOrder( bs, teammates[i] );
		}
		for ( i = 0; i < attackers; i++ ) {
			//
			if ( teammates[numteammates - i - 1] == bs->flagcarrier ) {
				continue;
			}
			//
			ClientName( teammates[numteammates - i - 1], name, sizeof( name ) );
			BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
			BotSayTeamOrder( bs, teammates[numteammates - i - 1] );
		}
		//
		break;
	}
	}
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_FlagNotAtBase( bot_state_t *bs ) {
	int numteammates, defenders, attackers, i;
	int teammates[MAX_CLIENTS];
	char name[MAX_NETNAME];

	numteammates = BotSortTeamMatesByBaseTravelTime( bs, teammates, sizeof( teammates ) );
	//different orders based on the number of team mates
	switch ( bs->numteammates ) {
	case 1: break;
	case 2:
	{
		//the one closest to the base will defend the base
		ClientName( teammates[0], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, teammates[0] );
		//the other will get the flag
		ClientName( teammates[1], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, teammates[1] );
		break;
	}
	case 3:
	{
		//the one closest to the base will defend the base
		ClientName( teammates[0], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, teammates[0] );
		//the other two get the flag
		ClientName( teammates[1], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, teammates[1] );
		//
		ClientName( teammates[2], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, teammates[2] );
		break;
	}
	default:
	{
		defenders = (int) ( float ) numteammates * 0.3 + 0.5;
		attackers = (int) ( float ) numteammates * 0.5 + 0.5;
		for ( i = 0; i < defenders; i++ ) {
			//
			ClientName( teammates[i], name, sizeof( name ) );
			BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
			BotSayTeamOrder( bs, teammates[i] );
		}
		for ( i = 0; i < attackers; i++ ) {
			//
			ClientName( teammates[numteammates - i - 1], name, sizeof( name ) );
			BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
			BotSayTeamOrder( bs, teammates[numteammates - i - 1] );
		}
		//
		break;
	}
	}
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_EnemyFlagNotAtBase( bot_state_t *bs ) {
	int numteammates, defenders, attackers, i, other;
	int teammates[MAX_CLIENTS];
	char name[MAX_NETNAME], carriername[MAX_NETNAME];

	numteammates = BotSortTeamMatesByBaseTravelTime( bs, teammates, sizeof( teammates ) );
	//different orders based on the number of team mates
	switch ( numteammates ) {
	case 1: break;
	case 2:
	{
		//tell the one not carrying the flag to defend the base
		if ( teammates[0] == bs->flagcarrier ) {
			other = teammates[1];
		} else { other = teammates[0];}
		ClientName( other, name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, other );
		break;
	}
	case 3:
	{
		//tell the one closest to the base not carrying the flag to defend the base
		if ( teammates[0] != bs->flagcarrier ) {
			other = teammates[0];
		} else { other = teammates[1];}
		ClientName( other, name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, other );
		//tell the one furthest from the base not carrying the flag to accompany the flag carrier
		if ( teammates[2] != bs->flagcarrier ) {
			other = teammates[2];
		} else { other = teammates[1];}
		ClientName( other, name, sizeof( name ) );
		ClientName( bs->flagcarrier, carriername, sizeof( carriername ) );
		if ( bs->flagcarrier == bs->client ) {
			BotAI_BotInitialChat( bs, "cmd_accompanyme", name, NULL );
		} else {
			BotAI_BotInitialChat( bs, "cmd_accompany", name, carriername, NULL );
		}
		BotSayTeamOrder( bs, other );
		break;
	}
	default:
	{
		//40% will defend the base
		defenders = (int) ( float ) numteammates * 0.4 + 0.5;
		//50% accompanies the flag carrier
		attackers = (int) ( float ) numteammates * 0.5 + 0.5;
		for ( i = 0; i < defenders; i++ ) {
			//
			if ( teammates[i] == bs->flagcarrier ) {
				continue;
			}
			ClientName( teammates[i], name, sizeof( name ) );
			BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
			BotSayTeamOrder( bs, teammates[i] );
		}
		ClientName( bs->flagcarrier, carriername, sizeof( carriername ) );
		for ( i = 0; i < attackers; i++ ) {
			//
			if ( teammates[numteammates - i - 1] == bs->flagcarrier ) {
				continue;
			}
			//
			ClientName( teammates[numteammates - i - 1], name, sizeof( name ) );
			if ( bs->flagcarrier == bs->client ) {
				BotAI_BotInitialChat( bs, "cmd_accompanyme", name, NULL );
			} else {
				BotAI_BotInitialChat( bs, "cmd_accompany", name, carriername, NULL );
			}
			BotSayTeamOrder( bs, teammates[numteammates - i - 1] );
		}
		//
		break;
	}
	}
}


/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_BothFlagsAtBase( bot_state_t *bs ) {
	int numteammates, defenders, attackers, i;
	int teammates[MAX_CLIENTS];
	char name[MAX_NETNAME];
//	char buf[MAX_MESSAGE_SIZE];

	numteammates = BotSortTeamMatesByBaseTravelTime( bs, teammates, sizeof( teammates ) );
	//different orders based on the number of team mates
	switch ( numteammates ) {
	case 1: break;
	case 2:
	{
		//the one closest to the base will defend the base
		ClientName( teammates[0], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, teammates[0] );
		//the other will get the flag
		ClientName( teammates[1], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, teammates[1] );
		break;
	}
	case 3:
	{
		//the one closest to the base will defend the base
		ClientName( teammates[0], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, teammates[0] );
		//the second one closest to the base will defend the base
		ClientName( teammates[1], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
		BotSayTeamOrder( bs, teammates[1] );
		//the other will get the flag
		ClientName( teammates[2], name, sizeof( name ) );
		BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
		BotSayTeamOrder( bs, teammates[2] );
		break;
	}
	default:
	{
		defenders = (int) ( float ) numteammates * 0.5 + 0.5;
		attackers = (int) ( float ) numteammates * 0.3 + 0.5;
		for ( i = 0; i < defenders; i++ ) {
			//
			ClientName( teammates[i], name, sizeof( name ) );
			BotAI_BotInitialChat( bs, "cmd_defendbase", name, NULL );
			BotSayTeamOrder( bs, teammates[i] );
		}
		for ( i = 0; i < attackers; i++ ) {
			//
			ClientName( teammates[numteammates - i - 1], name, sizeof( name ) );
			BotAI_BotInitialChat( bs, "cmd_getflag", name, NULL );
			BotSayTeamOrder( bs, teammates[numteammates - i - 1] );
		}
		//
		break;
	}
	}
}


/*
==================
BotTeamOrders
==================
*/
void BotTeamOrders( bot_state_t *bs ) {
	//no teamplay orders at this time
}


// Start - TAT 9/20/2002
// Covert Ops bot searches for a body to steal the uniform from
qboolean BotClass_CovertOpsCheckDisguises( bot_state_t *bs, int maxTravel, bot_goal_t *goal ) {
	gentity_t *trav;
	int t, area, best = -1, bestTravel, bestArea = -1; // Arnout: bestArea was not initialized
	bot_goal_t target;
	int list[32], numList;
	vec3_t loc;

	//if we are not covert ops
	if ( bs->sess.playerType != PC_COVERTOPS ) {
		return qfalse;
	}

	bestTravel = maxTravel;

	trav = NULL;
	// loop through all the corpses in the world
	while ( ( trav = G_Find( trav, FOFS( classname ), "corpse" ) ) )
	{
		// if on the same team
		if ( OnSameTeam( BotGetEntity( bs->client ), trav ) ) {
			continue;
		}

		// make sure there isn't already a covertop snagging their getup
		numList = BotNumTeamMatesWithTargetByClass( bs, trav->s.number, list, 32, PC_COVERTOPS );
		if ( numList ) {
			numList = BotReduceListByTravelTime( list, numList, BotGetOrigin( trav->s.number ), BotGetArea( trav->s.number ), BotTravelTimeToEntity( bs, trav->s.number ) );
		}
		if ( numList ) {
			continue;
		}
		//
		t = 0;

		VectorCopy( trav->r.currentOrigin, loc );
		loc[2] += 30;

		// check the route to them
		area = trap_AAS_PointAreaNum( loc );
		if ( area ) {
			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, area, bs->tfl );
		}

		if ( t && t < bestTravel ) {
			BotClearGoal( &target );
			target.entitynum = trav->s.number;
			target.areanum = area;
			VectorCopy( trav->r.mins, target.mins );
			VectorCopy( trav->r.maxs, target.maxs );
			VectorCopy( loc, target.origin );
			//
			if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_HIGH ) ) {
				best = trav->s.number;
				bestTravel = t;
				bestArea = area;
			}
		}
	}

	// did we find someone
	if ( best >= 0 ) {
		// yes, so copy the goal over
		*goal = target;

		// and return success
		return qtrue;
	}
	//
	return qfalse;
}
// End - TAT 9/20/2002

/*
==============
BotClass_MedicCheckRevives
==============
*/
qboolean BotClass_MedicCheckRevives( bot_state_t *bs, int maxtravel, bot_goal_t *goal, qboolean lookForBots ) {
	gentity_t *trav;
	int i, t, area, best = -1, bestTravel, bestArea = -1; // Arnout: bestArea was not initialized
	int teammates[64], numTeammates;
	bot_goal_t target;
	int list[32], numList;

	//if we are not medic
	if ( bs->sess.playerType != PC_MEDIC ) {
		return qfalse;
	}

	if ( !BotGotEnoughAmmoForWeapon( bs, WP_MEDIC_SYRINGE ) ) {
		return qfalse;
	}

	bestTravel = maxtravel;
	numTeammates = BotNumTeamMates( bs, teammates, 64 );
	for ( i = 0; i < numTeammates; i++ ) {
		trav = &g_entities[teammates[i]];
		if ( trav->botIgnoreHealthTime > level.time ) {
			continue;
		}

		if ( trav->client->ps.pm_type != PM_DEAD ) {
			continue;
		}

		if ( trav->client->ps.pm_flags & PMF_LIMBO ) {
			continue;
		}

		// make sure there isn't already another medic helping them
		if ( ( numList = BotNumTeamMatesWithTargetByClass( bs, teammates[i], list, 32, PC_MEDIC ) ) ) {
			if ( BotReduceListByTravelTime( list, numList, BotGetOrigin( teammates[i] ), BotGetArea( teammates[i] ), BotTravelTimeToEntity( bs, teammates[i] ) ) ) {
				continue;
			}
		}

		// check the route to them
		area = BotGetArea( trav->s.number );  //trap_AAS_PointAreaNum(trav->r.currentOrigin);
		if ( !area ) {
			continue;
		}

		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, area, bs->tfl );
		if ( !t ) {
			continue;
		}

		if ( t < bestTravel ) {
			BotClearGoal( &target );
			target.entitynum = i;
			target.areanum = area;
			VectorCopy( g_entities[teammates[i]].r.mins, target.mins );
			VectorCopy( g_entities[teammates[i]].r.maxs, target.maxs );
			VectorCopy( g_entities[teammates[i]].r.currentOrigin, target.origin );

			if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_HIGH ) ) {
				best = teammates[i];
				bestTravel = t;
				bestArea = area;
			}
		}
	}

	if ( best >= 0 ) {
		// make this our goal?
		BotClearGoal( &target );
		target.entitynum = best;
		target.areanum = bestArea;
		VectorCopy( g_entities[best].r.mins, target.mins );
		VectorCopy( g_entities[best].r.maxs, target.maxs );
		VectorCopy( g_entities[best].r.currentOrigin, target.origin );

		*goal = target;
		return qtrue;
	}

	return qfalse;
}

/*
==============
G_RequestedHealth
==============
*/
qboolean G_RequestedHealth( bot_state_t *bs, int client, qboolean clearRequest ) {
	bot_chat_t *trav;
	int i;

	trav = bs->vchats;
	for ( i = 0; i < MAX_VCHATS; i++, trav++ ) {
		if ( !trav->time ) {
			continue;
		}

		if ( trav->time < level.time - 8000 ) {
			trav->time = 0;
			continue;
		}

		if ( trav->client != client ) {
			continue;
		}

		if ( !OnSameTeam( BotGetEntity( trav->client ), BotGetEntity( client ) ) ) {
			continue;
		}

		if ( trav->id != VCHAT_MEDIC ) {
			continue;
		}

		// they want health!
		if ( clearRequest ) {
			trav->time = 0;     // chat has been processed
		}

		return qtrue;
	}
	return qfalse;
}

/*
==============
BotClass_MedicCheckGiveHealth
==============
*/
qboolean BotClass_MedicCheckGiveHealth( bot_state_t *bs, int maxTravelTime, bot_goal_t *goal ) {
	gentity_t *trav;
	int i, area, best = -1, bestArea = -1; // Arnout: bestArea was not initialized
	int time;
	int bestTravelTime = maxTravelTime;
	int teammates[64], numTeammates;
	bot_goal_t target;
	int list[32], numList;

	//if we are not medic
	if ( bs->sess.playerType != PC_MEDIC ) {
		return qfalse;
	}

	if ( !BotWeaponCharged( bs, WP_MEDKIT ) ) {
		return qfalse;
	}

	numTeammates = BotNumTeamMates( bs, teammates, 64 );
	for ( i = 0; i < numTeammates; i++ ) {
		float scale;
		trav = &g_entities[teammates[i]];

		scale = BotHealthScale( trav - g_entities );
		if ( scale <= 0 || scale >= 1.f ) {
			continue;
		}

		if ( trav->botIgnoreHealthTime > level.time ) {
			continue;
		}

		if ( trav->s.number == bs->target_goal.entitynum ) {
			continue;
		}

		if ( trav->client->sess.playerType == PC_MEDIC ) {
			continue;
		}

		if ( trav->client->ps.pm_type != PM_NORMAL ) {
			continue;
		}

		// make sure there isn't already another medic helping them
		if ( ( numList = BotNumTeamMatesWithTargetByClass( bs, teammates[i], list, 32, PC_MEDIC ) ) ) {
			if ( BotReduceListByTravelTime( list, numList, BotGetOrigin( teammates[i] ), BotGetArea( teammates[i] ), BotTravelTimeToEntity( bs, teammates[i] ) ) ) {
				continue;
			}
		}

		// check the route to them
		area = BotGetArea( trav->s.number ); //trap_AAS_PointAreaNum(trav->r.currentOrigin);
		if ( !area ) {
			continue;
		}

		time = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, area, bs->tfl );
		if ( !time ) {
			continue;
		}

		if ( !G_RequestedHealth( bs, trav->s.number, qfalse ) ) {
			if ( !trap_InPVS( bs->origin, trav->r.currentOrigin ) ) {
				continue;
			}
			if ( BotHealthScale( trav->s.number ) > 0.5 ) {
				time += ( BotHealthScale( trav->s.number ) - 0.5 ) * 100 * 5;
			}
		} else {
			time -= 1500;
			if ( time <= 0 ) {
				time = 1;
			}
		}

		if ( time && time < bestTravelTime ) {
			BotClearGoal( &target );
			target.entitynum = i;
			target.areanum = area;
			VectorCopy( g_entities[teammates[i]].r.mins, target.mins );
			VectorCopy( g_entities[teammates[i]].r.maxs, target.maxs );
			VectorCopy( g_entities[teammates[i]].r.currentOrigin, target.origin );

			if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
				best = teammates[i];
				bestTravelTime = time;
				bestArea = area;
			}
		}
	}

	if ( best >= 0 ) {
		G_RequestedHealth( bs, best, qtrue );

		// make this our goal?
		BotClearGoal( &target );
		target.entitynum = best;
		target.areanum = bestArea;
		VectorCopy( g_entities[best].r.mins, target.mins );
		VectorCopy( g_entities[best].r.maxs, target.maxs );
		VectorCopy( g_entities[best].r.currentOrigin, target.origin );

		*goal = target;
		return qtrue;
	}

	return qfalse;
}

/*
==============
G_RequestedAmmo
==============
*/
qboolean G_RequestedAmmo( bot_state_t *bs, int client, qboolean clear ) {
	bot_chat_t *trav;
	int i;
	qboolean clearRequest = qfalse;
	//
	if ( client < 0 ) {
		clearRequest = qtrue;
		client = -client - 1;
	}
	//
	trav = bs->vchats;
	for ( i = 0; i < MAX_VCHATS; i++, trav++ ) {
		if ( !trav->time ) {
			continue;
		}
		if ( trav->time < level.time - 8000 ) {
			trav->time = 0;
			continue;
		}
		if ( trav->client != client ) {
			continue;
		}
		if ( !OnSameTeam( BotGetEntity( trav->client ), BotGetEntity( client ) ) ) {
			continue;
		}
		if ( trav->id != VCHAT_NEEDAMMO ) {
			continue;
		}
		// they want ammo!
		if ( clearRequest ) {
			trav->time = 0;     // chat has been processed
		}
		return qtrue;
	}
	//
	return qfalse;
}

/*
==============
BotClass_LtCheckGiveAmmo
==============
*/
qboolean BotClass_LtCheckGiveAmmo( bot_state_t *bs, int maxTravelTime, bot_goal_t *goal ) {
	gentity_t *trav;
	int i, area = 0, best = -1, bestArea = -1; // Arnout: bestArea was not initialized
	int time;
	int bestTravelTime = maxTravelTime;
	int teammates[64], numTeammates;
	bot_goal_t target;
	int list[32], numList;

	//if we are not fieldops
	if ( bs->sess.playerType != PC_FIELDOPS ) {
		return qfalse;
	}

	if ( !BotWeaponCharged( bs, WP_AMMO ) ) {
		return qfalse;
	}

	numTeammates = BotNumTeamMates( bs, teammates, 64 );
	for ( i = 0; i < numTeammates; i++ ) {
		trav = &g_entities[teammates[i]];
		if ( trav->health <= 0 ) {
			continue;
		}

		if ( trav->client->sess.playerType == PC_FIELDOPS ) {
			continue;   // fieldops's dont need ammo from us
		}

		if ( trav->botIgnoreAmmoTime > level.time ) {
			continue;
		}

		if ( trav->s.number == bs->target_goal.entitynum ) {
			continue;
		}

		// make sure there isn't already another field op helping them
		if ( ( numList = BotNumTeamMatesWithTargetByClass( bs, teammates[i], list, 32, PC_FIELDOPS ) ) ) {
			if ( BotReduceListByTravelTime( list, numList, BotGetOrigin( teammates[i] ), BotGetArea( teammates[i] ), BotTravelTimeToEntity( bs, teammates[i] ) ) ) {
				continue;
			}
		}

		time = 0;
		if ( trav->client->ps.pm_type == PM_NORMAL ) {

			// if they requested ammo, ignore check for PVS
			if ( !G_RequestedAmmo( bs, teammates[i], qfalse ) ) {
				// do they need ammo?
				if ( !ClientNeedsAmmo( teammates[i] ) ) {
					continue;
				}
				if ( !trap_InPVS( bs->origin, trav->r.currentOrigin ) ) {
					continue;
				}
			} else {
				if ( !ClientNeedsAmmo( teammates[i] ) ) {
					continue;
				}
			}

			// check the route to them
			if ( !( area = BotGetArea( trav->s.number ) ) ) {
				continue;
			}

			time = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, area, bs->tfl );
			if ( !time ) {
				continue;
			}

			if ( G_RequestedAmmo( bs, teammates[i], qfalse ) ) {
				time -= 1500;
				if ( time < 0 ) {
					time = 1;
				}
			}
		}

		if ( time && time < bestTravelTime ) {
			BotClearGoal( &target );
			target.entitynum = i;
			target.areanum = area;
			VectorCopy( g_entities[teammates[i]].r.mins, target.mins );
			VectorCopy( g_entities[teammates[i]].r.maxs, target.maxs );
			VectorCopy( g_entities[teammates[i]].r.currentOrigin, target.origin );

			if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
				best = teammates[i];
				bestTravelTime = time;
				bestArea = area;
			}
		}
	}

	if ( best >= 0 ) {
		G_RequestedAmmo( bs, best, qtrue ); // clear the request

		// make this our goal
		BotClearGoal( &target );
		target.entitynum = best;
		target.areanum = bestArea;
		VectorCopy( g_entities[best].r.mins, target.mins );
		VectorCopy( g_entities[best].r.maxs, target.maxs );
		VectorCopy( g_entities[best].r.currentOrigin, target.origin );

		*goal = target;
		return qtrue;
	}

	return qfalse;
}

extern void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly );

// NOTE!!! must be in synch with enum table in ai_team.h
char    *vchat_idstr[] =
{
	"HealMe",
	"NeedAmmo",
	"NeedBackup",
	"DynamitePlanted",
	"GreatShot",
	"Hi",
	"Thanks",
	"FollowMe",
	"DropFlag",
	"AggressionLow",
	"AggressionMed",
	"AggressionHigh",
	"AggressionCycle",
	"AutonomyLow",
	"AutonomyMed",
	"AutonomyHigh",
	"AutonomyCycle",

	// Start - TAT 8/21/2002
	// corresponding strings from the .menu file that trigger the vchat events below
	"HoldPosition",
	// End - TAT 8/21/2002

	// START	xkan, 9/13/2002
	"Prone",
	// END		xkan, 9/13/2002
	"Crouch",
	"Stand",
	"PositionCycle",

	"FireteamA",
	"FireteamB",
	"FireteamC",

	"ReviveMe",
	"HealTeam",
	"TeamAmmo",
	// End - TAT 9/23/2002

	"CycleWeapon",
	"BestWeapon",

	"HelpMe",
	"HelpTeam",

	NULL
};

/*
===============
BotRecordVoiceChat
===============
*/
void BotRecordVoiceChat( int client, int destclient, const char *id, int mode, qboolean noResponse ) {
	int i,j, vchat_id;
	bot_chat_t *trav, *oldest;
	bot_state_t *bs;

	if ( destclient == client ) {
		return;
	}

	if ( noResponse ) {
		return;
	}

	vchat_id = -1;
	for ( j = 0; vchat_idstr[j]; j++ ) {
		if ( !Q_stricmp( id, vchat_idstr[j] ) ) {
			vchat_id = j;
			break;
		}
	}

	if ( vchat_id == -1 ) {
		return; // not a known vchat
	}

	bs = &botstates[destclient];
	if ( !bs->inuse ) {
		return;
	}

	// find a free chat slot
	trav = bs->vchats;
	oldest = NULL;
	for ( i = 0; i < MAX_VCHATS; i++, trav++ ) {
		if ( trav->time ) {
			if ( !oldest || trav->time < oldest->time ) {
				oldest = trav;
			}
			continue;
		}

		trav->id = vchat_id;

		// RF, if this is a request to drop the flag from a teammate, then drop it
		if ( trav->id == VCHAT_DROPFLAG ) {
			if ( bs && BotSameTeam( bs, client ) && BotCarryingFlag( destclient ) ) {
				BotDropFlag( bs );
			}
		}

		// record it
		oldest = NULL;
		break;
	}

	if ( i == MAX_VCHATS ) {
		trav = NULL;
	}

	if ( oldest ) {
		trav = oldest;
		trav->id = vchat_id;
	}

	if ( trav ) {
		trav->client = client;
		trav->mode = mode;
		trav->time = level.time + 1200 + rand() % 2000;

		BotCheckVoiceChatResponse( bs );
	}
}

/*
===============
BotCheckVoiceChatResponse
===============
*/
void BotCheckVoiceChatResponse( bot_state_t *bs ) {
	int i;
	bot_chat_t *trav;
	gentity_t *ent, *other;
	qboolean clear;

	ent = &g_entities[bs->client];
	trav = bs->vchats;
	for ( i = 0; i < MAX_VCHATS; i++, trav++ ) {
		if ( !trav->time ) {
			continue;
		}
		if ( trav->time < level.time ) {
			continue;
		}
		if ( trav->time > level.time + 5000 ) {
			memset( trav, 0, sizeof( *trav ) );
			continue;
		}

		other = &g_entities[trav->client];
		clear = qfalse;

		switch ( trav->id ) {
		case VCHAT_THANKS:
			clear = qtrue;
			if ( !BotSameTeam( bs, trav->client ) ) {
				break;
			}

			// did we just help them?
			if ( bs->last_helped_client != trav->client ) {
				break;
			}
			if ( bs->last_helped_time < level.time - 5000 ) {
				break;
			}

			// your welcome!
			BotVoiceChatAfterIdleTime( bs->client, "Welcome", SAY_TEAM, 1000 + rand() % 1000, BOT_SHOWTEXT, 3000, qfalse  );
			break;
		case VCHAT_GREATSHOT:
			clear = qtrue;
			// did we just kill them?
			if ( other->health > 0 ) {
				break;
			}
			if ( other->client->lasthurt_client != bs->client ) {
				break;
			}
			if ( ent->client->lastKillTime > level.time - 10000 ) {
				break;
			}

			// thanks!
			BotSendVoiceChat( bs, "Thanks", SAY_ALL, 1000 + rand() % 1000, BOT_SHOWTEXT, qfalse );
			break;
		case VCHAT_HI:
			clear = qtrue;
			if ( other->client->sess.sessionTeam && !BotSameTeam( bs, trav->client ) ) {
				break;
			}

			// if we dont feel like talking
			if ( rand() % 100 > 50 ) {
				break;
			}

			if ( other->client->sess.sessionTeam ) {
				BotVoiceChatAfterIdleTime( bs->client, "Hi", SAY_TEAM, 1000 + rand() % 6000, BOT_SHOWTEXT, 7000, qfalse  );
			} else {
				BotVoiceChatAfterIdleTime( bs->client, "Hi", SAY_ALL, 1000 + rand() % 6000, BOT_SHOWTEXT, 7000, qfalse  );
			}
			break;

			// Make these explicit requests
		case VCHAT_NEEDAMMO:
		case VCHAT_MEDIC:
			break;
		}

		if ( clear ) {
			memset( trav, 0, sizeof( *trav ) );
		}
	}
}

/*
===============
BotDelayedVoiceChat
===============
*/
void BotDelayedVoiceChat( gentity_t *ent ) {
	bot_state_t *bs;

	// Should we force the voice to play even if the bot is dead?
	qboolean forceIsDead = ent->spawnflags & 1;

	//
	if ( level.intermissiontime ) {
		G_FreeEntity( ent );
		return;
	}
	//
	bs = &botstates[ent->r.ownerNum];
	if ( !bs->inuse ) {
		G_FreeEntity( ent );
		return;
	}
	//
	if ( !forceIsDead && BotIsDead( bs ) ) {
		G_FreeEntity( ent );
		return;
	}
	//
	if ( g_entities[ent->r.ownerNum].inuse && ent->count2 == g_entities[ent->r.ownerNum].client->pers.connectTime ) {
		bs->last_voice_chat = level.time;
		G_Voice( BotGetEntity( ent->r.ownerNum ), NULL, ent->missionLevel, ent->spawnitem, ent->count );
	}

	G_FreeEntity( ent );
}

/*
===============
BotSendVoiceChat
===============
*/
void BotSendVoiceChat( bot_state_t *bs, const char *id, int mode, int delay, qboolean voiceonly, qboolean forceIfDead ) {
	gentity_t *thinker;
	//
	if ( level.intermissiontime ) {
		return;
	}
	//
	if ( !forceIfDead && BotIsDead( bs ) ) {
		return;
	}
	//
	bs->last_voice_chat = level.time;
	//
	if ( delay ) {
		thinker = G_Spawn();
		if ( !thinker ) {
			return;
		}
		thinker->nextthink = level.time + delay;
		thinker->think = BotDelayedVoiceChat;

		thinker->spawnitem = (char *)id;
		thinker->r.ownerNum = bs->client;
		thinker->missionLevel = mode;
		thinker->count = voiceonly;
		thinker->count2 = g_entities[bs->client].client->pers.connectTime;

		// We need to signal if this VO can play if the bot is dead
		if ( forceIfDead ) {
			thinker->spawnflags |= 1;
		}

		return;
	}
	G_Voice( BotGetEntity( bs->client ), NULL, mode, id, voiceonly );
}

/*
===============
BotVoiceChatAfterIdleTime
===============
*/
void BotVoiceChatAfterIdleTime( int client, const char *id, int mode, int delay, qboolean voiceonly, int idleTime, qboolean forceIfDead ) {
	bot_state_t* bs = &botstates[client];
	if ( !bs->inuse ) {
		return;
	}

	if ( !forceIfDead && BotIsDead( bs ) ) {
		return;
	}

	if ( bs->last_voice_chat && bs->last_voice_chat > level.time - idleTime ) {
		return; // ignore

	}
	BotSendVoiceChat( bs, id, mode, delay, voiceonly, forceIfDead );
}


//
// AI_Team_Init
//
// Description: Blank out the AI Team information
// Written: 10/31/2002
//
void AI_Team_Init
(
	AI_Team_t *thisOne
) {
	// Reset the last Team VO time
	thisOne->last_voice_chat = 0;

}
//
// AI_Team_Init
//



//
// AI_Team_Init_All_Teams
//
// Description: Blank out the AI Team information for All teams
// Written: 10/31/2002
//
void AI_Team_Init_All_Teams
(
) {
	// Local Variables ////////////////////////////////////////////////////////
	int i;
	///////////////////////////////////////////////////////////////////////////

	// Loop through the AI teams and blank each one
	for ( i = 0; i < TEAM_NUM_TEAMS; i++ )
	{
		// Blank the team
		AI_Team_Init( &( g_aiTeam[i] ) );

	} // for (i = 0; i < TEAM_NUM_TEAMS; i++)...
}
//
// AI_Team_Init_All_Teams
//



/*
===============
BotVoiceChatAfterTeamIdleTime
===============

Says the specified line if the TEAM has been idle for the specified time.

*/
void BotVoiceChatAfterTeamIdleTime( int client, const char *id, int mode, int delay, qboolean voiceonly, int idleTime, qboolean forceIfDead ) {
	bot_state_t* bs = &botstates[client];

	if ( !bs->inuse ) {
		return;
	}

	//
	if ( !forceIfDead && BotIsDead( bs ) ) {
		return;
	}


	// If we had a team voice chat too recently
	if ( g_aiTeam[bs->sess.sessionTeam].last_voice_chat && ( g_aiTeam[bs->sess.sessionTeam].last_voice_chat > level.time - idleTime ) ) {
		// Ignore this request
		return;

	}

	// We'll have chatted now
	g_aiTeam[bs->sess.sessionTeam].last_voice_chat = level.time;

	// Say the line after the specified delay
	BotSendVoiceChat( bs, id, mode, delay, voiceonly, forceIfDead );
}

void G_SpawnGEntityFromSpawnVars( void );
char *G_AddSpawnVarToken( const char *string );

/*
===============
BotSpawnSpecialEntities
===============
*/
void BotSpawnSpecialEntities( void ) {
	vmCvar_t cvar_mapname;
	char keyname[MAX_QPATH];
	char        *com_token;
	char string[8192], *pStr;
	char filename[MAX_QPATH];
	int len;
	fileHandle_t f;


	// HACK, spawn special entities that would usually be compiled into maps

	trap_Cvar_Register( &cvar_mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	Com_sprintf( filename, sizeof( filename ), "maps/%s.botents", cvar_mapname.string );
	if ( ( len = trap_FS_FOpenFile( filename, &f, FS_READ ) ) < 0 ) {
		return;
	}

	if ( len >= sizeof( string ) ) {
		G_Error( "BotSpawnSpecialEntities: (%s) file is too big", filename );
	}

	memset( string, 0, sizeof( string ) );
	trap_FS_Read( string, len, f );

	trap_FS_FCloseFile( f );

	pStr = string;

	G_Printf( "Enable spawning!\n" );
	level.spawning = qtrue;

	while ( 1 ) {
		level.numSpawnVars = 0;
		level.numSpawnVarChars = 0;

		// parse the opening brace
		com_token = COM_Parse( &pStr );
		if ( !com_token || !com_token[0] ) {
			// end of spawn string
			return;
		}
		if ( com_token[0] != '{' ) {
			G_Error( "BotSpawnSpecialEntities: (%s) found %s when expecting {", filename, com_token );
		}

		// go through all the key / value pairs
		while ( 1 ) {
			// parse key
			com_token = COM_Parse( &pStr );
			if ( !com_token || !com_token[0] ) {
				G_Error( "BotSpawnSpecialEntities: (%s) EOF without closing brace", filename );
			}
			Q_strncpyz( keyname, com_token, sizeof( keyname ) );
			if ( keyname[0] == '}' ) {
				break;
			}

			// parse value
			com_token = COM_Parse( &pStr );
			if ( !com_token || !com_token[0] ) {
				G_Error( "BotSpawnSpecialEntities: (%s) EOF without closing brace", filename );
			}

			if ( com_token[0] == '}' ) {
				G_Error( "BotSpawnSpecialEntities: (%s) closing brace without data", filename );
			}
			if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
				G_Error( "BotSpawnSpecialEntities: (%s) MAX_SPAWN_VARS", filename );
			}
			level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
			level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
			level.numSpawnVars++;
		}

		// spawn the entity
		G_SpawnGEntityFromSpawnVars();
	}

	G_Printf( "Disable spawning!\n" );
	level.spawning = qfalse;

	return;
}

/*
==============
G_RequestedFollow
==============
*/
qboolean G_RequestedFollow( bot_state_t *bs, int client ) {
	bot_chat_t *trav;
	int i;
	qboolean clearRequest = qfalse;
	//
	if ( client < 0 ) {
		clearRequest = qtrue;
		client = -client - 1;
	}
	//
	trav = bs->vchats;
	for ( i = 0; i < MAX_VCHATS; i++, trav++ ) {
		if ( !trav->time ) {
			continue;
		}
		if ( trav->time < level.time - 30000 ) {
			trav->time = 0;
			continue;
		}
		if ( trav->client != client ) {
			continue;
		}
		if ( !OnSameTeam( BotGetEntity( trav->client ), BotGetEntity( client ) ) ) {
			continue;
		}
		if ( trav->id != VCHAT_FOLLOWME && trav->id != VCHAT_NEEDBACKUP ) {
			continue;
		}
		// they want us to follow!
		//if (clearRequest)
		trav->time = 0;         // chat has been processed
		return qtrue;
	}
	//
	return qfalse;
}

/*
===============
BotGetLeader
===============
*/
int BotGetLeader( bot_state_t *bs, qboolean onlyRequested ) {
	int i, j, t, best = -1, bestTime = 99999;
	gentity_t *trav;
	bot_goal_t goal;

	int heavyWeapons[] = {WP_PANZERFAUST, WP_MOBILE_MG42, -1};

	qboolean requested, bestRequested = qfalse;
	int list[MAX_CLIENTS], numList;
	float ourDist, *distances;
	bot_state_t *obs;

	//
	// if we have a scripted leader
	if ( bs->leader > -1 && ( bs->script.frameFlags & BSFFL_FOLLOW_LEADER ) && ( bs->leader == bs->script.entityNum ) ) {
		return bs->leader;
	}
	// if our current leader is carrying the flag, stay with them
	if ( bs->leader > -1 && BotCarryingFlag( bs->leader ) ) {
		return bs->leader;
	}
	// if we have a scripting enforced movement autonomy, stay put, unless we are given a new scripted leader
	if ( bs->leader == -1 && ( bs->script.flags & BSFL_FORCED_MOVEMENT_AUTONOMY ) ) {
		return -1;
	}
	best = bs->leader;

	//
	// find a teammate that is close, and is worth protecting (heavy weapons)
	for ( i = 0; i < level.numConnectedClients; i++, trav++ ) {
		trav = &g_entities[level.sortedClients[i]];

		if ( i == bs->client ) {
			continue;
		}

		if ( bs->leader == i ) {
			continue;
		}

		if ( trav->r.svFlags & SVF_BOT ) {
			obs = &botstates[i];
		} else {
			obs = NULL;
		}

		if ( trav->health <= 0 ) {
			continue;
		}

		if ( !BotSameTeam( bs, i ) ) {
			continue;
		}

		if ( obs && obs->leader > -1 ) {
			continue;
		}

		requested = qfalse;
		if ( G_RequestedFollow( bs, i ) ) {
			requested = qtrue;
		}

		if ( BotCarryingFlag( i ) ) {
			requested = qtrue;
		}

		if ( !requested && obs ) {
			// let us follow engineers only
			if ( trav->client->sess.playerType != PC_ENGINEER ) {
				continue;   // if not a bot, only follow if requested
			}
		}

		if ( !requested && bestRequested ) {
			continue;
		}

		if ( !requested && bs->leader > -1 ) {
			continue;
		}

/*		if (!requested && (trav->client->sess.playerType != PC_SOLDIER)) {
			// follow engineers only if they have a critical task
			if (trav->client->sess.playerType == PC_ENGINEER && obs) {
				if (!(obs->ainode == AINode_MP_DynamiteTarget || obs->ainode == AINode_MP_ConstructibleTarget)) {
					continue;
				} else {
					requested = qtrue;
				}
			} else {
				continue;
			}
		}*/

		if ( ( ourDist = VectorDistanceSquared( trav->r.currentOrigin, bs->origin ) ) > ( MAX_BOTLEADER_DIST * MAX_BOTLEADER_DIST ) ) {
			continue;
		}

		if ( requested && ( !trap_InPVS( trav->r.currentOrigin, bs->origin ) || ( ourDist > ( 1024 * 1024 ) ) ) ) {
			continue;
		}

		if ( ( numList = BotNumTeamMatesWithTarget( bs, i, list, MAX_CLIENTS ) ) >= 2 + ( requested ? 2 : 0 ) ) {
			// if there are too many defenders, and we are the furthest away, dont defend
			distances = BotSortPlayersByDistance( trav->r.currentOrigin, list, numList );
			if ( distances[numList] < ourDist ) {
				// we are the furthest
				continue;
			}
		}

		if ( !requested ) {
			for ( j = 0; heavyWeapons[j] != -1; j++ ) {
				if ( COM_BitCheck( trav->client->ps.weapons, heavyWeapons[j] ) ) {
					break;
				}
			}

			if ( heavyWeapons[j] == -1 ) {
				continue;
			}
		}

		// check travel time
		if ( !BotGoalForEntity( bs, i, &goal, BGU_MEDIUM ) && !requested ) {
			continue;   // NOTE: BotGoalForEntity() must be called first, so that if it fails, but we requested it, it still goes through with valid goal information
		}

		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, goal.areanum, bs->tfl );
		if ( !t || t > MAX_BOTLEADER_TRAVEL ) {
			continue;
		}

		if ( ( bestRequested == requested ) && ( t > bestTime ) ) {
			continue;
		}

		// we have a valid leader
		if ( requested ) {
			j = rand() % 2;
			if ( j == 0 ) {
				BotVoiceChatAfterIdleTime( bs->client, "WhereTo", SAY_TEAM, 1000 + rand() % 3000, BOT_SHOWTEXT, ( requested ? 4000 : 20000 ), qfalse );
			} else if ( j == 1 ) {
				BotVoiceChatAfterIdleTime( bs->client, "LetsGo", SAY_TEAM, 1000 + rand() % 3000, BOT_SHOWTEXT, ( requested ? 4000 : 20000 ), qfalse );
			}

			bs->lead_time = trap_AAS_Time() + 99999.0;
			bestRequested = qtrue;
		}
		best = i;
	}
	//
	return best;
}

int EntGetNumBotFollowers( int entNum ) {
	return 0;
}


/*
==================
BotSetLeaderTagEnt
==================
*/
void BotSetLeaderTagEnt( bot_state_t *bs ) {
}

int BotEngagementFunc( bot_state_t *bs ) {
	return 0;
}

int BotBehaviourFunc( bot_state_t *bs ) {
	return 0;
}

int BotObjectiveFunc( bot_state_t *bs ) {
	return 0;
}
