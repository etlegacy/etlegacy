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
 * name:		ai_main.c
 *
 * desc:		Wolf bot AI
 *
 *
 *****************************************************************************/

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_cmd.h"
#include "ai_team.h"
#include "ai_distances.h"
//
#include "chars.h"
#include "inv.h"
#include "syn.h"

#ifndef MAX_PATH // LBO 1/26/05
#define MAX_PATH        144
#endif

//bot states
bot_state_t botstates[MAX_CLIENTS];
//number of bots
int ai_numbots;
//time to do a regular update
float regularupdate_time;
//
vmCvar_t bot_thinktime;
vmCvar_t bot_verbose;
vmCvar_t memorydump;
vmCvar_t bot_profile;
vmCvar_t bot_findgoal;

/*
==================
BotAI_Print
==================
*/
void QDECL BotAI_Print( int type, char *fmt, ... ) {
	char str[2048];
	va_list ap;

	va_start( ap, fmt );
	Q_vsnprintf( str, sizeof( str ), fmt, ap );
	va_end( ap );

	switch ( type ) {
	case PRT_MESSAGE: {
		trap_Cvar_Update( &bot_verbose );
		if ( bot_verbose.integer == 1 ) {
			G_Printf( "%s", str );
		}
		break;
	}
	case PRT_WARNING: {
		G_Printf( S_COLOR_YELLOW "Warning: %s", str );
		break;
	}
	case PRT_ERROR: {
		G_Printf( S_COLOR_RED "Error: %s", str );
		break;
	}
	case PRT_FATAL: {
		G_Printf( S_COLOR_RED "Fatal: %s", str );
		break;
	}
	case PRT_EXIT: {
		G_Error( S_COLOR_RED "Exit: %s", str );
		break;
	}
	default: {
		G_Printf( "unknown print type\n" );
		break;
	}
	}
}

/*
==================
BotAI_Trace
==================
*/
void BotAI_Trace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask ) {
	trace_t trace;

	trap_Trace( &trace, start, mins, maxs, end, passent, contentmask );
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy( trace.endpos, bsptrace->endpos );
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy( trace.plane.normal, bsptrace->plane.normal );
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t* ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof( playerState_t ) );
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t   *ent;

	ent = BotGetEntity( entityNum );
	memset( state, 0, sizeof( entityState_t ) );
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->r.linked ) {
		return qfalse;
	}
	if ( ent->r.svFlags & SVF_NOCLIENT ) {
		return qfalse;
	}
	memcpy( state, &ent->s, sizeof( entityState_t ) );
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int entNum;

	entNum = trap_BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset( state, 0, sizeof( entityState_t ) );
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

/*
==================
BotAI_BotInitialChat
==================
*/
void QDECL BotAI_BotInitialChat( bot_state_t *bs, char *type, ... ) {
	int i, mcontext;
	va_list ap;
	char    *p;
	char    *vars[MAX_MATCHVARIABLES];

// RF, disabled
	return;

	memset( vars, 0, sizeof( vars ) );
	va_start( ap, type );
	p = va_arg( ap, char * );
	for ( i = 0; i < MAX_MATCHVARIABLES; i++ ) {
		if ( !p ) {
			break;
		}
		vars[i] = p;
		p = va_arg( ap, char * );
	}
	va_end( ap );

	mcontext = CONTEXT_NORMAL | CONTEXT_NEARBYITEM | CONTEXT_NAMES;

	trap_BotInitialChat( bs->cs, type, mcontext, vars[0], vars[1], vars[2], vars[3], vars[4], vars[5], vars[6], vars[7] );
}

/*
==============
BotInterbreeding
==============
*/
void BotInterbreeding( void ) {
	float ranks[MAX_CLIENTS];
	int parent1, parent2, child;
	int i, j;

	// get rankings for all the bots
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];
		if ( botstates[j].inuse ) {
			ranks[j] = botstates[i].num_kills * 2 - botstates[j].num_deaths;
		} else {
			ranks[j] = -1;
		}
	}

	if ( trap_GeneticParentsAndChildSelection( MAX_CLIENTS, ranks, &parent1, &parent2, &child ) ) {
		trap_BotInterbreedGoalFuzzyLogic( botstates[parent1].gs, botstates[parent2].gs, botstates[child].gs );
		trap_BotMutateGoalFuzzyLogic( botstates[child].gs, 1 );
	}
	// reset the kills and deaths
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];
		if ( botstates[j].inuse ) {
			botstates[j].num_kills = 0;
			botstates[j].num_deaths = 0;
		}
	}
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo( int entnum, aas_entityinfo_t *info ) {
	trap_AAS_EntityInfo( entnum, info );
}

/*
==============
BotAI_GetNumBots
==============
*/
int BotAI_GetNumBots( void ) {
	return ai_numbots;
}

/*
==============
BotAI_SetNumBots
==============
*/
void BotAI_SetNumBots( int numbots ) {
	ai_numbots = numbots;
}


