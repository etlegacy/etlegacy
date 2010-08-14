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
 * name:		ai_dmq3.c
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
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_cmd.h"
#include "ai_team.h"
#include "ai_dmnet_mp.h"
#include "ai_dmgoal_mp.h"
#include "ai_matrix.h"
#include "ai_distances.h"
//
#include "chars.h"               //characteristics
#include "inv.h"             //indexes into the inventory
#include "syn.h"             //synonyms
#include "match.h"               //string matching types and vars

#define IDEAL_ATTACKDIST            140
//#define WEAPONINDEX_MACHINEGUN	2

#define DONT_PRINT_REPEATED_AI_ERRORS

// BOT MOVEMENT AUTONOMY
float movementAutonomyRange[NUM_BMA] =
{
	256,            // LOW
	1024,           // MEDIUM
	99999,          // HIGH
};

// BOT MOVEMENT AUTONOMY for Single Player
float movementAutonomyRangeSP[NUM_BMA] =
{
	200,            // LOW
	350,            // MEDIUM
	700,            // HIGH
};

///////////////////////
//
// COMBAT CONSTANTS
//
///////////////////////


//////////////////
// from aasfile.h
//#define AREACONTENTS_MOVER				1024
//#define AREACONTENTS_MODELNUMSHIFT		24
//#define AREACONTENTS_MAXMODELNUM		0xFF
//#define AREACONTENTS_MODELNUM			(AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT)
//////////////////
//
bot_waypoint_t botai_waypoints[MAX_BOTAIWAYPOINTS];
bot_waypoint_t  *botai_freewaypoints;

//NOTE: not using a cvar which can be updated because the game should be reloaded anyway
int gametype;       //game type

// Rafael gameskill
//int	gameskill;

vmCvar_t bot_grapple;
vmCvar_t bot_rocketjump;
vmCvar_t bot_fastchat;
vmCvar_t bot_nochat;
vmCvar_t bot_testrchat;

vec3_t lastteleport_origin;
float lastteleport_time;
//true when the map changed
int max_bspmodelindex;  //maximum BSP model index

//CTF flag goals
bot_goal_t ctf_redflag;
bot_goal_t ctf_blueflag;

/*
==================
BotCarryingFlag
==================
*/
qboolean BotCarryingFlag( int client ) {
	if ( gametype < GT_WOLF ) {
		return qfalse;
	}

	if ( g_entities[client].health <= 0 ) {
		return qfalse;
	}

	if ( level.clients[client].ps.powerups[PW_REDFLAG] || level.clients[client].ps.powerups[PW_BLUEFLAG] ) {
		return qtrue;
	}

	return qfalse;
}

extern vec3_t playerMins;
extern vec3_t playerMaxs;

byte botCheckedAreas[65536];

/*
==================
BotFirstReachabilityArea
==================
*/
int BotFirstReachabilityArea( int entnum, vec3_t origin, int *areas, int numareas, qboolean distCheck ) {
	int i, best = 0;
	trace_t tr;
	vec3_t center;
	float bestDist, dist;
	vec3_t mins, maxs;
	//
	if ( entnum >= 0 && entnum < level.maxclients ) {
		VectorCopy( playerMins, mins );
		mins[2] += 18;  // STEPSIZE
		VectorCopy( playerMaxs, maxs );
	} else {
		VectorCopy( vec3_origin, mins );
		VectorCopy( vec3_origin, maxs );
	}
	bestDist = 999999;
	for ( i = 0; i < numareas; i++ ) {
		if ( botCheckedAreas[areas[i]] ) {
			continue;
		}
		botCheckedAreas[areas[i]] = 1;
		if ( trap_AAS_AreaReachability( areas[i] ) ) {
			// make sure this area is visible
			if ( !trap_AAS_AreaWaypoint( areas[i], center ) ) {
				trap_AAS_AreaCenter( areas[i], center );
			}
			if ( distCheck ) {
				dist = VectorDistance( center, origin );
				if ( center[2] > origin[2] ) {
					dist += 32 * ( center[2] - origin[2] );
				}
				if ( dist < bestDist ) {
					trap_Trace( &tr, origin, mins, maxs, center, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
					if ( tr.fraction > .99f || tr.startsolid ) { // if we start in solid, ignore trace test
						best = areas[i];
						bestDist = dist;
						//if (dist < 128) {
						//	return best;
						//}
					}
				}
			} else {
				trap_Trace( &tr, origin, mins, maxs, center, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
				if ( tr.fraction > .99f || tr.startsolid ) {  // if we start in solid, ignore trace test
					return areas[i];
				}
			}
		}
	}
	//
	return best;
}

/*
==================
BotFirstLadderArea
==================
*/
int BotFirstLadderArea( int entnum, int *areas, int numareas ) {
	int i;
	//
	for ( i = 0; i < numareas; i++ ) {
		if ( trap_AAS_AreaLadder( areas[i] ) ) {
			return areas[i];
		}
	}
	//
	return 0;
}

/*
==================
BotPointAreaNum
==================
*/
int BotPointAreaNum( int entnum, vec3_t origin ) {
	int areanum, numareas, areas[50], bestarea = 0, i;
	vec3_t end, start, ofs, mins, maxs;
	float f;
	gentity_t *ent = NULL;
	#define BOTAREA_BOX_DIST        256
	#define BOTAREA_JIGGLE_DIST     32
	//
	if ( entnum >= 0 && VectorCompare( origin, g_entities[entnum].botAreaPos ) ) {
		return g_entities[entnum].botAreaNum;
	}

	memset( botCheckedAreas, 0, sizeof( botCheckedAreas ) );

	if ( entnum >= 0 ) {
		ent = &g_entities[entnum];
	}

	// if this is a bot, and it's touching a ladder, do special handling
	if ( ent && ent->client && ent->client->ps.pm_flags & PMF_LADDER ) {
		// use the point only if its a ladder area
		areanum = trap_AAS_PointAreaNum( origin );
		if ( areanum && !trap_AAS_AreaLadder( areanum ) ) {
			areanum = 0;
		}
		if ( areanum ) {
			bestarea = areanum;
			goto done;
		}

		// try a small box, and take a ladder area as preference
		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 4;
		VectorSubtract( origin, maxs, mins );
		VectorAdd( origin, maxs, maxs );
		numareas = trap_AAS_BBoxAreas( mins, maxs, areas, 50 );
		if ( numareas > 0 ) {
			bestarea = BotFirstLadderArea( entnum, areas, numareas );
		}
		if ( bestarea ) {
			goto done;
		}

		// try the actual point
		areanum = trap_AAS_PointAreaNum( origin );
		if ( areanum && !trap_AAS_AreaReachability( areanum ) ) {
			areanum = 0;
		}
		if ( areanum ) {
			bestarea = areanum;
			goto done;
		}
	} else {
		//
		areanum = trap_AAS_PointAreaNum( origin );
		if ( areanum && !trap_AAS_AreaReachability( areanum ) ) {
			areanum = 0;
		}
		if ( areanum ) {
			bestarea = areanum;
			goto done;
		}
		// trace a line from below us, upwards, finding the first area the line touches
		VectorCopy( origin, start );
		VectorCopy( origin, end );
		if ( ( entnum >= 0 ) && g_entities[entnum].inuse && g_entities[entnum].client ) {
			end[2] += g_entities[entnum].client->ps.viewheight;
		}
		start[2] -= 30;
		numareas = trap_AAS_TraceAreas( start, end, areas, NULL, 50 );
		if ( numareas > 0 ) {
			bestarea = BotFirstReachabilityArea( entnum, origin, areas, numareas, qfalse );
		}
		if ( bestarea ) {
			goto done;
		}

		// try a small box around the origin
		maxs[0] = 4;
		maxs[1] = 4;
		maxs[2] = 4;
		VectorSubtract( origin, maxs, mins );
		VectorAdd( origin, maxs, maxs );
		numareas = trap_AAS_BBoxAreas( mins, maxs, areas, 50 );
		if ( numareas > 0 ) {
			bestarea = BotFirstReachabilityArea( entnum, origin, areas, numareas, qtrue );
		}
		if ( bestarea ) {
			goto done;
		}
	}

	// try using the players bounding box
	if ( ( entnum >= 0 ) && g_entities[entnum].inuse && g_entities[entnum].client ) {
		numareas = trap_AAS_BBoxAreas( g_entities[entnum].r.absmin, g_entities[entnum].r.absmax, areas, 50 );
		if ( numareas > 0 ) {
			bestarea = BotFirstReachabilityArea( entnum, origin, areas, numareas, qtrue );
		}
		if ( bestarea ) {
			goto done;
		}
	}

	//@TODO.  The following code seems to often cause bogus areanums to be returned.  They are offset
	// from the real areas, and this causes all sorts of bot stickiness.

	// try half size first
	for ( f = 0.1; f <= 1.0; f += 0.45 ) {
		VectorCopy( origin, end );
		end[2] += 80;
		VectorCopy( origin, ofs );
		ofs[2] -= 60;
		for ( i = 0; i < 2; i++ ) end[i] += BOTAREA_BOX_DIST * f;
		for ( i = 0; i < 2; i++ ) ofs[i] -= BOTAREA_BOX_DIST * f;
		//
		numareas = trap_AAS_BBoxAreas( ofs, end, areas, 50 );
		if ( numareas > 0 ) {
			bestarea = BotFirstReachabilityArea( entnum, origin, areas, numareas, qtrue );
		}
		if ( bestarea ) {
			goto done;
		}
	}
	//
done:
	if ( entnum >= 0 ) {
		VectorCopy( origin, g_entities[entnum].botAreaPos );
		g_entities[entnum].botAreaNum = bestarea;
	}
	return bestarea;
}

/*
===================
BotReachableBBoxAreaNum
===================
*/
int BotReachableBBoxAreaNum( bot_state_t *bs, vec3_t absmin, vec3_t absmax ) {
	int numareas, areas[64], sorted[64], bestarea = 0, i, j;
	vec3_t center, v;
	float dists[200], bestdist;
	//
	// find the area that is reachable from bs, and closest to the center of the box
	numareas = trap_AAS_BBoxAreas( absmin, absmax, areas, 64 );
	// sort them by distance from center
	VectorAdd( absmin, absmax, center );
	VectorScale( center, 0.5, center );

	for ( i = 0; i < numareas; i++ ) {
		trap_AAS_AreaWaypoint( areas[i], v );
		dists[i] = VectorDistanceSquared( center, v );
	}

	for ( i = 0; i < numareas; i++ ) {
		bestdist = -1;
		for ( j = 0; j < numareas; j++ ) {
			if ( dists[j] > 0 && ( bestdist < 0 || dists[j] < bestdist ) ) {
				bestdist = dists[j];
				bestarea = j;
			}
		}
		dists[bestarea] = -1;
		sorted[i] = areas[bestarea];
	}

	// now take the first area that we can reach
	for ( i = 0; i < numareas; i++ ) {
		if ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, sorted[i], bs->tfl ) ) {
			return sorted[i];
		}
	}

	return 0;
}

/*
==================
BotFindNearbyGoal
==================
*/

static qboolean sDoNearbyGoalCheck( bot_state_t *bs, vec3_t loc, gentity_t *target ) {
	int t, areanum;
	bot_goal_t goal;

	#define MAX_NEARBY_DIST     512
	#define MAX_NEARBY_TIME     1500

	// if it's close enough
	if ( VectorDistanceSquared( bs->origin, loc ) > SQR( MAX_NEARBY_DIST ) ) {
		return qfalse;
	}

	// if it's not within travel time
	areanum = trap_AAS_PointAreaNum( loc );
	if ( areanum || !trap_AAS_AreaReachability( areanum ) ) {
		areanum = BotPointAreaNum( -1, loc );
	}
	if ( !areanum ) {
		return qfalse;
	}

	t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl );

	if ( !t || ( t > MAX_NEARBY_TIME ) ) {
		return qfalse;
	}

	// found one!
	BotClearGoal( &goal );
	goal.areanum = areanum;
	goal.entitynum = target->s.number;
	VectorCopy( target->r.mins, goal.mins );
	VectorCopy( target->r.maxs, goal.maxs );
	VectorCopy( loc, goal.origin );

	// always get it if it's close
	if ( t > 200 && !BotGoalWithinMovementAutonomy( bs, &goal, BGU_LOW ) ) {
		return qfalse;
	}

	bs->nearbygoal = goal;
	return qtrue;

}