/*
==============
AngleDifference
==============
*/
float AngleDifference( float ang1, float ang2 ) {
	float diff;

	diff = ang1 - ang2;
	if ( ang1 > ang2 ) {
		if ( diff > 180.0 ) {
			diff -= 360.0;
		}
	} else {
		if ( diff < -180.0 ) {
			diff += 360.0;
		}
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle( float angle, float ideal_angle, float speed ) {
	float move;

	angle = AngleMod( angle );
	ideal_angle = AngleMod( ideal_angle );
	if ( angle == ideal_angle ) {
		return angle;
	}
	move = ideal_angle - angle;
	if ( ideal_angle > angle ) {
		if ( move > 180.0 ) {
			move -= 360.0;
		}
	} else {
		if ( move < -180.0 ) {
			move += 360.0;
		}
	}
	if ( move > 0 ) {
		if ( move > speed ) {
			move = speed;
		}
	} else {
		if ( move < -speed ) {
			move = -speed;
		}
	}
	return AngleMod( angle + move );
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles( bot_state_t *bs, float thinktime ) {
	float diff, factor, maxchange, anglespeed;
	int i;

	if ( bs->ideal_viewangles[PITCH] > 180 ) {
		bs->ideal_viewangles[PITCH] -= 360;
	}
	//
	if ( bs->enemy >= 0 ) {
		factor = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01, 1 );
		maxchange = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1, 1800 );
	} else {
		factor = 0.15;
		maxchange = 240;
	}
	maxchange *= thinktime;
	for ( i = 0; i < 2; i++ ) {
		diff = fabs( AngleDifference( bs->viewangles[i], bs->ideal_viewangles[i] ) );
		anglespeed = diff * factor;
		if ( anglespeed > maxchange ) {
			anglespeed = maxchange;
		}
		bs->viewangles[i] = BotChangeViewAngle( bs->viewangles[i],
												bs->ideal_viewangles[i], anglespeed );
		//BotAI_Print(PRT_MESSAGE, "ideal_angles %f %f\n", bs->ideal_viewangles[0], bs->ideal_viewangles[1], bs->ideal_viewangles[2]);`
		//bs->viewangles[i] = bs->ideal_viewangles[i];
	}
	if ( bs->viewangles[PITCH] > 180 ) {
		bs->viewangles[PITCH] -= 360;
	}
	//elementary action: view
	trap_EA_View( bs->client, bs->viewangles );
}


/*
==============
BotSpeedBonus
==============
*/
void BotSpeedBonus( int clientNum ) {
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand( bot_state_t *bs, bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time ) {
	vec3_t angles, forward, right;
	short temp;
	int j;
	int value = 0;

	//clear the whole structure
	memset( ucmd, 0, sizeof( usercmd_t ) );
	//
	//Com_Printf("dir = %f %f %f speed = %f\n", bi->dir[0], bi->dir[1], bi->dir[2], bi->speed);
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if ( bi->actionflags & ACTION_DELAYEDJUMP ) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	//if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if ( bi->actionflags & ACTION_ATTACK ) {
		ucmd->buttons |= BUTTON_ATTACK;
	}
	if ( bi->actionflags & ACTION_TALK ) {
		ucmd->buttons |= BUTTON_TALK;
	}
	if ( bi->actionflags & ACTION_GESTURE ) {
		ucmd->buttons |= BUTTON_GESTURE;
	}
	if ( bi->actionflags & ACTION_USE ) {
		ucmd->buttons |= BUTTON_ACTIVATE;
	}
	if ( bi->actionflags & ACTION_WALK ) {
		ucmd->buttons |= BUTTON_WALKING;
	}
	if ( bi->actionflags & ACTION_RELOAD ) {
		ucmd->wbuttons |= WBUTTON_RELOAD;
	}

	// START	xkan, 9/16/2002
	// see if the bots should prone
	// there are 3 cases here (from highest priority to lowest):
	//   1. the script set the bot to prone
	//   2. the player has ordered the bot to prone
	//   3. the player is proning, and the bot is following his lead
	if ( ( bi->actionflags & ACTION_PRONE || bs->script.flags & BSFL_PRONE )
		 && !( g_entities[bs->client].client->ps.eFlags & EF_PRONE ) ) {
		// xkan, 10/23/2002 - only set WBUTTON_PRONE if we are not proning already
		// 'cause setting it again will cancel the proning
		ucmd->wbuttons |= WBUTTON_PRONE;
	} else if ( !( bi->actionflags & ACTION_PRONE )
				&& !( bs->script.flags & BSFL_PRONE )
				&& ( g_entities[bs->client].client->ps.eFlags & EF_PRONE ) ) {
		// now see if we should unprone, setting WBUTTON_PRONE again while
		// we are already proning will unprone - xkan, 10/23/2002
		ucmd->wbuttons |= WBUTTON_PRONE;
	}
	// END		xkan, 9/16/2002

	// Start - TAT 9/18/2002
	// Bots aren't zooming when they start using binoculars
	//		so if our new weapon is binoculars, do a start zoom
	if ( ucmd->weapon != bi->weapon && bi->weapon == WP_BINOCULARS ) {
		ucmd->wbuttons |= WBUTTON_ZOOM;
	}
	// End - TAT 9/18/2002

/*	// Test having the bots lean left
	if (bs->leanleft)
	{
		// Force them to lean left
		ucmd->wbuttons |= WBUTTON_LEANLEFT;

	} // if (bs->leanleft)...
	*/

	ucmd->weapon = bi->weapon;
	// set the team elements
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT( bi->viewangles[PITCH] );
	ucmd->angles[YAW] = ANGLE2SHORT( bi->viewangles[YAW] );
	ucmd->angles[ROLL] = ANGLE2SHORT( bi->viewangles[ROLL] );
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		temp = ucmd->angles[j] - delta_angles[j];
		/*NOTE: disabled because temp should be mod first
		if ( j == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) temp = 16000;
			else if ( temp < -16000 ) temp = -16000;
		}
		*/
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if ( bi->dir[2] ) {
		angles[PITCH] = bi->viewangles[PITCH];
	} else { angles[PITCH] = 0;}
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors( angles, forward, right, NULL );
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	// SP: adjust speed according to current animation
	/*
	if (BotSinglePlayer() && !(bi->actionflags & ACTION_CROUCH)) {
		animation_t		*anim;
		int animIndex;
		bg_playerclass_t *classInfo;
		//
		classInfo = BG_GetPlayerClassInfo( bs->sess.sessionTeam, bs->sess.playerType );
		if (classInfo) {
			animIndex = BG_GetAnimScriptAnimation( bs->client, classInfo, (ucmd->buttons & BUTTON_WALKING) ? ANIM_MT_WALK : ANIM_MT_RUN );
			if (animIndex >= 0) {
				anim = BG_GetAnimationForIndex( classInfo, animIndex );
				if (anim->moveSpeed > 0) {
					bi->speed *= (float)anim->moveSpeed / g_speed.value;
					if (bi->speed > 1.0f) bi->speed = 1.0f;
				}
			}
		}
	}
	*/
//Com_Printf( "BotInputToUserCommand: bi->dir %s\n", vtosf(bi->dir) );
	//set the view independent movement
	ucmd->forwardmove = DotProduct( forward, bi->dir ) * bi->speed;
	ucmd->rightmove = DotProduct( right, bi->dir ) * bi->speed;
	ucmd->upmove = abs( forward[2] ) * bi->dir[2] * bi->speed;
	//normal keyboard movement
	if ( bi->actionflags & ACTION_MOVEFORWARD ) {
		ucmd->forwardmove += 127;
	}
	if ( bi->actionflags & ACTION_MOVEBACK ) {
		ucmd->forwardmove -= 127;
	}
	if ( bi->actionflags & ACTION_MOVELEFT ) {
		ucmd->rightmove -= 127;
	}
	if ( bi->actionflags & ACTION_MOVERIGHT ) {
		ucmd->rightmove += 127;
	}
	//jump/moveup
	if ( bi->actionflags & ACTION_JUMP ) {
		ucmd->upmove += 127;
	}
	//crouch/movedown
	if ( ( bi->actionflags & ACTION_CROUCH )
		 // START	xkan, 8/23/2002
		 // if the script says crouch, then crouch.
		 || ( bs->script.flags & BSFL_CROUCH )
		 // END		xkan, 8/23/2002
		 ) {
		ucmd->upmove -= 127;
	}


	//
	//in single player, restrict movement speeds
	value = -1;
	if ( BotSinglePlayer() || BotCoop() ) {
		// Axis bots should be slow
		if ( bs->sess.sessionTeam == TEAM_AXIS ) {
			if ( ucmd->buttons & BUTTON_WALKING ) {
				if ( ucmd->forwardmove || ucmd->rightmove ) {
					g_entities[bs->client].client->ps.friction = 0.1;
				} else {
					g_entities[bs->client].client->ps.friction = 1.0;
				}
				value = 24; // make sure feet dont slide in single player
			} else {
				g_entities[bs->client].client->ps.friction = 1.0;
				value = 80;
			}
		} // if (bs->sess.sessionTeam == TEAM_AXIS)) ...
		  // Allied bots should keep up with you
		else
		{
			if ( ucmd->buttons & BUTTON_WALKING ) {
				if ( ucmd->forwardmove || ucmd->rightmove ) {
					g_entities[bs->client].client->ps.friction = 0.1;
				} else {
					g_entities[bs->client].client->ps.friction = 1.0;
				}
				value = 64; // as fast as the player
			} else {
				g_entities[bs->client].client->ps.friction = 1.0;
				value = 127; // as fast as the player
			} // else...
		} // if (BotSinglePlayer())...

	} else {
		if ( ucmd->buttons & BUTTON_WALKING ) {
			value = 64; // keep in line with player movements
		}
	}
	//
	//if (bs->leader == 0 && bs->leader_tagent == 0)
	//	G_Printf( "%s: %i, %i\n", g_entities[bs->client].client->pers.netname, ucmd->forwardmove, ucmd->rightmove );
	if ( value > 0 ) {
		if ( ucmd->forwardmove > value ) {
			ucmd->rightmove = (int)( (float)ucmd->rightmove * ( (float)value / (float)ucmd->forwardmove ) );
			ucmd->forwardmove = value;
		} else if ( ucmd->forwardmove < -value )     {
			ucmd->rightmove = (int)( (float)ucmd->rightmove * ( (float)-value / (float)ucmd->forwardmove ) );
			ucmd->forwardmove = -value;
		}
		//
		if ( ucmd->rightmove > value ) {
			ucmd->forwardmove = (int)( (float)ucmd->forwardmove * ( (float)value / (float)ucmd->rightmove ) );
			ucmd->rightmove = value;
		} else if ( ucmd->rightmove < -value )     {
			ucmd->forwardmove = (int)( (float)ucmd->forwardmove * ( (float)-value / (float)ucmd->rightmove ) );
			ucmd->rightmove = -value;
		}
	}
	//Com_Printf("forward = %d right = %d up = %d\n", ucmd.forwardmove, ucmd.rightmove, ucmd.upmove);
	//Com_Printf("ucmd->serverTime = %d\n", ucmd->serverTime);
}

/*
==============
BotUpdateInput
==============
*/
void BotUpdateInput( bot_state_t *bs, int time ) {
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] + SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//
	BotChangeViewAngles( bs, (float) time / 1000 );
	trap_EA_GetInput( bs->client, (float) time / 1000, &bi );
	//respawn hack
	if ( bi.actionflags & ACTION_RESPAWN ) {
		if ( bs->lastucmd.buttons & BUTTON_ATTACK ) {
			bi.actionflags &= ~( ACTION_RESPAWN | ACTION_ATTACK );
		}
	}
	//
	BotInputToUserCommand( bs, &bi, &bs->lastucmd, bs->cur_ps.delta_angles, time );
	// sprint always if we have no enemy
	if ( !( bs->lastucmd.buttons & BUTTON_WALKING ) ) {
		if ( bs->flags & BFL_SPRINT ) {
			bs->lastucmd.buttons |= BUTTON_SPRINT;
			if ( level.clients[bs->client].pmext.sprintTime < 200 ) {
				bs->flags &= ~BFL_SPRINT;
			}
		} else {
			if ( level.clients[bs->client].pmext.sprintTime > 7000 ) {
				// if we are trying to escape an enemy, or chasing them
				if ( bs->enemy > -1 && bs->last_fire < level.time - 1000 ) {
					bs->flags |= BFL_SPRINT;
				}
				// if we are defending someone, try to stay close
				if ( bs->target_goal.entitynum == bs->leader && bs->leader >= 0 ) {
					bs->flags |= BFL_SPRINT;
				}
			}
		}
	}
	// if someone is trying to help us, walk
	if ( g_entities[bs->client].missionLevel > level.time + 200 ) {
		bs->lastucmd.buttons |= BUTTON_WALKING;
	}
	bs->lastucmd.serverTime = time;
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] - SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate( void ) {
	if ( regularupdate_time < trap_AAS_Time() ) {
		trap_BotUpdateEntityItems();
		regularupdate_time = trap_AAS_Time() + 1;
	}
}

/*
================
BotTravelFlagsForClient

  Only returns special flags that are absolutely necessary for this level
================
*/
int BotTravelFlagsForClient( int client ) {
	int tfl;
	gclient_t *cl = &level.clients[client];
	//
	if ( !cl || !cl->pers.connected == CON_CONNECTED ) {
		return 0;
	}
	//
	tfl = TFL_DEFAULT;
	//
	if ( cl->sess.sessionTeam == TEAM_ALLIES ) {
		tfl |= TFL_TEAM_ALLIES;
	} else if ( cl->sess.sessionTeam == TEAM_AXIS ) {
		tfl |= TFL_TEAM_AXIS;
	}
	//
	// if there are no team doors in the level, then always use defaults
	if ( !level.doorAllowTeams ) {
		return tfl;
	}
	//
	if ( ( level.doorAllowTeams & TEAM_ALLIES ) && cl->sess.sessionTeam == TEAM_ALLIES ) {
		tfl |= TFL_TEAM_ALLIES;
		if ( ( level.doorAllowTeams & ALLOW_DISGUISED_CVOPS ) && cl->ps.powerups[PW_OPS_DISGUISED] ) {
			tfl |= TFL_TEAM_ALLIES_DISGUISED;
		}
	}
	if ( ( level.doorAllowTeams & TEAM_AXIS ) && cl->sess.sessionTeam == TEAM_AXIS ) {
		tfl |= TFL_TEAM_AXIS;
		if ( ( level.doorAllowTeams & ALLOW_DISGUISED_CVOPS ) && cl->ps.powerups[PW_OPS_DISGUISED] ) {
			tfl |= TFL_TEAM_AXIS_DISGUISED;
		}
	}
	//
	return tfl;
}

/*
==============
BotAI
==============
*/
int BotAI( int client, float thinktime ) {
	bot_state_t *bs;
	char buf[1024];
	int j, areanum;

	trap_EA_ResetInput( client, NULL );

	bs = &botstates[client];
	if ( !bs->inuse ) {
		BotAI_Print( PRT_FATAL, "client %d hasn't been setup\n", client );
		return BLERR_AICLIENTNOTSETUP;
	}

	//retrieve the current client state
	BotAI_GetClientState( client, &bs->cur_ps );

	// Mad Doctor I, 9/19/2002
	// If we haven't yet set up our initial desired weapon, do so now.
	if ( bs->weaponnum == -1 ) {
		// We at first want to use the weapon we spawned with
		bs->weaponnum = bs->cur_ps.weapon;

	} // if (bs->weaponnum == -1)...

	//get the session data
	bs->sess = level.clients[client].sess;
	// set the team
	bs->mpTeam = bs->sess.sessionTeam;

	//retrieve any waiting console messages
	// Gordon: you MUST do this to acknowledge any commands sent
	while ( trap_BotGetServerCommand( client, buf, sizeof( buf ) ) ) ;

	//add the delta angles to the bot's current view angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] + SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//increase the local time of the bot

	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy( bs->cur_ps.origin, bs->origin );
	//eye coordinates of the bot
	VectorCopy( bs->cur_ps.origin, bs->eye );
	bs->eye[2] += bs->cur_ps.viewheight;

	//set the travelflags for this bot
	bs->tfl = BotTravelFlagsForClient( bs->client );

	//get the area the bot is in
	areanum = BotPointAreaNum( bs->client, bs->origin );
	if ( areanum ) {
		bs->areanum = areanum;
	}

	if ( bot_profile.integer == 3 ) {
		int t = trap_Milliseconds();

		// the real AI
		BotDeathmatchAI( bs, thinktime );

		t = trap_Milliseconds() - t;

		G_Printf( "Time for BotDeathmatchAI: %s: %i\n", level.clients[bs->client].pers.netname, t );
	} else {
		BotDeathmatchAI( bs, thinktime );
	}

	level.clients[bs->client].sess.latchPlayerType = bs->mpClass;   // FIXME: better place for this?

	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] - SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}

	//everything was ok
	return BLERR_NOERROR;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink( void ) {
	int i, j, botnum = 0;
	int numbots = BotAI_GetNumBots();

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];
		if ( !botstates[j].inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[j].botthink_residual = bot_thinktime.integer * botnum / numbots;
		botnum++;
	}
}

/*
==============
BotGetInitialAttributes
==============
*/
void BotGetInitialAttributes( bot_state_t *bs ) {
	if ( G_IsSinglePlayerGame() ) {
		// Default for Allies
		if ( bs->mpTeam == TEAM_ALLIES ) {
			bs->attribs[BOT_REACTION_TIME] = 0.5f;
			bs->attribs[BOT_AIM_ACCURACY] = 0.35f;
			bs->attribs[BOT_WIMP_FACTOR] = 0.25f;

		} // if (bs->mp_team == TEAM_ALLIES)...
		  // Make Nazis wimpier
		else
		{
			bs->attribs[BOT_REACTION_TIME] = 1.5f;
			bs->attribs[BOT_AIM_ACCURACY] = 0.2f;
			bs->attribs[BOT_WIMP_FACTOR] = 0.25f;

		} // else...

	} // if (G_IsSinglePlayerGame())...
	else
	{



		bs->attribs[BOT_REACTION_TIME] = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1 );
		bs->attribs[BOT_AIM_ACCURACY] = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1 );





		// default to not wimpy at all
		bs->attribs[BOT_WIMP_FACTOR] = 0.01f;

	} // else...


}


enum botDefaultKeyEnum
{
	e_BOT_NAME,
	e_BOT_REACTION_TIME,
	e_BOT_AIM_ACCURACY,
	e_BOT_WIMP_FACTOR,
	e_SETSPEEDCOEFFICIENT,
	e_SETFIRERATE,
	e_SETFIRECYCLETIME,
	e_SETFIELDOFVIEW,
	e_SETHEARINGRANGE,
	e_SETCLOSEHEARINGRANGE,
	e_SETVISIONRANGE,
	e_SETFARSEEINGRANGE,
	e_SETMOVEMENTAUTONOMY,
	e_SETWEAPONAUTONOMY,
	MAX_BOT_DEFAULT_KEYS
};


const char *g_botDefaultKeys[] =
{
	"BOT",
	"BOT_REACTION_TIME",
	"BOT_AIM_ACCURACY",
	"BOT_WIMP_FACTOR",
	"SETSPEEDCOEFFICIENT",
	"SETFIRERATE",
	"SETFIRECYCLETIME",
	"SETFIELDOFVIEW",
	"SETHEARINGRANGE",
	"SETCLOSEHEARINGRANGE",
	"SETVISIONRANGE",
	"SETFARSEEINGRANGE",
	"SETMOVEMENTAUTONOMY",
	"SETWEAPONAUTONOMY"
};


// Globally store the bot defaults
BotDefaultAttributes_t g_botDefaultValues[MAX_BOT_DEFAULTS];

// Have the default attributes been loaded (only need to do this once!)
qboolean g_loadedDefaultBotAttributes = qfalse;

// How many bots' default values have we loaded?
int g_botDefaultValueCount = 0;