// TAT 11/21/2002
//		Look for ammo and health triggers
qboolean BotFindNearbyTriggerGoal( bot_state_t *bs ) {
	gentity_t *trav;
	vec3_t loc;
	int i;
	char *goalnames[] = {"trigger_ammo", "trigger_heal", NULL};

	for ( i = 0; goalnames[i]; i++ )
	{
		// ammo
		if ( i == 0 ) {
			// does the bot need ammo?  also checks if they need ammo and health PACKS
			if ( !ClientNeedsAmmo( bs->client ) ) {
				continue;
			}
		}
		// health
		else if ( i == 1 ) {
			if ( BotHealthScale( bs->client ) >= 1.0 ) {
				continue;
			}
		}
		//
		trav = NULL;
		while ( ( trav = G_Find( trav, FOFS( classname ), goalnames[i] ) ) )
		{
			// triggers don't have a location, they just have bounds
			//		so see how far we are from the center of the trigger
			loc[0] = ( trav->r.mins[0] + trav->r.maxs[0] ) / 2.0f;
			loc[1] = ( trav->r.mins[1] + trav->r.maxs[1] ) / 2.0f;
			loc[2] = ( ( trav->r.mins[2] + trav->r.maxs[2] ) / 2.0f ) + 30.f;

			if ( sDoNearbyGoalCheck( bs, loc, trav ) ) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

qboolean BotFindNearbyGoal( bot_state_t *bs ) {
	vec3_t org;
	int i;
	qboolean needAmmo;
	qboolean needHealth;

	if ( bs->next_nearbygoal > level.time ) {
		return qfalse;
	}
	bs->next_nearbygoal = level.time + 500 + rand() % 500;

	needAmmo = ClientNeedsAmmo( bs->client );
	needHealth = BotHealthScale( bs->client ) >= 1.0 ? qfalse : qtrue;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		gentity_t* ent = &g_entities[i];
		switch ( ent->s.eType ) {
		case ET_ITEM:
		{
			gitem_t* item = &bg_itemlist[ent->s.modelindex];
			switch ( item->giType ) {
			case IT_TEAM:
				switch ( item->giType ) {
				case PW_REDFLAG:
					if ( bs->sess.sessionTeam == TEAM_AXIS ) {
						continue;
					}
					break;
				case PW_BLUEFLAG:
					if ( bs->sess.sessionTeam == TEAM_ALLIES ) {
						continue;
					}
					break;
				default:
					break;
				}
				break;
			case IT_WEAPON:
				if ( !needAmmo ) {
					continue;
				}
				switch ( item->giType ) {
				case WP_AMMO:
					break;
				default:
					continue;
				}
				break;
			case IT_HEALTH:
				if ( !needHealth ) {
					continue;
				}
				break;
			default:
				continue;
			}

			if ( ent->r.ownerNum == bs->client && ent->botIgnoreTime > level.time ) {
				continue;
			}
			VectorCopy( ent->r.currentOrigin, org );
			org[2] += 30;

			if ( sDoNearbyGoalCheck( bs, org, ent ) ) {
				return qtrue;
			}
		}
		case ET_SUPPLIER:
		case ET_HEALER:
		{
			vec3_t loc;
			if ( ent->s.eType == ET_HEALER && !needHealth ) {
				continue;
			}
			if ( ent->s.eType == ET_SUPPLIER && !needAmmo ) {
				continue;
			}

			VectorAdd( ent->r.mins, ent->r.maxs, loc );
			VectorScale( loc, 0.5f, loc );
			loc[2] += 30.f;

			if ( sDoNearbyGoalCheck( bs, loc, ent ) ) {
				return qtrue;
			}
		}
		default:
			continue;
		}

	}

	return qfalse;
}

/*
==================
ClientName
==================
*/
char *ClientName( int client, char *name, int size ) {
	char buf[MAX_INFO_STRING];

	if ( client < 0 || client >= MAX_CLIENTS ) {
		BotAI_Print( PRT_ERROR, "ClientName: client out of range\n" );
		return "[client out of range]";
	}
	trap_GetConfigstring( CS_PLAYERS + client, buf, sizeof( buf ) );
	strncpy( name, Info_ValueForKey( buf, "n" ), size - 1 );
	name[size - 1] = '\0';
	Q_CleanStr( name );
	return name;
}

/*
==================
ClientSkin
==================
*/
char *ClientSkin( int client, char *skin, int size ) {
	char buf[MAX_INFO_STRING];

	if ( client < 0 || client >= MAX_CLIENTS ) {
		BotAI_Print( PRT_ERROR, "ClientSkin: client out of range\n" );
		return "[client out of range]";
	}
	trap_GetConfigstring( CS_PLAYERS + client, buf, sizeof( buf ) );
	strncpy( skin, Info_ValueForKey( buf, "model" ), size - 1 );
	skin[size - 1] = '\0';
	return skin;
}

/*
==================
ClientFromName
==================
*/
int ClientFromName( char *name ) {
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
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		Q_CleanStr( buf );
		if ( !Q_stricmp( Info_ValueForKey( buf, "n" ), name ) ) {
			return i;
		}
	}
	return -1;
}

/*
==================
stristr
==================
*/
char *stristr( char *str, char *charset ) {
	int i;

	while ( *str ) {
		for ( i = 0; charset[i] && str[i]; i++ ) {
			if ( toupper( charset[i] ) != toupper( str[i] ) ) {
				break;
			}
		}
		if ( !charset[i] ) {
			return str;
		}
		str++;
	}
	return NULL;
}

/*
==================
EasyClientName
==================
*/
char *EasyClientName( int client, char *buf, int size ) {
	int i;
	char *str1, *str2, *ptr, c;
	char name[128];

	strcpy( name, ClientName( client, name, sizeof( name ) ) );
	for ( i = 0; name[i]; i++ ) name[i] &= 127;
	//remove all spaces
	for ( ptr = strstr( name, " " ); ptr; ptr = strstr( name, " " ) ) {
		memmove( ptr, ptr + 1, strlen( ptr + 1 ) + 1 );
	}
	//check for [x] and ]x[ clan names
	str1 = strstr( name, "[" );
	str2 = strstr( name, "]" );
	if ( str1 && str2 ) {
		if ( str2 > str1 ) {
			memmove( str1, str2 + 1, strlen( str2 + 1 ) + 1 );
		} else { memmove( str2, str1 + 1, strlen( str1 + 1 ) + 1 );}
	}
	//remove Mr prefix
	if ( ( name[0] == 'm' || name[0] == 'M' ) &&
		 ( name[1] == 'r' || name[1] == 'R' ) ) {
		memmove( name, name + 2, strlen( name + 2 ) + 1 );
	}
	//only allow lower case alphabet characters
	ptr = name;
	while ( *ptr ) {
		c = *ptr;
		if ( ( c >= 'a' && c <= 'z' ) ||
			 ( c >= '0' && c <= '9' ) || c == '_' ) {
			ptr++;
		} else if ( c >= 'A' && c <= 'Z' )       {
			*ptr += 'a' - 'A';
			ptr++;
		} else {
			memmove( ptr, ptr + 1, strlen( ptr + 1 ) + 1 );
		}
	}
	strncpy( buf, name, size - 1 );
	buf[size - 1] = '\0';
	return buf;
}

/*
==============
BotGotEnoughAmmoForWeapon
==============
*/
qboolean BotGotEnoughAmmoForWeapon( bot_state_t *bs, int weapon ) {
	int ammo, clip;

	// if this is a charged weapon, check that it is ready for use (soon)
	if ( !BotWeaponCharged( bs, weapon ) ) {
		return qfalse;
	}

	ammo = bs->cur_ps.ammo[BG_FindAmmoForWeapon( weapon )];
	clip = bs->cur_ps.ammoclip[BG_FindClipForWeapon( weapon )];

	// TODO!! check some kind of weapon list that holds the minimum requirements for each weapon
	switch ( weapon ) {
	default:
		return (qboolean)( ( clip >= GetAmmoTableData( weapon )->uses ) || ( ammo >= GetAmmoTableData( weapon )->uses ) );    //----(SA)
	}
}

/*
==============
BotWeaponCharged
==============
*/
#define WC_WEAPON_TIME_LEFT level.time - ps->classWeaponTime
#define WC_SOLDIER_TIME     level.soldierChargeTime     [team - TEAM_AXIS]
#define WC_ENGINEER_TIME    level.engineerChargeTime    [team - TEAM_AXIS]
#define WC_FIELDOPS_TIME    level.lieutenantChargeTime  [team - TEAM_AXIS]
#define WC_MEDIC_TIME       level.medicChargeTime       [team - TEAM_AXIS]
#define WC_COVERTOPS_TIME   level.covertopsChargeTime   [team - TEAM_AXIS]

qboolean G_WeaponCharged( playerState_t* ps, team_t team, int weapon, int* skill ) {
	switch ( weapon ) {
	case WP_PANZERFAUST:
		if ( ps->eFlags & EF_PRONE ) {
			return qfalse;
		}

		if ( skill[SK_HEAVY_WEAPONS] >= 1 ) {
			if ( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME * 0.66f ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME ) {
			return qfalse;
		}
	case WP_MORTAR_SET:
		if ( skill[SK_HEAVY_WEAPONS] >= 1 ) {
			if ( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME * 0.33f ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME * 0.5f ) {
			return qfalse;
		}
		return qtrue;
	case WP_SMOKE_BOMB:
	case WP_SATCHEL:
		if ( skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 2 ) {
			if ( WC_WEAPON_TIME_LEFT < WC_COVERTOPS_TIME * 0.66f ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_COVERTOPS_TIME ) {
			return qfalse;
		}
		break;

	case WP_LANDMINE:
		if ( skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 2 ) {
			if ( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.33f ) ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.5f ) ) {
			return qfalse;
		}
		break;

	case WP_DYNAMITE:
		if ( skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3 ) {
			if ( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.66f ) ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_ENGINEER_TIME ) {
			return qfalse;
		}
		break;

	case WP_MEDKIT:
		if ( skill[SK_FIRST_AID] >= 2 ) {
			if ( WC_WEAPON_TIME_LEFT < WC_MEDIC_TIME * 0.15f ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_MEDIC_TIME * 0.25f ) {
			return qfalse;
		}
		break;

	case WP_AMMO:
		if ( skill[SK_SIGNALS] >= 1 ) {
			if ( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME * 0.15f ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME * 0.25f ) {
			return qfalse;
		}
		break;

	case WP_SMOKE_MARKER:
		if ( skill[SK_SIGNALS] >= 2 ) {
			if ( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME * 0.66f ) {
				return qfalse;
			}
		} else if ( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME ) {
			return qfalse;
		}
		break;

	case WP_MEDIC_ADRENALINE:
		if ( WC_WEAPON_TIME_LEFT < WC_MEDIC_TIME ) {
			return qfalse;
		}
		break;

	case WP_BINOCULARS:
		switch ( ps->stats[ STAT_PLAYER_CLASS ] ) {
		case PC_FIELDOPS:
			if ( skill[SK_SIGNALS] >= 2 ) {
				if ( WC_WEAPON_TIME_LEFT <= WC_FIELDOPS_TIME * 0.66f ) {
					return qfalse;
				}
			} else if ( WC_WEAPON_TIME_LEFT <= WC_FIELDOPS_TIME ) {
				return qfalse;
			}
		default:
			return qfalse;
		}
		break;

	case WP_GPG40:
	case WP_M7:
		if ( WC_WEAPON_TIME_LEFT < WC_ENGINEER_TIME * 0.5f ) {
			return qfalse;
		}
		break;
	}

	return qtrue;
}

qboolean BotWeaponCharged( bot_state_t *bs, int weapon ) {
	return G_WeaponCharged( &bs->cur_ps, bs->sess.sessionTeam, weapon, bs->sess.skill );
}

/*
==============
BotWeaponOnlyUseIfInInRange

  returns qtrue if the given weapon should only be used if the enemy is within range (binocs, panzer, etc)
==============
*/
qboolean BotWeaponOnlyUseIfInInRange( int weaponnum ) {
	switch ( weaponnum ) {
	case WP_BINOCULARS:
	case WP_SMOKE_MARKER:
	case WP_PANZERFAUST:
	case WP_MOBILE_MG42:
	case WP_MORTAR:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		return qtrue;
	}
	return qfalse;
}

/*
==============
BotWeaponClosestDist
==============
*/
float BotWeaponClosestDist( int weaponnum ) {
	switch ( weaponnum ) {
	case WP_PANZERFAUST:
		return 512.0f;

	case WP_BINOCULARS:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		return 1024.0f;

	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
		return 128;

	case WP_M7:
	case WP_GPG40:
		return 256;
	}
	return 0;
}

int BotTeamMatesNearEnemy( bot_state_t* bs ) {
	int i, j;
	vec_t* vec;
	float range = SQR( G_GetWeaponDamage( WP_PANZERFAUST ) );
	int cnt = 0;

	if ( bs->enemy < 0 ) {
		return 0;
	}

	vec = BotGetOrigin( bs->enemy );

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( j == bs->client ) {
			continue;
		}

		if ( !BotSameTeam( bs, j ) ) {
			continue;
		}

		if ( BotIsDead( &botstates[j] ) ) {
			continue;
		}

		if ( VectorDistanceSquared( vec, BotGetOrigin( j ) ) > range ) {
			continue;
		}

		cnt++;
	}

	return cnt;
}

/*
==============
BotWeaponWantScale
==============
*/
float BotWeaponWantScale( bot_state_t *bs, weapon_t weapon ) {
	qboolean moving;

	weapon_t clip = BG_FindClipForWeapon( weapon );
	weapon_t ammo = BG_FindAmmoForWeapon( weapon );

	if ( !bs->cur_ps.ammo[ammo] && !bs->cur_ps.ammoclip[clip] ) {
		return 0.f;
	}

	if ( !BotWeaponCharged( bs, weapon ) ) {
		return 0.f;
	}

	moving = ( VectorLengthSquared( bs->cur_ps.velocity ) > SQR( 10 ) );

	switch ( weapon ) {
	case WP_KNIFE:
		// for fun, have random bots use the knife in warmup scrumage
		if ( level.warmupTime > level.time && !( ( bs->client + level.warmupTime / 100 ) % 5 ) ) {
			return 2.0;
		}
		return 0.2;

	case WP_LUGER:
	case WP_COLT:
		return 0.4;

	case WP_SILENCER:
	case WP_SILENCED_COLT:
		return 0.45;

	case WP_AKIMBO_COLT:
	case WP_AKIMBO_LUGER:
		return 0.5;

	case WP_AKIMBO_SILENCEDCOLT:
	case WP_AKIMBO_SILENCEDLUGER:
		return 0.55;

	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:
		return 0.6;

	case WP_GPG40:
	case WP_M7:
		if ( bs->inventory[ENEMY_HORIZONTAL_DIST] > 512 ) {
			return 1.0;
		}
		return 0.1;

	case WP_CARBINE:
	case WP_GARAND:
	case WP_FG42:
	case WP_KAR98:
	case WP_K43:
		return 0.6;

	case WP_MOBILE_MG42:
		if ( !moving && bs->inventory[ENEMY_HORIZONTAL_DIST] > 500 ) {
			return 3.0;
		}
		return 0.3;
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		if ( ( !moving && bs->enemy > -1 ) && ( bs->inventory[ENEMY_HORIZONTAL_DIST] > 300 ) ) {
			return 1.0;
		}
		return 0.1;
	case WP_FLAMETHROWER:
		if ( ( !moving || bs->enemy > -1 ) && bs->inventory[ENEMY_HORIZONTAL_DIST] < 800 ) {
			return 1.0;
		}
		return 0.1;
	case WP_PANZERFAUST:
		if ( !moving || bs->enemy > -1 ) {
			if ( bs->enemy >= 0 ) {
				if ( BotTeamMatesNearEnemy( bs ) > 1 ) {
					return 0.f;
				}
			}

			if ( bs->inventory[INVENTORY_HEALTH] < 15 || bs->inventory[ENEMY_HORIZONTAL_DIST] > 400 ) {
				return 1.0;
			}
		}
		return 0.1;
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:

		if ( bs->enemy > -1 && bs->inventory[ENEMY_HORIZONTAL_DIST] < 300 ) {
			if ( BotHealthScale( bs->client ) < 0.3 && ( bs->enemy < 0 || !BotCarryingFlag( bs->enemy ) ) ) {
				return 2.0 * ( 1.0 - (float)bs->inventory[INVENTORY_HEALTH] / 40.0 );   // try and get a grenade off before death
			}
			if ( bs->inventory[ENEMY_HORIZONTAL_DIST] > 200 ) {
				return 0.5;
			} else {
				return 0.3;
			}
		} else {
			return 0.1;
		}
		break;
	case WP_SMOKE_MARKER:
		if ( bs->sess.playerType == PC_FIELDOPS && bs->enemy > -1 && ( bs->inventory[ENEMY_HORIZONTAL_DIST] < 400 ) ) {
			if ( BG_GetSkyHeightAtPoint( BotGetOrigin( bs->enemy ) ) == MAX_MAP_SIZE ) {
				return 0.f;
			}

			return 1.f;
		}
		break;
	case WP_BINOCULARS:
		if ( !moving ) {
			if ( bs->sess.playerType == PC_FIELDOPS ) {
				if ( bs->enemy > -1 && ( bs->inventory[ENEMY_HORIZONTAL_DIST] < 400 ) ) {
					if ( BG_GetSkyHeightAtPoint( BotGetOrigin( bs->enemy ) ) == MAX_MAP_SIZE ) {
						return 0.f;
					}
					return 1.f;
				}
				return 0.f;
			} else if ( bs->sess.playerType == PC_COVERTOPS ) {
				return 1.f;
			}
		} else {
			return 0.01;
		}
		break;
	default:
		break;
	}

	// anything else must be non-combat
	return 0.0;
}


/*
==================
BotBestFightWeapon
==================
*/
int BotBestFightWeapon( bot_state_t *bs ) {
	weapon_t bestWeapon;
	int i, *ammo;
	float wantScale, bestWantScale, dist, thisRange, bestRange;
	qboolean inRange, bestInRange;

	ammo = bs->cur_ps.ammo;
	bestWantScale = 0.0;
	bestRange = 0.0;
	bestWeapon = bs->weaponnum; // default to current weapon
	bestInRange = qfalse;

	dist = -1;
	if ( bs->enemy >= 0 ) {
		dist = VectorDistance( bs->origin, BotGetOrigin( bs->enemy ) );
	}

	for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
		if ( COM_BitCheck( bs->cur_ps.weapons, i ) ) {

			// check that our ammo is enough
			if ( !BotGotEnoughAmmoForWeapon( bs, i ) ) {
				continue;
			}

			// if they are too close, dont use this weapon
			if ( dist != -1 && BotWeaponClosestDist( i ) > dist ) {
				continue;
			}

			// if this is a scoped weapon, only use it if we aren't moving
			if ( BotScopedWeapon( i ) && VectorLengthSquared( bs->cur_ps.velocity ) > SQR( 10 ) ) {
				continue;
			}

			// check the range
			if ( ( thisRange = BotWeaponRange( bs, i ) ) >= dist ) {
				inRange = qtrue;
			} else {
				// if this weapon should only be used if we are within range
				if ( dist != -1 && BotWeaponOnlyUseIfInInRange( i ) ) {
					continue;
				}
				inRange = qfalse;
			}

			if ( !inRange && bestInRange ) {
				continue;
			}

			//
			// get the wantScale for this weapon given the current circumstances (0.0 - 1.0)
			wantScale = BotWeaponWantScale( bs, i );
			//
			if ( ( inRange && !bestInRange ) || ( inRange && wantScale >= bestWantScale ) || ( !inRange && thisRange > bestRange ) ) {
				bestWeapon = i;
				bestWantScale = wantScale;
				bestRange = thisRange;
				bestInRange = inRange;
			}
		}
	}
	//
	return bestWeapon;
}

/*
==================
BotChooseWeapon
==================
*/
void BotChooseWeapon( bot_state_t *bs ) {
	int newweaponnum;

	if ( bs->cur_ps.weaponstate == WEAPON_RAISING ||
		 bs->cur_ps.weaponstate == WEAPON_DROPPING ||
		 bs->cur_ps.weaponDelay ) {
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );
	} else {
		if ( ( newweaponnum = BotBestFightWeapon( bs ) ) ) {

			if ( bs->weaponnum != newweaponnum ) {
				bs->weaponchange_time = trap_AAS_Time();
			}

			bs->weaponnum = newweaponnum;
			//BotAI_Print(PRT_MESSAGE, "bs->weaponnum = %d\n", bs->weaponnum);
			trap_EA_SelectWeapon( bs->client, bs->weaponnum );
		}
	}
}

/*
==================
BotCycleWeapon
==================

Bot cycles to next weapon in inventory, and will use it until told to cycle again

TAT 11/14/2002

*/
void BotCycleWeapon( bot_state_t *bs ) {
	int i;
	int curWeapon = bs->weaponnum;
	float wantScale;

	// loop through all the weapons, starting after the one we have equipped
	for ( i = curWeapon + 1; i != curWeapon; i++ )
	{
		// if we went off the end, start at the beginning
		if ( i >= WP_NUM_WEAPONS ) {
			i = 0;
			// we have an endless loop here, when we have no weapons, our current weapon is 0, and we never hit the ending condition in the loop
			//		since it's set in here
			if ( curWeapon == 0 ) {
				break;
			}
		}

		// if we have this weapon
		if ( COM_BitCheck( bs->cur_ps.weapons, i ) ) {
			// if we don't have ammo for it, we can't choose it
			if ( !BotGotEnoughAmmoForWeapon( bs, i ) ) {
				continue;
			}

			// get the wantScale for this weapon given the current circumstances (0.0 - 1.0)
			wantScale = BotWeaponWantScale( bs, i );

			// if the wantscale is positive, then it is a weapon
			if ( wantScale > 0 ) {
				break;
			}
		}
	}

	if ( i != curWeapon ) {
		// we found a new weapon
		//	set it as our selected weapon
		bs->commandedWeapon = i;
		bs->weaponnum = i;
		trap_EA_SelectWeapon( bs->client, i );
	}
}

/*
==================
BotSetupForMovement
==================
*/
void BotSetupForMovement( bot_state_t *bs ) {
	bot_initmove_t initmove;

	memset( &initmove, 0, sizeof( bot_initmove_t ) );
	VectorCopy( bs->cur_ps.origin, initmove.origin );
	VectorCopy( bs->cur_ps.velocity, initmove.velocity );
	VectorCopy( bs->cur_ps.origin, initmove.viewoffset );
	initmove.viewoffset[2] += bs->cur_ps.viewheight;
	initmove.entitynum = bs->entitynum;
	initmove.client = bs->client;
	initmove.thinktime = bs->thinktime;
	initmove.areanum = bs->areanum;
	//set the onground flag
	if ( bs->cur_ps.groundEntityNum != ENTITYNUM_NONE ) {
		initmove.or_moveflags |= MFL_ONGROUND;
	}
	//set the teleported flag
	if ( ( bs->cur_ps.pm_flags & PMF_TIME_KNOCKBACK ) && ( bs->cur_ps.pm_time > 0 ) ) {
		initmove.or_moveflags |= MFL_TELEPORTED;
	}
	//set the waterjump flag
	if ( ( bs->cur_ps.pm_flags & PMF_TIME_WATERJUMP ) && ( bs->cur_ps.pm_time > 0 ) ) {
		initmove.or_moveflags |= MFL_WATERJUMP;
	}
	//set presence type
	if ( bs->cur_ps.pm_flags & PMF_DUCKED ) {
		initmove.presencetype = PRESENCE_CROUCH;
	} else { initmove.presencetype = PRESENCE_NORMAL;}
	//
	if ( bs->walker > 0.5 ) {
		initmove.or_moveflags |= MFL_WALK;
	}
	//
	VectorCopy( bs->viewangles, initmove.viewangles );
	//
	trap_BotInitMoveState( bs->ms, &initmove );
}

/*
==================
BotUpdateInventory
==================
*/
void BotUpdateInventory( bot_state_t *bs ) {
	//powerups
	bs->inventory[INVENTORY_HEALTH] = bs->cur_ps.stats[STAT_HEALTH];
	if ( bs->target_goal.entitynum != -1 ) {
		bs->inventory[GOAL_TRAVELTIME] = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->target_goal.areanum, bs->tfl );
	} else {
		bs->inventory[GOAL_TRAVELTIME] = 0;
	}
/*	bs->inventory[INVENTORY_TELEPORTER] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_TELEPORTER;
	bs->inventory[INVENTORY_MEDKIT] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_MEDKIT;
	bs->inventory[INVENTORY_REDFLAG] = bs->cur_ps.powerups[PW_REDFLAG] != 0;
	bs->inventory[INVENTORY_BLUEFLAG] = bs->cur_ps.powerups[PW_BLUEFLAG] != 0;*/
}

/*
==================
BotUpdateBattleInventory
==================
*/
void BotUpdateBattleInventory( bot_state_t *bs, int enemy ) {
	vec3_t dir;
	aas_entityinfo_t entinfo;

	BotEntityInfo( enemy, &entinfo );
	VectorSubtract( entinfo.origin, bs->origin, dir );
	bs->inventory[ENEMY_HEIGHT] = (int) dir[2];
	dir[2] = 0;
	bs->inventory[ENEMY_HORIZONTAL_DIST] = (int) VectorLength( dir );
	//FIXME: add num visible enemies and num visible team mates to the inventory
}

/*
==================
BotBattleUseItems
==================
*/
void BotBattleUseItems( bot_state_t *bs ) {
/*
	if (bs->inventory[INVENTORY_HEALTH] < 40) {
		if (bs->inventory[INVENTORY_TELEPORTER] > 0) {
			trap_EA_Use(bs->client);
		}
		if (bs->inventory[INVENTORY_MEDKIT] > 0) {
			trap_EA_Use(bs->client);
		}
	}
*/
}

/*
==================
BotSetTeleportTime
==================
*/
void BotSetTeleportTime( bot_state_t *bs ) {
	if ( ( bs->cur_ps.eFlags ^ bs->last_eFlags ) & EF_TELEPORT_BIT ) {
		bs->teleport_time = trap_AAS_Time();
	}
	bs->last_eFlags = bs->cur_ps.eFlags;
}

/*
==================
BotIsDead
==================
*/
qboolean BotIsDead( bot_state_t *bs ) {
	if ( bs->cur_ps.pm_flags & PMF_LIMBO ) {
		return qtrue;
	}
	// RF, re-enabled these, they are required for the bots to respawn
	if ( bs->cur_ps.pm_type == PM_DEAD ) {
		return qtrue;
	}
	if ( g_entities[bs->client].health <= 0 ) {
		return qtrue;
	}
	return qfalse;
}

// Gordon: 27/11/02: check if the bot is prisoner of war
/*
==================
BotIsPOW
==================
*/
qboolean BotIsPOW( bot_state_t *bs ) {
	return bs->isPOW;
}

/*
==================
BotIsObserver
==================
*/
qboolean BotIsObserver( bot_state_t *bs ) {
	char buf[MAX_INFO_STRING];
	if ( bs->cur_ps.pm_type == PM_SPECTATOR ) {
		return qtrue;
	}
	trap_GetConfigstring( CS_PLAYERS + bs->client, buf, sizeof( buf ) );
	if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
BotIntermission
==================
*/
qboolean BotIntermission( bot_state_t *bs ) {
	//NOTE: we shouldn't look at the game code...
	if ( level.intermissiontime ) {
		return qtrue;
	}
	return ( bs->cur_ps.pm_type == PM_FREEZE || bs->cur_ps.pm_type == PM_INTERMISSION );
}


/*
==============
BotInLava
==============
*/
qboolean BotInLava( bot_state_t *bs ) {
	vec3_t feet;

	VectorCopy( bs->origin, feet );
	feet[2] -= 23;
	return ( trap_AAS_PointContents( feet ) & CONTENTS_LAVA );
}

/*
==============
BotInSlime
==============
*/
qboolean BotInSlime( bot_state_t *bs ) {
	vec3_t feet;

	VectorCopy( bs->origin, feet );
	feet[2] -= 23;
	return ( trap_AAS_PointContents( feet ) & CONTENTS_SLIME );
}

/*
==================
EntityIsDead
==================
*/
qboolean EntityIsDead( aas_entityinfo_t *entinfo ) {
	if ( entinfo->number >= 0 && entinfo->number < MAX_CLIENTS ) {
		if ( !g_entities[entinfo->number].inuse ) {
			return qtrue;
		}
		if ( g_entities[entinfo->number].health <= 0 ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
EntityInLimbo
==================
*/
qboolean EntityInLimbo( aas_entityinfo_t *entinfo ) {
	if ( !g_entities[entinfo->number].client ) {
		return qfalse;
	}

	return ( g_entities[entinfo->number].client->ps.pm_flags & PMF_LIMBO ) ? qtrue : qfalse;
}

/*
==================
EntityIsInvisible
==================
*/
qboolean EntityIsInvisible( aas_entityinfo_t *entinfo ) {
	return qfalse;
}

/*
==================
EntityIsShooting
==================
*/
qboolean EntityIsShooting( aas_entityinfo_t *entinfo ) {
	if ( entinfo->flags & EF_FIRING ) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
EntityIsChatting
==================
*/
qboolean EntityIsChatting( aas_entityinfo_t *entinfo ) {
	if ( entinfo->flags & EF_TALK ) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
EntityHasQuad
==================
*/
qboolean EntityHasQuad( aas_entityinfo_t *entinfo ) {
	return qfalse;
}

/*
==================
BotCreateWayPoint
==================
*/
bot_waypoint_t *BotCreateWayPoint( char *name, vec3_t origin, int areanum ) {
	bot_waypoint_t *wp;
	vec3_t waypointmins = {-8, -8, -8}, waypointmaxs = {8, 8, 8};

	wp = botai_freewaypoints;
	if ( !wp ) {
		BotAI_Print( PRT_WARNING, "BotCreateWayPoint: Out of waypoints\n" );
		return NULL;
	}
	botai_freewaypoints = botai_freewaypoints->next;

	Q_strncpyz( wp->name, name, sizeof( wp->name ) );
	VectorCopy( origin, wp->goal.origin );
	VectorCopy( waypointmins, wp->goal.mins );
	VectorCopy( waypointmaxs, wp->goal.maxs );
	wp->goal.areanum = areanum;
	wp->next = NULL;
	wp->prev = NULL;
	return wp;
}

/*
==================
BotFindWayPoint
==================
*/
bot_waypoint_t *BotFindWayPoint( bot_waypoint_t *waypoints, char *name ) {
	bot_waypoint_t *wp;

	for ( wp = waypoints; wp; wp = wp->next ) {
		if ( !Q_stricmp( wp->name, name ) ) {
			return wp;
		}
	}
	return NULL;
}

/*
==================
BotFreeWaypoints
==================
*/
void BotFreeWaypoints( bot_waypoint_t *wp ) {
	bot_waypoint_t *nextwp;

	for (; wp; wp = nextwp ) {
		nextwp = wp->next;
		wp->next = botai_freewaypoints;
		botai_freewaypoints = wp;
	}
}

/*
==================
BotInitWaypoints
==================
*/
void BotInitWaypoints( void ) {
	int i;

	botai_freewaypoints = NULL;
	for ( i = 0; i < MAX_BOTAIWAYPOINTS; i++ ) {
		botai_waypoints[i].next = botai_freewaypoints;
		botai_freewaypoints = &botai_waypoints[i];
	}
}

/*
==================
TeamPlayIsOn
==================
*/
int TeamPlayIsOn( void ) {
	return qtrue; //( gametype == GT_TEAM || gametype == GT_CTF );
}

/*
==================
BotAggression

FIXME: move this to external fuzzy logic

  NOTE!!: I made no changes to this code for wolf weapon awareness.  (SA)
==================
*/
float BotAggression( bot_state_t *bs ) {
	//otherwise the bot is not feeling too good
	return 0;
}

/*
==================
BotWantsToRetreat
==================
*/
int BotWantsToRetreat( bot_state_t *bs ) {
/*	if (bs->engagementMatrix && (BotEngagementFunc(bs) & BOT_SEEK_COVER)) {
		return qtrue;
	}
	if (bs->enemy > -1 && BotCarryingFlag(bs->enemy)) {
		return qfalse;
	}
	if (BotAggression(bs) < 50) {
		return qtrue;
	}*/
	return qfalse;
}

/*
==================
BotWantsToChase
==================
*/
int BotWantsToChase( bot_state_t *bs ) {
	if ( BotCarryingFlag( bs->client ) ) {
		return qfalse;
	}
/*	if (bs->engagementMatrix && !(BotEngagementFunc(bs) & BOT_ROE_PURSUE)) {
		return qfalse;
	}*/
	if ( bs->enemy > -1 && BotCarryingFlag( bs->enemy ) ) {
		return qtrue;
	}
/*	if (BotAggression(bs) > 50) {
		return qtrue;
	}*/
	return qfalse;
}

/*
==================
BotWantsToHelp
==================
*/
int BotWantsToHelp( bot_state_t *bs ) {
	return qtrue;
}

/*
==================
BotCanAndWantsToRocketJump
==================
*/
int BotCanAndWantsToRocketJump( bot_state_t *bs ) {

	return qfalse;
/*
	float rocketjumper;

	//if rocket jumping is disabled
	if (!bot_rocketjump.integer) return qfalse;
	//if no rocket launcher
	if (bs->inventory[INVENTORY_ROCKETLAUNCHER] <= 0) return qfalse;
	//if low on rockets
	if (bs->inventory[INVENTORY_ROCKETS] < 3) return qfalse;
	//never rocket jump with the Quad
	if (bs->inventory[INVENTORY_QUAD]) return qfalse;
	//if low on health
	if (bs->inventory[INVENTORY_HEALTH] < 60) return qfalse;
	//if not full health
	if (bs->inventory[INVENTORY_HEALTH] < 90) {
		//if the bot has insufficient armor
		if (bs->inventory[INVENTORY_ARMOR] < 40) return qfalse;
	}
	rocketjumper = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_WEAPONJUMPING, 0, 1);
	if (rocketjumper < 0.5) return qfalse;
	return qtrue;
*/
}

/*
==================
BotGoCamp
==================
*/
void BotGoCamp( bot_state_t *bs, bot_goal_t *goal ) {
/*
	float camper;

	//set message time to zero so bot will NOT show any message
	bs->teammessage_time = 0;
	//set the ltg type
	bs->ltgtype = LTG_CAMP;
	//set the team goal
	memcpy(&bs->teamgoal, goal, sizeof(bot_goal_t));
	//get the team goal time
	camper = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CAMPER, 0, 1);
	if (camper > 0.99) bs->teamgoal_time = 99999;
	else bs->teamgoal_time = 120 + 180 * camper + random() * 15;
	//set the last time the bot started camping
	bs->camp_time = trap_AAS_Time();
	//the teammate that requested the camping
	bs->teammate = 0;
	//do NOT type arrive message
	bs->arrive_time = 1;
*/
}

/*
==================
BotWantsToCamp
==================
*/
/*int BotWantsToCamp(bot_state_t *bs) {
	float camper;
	int cs, traveltime, besttraveltime;
	bot_goal_t goal, bestgoal;

	camper = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CAMPER, 0, 1);
	if (camper < 0.1) return qfalse;
	//if the bot has a team goal
	if (bs->ltgtype == LTG_TEAMHELP ||
			bs->ltgtype == LTG_TEAMACCOMPANY ||
			bs->ltgtype == LTG_DEFENDKEYAREA ||
			bs->ltgtype == LTG_GETFLAG ||
			bs->ltgtype == LTG_RUSHBASE ||
			bs->ltgtype == LTG_CAMP ||
			bs->ltgtype == LTG_CAMPORDER ||
			bs->ltgtype == LTG_PATROL) {
		return qfalse;
	}
	//if camped recently
	if (bs->camp_time > trap_AAS_Time() - 60 + 300 * (1-camper)) return qfalse;
	//
	if (random() > camper) {
		bs->camp_time = trap_AAS_Time();
		return qfalse;
	}
	//if the bot isn't healthy anough
	if (BotAggression(bs) < 50) return qfalse;
	//the bot should have at least have the rocket launcher, the railgun or the bfg10k with some ammo
//	if ((bs->inventory[INVENTORY_ROCKETLAUNCHER] <= 0 || bs->inventory[INVENTORY_ROCKETS < 10])
//		&& (bs->inventory[INVENTORY_RAILGUN] <= 0 || bs->inventory[INVENTORY_SLUGS] < 10)
//		&& (bs->inventory[INVENTORY_BFG10K] <= 0 || bs->inventory[INVENTORY_BFGAMMO] < 10)
//		){
//		return qfalse;
//	}
	//find the closest camp spot
	besttraveltime = 99999;
	for (cs = trap_BotGetNextCampSpotGoal(0, &goal); cs; cs = trap_BotGetNextCampSpotGoal(cs, &goal)) {
		traveltime = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, goal.areanum, bs->tfl);
		if (traveltime && traveltime < besttraveltime) {
			besttraveltime = traveltime;
			memcpy(&bestgoal, &goal, sizeof(bot_goal_t));
		}
	}
	if (besttraveltime > 150) return qfalse;
	//ok found a camp spot, go camp there
	BotGoCamp(bs, &bestgoal);
	//
	return qtrue;
}*/

/*
==================
BotDontAvoid
==================
*/
void BotDontAvoid( bot_state_t *bs, char *itemname ) {
	bot_goal_t goal;
	int num;

	num = trap_BotGetLevelItemGoal( -1, itemname, &goal );
	while ( num >= 0 ) {
		trap_BotRemoveFromAvoidGoals( bs->gs, goal.number );
		num = trap_BotGetLevelItemGoal( num, itemname, &goal );
	}
}

/*
==================
BotGoForPowerups
==================
*/
void BotGoForPowerups( bot_state_t *bs ) {

	//don't avoid any of the powerups anymore
	BotDontAvoid( bs, "Quad Damage" );
	BotDontAvoid( bs, "Regeneration" );
	BotDontAvoid( bs, "Battle Suit" );
	BotDontAvoid( bs, "Speed" );
	BotDontAvoid( bs, "Invisibility" );
	//BotDontAvoid(bs, "Flight");
	//reset the long term goal time so the bot will go for the powerup
	//NOTE: the long term goal type doesn't change

}

/*
==================
BotRoamGoal
==================
*/
void BotRoamGoal( bot_state_t *bs, vec3_t goal ) {
	float len, r1, r2, sign, n;
	int pc;
	vec3_t dir, bestorg, belowbestorg;
	bsp_trace_t trace;

	for ( n = 0; n < 10; n++ ) {
		//start at the bot origin
		VectorCopy( bs->origin, bestorg );
		r1 = random();
		if ( r1 < 0.8 ) {
			//add a random value to the x-coordinate
			r2 = random();
			if ( r2 < 0.5 ) {
				sign = -1;
			} else { sign = 1;}
			bestorg[0] += sign * 700 * random() + 50;
		}
		if ( r1 > 0.2 ) {
			//add a random value to the y-coordinate
			r2 = random();
			if ( r2 < 0.5 ) {
				sign = -1;
			} else { sign = 1;}
			bestorg[1] += sign * 700 * random() + 50;
		}
		//add a random value to the z-coordinate (NOTE: 48 = maxjump?)
		bestorg[2] += 3 * 48 * random() - 2 * 48 - 1;
		//trace a line from the origin to the roam target
		BotAI_Trace( &trace, bs->origin, NULL, NULL, bestorg, bs->entitynum, MASK_SOLID );
		//direction and length towards the roam target
		VectorSubtract( bestorg, bs->origin, dir );
		len = VectorNormalize( dir );
		//if the roam target is far away anough
		if ( len > 200 ) {
			//the roam target is in the given direction before walls
			VectorScale( dir, len * trace.fraction - 40, dir );
			VectorAdd( bs->origin, dir, bestorg );
			//get the coordinates of the floor below the roam target
			belowbestorg[0] = bestorg[0];
			belowbestorg[1] = bestorg[1];
			belowbestorg[2] = bestorg[2] - 800;
			BotAI_Trace( &trace, bestorg, NULL, NULL, belowbestorg, bs->entitynum, MASK_SOLID );
			//
			if ( !trace.startsolid ) {
				trace.endpos[2]++;
				pc = trap_PointContents( trace.endpos,bs->entitynum );
				if ( !( pc & CONTENTS_LAVA ) ) {    //----(SA)	modified since slime is no longer deadly
//				if (!(pc & (CONTENTS_LAVA | CONTENTS_SLIME))) {
					VectorCopy( bestorg, goal );
					return;
				}
			}
		}
	}
	VectorCopy( bestorg, goal );
}

/*
==================
BotAttackMove
==================
*/
bot_moveresult_t BotAttackMove( bot_state_t *bs, int tfl ) {
	int movetype, i;
	float attack_skill, jumper, croucher, dist, strafechange_time;
	float attack_dist, attack_range;
	vec3_t forward, backward, sideward, hordir, up = {0, 0, 1}, end;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	bot_goal_t goal;
	trace_t tr;
	aas_clientmove_t move;

	if ( bs->attackchase_time > trap_AAS_Time() ) {
		//create the chase goal
		goal.entitynum = bs->enemy;
		goal.areanum = bs->lastenemyareanum;
		VectorCopy( bs->lastenemyorigin, goal.origin );
		VectorSet( goal.mins, -8, -8, -8 );
		VectorSet( goal.maxs, 8, 8, 8 );
		//initialize the movement state
		BotSetupForMovement( bs );
		//move towards the goal
		trap_BotMoveToGoal( &moveresult, bs->ms, &goal, tfl );
		return moveresult;
	}
	//
	memset( &moveresult, 0, sizeof( bot_moveresult_t ) );
	//
	attack_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1 );
	jumper = 0.05 * trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_JUMPER, 0, 1 );
	croucher = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CROUCHER, 0, 1 );
	//if the bot is really stupid
	if ( attack_skill < 0.2 ) {
		return moveresult;
	}
	// if the bot is in the air
	if ( bs->cur_ps.groundEntityNum == ENTITYNUM_NONE ) {
		return moveresult;
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//get the enemy entity info
	BotEntityInfo( bs->enemy, &entinfo );
	//direction towards the enemy
	VectorSubtract( entinfo.origin, bs->origin, forward );
	//the distance towards the enemy
	dist = VectorNormalize( forward );
	VectorNegate( forward, backward );
// RF, removed, has it's own AI Node now
/*	// if using mobile mg42, go prone if possible
	if (bs->weaponnum == WP_MOBILE_MG42) {
		if (dist > 1024.0f) {
			int oldviewheight;
			// check for obstruction at feet
			oldviewheight = level.clients[bs->client].ps.viewheight;
			level.clients[bs->client].ps.viewheight = PRONE_VIEWHEIGHT;
			if (BotVisibleFromPos( bs->origin, bs->client, BotGetOrigin( bs->enemy ), bs->enemy, qtrue )) {
				trap_EA_Prone( bs->client );
				level.clients[bs->client].ps.viewheight = oldviewheight;
				return moveresult;
			} else {
				// just crouch
				trap_EA_Crouch( bs->client );
				level.clients[bs->client].ps.viewheight = oldviewheight;
				return moveresult;
			}
		}
	}
*/
	// if the enemy is too far away
	if ( dist > 1350.0f || !BotMoveWhileFiring( bs->weaponnum ) ) {
		// crouching, no movement
		trap_EA_Crouch( bs->client );
		return moveresult;
	}
	//walk, crouch or jump
	movetype = MOVE_WALK;
	//
	if ( bs->attackcrouch_time < trap_AAS_Time() - 1 ) {
		if ( ( g_gametype.integer != GT_SINGLE_PLAYER && g_gametype.integer != GT_COOP ) && ( random() < jumper ) ) {
			movetype = MOVE_JUMP;
		}
		//wait at least one second before crouching again
		else if ( bs->attackcrouch_time < trap_AAS_Time() - 1 && random() < croucher ) {
			bs->attackcrouch_time = trap_AAS_Time() + croucher * 5;
		}
	}
	if ( bs->attackcrouch_time > trap_AAS_Time() ) {
		movetype = MOVE_CROUCH;
	} else if ( bs->script.flags & BSFL_CROUCH ) {
		movetype = MOVE_CROUCH;
	}
	//if the bot should jump
	if ( movetype == MOVE_JUMP ) {
		//if jumped last frame
		if ( bs->attackjump_time > trap_AAS_Time() ) {
			movetype = MOVE_WALK;
		} else {
			bs->attackjump_time = trap_AAS_Time() + 1;
		}
	}
	attack_dist = IDEAL_ATTACKDIST;
	attack_range = 40;
/*	//if the bot is stupid
	if (attack_skill <= 0.4) {
		//just walk to or away from the enemy
		if (dist > attack_dist + attack_range) {
			if (trap_BotMoveInDirection(bs->ms, forward, 400, movetype)) return moveresult;
		}
		if (dist < attack_dist - attack_range) {
			if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype)) return moveresult;
		}
		return moveresult;
	}
*/
	if ( BotSinglePlayer() || BotCoop() ) {
		if ( dist > 512 ) {
			// no attack move if they are far away
			return moveresult;
		}
	}
	//increase the strafe time
	bs->attackstrafe_time += bs->thinktime;
	//get the strafe change time
	strafechange_time = 0.7 + ( 1 - attack_skill ) * 0.2;
	if ( attack_skill > 0.7 ) {
		strafechange_time += crandom() * 0.2;
	}
	//if the strafe direction should be changed
	if ( bs->attackstrafe_time > strafechange_time ) {
		//some magic number :)
		if ( random() > 0.945 ) {
			//flip the strafe direction
			bs->flags ^= BFL_STRAFERIGHT;
			bs->attackstrafe_time = 0;
		}
	}
	//
	for ( i = 0; i < 2; i++ ) {
		hordir[0] = forward[0];
		hordir[1] = forward[1];
		hordir[2] = 0;
		VectorNormalize( hordir );
		//get the sideward vector
		CrossProduct( hordir, up, sideward );
		//reverse the vector depending on the strafe direction
		if ( bs->flags & BFL_STRAFERIGHT ) {
			VectorNegate( sideward, sideward );
		}
/*		//randomly go back a little
		if (random() > 0.9) {
			VectorAdd(sideward, backward, sideward);
		}
		else {
			//walk forward or backward to get at the ideal attack distance
			if (dist > attack_dist + attack_range) VectorAdd(sideward, forward, sideward);
			else if (dist < attack_dist - attack_range) VectorAdd(sideward, backward, sideward);
		}
*/                                                                                                                                                                                                                                                                                                                                                                  //
		// check this direction for trigger_hurt
		VectorMA( bs->origin, 80, sideward, end );
		trap_Trace( &tr, bs->origin, vec3_origin, vec3_origin, end, bs->client, CONTENTS_TRIGGER );
		if ( tr.fraction < 1.0 && !Q_stricmp( g_entities[tr.entityNum].classname, "trigger_hurt" ) ) {
			// hit something that will hurt us
			bs->flags ^= BFL_STRAFERIGHT;
			bs->attackstrafe_time = 0;
			return moveresult;
		}
		// check for walking off a ledge
		if ( trap_AAS_PredictClientMovement( &move, bs->client, bs->origin, -1, qtrue, sideward, vec3_origin, 10, 0, 0.1, SE_GAP | SE_LEAVEGROUND | SE_STUCK, -1, qfalse ) ) {
			// if this is a bad movement, flip the direction, and wait util next frame when we can check that that move is ok
			switch ( move.stopevent ) {
			case SE_GAP:
			case SE_LEAVEGROUND:
			case SE_STUCK:
				// bad movement
				bs->flags ^= BFL_STRAFERIGHT;
				bs->attackstrafe_time = 0;
				return moveresult;
			}
		} else {
			// dont move
			return moveresult;
		}
		//
		//perform the movement
		if ( trap_BotMoveInDirection( bs->ms, sideward, 190, movetype ) ) {
			return moveresult;
		}
		//movement failed, flip the strafe direction
		bs->flags ^= BFL_STRAFERIGHT;
		bs->attackstrafe_time = 0;
	}
	//bot couldn't do any usefull movement
//	bs->attackchase_time = AAS_Time() + 6;
	return moveresult;
}

/*
==================
BotSameTeam
==================
*/
int BotSameTeam( bot_state_t *bs, int entnum ) {
	// Gordon: this function is way too generic for this to work properly

/*	if (level.warmupTime > level.time) {
		// fight everyone in warmup mode
		return qfalse;
	}*/

	return OnSameTeam( &g_entities[bs->client], &g_entities[entnum] );
}

/*
==================
InFieldOfVision
==================
*/
qboolean InFieldOfVision( vec3_t viewangles, float fov, vec3_t angles ) {
	int i;
	float diff, angle;

	for ( i = 0; i < 2; i++ ) {
		angle = AngleMod( viewangles[i] );
		angles[i] = AngleMod( angles[i] );
		diff = angles[i] - angle;
		if ( angles[i] > angle ) {
			if ( diff > 180.0 ) {
				diff -= 360.0;
			}
		} else {
			if ( diff < -180.0 ) {
				diff += 360.0;
			}
		}
		if ( diff > 0 ) {
			if ( diff > fov * 0.5 ) {
				return qfalse;
			}
		} else {
			if ( diff < -fov * 0.5 ) {
				return qfalse;
			}
		}
	}
	return qtrue;
}

/*
==================
BotEntInvisibleBySmokeBomb

returns whether smoke from smoke bombs blocks vision from start to end
==================

  Mad Doc xkan, 11/25/2002
*/
#define MAX_SMOKE_RADIUS 320.0
#define MAX_SMOKE_RADIUS_TIME 10000.0
#define UNAFFECTED_BY_SMOKE_DIST SQR( 100 )

qboolean BotEntInvisibleBySmokeBomb( vec3_t start, vec3_t end ) {
	gentity_t *ent = NULL;
	vec3_t smokeCenter;
	float smokeRadius;

	// if the target is close enough, vision is not affected by smoke bomb
	if ( DistanceSquared( start,end ) < UNAFFECTED_BY_SMOKE_DIST ) {
		return qfalse;
	}

	while ( ( ent = G_FindSmokeBomb( ent ) ) ) {
		if ( ent->s.effect1Time == 16 ) {
			// xkan, the smoke has not really started yet, see weapon_smokeBombExplode
			// and CG_RenderSmokeGrenadeSmoke
			continue;
		}
		// check the distance
		VectorCopy( ent->s.pos.trBase, smokeCenter );
		// raise the center to better match the position of the smoke, see
		// CG_SpawnSmokeSprite().
		smokeCenter[2] += 32;
		// smoke sprite has a maximum radius of 640/2. and it takes a while for it to
		// reach that size, so adjust the radius accordingly.
		smokeRadius = MAX_SMOKE_RADIUS *
					  ( ( level.time - ent->grenadeExplodeTime ) / MAX_SMOKE_RADIUS_TIME );
		if ( smokeRadius > MAX_SMOKE_RADIUS ) {
			smokeRadius = MAX_SMOKE_RADIUS;
		}
		// if distance from line is short enough, vision is blocked by smoke
		if ( DistanceFromLineSquared( smokeCenter, start, end ) < smokeRadius * smokeRadius ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
==================
BotEntityVisible

returns visibility in the range [0, 1] taking fog and water surfaces into account
==================
*/
float BotEntityVisible( int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent, vec3_t entorigin ) {
	int i, contents_mask, passent, hitent, infog, inwater, otherinfog, pc;
	float fogdist = 0, waterfactor, vis, bestvis;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vec3_t dir, entangles, start, end, middle;
	int checkCount;

	//calculate middle of bounding box
	BotEntityInfo( ent, &entinfo );
	VectorAdd( entinfo.mins, entinfo.maxs, middle );
	VectorScale( middle, 0.5, middle );
	if ( entorigin ) {
		VectorAdd( entorigin, middle, middle );
	} else {
		VectorAdd( entinfo.origin, middle, middle );
	}

	// if the entity is using an mg42, then move the trace upwards to avoid the gun
	if ( g_entities[ent].s.eFlags & EF_MG42_ACTIVE ) {
		middle[2] += 16;
	}

	//check if entity is within field of vision
	VectorSubtract( middle, eye, dir );
	vectoangles( dir, entangles );
	if ( fov < 360 && !InFieldOfVision( viewangles, fov, entangles ) ) {
		return 0;
	}

	// RF, check PVS
	if ( !trap_InPVS( eye, middle ) ) {
		return 0.f;
	}

	// RF, if they are carrying the flag, then we can see them if they are in PVS
	if ( BotCarryingFlag( ent ) ) {
		return 1.f;
	}

	// RF, if they are far away, and we arent using a sniper rifle, then only do 1 check
	checkCount = 3;
	if ( ( botstates[viewer].inuse && BotCanSnipe( &botstates[viewer], qtrue ) ) && ( VectorLengthSquared( dir ) > SQR( 1024 ) ) ) {
		checkCount = 1;
	}

	pc = trap_AAS_PointContents( eye );
	infog = ( pc & CONTENTS_SOLID );
	inwater = ( pc & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) );

	bestvis = 0;
	for ( i = 0; i < checkCount; i++ ) {
		//
		contents_mask = MASK_SHOT & ~( CONTENTS_CORPSE ); //CONTENTS_SOLID|CONTENTS_PLAYERCLIP;
		passent = viewer;
		hitent = ent;
		VectorCopy( eye, start );
		VectorCopy( middle, end );
		//if the entity is in water, lava or slime
		if ( trap_AAS_PointContents( middle ) & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) {
			contents_mask |= ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER );
		}
		//if eye is in water, lava or slime
		if ( inwater ) {
			if ( !( contents_mask & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) ) {
				passent = ent;
				hitent = viewer;
				VectorCopy( middle, start );
				VectorCopy( eye, end );
			}
			contents_mask ^= ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER );
		}
		//trace from start to end
		BotAI_Trace( &trace, start, NULL, NULL, end, passent, contents_mask );
		//if water was hit
		waterfactor = 1.0;
		if ( trace.contents & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) {
			//if the water surface is translucent
			//trace through the water
			contents_mask &= ~( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER );
			BotAI_Trace( &trace, trace.endpos, NULL, NULL, end, passent, contents_mask );
			waterfactor = 0.5;
		}

		//if a full trace or the hitent was hit
		if ( trace.fraction >= .99f || trace.ent == hitent || ( ( entinfo.flags & EF_TAGCONNECT ) && ( g_entities[trace.ent].nextTrain == g_entities[ent].tagParent ) ) ) {

			//check for fog, assuming there's only one fog brush where
			//either the viewer or the entity is in or both are in
			otherinfog = ( trap_AAS_PointContents( middle ) & CONTENTS_FOG );
			if ( infog && otherinfog ) {
				VectorSubtract( trace.endpos, eye, dir );
				fogdist = VectorLength( dir );
			} else if ( infog ) {
				VectorCopy( trace.endpos, start );
				BotAI_Trace( &trace, start, NULL, NULL, eye, viewer, CONTENTS_FOG );
				VectorSubtract( eye, trace.endpos, dir );
				fogdist = VectorLength( dir );
			} else if ( otherinfog ) {
				VectorCopy( trace.endpos, end );
				BotAI_Trace( &trace, eye, NULL, NULL, end, viewer, CONTENTS_FOG );
				VectorSubtract( end, trace.endpos, dir );
				fogdist = VectorLength( dir );
			} else {
				//if the entity and the viewer are not in fog assume there's no fog in between
				fogdist = 0;

			} // else...

			//decrease visibility with the view distance through fog
			vis = 1 / ( ( fogdist * fogdist * 0.001 ) < 1 ? 1 : ( fogdist * fogdist * 0.001 ) );

			//if entering water visibility is reduced
			vis *= waterfactor;

			// xkan 11/25/2002 - are there smoke (from smoke bombs) blocking the sight?
			if ( vis > 0 && BotEntInvisibleBySmokeBomb( start, end ) ) {
				vis = 0;
			}

			if ( vis > bestvis ) {
				bestvis = vis;
			}

			// if pretty much no fog
			if ( bestvis >= 0.95 ) {
				return bestvis;
			}
		}

		//check bottom and top of bounding box as well
		if ( i == 0 ) {
			middle[2] += entinfo.mins[2];
		} else if ( i == 1 ) {
			middle[2] += entinfo.maxs[2] - entinfo.mins[2];
		}
	}
	return bestvis;
}

/*
================
BotVisibleFromPos
================
*/
qboolean BotVisibleFromPos( vec3_t srcorigin, int srcnum, vec3_t destorigin, int destent, qboolean dummy ) {
	vec3_t eye;
	//
	VectorCopy( srcorigin, eye );
	eye[2] += level.clients[srcnum].ps.viewheight;
	//
	if ( BotEntityVisible( srcnum, eye, vec3_origin, 360, destent, destorigin ) ) {
		return qtrue;
	}
	//
	return qfalse;
}

/*
================
BotCheckAttackAtPos

  FIXME: do better testing here
================
*/
qboolean BotCheckAttackAtPos(   int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld ) {
	vec3_t eye;
	//
	VectorCopy( pos, eye );
	eye[2] += level.clients[entnum].ps.viewheight;
	//
	if ( BotEntityVisible( entnum, eye, vec3_origin, 360, enemy, NULL ) ) {
		return qtrue;
	}
	//
	return qfalse;
}

/*
==================
BotGetMovementAutonomyLevel
==================
*/
int BotGetMovementAutonomyLevel( bot_state_t *bs ) {
	return BMA_HIGH;
}

/*
=====================
BotGetMovementAutonomyPos

  returns qtrue if a valid pos was returned
=====================
*/
qboolean BotGetMovementAutonomyPos( bot_state_t *bs, vec3_t pos ) {
	// if we are doing a scripted move, then make the destination our autonomy pos
	if ( bs->script.frameFlags & BSFFL_MOVETOTARGET ) {
		if ( bs->target_goal.entitynum == bs->script.entityNum ) {
			//VectorCopy( g_entities[bs->leader].s.origin, pos );
			//VectorCopy( pos, bs->script.movementAutonomyPos );		// use this as the new autonomy pos
			//VectorCopy( pos, bs->movementAutonomyPos );		// use this as the new autonomy pos

			// TAT 10/8/2002 - set in AIEnter_SP_Script_MoveToMarker
			VectorCopy( bs->movementAutonomyPos, pos );
			return qtrue;
		}
	}

	// if we have been commanded by a player, then only override that
	// if scripting has issued a "force" command
	if ( bs->movementAutonomy != BMA_NOVALUE ) {
		if ( !( bs->script.flags & BSFL_FORCED_MOVEMENT_AUTONOMY ) ) {
			// if we are following a leader
			VectorCopy( bs->movementAutonomyPos, pos );
			return qtrue;
		}
	}

	// if the scripting has set the autonomy
	if ( bs->script.movementAutonomy != BMA_NOVALUE ) {
		// if we are following a leader
		VectorCopy( bs->script.movementAutonomyPos, pos );
		return qtrue;
	}

	return qfalse;
}

float BotGetFollowAutonomyDist( bot_state_t *bs );

/*
==================
BotGetRawMovementAutonomyRange
==================
*/
float BotGetRawMovementAutonomyRange( bot_state_t *bs ) {
	float range;
	int level;
	//
	// RF, if we are in followme mode, then use special following ranges
	//		TAT 12/16/2002 - Don't use this range in SP here - if we want the follow distance, we check it explicitly
	if ( ( bs->leader >= 0 ) && !G_IsSinglePlayerGame() ) {
		range = BotGetFollowAutonomyDist( bs );
		return range;
	}
	//
	level = BotGetMovementAutonomyLevel( bs );
	if ( level > BMA_HIGH ) {
		G_Printf( "BotGetMovementAutonomyRange(): autonomy exceeds BMA_HIGH\n" );
		return 0;   // should never happen
	} else if ( level < BMA_NOVALUE ) {
		G_Printf( "BotGetMovementAutonomyRange(): autonomy range less than BMA_NOVALUE\n" );
		return 0;   // should never happen
	}

	// Use a different value for SP
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		// Use the special SP table
		range = movementAutonomyRangeSP[level];
		return range;
	}

	//
	range = movementAutonomyRange[level];
	return range;
}
/*
==================
BotGetMovementAutonomyRange
==================
*/
float BotGetMovementAutonomyRange( bot_state_t *bs, bot_goal_t *goal ) {
	float range;

	// Get the basic movement autonomy range
	range = BotGetRawMovementAutonomyRange( bs );

	// medium urgency goals let us wander slightly outside the autonomy range
	if ( goal && goal->urgency >= BGU_MEDIUM ) {
		range = ( range * ( 1.0 + 0.25 * ( goal->urgency - BGU_LOW ) ) ) + 256 * ( goal->urgency - BGU_LOW );
	}

	//
	return range;
}

// Start - TAT 9/18/2002
//	What distance should a bot be from its leader during a follow order?  Based on autonomy
float BotGetFollowAutonomyDist( bot_state_t *bs ) {
	int level;

	// The autonomy distances have nothing to do with how far a unit should be for following
	//		Let's make up some constants
	static float followAutonomyDist[NUM_BMA] =
	{
		128,            // LOW
		256,            // MEDIUM
		384,            // HIGH
	};

	level = BotGetMovementAutonomyLevel( bs );
	if ( level > BMA_HIGH ) {
		G_Printf( "BotGetMovementAutonomyRange(): autonomy exceeds BMA_HIGH\n" );
		return 0;   // should never happen
	} else if ( level < BMA_NOVALUE ) {
		G_Printf( "BotGetMovementAutonomyRange(): autonomy range less than BMA_NOVALUE\n" );
		return 0;   // should never happen
	}

	return followAutonomyDist[level];
}

// Is a bot within the desired distance of its leader?
qboolean BotWithinLeaderFollowDist( bot_state_t *bs ) {
	float dist;
	gentity_t* leader;

	if ( !BotSinglePlayer() && !BotCoop() ) {
		return qtrue;
	}

	// if there's no leader, than sure, we're close enough
	if ( bs->leader == -1 ) {
		return qtrue;
	}

	// get the leader
	leader = BotGetEntity( bs->leader );

	// how far is the bot from the leader?
	dist = BotGetFollowAutonomyDist( bs );
	dist *= dist;

	// is the distance is greater than the follow dist
	if ( dist > VectorDistanceSquared( bs->origin, leader->r.currentOrigin ) ) {
		return qfalse;
	}

	return qtrue;
}

// End - TAT 9/18/2002

/*
==================
BotPointWithinMovementAutonomy
==================
*/
qboolean BotPointWithinMovementAutonomy( bot_state_t *bs, bot_goal_t *goal, vec3_t point ) {
	float dist;
	vec3_t pos;
	//
	// no autonomy in MP
	if ( !BotSinglePlayer() && !BotCoop() ) {
		return qtrue;
	}
	//
	if ( !BotGetMovementAutonomyPos( bs, pos ) ) {
		return qtrue;   // no autonomy yet defined
	}
	//
	dist = VectorDistance( pos, point );
	//
	// must be outside range of current goal origin
	if ( dist > BotGetMovementAutonomyRange( bs, goal ) ) {
		// also if we are following a leader, we are allowed to be near them
		if ( bs->leader >= 0 ) {
			dist = VectorDistance( g_entities[bs->leader].r.currentOrigin, point );
			if ( dist > BotGetMovementAutonomyRange( bs, goal ) ) {
				return qfalse;
			}
		} else {
			return qfalse;
		}
	}
	//
	// RF, had to remove, alcoves were causing this to fail, when they shouldn't be
	//if ( !trap_InPVS( pos, point ) ) {
	//	return qfalse;
	//}
	//
	return qtrue;
}

qboolean BotPointWithinRawMovementAutonomy( bot_state_t *bs, vec3_t point ) {
	float dist;
	vec3_t pos;

	if ( !BotGetMovementAutonomyPos( bs, pos ) ) {
		return qtrue;   // no autonomy yet defined

	}
	dist = VectorDistance( pos, point );

	// must be outside range of current goal origin
	if ( dist > BotGetRawMovementAutonomyRange( bs ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
==================
BotGoalWithinMovementAutonomy
==================
*/
qboolean BotGoalWithinMovementAutonomy( bot_state_t *bs, bot_goal_t *goal, int urgency ) {
	int i;
	botIgnoreGoal_t *ignoreTrav;
	vec3_t pos;

	// no autonomy in MP
	if ( !BotSinglePlayer() && !BotCoop() ) {
		return qtrue;
	}
	//
	if ( !BotGetMovementAutonomyPos( bs, pos ) ) {
		return qtrue;   // no autonomy yet defined
	}
	//
	// if this goal is in our ignore list, then ignore it
	for ( i = 0, ignoreTrav = bs->ignoreGoals; i < MAX_IGNORE_GOALS; i++, ignoreTrav++ ) {
		if ( !ignoreTrav->expireTime || ignoreTrav->expireTime <= level.time ) {
			continue;
		}
		if ( ignoreTrav->entityNum != goal->entitynum ) {
			continue;
		}
		if ( ignoreTrav->areanum != goal->areanum ) {
			continue;
		}
		// has our autonomy pos changed?
		if ( !VectorCompare( pos, ignoreTrav->autonomyPos ) ) {
			ignoreTrav->expireTime = 0;
			continue;
		}
		// this ignoreGoal relates to this goal, so ignore it
		return qfalse;
	}
	//
	return BotPointWithinMovementAutonomy( bs, goal, goal->origin );
}

/*
==================
BotGoalForEntity

  returns qfalse if the goal is outside our movement autonomy range

  if bs is NULL, qtrue is always returned
==================
*/
qboolean BotGoalForEntity( bot_state_t *bs, int entityNum, bot_goal_t *goal, int urgency ) {
	vec3_t p;

	gentity_t *ent = BotGetEntity( entityNum );
	BotClearGoal( goal );

	if ( ent ) {
		goal->entitynum = entityNum;
		if ( VectorLengthSquared( ent->r.absmax ) && ( /*ent->s.eType == ET_MOVER ||*/ ent->s.eType == ET_GENERAL ) ) {
			VectorAdd( ent->r.absmax, ent->r.absmin, p );
			VectorScale( p, 0.5, p );

			if ( bs && !BotGetReachableEntityArea( bs, entityNum, goal ) ) {
				return qfalse;
			}
		} else {
			VectorCopy( ent->r.currentOrigin, p );
			p[2] += 30;
			VectorCopy( ent->r.mins, goal->mins );
			VectorCopy( ent->r.maxs, goal->maxs );
		}

		if ( !goal->areanum ) {
			goal->areanum = trap_AAS_PointAreaNum( p );
			if ( !goal->areanum || !trap_AAS_AreaReachability( goal->areanum ) ) {
				goal->areanum = BotPointAreaNum( -1, p );
			}
		}

		// RF, drop out if no area is found
		if ( !goal->areanum ) {
			return qfalse;
		}
	} else {
		// TAT - try server entity
		g_serverEntity_t *serverEnt = GetServerEntity( entityNum );
		if ( serverEnt ) {
			goal->entitynum = entityNum;
			VectorCopy( serverEnt->origin, p );
			p[2] += 30;
			if ( !( goal->areanum = BotGetArea( entityNum ) ) ) {
				return qfalse;
			}
		} else {
			return qfalse;
		}
	}

	goal->number = -1;
	VectorCopy( p, goal->origin );
	goal->urgency = urgency;

	if ( bs ) {
		if ( !BotGoalWithinMovementAutonomy( bs, goal, urgency ) ) {
			return qfalse;
		}
		return qtrue;
	} else {
		return qtrue;
	}
}



// Start - TAT 8/26/2002
// NOTE:  The following 3 funcs to create and destroy the bot indicator objects should probably live somewhere else
//		but I don't have time to figure out where
#define BOTINDICATORSET_POSTDELAY_TIME  1000    // can only set a waypoint once every 1 seconds

void botindicator_think( gentity_t *ent ) {

	if ( level.time - ent->lastHintCheckTime < BOTINDICATORSET_POSTDELAY_TIME ) {
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	ent->nextthink = level.time + FRAMETIME;

}

// a helper func, this returns the angle between 2 vectors
float sAngleBetweenVectors( vec3_t a, vec3_t b ) {
	float val = DotProduct( a, b ) / sqrt( DotProduct( a, a ) * DotProduct( b, b ) );

	if ( val <= -1.0f ) {
		return (float)M_PI;
	} else if ( val >= 1.0f ) {
		return 0.0f;
	}

	return acos( val );
}

// Gordon: 25/11/02: removing alot of the statics on these funcs, need them elsewhere, rename?
// Move a point back towards the player a bit, and down to the floor
void sAdjustPointTowardsPlayer( vec3_t playerLoc, vec3_t endPos, qboolean shouldLoop, vec3_t outPos /* Gordon: vec3_t is an array, no need for the pointer (was vec3_t*) */ ) {
	vec3_t diff, normalizedDiff, scaled;
	trace_t trace;
	vec3_t point;
	// put in a max number of times we'll do this loop
	int timesThrough = 0;

	// amount to move back from the wall if our goal is on it
	const float wallDist = 75.0f;

	// Vector pointing straight up
	vec3_t floorNormal;
	// setup the floor normal
	VectorSet( floorNormal, 0, 0, 1 );

	VectorCopy( endPos, point );

	do
	{
		// gotta move the point
		//	New Loc = endpos + k * normalized(playerLoc - endpos)

		// diff = goal point - our start point
		VectorSubtract( playerLoc, point, diff );

		// normalize that vector
		VectorNormalize2( diff, normalizedDiff );

		// scale that vector by a constant amount, then stuff it in muzzlePoint
		VectorScale( normalizedDiff, wallDist, scaled );

		// and finally add back the original end point to get the final goal
		VectorAdd( point, scaled, outPos );

		// Drop the point to the floor, we don't want the bots trying to climb the walls or anything
		//		Started with some code from BotDropToFloor

		// We're going to do a trace from our goal point to a new location that is way below the old one
		//		the trace will stop when it hits something solid
		VectorSet( diff, outPos[0], outPos[1], outPos[2] - 4096 );
		trap_Trace( &trace, outPos, NULL, NULL, diff, -1, MASK_PLAYERSOLID );

		// now copy the result of the trace into our loc
		VectorCopy( trace.endpos, outPos );

		VectorCopy( trace.endpos, point );
		timesThrough++;
		// only loop if we're supposed to, and don't do it more than 10 times
	} while ( shouldLoop && ( timesThrough < 10 ) && sAngleBetweenVectors( floorNormal, trace.plane.normal ) >= M_PI / 4 );
}

// END - TAT 9/16/2002

/*
=====================
BotCheckMovementAutonomy

  if we wander outside out movement autonomy, it better be for a bloody good reason

	returns qtrue if we have moved outside autonomy range, and need to get back
=====================
*/
qboolean BotCheckMovementAutonomy( bot_state_t *bs, bot_goal_t *goal ) {
	// no autonomy in MP
	if ( !BotSinglePlayer() && !BotCoop() ) {
		return qfalse;
	}

	// is our current goal too important to abort?
	if ( goal->urgency == BGU_MAXIMUM ) {
		return qfalse;
	}

	// are we outside range?
	if ( !BotPointWithinMovementAutonomy( bs, goal, bs->origin ) ) {
		return qtrue;
	}

	// is our goal outside the range?
	if ( !VectorCompare( vec3_origin, goal->origin ) && !BotPointWithinMovementAutonomy( bs, goal, goal->origin ) ) {
		return qtrue;
	}

	// everything is ok
	return qfalse;
}

/*
=================
BotIgnoreGoal
=================
*/
void BotIgnoreGoal( bot_state_t *bs, bot_goal_t *goal, int duration ) {
	int i;
	botIgnoreGoal_t *ignoreTrav, *ignoreOldest = NULL;
	vec3_t pos;

	// no autonomy in MP
	if ( !BotSinglePlayer() && !BotCoop() ) {
		return;
	}

	if ( !BotGetMovementAutonomyPos( bs, pos ) ) {
		return; // no autonomy yet defined
	}

	// are we sure we aren't already ignoring this goal?
	for ( i = 0, ignoreTrav = bs->ignoreGoals; i < MAX_IGNORE_GOALS; i++, ignoreTrav++ ) {
		if ( goal->entitynum >= 0 && ignoreTrav->entityNum == goal->entitynum ) {
			// found a match, update expiryTime and get out of here
			ignoreTrav->areanum = goal->areanum;
			ignoreTrav->entityNum = goal->entitynum;
			VectorCopy( pos, ignoreTrav->autonomyPos );
			ignoreTrav->expireTime = level.time + duration;
			return;
		}
	}

	// find a free slot
	for ( i = 0, ignoreTrav = bs->ignoreGoals; i < MAX_IGNORE_GOALS; i++, ignoreTrav++ ) {
		if ( ignoreTrav->expireTime >= level.time ) {
			if ( ( ignoreTrav->expireTime < level.time + duration ) && ( !ignoreOldest || ( ignoreOldest->expireTime < ignoreTrav->expireTime ) ) ) {
				ignoreOldest = ignoreTrav;
			}
			continue;
		}
		// this slot is free
		ignoreTrav->areanum = goal->areanum;
		ignoreTrav->entityNum = goal->entitynum;
		VectorCopy( pos, ignoreTrav->autonomyPos );
		ignoreTrav->expireTime = level.time + duration;
		return;
	}
}

/*
==================
BotDangerousGoal

  returns qtrue if the given goal is in a dangerous area (dynamite, grenade, etc)
==================
*/
#define MAX_DANGER_ENTS 64

qboolean BotDangerousGoal( bot_state_t *bs, bot_goal_t *goal ) {
	int i, j;
	vec3_t bangPos;
	float radius = 0.0;

	gentity_t *trav;
	int dangerEnts[MAX_DANGER_ENTS];
	int dangerEntsCount = 0;

	if ( bs->last_dangerousgoal > level.time - 300 ) {
		return qfalse;
	}
	bs->last_dangerousgoal = level.time + rand() % 100;

	if ( dangerEntsCount < MAX_DANGER_ENTS ) {
		if ( level.time - level.clients[bs->client].lastConstructibleBlockingWarnTime < 5000 ) {
			dangerEnts[dangerEntsCount] = level.clients[bs->client].lastConstructibleBlockingWarnEnt;
			dangerEntsCount++;
		}
	}

	trav = g_entities;
	for ( j = 0; j < level.num_entities; j++, trav++ ) {
		if ( !trav->inuse ) {
			continue;
		}

		switch ( trav->s.eType ) {
		case ET_FLAMETHROWER_CHUNK:
			break;
		case ET_PLAYER:
			if ( trav->client ) {
				// is this player dangerous?
				if ( !( trav->client->ps.weapon == WP_PANZERFAUST && trav->client->ps.weaponDelay ) ) {
					continue;
				}
			}
			break;
		case ET_MISSILE:
			switch ( trav->methodOfDeath ) {
			case MOD_ARTY:
				break;
			case MOD_DYNAMITE:
				if ( trav->s.teamNum >= 4 ) {
					continue;           // not armed
				}
				break;
			case MOD_LANDMINE:
				if ( !G_LandmineArmed( trav ) ) {
					continue;
				}

				if ( G_LandmineTeam( trav ) != bs->sess.sessionTeam ) {
					if ( !G_LandmineSpotted( trav ) ) {
						continue;
					}
				}

				if ( goal->entitynum == trav->s.number ) {
					continue;
				}
			case MOD_PANZERFAUST:
			case MOD_GRENADE:
			case MOD_GRENADE_LAUNCHER:
				break;
			default:
				continue;
			}
			break;
		default:
			continue;
		}

		// save this to the cache
		dangerEnts[dangerEntsCount] = j;
		dangerEntsCount++;

		if ( dangerEntsCount >= MAX_DANGER_ENTS ) {
			break;  // too many
		}
	}

	for ( i = 0; i < dangerEntsCount; i++ ) {
		trav = &g_entities[dangerEnts[i]];

		switch ( trav->s.eType ) {
		case ET_MISSILE:
			switch ( trav->methodOfDeath ) {
			case MOD_PANZERFAUST:
			case MOD_GRENADE:
			case MOD_GRENADE_LAUNCHER:
				if ( trav->nextthink > level.time + ( 2000 + 8 * trav->splashRadius ) ) {
					continue;
				}
				if ( !G_PredictMissile( trav, trav->nextthink - level.time, bangPos, trav->methodOfDeath == MOD_PANZERFAUST ? qfalse : qtrue ) ) {
					// not dangerous
					continue;
				}
				break;
			default:
				VectorCopy( trav->r.currentOrigin, bangPos );
				break;
			}
			break;
		default:
			VectorCopy( trav->r.currentOrigin, bangPos );
			break;
		}

		if ( !trap_InPVS( bs->origin, bangPos ) ) {
			continue;
		}

		// if the bot is zoomed, they cant see it unless they are looking at it
		if ( BG_IsScopedWeapon( bs->cur_ps.weapon ) ) {
			vec3_t dir, ang;

			VectorSubtract( trav->r.currentOrigin, bs->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ang );
			if ( !InFieldOfVision( bs->viewangles, 15, ang ) ) {
				continue;
			}
		}

		switch ( trav->s.eType ) {
		case ET_FLAMETHROWER_CHUNK:
			radius = 128;
			break;
		case ET_PLAYER:
			radius = 512;
			break;
		case ET_CONSTRUCTIBLE:
			radius = 32;
			break;
		case ET_MISSILE:
			switch ( trav->methodOfDeath ) {
			case MOD_ARTY:
				radius = 550;
				break;
			case MOD_DYNAMITE:
			case MOD_LANDMINE:
			case MOD_PANZERFAUST:
			case MOD_GRENADE:
			case MOD_GRENADE_LAUNCHER:
				radius = trav->splashRadius;
				break;
			default:         // rain - default
				radius = 0;
				break;
			}
			break;
		default:     // rain - default
			radius = 0;
			break;
		}

		// are we far enough away from it now?
		if ( VectorDistanceSquared( bs->origin, bangPos ) > SQR( radius + 500 ) ) {
			continue;
		}

		//
		// will it explode near the goal?
		if ( trav->s.eType == ET_CONSTRUCTIBLE || ( VectorDistanceSquared( bangPos, goal->origin ) < SQR( radius + 90 ) ) || ( VectorDistanceSquared( bangPos, bs->origin ) < SQR( radius + 90 ) ) ) {
			// we must avoid this danger
			BotClearGoal( &bs->avoid_goal );
			bs->avoid_goal.entitynum = trav->s.number;
			bs->avoid_goal.number = radius;
			if ( trav->s.eType == ET_MISSILE && trav->methodOfDeath == MOD_ARTY ) {
				bs->avoid_goal.goalEndTime = level.time + 15000;
			} else {
				bs->avoid_goal.goalEndTime = level.time + 100;
			}
			VectorCopy( bangPos, bs->avoid_goal.origin );
			bs->avoid_spawnCount = trav->spawnCount;

			return qtrue;
		}
	}

	return qfalse;
}



/*
===================
BotNoLeaderPenalty
===================

  We can have a penalty associated with not being near a leader.
  0 == no penalty, 1 == max penalty

*/
float BotNoLeaderPenalty( bot_state_t *bs ) {
	//@COOPTODO.  Make this look for any allied player
	// Distance to nearest allied player
	float distanceToPlayer = 0;

	// What sort of penalty do we get for being away from a leader
	float noLeaderPenalty = 0;

	// How far are we from the player?
	distanceToPlayer = VectorDistance( bs->origin, g_entities[0].r.currentOrigin );

	// If we're really close, or we're Nazis, don't bother
	if ( ( distanceToPlayer <= kBOT_NEAR_LEADER_DISTANCE )
		 || ( bs->mpTeam == TEAM_AXIS )
		 ) {
		// Jsut use the entity damage ratio
		return 0;

	}

	// If we're really far, max out the farness
	if ( distanceToPlayer >= kBOT_FAR_FROM_LEADER_DISTANCE ) {
		// Just use the max distance
		distanceToPlayer = kBOT_FAR_FROM_LEADER_DISTANCE;

	} // if (distanceToPlayer >= kBOT_FAR_FROM_LEADER_DISTANCE)...

	// What's our penalty?
	noLeaderPenalty = ( distanceToPlayer - kBOT_NEAR_LEADER_DISTANCE )
					  / ( kBOT_FAR_FROM_LEADER_DISTANCE - kBOT_NEAR_LEADER_DISTANCE );

	// Send the penalty back
	return noLeaderPenalty;

}



/*
==================
BotFindLeadersEnemy

Inherit a target from the leader, if one exists.
==================
*/
/*int BotFindLeadersEnemy(bot_state_t *bs, int curenemy, aas_entityinfo_t *entinfo)
{
	//
	// if we dont have a curenemy, and our leader has an enemy, we should know about it
	if (curenemy < 0 && bs->leader_tagent >= 0 && botstates[bs->leader_tagent].inuse) {
		if (botstates[bs->leader_tagent].enemy >= 0) {
			BotEntityInfo(botstates[bs->leader_tagent].enemy, entinfo);
			if (!EntityIsDead(entinfo)) {
				// if we can see our leader
				if (BotEntityVisible( bs->client, bs->eye, bs->viewangles, 360, bs->leader_tagent, BotGetOrigin(bs->leader_tagent) ) ) {
					//found an enemy
					bs->enemy = botstates[bs->leader_tagent].enemy;
					// if they aren't visible, and we dont want to chase, then ignore them
					if (!BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL) && !BotWantsToChase(bs)) {
						bs->enemy = -1;
					} else {
						if (curenemy >= 0)
							bs->enemysight_time = trap_AAS_Time() - 2;
						else
							bs->enemysight_time = trap_AAS_Time();
//						bs->enemysuicide = qfalse;
						bs->enemydeath_time = 0;
						bs->last_findenemy_enemy = bs->enemy;
						if (!bs->last_enemysight || (bs->last_enemysight < level.time - 10000)) {
							Bot_ScriptEvent( bs->client, "enemysight", !(g_entities[bs->enemy].r.svFlags & SVF_BOT) ? g_entities[bs->enemy].client->pers.netname : g_entities[bs->enemy].scriptName );
						}
						bs->last_enemysight = level.time;
						return qtrue;
					}
				}
			}
		}
	}

	// We did not inherit a target from our leader
	return qfalse;

} // int BotFindLeadersEnemy(bot_state_t *bs, int curenemy) ... */

/*=====================
change the bot's alert state

  return whether the alert state was changed
=====================*/
qboolean ChangeBotAlertState( bot_state_t *bs, aistateEnum_t newAlertState, qboolean force ) {
	if ( force ) { // should only be used by script action
		aistateEnum_t oldState = bs->alertState;
		bs->alertState = newAlertState;
		bs->alertStateChangeTime = level.time;
		bs->alertStateSetTime = level.time;
		return ( bs->alertState != oldState );
	}

	if ( newAlertState != bs->alertState
		 && level.time > bs->alertStateAllowChangeTime
		 // if we are relaxing, make sure we don't relax too quickly, this would prevent model from
		 // twitching in some weird situations (alert->combat->alert->combat in rapid fire fashion).
		 && ( newAlertState > bs->alertState || level.time - bs->alertStateChangeTime > 2000 ) ) {
		bs->alertState = newAlertState;
		bs->alertStateChangeTime = level.time;
		bs->alertStateSetTime = level.time;
		return qtrue;
	} else if ( newAlertState == bs->alertState )     {
		// record the time although we simply reaffirmed our current alert state.
		bs->alertStateSetTime = level.time;
	}

	return qfalse;
}

void BotUpdateAlertStateSquadSensingInfo( bot_state_t *bs,  qboolean canSeeTarget, qboolean heardFootSteps,  qboolean heardShooting ) {
}



/*
================
BotGetEntitySurfaceSoundCoefficient

Return a modifier for how loud a bot is depending on the surface they're moving on

================
*/
float BotGetEntitySurfaceSoundCoefficient( int clientNum ) {
	if ( g_entities[clientNum].surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( g_entities[clientNum].surfaceFlags & SURF_METAL ) {
		return 2.0f;
	}

	if ( g_entities[clientNum].surfaceFlags & SURF_WOOD ) {
		return 1.5f;
	}

	if ( g_entities[clientNum].surfaceFlags & SURF_GRASS ) {
		return 0.6f;
	}

	if ( g_entities[clientNum].surfaceFlags & SURF_GRAVEL ) {
		return 1.2f;
	}

	if ( g_entities[clientNum].surfaceFlags & SURF_ROOF ) {
		return 1.3f;
	}

	if ( g_entities[clientNum].surfaceFlags & SURF_SNOW ) {
		return 1.0f;
	}

	if ( g_entities[clientNum].surfaceFlags & SURF_CARPET ) {
		return 0.9f;
	}
	return 1.0f;
}






//
// BotIsValidTarget
//
// Description: Is this target at all valid?
// Written: 10/31/2002
//
qboolean BotIsValidTarget
(
	bot_state_t *bs,
	int target,
	int curenemy
) {
	// Local Variables ////////////////////////////////////////////////////////

	// Info about the target
	aas_entityinfo_t entinfo;

	///////////////////////////////////////////////////////////////////////////

	// Fill in the info for this potential target
	BotEntityInfo( target, &entinfo );

	// We are not our own enemy.  Ever.  At least in the game.  In real life, it
	// happens all the time.
	if ( target == bs->client ) {
		return qfalse;
	}

	// if it's the current enemy, don't do more analysis
	if ( target == curenemy ) {
		return qfalse;
	}

	// Fill in the info for this potential target
	BotEntityInfo( target, &entinfo );

	// Skip clients who are no longer good.
	if ( !entinfo.valid ) {
		return qfalse;
	}

	// See if the enemy has a notarget on it, and skip if so.
	if ( g_entities[target].flags & FL_NOTARGET ) {
		return qfalse;
	}

	//if on the same team
	if ( BotSameTeam( bs, target ) ) {
		return qfalse;
	}

	// xkan, 1/6/2003 - if target is civilian
	if ( g_entities[target].client->isCivilian ) {
		return qfalse;  // don't target civilian

	}
	//if the enemy isn't dead and the enemy isn't the bot self
	if ( EntityIsDead( &entinfo ) || entinfo.number == bs->entitynum ) {
		return qfalse;
	}

	//if the enemy is invisible and not shooting
	if ( EntityIsInvisible( &entinfo ) && !EntityIsShooting( &entinfo ) ) {
		return qfalse;
	}

	// TAT 10/10/2002
	//		If it's disguised, don't target them
	if ( g_entities[target].client->ps.powerups[PW_OPS_DISGUISED] ) {
		return qfalse;
	}

	// If we are not mounted on an MG42, or sniping, then ignore clients that are not in a valid area
	if ( !( bs->cur_ps.eFlags & EF_MG42_ACTIVE ) && !( bs->flags & BFL_SNIPING ) ) {
		// if they are not in a valid area
		if ( !BotGetArea( target ) ) {
			return qfalse;
		}

	} // if (!(bs->script.flags & BSFL_MOUNT_MG42))...

	// He's OK to use as a target
	return qtrue;
}
//
// BotIsValidTarget
//



//
// BotFindEnemies
//
// Description: Set up our danger spots
// Written: 12/26/2002
//
void BotFindEnemies
(
	bot_state_t *bs,
	int *dangerSpots,
	int *dangerSpotCount
) {
	int i;
	float dist;
	aas_entityinfo_t entinfo;
	vec3_t dir;

	// Loop through all potential targets
	for ( i = 0; i < level.maxclients; i++ )
	{

		// Fill in the info for this potential target
		BotEntityInfo( i, &entinfo );

		// Is it a valid target?  Puts all checks in one place
		if ( !BotIsValidTarget( bs, i, -1 ) ) {
			continue;
		}

		//calculate the distance towards the enemy
		VectorSubtract( entinfo.origin, bs->origin, dir );
		dist = VectorLength( dir );

		// if this enemy is too far, skip her
		if ( dist > kBOT_MAX_RETREAT_ENEMY_DIST ) {
			continue;
		}

		// Add this guy's area to the list
		dangerSpots[*dangerSpotCount] = BotGetArea( i );

		// Record that we have one more guy
		( *dangerSpotCount )++;

	}

}
//
// BotFindEnemies
//


float BotWeaponRange( bot_state_t *bs, int weaponnum ) {
	switch ( weaponnum ) {
		// dont use unless manually forced
	case WP_PLIERS:
	case WP_MEDKIT:
	case WP_AMMO:
	case WP_MEDIC_SYRINGE:
	case WP_LANDMINE:
	case WP_SATCHEL:
	case WP_SATCHEL_DET:
	case WP_TRIPMINE:
	case WP_MEDIC_ADRENALINE:
	case WP_DYNAMITE:
	case WP_ARTY:
		return -2.0f;

		// short range
	case WP_KNIFE:
		return 256.0f;

	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
		return 512.0f;

	case WP_LUGER:
	case WP_COLT:

	case WP_AKIMBO_COLT:
	case WP_AKIMBO_LUGER:
	case WP_SILENCER:

	case WP_SILENCED_COLT:

	case WP_FLAMETHROWER:
	case WP_SMOKE_MARKER:
	case WP_SMOKE_BOMB:

	case WP_AKIMBO_SILENCEDCOLT:
	case WP_AKIMBO_SILENCEDLUGER:

		return 1000.f;

		// low-mid range
	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:

	case WP_GPG40:
	case WP_M7:

		return 2000.f;

		// mid range
	case WP_KAR98:
	case WP_CARBINE:
	case WP_GARAND:

	case WP_MOBILE_MG42:
	case WP_K43:
	case WP_FG42:

		return 3000.f;

		// long range
	case WP_PANZERFAUST:

		return 7000.f;

	case WP_MORTAR:
	case WP_MORTAR_SET:

		return 4500.f;

	case WP_BINOCULARS:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:

		return 6500.f;

	default:
		return 999999.0;
	}
}

qboolean BotHasWeaponWithRange( bot_state_t *bs, float dist ) {
	int i, *ammo;

	ammo = bs->cur_ps.ammo;

	// if we are mounted on an MG42, always return true
	if ( g_entities[bs->client].s.eFlags & EF_MG42_ACTIVE ) {
		return qtrue;
	}

	for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
		if ( COM_BitCheck( bs->cur_ps.weapons, i ) ) {
			if ( !BotGotEnoughAmmoForWeapon( bs, i ) ) {
				continue;
			}
			if ( BotWeaponRange( bs, i ) < dist ) {
				continue;
			}
			// found one
			return qtrue;
		}
	}

	return qfalse;
}

/*
==================
BotSortClientsByDistance
==================
*/
void BotSortClientsByDistance( vec3_t srcpos, int *sorted, qboolean hasPanzer ) {
	int i, j, best = 0;
	float distances[MAX_CLIENTS];
	int indexes[MAX_CLIENTS];
	float closest;

	memset( distances, 0, sizeof( distances ) );
	memset( indexes, 0, sizeof( indexes ) );
	//
	// build the distances
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		int k = level.sortedClients[i];


		distances[i] = VectorDistanceSquared( srcpos, level.clients[k].ps.origin );
		if ( hasPanzer && level.clients[k].ps.eFlags & EF_MG42_ACTIVE ) {
			distances[i] /= ( 3 * 3 );
		}
		indexes[i] = k;
	}

	//
	// now build the output list
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		closest = -1;
		for ( j = 0; j < level.numConnectedClients; j++ ) {
			if ( indexes[j] < 0 ) {
				continue;
			}

			if ( closest < 0 || distances[j] < closest ) {
				best = j;
				closest = distances[j];
			}
		}

		sorted[i] = indexes[best];
		indexes[best] = -1;
	}
}

/*
==================
BotFindEnemyMP

Find a new target for bots in multiplayer
==================
*/
int BotFindEnemyMP( bot_state_t *bs, int curenemy, qboolean ignoreViewRestrictions ) {
	int i, healthdecrease, j;
	float fov, dist, curdist, alertness, easyfragger, vis;
	aas_entityinfo_t entinfo, curenemyinfo;
	vec3_t dir, ang;
	int heardShooting, heardFootSteps;
	int distanceSorted[MAX_CLIENTS];
	int startTime = 0;
	if ( bot_profile.integer == 1 ) {
		startTime = trap_Milliseconds();
	}

	if ( bs->last_findenemy == level.time ) {
		if ( bs->last_findenemy_enemy >= 0 || curenemy >= 0 ) {
			bs->enemy = bs->last_findenemy_enemy;
		}

		if ( bot_profile.integer == 1 ) {
			botTime_FindEnemy += trap_Milliseconds() - startTime;
		}
		return ( bs->last_findenemy_enemy > -1 );
	}
	bs->last_findenemy = level.time;

	alertness = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_ALERTNESS, 0, 1 );
	easyfragger = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_EASY_FRAGGER, 0, 1 );
	//check if the health decreased
	healthdecrease = bs->lasthealth > bs->inventory[INVENTORY_HEALTH];
	//remember the current health value
	bs->lasthealth = bs->inventory[INVENTORY_HEALTH];
	//
	if ( curenemy >= 0 ) {
		BotEntityInfo( curenemy, &curenemyinfo );
		if ( EntityIsDead( &curenemyinfo ) ) {
			bs->enemy = -1;
			curenemy = -1;
			curdist = 0;
		} else {
			VectorSubtract( curenemyinfo.origin, bs->origin, dir );
			curdist = VectorLength( dir );
		}
	} else {
		curdist = 0;
	}

	// See if we're inheriting a target from our leader
//	if (BotFindLeadersEnemy( bs, curenemy, &entinfo))
//		return qtrue;

	BotSortClientsByDistance( bs->origin, distanceSorted, COM_BitCheck( bs->cur_ps.weapons, WP_PANZERFAUST ) );

	//
	heardShooting = qfalse;
	heardFootSteps = qfalse;
	for ( i = 0; i < level.numConnectedClients; i++ ) {
//		j = level.sortedClients[i];
		j = distanceSorted[i];

		if ( j == bs->client ) {
			continue;
		}

		//if it's the current enemy
		if ( j == curenemy ) {
			continue;
		}

		BotEntityInfo( j, &entinfo );
		if ( !entinfo.valid ) {
			continue;
		}

		// See if the enemy has a notarget on it
		if ( g_entities[j].flags & FL_NOTARGET ) {
			continue;
		}

		//if on the same team
		if ( BotSameTeam( bs, j ) ) {
			continue;
		}

		//if the enemy is dead, Gordon: and not in limbo, still want to kill em totally
		if ( EntityIsDead( &entinfo ) && !EntityInLimbo( &entinfo ) ) {
			continue;
		}

		//if the enemy is invisible and not shooting
		//if(EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
		//	continue;
		//}

		// TAT 10/10/2002
		//		If it's disguised, don't target them
		if ( g_entities[j].client->ps.powerups[PW_OPS_DISGUISED] ) {
			continue;
		}

		//if not an easy fragger don't shoot at chatting players
		if ( easyfragger < 0.5 && EntityIsChatting( &entinfo ) ) {
			continue;
		}

		// this is a complete check, which takes into account all forms of view restriction
		if ( !ignoreViewRestrictions && !BotEntityWithinView( bs, j ) ) {
			continue;
		}

		//
		//if (lastteleport_time > trap_AAS_Time() - 3) {
		//	VectorSubtract(entinfo.origin, lastteleport_origin, dir);
		//	if (VectorLength(dir) < 70) continue;
		//}

		//calculate the distance towards the enemy
		VectorSubtract( entinfo.origin, bs->origin, dir );
		dist = VectorLength( dir );
		vectoangles( dir, ang );

		// Gordon: if using panzerfaust, attack guys on mg42s more readily
		if ( COM_BitCheck( bs->cur_ps.weapons, WP_PANZERFAUST ) && ( g_entities[j].client->ps.eFlags & EF_MG42_ACTIVE ) ) {
			dist /= 3;
		}

		// If this is enemy is attempting to disarm dynamite/build, they take preference!
		if ( g_entities[j].client->ps.weapon == WP_PLIERS && g_entities[j].client->touchingTOI ) {
			dist /= 3;
		}

		//if this enemy is further away than the current one
		if ( curenemy >= 0 && dist > curdist ) {
			continue;
		}

		//if the bot has no
		// RF, disabled, doesnt work well with snipers
		//if (dist > 900 + alertness * 4000) continue;

		// weapons have range limit
		// if we cant get to them, or we dont want to chase them, ignore them
		if ( ( !BotWantsToChase( bs ) || !trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, BotGetArea( j ), bs->tfl ) )
			 && !BotHasWeaponWithRange( bs, dist ) ) {
			continue;
		}

		// if the bot's health decreased or the enemy is shooting
		if ( curenemy < 0 && ( healthdecrease || EntityIsShooting( &entinfo ) ) ) {
			fov = 360;
		} else {
			fov = 90 + 270 - ( ( dist > 810 ? 810 : dist ) / 3 );
		}

		// RF, smaller fov with sniperrifle
		if ( BG_IsScopedWeapon( bs->weaponnum ) ) {
			fov = 10;
		}

		VectorSubtract( bs->origin, entinfo.origin, dir );
/*		//if the enemy is quite far away, not shooting and the bot is not damaged
		if (curenemy < 0 && dist > 200 && !healthdecrease && !EntityIsShooting(&entinfo))
		{
			//check if we can avoid this enemy
			vectoangles(dir, angles);
			//if the bot isn't in the fov of the enemy
			if (!InFieldOfVision(g_entities[j].client->ps.viewangles, 120, angles)) {
				//update some stuff for this enemy
				BotUpdateBattleInventory(bs, j);
				//if the bot doesn't really want to fight
				if (BotWantsToRetreat(bs))
					continue;
			}
		}
*/
		// Gordon: with our aggresive botclipping this is a LOT of places, so still attack them,
		// NOTE: perhaps change this to check if not valid area, then see if we can hit with long range weapon, if not, THEN ignore
		// if we are not mounted on an MG42, or sniping, then ignore clients that are not in a valid area
/*		if (!(bs->cur_ps.eFlags & EF_MG42_ACTIVE) && !(bs->flags & BFL_SNIPING))
		{
			// if they are not in a valid area
			if (!BotGetArea( j ))
				continue;
		} */

		// check if the enemy visibility.  This does a trace to make sure you're actually visible
		vis = BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, fov, j, NULL );

		// If we're not visible, and we're not heard, skip out
		if ( ( vis <= 0 ) && !heardShooting && !heardFootSteps ) {
			continue;
		}

		// found an enemy
		bs->enemy = entinfo.number;
		if ( curenemy >= 0 || bs->last_fire > level.time - 2000 || bs->last_enemysight > level.time - 2000 ) {
			bs->enemysight_time = trap_AAS_Time() - 2;
		} else {
			bs->enemysight_time = trap_AAS_Time();
		}
		//		bs->enemysuicide = qfalse;
		bs->enemydeath_time = 0;

/*		if (!bs->last_enemysight || (bs->last_enemysight < level.time - 10000))
		{
			Bot_ScriptEvent( bs->client, "enemysight", !(g_entities[bs->enemy].r.svFlags & SVF_BOT) ? g_entities[bs->enemy].client->pers.netname : g_entities[bs->enemy].scriptName );
		}*/

		if ( bot_profile.integer == 1 ) {
			botTime_FindEnemy += trap_Milliseconds() - startTime;
		}

		bs->last_findenemy_enemy = bs->enemy;
		bs->last_enemysight = level.time;
		return qtrue;
	}

	if ( bot_profile.integer == 1 ) {
		botTime_FindEnemy += trap_Milliseconds() - startTime;
	}

	bs->last_findenemy_enemy = -1;
	return qfalse;

} // int BotFindEnemyMP(bot_state_t *bs, int curenemy) ...

// Reaction time for this bot
float BotGetReactionTime( bot_state_t *bs ) {
	// Just use the scripted value
	return bs->attribs[BOT_REACTION_TIME]; // * trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1);
}

float swayrand( float x, float y ) {
	return sin( level.time / 1000.0 * x * M_PI * 2 ) * cos( level.time / 1000.0 * y * M_PI * 2 );
}

/*
==================
BotAimAtEnemy
==================
*/
void BotAimAtEnemy( bot_state_t *bs ) {
	int /*i, */ enemyvisible;
	float dist, f, aim_skill, aim_accuracy, speed, reactiontime;
	vec3_t dir, bestorigin, end, start, groundtarget, cmdmove, enemyvelocity;
	vec3_t mins = {-4,-4,-4}, maxs = {4, 4, 4};
	weaponinfo_t wi;
	aas_entityinfo_t entinfo;
	bot_goal_t goal;
	bsp_trace_t trace;
	vec3_t target;

	//if the bot has no enemy
	if ( bs->enemy < 0 ) {
		return;
	}

	//BotAI_Print(PRT_MESSAGE, "client %d: aiming at client %d\n", bs->entitynum, bs->enemy);
	aim_skill =     trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1 );
	aim_accuracy =  trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1 );

	if ( aim_skill > 0.95 ) {
		//don't aim too early
		reactiontime = 0.5 * trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1 );
		if ( bs->enemysight_time > trap_AAS_Time() - reactiontime ) {
			return;
		}
		if ( bs->teleport_time > trap_AAS_Time() - reactiontime ) {
			return;
		}
	}

	//get the weapon information
	trap_BotGetWeaponInfo( bs->ws, bs->weaponnum, &wi );
	// if we have mounted an mg42
	if ( g_entities[bs->client].s.eFlags & EF_MG42_ACTIVE ) {
		// Gordon: lowering all of these from 0.8 per atvi request
		aim_accuracy = 0.65;
		aim_skill = 0.65;
	} else {
		//get the weapon specific aim accuracy and or aim skill
		if ( bs->weaponnum == WP_GRENADE_LAUNCHER || bs->weaponnum == WP_GRENADE_PINEAPPLE ) {
			aim_accuracy =  trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_GRENADELAUNCHER, 0, 1 );
			aim_skill =     trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_GRENADELAUNCHER, 0, 1 );
		}
		if ( bs->weaponnum == WP_PANZERFAUST || bs->weaponnum == WP_MORTAR_SET ) {
			aim_accuracy =  trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_ROCKETLAUNCHER, 0, 1 );
			aim_skill =     trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_ROCKETLAUNCHER, 0, 1 );
		}
		if ( bs->weaponnum == WP_FLAMETHROWER ) {
			aim_accuracy =  trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_FLAMETHROWER, 0, 1 );
		}
		if ( bs->weaponnum == WP_MOBILE_MG42 && bs->cur_ps.eFlags & EF_PRONE ) {
			aim_accuracy =  trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_FLAMETHROWER, 0, 1 );
		}
	}
	//
	if ( aim_accuracy <= 0 ) {
		aim_accuracy = 0.0001;
	}
	//get the enemy entity information
	BotEntityInfo( bs->enemy, &entinfo );
	//if the enemy is invisible then shoot crappy most of the time
	if ( EntityIsInvisible( &entinfo ) ) {
		if ( random() > 0.1 ) {
			aim_accuracy *= 0.4;
		}
	}
	//
	VectorSubtract( entinfo.origin, entinfo.lastvisorigin, enemyvelocity );
	VectorScale( enemyvelocity, 1 / entinfo.update_time, enemyvelocity );
	//enemy origin and velocity is remembered every 0.5 seconds
	if ( bs->enemyposition_time < trap_AAS_Time() ) {
		//
		bs->enemyposition_time = trap_AAS_Time() + 0.5;
		VectorCopy( enemyvelocity, bs->enemyvelocity );
		VectorCopy( entinfo.origin, bs->enemyorigin );
	}
	// aiming gets better with time
	f = trap_AAS_Time() - bs->enemysight_time;
	if ( f > 2.0 ) {
		f = 2.0;
	}
	aim_accuracy += 0.2 * f / 2.0;
	if ( aim_accuracy > 1.0 ) {
		aim_accuracy = 1.0;
	}
	// better if enemy is not moving
	f = VectorLength( bs->enemyvelocity );
	if ( f > 200 ) {
		f = 200;
	}
	aim_accuracy += 0.2 * ( 0.5 - ( f / 200.0 ) );
	//if not extremely skilled
	if ( aim_skill < 0.96 ) {
		VectorSubtract( entinfo.origin, bs->enemyorigin, dir );
		//if the enemy moved a bit
		if ( VectorLengthSquared( dir ) > SQR( 48 ) ) {
			//if the enemy changed direction
			if ( DotProduct( bs->enemyvelocity, enemyvelocity ) < 0 ) {
				//aim accuracy should be worse now
				aim_accuracy *= 0.45;
			}
		}
	}
	if ( aim_accuracy > 1.0 ) {
		aim_accuracy = 1.0;
	}

	//check visibility of enemy
	enemyvisible = BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL );
	//if the enemy is visible
	if ( enemyvisible ) {
		//
		VectorCopy( entinfo.origin, bestorigin );
		bestorigin[2] += g_entities[bs->enemy].client->ps.viewheight - 6;
		//get the start point shooting from
		//NOTE: the x and y projectile start offsets are ignored
		VectorCopy( bs->origin, start );
		start[2] += bs->cur_ps.viewheight;
		//start[2] += wi.offset[2];
		//
		BotAI_Trace( &trace, start, mins, maxs, bestorigin, bs->entitynum, MASK_SHOT );
		//if the enemy is NOT hit
		if ( trace.fraction <= 1 && trace.ent != entinfo.number ) {
			bestorigin[2] = g_entities[bs->enemy].r.absmax[2] - 2;
		}
		//if it is not an instant hit weapon the bot might want to predict the enemy
		if ( wi.speed ) {
			//
			VectorSubtract( bestorigin, bs->origin, dir );
			dist = VectorLength( dir );
			VectorSubtract( entinfo.origin, bs->enemyorigin, dir );
			//if the enemy is NOT pretty far away and strafing just small steps left and right
			if ( !( dist > 100 && VectorLengthSquared( dir ) < SQR( 32 ) ) ) {
				//if skilled anough do exact prediction
				if ( aim_skill > 0.8 &&
					 //if the weapon is ready to fire
					 bs->cur_ps.weaponstate == WEAPON_READY ) {
					aas_clientmove_t move;
					vec3_t origin;

					VectorSubtract( entinfo.origin, bs->origin, dir );
					//distance towards the enemy
					dist = VectorLength( dir );
					//direction the enemy is moving in
					VectorSubtract( entinfo.origin, entinfo.lastvisorigin, dir );
					//
					VectorScale( dir, 1 / entinfo.update_time, dir );
					//
					VectorCopy( entinfo.origin, origin );
					origin[2] += 1;
					//
					VectorClear( cmdmove );
					//AAS_ClearShownDebugLines();
					trap_AAS_PredictClientMovement( &move, bs->enemy, origin,
													PRESENCE_CROUCH, qfalse,
													dir, cmdmove, 0,
													dist * 10 / wi.speed, 0.1, 0, 0, qfalse );
					VectorCopy( move.endpos, bestorigin );
					//BotAI_Print(PRT_MESSAGE, "%1.1f predicted speed = %f, frames = %f\n", trap_AAS_Time(), VectorLength(dir), dist * 10 / wi.speed);
				}
				//if not that skilled do linear prediction
				else if ( aim_skill > 0.4 ) {
					VectorSubtract( entinfo.origin, bs->origin, dir );
					//distance towards the enemy
					dist = VectorLength( dir );
					//direction the enemy is moving in
					VectorSubtract( entinfo.origin, entinfo.lastvisorigin, dir );
					dir[2] = 0;
					//
					speed = VectorNormalize( dir ) / entinfo.update_time;
					//botimport.Print(PRT_MESSAGE, "speed = %f, wi->speed = %f\n", speed, wi->speed);
					//best spot to aim at
					VectorMA( entinfo.origin, ( dist / wi.speed ) * speed, dir, bestorigin );
				}
			}

		} // if (wi.speed) ...

		//if the projectile does radial damage
		if ( ( bs->weaponnum != WP_FLAMETHROWER ) && ( wi.proj.damagetype & DAMAGETYPE_RADIAL ) ) {
			//if the enemy isn't standing significantly higher than the bot
			if ( entinfo.origin[2] < bs->origin[2] + 16 ) {
				//try to aim at the ground in front of the enemy
				VectorCopy( entinfo.origin, end );
				end[2] -= 64;
				BotAI_Trace( &trace, bestorigin, NULL, NULL, end, entinfo.number, MASK_SHOT );
				//
				VectorCopy( bestorigin, groundtarget );
				if ( trace.startsolid ) {
					groundtarget[2] = entinfo.origin[2] - 16;
				} else { groundtarget[2] = trace.endpos[2];}
				//trace a line from projectile start to ground target
				BotAI_Trace( &trace, start, NULL, NULL, groundtarget, bs->entitynum, MASK_SHOT );
				//if hitpoint is not vertically too far from the ground target
				if ( fabs( trace.endpos[2] - groundtarget[2] ) < 50 ) {
					VectorSubtract( trace.endpos, groundtarget, dir );
					//if the hitpoint is near anough the ground target
					if ( VectorLengthSquared( dir ) < SQR( 100 ) ) {
						VectorSubtract( trace.endpos, start, dir );
						//if the hitpoint is far anough from the bot
						if ( VectorLengthSquared( dir ) > SQR( 100 ) ) {
							//check if the bot is visible from the ground target
							trace.endpos[2] += 1;
							BotAI_Trace( &trace, trace.endpos, NULL, NULL, entinfo.origin, entinfo.number, MASK_SHOT );
							if ( trace.fraction >= .99f ) {
								//botimport.Print(PRT_MESSAGE, "%1.1f aiming at ground\n", AAS_Time());
								VectorCopy( groundtarget, bestorigin );
							}
						}
					}
				}
			}
		}
		if ( BotScopedWeapon( bs->cur_ps.weapon ) ) {
			bestorigin[0] += 20 * swayrand( 0.47f, 0.53f ) * ( 1 - aim_accuracy );
			bestorigin[1] += 20 * swayrand( 0.44f, 0.57f ) * ( 1 - aim_accuracy );
			bestorigin[2] += 10 * swayrand( 0.52f, 0.49f ) * ( 1 - aim_accuracy );
		}
	} else {
		//
		VectorCopy( bs->lastenemyorigin, bestorigin );
		bestorigin[2] += 8;
		//if the bot is skilled anough
		if ( aim_skill > 0.5 ) {
			//do prediction shots around corners
//			if (wi.number == WP_BFG ||	//----(SA)	removing old weapon references
			if ( wi.number == WP_PANZERFAUST ||
				 wi.number == WP_GRENADE_LAUNCHER ||
				 wi.number == WP_GRENADE_PINEAPPLE ) {
				//create the chase goal
				goal.entitynum = bs->client;
				goal.areanum = bs->areanum;
				VectorCopy( bs->eye, goal.origin );
				VectorSet( goal.mins, -8, -8, -8 );
				VectorSet( goal.maxs, 8, 8, 8 );
				//
				if ( trap_BotPredictVisiblePosition( bs->lastenemyorigin, bs->lastenemyareanum, &goal, bs->tfl, target ) ) {
					VectorCopy( target, bestorigin );
					bestorigin[2] -= 20;
				}
				aim_accuracy = 1;
			}
		}
	}
	//
	if ( enemyvisible ) {
		BotAI_Trace( &trace, bs->eye, NULL, NULL, bestorigin, bs->entitynum, MASK_SHOT );
		VectorCopy( trace.endpos, bs->aimtarget );
	} else {
		VectorCopy( bestorigin, bs->aimtarget );
	}
	//get aim direction
	VectorSubtract( bestorigin, bs->eye, dir );
	//
	//distance towards the enemy
	dist = VectorLength( dir );
	if ( dist > 150 ) {
		dist = 150;
	}
	f = 0.6 + dist / 150 * 0.4;
	aim_accuracy *= f;

	//
	//add some random stuff to the aim direction depending on the aim accuracy
	if ( aim_accuracy < 0.8 ) {
		VectorNormalize( dir );
		dir[0] += 0.3 * swayrand( 0.58f, 0.37f ) * ( 1 - aim_accuracy );
		dir[1] += 0.3 * swayrand( 0.54f, 0.47f ) * ( 1 - aim_accuracy );
		dir[2] += 0.3 * swayrand( 0.61f, 0.38f ) * ( 1 - aim_accuracy );
	}

	//set the ideal view angles
	vectoangles( dir, bs->ideal_viewangles );

	//take the weapon spread into account for lower skilled bots
	bs->ideal_viewangles[PITCH] += 6 * wi.vspread * swayrand( 0.36f, 0.45f ) * ( 1 - aim_accuracy );
	bs->ideal_viewangles[PITCH] = AngleMod( bs->ideal_viewangles[PITCH] );
	bs->ideal_viewangles[YAW] += 6 * wi.hspread * swayrand( 0.654f, 0.54f ) * ( 1 - aim_accuracy );
	bs->ideal_viewangles[YAW] = AngleMod( bs->ideal_viewangles[YAW] );

	// adjust for sway
	if ( BotScopedWeapon( bs->weaponnum ) ) {
		float spreadfrac, phase;
		vec3_t swayang;
		gentity_t *ent = BotGetEntity( bs->client );
		int i;

		spreadfrac = ent->client->currentAimSpreadScale;

		// rotate 'forward' vector by the sway
		phase = level.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_PI * 2;
		swayang[PITCH] = ZOOM_PITCH_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_PITCH_MIN_AMPLITUDE );

		phase = level.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_PI * 2;
		swayang[YAW] = ZOOM_YAW_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_YAW_MIN_AMPLITUDE );

		swayang[ROLL] = 0;

		// adjust sway-correction with aim_skill
		aim_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_SNIPERRIFLE, 0, 1 );
		VectorMA( bs->ideal_viewangles, -aim_skill, swayang, bs->ideal_viewangles );
		for ( i = 0; i < 3; i++ ) bs->ideal_viewangles[i] = AngleNormalize360( bs->ideal_viewangles[i] );
	} else if ( !( bs->cur_ps.eFlags & EF_MG42_ACTIVE ) && ( bs->weaponnum == WP_MOBILE_MG42 ) )         {
		// aim downward slightly to adjust for kick
		bs->ideal_viewangles[PITCH] += 4;
	}

	// if smoke grenade, aim a bit higher
	if ( bs->cur_ps.weapon == WP_SMOKE_MARKER ) {
		if ( bs->ideal_viewangles[PITCH] > -60 ) {
			bs->ideal_viewangles[PITCH] = -60;
		}
	}

	//if the bot is really accurate and has the enemy in view for some time
	if ( !( bs->cur_ps.eFlags & EF_MG42_ACTIVE ) && aim_accuracy > 0.95 && bs->enemysight_time < trap_AAS_Time() - 2 ) {

		//set the view angles directly
		if ( bs->ideal_viewangles[PITCH] > 180 ) {
			bs->ideal_viewangles[PITCH] -= 360;
		}
		if ( AngleNormalize180( bs->ideal_viewangles[YAW] - bs->viewangles[YAW] ) < 25 ) {
			VectorCopy( bs->ideal_viewangles, bs->viewangles );
			trap_EA_View( bs->client, bs->viewangles );
		}
	}
}