//
// ParseBotDefaultAttributes
//
// Description: Read in the default values for AI Characters
// Written: 1/11/2003
//
void ParseBotDefaultAttributes
(
	char *fileName
) {
	// Local Variables ////////////////////////////////////////////////////////

	// The text file containing our default bot attributes
	fileHandle_t botAttributeFile;

	// Have we found a new bot
	qboolean foundBot = qfalse;

	// How many fields have we read for the bot?
	int fieldCount = 0;

	// Index for keys
	int j;

	// file length
	int len;

	// file data
	char data[MAX_BOT_DEFAULT_FILE_SIZE];
	char **p, *ptr;
	char *token;

	// Level of autonomy setting
	int level;

	///////////////////////////////////////////////////////////////////////////

	// Bail if we've already loaded this
	if ( g_loadedDefaultBotAttributes ) {
		return;
	}

	// Open the text file containing the AI default attributes
	len = trap_FS_FOpenFile( fileName, &botAttributeFile, FS_READ );

	// empty file?
	if ( len <= 0 ) {
		return;
	}

	// too big file
	if ( len >= ( sizeof( data ) - 1 ) ) {
		Com_Printf( "File %s too long\n", fileName );
		return;
	}

	// read all the data into the buffer
	trap_FS_Read( data, len, botAttributeFile );
	data[len] = 0;
	trap_FS_FCloseFile( botAttributeFile );

	ptr = data;
	p = &ptr;


	while ( 1 )
	{
		// Get the next token
		token = COM_ParseExt( p, qtrue );

		// if no more tokens, we're done looping
		if ( !token || token[0] == 0 ) {
			break;
		}

		// Are we finished?
		if ( !Q_stricmp( token, "DONE" ) ) {
			// If we'd found a bot already, move on
			if ( foundBot ) {
				// Report an error if there were not enough fields
				if ( fieldCount < MAX_BOT_DEFAULT_KEYS ) {
					Com_Printf( "File %s has bot %s without enough fields\n",
								fileName,  g_botDefaultValues[g_botDefaultValueCount].m_botName );

				} // if (fieldCount < MAX_BOT_DEFAULT_KEYS)...

				// Go on to the next bot
				g_botDefaultValueCount++;

			} // if (foundBot)...

			break;

		} // if (!Q_stricmp(token, "DONE"))...


		// Check to see if this was a special element
		for ( j = e_BOT_NAME; j < MAX_BOT_DEFAULT_KEYS; j++ )
		{
			// Does this line match one of our key lines?
			if ( !Q_stricmp( token, g_botDefaultKeys[j] ) ) {
				// Read the data for this key line
				switch ( j )
				{
				case e_BOT_NAME:
					// If we'd found a bot already, move on
					if ( foundBot ) {
						// Report an error if there were not enough fields
						if ( fieldCount < MAX_BOT_DEFAULT_KEYS ) {
							Com_Printf( "File %s has bot %s without enough fields\n",
										fileName,  g_botDefaultValues[g_botDefaultValueCount].m_botName );

						} // if (fieldCount < MAX_BOT_DEFAULT_KEYS)...

						// Go on to the next bot
						g_botDefaultValueCount++;

					} // if (foundBot)...

					// Read the name of its target
					token = COM_ParseExt( p, qtrue );
					strcpy( g_botDefaultValues[g_botDefaultValueCount].m_botName, token );

					// We found a new bot
					foundBot = qtrue;

					// We have one of the fields (name) filled
					fieldCount = 1;

					break;
				case e_BOT_REACTION_TIME:
					// Read entity's name
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_reactionTime = atof( token );

					// We've found another field
					fieldCount++;

					break;
				case e_BOT_AIM_ACCURACY:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_aimAccuracy = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_BOT_WIMP_FACTOR:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_wimpFactor = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETSPEEDCOEFFICIENT:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_speedCoefficient = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETFIRERATE:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_fireRate = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETFIRECYCLETIME:
					// Read min fire rate
					token = COM_ParseExt( p, qtrue );

					g_botDefaultValues[g_botDefaultValueCount].m_minFireRateCycleTime =
						atoi( token );

					// Read max fire rate
					token = COM_ParseExt( p, qtrue );

					g_botDefaultValues[g_botDefaultValueCount].m_maxFireRateCycleTime =
						atoi( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETFIELDOFVIEW:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_scriptedFieldOfView = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETHEARINGRANGE:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_scriptedHearingRange = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETCLOSEHEARINGRANGE:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_closeHearingRange = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETVISIONRANGE:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_visionRange = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETFARSEEINGRANGE:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					g_botDefaultValues[g_botDefaultValueCount].m_farSeeingRange = atof( token );

					// We've found another field
					fieldCount++;

					break;

				case e_SETMOVEMENTAUTONOMY:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					level = BotWeaponAutonomyForString( token );
					g_botDefaultValues[g_botDefaultValueCount].m_movementAutonomy = level;

					// We've found another field
					fieldCount++;

					break;

				case e_SETWEAPONAUTONOMY:
					// Read entity's classname
					token = COM_ParseExt( p, qtrue );
					level = BotMovementAutonomyForString( token );
					g_botDefaultValues[g_botDefaultValueCount].m_weaponAutonomy = level;

					// We've found another field
					fieldCount++;

					break;

				};

				// We found a match, so don't keep checking
				break;

			} // if (!Q_stricmp(token, keys[j]))...

		} // for ( j = e_BOT_NAME; j < MAX_BOT_DEFAULT_KEYS; j++)...

	} // while (1)

	// We've loaded this file.  Don't do it again
	g_loadedDefaultBotAttributes = qtrue;
}
//
// ParseBotDefaultAttributes
//




//
// BotSetCharacterAttributes
//
// Description: Set attributes for specific characters in code as a default
// these can all be overriden by scripts
// Written: 1/11/2003
//
void BotSetCharacterAttributes
(
	bot_state_t *bs,

	// What default attributes should we use?
	BotDefaultAttributes_t *defaults
) {
	// Local Variables ////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	// What delay is there before we react to an enemy?
	bs->attribs[BOT_REACTION_TIME] = defaults->m_reactionTime;

	// How often do we hit our target?
	bs->attribs[BOT_AIM_ACCURACY] = defaults->m_aimAccuracy;

	// How likely are we to run back to the player
	bs->attribs[BOT_WIMP_FACTOR] = defaults->m_wimpFactor;

	// How fast do we move?
	bs->speedCoefficient = defaults->m_speedCoefficient;

	// What % of the time will we fire?
	bs->fireRate = defaults->m_fireRate;

	// Set the fire cycle length
	bs->minFireRateCycleTime = defaults->m_minFireRateCycleTime;
	bs->maxFireRateCycleTime = defaults->m_maxFireRateCycleTime;

	// What is our scripted FOV?
	bs->scriptedFieldOfView = defaults->m_scriptedFieldOfView;

	// What is our scripted hearing range?
	bs->scriptedHearingRange = defaults->m_scriptedHearingRange;

	// When an enemy is this close, we can hear them even outside our
	// field of view
	bs->closeHearingRange = defaults->m_closeHearingRange;

	// How far can this bot see?
	bs->visionRange = defaults->m_visionRange;

	// This range is range to which we can see enemies, even if they
	// are too far to attack
	bs->farSeeingRange = defaults->m_farSeeingRange;

	// How independent?
	bs->movementAutonomy = defaults->m_movementAutonomy;
	bs->script.movementAutonomy = defaults->m_movementAutonomy;
}
//
// BotSetCharacterAttributes
//




//
// BotSetUpCharacter
//
// Description: Determine which character this is and set the attributes
// Written: 1/11/2003
//
void BotSetUpCharacter
(
	bot_state_t *bs
) {
	// Local Variables ////////////////////////////////////////////////////////
	int i;
	///////////////////////////////////////////////////////////////////////////

	// Make sure we've parsed the default attributes file
	ParseBotDefaultAttributes( "botfiles\\botAttributes.bot" );

	// Loop through the loaded bot default attributes and find out if we've got
	// one of this character name.
	for ( i = 0; i < g_botDefaultValueCount; i++ )
	{
		// Is this the bot default to use?
		if ( !Q_stricmp( g_entities[bs->client].scriptName,
						 g_botDefaultValues[i].m_botName ) ) {
			// Use these defaults
			BotSetCharacterAttributes( bs, &( g_botDefaultValues[i] ) );

		} // if (!Q_stricmp(g_entities[bs->client].scriptName...

	} // for (i = 0; i < g_botDefaultValueCount; i++)...


}
//
// BotSetUpCharacter
//



/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient( int client, struct bot_settings_s *settings ) {
	char filename[MAX_PATH], name[MAX_PATH], gender[MAX_PATH];
	bot_state_t *bs;
	int errnum, i;

	bs = &botstates[client];
	if ( bs->inuse ) {
		BotAI_Print( PRT_FATAL, "client %d already setup\n", client );
		return qfalse;
	}

	if ( !trap_AAS_Initialized() ) {
		BotAI_Print( PRT_FATAL, "AAS not initialized\n" );
		//	return qfalse;
	}

	//load the bot character
	bs->character = trap_BotLoadCharacter( settings->characterfile, settings->skill );
	if ( !bs->character ) {
		BotAI_Print( PRT_FATAL, "couldn't load skill %d from %s\n", settings->skill, settings->characterfile );
		return qfalse;
	}
	//copy the settings
	memcpy( &bs->settings, settings, sizeof( bot_settings_t ) );
	//allocate a goal state
	bs->gs = trap_BotAllocGoalState( client );
	//load the item weights
	trap_Characteristic_String( bs->character, CHARACTERISTIC_ITEMWEIGHTS, filename, MAX_PATH );
	errnum = trap_BotLoadItemWeights( bs->gs, filename );
	if ( errnum != BLERR_NOERROR ) {
		trap_BotFreeGoalState( bs->gs );
		return qfalse;
	}
	//allocate a weapon state
	bs->ws = trap_BotAllocWeaponState();
	//load the weapon weights
	trap_Characteristic_String( bs->character, CHARACTERISTIC_WEAPONWEIGHTS, filename, MAX_PATH );
	errnum = trap_BotLoadWeaponWeights( bs->ws, filename );
	if ( errnum != BLERR_NOERROR ) {
		trap_BotFreeGoalState( bs->gs );
		trap_BotFreeWeaponState( bs->ws );
		return qfalse;
	}
	//allocate a chat state
	bs->cs = trap_BotAllocChatState();
	//load the chat file
	trap_Characteristic_String( bs->character, CHARACTERISTIC_CHAT_FILE, filename, MAX_PATH );
	trap_Characteristic_String( bs->character, CHARACTERISTIC_CHAT_NAME, name, MAX_PATH );
	errnum = trap_BotLoadChatFile( bs->cs, filename, name );
	if ( errnum != BLERR_NOERROR ) {
		trap_BotFreeChatState( bs->cs );
		trap_BotFreeGoalState( bs->gs );
		trap_BotFreeWeaponState( bs->ws );
		return qfalse;
	}
	//get the gender characteristic
	trap_Characteristic_String( bs->character, CHARACTERISTIC_GENDER, gender, MAX_PATH );
	//set the chat gender
	if ( *gender == 'f' || *gender == 'F' ) {
		trap_BotSetChatGender( bs->cs, CHAT_GENDERFEMALE );
	} else if ( *gender == 'm' || *gender == 'M' )  {
		trap_BotSetChatGender( bs->cs, CHAT_GENDERMALE );
	} else { trap_BotSetChatGender( bs->cs, CHAT_GENDERLESS );}

	bs->inuse = qtrue;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = 4;

	bs->ms = trap_BotAllocMoveState();
	bs->walker = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_WALKER, 0, 1 );
	bs->enemy = -1;
	bs->altenemy = -1;
	bs->leader = -1;
	bs->leader_tagent = -1;
	bs->returnToLeader = -1;
	bs->isBotSelectable = qtrue;
	// setup initial teamplay selections
	bs->mpTeam = ( !Q_stricmp( settings->team, "axis" ) ? TEAM_AXIS : TEAM_ALLIES );
	bs->mpClass = BotSuggestClass( bs, bs->mpTeam );
	bs->mpWeapon = BotSuggestWeapon( bs, bs->mpTeam );
	level.clients[bs->client].sess.latchPlayerType = bs->mpClass;   // FIXME: better place for this?
	level.clients[bs->client].sess.latchPlayerWeapon = bs->mpWeapon;    // FIXME: better place for this?
	level.clients[bs->client].sess.latchPlayerWeapon2 = 0;  // FIXME: better place for this?
	//
	bs->movementAutonomy = BMA_NOVALUE;
	bs->script.movementAutonomy = BMA_NOVALUE;
	bs->script.weaponAutonomy = BWA_NOVALUE;
	bs->scriptAutonomy = BSA_QUITSCRIPT;

	//	TAT 11/14/2002 - no commanded weapon, use whatever you want
	bs->commandedWeapon = -1;

// START	Mad Doctor I changes, 8/15/2002

	// When did we enter the current AI Node?
	bs->enteredCurrentAINodeTime = 0;

	// We've not seen an enemy yet
	bs->alertState = AISTATE_RELAXED;

	// What is our scripted FOV?
	bs->scriptedFieldOfView = 120;

	// What is our scripted hearing range?
	bs->scriptedHearingRange = 750;

	// When an enemy is this close, we can hear them even outside our
	// field of view
	bs->closeHearingRange = 200;

	// What is our scripted speed coefficient
	bs->speedCoefficient = 1;

	// Default to a longer vision range for Allied Bots
	if ( bs->mpTeam == TEAM_ALLIES ) {
		// How far can this bot see?
		bs->visionRange = 1200;

		// Allies can see further off
		bs->farSeeingRange = 1500;

	} // if (bs->mpTeam == TEAM_ALLIES)...
	else
	{
		// How far can this bot see?
		bs->visionRange = 750;

		// Nazis aren't far-sightes
		bs->farSeeingRange = bs->visionRange;

	} // else...

	// Not doing any avoid goal yet
	bs->avoid_goal.goalEndTime = 0;

	// Default to the scripts not telling us to use a specific weapon
	bs->scriptedWeapon = -1;

	// Default to not scritped asleep
	bs->scriptedSleep = qfalse;

	// We haven't failed a move yet.
	bs->movementFailedBadly = qfalse;

	// We haven't overriden a movetomarker
	bs->overrideMovementScripts = qfalse;

	// last time an anim was played using scripting
	bs->scriptAnimTime = -1;

	// which anim was last played by the scripts


	// We'll want to find a new combat spot right away
	bs->whenToFindNewCombatSpot = 0;

	// We have no assigned seek cover spot
	bs->seekCoverSpot = -1;

	// Are we currently hiding?
	bs->amIHiding = qfalse;

	// When should we stop hiding
	bs->toggleHidingTime = 0;

	// We have no scripted cover spot yet
	bs->scriptedCoverSpot = NULL;

	for ( i = 0; i < MAX_STORED_SEEKCOVERS; i++ )
		bs->lastSeekCoverSpots[i] = -1;

	// We're not in a chain of cover spots yet
	bs->coverSpotType = COVER_CHAIN_NONE;

// END		Mad Doctor I changes, 8/15/2002

	// RF, note: all script variables should exist within bs->script structure
	// START	xkan, 8/21/2002
	// default to no crouch and no prone
	bs->script.flags &= ~BSFL_CROUCH;
	bs->script.flags &= ~BSFL_PRONE;
	// END		xkan, 8/21/2002

	// Mad Doctor I, 9/19/2002.  Init to "not inited"!
	bs->weaponnum = -1;

	//
	BotAI_SetNumBots( BotAI_GetNumBots() + 1 );

	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();
	// get the attributes
	BotGetInitialAttributes( bs );
	// no script data yet
	bs->script.data = NULL;
	//

	bs->fireRate = 1.0; // xkan, 10/23/2002 - init to normal(maximum) fire rate

	// We can check fire rate at start
	bs->fireRateCheckTime = 0;
	bs->minFireRateCycleTime = kBOT_MIN_FIRE_CYCLE_TIME;
	bs->maxFireRateCycleTime = kBOT_MAX_FIRE_CYCLE_TIME;

	// We haven't arrived at our spot yet
	bs->arrived = qfalse;

	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient( int client ) {
	bot_state_t *bs;

	bs = &botstates[client];
	if ( !bs->inuse ) {
		// BotAI_Print(PRT_ERROR, "client %d already shutdown\n", client);
		return BLERR_AICLIENTALREADYSHUTDOWN;
	}

	// close log file
	if ( bs->script.logFile && ( bs->script.flags & BSFL_LOGGING ) ) {
		bs->script.flags &= ~BSFL_LOGGING;
		trap_FS_FCloseFile( bs->script.logFile );
		bs->script.logFile = 0;
	}

	trap_BotFreeMoveState( bs->ms );
	//free the goal state
	trap_BotFreeGoalState( bs->gs );
	//free the chat file
	trap_BotFreeChatState( bs->cs );
	//free the weapon weights
	trap_BotFreeWeaponState( bs->ws );
	//free the bot character
	trap_BotFreeCharacter( bs->character );
	//
	BotFreeWaypoints( bs->checkpoints );
	BotFreeWaypoints( bs->patrolpoints );
	//clear the bot state
	memset( bs, 0, sizeof( bot_state_t ) );
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	BotAI_SetNumBots( BotAI_GetNumBots() - 1 );
	//everything went ok
	return BLERR_NOERROR;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState( bot_state_t *bs ) {
	int client, entitynum, inuse;
	int movestate, goalstate, chatstate, weaponstate;
	bot_settings_t settings;
	int character;
	playerState_t ps;                           //current player state
//	float entergame_time;

	//save some things that should not be reset here
	memcpy( &settings, &bs->settings, sizeof( bot_settings_t ) );
	memcpy( &ps, &bs->cur_ps, sizeof( playerState_t ) );
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	character = bs->character;
	movestate = bs->ms;
	goalstate = bs->gs;
	chatstate = bs->cs;
	weaponstate = bs->ws;

	//free checkpoints and patrol points
	BotFreeWaypoints( bs->checkpoints );
	BotFreeWaypoints( bs->patrolpoints );
	//reset the whole state
	memset( bs, 0, sizeof( bot_state_t ) );
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->cs = chatstate;
	bs->ws = weaponstate;
	memcpy( &bs->cur_ps, &ps, sizeof( playerState_t ) );
	memcpy( &bs->settings, &settings, sizeof( bot_settings_t ) );
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->character = character;

	//reset several states
	if ( bs->ms ) {
		trap_BotResetMoveState( bs->ms );
	}
	if ( bs->gs ) {
		trap_BotResetGoalState( bs->gs );
	}
	if ( bs->ws ) {
		trap_BotResetWeaponState( bs->ws );
	}
	if ( bs->gs ) {
		trap_BotResetAvoidGoals( bs->gs );
	}
	if ( bs->ms ) {
		trap_BotResetAvoidReach( bs->ms );
	}

	// Start		TAT 9/27/2002
	// We should make sure the returnToLeader is -1, meaning no one
	bs->returnToLeader = -1;
	// End			TAT 9/27/2002
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap( int restart ) {
	int i;
	vmCvar_t mapname;

	if ( !restart ) {
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap_BotLibLoadMap( mapname.string );
	} else {
		// NULL mapname is a restart
		trap_BotLibLoadMap( NULL );
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( botstates[i].inuse ) {
			BotResetState( &botstates[i] );
			botstates[i].setupcount = 4;
		}
	}

	BotSetupDeathmatchAI();

	BotSpawnSpecialEntities();

	trap_BotLibStartFrame( (float) level.time / 1000 );

	return BLERR_NOERROR;
}



/*
==================
BotPreProcessAI

Perform Pre-processing steps on the bots and entities
==================
*/
void BotPreProcessAI() {
}

int botTime_EmergencyGoals;
int botTime_FindGoals;
int botTime_FindEnemy;

/*
==================
BotAIThinkFrame
==================
*/
int BotAIThinkFrame( int time ) {
	int i;
	int elapsed_time, thinktime, thinkcount, lastthinkbot, botcount;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;
	static int lastbot;
	int startTime /*, totalProfileTime*/;
	if ( bot_profile.integer == 1 ) {
		startTime = trap_Milliseconds();
	}

	trap_Cvar_Update( &bot_rocketjump );
	trap_Cvar_Update( &bot_grapple );
	trap_Cvar_Update( &bot_fastchat );
	trap_Cvar_Update( &bot_nochat );
	trap_Cvar_Update( &bot_testrchat );
	trap_Cvar_Update( &bot_thinktime );
	trap_Cvar_Update( &bot_profile );
	// Ridah, set the default AAS world
	trap_AAS_SetCurrentWorld( 0 );
	trap_Cvar_Update( &memorydump );

	botTime_EmergencyGoals = 0;
	botTime_FindGoals = 0;
	botTime_FindEnemy = 0;

	if ( memorydump.integer ) {
		trap_BotLibVarSet( "memorydump", "1" );
		trap_Cvar_Set( "memorydump", "0" );
	}

	//if the bot think time changed we should reschedule the bots
	if ( bot_thinktime.integer != lastbotthink_time ) {
		lastbotthink_time = bot_thinktime.integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if ( elapsed_time > bot_thinktime.integer ) {
		thinktime = elapsed_time;
	} else {
		thinktime = bot_thinktime.integer;
	}

	thinkcount = 0;
	lastthinkbot = lastbot;

	// execute scheduled bot AI
	for ( i = lastbot + 1, botcount = 0; botcount < MAX_CLIENTS; i++, botcount++ )  {
		// If we've gone past the end of the list, start over
		if ( i >= MAX_CLIENTS ) {
			// Start back at the beginning of the list
			i = 0;

			// Perform Pre-processing steps on the bots and entities
			BotPreProcessAI();
		}

		if ( !botstates[i].inuse ) {
			continue;
		}

		botstates[i].botthink_residual += elapsed_time + ( rand() % ( bot_thinktime.integer / 4 ) );    // randomize the times a bit so they are more likely to disperse amongst frames

		if ( botstates[i].botthink_residual >= thinktime * ( ( VectorLengthSquared( botstates[i].cur_ps.velocity ) < SQR( 10 ) ) ? 2 : 1 ) ) {
			botstates[i].botthink_residual -= thinktime * ( ( VectorLengthSquared( botstates[i].cur_ps.velocity ) < SQR( 10 ) ) ? 2 : 1 );
			if ( botstates[i].botthink_residual > thinktime ) {
				botstates[i].botthink_residual = thinktime;
			}

/*			if (!trap_AAS_Initialized()) {
				return BLERR_NOERROR;
			}*/

			if ( g_entities[i].client->pers.connected == CON_CONNECTED ) {
				BotAI( i, thinktime / 1000.f );
				BotUpdateInput( &botstates[i], time );
				trap_BotUserCommand( botstates[i].client, &botstates[i].lastucmd );
				//
				lastthinkbot = i;
			}

			thinkcount++;

/*			if(thinkcount >= 4) {
				break;
			}*/
		}
	}

	lastbot = lastthinkbot;

/*	if( bot_profile.integer == 1 ) {
		totalProfileTime = trap_Milliseconds() - startTime;
		G_Printf( "BotAIThinkFrame: %4i total (%4i em, %4i fg, %4i en) thinkcount: %i\n", totalProfileTime, botTime_EmergencyGoals, botTime_FindGoals, botTime_FindEnemy, thinkcount );
	}*/

	return BLERR_NOERROR;
}

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame( int time ) {
	int i;
	gentity_t   *ent;
	bot_entitystate_t state;
	//entityState_t entitystate;
	//vec3_t mins = {-15, -15, -24}, maxs = {15, 15, 32};
	int elapsed_time, thinktime;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;

	G_CheckBotSpawn();

	trap_Cvar_Update( &bot_rocketjump );
	trap_Cvar_Update( &bot_grapple );
	trap_Cvar_Update( &bot_fastchat );
	trap_Cvar_Update( &bot_nochat );
	trap_Cvar_Update( &bot_testrchat );
	trap_Cvar_Update( &bot_thinktime );
	// Ridah, set the default AAS world
	trap_AAS_SetCurrentWorld( 0 );
	trap_Cvar_Update( &memorydump );

	if ( memorydump.integer ) {
		trap_BotLibVarSet( "memorydump", "1" );
		trap_Cvar_Set( "memorydump", "0" );
	}

	//if the bot think time changed we should reschedule the bots
	if ( bot_thinktime.integer != lastbotthink_time ) {
		lastbotthink_time = bot_thinktime.integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if ( elapsed_time > bot_thinktime.integer ) {
		thinktime = elapsed_time;
	} else {
		thinktime = bot_thinktime.integer;
	}

	BotCountLandMines();

	// update the bot library
	if ( botlib_residual >= thinktime ) {
		botlib_residual -= thinktime;

		trap_BotLibStartFrame( (float) time / 1000 );

		// Ridah, only check the default world
		trap_AAS_SetCurrentWorld( 0 );

		if ( !trap_AAS_Initialized() ) {
			return BLERR_NOERROR;
		}

		//update entities in the botlib
		for ( i = 0; i < level.num_entities; i++ ) {
			// Ridah - WOLF, we only need client entity information
			//if (i > level.maxclients) {
			//	break;
			//}

			ent = &g_entities[i];
			if ( !ent->inuse ) {
				continue;
			}
			if ( !ent->r.linked ) {
				continue;
			}
			if ( ent->r.svFlags & SVF_NOCLIENT ) {
				continue;
			}

			memset( &state, 0, sizeof( bot_entitystate_t ) );

			VectorCopy( BotGetOrigin( i ), state.origin );

			if ( !VectorCompare( ent->r.currentAngles, vec3_origin ) ) {
				VectorCopy( ent->r.currentAngles, state.angles );
			} else if ( ent->client ) {
				VectorCopy( ent->client->ps.viewangles, state.angles );
			} else {
				VectorCopy( ent->s.angles, state.angles );
			}

			VectorCopy( ent->s.origin2, state.old_origin );
			VectorCopy( ent->r.mins, state.mins );
			VectorCopy( ent->r.maxs, state.maxs );
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;

			if ( ent->r.bmodel ) {
				state.solid = SOLID_BSP;
			} else {
				state.solid = SOLID_BBOX;
			}

			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			state.powerups = ent->s.powerups;
			state.legsAnim = ent->s.legsAnim;
			state.torsoAnim = ent->s.torsoAnim;
			state.weapon = ent->s.weapon;

			trap_BotLibUpdateEntity( i, &state );
		}

		BotAIRegularUpdate();

	}

#ifndef NO_BOT_SUPPORT
	// let bots do their thinking (only if this is dedicated, or the local client hasnt processed bot thinks in a while)
	if ( bot_enable.integer && ( g_dedicated.integer || ( level.lastClientBotThink < level.time - 200 ) ) ) {
		BotAIThinkFrame( level.time );
	}
#endif // NO_BOT_SUPPORT

// Since there is no interpolation on the client-side anymore, there is no need to move the bot at a set time-interval anymore
/*
	// execute bot user commands every frame
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i].inuse ) {
			continue;
		}
		if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		BotUpdateInput( &botstates[i], time );
		trap_BotUserCommand(botstates[i].client, &botstates[i].lastucmd);
	}
*/
	return BLERR_NOERROR;
}

/*
==============
BotInitLibrary
==============
*/
int BotInitLibrary( void ) {
	char buf[144];

	//set the maxclients and maxentities library variables before calling BotSetupLibrary
	trap_Cvar_VariableStringBuffer( "sv_maxclients", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "8" );
	}
	trap_BotLibVarSet( "maxclients", buf );
	Com_sprintf( buf, sizeof( buf ), "%d", MAX_GENTITIES );
	trap_BotLibVarSet( "maxentities", buf );
	//bsp checksum
	trap_Cvar_VariableStringBuffer( "sv_mapChecksum", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "sv_mapChecksum", buf );
	}
	//maximum number of aas links
	trap_Cvar_VariableStringBuffer( "max_aaslinks", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "max_aaslinks", buf );
	}
	//maximum number of items in a level
	trap_Cvar_VariableStringBuffer( "max_levelitems", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "max_levelitems", buf );
	}
	//automatically launch WinBSPC if AAS file not available
	trap_Cvar_VariableStringBuffer( "autolaunchbspc", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "autolaunchbspc", "1" );
	}
	//
	trap_Cvar_VariableStringBuffer( "g_gametype", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "g_gametype", buf );

	trap_Cvar_VariableStringBuffer( "bot_developer", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "bot_developer", buf );
	//log file
	trap_Cvar_VariableStringBuffer( "bot_developer", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "log", buf );
	//no chatting
	trap_Cvar_VariableStringBuffer( "bot_nochat", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "nochat", "0" );
	}
	//forced clustering calculations
	trap_Cvar_VariableStringBuffer( "forceclustering", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "forceclustering", buf );
	}
	//forced reachability calculations
	trap_Cvar_VariableStringBuffer( "forcereachability", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "forcereachability", buf );
	}
	//force writing of AAS to file
	trap_Cvar_VariableStringBuffer( "forcewrite", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "forcewrite", buf );
	}
	//no AAS optimization
	trap_Cvar_VariableStringBuffer( "nooptimize", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "nooptimize", buf );
	}
	//number of reachabilities to calculate each frame
	trap_Cvar_VariableStringBuffer( "framereachability", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "20" );
	}
	trap_BotLibVarSet( "framereachability", buf );
	//
	trap_Cvar_VariableStringBuffer( "bot_reloadcharacters", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "bot_reloadcharacters", buf );
	//base directory
	trap_Cvar_VariableStringBuffer( "fs_basepath", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "basedir", buf );
	}
	//game directory
	trap_Cvar_VariableStringBuffer( "fs_game", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "gamedir", buf );
	}
	//cd directory
	trap_Cvar_VariableStringBuffer( "fs_cdpath", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "cddir", buf );
	}
	//setup the bot library
	return trap_BotLibSetup();
}