/*
==================
BotAimAtEnemySP
==================
*/
void BotAimAtEnemySP( bot_state_t *bs ) {
}


/*
==================
BotMoveWhileFiring
==================
*/
qboolean BotMoveWhileFiring( int weapon ) {
	switch ( weapon ) {
	case WP_BINOCULARS:
	case WP_PANZERFAUST:
	case WP_MOBILE_MG42:
	case WP_MORTAR:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		return qfalse;
	}
	//
	return qtrue;
}

/*
==================
BotThrottleWeapon
==================
*/
qboolean BotThrottleWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_PANZERFAUST:
	case WP_FLAMETHROWER:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		return qfalse;
	}
	//
	return qtrue;
}

/*
===================
BotScopedWeapon
===================
*/
qboolean BotScopedWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_BINOCULARS:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		return qtrue;
	}
	//
	return qfalse;
}

/*
==================
BotCheckAttack

  returns qfalse if the enemy is unreachable. qtrue if it is possible to attack the enemy (this doesnt always mean
  the trigger is pulled).
==================
*/
qboolean BotCheckAttack( bot_state_t *bs ) {
	float reactiontime, fov, firethrottle, dist, aimskill = 0.0;
	bsp_trace_t bsptrace;
	//float selfpreservation;
	vec3_t forward, right, start, end, dir, angles;
	weaponinfo_t wi;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vec3_t mins = {-12, -12, -12}, maxs = {12, 12, 12};
	int i, fcnt, ecnt;
	int mask;

	if ( bs->enemy < 0 ) {
		return qfalse;
	}

	// Gordon: limit teh fire rate of the bots on these weapons
	if ( ( bs->weaponnum == WP_GARAND_SCOPE || bs->weaponnum == WP_K43_SCOPE ) && ( level.time - bs->last_fire ) < 800 ) {
		// pause for a bit
		return qtrue;
	}

	{
		float range = BotWeaponRange( bs, bs->weaponnum );
		if ( SQR( range ) < VectorDistanceSquared( bs->origin, BotGetOrigin( bs->enemy ) ) ) {
			// cant reach them
			return qfalse;
		}
	}

	//
	// if we have recently called this routine, then repeat the last result
	if ( ( bs->last_botcheckattack_weapon == bs->weaponnum ) && ( bs->enemy == bs->last_botcheckattack_enemy ) && ( bs->last_botcheckattack > level.time - 250 ) ) {
		// if our last call passed, then let it through
		if ( bs->last_botcheckattack <= bs->last_fire ) {
			goto passed;
		}
	}

	bs->last_botcheckattack = level.time + rand() % 80;
	bs->last_botcheckattack_weapon = bs->weaponnum;
	bs->last_botcheckattack_enemy = bs->enemy;

	reactiontime = BotGetReactionTime( bs );      //trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1);
	aimskill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1 );

	if ( bs->enemysight_time > trap_AAS_Time() - reactiontime ) {
		// wait until we have had time to react
		return qtrue;
	}

	if ( bs->teleport_time > trap_AAS_Time() - reactiontime ) {
		return qtrue;
	}

	if ( bs->weaponchange_time > trap_AAS_Time() - 0.1 ) {
		return qtrue;
	}

	if ( BotThrottleWeapon( bs->weaponnum ) ) {
		//check fire throttle characteristic
		if ( bs->firethrottlewait_time > trap_AAS_Time() ) {
			return qtrue;
		}
		firethrottle = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_FIRETHROTTLE, 0, 1 );
		if ( bs->firethrottleshoot_time < trap_AAS_Time() ) {
			if ( random() > firethrottle ) {
				bs->firethrottlewait_time = trap_AAS_Time() + firethrottle;
				bs->firethrottleshoot_time = 0;
			} else {
				bs->firethrottleshoot_time = trap_AAS_Time() + 1 - firethrottle;
				bs->firethrottlewait_time = 0;
			}
		}
	}
	// always pass if grenade is live
	if ( ( bs->cur_ps.weapon == WP_GRENADE_LAUNCHER || bs->cur_ps.weapon == WP_GRENADE_PINEAPPLE ) && bs->cur_ps.grenadeTimeLeft ) {
		goto passed;
	}
	//
	// always pass if smoke grenade
	if ( bs->cur_ps.weapon == WP_SMOKE_MARKER ) {
		goto passed;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	VectorSubtract( entinfo.origin, bs->eye, dir );
	//
	if ( bs->weaponnum == WP_KNIFE ) {
		if ( VectorLengthSquared( dir ) > SQR( 256 ) ) {
			return qfalse;  // too far away
		}
	}
	//
	if ( VectorLengthSquared( dir ) < SQR( 100 ) ) {
		fov = 120;
	} else {
		fov = 50;
	}

	vectoangles( dir, angles );

	if ( !InFieldOfVision( bs->viewangles, fov, angles ) ) {
		return qtrue;
	}

	// set mask based on weapon
	mask = MASK_SHOT;
	switch ( bs->cur_ps.weapon ) {
	case WP_PANZERFAUST:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		mask = MASK_MISSILESHOT;
	}

	BotAI_Trace( &bsptrace, bs->eye, NULL, NULL, bs->aimtarget, bs->client, mask );
	if ( !( bs->cur_ps.weapon == WP_GRENADE_LAUNCHER || bs->cur_ps.weapon == WP_GRENADE_PINEAPPLE ) ) {
		if ( bsptrace.fraction < .99f && bsptrace.ent != bs->enemy ) {
			return qfalse;
		}
	}

	//get the weapon info
	trap_BotGetWeaponInfo( bs->ws, bs->cur_ps.weapon, &wi );
	//get the start point shooting from
	VectorCopy( bs->origin, start );
	start[2] += bs->cur_ps.viewheight;
	AngleVectors( bs->viewangles, forward, right, NULL );
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	//end point aiming at
	VectorMA( start, BotWeaponRange( bs, bs->weaponnum ), forward, end );
	//a little back to make sure not inside a very close enemy
	VectorMA( start, -12, forward, start );
	BotAI_Trace( &trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT );  //----(SA) should this maybe check the weapon type and adjust the clipflag?  it seems like this is probably fine as-is, but I thought I'd note it.
	//if won't hit the enemy
	if ( trace.ent != bs->enemy ) {
		//if the entity is a client
		if ( trace.ent > 0 && trace.ent < MAX_CLIENTS ) {
			//if a teammate is hit
			if ( BotSameTeam( bs, trace.ent ) ) {
				return qfalse;
			}
		}
	}
	//if the projectile does a radial damage
	if ( wi.proj.damagetype & DAMAGETYPE_RADIAL ) {
		// if they are carrying the flag, and are close to the destination, fire away
		if ( BotCarryingFlag( bs->enemy ) ) {
			gentity_t *trav;
			trav = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
			if ( !trav ) {
				trav = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
			}
			if ( trav ) {
				VectorAdd( trav->r.absmin, trav->r.absmax, end );
				VectorScale( end, 0.5, end );
				if ( VectorDistanceSquared( g_entities[bs->enemy].r.currentOrigin, end ) < SQR( 800 ) ) {
					goto passed;
				}
			}
		}

		//check if a teammate gets radial damage
		fcnt = 0;
		ecnt = 0;
		for ( i = 0; i < level.maxclients; i++ ) {
			if ( !g_entities[i].inuse ) {
				continue;
			}
			if ( i == bs->client ) {
				continue;
			}
			if ( g_entities[i].client->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( g_entities[i].client->sess.sessionTeam != TEAM_AXIS && g_entities[i].client->sess.sessionTeam != TEAM_ALLIES ) {
				continue;
			}
			if ( !CanDamage( BotGetEntity( i ), end ) && !CanDamage( BotGetEntity( i ), bs->origin ) ) {
				continue;
			}
			// if they are within range
			if ( ( dist = VectorDistance( g_entities[i].r.currentOrigin, trace.endpos ) ) < wi.proj.radius ) {
				//points = (wi.proj.damage * (dist / wi.proj.radius));
				//if (points > 10) {
				if ( BotSameTeam( bs, i ) ) {
					fcnt++;
				} else {
					ecnt++;
				}
				//}
				// check distance from source
			} else if ( ( dist = 4.0 * VectorDistance( g_entities[i].r.currentOrigin, bs->origin ) ) < wi.proj.radius ) {
				//dist /= 4.0;
				//points = (wi.proj.damage * (dist / wi.proj.radius));
				//if (points > 10) {
				if ( BotSameTeam( bs, i ) ) {
					fcnt++;
				} else {
					ecnt++;
				}
				//}
			}
		}
		//
		if ( fcnt >= ecnt ) {
			return qfalse;  // too dangerous
		}
	}

passed:

	//if fire has to be release to activate weapon
	if ( bs->cur_ps.weapon == WP_GRENADE_LAUNCHER || bs->cur_ps.weapon == WP_GRENADE_PINEAPPLE ) {
		if ( bs->cur_ps.grenadeTimeLeft &&
			 (   ( bs->inventory[INVENTORY_HEALTH] < 15 ) ||
				 ( bs->cur_ps.grenadeTimeLeft < ( 700 + (int)( 4000.0 * ( 1.0 - aimskill ) ) ) ) ) ) {
			// release grenade
		} else {
			// hold onto it
			trap_EA_Attack( bs->client );
		}
	} else {
		trap_EA_Attack( bs->client );
	}
	bs->flags ^= BFL_ATTACKED;
	bs->last_fire = level.time;
	//
	// we have attacked them
	g_entities[bs->enemy].botLastAttackedTime = level.time;
	g_entities[bs->enemy].botLastAttackedEnt = bs->client;
	// if this is k43/garand, then add a delay since recoil usually makes it tough
	switch ( bs->weaponnum ) {
	case WP_K43:
	case WP_GARAND:
	case WP_K43_SCOPE:
	case WP_GARAND_SCOPE:
		bs->teleport_time = trap_AAS_Time() + 0.5 + random() * 0.4;
		break;
	}
	//
	return qtrue;
}

/*
==================
BotMapScripts
==================
*/
void BotMapScripts( bot_state_t *bs ) {
	return;
}

/*
==================
BotCheckButtons
==================
*/
/*
void CheckButtons(void)
{
	int modelindex, i, numbuttons = 0;
	char *classname, *model;
	float lip, health, dist;
	bsp_entity_t *ent;
	vec3_t mins, maxs, size, origin, angles, movedir, goalorigin;
	vec3_t start, end, bboxmins, bboxmaxs;
	aas_trace_t trace;

	for (ent = entities; ent; ent = ent->next)
	{
		classname = AAS_ValueForBSPEpairKey(ent, "classname");
		if (!strcmp(classname, "func_button"))
		{
			//create a bot goal towards the button
			model = AAS_ValueForBSPEpairKey(ent, "model");
			modelindex = AAS_IndexFromModel(model);
			//if the model is not loaded
			if (!modelindex) modelindex = atoi(model+1);
			VectorClear(angles);
			AAS_BSPModelMinsMaxsOrigin(modelindex - 1, angles, mins, maxs, NULL);
			//get the lip of the button
			lip = AAS_FloatForBSPEpairKey(ent, "lip");
			if (!lip) lip = 4;
			//get the move direction from the angle
			VectorSet(angles, 0, AAS_FloatForBSPEpairKey(ent, "angle"), 0);
			AAS_SetMovedir(angles, movedir);
			//button size
			VectorSubtract(maxs, mins, size);
			//button origin
			VectorAdd(mins, maxs, origin);
			VectorScale(origin, 0.5, origin);
			//touch distance of the button
			dist = fabs(movedir[0]) * size[0] + fabs(movedir[1]) * size[1] + fabs(movedir[2]) * size[2];// - lip;
			dist *= 0.5;
			//
			health = AAS_FloatForBSPEpairKey(ent, "health");
			//if the button is shootable
			if (health)
			{
				//calculate the goal origin
				VectorMA(origin, -dist, movedir, goalorigin);
				AAS_DrawPermanentCross(goalorigin, 4, LINECOLOR_BLUE);
			} //end if
			else
			{
				//add bounding box size to the dist
				AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bboxmins, bboxmaxs);
				for (i = 0; i < 3; i++)
				{
					if (movedir[i] < 0) dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
					else dist += fabs(movedir[i]) * fabs(bboxmins[i]);
				} //end for
				//calculate the goal origin
				VectorMA(origin, -dist, movedir, goalorigin);
				//
				VectorCopy(goalorigin, start);
				start[2] += 24;
				VectorSet(end, start[0], start[1], start[2] - 100);
				trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
				if (!trace.startsolid)
				{
					VectorCopy(trace.endpos, goalorigin);
				} //end if
				//
				AAS_DrawPermanentCross(goalorigin, 4, LINECOLOR_YELLOW);
				//
				VectorSubtract(mins, origin, mins);
				VectorSubtract(maxs, origin, maxs);
				//
				VectorAdd(mins, origin, start);
				AAS_DrawPermanentCross(start, 4, LINECOLOR_BLUE);
				VectorAdd(maxs, origin, start);
				AAS_DrawPermanentCross(start, 4, LINECOLOR_BLUE);
			} //end else
			if (++numbuttons > 5) return;
		} //end if
	} //end for
} //end of the function CheckButtons
*/

/*
==================
BotEntityToActivate
==================
*/
//#define OBSTACLEDEBUG

int BotEntityToActivate( int entitynum ) {
/*	int i, ent, cur_entities[10];
	char model[MAX_INFO_STRING], tmpmodel[128];
	char target[128], classname[128];
	float health;
	char targetname[10][128];
	aas_entityinfo_t entinfo;*/

	// RF, disabled this until I can figure out a way of getting this all working with the current code
	return 0;

/*	BotEntityInfo(entitynum, &entinfo);
	Com_sprintf(model, sizeof( model ), "*%d", entinfo.modelindex);
	for (ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent)) {
		if (!trap_AAS_ValueForBSPEpairKey(ent, "model", tmpmodel, sizeof(tmpmodel))) continue;
		if (!strcmp(model, tmpmodel)) break;
	}
	if (!ent) {
		BotAI_Print(PRT_ERROR, "BotEntityToActivate: no entity found with model %s\n", model);
		return 0;
	}
	trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname));
	if (!classname) {
		BotAI_Print(PRT_ERROR, "BotEntityToActivate: entity with model %s has no classname\n", model);
		return 0;
	}
	//if it is a door
	if (!strcmp(classname, "func_door")) {
		if (trap_AAS_FloatForBSPEpairKey(ent, "health", &health)) {
			//if health the door must be shot to open
			if (health) return ent;
		}
	}
	//if it is a explosive
	if (!strcmp(classname, "func_explosive")) {
		if (trap_AAS_IntForBSPEpairKey(ent, "spawnflags", &i)) {
			//if DYNOMITE then only dynomite can blow it up
			if (!(i&64)) return ent;
		}
	}
	//get the targetname so we can find an entity with a matching target
	if (!trap_AAS_ValueForBSPEpairKey(ent, "targetname", targetname[0], sizeof(targetname[0]))) {
#ifdef OBSTACLEDEBUG
		BotAI_Print(PRT_ERROR, "BotEntityToActivate: entity with model \"%s\" has no targetname\n", model);
#endif //OBSTACLEDEBUG
		return 0;
	}
	cur_entities[0] = trap_AAS_NextBSPEntity(0);
	for (i = 0; i >= 0 && i < 10;) {
		for (ent = cur_entities[i]; ent; ent = trap_AAS_NextBSPEntity(ent)) {
			if (!trap_AAS_ValueForBSPEpairKey(ent, "target", target, sizeof(target))) continue;
			if (!strcmp(targetname[i], target)) {
				cur_entities[i] = trap_AAS_NextBSPEntity(ent);
				break;
			}
		}
		if (!ent) {
			BotAI_Print(PRT_ERROR, "BotEntityToActivate: no entity with target \"%s\"\n", targetname[i]);
			i--;
			continue;
		}
		if (!trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname))) {
			BotAI_Print(PRT_ERROR, "BotEntityToActivate: entity with target \"%s\" has no classname\n", targetname[i]);
			continue;
		}
		if (!strcmp(classname, "func_button")) {
			//BSP button model
			return ent;
		}
		else if (!strcmp(classname, "trigger_multiple")) {
			//invisible trigger multiple box
			return ent;
		}
		else {
			i--;
		}
	}
	//BotAI_Print(PRT_ERROR, "BotEntityToActivate: unknown activator with classname \"%s\"\n", classname);
	return 0;*/
}

/*
==================
BotSetMovedir
==================
*/
vec3_t VEC_UP           = {0, -1,  0};
vec3_t MOVEDIR_UP       = {0,  0,  1};
vec3_t VEC_DOWN     = {0, -2,  0};
vec3_t MOVEDIR_DOWN = {0,  0, -1};

void BotSetMovedir( vec3_t angles, vec3_t movedir ) {
	if ( VectorCompare( angles, VEC_UP ) ) {
		VectorCopy( MOVEDIR_UP, movedir );
	} else if ( VectorCompare( angles, VEC_DOWN ) )       {
		VectorCopy( MOVEDIR_DOWN, movedir );
	} else {
		AngleVectors( angles, movedir, NULL, NULL );
	}
}

void BotModelMinsMaxs( int modelindex, vec3_t mins, vec3_t maxs ) {
	gentity_t *ent;
	int i;

	ent = BotGetEntity( 0 );
	for ( i = 0; i < level.num_entities; i++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( ent->s.modelindex == modelindex ) {
			VectorCopy( ent->r.mins, mins );
			VectorCopy( ent->r.maxs, maxs );
			return;
		}
	}
	VectorClear( mins );
	VectorClear( maxs );
}

/*
==================
BotAIBlocked
==================
*/
void BotAIBlocked( bot_state_t *bs, bot_moveresult_t *moveresult, int activate ) {
	int movetype, ent, modelindex;
	char classname[128], model[128];
#ifdef OBSTACLEDEBUG
	char buf[128];
#endif
	vec3_t up = {0, 0, 1};
	vec3_t angles, mins, maxs, origin, movedir, hordir, start, end, sideward;
	aas_entityinfo_t entinfo;
	int oldMoverState;

#ifdef OBSTACLEDEBUG
	char netname[MAX_NETNAME];
#endif

	if ( bs->blockentTime > level.time ) {
		// just strafe to avoid other players
		if ( level.time % 3000 < 1500 ) {
			trap_EA_Move( bs->client, vec3_origin, 0 );
			trap_EA_MoveLeft( bs->client );
		} else {
			trap_EA_Move( bs->client, vec3_origin, 0 );
			trap_EA_MoveRight( bs->client );
		}
		return;
	}

	if ( !moveresult->blocked ) {
		/*
		if (bs->blockent >= 0 && (bs->blockentTime >= level.time - 500)) {
			moveresult->blocked = qtrue;
			moveresult->blockentity = bs->blockent;
			//
			bs->blockent = -1;
			bs->blockentTime = 0;
		} else {*/
		return;
		//}
	}
	//
	BotEntityInfo( moveresult->blockentity, &entinfo );
	// RF, dont avoid our leader, just set this as our goal position
	if ( ( entinfo.number < level.maxclients ) && ( bs->blockentTime < level.time ) ) {
		// RF, in multiplayer, only ignore if this is our leader and we are currently following them
		if ( ( ( bs->ainode == AINode_MP_DefendTarget ) && ( bs->target_goal.flags & GFL_LEADER ) && ( bs->leader == entinfo.number ) ) ) {
			// make this our spot
			VectorCopy( bs->origin, bs->target_goal.origin );
			bs->target_goal.areanum = bs->areanum;
			return;
		}
		//
		if ( G_IsSinglePlayerGame() && bs->leader == entinfo.number ) {
			VectorCopy( bs->origin, bs->target_goal.origin );
			bs->target_goal.areanum = bs->areanum;
			return;
		}
		//
		if ( bs->target_goal.flags & GFL_LEADER ) {
			if ( ( BotPointWithinMovementAutonomy( bs, &bs->target_goal, bs->origin ) )
				 && ( VectorDistanceSquared( bs->origin, g_entities[bs->leader].r.currentOrigin ) < SQR( 512 ) ) ) {
				VectorCopy( bs->origin, bs->target_goal.origin );
				bs->target_goal.areanum = bs->areanum;
				return;
			}
		}

		bs->blockentTime = level.time + 400;
		bs->blockent = entinfo.number;
		return;
	}
	//
#ifdef OBSTACLEDEBUG
	ClientName( bs->client, netname, sizeof( netname ) );
	BotAI_Print( PRT_MESSAGE, "%s: I'm blocked by model %d\n", netname, entinfo.modelindex );
#endif /* OBSTACLEDEBUG */
	ent = 0;
	//if blocked by a bsp model and the bot wants to activate it if possible
	if ( entinfo.modelindex > 0 && entinfo.modelindex <= max_bspmodelindex && activate ) {
		//find the bsp entity which should be activated in order to remove
		//the blocking entity
		if ( ( ( Q_stricmp( g_entities[entinfo.number].classname, "func_door" ) == 0 ) || ( Q_stricmp( g_entities[entinfo.number].classname, "func_door_rotating" ) == 0 ) ) ) {
			if ( ( g_entities[entinfo.number].moverState == MOVER_POS1 ) || ( g_entities[entinfo.number].moverState == MOVER_POS1ROTATE ) ) {
				oldMoverState = g_entities[entinfo.number].moverState;
				G_TryDoor( BotGetEntity( entinfo.number ), BotGetEntity( bs->client ), BotGetEntity( bs->client ) );
				if ( g_entities[entinfo.number].moverState != oldMoverState ) {
					// wait for it to finish opening
					return;
				}
			} else if ( ( g_entities[entinfo.number].moverState == MOVER_POS2 ) || ( g_entities[entinfo.number].moverState == MOVER_POS2ROTATE ) ) {
				// it is fully open, try and avoid it
				ent = 0;
			} else {
				// else just wait for it to finish moving
				return;
			}
		} else {
			ent = BotEntityToActivate( entinfo.number );
		}
		if ( !ent ) {
			strcpy( classname, "" );
#ifdef OBSTACLEDEBUG
			BotAI_Print( PRT_MESSAGE, "%s: can't find activator for blocking entity\n", ClientName( bs->client, netname, sizeof( netname ) ) );
#endif //OBSTACLEDEBUG
		} else {
			trap_AAS_ValueForBSPEpairKey( ent, "classname", classname, sizeof( classname ) );
#ifdef OBSTACLEDEBUG
			ClientName( bs->client, netname, sizeof( netname ) );
			BotAI_Print( PRT_MESSAGE, "%s: I should activate %s\n", netname, classname );
#endif /* OBSTACLEDEBUG */
		}
#ifdef OBSTACLEDEBUG
//		ClientName(bs->client, netname, sizeof(netname));
//		BotAI_Print(PRT_MESSAGE, "%s: I've got no brain cells for activating entities\n", netname);
#endif /* OBSTACLEDEBUG */
	   //if it is an explosive we should shoot it
		if ( !strcmp( classname, "func_explosive" ) ) {
			//get the door model
			trap_AAS_ValueForBSPEpairKey( ent, "model", model, sizeof( model ) );
			modelindex = atoi( model + 1 );
			//if the model is not loaded
			if ( !modelindex ) {
				return;
			}
			VectorClear( angles );
			BotModelMinsMaxs( modelindex, mins, maxs );
			//get a goal to shoot at
			VectorAdd( maxs, mins, origin );
			VectorScale( origin, 0.5, origin );
			VectorSubtract( origin, bs->eye, movedir );
			//
			vectoangles( movedir, moveresult->ideal_viewangles );
			moveresult->flags |= MOVERESULT_MOVEMENTVIEW;
			//select a weapon
			BotChooseWeapon( bs );
			trap_EA_SelectWeapon( bs->client, WP_KNIFE );
			//shoot
			trap_EA_Attack( bs->client );
			//
			return;
		} //end if*/
	}
	if ( !strcmp( g_entities[entinfo.number].classname, "script_mover" ) ) {
		if ( entinfo.number == bs->target_goal.entitynum ) {
			// it's our goal, we need to get closer
			return;
		}
		//
		bs->blockentTime = level.time + 400;
		bs->blockent = entinfo.number;
		return;
	}
	//just some basic dynamic obstacle avoidance code
	hordir[0] = moveresult->movedir[0];
	hordir[1] = moveresult->movedir[1];
	hordir[2] = 0;
	//if no direction just take a random direction
	if ( VectorNormalize( hordir ) < 0.1 ) {
		VectorSet( angles, 0, 360 * random(), 0 );
		AngleVectors( angles, hordir, NULL, NULL );
	}
	//
//	if (moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE) movetype = MOVE_JUMP;
//	else
	movetype = MOVE_WALK;
	//if there's an obstacle at the bot's feet and head then
	//the bot might be able to crouch through
	VectorCopy( bs->origin, start );
	start[2] += 18;
	VectorMA( start, 5, hordir, end );
	VectorSet( mins, -16, -16, -24 );
	VectorSet( maxs, 16, 16, 4 );
	//
//	bsptrace = AAS_Trace(start, mins, maxs, end, bs->entitynum, MASK_PLAYERSOLID);
//	if (bsptrace.fraction >= 1) movetype = MOVE_CROUCH;
	//get the sideward vector
	// START	xkan, 8/22/2002
	// If the script says we should be crouching, then move crouched.
	if ( bs->script.flags & BSFL_CROUCH ) {
		movetype = MOVE_CROUCH;
	}
	// END		xkan, 8/22/2002
	CrossProduct( hordir, up, sideward );
	//
	if ( bs->flags & BFL_AVOIDRIGHT ) {
		VectorNegate( sideward, sideward );
	}
	//try to crouch straight forward?
	if ( movetype != MOVE_CROUCH || !trap_BotMoveInDirection( bs->ms, hordir, 400, movetype ) ) {
		//perform the movement
		if ( !trap_BotMoveInDirection( bs->ms, sideward, 400, movetype ) ) {
			//flip the avoid direction flag
			bs->flags ^= BFL_AVOIDRIGHT;
			//flip the direction
			VectorNegate( sideward, sideward );
			//move in the other direction
			trap_BotMoveInDirection( bs->ms, sideward, 400, movetype );
		}
	}
	//just reset goals and hope the bot will go into another direction
	//still needed??
}

/*
==================
BotCheckEvents
==================
*/
void BotCheckEvents( bot_state_t *bs, entityState_t *state ) {
	int event;
	char buf[128];

	// The array "entityeventTime" is hardcoded to 1024
	assert( state->number < 1024 );

	//
	//this sucks, we're accessing the gentity_t directly but there's no other fast way
	//to do it right now
	if ( bs->entityeventTime[state->number] == g_entities[state->number].eventTime ) {
		return;
	}
	bs->entityeventTime[state->number] = g_entities[state->number].eventTime;

//@TEST.  See if this is ever called when the state->number isn't us!
	if ( state->number != bs->client ) {
		int foo;
		foo = 7;

	} // if (state->number != bs->client)...
	  //if it's an event only entity
	if ( state->eType > ET_EVENTS ) {
		event = ( state->eType - ET_EVENTS ) & ~EV_EVENT_BITS;
	} else {
		event = state->event & ~EV_EVENT_BITS;
	}
	//
	switch ( event ) {
		//client obituary event
	case EV_OBITUARY:
	{
		int target, attacker, mod;

		target = state->otherEntityNum;
		attacker = state->otherEntityNum2;
		mod = state->eventParm;
		//
		if ( target == bs->client ) {


			//


			//
			bs->num_deaths++;
		}
		//else if this client was killed by the bot
		else if ( attacker == bs->client ) {



			//
			bs->num_kills++;
		} else if ( attacker == bs->enemy && target == attacker )     {

		}
		break;
	}
	case EV_GLOBAL_SOUND:
		break;
	case EV_PLAYER_TELEPORT_IN:
	{
		VectorCopy( state->origin, lastteleport_origin );
		lastteleport_time = trap_AAS_Time();
		break;
	}
	case EV_GENERAL_SOUND:
	{
		//if this sound is played on the bot
		if ( state->number == bs->client ) {
			if ( state->eventParm < 0 || state->eventParm > MAX_SOUNDS ) {
				BotAI_Print( PRT_ERROR, "EV_GENERAL_SOUND: eventParm (%d) out of range\n", state->eventParm );
				break;
			}
			//check out the sound
			trap_GetConfigstring( CS_SOUNDS + state->eventParm, buf, sizeof( buf ) );
			//if falling into a death pit
			if ( !strcmp( buf, "*falling1.wav" ) ) {
/*					//if the bot has a personal teleporter
					if (bs->inventory[INVENTORY_TELEPORTER] > 0) {
						//use the holdable item
						trap_EA_Use(bs->client);
					}
*/          }
		}
		break;
	}
	}
}

/*
==================
BotCheckSnapshot
==================
*/
void BotCheckSnapshot( bot_state_t *bs ) {
	int ent;
	entityState_t state;

	//
	ent = 0;
	while ( ( ent = BotAI_GetSnapshotEntity( bs->client, ent, &state ) ) != -1 ) {
		//check the entity state for events
		BotCheckEvents( bs, &state );
	}
	//check the player state for events
	BotAI_GetEntityState( bs->client, &state );
	//copy the player state events to the entity state
	//state.event = bs->cur_ps.externalEvent;
	//state.eventParm = bs->cur_ps.externalEventParm;
	//
	BotCheckEvents( bs, &state );
}

/*
==================
BotCheckAir
==================
*/
void BotCheckAir( bot_state_t *bs ) {
/*	if (bs->inventory[INVENTORY_ENVIRONMENTSUIT] <= 0) {
		if (trap_AAS_PointContents(bs->eye) & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)) {
			return;
		}
	}*/
}

// START	Mad Doctor I changes, 8/15/2002

/*
==================
BotCheckAlert
==================
*/
#define AISTATE_RELAXING_STAGE1 4000        // bot still feels like in battle mode
#define AISTATE_RELAXING_STAGE2 60000       // bot is in high alert
#define AISTATE_RELAXING_FOOTSTEP 4000      // time it takes to relax from foot step hearing

void BotCheckAlert( bot_state_t *bs ) {
}



// END		Mad Doctor I changes, 8/15/2002

//============================================================================

// Start	TAT 9/23/2002

// Update recon state information for a bot
void BotUpdateReconInfo( bot_state_t *bs ) {
	// Index for looping through entities
	int i = 0;

	// Detailed info about the entities
	aas_entityinfo_t entinfo;


	// Only relevant for Single Player
	if ( !BotSinglePlayer() && !BotCoop() ) {
		return;
	}

	// See if I've been hit.
	if ( bs->lasthealth > bs->inventory[INVENTORY_HEALTH] ) {
		bs->reconInfo = BOTRECON_UNDERFIRE;
		return;
	}

	// want to update if we've been hit above regardless, since that's worse than just spotting an enemy
	//		but now if it's not all clear, we don't need to do any further updates
	if ( bs->reconInfo != BOTRECON_ALLCLEAR ) {
		return;
	}

	// not hit, no enemies spotted, so see if we can find anyone
	// Check all entities to see if someone near me is shooting
	for ( i = 0; i < level.maxclients; i++ )
	{
		// We don't count
		if ( i == bs->client ) {
			continue;
		}

		// Fill in the detailed bot info
		BotEntityInfo( i, &entinfo );

		// Skip invalid entities
		if ( !entinfo.valid ) {
			continue;
		}

		// if the enemy isn't dead and the enemy isn't the bot self
		if ( EntityIsDead( &entinfo ) || entinfo.number == bs->entitynum ) {
			continue;
		}

		// if the enemy is invisible
		if ( EntityIsInvisible( &entinfo ) ) {
			continue;
		}

		// if it's an ally, skip it
		if ( BotSameTeam( bs, entinfo.number ) ) {
			continue;
		}

		// If I can see the bot, then we're done
		if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 120, entinfo.number, NULL ) ) {
			bs->reconInfo = BOTRECON_ENEMYSPOTTED;
			return;
		}
	}
}

// End		TAT 9/23/2002

#define MAX_NODESWITCHES    10
int numnodeswitches;
char nodeswitch[MAX_NODESWITCHES + 1][144];

/*
==================
BotResetNodeSwitches()
==================
*/
void BotResetNodeSwitches( void ) {
	numnodeswitches = 0;
}

/*
==================
BotDumpNodeSwitches()
==================
*/
void BotDumpNodeSwitches( bot_state_t *bs ) {
	int i;
	char netname[MAX_NETNAME];

	ClientName( bs->client, netname, sizeof( netname ) );

#ifndef DONT_PRINT_REPEATED_AI_ERRORS

	BotAI_Print( PRT_MESSAGE, "%s at %1.1f switched more than %d AI nodes\n", netname, trap_AAS_Time(), MAX_NODESWITCHES );

#endif // #ifndef DONT_PRINT_REPEATED_AI_ERRORS

	for ( i = 0; i < numnodeswitches; i++ ) {
		BotAI_Print( PRT_MESSAGE, nodeswitch[i] );
	}

#ifndef DONT_PRINT_REPEATED_AI_ERRORS
	BotAI_Print( PRT_FATAL, "" );
#endif // #ifndef DONT_PRINT_REPEATED_AI_ERRORS
}

//============================================================================

/*
==================
BotSetupDeathmatchAI
==================
*/
void BotDeathmatchAIFirstCalled
(
	bot_state_t *bs
) {
	char gender[144], name[144];
	char userinfo[MAX_INFO_STRING];

	bs->setupcount--;
	if ( bs->setupcount > 0 ) {
		return;
	}
	//get the gender characteristic
	trap_Characteristic_String( bs->character, CHARACTERISTIC_GENDER, gender, sizeof( gender ) );
	//set the bot gender
	trap_GetUserinfo( bs->client, userinfo, sizeof( userinfo ) );
//		Info_SetValueForKey(userinfo, "sex", gender);
	trap_SetUserinfo( bs->client, userinfo );
	/*
	//set the team
	if ( g_gametype.integer != GT_TOURNAMENT ) {
		Com_sprintf(buf, sizeof(buf), "team %s", bs->settings.team);
		trap_EA_Command(bs->client, buf);
	}
	*/
	//set the chat gender
	if ( gender[0] == 'm' ) {
		trap_BotSetChatGender( bs->cs, CHAT_GENDERMALE );
	} else if ( gender[0] == 'f' ) {
		trap_BotSetChatGender( bs->cs, CHAT_GENDERFEMALE );
	} else { trap_BotSetChatGender( bs->cs, CHAT_GENDERLESS );}
	//set the chat name
	ClientName( bs->client, name, sizeof( name ) );
	trap_BotSetChatName( bs->cs, name );
	//
	bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];

	//
	bs->setupcount = 0;

}