/*
==============
BotAISetup
==============
*/
int BotAISetup( int restart ) {
	int errnum;

#ifdef RANDOMIZE
	srand( (unsigned)time( NULL ) );
#endif //RANDOMIZE

	trap_Cvar_Register( &bot_verbose, "bot_verbose", "0", 0 );
	trap_Cvar_Register( &bot_thinktime, "bot_thinktime", "50", 0 );
	trap_Cvar_Register( &bot_profile, "bot_profile", "0", 0 );
	trap_Cvar_Register( &memorydump, "memorydump", "0", 0 );
	trap_Cvar_Register( &bot_findgoal, "bot_findgoal", "0", 0 );

	//if the game is restarted for a tournament
	if ( restart ) {
		return BLERR_NOERROR;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof( botstates ) );

	errnum = BotInitLibrary();
	if ( errnum != BLERR_NOERROR ) {
		return qfalse;
	}
	return BLERR_NOERROR;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown( int restart ) {

	int i;

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for ( i = 0; i < level.numConnectedClients; i++ ) {
			if ( botstates[level.sortedClients[i]].inuse ) {
				BotAIShutdownClient( botstates[level.sortedClients[i]].client );
			}
		}
		//don't shutdown the bot library
	} else {
		trap_BotLibShutdown();
	}
	return qtrue;
}

//====================================================
// BOT GAME ENTITIES
//
//	Used for point entities that dont need to be sitting
//	in the global list of entities, wasting valuable slots.

// TAT 11/13/2002 NUM_BOTGAMEENTITIES defined in botlib.h
/*
gentity_t botGameEntities[NUM_BOTGAMEENTITIES];
int numBotGameEntities;

// TAT - bot_mg42_spot works, if we want to use this system
char *botGameEntityNames[] = {
//	"ai_marker",
//	"bot_sniper_spot",
//	"bot_mg42_spot",
//	"bot_seek_cover_spot",
	NULL
};

void BotInitBotGameEntities(void) {
	memset( botGameEntities, 0, sizeof(botGameEntities) );
	numBotGameEntities = 0;
}

gentity_t *BotSpawnGameEntity(void) {
	if (numBotGameEntities >= NUM_BOTGAMEENTITIES) return NULL;
	//
	botGameEntities[numBotGameEntities].s.number = MAX_GENTITIES + numBotGameEntities;
	botGameEntities[numBotGameEntities].inuse = qtrue;
	return &botGameEntities[numBotGameEntities++];
}

// TAT 11/12/2002 - we're going to want to check that something is from this list
//		first thing when we spawn it
//		And this returns a ptr to the new entity or to the old one if it isn't from our list
gentity_t *BotCheckBotGameEntity( gentity_t *ent )
{
	int i;
	gentity_t *botent;
	int num;

	if (!ent->classname) return ent;

	// is this a bot game entity?
	for (i=0; botGameEntityNames[i]; i++) {
		if (!Q_stricmp( botGameEntityNames[i], ent->classname )) {
			botent = BotSpawnGameEntity();
			if (!botent) {
				G_Error( "BotCheckBotGameEntity: exceeded NUM_BOTGAMEENTITIES (%i)", NUM_BOTGAMEENTITIES );
			}
			num = botent->s.number;
			memcpy( botent, ent, sizeof(gentity_t) );
			botent->s.number = num;
			// free the entity
			G_FreeEntity( ent );

			// in our list, return the new entity
			return botent;
		}
	}

	// not in our list
	return ent;
}
*/