void BotPowThink
(
	bot_state_t *bs
) {
	if ( bs->clientCheckTime < trap_AAS_Time() ) {
		int i;
		gentity_t*  ent;
		vec3_t vec;
		qboolean axisNearby = qfalse;
		qboolean alliesNearby = qfalse;

		for ( i = 0; i < level.numConnectedClients; i++ ) {
			if ( bs->client == level.sortedClients[i] ) {
				continue;
			}

			ent = BotGetEntity( level.sortedClients[i] );
			if ( !ent ) {
				continue;
			}

			if ( ent->client->sess.sessionTeam != TEAM_AXIS && ent->client->sess.sessionTeam != TEAM_ALLIES ) {
				continue;
			}

			if ( ent->health <= 0 ) {
				continue;
			}

			if ( fabs( ent->client->ps.origin[2] - bs->origin[2] ) > 64 ) {
				// basically, not thru ceilings
				continue;
			}

			VectorSubtract( ent->client->ps.origin, bs->origin, vec );
			vec[2] = 0;
			// ignore height diffs

			if ( DotProduct( vec, vec ) > ( 128 * 128 ) ) {
				continue;
			}

			if ( ent->client->sess.sessionTeam == TEAM_AXIS ) {
				axisNearby = qtrue;
				if ( alliesNearby ) {
					break;
				}
			} else if ( ent->client->sess.sessionTeam == TEAM_ALLIES ) {
				alliesNearby = qtrue;
				if ( axisNearby ) {
					break;
				}
			}

		}

		if ( axisNearby && !alliesNearby ) {
			Bot_ScriptEvent( bs->client, "trigger", "axisnearby" );
		} else if ( !axisNearby && alliesNearby ) {
			Bot_ScriptEvent( bs->client, "trigger", "alliesnearby" );
		}

		bs->clientCheckTime = trap_AAS_Time() + 1.f;
	}
}