gentity_t *BotFindEntity( gentity_t *from, int fieldofs, char *match ) {
	return G_Find( from, fieldofs, match );
}


/*
=================
FindBotByName
=================

Get the bot state of a named bot

*/
bot_state_t *FindBotByName
(
	// Name of the bot to look up
	char * botName
) {
	// Pointer to another bot that might be specified in the scripting
	bot_state_t *otherBot = NULL;

	// Index for looping through bots
	int botNum = 0;

	// Loop through all the bots
	for ( botNum = 0; botNum < level.maxclients; botNum++ )
	{
		// RF, make sure it's still in use
		if ( !botstates[botNum].inuse ) {
			continue;
		}

		// Grab the next bot to check its name
		otherBot = &botstates[botNum];

		// Does this bot have the right name?
		if ( !Q_stricmp( g_entities[otherBot->client].scriptName, botName ) ) {
			// This is our bot, send it up the chain!
			return otherBot;

		} // if (!Q_stricmp(g_entities[bs->client].scriptName, botName))...

	} // for (botNum=0; botNum<level.maxclients; botNum++) ...

	// We got nobody with this name
	return NULL;

} // bot_state_t *FindBotByName...


gentity_t *BotGetEntity( int entityNum ) {
#ifdef _DEBUG
	if ( ( entityNum < 0 ) || ( entityNum >= MAX_GENTITIES ) ) {
		G_Printf( "^1BotGetEntity: Invalid entityNum\n" );
		return NULL;
	}
#endif // _DEBUG

	return &g_entities[entityNum];
}

//====================================================
// BOT STATIC ENTITY CACHE
//
//	Used to speed up searching of common entities
//	!!! NOTE: must be in synch with enum list in ai_main.h

char *botStaticEntityStrings[NUM_BOTSTATICENTITY] = {
	"team_WOLF_checkpoint",
	"trigger_flagonly",
	"misc_mg42",
	"trigger_objective_info",
	"team_CTF_redflag",
	"team_CTF_blueflag",
	"func_explosive",
	"func_door",
	"func_door_rotating",
	"func_constructible",
	"trigger_multiple",
	"trigger_flagonly_multiple",
	"bot_landmine_area",
	"bot_attractor",
	"bot_sniper_spot",
	"bot_landminespot_spot",
};

gentity_t *botStaticEntityList[NUM_BOTSTATICENTITY];

/*
===============
BotBuildStaticEntityCache
===============
*/
void BotBuildStaticEntityCache( void ) {
	int i;
	gentity_t *trav, *p;
	//
	memset( botStaticEntityList, 0, sizeof( botStaticEntityList ) );
	//
	for ( i = 0; i < NUM_BOTSTATICENTITY; i++ ) {
		trav = NULL;
		while ( ( trav = G_Find( trav, FOFS( classname ), botStaticEntityStrings[i] ) ) ) {
			trav->botNextStaticEntity = NULL;
			p = botStaticEntityList[i];
			if ( !p ) {
				botStaticEntityList[i] = trav;
			} else {    // add trav to the end of the list
				while ( p->botNextStaticEntity ) p = p->botNextStaticEntity;
				p->botNextStaticEntity = trav;
			}
		}
	}
	//
	level.initStaticEnts = qtrue;
}

/*
================
BotFindNextStaticEntity
================
*/
gentity_t *BotFindNextStaticEntity( gentity_t *start, botStaticEntityEnum_t entityEnum ) {
	gentity_t *trav;

	// Gordon: give stuff time to spawn, just in case
	if ( level.time - level.startTime < FRAMETIME * 5 ) {
		return NULL;
	}

	if ( !level.initStaticEnts ) {
		BotBuildStaticEntityCache();
	}

	trav = botStaticEntityList[entityEnum];
	while ( trav && start && trav->s.number <= start->s.number ) {
		trav = trav->botNextStaticEntity;
	}

	return trav;
}

/*
===============
BotFindEntityForName
===============
*/
gentity_t *BotFindEntityForName( char *name ) {
	gentity_t *trav;
	int i;

	for ( trav = g_entities, i = 0; i < level.maxclients; i++, trav++ ) {
		if ( !trav->inuse ) {
			continue;
		}
		if ( !trav->client ) {
			continue;
		}
		if ( !trav->aiName ) {
			continue;
		}
		if ( Q_stricmp( trav->aiName, name ) ) {
			continue;
		}
		return trav;
	}
	return NULL;
}

/*
================
BotSinglePlayer
================
*/
qboolean BotSinglePlayer() {
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return qtrue;
	}
	//
	return qfalse;
}

/*
================
BotSinglePlayer
================
*/
qboolean BotCoop() {
	if ( g_gametype.integer == GT_COOP ) {
		return qtrue;
	}
	//
	return qfalse;
}