/*
==================
BotDeathmatchAI
==================
*/
void BotDeathmatchAI( bot_state_t *bs, float thinktime ) {
	int i = 0;

	//if the bot has just been setup
	if ( bs->setupcount > 0 ) {
		BotDeathmatchAIFirstCalled( bs );
	}

	//while in warmup, keep checking for best class
	if ( ( level.warmupTime > level.time ) && ( bs->lastClassCheck < level.time - 1000 ) ) {  // check for a better class
		bs->mpClass = BotSuggestClass( bs, bs->mpTeam );
		level.clients[bs->client].sess.latchPlayerType = bs->mpClass;
		bs->lastClassCheck = level.time;
	}
	// if we want to dismount
	if ( bs->flags & BFL_DISMOUNT_MG42 ) {
		// clear scripted mg42 command
		bs->script.flags &= ~BSFL_MOUNT_MG42;
		bs->script.mg42entnum = -1;
		//
		if ( !( g_entities[bs->client].s.eFlags & EF_MG42_ACTIVE ) ) {
			bs->flags &= ~BFL_DISMOUNT_MG42;
		} else {
			if ( rand() % 2 ) {
				trap_EA_Activate( bs->client );
			}
		}
	}

	bs->leader_tagent = -1;

	// share last attacked
	BotShareLastAttacked( bs );

	//no ideal view set
	bs->flags &= ~BFL_IDEALVIEWSET;
	bs->flags &= ~BFL_BATTLE_MODE;  // set it if we are
	bs->flags &= ~BFL_SNIPING;

	//set the teleport time
	BotSetTeleportTime( bs );

	//update some inventory values
	BotUpdateInventory( bs );

	//check out the snapshot
	BotCheckSnapshot( bs );

	//check for air
	BotCheckAir( bs );

	//check for required engineer's
	if ( BotIsDead( bs ) || bs->sess.playerType != PC_ENGINEER ) {
		if ( BotCheckNeedEngineer( bs, bs->sess.sessionTeam ) ) {
			// should change automatically
			bs->mpClass = PC_ENGINEER;
			level.clients[bs->client].sess.latchPlayerType = bs->mpClass;
			if ( !BotIsDead( bs ) ) {
				Cmd_Kill_f( &g_entities[bs->client] );
			}
		}
	}

	// Check to see if I need to change my level of alert
//	BotCheckAlert(bs);

	// xkan, 1/10/2003 - set aiState of playerState (used by animation system)
	g_entities[bs->client].client->ps.aiState = bs->alertState;

	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	if ( BotIsPOW( bs ) ) {
		BotPowThink( bs );
	}

	// if the bot has no ai node
	if ( !bs->ainode ) {
		BotDefaultNode( bs );
		if ( !bs->ainode ) {
			return; // nothing to do..
		}
	}

	// reset the node switches from the previous frame
	BotResetNodeSwitches();

	// If we've been scripted to SLEEP, we want to not execute ainodes!
	// But, we'd still want to do our scripted, etc...
/*	if (!bs->scriptedSleep) {
		//execute AI nodes
		for (i = 0; i < MAX_NODESWITCHES && bs->ainode; i++) {
			if(bot_profile.integer == 4 || bot_profile.integer == 5) {
				qboolean ret;
				int t = trap_Milliseconds();
				const char* txt = bs->ainodeText;

				ret = bs->ainode(bs);

				t = trap_Milliseconds() - t;

				if(bot_profile.integer == 5) {
					if(t > 5) {
						G_Printf( "AINode %s took %i ms\n", txt, t );
					}
				} else {
					G_Printf( "AINode %s took %i ms\n", txt, t );
				}

				if( ret ) {
					break;
				}
			} else {
				if( bs->ainode(bs)) {
					break;
				}
			}
		}
	}*/

	//if the bot removed itself :)
	if ( !bs->inuse ) {
		return;
	}

	// if the node changed, log entry
	if ( i > 0 ) {
//		Bot_ScriptLog_Entry( bs, qfalse, ">> NODE CHANGE <<", va("%s\r\n", bs->ainodeText) );
	}

	// bot scripting
	Bot_ScriptRun( bs, qfalse );

	//if the bot executed too many AI nodes
	if ( i >= MAX_NODESWITCHES ) {
#ifdef _DEBUG
		char name[144];
		trap_BotDumpGoalStack( bs->gs );
		trap_BotDumpAvoidGoals( bs->gs );
		BotDumpNodeSwitches( bs );
		ClientName( bs->client, name, sizeof( name ) );

//#ifndef DONT_PRINT_REPEATED_AI_ERRORS
		BotAI_Print( PRT_ERROR, "%s at %1.1f switched more than %d AI nodes\n", name, trap_AAS_Time(), MAX_NODESWITCHES );
//#endif // #ifndef DONT_PRINT_REPEATED_AI_ERRORS
#endif
	}

	//
	bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];


	bs->lasthealth = g_entities[bs->client].health;
}

/*
==================
BotSetupDeathmatchAI
==================
*/
void BotSetupDeathmatchAI( void ) {
	int ent, modelnum;
	char model[128];

	gametype = trap_Cvar_VariableIntegerValue( "g_gametype" );

	trap_Cvar_Register( &bot_rocketjump, "bot_rocketjump", "0", 0 );
	trap_Cvar_Register( &bot_grapple, "bot_grapple", "0", 0 );
	trap_Cvar_Register( &bot_fastchat, "bot_fastchat", "0", 0 );
	trap_Cvar_Register( &bot_nochat, "bot_nochat", "1", CVAR_ROM );
	trap_Cvar_Register( &bot_testrchat, "bot_testrchat", "0", 0 );
	//
	max_bspmodelindex = 0;
	for ( ent = trap_AAS_NextBSPEntity( 0 ); ent; ent = trap_AAS_NextBSPEntity( ent ) ) {
		if ( !trap_AAS_ValueForBSPEpairKey( ent, "model", model, sizeof( model ) ) ) {
			continue;
		}
		if ( model[0] == '*' ) {
			modelnum = atoi( model + 1 );
			if ( modelnum > max_bspmodelindex ) {
				max_bspmodelindex = modelnum;
			}
		}
	}
	//initialize the waypoint heap
	BotInitWaypoints();
}

/*
==================
BotShutdownDeathmatchAI
==================
*/
void BotShutdownDeathmatchAI( void ) {
}

/*
===============
BotSetBlockEnt
===============
*/
void BotSetBlockEnt( int client, int blocker ) {
	botstates[client].blockent = blocker;
	botstates[client].blockentTime = level.time;
}


/*
==================
BotMoveToIntermission
==================
*/
void BotMoveToIntermission( int client ) {
	char cs[MAX_STRING_CHARS];              // DHM - Nerve
	char        *buf;                       // DHM - Nerve
	int winner;                             // DHM - Nerve
	bot_state_t *bs;

	if ( !g_entities[client].r.svFlags & SVF_BOT ) {
		return;
	}

	bs = &botstates[client];

	winner = -1;
	if ( g_gametype.integer >= GT_WOLF ) {
		if ( rand() % 2 == 0 ) {
			trap_GetConfigstring( CS_MULTI_MAPWINNER, cs, sizeof( cs ) );
			buf = Info_ValueForKey( cs, "winner" );
			winner = atoi( buf );
			// if we won, talk it up
			if ( winner == ( bs->sess.sessionTeam - 1 ) ) {
				//if (rand()%3) {
				BotSendVoiceChat( bs, "Cheer", SAY_ALL, 1000 + rand() % 5000, BOT_SHOWTEXT, qfalse );
				//} else {
				//	BotSendVoiceChat( bs, "GoodGame", SAY_ALL, 1000 + rand()%3000, BOT_SHOWTEXT, qfalse );
				//}
			} else if ( bs->sess.sessionTeam ) {
				//if (rand()%2) {
				BotSendVoiceChat( bs, "Negative", SAY_ALL, 1000 + rand() % 5000, BOT_SHOWTEXT, qfalse );
				//} else {
				//	BotSendVoiceChat( bs, "GoodGame", SAY_ALL, 1000 + rand()%3000, BOT_SHOWTEXT, qfalse );
				//}
			}
		}
	}

}

/*
================
BotRecordTeamChange
================
*/
void BotRecordTeamChange( int client ) {
	int team;
	int i;
	bot_state_t *bs;

	team = g_entities[client].client->sess.sessionTeam;

	// greet the new player
	for ( i = 0; i < level.maxclients; i++ ) {
		bs = &botstates[i];
		if ( !bs->inuse ) {
			continue;
		}
		if ( bs->sess.sessionTeam != team ) {
			continue;
		}

		// Don't use excess voices in Single Player
		if ( !BotSinglePlayer() && !BotCoop() ) {
			BotVoiceChatAfterIdleTime( bs->client, "Hi", SAY_TEAM, 1000 + rand() % 6000, BOT_SHOWTEXT, 7000, qfalse  );
		}
	}
}

/*
==================
BotRecordKill
==================
*/
void BotRecordKill( int client, int enemy ) {
	bot_state_t *bs = &botstates[client];

	if ( client == enemy ) {
		return;
	}

	// if this is a team kill, ignore
	if ( BotSameTeam( bs, enemy ) ) {
		// friendly fire!
		// Don't use excess voices in Single Player
		if ( !BotSinglePlayer() && !BotCoop() ) {
			BotVoiceChatAfterIdleTime( bs->client, "Sorry", SAY_TEAM, 1000 + rand() % 4000, qfalse, 3000 + rand() % 2000, qfalse  );
		}
		return;
	}

	// if last kill was too long ago
	if ( bs->last_kill_time < level.time - 10000 ) {
		bs->shorterm_kill_count = 0;
		bs->last_kill_time = level.time;
		return;
	}

	// another kill
	bs->shorterm_kill_count++;
	bs->last_kill_time = level.time;

	// Don't use excess voices in Single Player
	if ( !G_IsSinglePlayerGame() ) {
		// if we have killed enough in the short term, talk it up
		if ( bs->shorterm_kill_count > 2 ) {
			BotSendVoiceChat( bs, "Yeah", SAY_ALL, 1000 + rand() % 1000, qfalse, qfalse );
		} else if ( bs->shorterm_kill_count > 1 )     {
			BotVoiceChatAfterIdleTime( bs->client, "EnemyWeak", SAY_TEAM, 1000 + rand() % 1000, qfalse, 3000, qfalse  );
		}
	}
}

/*
===================
BotRecordPain
===================
*/
void BotRecordPain( int client, int enemy, int mod ) {
	bot_state_t *bs = &botstates[client];
	gentity_t *targ;

	if ( client == enemy ) {
		return;
	}

	// Mad Doc xkan, 10/30/2002 - if we are already dead, just return
	if ( g_entities[bs->client].health <= 0 ) {
		return;
	}

	// if this is a team kill, ignore
	if ( enemy < level.maxclients && BotSameTeam( bs, enemy ) ) {
		// friendly fire!
		// Don't use excess voices in Single Player
		BotVoiceChatAfterIdleTime( bs->client, "HoldYourFire", SAY_TEAM, 1000 + rand() % 1000, qfalse, 3000 + rand() % 2000, qfalse  );
		return;
	}

	if ( enemy >= level.maxclients ) {
		return;
	}

	bs->last_pain = level.time;
	bs->last_pain_client = enemy;
	g_entities[bs->client].botLastAttackedTime = level.time;

	// if we are defending/near an objective
	if ( bs->defendgoal.entitynum >= level.maxclients ) {
		targ = BotGetEntity( bs->defendgoal.entitynum );
		if ( !targ->inuse ) {
			return;
		}
		if ( Q_stricmp( targ->classname, "team_CTF_redflag" ) &&
			 Q_stricmp( targ->classname, "team_CTF_blueflag" ) &&
			 Q_stricmp( targ->classname, "trigger_flagonly" ) &&
			 Q_stricmp( targ->classname, "team_WOLF_checkpoint" ) ) {
			return;
		}
		if ( VectorDistanceSquared( bs->origin, bs->defendgoal.origin ) > SQR( 1024 ) ) {
			return;
		}
		//if (!trap_InPVS( bs->origin, bs->defendgoal.origin )) return;
		//
		// we are near the goal
		// Don't use excess voices in Single Player
		if ( !BotSinglePlayer() && !BotCoop() ) {
			BotVoiceChatAfterIdleTime( bs->client, "TakingFire", SAY_TEAM, 1000 + rand() % 1000, qfalse, 5000 + rand() % 4000, qfalse  );
		}
	}

}

/*
===================
BotRecordDeath
===================
*/
void BotRecordDeath( int client, int enemy ) {
	bot_state_t *bs = &botstates[client];
	gentity_t *targ;

	if ( client == enemy ) {
		return;
	}

	// if this is a team kill, ignore
	if ( enemy < level.maxclients && BotSameTeam( bs, enemy ) ) {
		return;
	}

	// if we are defending/near an objective
	if ( bs->defendgoal.entitynum > level.maxclients ) {
		targ = BotGetEntity( bs->defendgoal.entitynum );
		if ( !targ->inuse ) {
			return;
		}
		if ( Q_stricmp( targ->classname, "team_CTF_redflag" ) &&
			 Q_stricmp( targ->classname, "team_CTF_blueflag" ) &&
			 Q_stricmp( targ->classname, "trigger_flagonly" ) &&
			 Q_stricmp( targ->classname, "team_WOLF_checkpoint" ) ) {
			return;
		}
		if ( VectorDistanceSquared( bs->origin, bs->defendgoal.origin ) > SQR( 1024 ) ) {
			return;
		}
		//
		// we are near the goal
		BotVoiceChatAfterIdleTime( bs->client, "Incoming", SAY_TEAM, 1000 + rand() % 1000, qfalse, 6000, qfalse  );
	}

}

/*
===================
BotGetAimAccuracySkill
	Get the bot's aim accuracy and skill accounting for distance from leader,
	weapon, pose (standing vs. crouching vs. prone)
===================
*/
void BotGetAimAccuracySkill( bot_state_t *bs, float *outAimAccuracy, float *outAimSkill ) {
	float aim_accuracy, aim_skill;
	//weaponinfo_t wi;
	gclient_t *client = g_entities[bs->client].client;
	gclient_t *clientEnemy;
	//
	// What sort of penalty do we get for being away from a leader
	float noLeaderPenalty = BotNoLeaderPenalty( bs );

	aim_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1 );
	//aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1);
	//
	//get the weapon information
	//trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	//get the weapon specific aim accuracy and or aim skill
	if ( bs->weaponnum == WP_GRENADE_LAUNCHER || bs->weaponnum == WP_GRENADE_PINEAPPLE ) {
		//aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_GRENADELAUNCHER, 0, 1);
		aim_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_GRENADELAUNCHER, 0, 1 );
	}
	if ( bs->weaponnum == WP_PANZERFAUST ) {
		//aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_ROCKETLAUNCHER, 0, 1);
		aim_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_ROCKETLAUNCHER, 0, 1 );
	}
	if ( bs->weaponnum == WP_FLAMETHROWER ) {
		//aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_FLAMETHROWER, 0, 1);
	}

	aim_accuracy = bs->attribs[BOT_AIM_ACCURACY];

	if ( aim_skill > 1 ) {
		aim_skill = 1;
	}
	if ( aim_accuracy > 1 ) {
		aim_accuracy = 1;
	}

	// The current accuracy is original minus original times penalty
	aim_accuracy = aim_accuracy
				   * ( 1.0f - ( NO_LEADER_MAX_AIM_PENALTY * noLeaderPenalty ) );

	if ( client->ps.eFlags & EF_PRONE ) {
		// if prone, increase aim accuracy/skill
		aim_accuracy += ( 1 - aim_accuracy ) * AIM_ACCURACY_BONUS_PRONE;
		aim_skill += ( 1 - aim_skill ) * AIM_SKILL_BONUS_PRONE;
	} else if ( client->ps.eFlags & EF_CROUCHING )     {
		// if crouching, increase aim accuracy
		aim_accuracy += ( 1 - aim_accuracy ) * AIM_ACCURACY_BONUS_CROUCH;
	}

	if ( bs->enemy >= 0 && ( clientEnemy = g_entities[bs->enemy].client ) != NULL ) {
		// if our enemy is prone/crouching, we are less accurate
		if ( clientEnemy->ps.eFlags & EF_PRONE ) {
			aim_accuracy *= ( 1 - AIM_ACCURACY_ENEMY_PENALTY_PRONE );
		} else if ( clientEnemy->ps.eFlags & EF_CROUCHING ) {
			aim_accuracy *= ( 1 - AIM_ACCURACY_ENEMY_PENALTY_CROUCH );
		}
	}

	// return the result through output params
	if ( outAimAccuracy ) {
		*outAimAccuracy = aim_accuracy;
	}
	if ( outAimSkill ) {
		*outAimSkill = aim_skill;
	}
}


/*
================
BotBestSniperSpot

  returns -1 if no spot found
================
*/
int BotBestSniperSpot( bot_state_t *bs ) {
	gentity_t *trav, *bestSpot;
	int areanum, t, bestTime;

	trav = NULL;
	bestSpot = NULL;
	bestTime = 99999;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_SNIPERSPOT ) ) ) {
		// is it disabled?
		if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
			continue;
		}

		// if this is not for our team
		if ( trav->aiTeam && trav->aiTeam != bs->sess.sessionTeam ) {
			continue;
		}

		// get the travel time to the goal
		areanum = BotPointAreaNum( trav->s.number, trav->s.origin );
		if ( !areanum ) {
			continue;
		}

		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl );
		if ( !t ) {
			continue;
		}

		if ( bs->target_goal.entitynum != trav - g_entities ) {
			// if we have recently used this spot, avoid it
			if ( trav->missionLevel && ( trav->missionLevel - level.time < 60000 ) ) {
				continue;
			}
		}

		if ( t < bestTime ) {
			bestTime = t;
			bestSpot = trav;
		}
	}

	if ( bestSpot ) {
		// avoid this spot for a while
		bestSpot->missionLevel = level.time;
		return bestSpot->s.number;
	}

	return -1;
}

/*
================
BotBestLandmineSpotingSpot

  returns -1 if no spot found
================
*/
int BotBestLandmineSpotingSpot( bot_state_t *bs ) {
	gentity_t *trav, *bestSpot;
	int areanum, t, bestTime;

	trav = NULL;
	bestSpot = NULL;
	bestTime = 99999;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_LANDMINESPOTINGSPOT ) ) ) {
		// is it disabled?
		if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
			continue;
		}

		// if this is not for our team
		if ( trav->aiTeam && trav->aiTeam != bs->sess.sessionTeam ) {
			continue;
		}

		// get the travel time to the goal
		areanum = BotPointAreaNum( trav->s.number, trav->s.origin );
		if ( !areanum ) {
			continue;
		}

		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl );
		if ( !t ) {
			continue;
		}

		if ( bs->target_goal.entitynum != trav - g_entities ) {
			// if we have recently used this spot, avoid it
			if ( trav->missionLevel && ( trav->missionLevel - level.time < 20000 ) ) {
				continue;
			}
		}

		if ( t < bestTime ) {
			bestTime = t;
			bestSpot = trav;
		}
	}

	if ( bestSpot ) {
		// avoid this spot for a while
		bestSpot->missionLevel = level.time;
		return bestSpot->s.number;
	}

	return -1;
}

/*
================
BotBestMG42Spot

  returns -1 if no spot found
================
*/
int BotBestMG42Spot( bot_state_t *bs, qboolean force ) {
	gentity_t *trav, *bestSpot, *mg42;
	int areanum, t, bestTime;
	//
	trav = NULL;
	mg42 = NULL;
	bestSpot = NULL;
	bestTime = 99999;
	while ( ( mg42 = BotFindNextStaticEntity( mg42, BOTSTATICENTITY_MG42 ) ) ) {
		//while (trav = BotFindEntity( trav, FOFS(classname), "bot_mg42_spot" )) {
		if ( !mg42->melee ) {
			continue;               // it doesnt have a "mg42 spot"
		}
		trav = mg42->melee;
		// is it disabled?
		if ( mg42->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
			continue;
		}
		// if this is not for our team
		//if (trav->aiTeam && trav->aiTeam != bs->sess.sessionTeam) continue;
		if ( !trav->melee->takedamage ) {
			continue;
		}
		if ( trav->melee->entstate != STATE_DEFAULT ) {
			continue;
		}
		if ( trav->melee->active ) {
			// if the person using it is from the other team, then we should still head for it, to try and take it from them
			if ( mg42->r.ownerNum < level.maxclients && ( g_entities[mg42->r.ownerNum].client->sess.sessionTeam != bs->sess.sessionTeam ) ) {
				// errr..
			} else {
				continue;
			}
		}
		// if we have recently used this spot, ignore it
		// NOTE: if it's active, and we got to here, then it must be an enemy, so "ignore" the ignoreTime
		if ( !force && !mg42->active && trav->botIgnoreTime && ( trav->botIgnoreTime > level.time - 1000 ) ) {
			continue;
		}
		// get the travel time to the goal
		areanum = BotPointAreaNum( trav->s.number, trav->s.origin );
		if ( !areanum ) {
			continue;
		}
		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl );
		if ( !t ) {
			continue;
		}
		if ( t < bestTime ) {
			bestTime = t;
			bestSpot = trav;
		}
	}
	//
	if ( bestSpot ) {
		// avoid this spot for a while
		bestSpot->botIgnoreTime = level.time;
		return bestSpot->s.number;
	}
	//
	return -1;
}

/*
================
BotGetNumVisibleSniperSpots
================
*/
int BotGetNumVisibleSniperSpots( bot_state_t *bs ) {
	gentity_t *trav;
	int cnt;
	vec3_t dest;
	trace_t tr;
	//
	// count the spots first
	trav = NULL;
	cnt = 0;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_SNIPERSPOT ) ) ) {
		// if this is not for the other team
		if ( trav->aiTeam && trav->aiTeam == bs->sess.sessionTeam ) {
			continue;
		}
		// is this spot visible?
		VectorCopy( trav->s.origin, dest );
		trap_Trace( &tr, bs->eye, NULL, NULL, dest, bs->client, MASK_SHOT );
		if ( tr.fraction < 0.9 ) {
			continue;
		}
		cnt++;
	}
	//
	// return a random spot
	return cnt;
}

/*
================
BotGetRandomVisibleSniperSpot
================
*/
int BotGetRandomVisibleSniperSpot( bot_state_t *bs ) {
	#define MAX_SNIPER_SPOTS 40
	gentity_t *trav;
	int cnt, spots[MAX_SNIPER_SPOTS];
	vec3_t dest;
	trace_t tr;
	//
	// count the spots first
	trav = NULL;
	cnt = 0;
	while ( ( trav = BotFindEntity( trav, FOFS( classname ), "bot_sniper_spot" ) ) ) {
		// if this is not for the other team
		if ( trav->aiTeam && trav->aiTeam == bs->sess.sessionTeam ) {
			continue;
		}
		// is this spot visible?
		VectorCopy( trav->s.origin, dest );
		trap_Trace( &tr, bs->eye, NULL, NULL, dest, bs->client, MASK_SHOT );
		if ( tr.fraction < 0.9 ) {
			continue;
		}
		spots[cnt++] = trav->s.number;
	}
	//
	if ( !cnt ) {
		return -1;
	}
	// return a random spot
	return spots[rand() % cnt];
}

/*
==================
BotClearGoal
==================
*/
void BotClearGoal( bot_goal_t *goal ) {
	memset( goal, 0, sizeof( *goal ) );
	goal->entitynum = -1;
}

/*
==================
BotDropFlag
==================
*/
void BotDropFlag( bot_state_t *bs ) {
	gentity_t *ent;
	gitem_t     *item = NULL;
	gentity_t   *flag = NULL;
	vec3_t launchvel;

	ent = BotGetEntity( bs->client );
	if ( ent->client->ps.powerups[PW_REDFLAG] ) {
		item = BG_FindItem( "Red Flag" );
		if ( !item ) {
			item = BG_FindItem( "Objective" );
		}

		ent->client->ps.powerups[PW_REDFLAG] = 0;
	}
	if ( ent->client->ps.powerups[PW_BLUEFLAG] ) {
		item = BG_FindItem( "Blue Flag" );
		if ( !item ) {
			item = BG_FindItem( "Objective" );
		}

		ent->client->ps.powerups[PW_BLUEFLAG] = 0;
	}

	if ( item ) {
		launchvel[0] = crandom() * 20;
		launchvel[1] = crandom() * 20;
		launchvel[2] = 10 + random() * 10;

		flag = LaunchItem( item,ent->r.currentOrigin,launchvel,ent->s.number );
		flag->s.modelindex2 = ent->s.otherEntityNum2; // JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
		flag->message = ent->message;   // DHM - Nerve :: also restore item name
		flag->botIgnoreTime = level.time + 2500;
		flag->r.ownerNum = bs->client;
		// Clear out player's temp copies
		ent->s.otherEntityNum2 = 0;
		ent->message = NULL;
	}
}

/*
===================
BotCalculateMg42Spots
===================
*/
extern vec3_t playerMins, playerMaxs;
void BotCalculateMg42Spots( void ) {
	gentity_t *trav, *sptrav, *newent;
	vec3_t pos, epos, dir, v;
	trace_t tr;
	vec3_t mins, maxs;
	int blueCount, redCount;
	int i;
	float dist;

	// Start	TAT	9/24/2002
	//		This isn't working for constructible MG42s, because our traces are hitting the constructible markers
	//		so we need to temporarily unlink all the linked constructible markers, and then we'll link
	//		them again after we're done, so they don't show up in the traces
	int constructMarkers[MAX_GENTITIES];
	int numMarkers = 0;

	trav = NULL;
	// loop through all the constructible markers
	while ( ( trav = G_Find( trav, FOFS( classname ), "misc_constructiblemarker" ) ) )
	{
		// if it's linked
		if ( trav->r.linked ) {
			// add it to our list
			constructMarkers[numMarkers++] = trav->s.number;

			// and unlink it
			trap_UnlinkEntity( trav );
		}
	}

	// End		TAT 9/24/2002


	VectorCopy( playerMins, mins );
	VectorCopy( playerMaxs, maxs );

	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_MG42 ) ) ) {
		//
		// RF, if this entity already has an mg42 spot, then skip it
		if ( trav->melee ) {
			continue;
		}
		//
		blueCount = 0;
		redCount = 0;
		// go backwards from the mg42 mount position
		AngleVectors( trav->s.angles, dir, NULL, NULL );

		if ( trav->r.maxs[0] > maxs[0] ) {
			dist = trav->r.maxs[0];
		} else {
			dist = maxs[0];
		}

		while ( ( dist += 2 ) < 80 ) {
			maxs[2] = 4;
			mins[2] = 0;
			//
			VectorMA( trav->r.currentOrigin, -dist, dir, pos );
			trap_Trace( &tr, pos, mins, maxs, pos, ENTITYNUM_NONE, MASK_PLAYERSOLID );
			if ( tr.startsolid || tr.allsolid ) {
				continue;
			}
			VectorCopy( tr.endpos, pos );
			VectorCopy( tr.endpos, epos );
			epos[2] -= 48;
			trap_Trace( &tr, pos, mins, maxs, epos, ENTITYNUM_NONE, MASK_PLAYERSOLID );
			if ( tr.startsolid || tr.allsolid ) {
				continue;
			}
			VectorCopy( tr.endpos, pos );
			// move it up to allow for bounding mins
			pos[2] += -playerMins[2];
			mins[2] = playerMins[2];
			// one last check
			trap_Trace( &tr, pos, mins, maxs, pos, ENTITYNUM_NONE, MASK_PLAYERSOLID );
			if ( tr.startsolid || tr.allsolid ) {
				continue;
			}
			break;
		}
		if ( tr.startsolid || tr.allsolid ) {
			// didnt find a good spot
			continue;
		}
		// go back to normal head height
		//maxs[2] = playerMaxs[2];
		// trace down to the ground
		VectorCopy( pos, epos );
		epos[2] -= 128;
		//trap_Trace( &tr, pos, mins, maxs, epos, trav->s.number, MASK_PLAYERSOLID );
		trap_Trace( &tr, pos, mins, maxs, epos, ENTITYNUM_NONE, MASK_PLAYERSOLID );
		if ( tr.startsolid || tr.allsolid ) {
			continue;
		}
		VectorCopy( tr.endpos, pos );
		// go back to normal head height, check position
		maxs[2] = playerMaxs[2];
		trap_Trace( &tr, pos, mins, maxs, pos, ENTITYNUM_NONE, MASK_PLAYERSOLID );
		if ( tr.startsolid || tr.allsolid ) {
			continue;
		}
		// if this position is close enough to the mg42, then assume it's valid
		if ( VectorDistanceSquared( trav->r.currentOrigin, pos ) > SQR( 48 ) ) {
			continue;
		}
		//
		// spawn an mg42 spot
		newent = G_Spawn();
		newent->classname = "bot_mg42_spot";

		// try and place it into the bot game entities
//		newent = BotCheckBotGameEntity( newent );

		newent->melee = trav;
		trav->melee = newent;
		VectorCopy( pos, newent->s.origin );
		VectorCopy( pos, newent->r.currentOrigin );
		VectorAdd( pos, playerMaxs, newent->r.absmax );
		VectorAdd( pos, playerMins, newent->r.absmin );
		VectorCopy( trav->r.currentAngles, newent->r.currentAngles );
		// is this pointing to an axis or allied spawn point
		// BLUE
		sptrav = NULL;
		while ( ( sptrav = G_Find( sptrav, FOFS( classname ), "team_CTF_bluespawn" ) ) ) {
			if ( !( sptrav->spawnflags & 2 ) ) {
				continue;                               // ignore NON-STARTACTIVE spawns
			}
			VectorSubtract( sptrav->s.origin, trav->r.currentOrigin, v );
			VectorNormalize( v );
			if ( DotProduct( v, dir ) > 0 ) {
				blueCount++;
			}
		}
		// RED
		sptrav = NULL;
		while ( ( sptrav = G_Find( sptrav, FOFS( classname ), "team_CTF_redspawn" ) ) ) {
			if ( !( sptrav->spawnflags & 2 ) ) {
				continue;                               // ignore NON-STARTACTIVE spawns
			}
			VectorSubtract( sptrav->s.origin, trav->r.currentOrigin, v );
			VectorNormalize( v );
			if ( DotProduct( v, dir ) > 0 ) {
				redCount++;
			}
		}
		// if the count is hugely in favor of one side, then it must belong to the other side
		if ( blueCount - redCount > 4 ) {
			newent->aiTeam = TEAM_AXIS;                             // mostly facing blue spots
		} else if ( redCount - blueCount > 4 )                                                                   {
			newent->aiTeam = TEAM_ALLIES;                                   // mostly facing red spots
		} else { newent->aiTeam = 0;}
	}


	// Start		TAT 9/24/2002
	//		Relink those constructible markers
	for ( i = 0; i < numMarkers; i++ )
	{
		trap_LinkEntity( &g_entities[constructMarkers[i]] );

	}
	// End			TAT 9/24/2002

}

/*
=================
BotMovementAutonomyForString
=================
*/
int BotMovementAutonomyForString( char *string ) {
	if ( !Q_stricmp( string, "high" ) ) {
		return BMA_HIGH;
	} else if ( !Q_stricmp( string, "medium" ) ) {
		return BMA_MEDIUM;
	} else if ( !Q_stricmp( string, "low" ) ) {
		return BMA_LOW;
	} else {
		return -1;
	}
}

/*
=================
BotStringForMovementAutonomy
=================
*/
char *BotStringForMovementAutonomy( int value ) {
	switch ( value ) {
	case BMA_LOW: return "LOW";
	case BMA_MEDIUM: return "MEDIUM";
	case BMA_HIGH: return "HIGH";
	}
	//
	return "(unknown)";
}

/*
=================
BotWeaponAutonomyForString
=================
*/
int BotWeaponAutonomyForString( char *string ) {
	if ( !Q_stricmp( string, "high" ) ) {
		return BWA_HIGH;
	} else if ( !Q_stricmp( string, "medium" ) ) {
		return BWA_MEDIUM;
	} else if ( !Q_stricmp( string, "low" ) ) {
		return BWA_LOW;
	} else {
		return -1;
	}
}

/*
=================
BotStringForWeaponAutonomy
=================
*/
char *BotStringForWeaponAutonomy( int value ) {
	switch ( value ) {
	case BMA_LOW: return "LOW";
	case BMA_MEDIUM: return "MEDIUM";
	case BMA_HIGH: return "HIGH";
	}
	//
	return "(unknown)";
}


/*
=================
BotScriptAutonomyForString
=================
*/
int BotScriptAutonomyForString( char *string ) {
	if ( !Q_stricmp( string, "quitscript" ) ) {
		return BSA_QUITSCRIPT;
	} else if ( !Q_stricmp( string, "nochase" ) ) {
		return BSA_NOCHASE;
	} else if ( !Q_stricmp( string, "maintainscript" ) ) {
		return BSA_MAINTAINSCRIPT;
	} else if ( !Q_stricmp( string, "ignoreenemies" ) ) {
		return BSA_IGNOREENEMIES;
	} else {
		return -1;
	}
}


/*
==================
BotCheckEmergencyTargets()
==================
*/
qboolean BotCheckEmergencyTargets( bot_state_t *bs ) {
	qboolean retval;
	int startTime = 0;
	if ( bot_profile.integer == 1 ) {
		startTime = trap_Milliseconds();
	}

	retval = BotMP_CheckEmergencyGoals( bs );

	if ( bot_profile.integer == 1 ) {
		botTime_EmergencyGoals += trap_Milliseconds() - startTime;
	}

	return retval;
}

/*
==================
BotFindSpecialGoals()
==================
*/
qboolean BotFindSpecialGoals( bot_state_t *bs ) {
	qboolean retval;
	int startTime = 0;
	if ( bot_profile.integer == 1 ) {
		startTime = trap_Milliseconds();
	}

	trap_Cvar_Update( &bot_findgoal );
	if ( !bot_findgoal.integer ) {
		retval = BotMP_FindGoal_New( bs );
	} else {
		retval = BotMP_FindGoal( bs );
	}

	if ( bot_profile.integer == 1 ) {
		botTime_FindGoals += trap_Milliseconds() - startTime;
	}

	return retval;
}

/*
===================
BotDefaultNode
===================
*/
void BotDefaultNode( bot_state_t *bs ) {
	//if the bot is not in a valid area, then there's not much we can do
	if ( !bs->areanum || bs->sess.sessionTeam >= TEAM_SPECTATOR || !bs->sess.sessionTeam ) {
		AIEnter_MP_Stand( bs );

		return;
	}

	//check emergency goals
	BotClearGoal( &bs->target_goal );
	bs->last_checkemergencytargets = 0;     // bypass optimizations
	if ( BotCheckEmergencyTargets( bs ) ) {
		return;
	}

	//check idle goals
	bs->last_findspecialgoals = 0;          // bypass optimizations
	if ( BotFindSpecialGoals( bs ) ) {
		return;
	}

	//just stand around, waiting for a goal (or enemy) to present itself
	AIEnter_MP_Stand( bs );
	return;
}

#define PLAYER_PERFORMANCE_COMMENT_FREQENCY 5       // the number of recent shots we will comment on
#define ACCURACY_CHANGE_THRESHOLD   0.2             // how big should the change (in either direction) be to consider commenting

// If we're doing well even if it's not an improvement, comment
#define GOOD_ACCURACY_THRESHOLD ( 0.6f )

// If we're doing poorly even if it's not worse, comment
#define BAD_ACCURACY_THRESHOLD ( 0.1f )

/*
=================
BotCheckVoiceChats()
=================
*/
void BotCheckVoiceChats( bot_state_t *bs ) {
	if ( VectorLengthSquared( bs->cur_ps.velocity ) < SQR( 10 ) ) {
		// do we need ammo?
		//		TAT 10/8/2002 - Lieutenants shouldn't bother asking for ammo
		if ( bs->sess.playerType != PC_FIELDOPS && ClientNeedsAmmo( bs->client ) ) {
			BotVoiceChatAfterIdleTime( bs->client, "NeedAmmo", SAY_TEAM, 2000 + rand() % 10000, qfalse, 40000 + rand() % 15000, qfalse  );
		}

		// do we need health?
		// TAT 10/8/2002 - Medics only ask for health if they are dead
		if ( ( bs->sess.playerType == PC_MEDIC && BotHealthScale( bs->client ) <= 0.0 ) || ( bs->sess.playerType != PC_MEDIC && BotHealthScale( bs->client ) <= 0.2 ) ) {
			BotVoiceChatAfterIdleTime( bs->client, "Medic", SAY_TEAM, 2000 + rand() % 10000, qfalse, 30000 + rand() % 10000, qfalse  );
		}

		// if we have received health, then thank someone
		if ( bs->sess.playerType != PC_MEDIC && bs->last_checkvoice_health > 0 && bs->cur_ps.stats[STAT_HEALTH] > bs->last_checkvoice_health ) {
			BotVoiceChatAfterIdleTime( bs->client, "Thanks", SAY_TEAM, 500 + rand() % 1000, qfalse, 5000 + rand() % 5000, qfalse  );
		}
	}

	bs->last_checkvoice_health = bs->cur_ps.stats[STAT_HEALTH];
}


/*
================
BotEnemyFire
================
*/
void BotEnemyFire( bot_state_t *bs ) {
	if ( bs->enemy < 0 ) {
		BotFindEnemyMP( bs, -1, qfalse );
	}

	if ( bs->enemy >= 0 ) {
		aas_entityinfo_t entinfo;
		BotEntityInfo( bs->enemy, &entinfo );
		//if the enemy is dead
		if ( bs->enemydeath_time ) {
			if ( bs->enemydeath_time < trap_AAS_Time() - 0.3 ) {
				bs->enemydeath_time = 0;

				bs->enemy = -1;
			}
		} else {
			if ( EntityIsDead( &entinfo ) ) {
				bs->enemydeath_time = trap_AAS_Time();
			}
		}
		//
		if ( bs->enemy >= 0 ) {
			// attack and keep moving
			if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
				//choose the best weapon to fight with
				BotChooseWeapon( bs );

				// TAT 12/6/2002 - don't use the knife here (only can successfully use it in BattleChase)
				if ( bs->weaponnum != WP_KNIFE ) {
					//aim at the enemy
					BotAimAtEnemySP( bs );
					//attack the enemy if possible
					if ( bs->weaponnum == bs->cur_ps.weapon ) {
						BotCheckAttack( bs );
					}
				}
			} else {
				bs->enemy = -1;
			}
		}
	}
}

/*
================
BotShareLastAttacked
================
*/
void BotShareLastAttacked( bot_state_t *bs ) {
	int i;
	bot_state_t *ptbs, *tbs;
	gentity_t *ent, *trav;
	//
	if ( bs->lastAttackShared > level.time - 400 ) {
		return;
	}
	bs->lastAttackShared = level.time;
	//
	ent = BotGetEntity( bs->client );
	//
	for ( i = 0, ptbs = botstates, trav = g_entities; i < level.maxclients; i++, ptbs++, trav++ )
	{
		if ( !ptbs->inuse ) {
			continue;
		}
		if ( !BotSameTeam( bs, i ) ) {
			continue;
		}
		//
		tbs = ptbs;
		//
		if ( ent->botLastAttackedTime >= trav->botLastAttackedTime ) {
			continue;                                                           // no need to share
		}
		if ( VectorDistanceSquared( bs->origin, tbs->origin ) > SQR( 2048 ) ) {
			continue;
		}
		if ( !trap_InPVS( bs->origin, tbs->origin ) ) {
			continue;
		}
		//
		// get their attack time
		ent->botLastAttackedTime = trav->botLastAttackedTime;
		ent->botLastAttackedEnt = trav->botLastAttackedEnt;
	}
}