/*
===============
G_SetAASBlockingEntity

  Adjusts routing so AI knows it can't move through this entity
===============
*/
void G_SetAASBlockingEntity( gentity_t *ent, int blocking ) {
	// Gordon: short circuit this as we dont need it
	return;

	if ( blocking > 0 ) {
		// always add first flag if we are not clearing the flags
		blocking |= 1;
	}

	// if we are not blocking now, and we were previously, always use the same box for removing the blocking areas
	if ( !( blocking & 1 ) ) {
		if ( ent->AASblocking & 1 ) {
			// turn off old blocking areas
			trap_AAS_SetAASBlockingEntity( ent->AASblocking_mins, ent->AASblocking_maxs, qfalse );
			ent->AASblocking = qfalse;
		}
		// we're done
		return;
	}

	// if we are currently blocking, and we are being asked to block again, but with a different box, turn off previous blocking
	if ( blocking & 1 && ent->AASblocking & 1 && ( !VectorCompare( ent->r.absmin, ent->AASblocking_mins ) || !VectorCompare( ent->r.absmax, ent->AASblocking_maxs ) ) ) {
		// turn off old blocking areas
		trap_AAS_SetAASBlockingEntity( ent->AASblocking_mins, ent->AASblocking_maxs, qfalse );
	}

	// if we are blocking now and before, and the bounds are the same, ignore
	if ( ( blocking == ent->AASblocking ) && ( VectorCompare( ent->r.absmin, ent->AASblocking_mins ) && VectorCompare( ent->r.absmax, ent->AASblocking_maxs ) ) ) {
		return;
	}

	//
	// determine mover status
	//
	if ( ent->s.eType == ET_EXPLOSIVE ) {
		blocking |= BLOCKINGFLAG_MOVER;
	} else if ( ent->s.eType == ET_CONSTRUCTIBLE && !( ent->spawnflags & 1024 ) ) {
		if ( ent->spawnflags & 128 ) {   // must be blocking
			if ( !( ent->spawnflags & 512 ) ) {    // not ignored by AAS
				blocking |= BLOCKINGFLAG_MOVER;
			}
		}
	}

	//
	// set the new blocking
	//
	ent->AASblocking = ( blocking & ~BLOCKINGFLAG_MOVER );
	trap_AAS_SetAASBlockingEntity( ent->r.absmin, ent->r.absmax, blocking );
	if ( blocking ) {
		VectorCopy( ent->r.absmin, ent->AASblocking_mins );
		VectorCopy( ent->r.absmax, ent->AASblocking_maxs );
	}
}

/*
===================
BotRecordTeamDeath

  Allows the AAS to make dangerous routes more costly, therefore bots will avoid them when possible
===================
*/
void BotRecordTeamDeath( int client ) {
	vec3_t org;
	int area, travelflags, teamCount, team;
	//
//RF, disabling for now, seems to cause routing loops
	return;

	if ( G_IsSinglePlayerGame() ) {
		return;
	}
	//
	VectorCopy( BotGetOrigin( client ), org );
	area = BotGetArea( client );
	if ( !area ) {
		return;
	}
	travelflags = BotTravelFlagsForClient( client );
	team = level.clients[client].sess.sessionTeam;
	teamCount = TeamCount( -1, team );
	//
	trap_AAS_RecordTeamDeathArea( org, area, team, teamCount, travelflags );
}


/*
===============
BotRecordAttack

  src has just attacked dest
===============
*/
void BotRecordAttack( int src, int dest ) {
	g_entities[dest].botLastAttackedTime = level.time;
	g_entities[dest].botLastAttackedEnt = src;
}


// START - Mad Doc - TDf
/*
===============
BotDebug

  fills the appropriate cvars for showing a bot's "thought bubble"
===============
*/
void BotDebug( int clientNum ) {
	char buf[256];

	// get the botstate
	bot_state_t *bs;

	bs = &botstates[clientNum];

	if ( bs->inuse ) {
		// TAT - print more detailed info for follow leader
		if ( bs->leader > -1 ) {
			trap_Cvar_Set( "bot_debug_curAINode", va( "%s: leader = %i tagent = %i", bs->ainodeText, bs->leader, bs->leader_tagent ) );
		} else {
			trap_Cvar_Set( "bot_debug_curAINode", bs->ainodeText );
		}
		switch ( bs->alertState )
		{
		case AISTATE_RELAXED:
			trap_Cvar_Set( "bot_debug_alertState", "RELAXED" );
			break;
		case AISTATE_QUERY:
			trap_Cvar_Set( "bot_debug_alertState", "QUERY" );
			break;
		case AISTATE_ALERT:
			trap_Cvar_Set( "bot_debug_alertState", "ALERT" );
			break;
		case AISTATE_COMBAT:
			trap_Cvar_Set( "bot_debug_alertState", "COMBAT" );
			break;
		default:
			trap_Cvar_Set( "bot_debug_alertState", "ERROR bad state" );
			break;
		}

		{
			playerState_t   *ps = &bs->cur_ps;
			animModelInfo_t *animModelInfo = BG_GetCharacterForPlayerstate( ps )->animModelInfo;
			trap_Cvar_Set( "bot_debug_anim", va( "leg-%s torso-%s",
												 animModelInfo->animations[ps->legsAnim & ~ANIM_TOGGLEBIT]->name,
												 animModelInfo->animations[ps->torsoAnim & ~ANIM_TOGGLEBIT]->name ) );
		}

		trap_Cvar_Set( "bot_debug_pos", ( va( "(%f,%f,%f)", bs->origin[0], bs->origin[1], bs->origin[2] ) ) );

		// curr script function handled differently, so nothing here about it

		Com_sprintf( buf, sizeof( buf ), "%i", BotGetMovementAutonomyLevel( bs ) );
		trap_Cvar_Set( "bot_debug_moveAut", buf );

		// TAT 12/9/2002 - Throwing some extra info into the cover spot display
		{
			g_serverEntity_t *coverSpot = GetServerEntity( bs->seekCoverSpot );
			Com_sprintf( buf, sizeof( buf ), "%i(%s)  Enemy = %i", bs->seekCoverSpot, coverSpot ? coverSpot->name : "", bs->enemy );
			trap_Cvar_Set( "bot_debug_cover_spot", buf );
		}
	} else
	{
		trap_Cvar_Set( "bot_debug_curAINode", "NULL" );
		trap_Cvar_Set( "bot_debug_alertState", "NULL" );
		trap_Cvar_Set( "bot_debug_pos", "(--,--,--)" );
		trap_Cvar_Set( "bot_debug_scriptFunc", "NULL" );
		trap_Cvar_Set( "bot_debug_weapAut", "NULL" );
		trap_Cvar_Set( "bot_debug_moveAut", "NULL" );
		trap_Cvar_Set( "bot_debug_cover_spot", "NULL" );
		trap_Cvar_Set( "bot_debug_anim", "NULL" );
	}
}


// Mad Doc - TDF
/*
===============
GetBotAutonomies

  stuffs the parms with the appropriate data
===============
*/
void GetBotAutonomies( int clientNum, int *weapAutonomy, int *moveAutonomy ) {
	// get the botstate
	bot_state_t *bs;

	bs = &botstates[clientNum];

	if ( bs->inuse ) {
		// use +1 so we can have 0 mean not found
		*moveAutonomy = BotGetMovementAutonomyLevel( bs ) + 1;
	} else {
		*moveAutonomy = 0;
	}
}


// Mad Doc - TDF
/*
===============
GetBotAmmo

  stuffs the parms with the appropriate data
===============
*/
void GetBotAmmo( int clientNum, int *weapon, int *ammo, int *ammoclip ) {
	gentity_t *ent;

	ent = &g_entities[clientNum];

	*weapon = ent->client->ps.weapon;
	*ammo = ent->client->ps.ammo[BG_FindAmmoForWeapon( *weapon )];
	*ammoclip = ent->client->ps.ammoclip[BG_FindClipForWeapon( *weapon )];
}

// END Mad Doc - TDF

// xkan - sets the ideal view angles
void BotSetIdealViewAngles( int clientNum, vec3_t angle ) {
	// get the botstate
	bot_state_t *bs;

	bs = &botstates[clientNum];
	if ( bs->inuse ) {
		VectorCopy( angle, bs->ideal_viewangles );
	}
}

// TAT 1/14/2003 - init the bot's movement autonomy pos to it's current position
void BotInitMovementAutonomyPos( gentity_t *bot ) {
	bot_state_t* bs = &botstates[bot->s.number];

	if ( bs->inuse ) {
		// TAT 1/14/2003 - set the autonomy position to the current position
		VectorCopy( bot->client->ps.origin, bs->script.movementAutonomyPos );
		VectorCopy( bot->client->ps.origin, bs->movementAutonomyPos );
	}
}

/*
===================
BotDebugViewClient
===================
*/
void BotDebugViewClient( int client ) {
	static int lastChange;
	if ( bot_debug.integer != 10 ) {
		return;
	}
	if ( !g_cheats.integer ) {
		return;
	}
	if ( lastChange < level.time && lastChange > level.time - 5000 ) {
		return;
	}
	if ( !level.clients[0].pers.connected == CON_CONNECTED ) {
		return;
	}
	if ( g_entities[0].r.svFlags & SVF_BOT ) {
		return;
	}
	if ( level.clients[0].sess.sessionTeam != TEAM_SPECTATOR ) {
		return;
	}
	//
	level.clients[0].sess.spectatorClient = client;
}