/*
==============
BotEnemyCarryingFlag
==============
*/
qboolean BotEnemyCarryingFlag( int entnum ) {
	bot_state_t *pbs;
	int i;

	for ( i = 0, pbs = botstates; i < level.maxclients; i++, pbs++ ) {
		if ( !pbs->inuse ) {
			continue;
		}
		if ( BotSameTeam( ( pbs ), entnum ) ) {
			continue;
		}
		if ( pbs->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( !BotCarryingFlag( i ) ) {
			continue;
		}
		//
		// carrying flag
		return qtrue;
	}
	//
	return qfalse;
}

/*
==============
BotLastAttacked
==============
*/
int BotLastAttacked( bot_state_t *bs ) {
	if ( !g_entities[bs->client].botLastAttackedTime ) {
		return -99999;                                              // allow for game starting at time index 0
	}
	return g_entities[bs->client].botLastAttackedTime;
}

/*
==============
BotLastHurt
==============
*/
int BotLastHurt( bot_state_t *bs ) {
	if ( !bs->last_pain ) {
		return -99999;                  // allow for game starting at time index 0
	}
	return bs->last_pain;
}

/*
=================
BotSeekCover
=================
*/
qboolean BotSeekCover( bot_state_t *bs ) {
	int area, enemyarea;
	vec3_t autonomyPos;
	//static int lastcall;

	if ( bs->enemy < 0 ) {
		return qfalse;
	}

	// TAT 10/8/2002
	//		If I'm in an invalid area, then don't do this, it will crash
	if ( bs->areanum == 0 ) {
		return qfalse;
	}

	//
	// limit calls
	//if (bs->lastSeekCover >= level.time - 500) return qfalse;	// dont hog calls
	//if (lastcall == level.time) return qfalse;
	//lastcall = level.time;


	// look for a place to hide from our enemy
	BotGetMovementAutonomyPos( bs, autonomyPos );
	//
	enemyarea = BotGetArea( bs->enemy );
	//
	area = trap_AAS_NearestHideArea( bs->client, bs->origin, bs->areanum, bs->enemy, g_entities[bs->enemy].r.currentOrigin, enemyarea, bs->tfl, BotGetMovementAutonomyRange( bs, NULL ), autonomyPos );
	//
	if ( area ) {
		// get the area waypoint, and return qtrue
		BotClearGoal( &bs->hidegoal );
		if ( trap_AAS_AreaWaypoint( area, bs->hidegoal.origin ) ) {
			bs->hidegoal.areanum = area;
			return qtrue;
		}
	}
	//
	return qfalse;
}

/*
=================
BotGetEye
=================
*/
float *BotGetEye( int entnum ) {
	#define BOT_GETEYE_NUMEYES  9
	static vec3_t eyes[BOT_GETEYE_NUMEYES];
	static int lastEye = 0;
	float   *eye;
	//
	if ( entnum < 0 || entnum >= level.maxclients ) {
		G_Error( "BotGetEye: entnum out of range" );
		return NULL;
	}
	//
	eye = &eyes[lastEye][0];
	if ( ++lastEye >= BOT_GETEYE_NUMEYES ) {
		lastEye = 0;
	}
	//
	VectorCopy( g_entities[entnum].client->ps.origin, eye );
	eye[2] += g_entities[entnum].client->ps.viewheight;
	//
	return eye;
}

/*
=================
BotGetOrigin
=================
*/
float *BotGetOrigin( int entnum ) {
	#define BOT_GETORIGIN_NUMEYES   9
	static vec3_t eyes[BOT_GETORIGIN_NUMEYES];
	static int lastEye = 0;
	float   *eye;
	gentity_t *ent = BotGetEntity( entnum );

	// TAT 11/12/2002 - using Ryan's botentity system, so might have an entity num out of range
	//		let's just see if we found one with BotGetEntity
	if ( !ent ) {
		G_Error( "BotGetOrigin: invalid entity num %d", entnum );
		return NULL;
	}
	//
//	if (entnum < 0 || entnum >= level.num_entities) {
//		G_Error( "BotGetOrigin: entnum out of range" );
//		return NULL;
//	}
	//
	eye = &( eyes[lastEye][0] );
	if ( ++lastEye >= BOT_GETORIGIN_NUMEYES ) {
		lastEye = 0;
	}
	//
	if ( ( entnum < level.maxclients ) && ( g_entities[entnum].client ) ) {
		VectorCopy( ent->client->ps.origin, eye );
	} else if ( g_entities[entnum].s.eType == ET_TRIGGER_MULTIPLE || g_entities[entnum].s.eType == ET_MOVER ) {
		VectorAdd( ent->r.absmin, ent->r.absmax, eye );
		VectorScale( eye, 0.5, eye );
	} else if ( VectorLengthSquared( ent->r.currentOrigin ) ) {
		VectorCopy( ent->r.currentOrigin, eye );
	} else if ( VectorLengthSquared( ent->s.origin ) ) {
		VectorCopy( ent->s.origin, eye );
	} else {
		VectorAdd( ent->r.absmin, ent->r.absmax, eye );
		VectorScale( eye, 0.5, eye );
	}
	//
	return eye;
}

/*
================
BotGetArea
================
*/
int BotGetArea( int entnum ) {
	bot_state_t *bs = NULL;
	gentity_t *ent;
	//
	if ( entnum < level.maxclients ) {
		bs = &botstates[entnum];
	}
	ent = BotGetEntity( entnum );
	//
	if ( !ent ) {
		// try to get a server entity
		g_serverEntity_t *serverEnt = GetServerEntity( entnum );
		// we found one, so return the cached value
		if ( serverEnt ) {
			if ( serverEnt->areaNum == -1 ) {
				// we haven't calculated it yet
				//		these don't move, so only calc once
				serverEnt->areaNum = BotPointAreaNum( -1, serverEnt->origin );
			}
			return serverEnt->areaNum;
		}

		// didn't find it
		return 0;
	}
	if ( !bs || !bs->inuse ) {
		if ( VectorCompare( BotGetOrigin( entnum ), ent->botGetAreaPos ) ) {
			return ent->botGetAreaNum;
		}
		VectorCopy( BotGetOrigin( entnum ), ent->botGetAreaPos );
		ent->botGetAreaNum = BotPointAreaNum( entnum, ent->botGetAreaPos );
		return ent->botGetAreaNum;
	} else {
		return bs->areanum;
	}
}

/*
==============
BotReduceListByRange
==============
*/
int BotReduceListByTravelTime( int *list, int numList, vec3_t destpos, int destarea, int traveltime ) {
	int listCopy[MAX_CLIENTS], numListCopy = 0, i;
	int areanum, t;
	bot_state_t *lbs;

	if ( !traveltime ) {
		return numList; // no change
	}

	for ( i = 0; i < numList; i++ ) {
		areanum = BotGetArea( list[i] );
		if ( !areanum ) {
			continue;   // eliminate them
		}
		lbs = &botstates[list[i]];
		if ( !lbs->inuse ) {
			continue;
		}
		t = trap_AAS_AreaTravelTimeToGoalArea( areanum, BotGetOrigin( list[i] ), destarea, lbs->tfl );
		if ( !t ) {
			continue;
		}
		if ( t >= traveltime ) {
			continue;
		}

		//
		// they passed so copy to the new list
		//
		listCopy[numListCopy++] = list[i];
	}

	memcpy( list, listCopy, sizeof( int ) * numListCopy );
	return numListCopy;
}

/*
================
BotTravelTimeToEntity
================
*/
int BotTravelTimeToEntity( bot_state_t *bs, int entnum ) {
	int area = 0;
	//
	if ( !bs->areanum ) {
		return 0;
	}
	//
	area = BotGetArea( entnum );
	//
	if ( !area ) {
		return 0;
	}
	//
	if ( !bs->tfl ) {
		bs->tfl = BotTravelFlagsForClient( bs->client );
	}
	return trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, area, bs->tfl );
}

// How far can a bot go to seek cover?
int BotGetSeekCoverRange( bot_state_t *bs, int targetEntity );

/*
==============
BotBattleNewNode
==============
*/
qboolean BotBattleNewNode( bot_state_t *bs ) {
	return qfalse;
}

/*
===============
BotDirectMoveToGoal
===============
*/
qboolean BotDirectMoveToGoal( bot_state_t *bs, bot_goal_t *goal, bot_moveresult_t *moveresult ) {
	vec3_t dir;
	aas_clientmove_t move;
	trace_t tr;
	float dist;
	//
	if ( VectorDistanceSquared( bs->origin, goal->origin ) > SQR( 1400 ) ) {
		return qfalse;
	}
	if ( !trap_InPVS( bs->origin, goal->origin ) ) {
		return qfalse;
	}
//	trap_Trace( &tr, bs->origin, vec3_origin, vec3_origin, goal->origin, bs->client, MASK_PLAYERSOLID & ~CONTENTS_BODY );
//	if (tr.fraction < 1.0) return qfalse;
	//
	VectorSubtract( goal->origin, bs->origin, dir );
	dist = VectorNormalize( dir );
	VectorScale( dir, 300, dir );
	//
	if ( trap_AAS_PredictClientMovement( &move, bs->client, bs->origin,
										 goal->entitynum, qfalse,
										 dir, goal->origin, -1,
										 40, 0.05, SE_ENTERAREA | SE_HITGROUNDDAMAGE | SE_HITENT | SE_HITGROUNDAREA | SE_STUCK | SE_GAP, goal->areanum,
#ifdef _DEBUG
										 qtrue ) )
#else
										 qfalse ) )
#endif
	{
		//
		// check for a good stop event
		//
		switch ( move.stopevent ) {
		case SE_ENTERAREA:
		case SE_HITENT:
		case SE_HITGROUNDAREA:
			memset( moveresult, 0, sizeof( *moveresult ) );
			VectorNormalize( dir );
			VectorCopy( dir, moveresult->movedir );
			if ( dist < 200 ) {
				trap_EA_Move( bs->client, dir, 400 - ( 320.0f * ( 128.0f - dist ) / 128.0f ) );
			} else {
				trap_EA_Move( bs->client, dir, 400 );
			}
			// check against other players/bots
			//		TAT 2/3/2003 - you can be blocked by stuff that isn't a player - changing trace mask to include other stuff
			trap_Trace( &tr, bs->origin, bs->cur_ps.mins, bs->cur_ps.maxs, goal->origin, bs->client, MASK_SHOT /*CONTENTS_BODY*/ );
			if ( tr.fraction < .99f && VectorDistanceSquared( bs->origin, tr.endpos ) < SQR( 30 ) && tr.entityNum != ENTITYNUM_WORLD ) { // best not be worldspawn
				// blocked, avoid them
				moveresult->blocked = qtrue;
				moveresult->blockentity = tr.entityNum;
			}
			moveresult->flags |= MOVERESULT_DIRECTMOVE;
			//
			return qtrue;
		}
	}
	//
	return qfalse;
}
/*
=================
BotEntityTargetClassnameMatch
=================
*/
qboolean BotEntityTargetClassnameMatch( int entityNum, const char *classname ) {
	gentity_t *ent;
	//
	if ( entityNum < 0 || entityNum > level.num_entities ) {
		return qfalse;
	}
	ent = BotGetEntity( entityNum );
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->target ) {
		return qfalse;
	}
	if ( !ent->target_ent ) {
		return qfalse;
	}
	if ( !ent->target_ent->inuse ) {
		return qfalse;
	}
	//
	if ( !Q_stricmp( ent->target_ent->classname, classname ) ) {
		return qtrue;
	}
	//
	return qfalse;
}

/*
================
BotGetReachableEntityArea
================
*/
qboolean BotGetReachableEntityArea( bot_state_t *bs, int entityNum, bot_goal_t *goal ) {
	vec3_t brushPos, vec, center, mins, maxs;
	//int list[256], numList;
	int oldestTime = 0, i, oldest = 0;
	//float bestDist, dist;
	gentity_t *ent;
	trace_t tr;

	ent = BotGetEntity( entityNum );

	if ( VectorDistanceSquared( ent->r.absmin, ent->r.absmax ) > ( 16 * 16 ) ) {
		VectorAdd( ent->r.absmin, ent->r.absmax, brushPos );
		VectorScale( brushPos, 0.5, brushPos );

		oldest = BotReachableBBoxAreaNum( bs, ent->r.absmin, ent->r.absmax );
		if ( !oldest ) {
			VectorCopy( ent->r.absmax, maxs );
			maxs[2] += 32;
			oldest = BotReachableBBoxAreaNum( bs, ent->r.absmin, maxs );
		}

		if ( oldest ) {
			oldestTime = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, oldest, bs->tfl );
		}
	} else {    // use entity point
		// TODO
		i = BotGetArea( entityNum );
		oldestTime = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, i, bs->tfl );
	}

	if ( oldestTime <= 0 ) {
		return qfalse;
	}

	BotClearGoal( goal );
	// use this as the goal origin
	if ( !trap_AAS_AreaWaypoint( oldest, center ) ) {
		trap_AAS_AreaCenter( oldest, center );
	}

	// if the entity is a trigger, then we must make sure we are within the brush
	if ( ent->r.contents & CONTENTS_TRIGGER ) {
		VectorCopy( center, vec );
		VectorAdd( center, bs->cur_ps.mins, mins );
		VectorAdd( center, bs->cur_ps.maxs, maxs );
		if ( !trap_EntityContactCapsule( mins, maxs, ent ) ) {
			VectorCopy( brushPos, center );
			center[2] = vec[2];
			VectorCopy( brushPos, vec );
			vec[2] -= 512;
			// trace to the ground
			trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, vec, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
			VectorCopy( tr.endpos, center );
			// test this spot
			VectorAdd( center, bs->cur_ps.mins, mins );
			VectorAdd( center, bs->cur_ps.maxs, maxs );
			if ( !trap_EntityContactCapsule( mins, maxs, ent ) ) {
				return qfalse;
			}
			oldest = BotPointAreaNum( bs->client, center );
			if ( !oldest ) {
				return qfalse;
			}
		}
	}
	VectorCopy( center, goal->origin );
	VectorCopy( bs->cur_ps.mins, goal->mins );
	VectorCopy( bs->cur_ps.maxs, goal->maxs );
	goal->areanum = oldest;
	goal->entitynum = ent->s.number;
	goal->flags = GFL_NOSLOWAPPROACH;

	return qtrue;
}

/*
====================
BotIsConstructible

  returns qtrue if the contructible attached to the given target_objective_info can be built
====================
*/
qboolean BotIsConstructible( team_t team, int toiNum ) {
	gentity_t* ent;
	gentity_t *toi = &g_entities[toiNum];

	// we dont wanna build this
	if ( toi->aiInactive & ( 1 << team ) ) {
		return qfalse;
	}

	if ( !( ent = G_ConstructionForTeam( toi, team ) ) ) {
		return qfalse;
	}

	if ( G_ConstructionIsFullyBuilt( ent ) ) {
		return qfalse;
	}

	if ( G_ConstructionIsPartlyBuilt( ent ) ) {
		return qtrue;
	}

	if ( ent->chain && G_ConstructionBegun( ent->chain ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
=================
BotCanSnipe

  returns WP_NONE if we cant snipe, otherwise returns the weapon we can snipe with
=================
*/
int BotCanSnipe( bot_state_t *bs, qboolean checkAmmo ) {
	int sniperWeapons[] = {WP_GARAND_SCOPE, WP_K43_SCOPE, WP_FG42SCOPE, -1};
	int i, best, bestAmmo, thisAmmo;

	// Gordon: early out if not covert ops only they have sniper weapons
	if ( bs->cur_ps.stats[STAT_PLAYER_CLASS] != PC_COVERTOPS ) {
		return WP_NONE;
	}

	best = WP_NONE;
	bestAmmo = 0;
	for ( i = 0; sniperWeapons[i] > -1; i++ ) {
		if ( !COM_BitCheck( bs->cur_ps.weapons, sniperWeapons[i] ) ) {
			continue;
		}

		thisAmmo = BotGotEnoughAmmoForWeapon( bs, sniperWeapons[i] );
		if ( checkAmmo && !thisAmmo ) {
			continue;
		}
		if ( best > -1 && !thisAmmo ) {
			continue;
		}

		if ( best == -1 || ( !bestAmmo && thisAmmo ) ) {
			best = sniperWeapons[i];
			bestAmmo = thisAmmo;
		}
	}

	return best;
}

/*
=================
BotHealthScale
=================
*/
float BotHealthScale( int entnum ) {
	return g_entities[entnum].health / (float)g_entities[entnum].client->ps.stats[STAT_MAX_HEALTH];
}

/*
==================
EnemyIsCloseEnoughToFight()
==================

Returns true if we can do the battle fight.
Returns false if enemy is too far or not visible

*/
qboolean EnemyIsCloseEnoughToFight
(
	bot_state_t *bs
) {
	// Local Variables ////////////////////////////////////////////////////////
	aas_entityinfo_t entinfo;
	///////////////////////////////////////////////////////////////////////////

	// Get location of enemy and other info.
	BotEntityInfo( bs->enemy, &entinfo );

	// If we've got the knife, the enemy isn't visible, or the enemy is too far, we
	// can't use battle fight.
	if  ( ( bs->weaponnum == WP_KNIFE )
		  || !BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL )
		  || ( VectorDistanceSquared( bs->origin, entinfo.origin ) > SQR( kBOT_CHASE_RANGE ) )
		  ) {
		return qfalse;
	}

	// Otherwise, we're good to fight
	return qtrue;

}

// TAT 11/21/2002
//		This is silly - almost all the single player ai nodes do the same thing
//		Find an enemy and try to attack it
void BotFindAndAttackEnemy( bot_state_t *bs ) {
	// check for enemies
	if ( bs->enemy < 0 ) {
		BotFindEnemyMP( bs, -1, qfalse );
	}
	if ( bs->enemy >= 0 ) {
		aas_entityinfo_t entinfo;
		BotEntityInfo( bs->enemy, &entinfo );
		//if the enemy is dead
		if ( bs->enemydeath_time ) {
			if ( bs->enemydeath_time < trap_AAS_Time() - 0.3 ) {
				bs->enemydeath_time = 0;

				bs->enemy = -1;
			}
		} else {
			if ( EntityIsDead( &entinfo ) ) {
				bs->enemydeath_time = trap_AAS_Time();
			}
		}
		//
		if ( bs->enemy >= 0 ) {
			// attack and keep moving
			if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
				//choose the best weapon to fight with
				BotChooseWeapon( bs );

				BotAimAtEnemy( bs );

				//attack the enemy if possible
				if ( bs->weaponnum == bs->cur_ps.weapon ) {
					BotCheckAttack( bs );
				}
			} else {
				bs->enemy = -1;
			}
		}
	}
}

void BotUpdateViewAngles( bot_state_t *bs, bot_goal_t *goal, bot_moveresult_t moveresult ) {
	vec3_t target;
	vec3_t dir;

	if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
		//FIXME: look at cluster portals?
		if ( VectorLengthSquared( moveresult.movedir ) ) {
			vectoangles( moveresult.movedir, bs->ideal_viewangles );
		} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
		} else if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
}

// Gorodn: set whether a bot is a POW or not
void BotSetPOW( int entityNum, qboolean isPOW ) {
	botstates[entityNum].isPOW = isPOW;
}

/*
==================
BotBestTargetWeapon
==================
*/
int BotBestTargetWeapon( bot_state_t *bs, int targetNum ) {
	int bestWeapon, i, *ammo, validWeapons[2];
	float wantScale, bestWantScale, dist;
	gentity_t *ent = &g_entities[targetNum];

	memset( validWeapons, 0, sizeof( validWeapons ) );

	// this function currently only supports the following types of target entities
	if ( ent->s.eType == ET_MOVER ) {
		if ( ent->health > 0 ) {
			// explosive weapons are always valid
			COM_BitSet( validWeapons, WP_PANZERFAUST );
			COM_BitSet( validWeapons, WP_GRENADE_LAUNCHER );
			COM_BitSet( validWeapons, WP_GRENADE_PINEAPPLE );
			COM_BitSet( validWeapons, WP_SMOKE_MARKER );

			if ( bs->sess.playerType == PC_FIELDOPS ) {
				COM_BitSet( validWeapons, WP_BINOCULARS );
			}

			COM_BitSet( validWeapons, WP_MORTAR );
			COM_BitSet( validWeapons, WP_GPG40 );
			COM_BitSet( validWeapons, WP_M7 );

			if ( !( ent->spawnflags & 4 ) ) {   // allow other weapons
				// use any of these
				COM_BitSet( validWeapons, WP_MP40 );
				COM_BitSet( validWeapons, WP_THOMPSON );
				COM_BitSet( validWeapons, WP_KAR98 );
				COM_BitSet( validWeapons, WP_CARBINE );
				COM_BitSet( validWeapons, WP_MOBILE_MG42 );
				COM_BitSet( validWeapons, WP_K43 );
				COM_BitSet( validWeapons, WP_FG42 );
			}
		}
	} else if ( ent->s.eType == ET_CONSTRUCTIBLE ) {
		if ( ent->health > 0 ) {
			if ( ent->spawnflags & 16 ) {
				// explosive
				COM_BitSet( validWeapons, WP_PANZERFAUST );
				COM_BitSet( validWeapons, WP_GRENADE_LAUNCHER );
				COM_BitSet( validWeapons, WP_GRENADE_PINEAPPLE );
				COM_BitSet( validWeapons, WP_SMOKE_MARKER );

				if ( bs->sess.playerType == PC_FIELDOPS ) {
					COM_BitSet( validWeapons, WP_BINOCULARS );
				}

				COM_BitSet( validWeapons, WP_MORTAR );
				COM_BitSet( validWeapons, WP_GPG40 );
				COM_BitSet( validWeapons, WP_M7 );
			}
		}
	}

	// fast out, if we simply have none of these weapons
	if ( !( validWeapons[0] & bs->cur_ps.weapons[0] ) && !( validWeapons[1] & bs->cur_ps.weapons[0] ) ) {
		return WP_NONE;
	}

	ammo = bs->cur_ps.ammo;
	bestWantScale = 0.0;
	bestWeapon = WP_NONE;       // if nothing it found, return WP_NONE

	dist = VectorDistanceSquared( bs->origin, BotGetOrigin( ent->s.number ) );

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ ) {
		if ( COM_BitCheck( bs->cur_ps.weapons, i ) && COM_BitCheck( validWeapons, i ) ) {
			float range;

			// check that our ammo is enough
			if ( !BotGotEnoughAmmoForWeapon( bs, i ) ) {
				continue;
			}

			// within range?
			range = BotWeaponRange( bs, i ) /*+ 512*/;
			if ( SQR( range ) < dist ) {
				continue;
			}

			// get the wantScale for this weapon given the current circumstances (0.0 - 1.0)
			wantScale = BotWeaponWantScale( bs, i );
			if ( wantScale >= bestWantScale ) {
				bestWeapon = i;
				bestWantScale = wantScale;
			}
		}
	}

	return bestWeapon;
}

/*
====================
BotGetVisibleDamagableScriptMover
====================
*/
gentity_t *BotGetVisibleDamagableScriptMover( bot_state_t *bs ) {
	gentity_t *trav;
	int i, wpn;

	for ( i = MAX_CLIENTS, trav = g_entities + MAX_CLIENTS; i < level.num_entities; i++, trav++ ) {
		if ( !trav->inuse ) {
			continue;
		}

		if ( trav->s.eType != ET_MOVER && trav->s.eType != ET_CONSTRUCTIBLE ) {
			continue;
		}

		// is it disabled?
		if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
			continue;
		}

		// is it damagable?
		if ( trav->health <= 0 ) {
			continue;
		}

		if ( trav->s.eType == ET_MOVER ) {
			// is it an enemy item?
			if ( bs->sess.sessionTeam == TEAM_ALLIES && ( trav->spawnflags & 32 ) ) {
				continue;   // it's ours
			}
			if ( bs->sess.sessionTeam == TEAM_AXIS   && ( trav->spawnflags & 64 ) ) {
				continue;   // it's ours
			}
		} else {
			if ( !( trav->spawnflags & 16 ) ) {
				continue;
			}
			// is it an enemy item?
			if ( bs->sess.sessionTeam == TEAM_ALLIES && ( trav->spawnflags & 8 ) ) {
				continue;   // it's ours
			}
			if ( bs->sess.sessionTeam == TEAM_AXIS   && ( trav->spawnflags & 4 ) ) {
				continue;   // it's ours
			}
		}

		// do we have a weapon that could hurt it?
		if ( ( wpn = BotBestTargetWeapon( bs, i ) ) == WP_NONE ) {
			continue;
		}

		if ( !BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, i, NULL ) ) {
			continue;
		}

/*		// it's an enemy mover, can we see it?
		if (!trap_InPVS( bs->eye, BotGetOrigin( i ) )) {
			continue;
		}

		trap_Trace( &tr, bs->eye, vec3_origin, vec3_origin, BotGetOrigin( i ), -1, MASK_PLAYERSOLID & ~(CONTENTS_BODY) );
		if (tr.entityNum != i) {
			continue;
		}*/

		//
		// we found one
		return trav;
	}

	return NULL;
}

/*
================
BotCountLandMines
================
*/
void BotCountLandMines( void ) {
	gentity_t *trav, *mine;
	vec3_t org;
	static int lasttime;

	// only check every second
	if ( lasttime && lasttime < level.time && lasttime > level.time - 1000 ) {
		return;
	}
	lasttime = level.time;

	// reset counts
	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_LANDMINE_AREA ) ) ) {
		trav->count2 = 0;
		VectorClear( trav->pos3 );
	}

	mine = g_entities + level.maxclients;
	while ( ( mine = G_FindLandmine( mine ) ) ) {
		// doesn't matter if it's not armed, so that we dont drop too many landmines at once

		VectorCopy( mine->r.currentOrigin, org );
		org[2] += 16;

		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_LANDMINE_AREA ) ) ) {
			// are we within range?
			if ( PointInBounds( org, trav->r.absmin, trav->r.absmax ) ) {
				trav->count2++;
				// add this position to the average point
				VectorAdd( trav->r.currentOrigin, trav->pos3, trav->pos3 );
				// dont break here or else if we have overlapping areas, all mines in the overlapping section will only count to the first area
			}
		}
	}

	// finalize average points
	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_LANDMINE_AREA ) ) ) {
		if ( trav->count2 ) {
			VectorSubtract( trav->pos3, BotGetOrigin( trav->s.number ), trav->pos3 );
			VectorScale( trav->pos3, -1.0f / trav->count2, trav->pos3 );
		}
	}
}

// TAT 1/6/2003 - Bot picks up a new weapon
void BotPickupWeapon( int client, int weaponnum, qboolean alreadyHave ) {
	// if we didn't have any weapon before, we want to use this one
	bot_state_t *bs = &botstates[client];
	gentity_t *player;
	int i;

	if ( !bs->inuse ) {
		return;
	}

	if ( !alreadyHave && ( bs->weaponnum == WP_NONE ) ) {
		bs->weaponnum = weaponnum;
	}

	// force an update of our weapon
	BotChooseWeapon( bs );

	// make sure we tell all the clients that we have a weapon
	for ( i = 0; i < level.numConnectedClients; i++ )
	{
		// send the noweapon command with who as the 1st param, and 0 meaning we don't have no weapon
		player = g_entities + level.sortedClients[i];
		if ( player->inuse && player->client->sess.sessionTeam == bs->mpTeam ) {
			trap_SendServerCommand( player->s.number, va( "nwp %i 0", bs->client ) );
		}
	}

}

/*
==================
BotEntityWithinView
==================
*/
qboolean BotEntityWithinView( bot_state_t *bs, int viewEnt ) {
	vec3_t dir, ang;
	pmoveExt_t *pmExt;
	float arcMin, arcMax, arcDiff, yawDiff, pitchDiff;
	float pitchMax = 40.f;
	//
	if ( viewEnt >= level.maxclients ) {
		return qfalse;
	}
	if ( level.clients[viewEnt].pers.connected != CON_CONNECTED ) {
		return qfalse;
	}
	//
	VectorSubtract( BotGetOrigin( viewEnt ), bs->origin, dir );
	VectorNormalize( dir );
	vectoangles( dir, ang );
	//
	pmExt = &( level.clients[bs->client].pmext );
	//
	if ( BG_PlayerMounted( bs->cur_ps.eFlags ) ) {

		// limit harc and varc

		// pitch (varc)
		arcMax = pmExt->varc;
		if ( bs->cur_ps.eFlags & EF_AAGUN_ACTIVE ) {
			arcMin = 0;
		} else if ( bs->cur_ps.eFlags & EF_MOUNTEDTANK ) {
			// FIXME: fix this at allow min angle clamp...
			arcMin = 20;
			arcMax = 50;
		} else {
			arcMin = pmExt->varc / 2;
		}
		arcDiff = AngleNormalize180( ang[PITCH] - pmExt->centerangles[PITCH] );

		if ( arcDiff > arcMin ) {
			return qfalse;
		} else if ( arcDiff < -arcMax ) {
			return qfalse;
		}

		if ( !( bs->cur_ps.eFlags & EF_MOUNTEDTANK ) ) {
			// yaw (harc)
			arcMin = arcMax = pmExt->harc;
			arcDiff = AngleNormalize180( ang[YAW] - pmExt->centerangles[YAW] );

			if ( arcDiff > arcMin ) {
				return qfalse;
			} else if ( arcDiff < -arcMax ) {
				return qfalse;
			}
		}
	} else if ( bs->cur_ps.weapon == WP_MORTAR_SET ) {
		// yaw
		yawDiff = ang[YAW] - pmExt->mountedWeaponAngles[YAW];

		if ( yawDiff > 180 ) {
			yawDiff -= 360;
		} else if ( yawDiff < -180 ) {
			yawDiff += 360;
		}

		if ( yawDiff > 30 ) {
			return qfalse;
		} else if ( yawDiff < -30 ) {
			return qfalse;
		}

		// pitch
		pitchDiff = ang[PITCH] - pmExt->mountedWeaponAngles[PITCH];

		if ( pitchDiff > 180 ) {
			pitchDiff -= 360;
		} else if ( pitchDiff < -180 ) {
			pitchDiff += 360;
		}

		if ( pitchDiff > ( pitchMax - 10.f ) ) {
			return qfalse;
		} else if ( pitchDiff < -( pitchMax ) ) {
			return qfalse;
		}
	} else if ( bs->cur_ps.eFlags & EF_PRONE ) {

		// Check if we are allowed to rotate to there
		if ( bs->cur_ps.weapon == WP_MOBILE_MG42_SET ) {
			pitchMax = 20.f;

			// yaw
			yawDiff = ang[YAW] - pmExt->mountedWeaponAngles[YAW];

			if ( yawDiff > 180 ) {
				yawDiff -= 360;
			} else if ( yawDiff < -180 ) {
				yawDiff += 360;
			}

			if ( yawDiff > 20 ) {
				return qfalse;
			} else if ( yawDiff < -20 ) {
				return qfalse;
			}
		}

		// pitch
		pitchDiff = ang[PITCH] - pmExt->mountedWeaponAngles[PITCH];

		if ( pitchDiff > 180 ) {
			pitchDiff -= 360;
		} else if ( pitchDiff < -180 ) {
			pitchDiff += 360;
		}

		if ( pitchDiff > pitchMax ) {
			return qfalse;
		} else if ( pitchDiff < -pitchMax ) {
			return qfalse;
		}
	}

	return qtrue;
}
