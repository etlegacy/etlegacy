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
 * name:		ai_dmnet_mp.c
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
#include "ai_team.h"
#include "ai_dmq3.h"
#include "ai_cmd.h"
#include "ai_dmnet_mp.h"

//data file headers
#include "chars.h"           //characteristics
#include "inv.h"         //indexes into the inventory
#include "syn.h"         //synonyms
#include "match.h"           //string matching types and vars

/*
==================
BotMP_MoveToGoal
==================
*/
void BotMP_MoveToGoal( bot_state_t *bs, bot_moveresult_t * result, int movestate, bot_goal_t * goal, int travelflags ) {
#ifdef _DEBUG
	if ( bot_debug.integer == 3 && level.clients[0].sess.spectatorClient == bs->client ) {
		goal->flags |= GFL_DEBUGPATH;
	} else {
		goal->flags &= ~GFL_DEBUGPATH;
	}
#endif // _DEBUG

	trap_BotMoveToGoal( result, movestate, goal, travelflags );
}

/*
==================
AIEnter_MP_Intermission()
==================
*/
void AIEnter_MP_Intermission( bot_state_t *bs ) {
	//reset the bot state
	BotResetState( bs );
	bs->ainode = AINode_MP_Intermission;
	bs->ainodeText = "AINode_MP_Intermission";
}

/*
==================
AINode_MP_Intermission()
==================
*/
int AINode_MP_Intermission( bot_state_t *bs ) {
	//if the intermission ended
	if ( !BotIntermission( bs ) ) {
		AIEnter_MP_Stand( bs );
	}
	return qtrue;
}

/*
==================
AIEnter_MP_Observer()
==================
*/
void AIEnter_MP_Observer( bot_state_t *bs ) {
	//reset the bot state
	BotResetState( bs );
	bs->ainode = AINode_MP_Observer;
	bs->ainodeText = "AINode_MP_Observer";
}

/*
==================
AINode_MP_Observer()
==================
*/
int AINode_MP_Observer( bot_state_t *bs ) {
	//if the bot left observer mode
	if ( !BotIsObserver( bs ) ) {
		AIEnter_MP_Stand( bs );
	}
	return qtrue;
}

/*
==================
AIEnter_MP_Stand()
==================
*/
void AIEnter_MP_Stand( bot_state_t *bs ) {
	//bs->standfindenemy_time = trap_AAS_Time() + 1;
	bs->respawn_time = trap_AAS_Time() + 20;    // after this long just standing around, suicide
	bs->ignore_specialgoal_time = 0;
	bs->ainode = AINode_MP_Stand;
	bs->ainodeText = "AINode_MP_Stand";
}

/*
==================
AINode_MP_Stand()
==================
*/
int AINode_MP_Stand( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t pos;


	// Gordon: pow, so just stand around till scripted
	if ( BotIsPOW( bs ) ) {
		return qtrue;
	}

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	if ( bs->standfindenemy_time < trap_AAS_Time() ) {
		if ( BotFindEnemyMP( bs, -1, qfalse ) ) {
			AIEnter_MP_Battle_Fight( bs );
			return qfalse;
		}
		//bs->standfindenemy_time = trap_AAS_Time() + 1;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	if ( bs->stand_time < trap_AAS_Time() ) {
		//trap_BotEnterChat(bs->cs, bs->client, bs->chatto);
		//bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		if ( bs->ainode != AINode_MP_Stand ) {
			return qfalse;
		} else if ( !bs->areanum ) {
			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
		} else {
			// stand for a bit longer
			bs->stand_time = trap_AAS_Time() + 0.4 + 0.4 * random();
		}
	} else {
		// look for health/ammo packs
		if ( BotFindNearbyGoal( bs ) ) {
			AIEnter_MP_Seek_NBG( bs );
			return qfalse;
		}
	}
	// check for dangerous elements
	VectorCopy( bs->origin, goal.origin );
	goal.areanum = bs->areanum;
	goal.entitynum = bs->client;
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	// if we are outside HALF autonomy range, get back there
	if ( BotGetMovementAutonomyPos( bs, pos ) ) {
		float halfDist = 0.5 * BotGetMovementAutonomyRange( bs, NULL );
		if ( VectorDistanceSquared( bs->origin, pos ) > ( halfDist * halfDist ) ) {
			AIEnter_MP_MoveToAutonomyRange( bs );
			return qfalse;
		}
	}
	// if we have been standing for too long
	if ( bs->respawn_time < trap_AAS_Time() ) {
		Cmd_Kill_f( &g_entities[bs->client] );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_Respawn()
==================
*/
void AIEnter_MP_Respawn( bot_state_t *bs ) {
	//reset some states
	trap_BotResetMoveState( bs->ms );
	trap_BotResetGoalState( bs->gs );
	trap_BotResetAvoidGoals( bs->gs );
	trap_BotResetAvoidReach( bs->ms );
	bs->respawn_time = trap_AAS_Time() + 1 + random();
	bs->respawnchat_time = 0;
	//
	bs->flags &= ~BFL_MISCFLAG;
	bs->lastClassCheck = 0;
	//set respawn state
	bs->respawn_wait = qfalse;
	bs->ainode = AINode_MP_Respawn;
	bs->ainodeText = "AINode_MP_Respawn";
}

/*
==================
AINode_MP_Respawn()
==================
*/
int AINode_MP_Respawn( bot_state_t *bs ) {
	qboolean do_respawn = qfalse;
	gentity_t *ent;
	int testtime;
	// RF, only hit jump if reinforcement time is about to run out
	ent = BotGetEntity( bs->entitynum );
// disabled, if medic has troubles finding us, we'll be waiting forever
//	if (ent->missionLevel < level.time) {	// else medic is heading to us to revive
	if ( ent->client->sess.sessionTeam == TEAM_AXIS ) {
		testtime = level.time % g_redlimbotime.integer;
		if ( testtime > g_redlimbotime.integer - 2000 ) {
			do_respawn = qtrue;
		}
	} else if ( ent->client->sess.sessionTeam == TEAM_ALLIES )     {
		testtime = level.time % g_bluelimbotime.integer;
		if ( testtime > g_bluelimbotime.integer - 2000 ) {
			do_respawn = qtrue;
		}
	}
//	}
	//
	if ( bs->lastClassCheck < level.time - 4000 ) {   // check for a better class
		bs->mpClass = BotSuggestClass( bs, bs->mpTeam );
		ent->client->sess.latchPlayerType = bs->mpClass;
		if ( bs->mpClass != ent->client->sess.playerType ) {
			bs->flags |= BFL_MISCFLAG;
		}
		bs->lastClassCheck = level.time + rand() % 1000;
		// sometimes when we die, we should re-evaluate our weapon selection
		if ( ( bs->flags & BFL_MISCFLAG ) || ( random() < 0.3 ) ) {
			bs->mpWeapon = BotSuggestWeapon( bs, bs->sess.sessionTeam );
			ent->client->sess.latchPlayerWeapon = bs->mpWeapon;
		}
	}
	if ( bs->respawn_wait ) {
		if ( !BotIsDead( bs ) ) {
			// perhaps we should tell everyone who we are
			if ( bs->flags & BFL_MISCFLAG ) {
				static int lastCall;
				if ( lastCall > level.time || lastCall < level.time - 2000 ) {
					lastCall = level.time;
					switch ( bs->mpClass ) {
					case PC_SOLDIER:
						BotVoiceChatAfterIdleTime( bs->client, "IamSoldier", SAY_TEAM, 1000 + rand() % 5000, BOT_SHOWTEXT, 20000, qfalse );
						break;
					case PC_MEDIC:
						BotVoiceChatAfterIdleTime( bs->client, "IamMedic", SAY_TEAM, 1000 + rand() % 5000, BOT_SHOWTEXT, 20000, qfalse  );
						break;
					case PC_FIELDOPS:
						BotVoiceChatAfterIdleTime( bs->client, "IamLieutenant", SAY_TEAM, 1000 + rand() % 5000, BOT_SHOWTEXT, 20000, qfalse  );
						break;
					case PC_ENGINEER:
						BotVoiceChatAfterIdleTime( bs->client, "IamEngineer", SAY_TEAM, 1000 + rand() % 5000, BOT_SHOWTEXT, 20000, qfalse  );
						break;
					}
				}
			} else if ( bs->sess.sessionTeam == level.attackingTeam ) {
				if ( rand() % 2 ) {
					BotVoiceChatAfterIdleTime( bs->client, "LetsGo", SAY_TEAM, 1000 + rand() % 2000, qfalse, 20000, qfalse  );
				}
			}
			//
			BotDefaultNode( bs );
		} else {
			trap_EA_Respawn( bs->client );
			// RF, Wolf uses jump
			if ( do_respawn ) {
				trap_EA_Jump( bs->client );
			}
		}
	} else if ( bs->respawn_time < trap_AAS_Time() )     {
		//wait until respawned
		bs->respawn_wait = qtrue;
		//elementary action respawn
		trap_EA_Respawn( bs->client );
		// RF, Wolf uses jump
		if ( do_respawn ) {
			trap_EA_Jump( bs->client );
		}
		//
		if ( bs->respawnchat_time ) {
			//trap_BotEnterChat(bs->cs, bs->client, bs->chatto);
			bs->enemy = -1;
		}
	}
	if ( bs->respawnchat_time && bs->respawnchat_time < trap_AAS_Time() - 0.5 ) {
		trap_EA_Talk( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_Seek_ActivateEntity()
==================
*/
void AIEnter_MP_Seek_ActivateEntity( bot_state_t *bs ) {
	bs->ainode = AINode_MP_Seek_ActivateEntity;
	bs->ainodeText = "AINode_MP_Seek_ActivateEntity";
}

/*
==================
AINode_MP_Seek_Activate_Entity()
==================
*/
int AINode_MP_Seek_ActivateEntity( bot_state_t *bs ) {
	bot_goal_t *goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}

	//no enemy
	bs->enemy = -1;
	//
	goal = &bs->activategoal;
	//if the bot has no goal
	if ( !goal ) {
		bs->activate_time = 0;
	}
	//if the bot touches the current goal
	else if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
		BotChooseWeapon( bs );
#ifdef DEBUG
		BotAI_Print( PRT_MESSAGE, "touched button or trigger\n" );
#endif //DEBUG
		bs->activate_time = 0;
	}
	//
	if ( bs->activate_time < trap_AAS_Time() ) {
		AIEnter_MP_Seek_NBG( bs );
		return qfalse;
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );

	}
	//check if the bot is blocked
	BotAIBlocked( bs, &moveresult, qtrue );
	//
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if ( !( bs->flags & BFL_IDEALVIEWSET ) )       {
		if ( trap_BotMovementViewTarget( bs->ms, goal, bs->tfl, 300, target ) ) {
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
		} else {
			//vectoangles(moveresult.movedir, bs->ideal_viewangles);
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//if there is an enemy
	if ( BotFindEnemyMP( bs, -1, qfalse ) ) {
		if ( BotWantsToRetreat( bs ) ) {
			//keep the current long term goal and retreat
			// !!! TODO
		} else {
			trap_BotResetLastAvoidReach( bs->ms );
			//empty the goal stack
			trap_BotEmptyGoalStack( bs->gs );
			//go fight
			AIEnter_MP_Battle_Fight( bs );
		}
	}
	return qtrue;
}

/*
==================
AIEnter_MP_Seek_NBG()
==================
*/
void AIEnter_MP_Seek_NBG( bot_state_t *bs ) {
//level.clients[0].sess.spectatorClient = bs->client;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_Seek_NBG;
	bs->ainodeText = "AINode_MP_Seek_NBG";
}

/*
==================
AINode_MP_Seek_NBG()
==================
*/
int AINode_MP_Seek_NBG( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent;
	//
	goal = bs->nearbygoal;
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	//
	// should we stop pursuing this target?
	ent = BotGetEntity( goal.entitynum );
	if ( !ent->inuse ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	} else if ( ent->s.eType == ET_SUPPLIER ) {
		if ( !ClientNeedsAmmo( bs->client ) ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
	} else if ( ent->s.eType == ET_HEALER ) {
		if ( BotHealthScale( bs->client ) >= 1.0 ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
	} else if ( ent->s.eType == ET_ITEM && ent->touch ) {
		if ( /*trap_BotTouchingGoal( bs->origin, &goal ) ||*/ ( ent->r.svFlags & SVF_NOCLIENT ) ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
	}
	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		// jump randomly
		trap_EA_Jump( bs->client );
		trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
		// fail
		bs->ainode = NULL;
		bs->ainodeText = "NULL";
		return qtrue;
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//

		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_AvoidDanger()
==================
*/
void AIEnter_MP_AvoidDanger( bot_state_t *bs ) {
	int bestarea;

	// if this is dynamite
	if ( g_entities[bs->avoid_goal.entitynum].s.eType == ET_MISSILE && g_entities[bs->avoid_goal.entitynum].methodOfDeath == MOD_DYNAMITE ) {
		if ( !( rand() % 3 ) ) {
			BotVoiceChatAfterIdleTime( bs->client, "FireInTheHole", SAY_TEAM, 500, qfalse, 3000, qfalse  );
		}
	}

	bs->flags &= ~BFL_MISCFLAG;
	if ( !( bestarea = trap_AAS_AvoidDangerArea( bs->origin, bs->areanum, bs->avoid_goal.origin, BotPointAreaNum( -1, bs->avoid_goal.origin ), bs->avoid_goal.number + 100, bs->tfl ) ) ) {                       // no hiding spot, ignore it
		bs->flags |= BFL_MISCFLAG;
	} else {
		trap_AAS_AreaWaypoint( bestarea, bs->avoid_goal.origin );
		bs->avoid_goal.areanum = bestarea;
	}

	bs->ainode = AINode_MP_AvoidDanger;
	bs->ainodeText = "AINode_MP_AvoidDanger";
}

/*
==================
AINode_MP_AvoidDanger()
==================
*/
int AINode_MP_AvoidDanger( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;
	int bestarea;
	qboolean moved = qfalse;

	memset( &moveresult, 0, sizeof( moveresult ) );
	goal = bs->avoid_goal;
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//if the target has gone
	trav = &g_entities[goal.entitynum];
	if ( !trav->inuse ) {
		// just look for a goal
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	} else if ( trav->client && trav->health <= 0 ) {
		// just look for a goal
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	} else if ( trav->s.eType == ET_CONSTRUCTIBLE ) {
		if ( ( g_entities[bs->client].client->lastConstructibleBlockingWarnEnt != goal.entitynum ) ||
			 ( ( level.time - g_entities[bs->client].client->lastConstructibleBlockingWarnTime ) > 5000 ) ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
	}

	// if the avoid entity has changed
	if ( bs->avoid_spawnCount != trav->spawnCount ) {
		// just look for a goal
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	// if the thing we're avoiding is a landmine, then we don't really need to run away from it, just take a few steps
	// don't need to move, is this a landmine?
	if ( ( trav->methodOfDeath == MOD_LANDMINE ) && VectorDistanceSquared( bs->origin, trav->r.currentOrigin ) > SQR( 256 ) ) {
		// we're done running
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}

	// make sure the current goal origin is still safe
	// is this entity dangerous?
	if ( trav->client ) {
		// is this player dangerous?
		if ( !( trav->client->ps.weapon == WP_PANZERFAUST && trav->client->ps.weaponDelay ) ) {
			// not dangerous
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
		VectorCopy( trav->r.currentOrigin, target );
	} else if ( trav->s.eType == ET_CONSTRUCTIBLE ) {
	} else {
		if ( trav->s.eType == ET_MISSILE && trav->s.weapon == WP_DYNAMITE ) {
			VectorCopy( trav->r.currentOrigin, target );
		} else {
			if ( !G_PredictMissile( trav, trav->nextthink - level.time, target, ( trav->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) ) {
				// not dangerous
				bs->ignore_specialgoal_time = 0;
				BotDefaultNode( bs );
				return qfalse;
			}
		}
		if ( ( bs->last_avoiddangerarea < level.time - 200 ) && ( VectorDistanceSquared( target, goal.origin ) < ( SQR( trav->splashRadius + 100 ) ) ) ) {
			bs->last_avoiddangerarea = level.time + rand() % 200;
			if ( !( bestarea = trap_AAS_AvoidDangerArea( bs->origin, bs->areanum, target, BotPointAreaNum( -1, target ), trav->splashRadius + 100, bs->tfl ) ) ) {
				// move away from the danger
				bs->flags |= BFL_MISCFLAG;
			} else {
				trap_AAS_AreaWaypoint( bestarea, bs->avoid_goal.origin );
				bs->avoid_goal.areanum = bestarea;
				goal = bs->avoid_goal;
			}
		}
	}
	//update goal information

	// check for emergency targets (flags, etc)
	//if (BotCheckEmergencyTargets( bs )) {
	//	return qfalse;
	//}
	if ( bs->flags & BFL_MISCFLAG ) {
		moved = qtrue;
		//initialize the movement state
		BotSetupForMovement( bs );
		// move away from danger
		VectorSubtract( target, bs->origin, dir );
		VectorNormalize( dir );
		trap_EA_Move( bs->client, dir, 400 );
		// randomly strafe also
		if ( level.time % 2000 < 1000 ) {
			trap_EA_MoveLeft( bs->client );
		} else { trap_EA_MoveRight( bs->client );}
	} else {
		// are we close enough to the goal?
		if ( VectorDistanceSquared( bs->origin, goal.origin ) > ( 24 * 24 ) ) {
			// MOVEMENT REQUIRED
			//
			moved = qtrue;
			//choose the best weapon to fight with
			BotChooseWeapon( bs );
			//initialize the movement state
			BotSetupForMovement( bs );
			//try a direct movement
			if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
				//move towards the goal
				BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
			}
			//if the movement failed
			if ( moveresult.failure ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

				// jump randomly?
				trap_EA_Jump( bs->client );
				trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
			}
			//
			BotAIBlocked( bs, &moveresult, qtrue );
		}
	}
	//
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( !moved ) {
				VectorSubtract( g_entities[goal.entitynum].r.currentOrigin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_GiveAmmo()
==================
*/
void AIEnter_MP_GiveAmmo( bot_state_t *bs ) {
//level.clients[0].sess.spectatorClient = bs->client;
	bs->give_health_time = 0;
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_GiveAmmo;
	bs->ainodeText = "AINode_MP_GiveAmmo";
}

/*
==================
AINode_MP_GiveAmmo()
==================
*/
int AINode_MP_GiveAmmo( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;

	goal = bs->target_goal;
	//if we have changed class
	if ( bs->sess.playerType != PC_FIELDOPS ) {
		BotDefaultNode( bs );
		return qfalse;
	}
	//if we have to wait
	// Gordon: FIXME: this looks wrong
	if ( bs->cur_ps.classWeaponTime > level.time - 8000 ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//if the target is dead
	trav = BotGetEntity( goal.entitynum );

	// FIXME: temp hack in dealing with NULL returns from BotGetEntity (??)
	if ( trav == NULL ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}

	if ( !trav->inuse ||
		 !trav->client ||
		 ( trav->client->ps.pm_type != PM_NORMAL ) ) {
		// let them roam again
		trav->awaitingHelpTime = 0;
		// just look for a goal
		BotDefaultNode( bs );
		return qfalse;
	}
	// do they have enough ammo now?
	if ( !ClientNeedsAmmo( trav->s.number ) ) {
		if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 72 ) ) {
			// we just helped them
			bs->last_helped_client = trav->s.number;
			bs->last_helped_time = level.time;
			// they should thank us
			if ( trav->r.svFlags & SVF_BOT ) {
				BotVoiceChatAfterIdleTime( trav->s.number, "Thanks", SAY_TEAM, 1000 + rand() % 2000, qfalse, 3000 + rand() % 2000, qfalse  );
			}
		}
		// let them roam again
		trav->awaitingHelpTime = 0;
		// just look for a goal
		BotDefaultNode( bs );
		return qfalse;
	}
	//update goal information
	VectorCopy( trav->r.currentOrigin, bs->target_goal.origin );
	bs->target_goal.areanum = BotPointAreaNum( trav->s.number, trav->r.currentOrigin );
	if ( !bs->target_goal.areanum ) {
		BotDefaultNode( bs );
		return qfalse;
	}
	goal = bs->target_goal;
	if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 100 ) && BotEntityVisible( bs->client, bs->eye, bs->viewangles, 360, trav->s.number, NULL ) ) {
		// make sure other bots dont head for this target also
		trav->awaitingHelpTime = level.time + 1500;
	}

	// are we close enough to the goal?
	if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 72 ) ) {
		if ( !bs->give_health_time ) {
			bs->give_health_time = level.time + 8000;
		} else if ( bs->give_health_time < level.time ) {
			BotDefaultNode( bs );
			return qfalse;
		}
		// make sure other bots dont head for this target also
		trav->awaitingHelpTime = level.time + 1500;
		trav->botIgnoreAmmoTime = level.time + 1000;
		// switch to regen and pump away
		bs->weaponnum = WP_AMMO;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );
		// aim directly at the dynamite
		VectorCopy( bs->origin, target );
		VectorSubtract( trav->r.currentOrigin, target, dir );
		dir[2] *= 0.5;
		VectorNormalize( dir );
		vectoangles( dir, bs->ideal_viewangles );
		// hold fire
		if ( bs->cur_ps.weapon == WP_AMMO && BotWeaponCharged( bs, WP_AMMO ) ) {
			trap_EA_Attack( bs->client );
		}
		//
		return qtrue;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	// MOVEMENT REQUIRED
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

		// jump randomly?
		trap_EA_Jump( bs->client );
		trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )]
											  < (int)( 0.8 * ( GetAmmoTableData( bs->cur_ps.weapon ) )->maxclip ) )
		 && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_MedicGiveHealth()
==================
*/
void AIEnter_MP_MedicGiveHealth( bot_state_t *bs ) {
	bs->give_health_time = 0;
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_MedicGiveHealth;
	bs->ainodeText = "AINode_MP_MedicGiveHealth";
}

/*
==================
AINode_MP_MedicGiveHealth()
==================
*/
int AINode_MP_MedicGiveHealth( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;

	goal = bs->target_goal;

	//if we have changed class
	if ( bs->sess.playerType != PC_MEDIC ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	//if the target has full health
	trav = &g_entities[goal.entitynum];

	if ( !trav->inuse || !trav->client || ( trav->client->ps.pm_type != PM_NORMAL ) || ( trav->client->ps.pm_flags & PMF_LIMBO ) ) {
		// let them roam again
		trav->awaitingHelpTime = 0;

		// just look for a goal
		BotDefaultNode( bs );
		return qfalse;
	}

	if ( BotHealthScale( trav->s.number ) >= 1.0 ) {
		if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 72 ) ) {
			// we just helped them
			bs->last_helped_client = trav->s.number;
			bs->last_helped_time = level.time;

			// they should thank us
			if ( trav->r.svFlags & SVF_BOT ) {
				BotVoiceChatAfterIdleTime( trav->s.number, "Thanks", SAY_TEAM, 1000 + rand() % 2000, qfalse, 3000 + rand() % 2000, qfalse  );
			}
		}

		// let them roam again
		trav->awaitingHelpTime = 0;

		// just look for a goal
		BotDefaultNode( bs );
		return qfalse;
	}

	//update goal information
	VectorCopy( trav->r.currentOrigin, BotGetOrigin( trav->s.number ) );
	bs->target_goal.areanum = BotGetArea( trav->s.number );
	if ( !bs->target_goal.areanum ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	goal = bs->target_goal;

	// are we close enough to the goal?
	if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 72 ) ) {
		if ( !bs->give_health_time ) {
			bs->give_health_time = level.time + 8000;
		} else if ( bs->give_health_time < level.time ) {
			BotDefaultNode( bs );
			return qfalse;
		}
		// make sure other bots dont head for this target also
		trav->awaitingHelpTime = level.time + 1500;
		trav->botIgnoreHealthTime = level.time + 1000;

		// switch to regen and pump away
		bs->weaponnum = WP_MEDKIT;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );

		// aim directly at the player
		VectorCopy( bs->origin, target );
		VectorSubtract( trav->r.currentOrigin, target, dir );
		dir[2] *= 0.5;
		VectorNormalize( dir );
		vectoangles( dir, bs->ideal_viewangles );

		// hold fire
		if ( bs->cur_ps.weapon == WP_MEDKIT && BotWeaponCharged( bs, WP_MEDKIT ) ) {
			trap_EA_Attack( bs->client );
		}

		return qtrue;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}

	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}

	// MOVEMENT REQUIRED
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	//initialize the movement state
	BotSetupForMovement( bs );

	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

		// jump randomly?
		trap_EA_Jump( bs->client );
		trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_MedicRevive()
==================
*/
void AIEnter_MP_MedicRevive( bot_state_t *bs ) {
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_MedicRevive;
	bs->ainodeText = "AINode_MP_MedicRevive";
}

/*
==================
AINode_MP_MedicRevive()
==================
*/
int AINode_MP_MedicRevive( bot_state_t *bs ) {
	bot_goal_t goal, target;
	vec3_t targetpos, dir;
	bot_moveresult_t moveresult;
	int range;
	gentity_t *trav;

	goal = bs->target_goal;
	//if we have changed class
	if ( bs->sess.playerType != PC_MEDIC ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//if the target is not dead or is in limbo
	trav = BotGetEntity( goal.entitynum );

	if (
		// START	Gordon changes, 23/8/2002
		!trav ||
		// END		Gordon changes, 23/8/2002
		!trav->inuse ||
		!trav->client ||
		( trav->client->ps.pm_flags & PMF_LIMBO ) ) {
		// just look for a goal
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	// if someone is close to them, only they should continue reviving
	if ( g_entities[goal.entitynum].botIgnoreHealthTime >= level.time ) {
		int list[MAX_CLIENTS], numList;
		float ourDist, *distances;
		//
		// if there are too many defenders, and we are the furthest away, stop defending
		if ( ( numList = BotNumTeamMatesWithTarget( bs, goal.entitynum, list, MAX_CLIENTS ) ) > 0 ) {
			ourDist = VectorDistanceSquared( bs->origin, g_entities[goal.entitynum].r.currentOrigin );
			if ( !trap_InPVS( bs->origin, g_entities[goal.entitynum].r.currentOrigin ) ) {
				ourDist += ( 2048 * 2048 );
			}
			distances = BotSortPlayersByDistance( g_entities[goal.entitynum].r.currentOrigin, list, numList );
			if ( distances[numList - 1] < ourDist ) {
				// we are the furthest
				bs->ignore_specialgoal_time = 0;
				bs->leader = -1;
				BotDefaultNode( bs );
				return qfalse;
			}
		}
	}
	// if they are alive
	if ( trav->client->ps.pm_type != PM_DEAD ) {
		if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 72 ) ) {
			// we just helped them
			bs->last_helped_client = trav->s.number;
			bs->last_helped_time = level.time;
			// they should thank us
			if ( trav->r.svFlags & SVF_BOT ) {
				BotVoiceChatAfterIdleTime( trav->s.number, "Thanks", SAY_TEAM, 1000 + rand() % 2000, qfalse, 4000 + rand() % 2000, qfalse  );
			}
		}
		// look for someone to heal (like the person we just revived)
		if ( BotClass_MedicCheckGiveHealth( bs, 200, &target ) ) {
			bs->target_goal = target;
			AIEnter_MP_MedicGiveHealth( bs );
			return qtrue;
		}
		// just look for a goal
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//update goal information
	VectorCopy( trav->r.currentOrigin, bs->target_goal.origin );
	bs->target_goal.areanum = BotPointAreaNum( trav->s.number, trav->r.currentOrigin );
	if ( !bs->target_goal.areanum ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	goal = bs->target_goal;
	//look for closer revives
	range = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, goal.areanum, bs->tfl );
	//
	if ( range < 200 && bs->enemy < 0 ) {
		// switch to regen
		bs->weaponnum = WP_MEDIC_SYRINGE;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );
		// make sure other bots dont head for this target also
		g_entities[goal.entitynum].botIgnoreHealthTime = level.time + 500;
	}

	// are we close enough to the goal?
	if ( VectorDistanceSquared( bs->eye, goal.origin ) < SQR( 42 ) ) {
		// make sure other bots dont head for this target also
		g_entities[goal.entitynum].botIgnoreHealthTime = level.time + 1500;
		// crouch down
		trap_EA_Crouch( bs->client );
		// switch to regen and pump away
		bs->weaponnum = WP_MEDIC_SYRINGE;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );
		// aim directly at the dynamite
		VectorCopy( bs->origin, targetpos );
		targetpos[2] += bs->cur_ps.viewheight;
		VectorSubtract( trav->r.currentOrigin, targetpos, dir );
		VectorNormalize( dir );
		vectoangles( dir, bs->ideal_viewangles );
		// hold fire
		if ( bs->cur_ps.weapon == WP_MEDIC_SYRINGE ) {
			trap_EA_Attack( bs->client );
		}
		//
		return qtrue;
	}
	//
	if ( range > 200 ) {
		// check for emergency targets (flags, etc)
		if ( BotCheckEmergencyTargets( bs ) ) {
			return qfalse;
		}
		// check for dangerous elements
		if ( BotDangerousGoal( bs, &goal ) ) {
			AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
			return qfalse;
		}
	}
	//
	// MOVEMENT REQUIRED
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		// jump randomly?
		trap_EA_Jump( bs->client );
		trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, targetpos );
			VectorSubtract( targetpos, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, targetpos ) ) {
				VectorSubtract( targetpos, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, targetpos );
				VectorSubtract( targetpos, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_PanzerTarget()
==================
*/
void AIEnter_MP_PanzerTarget( bot_state_t *bs ) {
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_PanzerTarget;
	bs->ainodeText = "AINode_MP_PanzerTarget";
	bs->enemy = ENTITYNUM_WORLD;    // fast view
}

/*
==================
AINode_MP_PanzerTarget()
==================
*/
int AINode_MP_PanzerTarget( bot_state_t *bs ) {
	vec3_t vec;
	//
	if ( BotIsDead( bs ) ) {
		bs->enemy = -1;
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//
	if ( !BotWeaponWantScale( bs, WP_PANZERFAUST ) ) {
		bs->enemy = -1;
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}

	bs->weaponnum = WP_PANZERFAUST;
	trap_EA_SelectWeapon( bs->client, bs->weaponnum );

	//
	// have we fired already?
	if ( !BotWeaponCharged( bs, bs->weaponnum ) ) {
		bs->enemy = -1;
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}

	// look at the target
	VectorSubtract( bs->target_goal.origin, bs->eye, vec );
	VectorNormalize( vec );
	vectoangles( vec, bs->ideal_viewangles );
	//
	if (    ( bs->cur_ps.weapon == bs->weaponnum )
			&&  ( AngleDifference( bs->ideal_viewangles[YAW], bs->viewangles[YAW] ) < 0.5 )
			&&  ( AngleDifference( bs->ideal_viewangles[PITCH], bs->viewangles[PITCH] ) < 0.5 ) ) {
		trap_EA_Attack( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_AttackTarget()
==================
*/
void AIEnter_MP_AttackTarget( bot_state_t *bs ) {
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_AttackTarget;
	bs->ainodeText = "AINode_MP_AttackTarget";
	bs->enemy = bs->target_goal.entitynum;  // fast view
}

/*
==================
AINode_MP_AttackTarget()
==================
*/
int AINode_MP_AttackTarget( bot_state_t *bs ) {
	vec3_t vec;
	bot_goal_t goal;
	gentity_t *check;
	//
	goal = bs->target_goal;
	bs->weaponnum = BotBestTargetWeapon( bs, goal.entitynum );
	if ( bs->weaponnum == WP_NONE ) {
		bs->enemy = -1;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	if ( BotIsDead( bs ) ) {
		bs->enemy = -1;
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//
	if ( BotIsObserver( bs ) ) {
		bs->enemy = -1;
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		bs->enemy = -1;
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		bs->enemy = -1;
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	//
	check = BotGetVisibleDamagableScriptMover( bs );
	if ( !check || ( check->s.number != goal.entitynum ) ) {
		bs->enemy = -1;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	trap_EA_SelectWeapon( bs->client, bs->weaponnum );
	// look at the target
	VectorSubtract( bs->target_goal.origin, bs->eye, vec );
	VectorNormalize( vec );
	vectoangles( vec, bs->ideal_viewangles );
	//
	if (    ( bs->cur_ps.weapon == bs->weaponnum )
			&&  ( AngleDifference( bs->ideal_viewangles[YAW], bs->viewangles[YAW] ) < 0.5 )
			&&  ( AngleDifference( bs->ideal_viewangles[PITCH], bs->viewangles[PITCH] ) < 0.5 ) ) {
		if ( bs->cur_ps.weapon == WP_GRENADE_LAUNCHER || bs->cur_ps.weapon == WP_GRENADE_PINEAPPLE ) {
			if ( BotSinglePlayer() || BotCoop() ) {
				// release immediately in single player
			} else if ( bs->cur_ps.grenadeTimeLeft ) {
				// release grenade
			} else {
				// hold onto it
				trap_EA_Attack( bs->client );
			}
		} else {
			trap_EA_Attack( bs->client );
		}
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_FixMG42()
==================
*/
void AIEnter_MP_FixMG42( bot_state_t *bs ) {
//level.clients[0].sess.spectatorClient = bs->client;
	bs->arrive_time = level.time;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_FixMG42;
	bs->ainodeText = "AINode_MP_FixMG42";
}

/*
==================
AINode_MP_FixMG42()
==================
*/
int AINode_MP_FixMG42( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent;
	trace_t tr;
	//
	goal = bs->target_goal;
	ent = BotGetEntity( bs->target_goal.entitynum );
	// return to this sniper spot if we go off temporarily
	ent->botIgnoreTime = 0;
	//
	if ( ent->melee->takedamage ) {
		bs->ainode = NULL;
		bs->ainodeText = "NULL";
		return qtrue;
	}
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		ent->botIgnoreTime = level.time + 5000; // other bots should avoid this spot
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		// return to this sniper spot after we're done
		ent->botIgnoreTime = 0;
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		// return to this sniper spot after we're done
		ent->botIgnoreTime = 0;
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}

	// have we been waiting here for too long?
	if ( bs->arrive_time < level.time - 40000 ) {
		ent->botIgnoreTime = level.time + 5000; // other bots should avoid this spot
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	//
	// other bots should avoid this spot
	ent->botIgnoreTime = level.time + 5000;

	VectorSubtract( bs->origin, goal.origin, dir );
	if ( fabs( dir[2] ) < 100 ) {
		dir[2] = 0;
	}

	// is the destination blocked?
	if ( VectorLengthSquared( dir ) < SQR( 64 ) ) {
		trap_Trace( &tr, ent->s.origin, NULL, NULL, ent->s.origin, bs->client, MASK_PLAYERSOLID );
		if ( tr.startsolid || tr.allsolid ) {
			if ( BotFindSpecialGoals( bs ) ) {
				return qfalse;
			}
		}
	}

	if ( VectorLengthSquared( dir ) > SQR( 8 ) ) {
		//choose the best weapon to fight with
		BotChooseWeapon( bs );
		//
		bs->arrive_time = level.time;   // wait a bit longer

		//initialize the movement state
		BotSetupForMovement( bs );
		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}

		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
			if ( BotFindSpecialGoals( bs ) ) {
				return qfalse;
			}
			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
			// fail
			bs->ainode = NULL;
			bs->ainodeText = "NULL";
			return qtrue;
		}

		BotAIBlocked( bs, &moveresult, qtrue );

		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		} else if ( moveresult.flags & MOVERESULT_WAITING ) { //if waiting for something
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
			if ( VectorDistanceSquared( bs->origin, goal.origin ) > SQR( 32 ) ) {
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
							bs->enemyposition_time = 0;
						}
					} else {
						if ( EntityIsDead( &entinfo ) ) {
							bs->enemydeath_time = trap_AAS_Time();
						}
					}

					if ( bs->enemy >= 0 ) {
						// attack and keep moving
						if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
							//choose the best weapon to fight with
							BotChooseWeapon( bs );
							//aim at the enemy
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

				if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
					if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
						VectorSubtract( goal.origin, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( VectorLengthSquared( moveresult.movedir ) ) { //FIXME: look at cluster portals?
						vectoangles( moveresult.movedir, bs->ideal_viewangles );
					} else if ( random() < bs->thinktime * 0.8 ) {
						BotRoamGoal( bs, target );
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
						bs->ideal_viewangles[2] *= 0.5;
					}
					bs->ideal_viewangles[2] *= 0.5;
				}
			}
		}
	}

	if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 16 ) ) {
		// We are at the spot, so start fixing
		bs->weaponnum = WP_PLIERS;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );

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
					bs->enemyposition_time = 0;
				}
			} else {
				if ( EntityIsDead( &entinfo ) ) {
					bs->enemydeath_time = trap_AAS_Time();
				}
			}
			//
			if ( bs->enemy >= 0 ) {
				if ( VectorDistanceSquared( bs->origin, bs->enemyorigin ) < SQR( 512 ) ) {
					// if they are real close, abort sniper mode
					if ( VectorDistanceSquared( bs->origin, g_entities[bs->enemy].r.currentOrigin ) < SQR( 1024 ) ) {
						AIEnter_MP_Battle_Fight( bs );
						return qfalse;
					}
				} else {
					bs->enemy = -1;
				}
			}
		} else {
			// face the mg42 and start repairing
			VectorSubtract( ent->melee->r.currentOrigin, bs->eye, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
			// hold fire
			trap_EA_Attack( bs->client );
			// dont abort until finished
			bs->arrive_time = level.time;
		}
	} else if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) { // reload?
		trap_EA_Reload( bs->client );
	}

	return qtrue;
}

/*
==================
AIEnter_MP_Battle_MobileMG42
==================
*/
void AIEnter_MP_Battle_MobileMG42( bot_state_t *bs ) {
	BotDebugViewClient( bs->client );
	//
	bs->arrive_time = level.time;
	bs->lasthealth = g_entities[bs->client].health;
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_Battle_MobileMG42;
	bs->ainodeText = "AINode_MP_Battle_MobileMG42";
}

/*
==================
AINode_MP_Battle_MobileMG42()
==================
*/
int AINode_MP_Battle_MobileMG42( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t dir, ang;
	int tookDamage = 0;

	bs->weaponnum = WP_MOBILE_MG42;
	trap_EA_SelectWeapon( bs->client, bs->weaponnum );
	bs->mobileMG42ProneTime = level.time;

	// if our health has dropped, abort
	tookDamage = bs->lasthealth - g_entities[bs->client].health;
//	bs->lasthealth = g_entities[bs->client].health;

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	// if we have run out of ammo
	if ( !BotGotEnoughAmmoForWeapon( bs, bs->weaponnum ) ) {
		BotDefaultNode( bs );
		return qtrue;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}

	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}

	// look for something better to do
	if ( level.captureFlagMode && BotFlagAtBase( bs->sess.sessionTeam, NULL ) == qfalse ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	// have we been waiting here for too long? or been hit too much?
	if ( tookDamage > 20 || g_entities[bs->client].health < 40 || bs->arrive_time < level.time - 5000 ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	BotAimAtEnemy( bs );
	// wait until we are facing them correctly before going prone
	if ( !( bs->cur_ps.eFlags & EF_PRONE ) && ( fabs( AngleDifference( bs->ideal_viewangles[YAW], bs->viewangles[YAW] ) ) > 5.0f ) ) {
		return qtrue;
	}

	// stay prone
	trap_EA_Prone( bs->client );

	// look for enemies
	// if we have an enemy, make sure they are still within view limits
	if ( bs->enemy >= 0 ) {
		if ( !BotEntityWithinView( bs, bs->enemy ) ) {
			bs->enemy = -1;
		}
	}

	if ( bs->enemy < 0 ) {
		BotFindEnemyMP( bs, -1, qfalse );
	}

	if ( bs->enemy < 0 ) {
		// if we still have no enemy, then after a slight pause, allow us to escape prone mode if we find an
		//	enemy outside our view
		if ( bs->arrive_time < level.time - 1500 ) {
			BotFindEnemyMP( bs, -1, qtrue );
			// if we found one this time, escape prone mode
			if ( bs->enemy >= 0 ) {
				AIEnter_MP_Battle_Fight( bs );
				return qfalse;
			}
		}
	}

	// if we took damage, check that there isn't a closer enemy we can attack
	if ( tookDamage > 0 ) {
		int oldEnemy;
		//
		oldEnemy = bs->enemy;
		bs->enemy = -1;
		BotFindEnemyMP( bs, -1, qtrue );
		if ( bs->enemy >= 0 && bs->enemy != oldEnemy ) {
			// found someone else to attack
			AIEnter_MP_Battle_Fight( bs );
			return qfalse;
		}
		// otherwise continue
		bs->enemy = oldEnemy;
	}

	if ( bs->enemy >= 0 ) {
		aas_entityinfo_t entinfo;
		BotEntityInfo( bs->enemy, &entinfo );
		//if the enemy is dead
		if ( bs->enemydeath_time ) {
			if ( bs->enemydeath_time < trap_AAS_Time() - 0.3 ) {
				bs->enemydeath_time = 0;
				bs->enemy = -1;
				bs->enemyposition_time = 0;
			}
		} else {
			if ( EntityIsDead( &entinfo ) ) {
				bs->enemydeath_time = trap_AAS_Time();
			}
		}

		if ( bs->enemy >= 0 ) {
			VectorSubtract( entinfo.origin, bs->eye, dir );
			VectorNormalize( dir );
			vectoangles( dir, ang );
			if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 120, bs->enemy, NULL ) ) {
				// if they are real close, abort mg42 mode
				if ( VectorDistanceSquared( bs->origin, g_entities[bs->enemy].r.currentOrigin ) < SQR( 400 ) ) {
					AIEnter_MP_Battle_Fight( bs );
					return qfalse;
				}
				//
				bs->arrive_time = level.time;   // wait a bit longer
				//aim at the enemy
				BotAimAtEnemy( bs );
				//attack the enemy if possible
				trap_EA_Attack( bs->client );
			} else {
				bs->enemy = -1;
			}
		}
	} else {
//		int spotNum;
		//
		// TODO: cycle through visible enemy sniper spots
		// NOTE: remember last visible enemy positions, so we can stay on them longer than usual
		if ( bs->enemyposition_time > trap_AAS_Time() - 5.0 ) {
			VectorSubtract( bs->enemyorigin, bs->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
		} else if ( bs->viewchangetime > level.time ) {
			// use same angles
/*		} else if ((spotNum = BotGetRandomVisibleSniperSpot( bs )) > -1) {
			// look at new spot
			VectorSubtract( g_entities[spotNum].s.origin, bs->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
			//
			bs->viewchangetime = level.time + 1000 + rand()%1500;
*/      } else {
			// use mg42 angles
			VectorCopy( level.clients[bs->client].pmext.mountedWeaponAngles, bs->ideal_viewangles );
			// add some random angle
			bs->ideal_viewangles[YAW] += crandom() * 20.f * 0.45;
			bs->ideal_viewangles[PITCH] += crandom() * 20.f * 0.15;
			//
			bs->viewchangetime = level.time + 1000 + rand() % 1500;
		}
	}

	//
	// stay mounted
	bs->flags &= ~BFL_DISMOUNT_MG42;

	return qtrue;
}

/*
==================
AIEnter_MP_MG42Scan()
==================
*/
void AIEnter_MP_MG42Scan( bot_state_t *bs ) {
//level.clients[0].sess.spectatorClient = bs->client;
	bs->arrive_time = level.time;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->lasthealth = g_entities[bs->client].health;
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_MG42Scan;
	bs->ainodeText = "AINode_MP_MG42Scan";
}

/*
==================
AINode_MP_MG42Scan()
==================
*/
int AINode_MP_MG42Scan( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t dir, ang;
	gentity_t *ent, *mg42;
	//
	// set the dismount flag. clear it if we want to stay on
	bs->flags |= BFL_DISMOUNT_MG42;
	//
	goal = bs->target_goal;
	ent = BotGetEntity( bs->target_goal.entitynum );
	mg42 = ent->melee;
	//
	ent->botIgnoreTime = 0;     // set this now, only reset it to ignore if we want to ignore it
	//
	// if it's the wrong one
	if ( VectorDistanceSquared( bs->origin, ent->r.currentOrigin ) > SQR( 64 ) ) {
		trap_EA_Activate( bs->client );
		return qfalse;
	}
	// return to this sniper spot if we go off temporarily
	ent->botIgnoreTime = 0;
	// if our health has dropped, abort
	if ( bs->lasthealth > g_entities[bs->client].health + 40 ) {
		ent->botIgnoreTime = level.time + 5000;
		bs->ainode = NULL;
		bs->ainodeText = "NULL";
		return qtrue;
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// if we are not mounted on an mg42
	if ( !( g_entities[bs->client].s.eFlags & EF_MG42_ACTIVE ) ) {
		bs->ainode = NULL;
		bs->ainodeText = "NULL";
		return qtrue;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		// return to this sniper spot after we're done
		ent->botIgnoreTime = 0;
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		// return to this sniper spot after we're done
		ent->botIgnoreTime = 0;
		//
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
/*	// check for special class actions
	if (BotCheckClassActions(bs)) {
		// return to this sniper spot after we're done
		ent->missionLevel = 0;
		return qfalse;
	}
*/                                                                                                                                                                      // look for something better to do
	if ( BotFlagAtBase( bs->sess.sessionTeam, NULL ) == qfalse ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	// have we been waiting here for too long?
	if ( bs->arrive_time < level.time - 40000 ) {
		ent->missionLevel = level.time; // other bots should avoid this spot
		ent->botIgnoreTime = level.time + 20000;    // we should ignore it for a while now
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	// if this spot is disabled now
	if ( mg42->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	//
	// other bots should avoid this spot
	if ( ent->botIgnoreTime < level.time + 5000 ) {
		ent->botIgnoreTime = level.time + 5000;
	}
	//
	// look for enemies
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
				bs->enemyposition_time = 0;
			}
		} else {
			if ( EntityIsDead( &entinfo ) ) {
				bs->enemydeath_time = trap_AAS_Time();
			}
		}
		//
		if ( bs->enemy >= 0 ) {
			VectorSubtract( entinfo.origin, mg42->r.currentOrigin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ang );
			if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 120, bs->enemy, NULL ) ) {

				if (    ( fabs( AngleDifference( ang[PITCH], mg42->s.angles[PITCH] ) ) >= mg42->varc )
						||  ( fabs( AngleDifference( ang[YAW], mg42->s.angles[YAW] ) ) >= mg42->harc ) ) {
					ent->botIgnoreTime = level.time + 5000;
					AIEnter_MP_Battle_Fight( bs );
					return qfalse;
				}

				// if they are real close, abort mg42 mode
				if ( VectorDistanceSquared( bs->origin, g_entities[bs->enemy].r.currentOrigin ) < SQR( 512 ) ) {
					ent->botIgnoreTime = level.time + 5000;
					AIEnter_MP_Battle_Fight( bs );
					return qfalse;
				}

				//
				bs->arrive_time = level.time;   // wait a bit longer
				//aim at the enemy
				BotAimAtEnemy( bs );
				//attack the enemy if possible
				trap_EA_Attack( bs->client );
			} else {
				bs->enemy = -1;
			}
		}
	} else {
//		int spotNum;
		//
		// TODO: cycle through visible enemy sniper spots
		// NOTE: remember last visible enemy positions, so we can stay on them longer than usual
		if ( bs->enemyposition_time > trap_AAS_Time() - 5.0 ) {
			VectorSubtract( bs->enemyorigin, bs->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
		} else if ( bs->viewchangetime > level.time ) {
			// use same angles
/*		} else if ((spotNum = BotGetRandomVisibleSniperSpot( bs )) > -1) {
			// look at new spot
			VectorSubtract( g_entities[spotNum].s.origin, bs->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
			//
			bs->viewchangetime = level.time + 1000 + rand()%1500;
*/      } else {
			vec3_t start, end;
			trace_t tr;
			// use mg42 angles
			VectorCopy( mg42->s.angles, bs->ideal_viewangles );
			// add some random angle
			bs->ideal_viewangles[YAW] += crandom() * mg42->harc * 0.45;
			bs->ideal_viewangles[PITCH] += crandom() * mg42->varc * 0.15;
			//
			// trace out to get the ground
			AngleVectors( bs->ideal_viewangles, dir, NULL, NULL );
			VectorMA( mg42->r.currentOrigin, 48, dir, start );
			VectorMA( start, 4096, dir, end );
			trap_Trace( &tr, start, NULL, NULL, end, bs->client, MASK_SHOT );
			if ( tr.fraction > 0.2 ) {
				VectorCopy( tr.endpos, start );
				VectorCopy( tr.endpos, end );
				end[2] -= 1024 * tr.fraction;
				trap_Trace( &tr, start, NULL, NULL, end, bs->client, MASK_SHOT );
				VectorSubtract( tr.endpos, bs->eye, dir );
				VectorNormalize( dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//
			bs->viewchangetime = level.time + 1000 + rand() % 1500;
		}
	}
	//
	// stay mounted
	bs->flags &= ~BFL_DISMOUNT_MG42;
	//
	return qtrue;
}

/*
==================
AIEnter_MP_MG42Mount()
==================
*/
void AIEnter_MP_MG42Mount( bot_state_t *bs ) {
//level.clients[0].sess.spectatorClient = bs->client;
	bs->arrive_time = level.time;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_MG42Mount;
	bs->ainodeText = "AINode_MP_MG42Mount";
}

/*
==================
AINode_MP_MG42Mount()
==================
*/
int AINode_MP_MG42Mount( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent;
	trace_t tr;

	goal = bs->target_goal;
	ent = BotGetEntity( bs->target_goal.entitynum );
	// return to this sniper spot if we go off temporarily
	ent->botIgnoreTime = 0;
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// if we have mounted an mg42
	if ( g_entities[bs->client].s.eFlags & EF_MG42_ACTIVE ) {
		// if it's the wrong one
		if ( VectorDistanceSquared( bs->origin, ent->r.currentOrigin ) > SQR( 64 ) ) {
			trap_EA_Activate( bs->client );
			return qfalse;
		}
		AIEnter_MP_MG42Scan( bs );
		return qfalse;
	}
	// if the mg42 is broken, or being used
	if ( ent->melee->health <= 0 || ( ent->melee->entstate != STATE_DEFAULT ) || ent->melee->active ) {
		bs->ainode = NULL;
		bs->ainodeText = "NULL";
		return qtrue;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		//
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
/*	// check for special class actions
	if (BotCheckClassActions(bs)) {
		return qfalse;
	}
*/                                                                                            // look for something better to do
	if ( BotFindSpecialGoals( bs ) ) {
		return qfalse;
	}
	// have we been waiting here for too long?
	if ( bs->arrive_time < level.time - 40000 ) {
		ent->botIgnoreTime = level.time + 15000;    // other bots should avoid this spot
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	//
	// other bots should avoid this spot
	ent->botIgnoreTime = level.time + 5000;
	//
	VectorSubtract( bs->origin, goal.origin, dir );
	if ( fabs( dir[2] ) < 100 ) {
		dir[2] = 0;
	}
	// is the destination blocked?
	if ( VectorLengthSquared( dir ) < SQR( 64 ) ) {
		trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, ent->r.currentOrigin, bs->client, MASK_PLAYERSOLID );
		if ( tr.startsolid || tr.allsolid ) {
			if ( BotFindSpecialGoals( bs ) ) {
				return qfalse;
			}
		}
	}
	if ( VectorLengthSquared( dir ) > SQR( 8 ) ) {
		//choose the best weapon to fight with
		BotChooseWeapon( bs );
		//
		bs->arrive_time = level.time;   // wait a bit longer

		//initialize the movement state
		BotSetupForMovement( bs );
		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}
		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

			if ( BotFindSpecialGoals( bs ) ) {
				return qfalse;
			}

			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
			// fail
			bs->ainode = NULL;
			bs->ainodeText = "NULL";
			return qtrue;
		}
		//
		BotAIBlocked( bs, &moveresult, qtrue );
		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		}
		//if waiting for something
		else if ( moveresult.flags & MOVERESULT_WAITING ) {
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
			if ( VectorDistanceSquared( bs->origin, goal.origin ) > SQR( 32 ) ) {
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
							bs->enemyposition_time = 0;
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
							if ( VectorDistanceSquared( bs->origin, goal.origin ) > SQR( 128 ) ||
								 VectorDistanceSquared( bs->origin, g_entities[bs->enemy].r.currentOrigin ) > SQR( 256 ) ) {
								//choose the best weapon to fight with
								BotChooseWeapon( bs );
								//aim at the enemy
								BotAimAtEnemy( bs );
								//attack the enemy if possible
								if ( bs->weaponnum == bs->cur_ps.weapon ) {
									BotCheckAttack( bs );
								}
							} else {    // go fight them
								AIEnter_MP_Battle_Fight( bs );
								return qfalse;
							}
						} else {
							bs->enemy = -1;
						}
					}
				}
				//
				if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
					if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
						VectorSubtract( goal.origin, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					}
					//FIXME: look at cluster portals?
					else if ( VectorLengthSquared( moveresult.movedir ) ) {
						vectoangles( moveresult.movedir, bs->ideal_viewangles );
					} else if ( random() < bs->thinktime * 0.8 )     {
						BotRoamGoal( bs, target );
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
						bs->ideal_viewangles[2] *= 0.5;
					}
					bs->ideal_viewangles[2] *= 0.5;
				}
			}
		}

	}
	//
	if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 64 ) ) {
		vec3_t forward, right, up, offset;
		// look at the gun
		AngleVectors( bs->viewangles, forward, right, up );
		CalcMuzzlePointForActivate( &g_entities[bs->client], forward, right, up, offset );
		VectorSubtract( ent->melee->r.currentOrigin, offset, dir );
		VectorNormalize( dir );
		vectoangles( dir, bs->ideal_viewangles );
		if ( bs->arrive_time < level.time - 500 ) {
			// randomize the angles a bit to solve some times when a direct sight isn't "enough"
			if ( rand() % 4 == 0 ) {
				bs->ideal_viewangles[YAW] += crandom() * 10.0;
				bs->ideal_viewangles[PITCH] += crandom() * 10.0;
			}
			// if we have been waiting longer, move backwards slowly
			if ( bs->arrive_time < level.time - 2000 ) {
				VectorInverse( dir );
				dir[2] = 0;
				VectorNormalize( dir );
				trap_EA_Move( bs->client, dir, 40 );
			}
		}
		// hit activate so we mount it
		if ( rand() % 2 ) {
			trap_EA_Activate( bs->client );
		}
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_ScanForLandmines()
==================
*/
void AIEnter_MP_ScanForLandmines( bot_state_t *bs ) {
	bs->arrive_time = level.time;
	bs->altenemy = 0;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_ScanForLandmines;
	bs->ainodeText = "AINode_MP_ScanForLandmines";
}

#define DETECT_RADIUS 225

/*
==================
AINode_MP_ScanForLandmines()
==================
*/
int AINode_MP_ScanForLandmines( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent;
	trace_t tr;

	goal = bs->target_goal;
	ent = &g_entities[bs->target_goal.entitynum];

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		ent->missionLevel = level.time;
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		ent->missionLevel = level.time;
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		ent->missionLevel = level.time;
		return qfalse;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		ent->missionLevel = level.time;
		return qfalse;
	}

	// check for dangerous elements
	if ( BotDangerousGoal( bs, &bs->target_goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		ent->missionLevel = level.time;
		return qfalse;
	}

	// is this spot disabled now?
	if ( ent->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
		if ( BotFindSpecialGoals( bs ) ) {
			ent->missionLevel = level.time;
			return qfalse;
		}
	}

	// is the destination blocked?
	trap_Trace( &tr, ent->s.origin, NULL, NULL, ent->s.origin, bs->client, MASK_PLAYERSOLID );
	if ( tr.startsolid || tr.allsolid ) {
		if ( BotFindSpecialGoals( bs ) ) {
			ent->missionLevel = level.time;
			return qfalse;
		}
	}

	VectorSubtract( bs->origin, goal.origin, dir );
	if ( fabs( dir[2] ) < 16 ) {
		dir[2] = 0;
	}

	if ( VectorLengthSquared( dir ) > SQR( 32 ) ) {
		//choose the best weapon to fight with
		BotChooseWeapon( bs );

		bs->arrive_time = level.time;   // wait a bit longer

		//initialize the movement state
		BotSetupForMovement( bs );

		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}
		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
			if ( BotFindSpecialGoals( bs ) ) {
				return qfalse;
			}
			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
			// fail
			bs->ainode = NULL;
			bs->ainodeText = "NULL";
			return qtrue;
		}

		BotAIBlocked( bs, &moveresult, qtrue );

		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		} else if ( moveresult.flags & MOVERESULT_WAITING ) { //if waiting for something
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
			if ( VectorDistanceSquared( bs->origin, goal.origin ) > SQR( 32 ) ) {
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
							bs->enemyposition_time = 0;
						}
					} else {
						if ( EntityIsDead( &entinfo ) ) {
							bs->enemydeath_time = trap_AAS_Time();
						}
					}

					if ( bs->enemy >= 0 ) {
						// attack and keep moving
						if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
							//choose the best weapon to fight with
							BotChooseWeapon( bs );
							//aim at the enemy
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

				if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
					if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
						VectorSubtract( goal.origin, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( VectorLengthSquared( moveresult.movedir ) ) { //FIXME: look at cluster portals?
						vectoangles( moveresult.movedir, bs->ideal_viewangles );
					} else if ( random() < bs->thinktime * 0.8 ) {
						BotRoamGoal( bs, target );
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
						bs->ideal_viewangles[2] *= 0.5;
					}
					bs->ideal_viewangles[2] *= 0.5;
				}
			}
		}
	}

	if ( VectorLengthSquared( dir ) < SQR( 32 ) ) {
		// start zooming if we aren't already
		bs->weaponnum = WP_BINOCULARS;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );

		if ( bs->altenemy <= 0 ) {
			vec3_t target;

			if ( bs->target_goal.number >= 3 ) {
				BotChooseWeapon( bs );
				BotDefaultNode( bs );
				ent->missionLevel = level.time;
				return qfalse;
			}

			// we're going to modify the point we're looking at, based on the current counter number
			VectorCopy( ent->s.origin2, target );

			// if our index is 0 or 2, we move it left
			switch ( bs->target_goal.number ) {
			case 0:
				target[1] += DETECT_RADIUS;
				break;
			case 1:
				target[0] += DETECT_RADIUS;
				break;
			case 2:
				target[0] -= DETECT_RADIUS;
				target[1] -= DETECT_RADIUS;
				break;
			}

			// look at the proper spot
			VectorSubtract( target, bs->eye, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );

			bs->altenemy = AngleNormalize360( RAD2DEG( 2 * tan( DETECT_RADIUS / VectorDistance( bs->eye, target ) ) ) );

			// set the time of our next view update
			bs->viewchangetime = level.time + 100 + rand() % 150;

			// next target!
			bs->target_goal.number++;
		} else if ( bs->viewchangetime < level.time ) {
			// we're doing a sweep - on the 2nd pass we sweep the reverse direction of the other 2 passes

			// move viewangle by a bit
			switch ( bs->target_goal.number ) {
			case 2:
				bs->ideal_viewangles[1]--;
				break;
			default:
				bs->ideal_viewangles[1]++;
				break;
			}

			bs->altenemy--;

			// set the time of our next view update
			bs->viewchangetime = level.time + 100 + rand() % 150;
		}
	}

	return qtrue;
}

/*
==================
AIEnter_MP_SniperSpot()
==================
*/
void AIEnter_MP_SniperSpot( bot_state_t *bs ) {
	bs->arrive_time = level.time;
	bs->enemyposition_time = 0;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_SniperSpot;
	bs->ainodeText = "AINode_MP_SniperSpot";
}

/*
==================
AINode_MP_SniperSpot()
==================
*/
int AINode_MP_SniperSpot( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent;
	trace_t tr;

	goal = bs->target_goal;
	ent = &g_entities[bs->target_goal.entitynum];

	// return to this sniper spot if we go off temporarily
	ent->missionLevel = 0;
	bs->flags |= BFL_SNIPING;

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		ent->missionLevel = level.time; // other bots should avoid this spot
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		// return to this sniper spot after we're done
		ent->missionLevel = 0;
		return qfalse;
	}

	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		// return to this sniper spot after we're done
		ent->missionLevel = 0;
		//
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}

	// look for something better to do
	if ( !BotFlagAtBase( bs->sess.sessionTeam, NULL ) ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	// have we got enough ammo?
	if ( !BotCanSnipe( bs, qfalse ) ) {
		if ( !BotFindSpecialGoals( bs ) ) {
			BotDefaultNode( bs );
		}
		return qfalse;
	}

	// is this spot disabled now?
	if ( ent->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	//
	// other bots should avoid this spot
	ent->missionLevel = level.time;

	// is the destination blocked?
	trap_Trace( &tr, ent->s.origin, NULL, NULL, ent->s.origin, bs->client, MASK_PLAYERSOLID );
	if ( tr.startsolid || tr.allsolid ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	VectorSubtract( bs->origin, goal.origin, dir );
	if ( fabs( dir[2] ) < 100 ) {
		dir[2] = 0;
	}

	if ( VectorLengthSquared( dir ) > SQR( 32 ) ) {
		//choose the best weapon to fight with
		BotChooseWeapon( bs );

		bs->arrive_time = level.time;   // wait a bit longer

		//initialize the movement state
		BotSetupForMovement( bs );

		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}
		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
			if ( BotFindSpecialGoals( bs ) ) {
				return qfalse;
			}
			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
			// fail
			bs->ainode = NULL;
			bs->ainodeText = "NULL";
			return qtrue;
		}

		BotAIBlocked( bs, &moveresult, qtrue );

		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		} else if ( moveresult.flags & MOVERESULT_WAITING ) { //if waiting for something
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
			if ( VectorDistanceSquared( bs->origin, goal.origin ) > SQR( 32 ) ) {
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
							bs->enemyposition_time = 0;
						}
					} else {
						if ( EntityIsDead( &entinfo ) ) {
							bs->enemydeath_time = trap_AAS_Time();
						}
					}

					if ( bs->enemy >= 0 ) {
						// attack and keep moving
						if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
							//choose the best weapon to fight with
							BotChooseWeapon( bs );
							//aim at the enemy
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

				if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
					if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
						VectorSubtract( goal.origin, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
					} else if ( VectorLengthSquared( moveresult.movedir ) ) { //FIXME: look at cluster portals?
						vectoangles( moveresult.movedir, bs->ideal_viewangles );
					} else if ( random() < bs->thinktime * 0.8 ) {
						BotRoamGoal( bs, target );
						VectorSubtract( target, bs->origin, dir );
						vectoangles( dir, bs->ideal_viewangles );
						bs->ideal_viewangles[2] *= 0.5;
					}
					bs->ideal_viewangles[2] *= 0.5;
				}
			}
		}
	}

	if ( VectorLengthSquared( dir ) < SQR( 32 ) ) {
		// have we been waiting here for too long?
		if ( bs->arrive_time < level.time - 40000 ) {
			ent->missionLevel = level.time; // other bots should avoid this spot
			if ( !BotFindSpecialGoals( bs ) ) {
				BotDefaultNode( bs );
			}
			return qfalse;
		}

		// Gordon: crouching spot
		if ( ent->spawnflags & 2 ) {
			trap_EA_Crouch( bs->client );
		}

		// We are at the sniper spot, so start sniping
		bs->weaponnum = BotCanSnipe( bs, qfalse );
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );

		// check for enemies
		if ( bs->enemy < 0 ) {
			BotFindEnemyMP( bs, -1, qfalse );
		}

		if ( bs->enemy >= 0 ) {
			aas_entityinfo_t entinfo;
			BotEntityInfo( bs->enemy, &entinfo );

			//if the enemy is dead
			if ( EntityIsDead( &entinfo ) ) {
				bs->enemy = -1;
			}

			if ( bs->enemy >= 0 ) {
				if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 50, bs->enemy, NULL ) ) {
					// if they are real close, abort sniper mode
					if ( VectorDistanceSquared( bs->origin, g_entities[bs->enemy].r.currentOrigin ) < SQR( 512 ) ) {
						AIEnter_MP_Battle_Fight( bs );
						return qfalse;
					}

					bs->arrive_time = level.time;   // wait a bit longer

					// remember this position
					bs->enemyposition_time = trap_AAS_Time();
					VectorCopy( BotGetOrigin( bs->enemy ), bs->enemyorigin );

					//aim at the enemy
					BotAimAtEnemy( bs );

					//attack the enemy if possible
					if ( bs->weaponnum == bs->cur_ps.weapon && BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 5, bs->enemy, NULL ) ) {
						BotCheckAttack( bs );
					}
				} else {
					bs->enemy = -1;
				}
			}
		} else {
			int spotNum;

			if ( bs->arrive_time < level.time - 20000 ) {
				// if not suggesting sniper, suicide so we can change
				if ( !BG_IsScopedWeapon( BotSuggestWeapon( bs, bs->sess.sessionTeam ) ) ) {
					if ( BotFindSpecialGoals( bs ) ) {
						return qfalse;
					}
				}
			}

			// NOTE: remember last visible enemy positions, so we can stay on them longer than usual
			if ( bs->enemyposition_time > trap_AAS_Time() - 5.0 ) {
				VectorSubtract( bs->enemyorigin, bs->origin, dir );
				VectorNormalize( dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( bs->viewchangetime > level.time ) {
				//
				// use same angles
				//
			} else if ( !( rand() % ( 1 + BotGetNumVisibleSniperSpots( bs ) ) ) && ( spotNum = BotGetRandomVisibleSniperSpot( bs ) ) > -1 ) {
				// look at new spot
				VectorSubtract( g_entities[spotNum].s.origin, bs->origin, dir );
				VectorNormalize( dir );
				vectoangles( dir, bs->ideal_viewangles );
				//
				bs->viewchangetime = level.time + 1500 + rand() % 3000;
			} else if ( rand() % 2 && bs->enemyposition_time && ( bs->enemyposition_time > trap_AAS_Time() - 60.0 ) ) {
				// look at last enemy pos
				VectorSubtract( bs->enemyorigin, bs->origin, dir );
				// if it's not too far away, use non-zoomed view
				if ( VectorNormalize( dir ) < 3000 ) {
					// switch to non-zoomed view
					if ( bs->weaponnum >= WP_BEGINSECONDARY && bs->weaponnum <= WP_LASTSECONDARY ) {
						bs->weaponnum = weapAlts[bs->weaponnum];
					}
					trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				}
				vectoangles( dir, bs->ideal_viewangles );

				bs->viewchangetime = level.time + 2000 + rand() % 2000;
			} else if ( !VectorCompare( ent->s.angles, vec3_origin ) ) {
				// just face direction of sniper spot
				// switch to non-zoomed view
				if ( bs->weaponnum >= WP_BEGINSECONDARY && bs->weaponnum <= WP_LASTSECONDARY ) {
					bs->weaponnum = weapAlts[bs->weaponnum];
				}
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				VectorCopy( ent->s.angles, bs->ideal_viewangles );
				//
				bs->viewchangetime = level.time + 2500 + rand() % 3000;
			}
		}
	}

	// reload?
	if ( ( level.time - bs->last_fire > 2000 ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}

	return qtrue;
}

/*
==================
AIEnter_MP_DefendTarget()
==================
*/
void AIEnter_MP_DefendTarget( bot_state_t *bs ) {
	//VectorCopy( bs->origin, bs->aimtarget );
	//choose the best weapon to fight with
	bs->arrive_time = 0;
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_DefendTarget;
	bs->ainodeText = "AINode_MP_DefendTarget";
}

/*
==================
AINode_MP_DefendTarget()
==================
*/
int AINode_MP_DefendTarget( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent, *flag;
	qboolean move;
	trace_t tr;

	goal = bs->target_goal;
	bs->defendgoal = bs->target_goal;

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}


	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}


	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}


	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}


/*	// check for special class actions
	if (BotCheckClassActions(bs)) {
		return qfalse;
	}
*/

	// look for health/ammo packs
	if ( BotFindNearbyGoal( bs ) ) {
		AIEnter_MP_Seek_NBG( bs );
		return qfalse;
	}


	if ( goal.entitynum < 0 ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	// look for better goals
	if ( BotFindSpecialGoals( bs ) ) {
		return qfalse;
	}


	// if we are defending a leader, and they are not valid anymore, then stop
	if ( goal.flags & GFL_LEADER ) {
		// make sure our leader is still valid
		if ( bs->leader > -1 ) {
			if ( !g_entities[bs->leader].inuse ||
				 !g_entities[bs->leader].client ||
				 ( g_entities[bs->leader].client->ps.pm_flags & PMF_LIMBO ) ) {
				bs->leader = -1;
				BotDefaultNode( bs );
				return qfalse;
			} else if ( bs->sess.playerType == PC_MEDIC ) {
				if ( g_entities[bs->leader].health <= 0 && trap_AAS_PointAreaNum( g_entities[bs->leader].r.currentOrigin ) &&
					 BotGoalForEntity( bs, bs->leader, &bs->target_goal, BGU_MEDIUM ) ) {
					// revive
					g_entities[bs->target_goal.entitynum].missionLevel = level.time + 3000;
					AIEnter_MP_MedicRevive( bs );
					return qfalse;
				} else if ( BotHealthScale( bs->leader ) <= 0.7 && trap_AAS_PointAreaNum( g_entities[bs->leader].r.currentOrigin ) &&
							BotGoalForEntity( bs, bs->leader, &bs->target_goal, BGU_MEDIUM ) ) { // health stock?
					// make this our goal
					g_entities[bs->target_goal.entitynum].missionLevel = level.time + 3000;
					AIEnter_MP_MedicGiveHealth( bs );
					return qfalse;
				}
			} else if ( ( VectorLengthSquared( g_entities[bs->leader].client->ps.velocity ) < SQR( 10 ) ) && ( VectorLengthSquared( bs->cur_ps.velocity ) < SQR( 10 ) ) ) {
				if ( !( g_entities[bs->leader].r.svFlags & SVF_BOT ) ) {
					BotVoiceChatAfterIdleTime( bs->client, "WhereTo", SAY_BUDDY, 1000 + rand() % 3000, BOT_SHOWTEXT, 12000, qfalse  );
				}
			}
			//
			if ( !g_entities[bs->leader].inuse ||
				 g_entities[bs->leader].health <= 0 ||
				 VectorDistanceSquared( g_entities[bs->leader].r.currentOrigin, bs->origin ) > SQR( MAX_BOTLEADER_DIST ) ) {
				bs->leader = -1;
			}
		}
		//
		if ( bs->leader == -1 ) {
			BotDefaultNode( bs );
			return qfalse;
		}
	}


	//
	//if there is an enemy
	if ( BotFindEnemyMP( bs, -1, qfalse ) ) {
		//choose the best weapon to fight with
		BotChooseWeapon( bs );
		// if we made it to the destination, let us roam to fight people
		if ( BotCarryingFlag( bs->enemy ) || ( !( goal.flags & GFL_LEADER ) && ( bs->flags & BFL_MISCFLAG ) ) ) {
			if ( !BotCarryingFlag( bs->enemy ) && VectorLengthSquared( bs->cur_ps.velocity ) && BotWantsToRetreat( bs ) ) {
				//keep the current long term goal and retreat
			} else {
				trap_BotResetLastAvoidReach( bs->ms );
				//go fight
				AIEnter_MP_Battle_Fight( bs );
				return qfalse;
			}
		}
	}


	// if the target has been cleared
	if ( goal.entitynum < 0 ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	// should we stop pursuing this target?
	ent = &g_entities[bs->target_goal.entitynum];
	switch ( ent->s.eType ) {
	case ET_TRIGGER_FLAGONLY:
		if ( BotFlagAtBase( bs->sess.sessionTeam, &flag ) == qtrue ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
		break;
	case ET_TRIGGER_FLAGONLY_MULTIPLE:
		if ( !BotEnemyCarryingFlag( bs->client ) ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
		break;
	case ET_ITEM:
		if ( !Q_stricmp( ent->classname, "team_CTF_redflag" ) && !( ent->flags & FL_DROPPED_ITEM ) ) {
			if ( bs->sess.sessionTeam != TEAM_AXIS ) {
				// this is the enemy flag, so abort "defending" it when it shows up
				if ( BotFlagAtBase( TEAM_AXIS, NULL ) == qtrue ) {
					BotDefaultNode( bs );
					return qfalse;
				} else if ( BotCarryingFlag( bs->client ) ) {   // we have it!
					BotDefaultNode( bs );
					return qfalse;
				}
			}
		} else if ( !Q_stricmp( ent->classname, "team_CTF_blueflag" ) && !( ent->flags & FL_DROPPED_ITEM ) ) {
			if ( bs->sess.sessionTeam != TEAM_ALLIES ) {
				// this is the enemy flag, so abort "defending" it when it shows up
				if ( BotFlagAtBase( TEAM_ALLIES, NULL ) ==   qtrue ) {
					BotDefaultNode( bs );
					return qfalse;
				} else if ( BotCarryingFlag( bs->client ) ) {   // we have it!
					BotDefaultNode( bs );
					return qfalse;
				}
			}
		} else if ( ent->touch ) {
			if ( ent->r.svFlags & SVF_NOCLIENT ) {
				BotDefaultNode( bs );
				return qfalse;
			}
		}
		break;
	case ET_TRAP:
		if ( !Q_stricmp( ent->classname, "team_WOLF_checkpoint" ) ) {
			if ( ent->count == ( bs->sess.sessionTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS ) ) {
				// the enemy is controlling this checkpoint now
				BotDefaultNode( bs );
				return qfalse;
			}
		}
		break;
	case ET_TRIGGER_MULTIPLE:
		// if we are within range, stop here
		if ( bs->arrive_time < level.time - 5000 ) {
			if ( ( VectorDistanceSquared( bs->origin, BotGetOrigin( ent->s.number ) ) < SQR( 600 ) )
				 && trap_InPVS( bs->origin, BotGetOrigin( ent->s.number ) ) ) {
				// check the trace
				trap_Trace( &tr, bs->eye, vec3_origin, vec3_origin, BotGetOrigin( ent->s.number ), -1, (MASK_SHOT) &~( CONTENTS_BODY | CONTENTS_CORPSE ) );
				if ( tr.entityNum != ENTITYNUM_WORLD ) {
					VectorCopy( bs->origin, bs->target_goal.origin );
					goal = bs->target_goal;
					bs->arrive_time = level.time;
				}
			}
		}
		break;
	case ET_PLAYER:
		if ( ent->client && ent->health <= 0 ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
		break;
	case ET_ATTRACTOR_HINT:
	default:     // rain
		break;
	}


	// if we are defending a player, and they have moved, get a new sparse defend area
	if ( ent->client && ( bs->target_goal.number < level.time - 300 ) && !VectorCompare( ent->r.currentOrigin, bs->defendgoal_origin ) ) {
		int list[MAX_CLIENTS], numList;
		float ourDist, *distances;

		//
		// if there are too many defenders, and we are the furthest away, stop defending
		if ( BotCarryingFlag( bs->target_goal.entitynum ) ) {
			if ( ( numList = BotNumTeamMatesWithTarget( bs, goal.entitynum, list, MAX_CLIENTS ) ) > BOT_FLAG_CARRIER_DEFENDERS ) {
				ourDist = VectorDistanceSquared( bs->origin, g_entities[goal.entitynum].r.currentOrigin );
				if ( !trap_InPVS( bs->origin, g_entities[goal.entitynum].r.currentOrigin ) ) {
					ourDist += 2048 * 2048;
				}
				distances = BotSortPlayersByDistance( g_entities[goal.entitynum].r.currentOrigin, list, numList );
				if ( distances[numList - 1] < ourDist ) {
					// we are the furthest
					bs->ignore_specialgoal_time = 0;
					bs->leader = -1;
					BotDefaultNode( bs );
					return qfalse;
				}
			}
		}

		// look for a new defend pos
		VectorCopy( ent->r.currentOrigin, bs->target_goal.origin );
		bs->target_goal.areanum = BotPointAreaNum( ent->s.number, ent->r.currentOrigin );
//		BotFindSparseDefendArea( bs, &bs->target_goal, qtrue );
		VectorCopy( ent->r.currentOrigin, bs->defendgoal_origin );
		bs->target_goal.number = level.time + rand() % 100;
		goal = bs->target_goal;
		bs->defendgoal = bs->target_goal;
	}

	//
	// remember the last position we couldnt see the DESTINATION from, so we can look there once we arrive
	if ( !trap_InPVS( bs->origin, goal.origin ) ) {
		VectorCopy( bs->origin, bs->aimtarget );
	}

	//
	// do we need to get closer to the goal?
	//
	move = qfalse;
	if ( !( bs->target_goal.flags & GFL_DEFEND_CLOSE ) ) {
		if ( !move && VectorDistanceSquared( bs->origin, goal.origin ) > SQR( 384 ) ) {
			move = qtrue;
		}
		if ( !move && trap_InPVS( bs->origin, goal.origin ) ) {
			trace_t tr;
			trap_Trace( &tr, bs->origin, NULL, NULL, goal.origin, -1, MASK_SHOT & ~( CONTENTS_BODY | CONTENTS_CORPSE ) );
			if ( tr.startsolid || tr.allsolid || tr.fraction < 1.0 ) {
				move = qtrue;
			}
		}
	} else {
		if ( !move && VectorDistanceSquared( bs->origin, goal.origin ) > ( ( goal.flags & GFL_LEADER ) ? SQR( 80 ) : SQR( 32 ) ) ) {
			move = qtrue;
		}
	}

	if ( move ) {
		//initialize the movement state
		BotSetupForMovement( bs );
		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}

		//if the movement failed
		if ( moveresult.failure ) {
			if ( goal.flags & GFL_LEADER ) {
				bs->leader = -1;
			}
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

			// jump randomly?
			//trap_EA_Jump(bs->client);
			//trap_EA_Move(bs->client, tv(crandom(), crandom(), crandom()), 100+random()*200 );
			// fail
			bs->ainode = NULL;
			bs->ainodeText = "NULL";
			return qtrue;
		}

		if ( bs->blockentTime > level.time - 500 && bs->blockent < level.maxclients ) {
			// if we are within range, then stop here
			if ( goal.flags & GFL_LEADER ) {
				if ( ( BotPointWithinMovementAutonomy( bs, &goal, bs->origin ) )
					 && ( VectorDistanceSquared( bs->origin, g_entities[bs->leader].r.currentOrigin ) < SQR( 512 ) ) ) {
					VectorCopy( bs->origin, bs->target_goal.origin );
					bs->target_goal.areanum = bs->areanum;
				}
			}
		}

		BotAIBlocked( bs, &moveresult, qtrue );
		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		} else if ( moveresult.flags & MOVERESULT_WAITING ) { //if waiting for something
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
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
						// under special circumstances, we should go into battle mode
						if ( !BotCarryingFlag( bs->target_goal.entitynum ) &&
							 ( ( bs->weaponnum == WP_LUGER ) || ( bs->weaponnum == WP_COLT ) || ( bs->weaponnum == WP_MOBILE_MG42 ) ) ) {
							AIEnter_MP_Battle_Fight( bs );
							return qfalse;
						} else {
							//choose the best weapon to fight with
							BotChooseWeapon( bs );
							//aim at the enemy
							BotAimAtEnemy( bs );
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

			if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
				if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
					VectorSubtract( goal.origin, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
				} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
				} else if ( VectorLengthSquared( moveresult.movedir ) ) { //FIXME: look at cluster portals?
					vectoangles( moveresult.movedir, bs->ideal_viewangles );
				} else if ( random() < bs->thinktime * 0.8 ) {
					BotRoamGoal( bs, target );
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
					bs->ideal_viewangles[2] *= 0.5;
				}
				bs->ideal_viewangles[2] *= 0.5;
				// check for giving ourselves some ammo
				if ( bs->sess.playerType == PC_FIELDOPS && ClientNeedsAmmo( bs->client ) ) {
					// switch to regen and pump away
					bs->weaponnum = WP_AMMO;
					trap_EA_SelectWeapon( bs->client, bs->weaponnum );
					if ( bs->cur_ps.weapon == WP_AMMO && BotWeaponCharged( bs, WP_AMMO ) ) {
						trap_EA_Attack( bs->client );
					}
				}
				// check for giving ourselves some health
				if ( bs->sess.playerType == PC_MEDIC && BotHealthScale( bs->client ) < 1.0 ) {
					// switch to regen and pump away
					bs->weaponnum = WP_MEDKIT;
					trap_EA_SelectWeapon( bs->client, bs->weaponnum );
					if ( bs->cur_ps.weapon == WP_MEDKIT && BotWeaponCharged( bs, WP_MEDKIT ) ) {
						trap_EA_Attack( bs->client );
					}
				}
			}
		}

	} else {
		gentity_t *objtarg;
		// NOTE: dont bother looking for enemies, since we have reached the destination, so we are free to
		// pursue enemies now
		//
		// call for an engineer (if required)
		if ( ( ent->s.eType == ET_OID_TRIGGER ) && ( objtarg = ent->target_ent ) ) {
			if ( objtarg->spawnflags & 64 ) {
				if ( ent->lastHintCheckTime < level.time && rand() % 2 ) {
					// if there is no dynamite planted here
					int list[10], numList, i;
					//
					numList = BotGetTargetExplosives( bs->sess.sessionTeam, list, 10, qfalse );
					for ( i = 0; i < numList; i++ ) {
						if ( list[i] == ent->s.number ) {
							break;
						}
					}
					// if this objective doesn ont have armed dynamite
					if ( numList && i < numList ) {
						BotVoiceChatAfterIdleTime( bs->client, "NeedEngineer", SAY_TEAM, 500 + rand() % 4000, qfalse, 5000 + rand() % 5000, qfalse  );
					}
				}
				// if we are an engineer, throw out an air-strike
				if ( bs->sess.playerType == PC_FIELDOPS && ( level.time - bs->cur_ps.classWeaponTime > ( level.lieutenantChargeTime[bs->sess.sessionTeam - 1] * 0.5f ) ) ) {
					// select smoke grenade
					bs->weaponnum = WP_SMOKE_MARKER;
					trap_EA_SelectWeapon( bs->client, bs->weaponnum );
					// look upwards
					bs->ideal_viewangles[PITCH] = -70;
					if ( bs->cur_ps.weapon == bs->weaponnum && bs->viewangles[PITCH] < -60 ) {
						trap_EA_Attack( bs->client );
					}
					return qtrue;
				}
			}
		}
		// set flag so we know that we made it to the destination
		bs->flags |= BFL_MISCFLAG;
		//
		// check for enemies
		if ( bs->enemy < 0 ) {
			BotFindEnemyMP( bs, -1, qfalse );
		}
		if ( bs->enemy >= 0 ) {
			if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
				//aim at the enemy
				BotAimAtEnemy( bs );
				//attack the enemy if possible
				if ( bs->weaponnum == bs->cur_ps.weapon ) {
					BotCheckAttack( bs );
				}
			} else {
				bs->enemy = -1;
			}
		} else {
			if ( bs->viewchangetime < level.time ) {
				// look for a better pos
				if ( !bs->arrive_time && random() < 0.1 ) {
					if ( bs->target_goal.entitynum ) {
						BotFindSparseDefendArea( bs, &bs->target_goal, qfalse );
					}
				}
				if ( ++bs->viewtype > 1 ) {
					bs->viewtype = 0;
				}
				bs->viewchangetime = level.time + 500 + rand() % 3000;
			}
			// if guarding a carrier, look towards the flag goal
			if ( goal.entitynum > -1 && goal.entitynum < level.maxclients && BotCarryingFlag( goal.entitynum ) ) {
				bot_goal_t fgoal;
				gentity_t *flaggoal;
				qboolean setdir = qfalse;
				//
				flaggoal = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
				if ( !flaggoal ) {
					flaggoal = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
				}
				if ( flaggoal ) {
					if ( BotGoalForEntity( NULL, flaggoal->s.number, &fgoal, BGU_HIGH ) ) {
						if ( trap_BotMovementViewTarget( bs->ms, &fgoal, bs->tfl, 300, target ) ) {
							VectorSubtract( target, bs->origin, dir );
							vectoangles( dir, bs->ideal_viewangles );
							setdir = qtrue;
						}
					}
				}

				if ( !setdir ) {
					// look away from the defense object
					VectorAdd( g_entities[bs->target_goal.entitynum].r.absmax, g_entities[bs->target_goal.entitynum].r.absmin, target );
					VectorScale( target, 0.5, target );
					VectorSubtract( target, bs->origin, dir );
					VectorNormalize( dir );
					VectorInverse( dir );
					vectoangles( dir, bs->ideal_viewangles );
					bs->ideal_viewangles[PITCH] = 0;
				}

			} else if ( bs->viewtype == 0 ) {
				// face the current aimtarget
				VectorSubtract( bs->aimtarget, bs->origin, dir );
				if ( VectorNormalize( dir ) ) {
					if ( fabs( dir[2] ) < 0.8 ) {
						vectoangles( dir, bs->ideal_viewangles );
					}
					bs->ideal_viewangles[PITCH] = 0;
				}
			} else if ( bs->target_goal.entitynum > 0 ) {
				// look at the defense object
				VectorAdd( g_entities[bs->target_goal.entitynum].r.absmax, g_entities[bs->target_goal.entitynum].r.absmin, target );
				VectorScale( target, 0.5, target );
				VectorSubtract( target, bs->origin, dir );
				VectorNormalize( dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[PITCH] = 0;
			}
		}
	}

	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}

	return qtrue;
}

/*
==================
AIEnter_MP_TouchTarget()
==================
*/
void AIEnter_MP_TouchTarget( bot_state_t *bs ) {
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_TouchTarget;
	bs->ainodeText = "AINode_MP_TouchTarget";
}

/*
==================
AINode_MP_TouchTarget()
==================
*/
int AINode_MP_TouchTarget( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *ent;
	qboolean altroute = qfalse;
	vec3_t mins, maxs;

	goal = bs->target_goal;
	if ( bs->alt_goal.number && bs->alt_goal.entitynum == goal.entitynum && bs->alt_goal.number == goal.number ) {
		if ( ( bs->alt_goal.number < level.time - 12000 ) || ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->target_goal.areanum, bs->tfl ) < 700 ) ) {
			// stop pursuing altgoal after some time
			BotClearGoal( &bs->alt_goal );
			bs->target_goal.number = 0;
			goal = bs->target_goal;
		} else {
			goal = bs->alt_goal;
			altroute = qtrue;
		}
	}

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}

	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}

/*	// check for special class actions
	if (BotCheckClassActions(bs)) {
		return qfalse;
	}
*/

	// look for health/ammo packs
	if ( BotFindNearbyGoal( bs ) ) {
		AIEnter_MP_Seek_NBG( bs );
		return qfalse;
	}

	//
	// should we stop pursuing this target?
	ent = &g_entities[bs->target_goal.entitynum];
	if ( ent->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
		if ( !BotFindSpecialGoals( bs ) ) {
			BotDefaultNode( bs );
		}
		return qfalse;
	}

	if ( BotFindSpecialGoals( bs ) ) {
		return qfalse;
	}

	if ( ent->s.eType == ET_TRAP && !Q_stricmp( ent->classname, "team_WOLF_checkpoint" ) ) {
		if ( ent->count == bs->sess.sessionTeam ) {
			// we have captured it
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
	} else if ( ent->s.eType == ET_ITEM && ent->touch ) {
		if ( ent->r.svFlags & SVF_NOCLIENT ) {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qfalse;
		}
	} else {
		if ( ent->s.eType == ET_TRIGGER_MULTIPLE ) {
			//
			if ( !ent->r.linked || ent->nextthink > level.time + 1000 ) {
				BotDefaultNode( bs );
				return qfalse;
			}

			// if we are currently touching the target, then use this as our spot
			if ( bs->arrive_time + 3000 < level.time ) {
				VectorAdd( bs->origin, bs->cur_ps.mins, mins );
				VectorAdd( bs->origin, bs->cur_ps.maxs, maxs );
				if ( trap_EntityContactCapsule( mins, maxs, ent ) ) {
					VectorCopy( bs->origin, goal.origin );
					bs->arrive_time = level.time;
				}
			}

			// if the current destination is no longer touching the brush, get a new position
			VectorAdd( goal.origin, bs->cur_ps.mins, mins );
			VectorAdd( goal.origin, bs->cur_ps.maxs, maxs );

			if ( !trap_EntityContactCapsule( mins, maxs, ent ) || ( rand() % 50 == 0 ) ) {
				// need a new position
				if ( !BotGetReachableEntityArea( bs, ent->s.number, &goal ) ) {
					BotDefaultNode( bs );
					return qfalse;
				}
				// check this position
				VectorAdd( goal.origin, bs->cur_ps.mins, mins );
				VectorAdd( goal.origin, bs->cur_ps.maxs, maxs );
				if ( !trap_EntityContactCapsule( mins, maxs, ent ) ) {
					BotDefaultNode( bs );
					return qfalse;
				}
				// this is the new goal
				bs->target_goal = goal;
			}
		}
		if ( !Q_stricmp( ent->classname, "trigger_flagonly" ) ) {
			// if we dont have the flag anymore, then stop
			if ( !BotCarryingFlag( bs->client ) ) {
				BotDefaultNode( bs );
				return qfalse;
			}
		}
	}
	//
	VectorCopy( goal.origin, target );
	if ( fabs( target[2] - bs->origin[2] ) < 80 ) {
		target[2] = bs->origin[2];
	}
	if ( altroute && VectorDistanceSquared( goal.origin, bs->origin ) < SQR( 80 ) ) {
		// made it to the altroute
		BotClearGoal( &bs->alt_goal );
		bs->target_goal.number = 0;
		goal = bs->target_goal;
	}

	//
	// if we have an enemy, then we should look for an alternate route away from them
	if ( ( BotCarryingFlag( bs->client ) ) && ( goal.number < level.time - 8000 ) &&
		 ( BotNumTeamMatesWithTarget( bs, bs->client, NULL, -1 ) < 2 ) &&
		 ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->target_goal.areanum, bs->tfl ) > 1500 ) ) {
		if ( bs->enemy > -1 ) {
			aas_altroutegoal_t altroutegoals[40];
			int numList, i;
			bot_goal_t tempgoal;
			vec3_t edir;
			//
			// get the vector towards the enemy
			VectorSubtract( g_entities[bs->enemy].r.currentOrigin, bs->origin, edir );
			VectorNormalize( edir );
			// if we are running towards them
			if ( DotProduct( edir, bs->cur_ps.velocity ) > 0 ) {
				// dont do this check again for a while
				bs->alt_goal.number = level.time;
				bs->target_goal.number = level.time;
				//initialize the movement state
				BotSetupForMovement( bs );
				// check for an alternate route
				if ( ( numList = trap_AAS_AlternativeRouteGoals( bs->origin, bs->target_goal.origin, bs->tfl, altroutegoals, 40, 0 ) ) ) {
					for ( i = 0; i < numList; i++ ) {
						BotClearGoal( &tempgoal );
						tempgoal.areanum = altroutegoals[i].areanum;
						VectorCopy( altroutegoals[i].origin, tempgoal.origin );
						if ( qtrue ) { //trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
							//VectorSubtract(target, bs->origin, dir);
							VectorSubtract( tempgoal.origin, bs->origin, dir );
							VectorNormalize( dir );
							if ( DotProduct( dir, edir ) < 0 ) {      // moving away from the enemy
								//make this the altroutegoal
								bs->alt_goal = bs->target_goal;
								trap_AAS_AreaWaypoint( altroutegoals[i].areanum, bs->alt_goal.origin );
								bs->alt_goal.areanum = trap_AAS_PointAreaNum( bs->alt_goal.origin );
								break;
							}
						}
					}
				}
			}
		} else if ( bs->enemy < 0 ) {
			int list[MAX_CLIENTS], numList;
			int i, t;
			bot_state_t *tbs;
			// if we have defenders, stop every now and then to let them get infront of us
			if ( ( bs->stand_time < level.time - 3000 ) && ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->target_goal.areanum, bs->tfl ) > 600 ) &&
				 ( numList = BotNumTeamMatesWithTarget( bs, bs->client, list, MAX_CLIENTS ) ) ) {
				// if one of them has a goal infront of us, and they are not near it, we should pause for a bit
				for ( i = 0; i < numList; i++ ) {
					tbs = &botstates[list[i]];
					//
					if ( ( VectorDistanceSquared( tbs->origin, tbs->defendgoal.origin ) < SQR( 1500 ) ) &&
						 ( VectorDistanceSquared( tbs->origin, bs->origin ) < SQR( 1024 ) ) &&
						 ( trap_InPVS( tbs->origin, bs->origin ) ) &&
						 ( trap_AAS_AreaTravelTimeToGoalArea( tbs->defendgoal.areanum, tbs->defendgoal.origin, goal.areanum, bs->tfl ) < trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->target_goal.areanum, bs->tfl ) ) &&
						 ( trap_AAS_AreaTravelTimeToGoalArea( tbs->areanum, tbs->origin, goal.areanum, bs->tfl ) - trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, goal.areanum, bs->tfl ) ) ) {
						t = trap_AAS_AreaTravelTimeToGoalArea( tbs->areanum, tbs->origin, tbs->defendgoal.areanum, bs->tfl );
						if ( t > 300 ) {
							t = 300;
						}
						if ( t > 100 ) {
							bs->stand_time = level.time + 1000 + 5 * t;
						}
					}
				}
			}
		}
	}
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//

	//initialize the movement state
	BotSetupForMovement( bs );
	//
	if ( bs->stand_time < level.time ) {
		qboolean move = qtrue;
		//move towards the goal
		if ( ent->s.eType == ET_TRAP && !Q_stricmp( ent->classname, "team_WOLF_checkpoint" )
			 && ( VectorDistanceSquared( goal.origin, bs->origin ) < SQR( 128 ) ) ) {
			// move straight torward it
			VectorAdd( ent->r.absmin, ent->r.absmax, target );
			VectorScale( target, 0.5, target );
			VectorSubtract( target, bs->origin, dir );
			dir[2] = 0;
			VectorNormalize( dir );
			trap_EA_Move( bs->client, dir, 300 );
			// randomly jump
			if ( rand() % 10 == 0 ) {
				trap_EA_Jump( bs->client );
			}
			move = qfalse;
		} else if ( ent->s.eType == ET_TRIGGER_MULTIPLE ) {
			int i;
			// if we are touching the brush, we dont need to move
			VectorAdd( bs->origin, bs->cur_ps.mins, mins );
			VectorAdd( bs->origin, bs->cur_ps.maxs, maxs );
			// make them a bit smaller to be safe
			for ( i = 0; i < 3; i++ ) {
				mins[i] += 2;
				maxs[i] -= 2;
			}

			if ( trap_EntityContactCapsule( mins, maxs, ent ) ) {
				move = qfalse;
			}
		}

		if ( move ) {
			//try a direct movement
			if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
				//move towards the goal
				BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
			}
			//if the movement failed
			if ( moveresult.failure ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

				// jump randomly
				trap_EA_Jump( bs->client );
				trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
				// fail
				bs->ainode = NULL;
				bs->ainodeText = "NULL";
				return qtrue;
			}
			//
			BotAIBlocked( bs, &moveresult, qtrue );
		}
	}
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
/*					// if we are not going for an urgent goal, go into battle mode
					if (!BotCarryingFlag(bs->client) && bs->ignore_specialgoal_time < level.time && BotWantsToChase(bs) &&
						(VectorDistanceSquared( goal.origin, bs->origin ) > SQR(384)) &&
						(VectorDistanceSquared( bs->enemyorigin, bs->origin ) < SQR(2048)) &&
						Q_stricmp(g_entities[goal.entitynum].classname, "team_CTF_redflag") &&
						Q_stricmp(g_entities[goal.entitynum].classname, "team_CTF_blueflag")) {
						AIEnter_MP_Battle_Fight(bs);
						return qfalse;
					}
*/
					//choose the best weapon to fight with
					BotChooseWeapon( bs );
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( ent->s.eType == ET_TRIGGER_MULTIPLE && trap_InPVS( bs->origin, goal.origin ) && Distance( bs->origin, goal.origin ) < 512 ) {
				VectorSubtract( ent->r.currentOrigin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( moveresult.flags & MOVERESULT_DIRECTMOVE )     {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
			// check for giving ourselves some ammo
			if ( bs->sess.playerType == PC_FIELDOPS && ClientNeedsAmmo( bs->client ) ) {
				// switch to regen and pump away
				bs->weaponnum = WP_AMMO;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				if ( bs->cur_ps.weapon == WP_AMMO && BotWeaponCharged( bs, WP_AMMO ) ) {
					trap_EA_Attack( bs->client );
				}
			}
			// check for giving ourselves some health
			if ( bs->sess.playerType == PC_MEDIC && BotHealthScale( bs->client ) < 1.0 ) {
				// switch to regen and pump away
				bs->weaponnum = WP_MEDKIT;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				if ( bs->cur_ps.weapon == WP_MEDKIT && BotWeaponCharged( bs, WP_MEDKIT ) ) {
					trap_EA_Attack( bs->client );
				}
			}
		}
	}

	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}

	return qtrue;
}

/*
==================
AIEnter_MP_SatchelChargeTarget
==================
*/
void AIEnter_MP_SatchelChargeTarget( bot_state_t *bs ) {
	bs->target_goal.flags &= ~GFL_NOSLOWAPPROACH;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_SatchelChargeTarget;
	bs->ainodeText = "AINode_MP_SatchelChargeTarget";
}

/*
==================
AINode_MP_SatchelChargeTarget
==================
*/
int AINode_MP_SatchelChargeTarget( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;
	int list[10], numList, i;
	float goalDist;
	aas_entityinfo_t entinfo;

	goal = bs->target_goal;
	//if we have changed class
	if ( bs->sess.playerType != PC_COVERTOPS ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	//if there are 2 bots going for this goal, then we should abort if we are further away
	goalDist = VectorDistanceSquared( bs->origin, goal.origin );
	if ( ( numList = BotNumTeamMatesWithTargetByClass( bs, goal.entitynum, list, 10, PC_ENGINEER ) ) ) {
		if ( goalDist > SQR( 256 ) ) {
			goalDist = SQR( 256 );    // only abort if one of our teammates is close to the goal
		}
		for ( i = 0; i < numList; i++ ) {
			if ( botstates[list[i]].ainode == AINode_MP_SatchelChargeTarget ) {
				if ( VectorDistanceSquared( botstates[list[i]].origin, goal.origin ) < goalDist ) {
					// make sure other bots dont head for this target also
					g_entities[goal.entitynum].lastHintCheckTime = level.time + 5000;
					BotDefaultNode( bs );
					return qfalse;
				}
			}
		}
	}

	VectorSubtract( bs->origin, goal.origin, dir );
	if ( fabs( dir[2] ) < 128 ) {
		dir[2] = 0;
	}

	goalDist = VectorLengthSquared( dir );
	// are we close enough to the goal?
	if ( goalDist < SQR( 32 ) ) {
		// have we recently dropped some dynamite?
		for ( trav = G_FindSatchels( NULL ); trav; trav = G_FindSatchels( trav ) ) {
			if ( !trav->parent || trav->parent->s.number != bs->client ) {
				continue;
			}
			if ( VectorDistanceSquared( bs->origin, trav->r.currentOrigin ) > SQR( 64 ) ) {
				continue;
			}

			// found some!
			bs->flags |= BFL_MISCFLAG;
			VectorCopy( trav->r.currentOrigin, bs->target_goal.origin );
			bs->target_goal.origin[2] += 24;
			break;
		}

		if ( !trav ) {
			// no dynamite found, keep trying to plant some
			if ( bs->flags & BFL_MISCFLAG ) {
				// it's gone, so reset goal
				BotDefaultNode( bs );
				return qfalse;
			}
			if ( BotWeaponCharged( bs, WP_SATCHEL ) ) {
				// VOICE: cover me!
				BotVoiceChatAfterIdleTime( bs->client, "CoverMe", SAY_TEAM, 500 + rand() % 1000, qfalse, 10000, qfalse  );
				// make sure other bots dont head for this target also
				g_entities[bs->target_goal.entitynum].lastHintCheckTime = level.time + 2000;
				// crouch down
				trap_EA_Crouch( bs->client );
				// look downwards
				bs->ideal_viewangles[PITCH] = 70;
				// select the dynamite
				bs->weaponnum = WP_SATCHEL;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				// hold fire
				if ( !VectorLengthSquared( bs->velocity ) && fabs( bs->viewangles[PITCH] - bs->ideal_viewangles[PITCH] ) < 2 && bs->cur_ps.weapon == WP_SATCHEL && !bs->cur_ps.grenadeTimeLeft ) {
					trap_EA_Attack( bs->client );
				}
				return qtrue;
			}
		} else if ( ( trav = G_FindSatchel( &g_entities[bs->client] ) ) ) {

/*			// check for emergency targets (flags, etc)
			if (BotCheckEmergencyTargets( bs )) {
				return qfalse;
			}
			// check for dangerous elements
			if (BotDangerousGoal(bs, &goal)) {
				AIEnter_MP_AvoidDanger(bs);	// avoid this danger until it passes
				return qfalse;
			}

			// just look for a goal
			BotDefaultNode(bs);
			return qfalse;*/
		}
	}

	if ( !( bs->flags & BFL_MISCFLAG ) ) {
		if ( goalDist > SQR( 128 ) ) {
			// check for emergency targets (flags, etc)
			if ( BotCheckEmergencyTargets( bs ) ) {
				return qfalse;
			}
		}
		// check for dangerous elements
		if ( BotDangerousGoal( bs, &goal ) ) {
			AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
			return qfalse;
		}
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}

	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	// are we close enough to the goal?
	if ( goalDist >= SQR( 32 ) ) {
		// MOVEMENT REQUIRED
		//
		//initialize the movement state
		BotSetupForMovement( bs );
		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}
		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
		}
		//
		BotAIBlocked( bs, &moveresult, qtrue );
		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		}
		//if waiting for something
		else if ( moveresult.flags & MOVERESULT_WAITING ) {
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
			// check for enemies
			if ( bs->enemy < 0 ) {
				BotFindEnemyMP( bs, -1, qfalse );
			}
			if ( bs->enemy >= 0 ) {
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
						//aim at the enemy
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
			//
			if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
				if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
				}
				//FIXME: look at cluster portals?
				else if ( VectorLengthSquared( moveresult.movedir ) ) {
					vectoangles( moveresult.movedir, bs->ideal_viewangles );
				} else if ( random() < bs->thinktime * 0.8 )     {
					BotRoamGoal( bs, target );
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
					bs->ideal_viewangles[2] *= 0.5;
				}
				bs->ideal_viewangles[2] *= 0.5;
			}
		}
	} else {
		// check for enemies
		if ( bs->enemy < 0 ) {
			BotFindEnemyMP( bs, -1, qfalse );
		}
		if ( bs->enemy >= 0 ) {
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
					//aim at the enemy
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
	//
	return qtrue;
}

/*
==================
AIEnter_MP_DynamiteTarget()
==================
*/
void AIEnter_MP_DynamiteTarget( bot_state_t *bs ) {
	bs->toggleHidingTime = level.time;
	bs->target_goal.flags &= ~GFL_NOSLOWAPPROACH;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_DynamiteTarget;
	bs->ainodeText = "AINode_MP_DynamiteTarget";
}

/*
==================
AINode_MP_DynamiteTarget()
==================
*/
int AINode_MP_DynamiteTarget( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;
	int list[10], numList, i;
	float goalDist;
	aas_entityinfo_t entinfo;

	goal = bs->target_goal;
	//if we have changed class
	if ( bs->sess.playerType != PC_ENGINEER ) {
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//if there are 2 bots going for this goal, then we should abort if we are further away
	goalDist = VectorDistanceSquared( bs->origin, goal.origin );
	if ( ( numList = BotNumTeamMatesWithTargetByClass( bs, goal.entitynum, list, 10, PC_ENGINEER ) ) ) {
		if ( goalDist > SQR( 256 ) ) {
			goalDist = SQR( 256 );    // only abort if one of our teammates is close to the goal
		}
		for ( i = 0; i < numList; i++ ) {
			if ( list[i] == bs->client ) {
				continue;
			}
			if ( botstates[list[i]].ainode == AINode_MP_DynamiteTarget ) {
				if ( VectorDistanceSquared( botstates[list[i]].origin, goal.origin ) < goalDist ) {
					// make sure other bots dont head for this target also
					g_entities[goal.entitynum].lastHintCheckTime = level.time + 5000;
					BotDefaultNode( bs );
					return qfalse;
				}
			}
		}
	}
	//
	//map specific code
	BotMapScripts( bs );
	//
	VectorSubtract( bs->origin, goal.origin, dir );
	if ( fabs( dir[2] ) < 128 ) {
		dir[2] = 0;
	}
	goalDist = VectorLengthSquared( dir );
	// are we close enough to the goal?
	if ( goalDist < SQR( 32 ) ) {
		// have we recently dropped some dynamite?
		for ( trav = G_FindDynamite( NULL ); trav; trav = G_FindDynamite( trav ) ) {
			if ( !trav->parent || trav->parent->s.number != bs->client ) {
				continue;
			}
			if ( VectorDistanceSquared( bs->origin, trav->r.currentOrigin ) > SQR( 100 ) ) {
				continue;
			}
			if ( trav->spawnTime < bs->toggleHidingTime ) {
				// armed before we started this node
				continue;
			}

			// found some!
			bs->flags |= BFL_MISCFLAG;
			VectorCopy( trav->r.currentOrigin, bs->target_goal.origin );
			bs->target_goal.origin[2] += 24;
			break;
		}

		if ( !trav ) {
			// no dynamite found, keep trying to plant some
			if ( bs->flags & BFL_MISCFLAG ) {
				// it's gone, so reset goal
				BotDefaultNode( bs );
				return qfalse;
			}
			if ( BotWeaponCharged( bs, WP_DYNAMITE ) ) {
				// VOICE: cover me!
				BotVoiceChatAfterIdleTime( bs->client, "CoverMe", SAY_TEAM, 500 + rand() % 1000, qfalse, 10000, qfalse  );
				// make sure other bots dont head for this target also
				g_entities[bs->target_goal.entitynum].lastHintCheckTime = level.time + 2000;
				// crouch down
				trap_EA_Crouch( bs->client );
				// look downwards
				bs->ideal_viewangles[PITCH] = 70;
				// select the dynamite
				bs->weaponnum = WP_DYNAMITE;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				// hold fire
				if ( rand() % 2 && !VectorLengthSquared( bs->velocity ) && fabs( bs->viewangles[PITCH] - bs->ideal_viewangles[PITCH] ) < 2 && bs->cur_ps.weapon == WP_DYNAMITE && !bs->cur_ps.grenadeTimeLeft ) {
					trap_EA_Attack( bs->client );
				}
				return qtrue;
			}
		} else if ( trav->s.teamNum >= 4 ) {  // the dynamite is not armed
			// switch to pliers and arm the dynamite
			bs->weaponnum = WP_PLIERS;
			trap_EA_SelectWeapon( bs->client, bs->weaponnum );
			// aim directly at the dynamite
			VectorCopy( bs->origin, target );
			target[2] += bs->cur_ps.viewheight;
			VectorSubtract( trav->r.currentOrigin, target, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
			// hold fire
			if ( bs->cur_ps.weapon == WP_PLIERS ) {
				trap_EA_Attack( bs->client );
			}
			return qtrue;
		} else {    // the dynamite is armed, get out of here!

			// check for dangerous elements
			if ( BotDangerousGoal( bs, &goal ) ) {
				AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
				return qfalse;
			}

			// just look for a goal
			BotDefaultNode( bs );

			// if we are still in dynamite mode, then we may be trying to plant more dynamite at same place
			bs->toggleHidingTime = level.time;

			return qfalse;
		}
	}
	//
	if ( !( bs->flags & BFL_MISCFLAG ) ) {
		if ( goalDist > SQR( 128 ) ) {
			// check for emergency targets (flags, etc)
			if ( BotCheckEmergencyTargets( bs ) ) {
				return qfalse;
			}
		}
		// check for dangerous elements
		if ( BotDangerousGoal( bs, &goal ) ) {
			AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
			return qfalse;
		}
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//
	// are we close enough to the goal?
	if ( goalDist >= SQR( 32 ) ) {
		// MOVEMENT REQUIRED
		//
		//initialize the movement state
		BotSetupForMovement( bs );
		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}
		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

			// jump randomly?
			trap_EA_Jump( bs->client );
			trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
		}
		//
		BotAIBlocked( bs, &moveresult, qtrue );
		//if the viewangles are used for the movement
		if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
			VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
		}
		//if waiting for something
		else if ( moveresult.flags & MOVERESULT_WAITING ) {
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
		} else {
			// check for enemies
			if ( bs->enemy < 0 ) {
				BotFindEnemyMP( bs, -1, qfalse );
			}
			if ( bs->enemy >= 0 ) {
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
						//aim at the enemy
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
			//
			if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
				if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
					VectorSubtract( goal.origin, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
				} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
				}
				//FIXME: look at cluster portals?
				else if ( VectorLengthSquared( moveresult.movedir ) ) {
					vectoangles( moveresult.movedir, bs->ideal_viewangles );
				} else if ( random() < bs->thinktime * 0.8 )     {
					BotRoamGoal( bs, target );
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
					bs->ideal_viewangles[2] *= 0.5;
				}
				bs->ideal_viewangles[2] *= 0.5;
			}
		}
	} else {
		// check for enemies
		if ( bs->enemy < 0 ) {
			BotFindEnemyMP( bs, -1, qfalse );
		}
		if ( bs->enemy >= 0 ) {
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
					//aim at the enemy
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
	//
	return qtrue;
}


/*
==================
AIEnter_MP_ConstructibleTarget()
==================
*/
void AIEnter_MP_ConstructibleTarget( bot_state_t *bs ) {
	bs->target_goal.flags &= ~GFL_NOSLOWAPPROACH;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_ConstructibleTarget;
	bs->ainodeText = "AINode_MP_ConstructibleTarget";
}

/*
==================
AINode_MP_ConstructibleTarget()
==================
*/
int AINode_MP_ConstructibleTarget( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav, *ent, *constructible;
	int list[10], numList, i;
	float goalDist;

	ent = &g_entities[bs->client];
	trav = &g_entities[bs->target_goal.entitynum];

	constructible = G_ConstructionForTeam( trav, bs->sess.sessionTeam );
	goal = bs->target_goal;

/*	if(bot_profile.integer == 2) {
		// update this in case it is moving
		if (bs->lastLeaderMoveTime < level.time - 500) {
			BotGoalForEntity( bs, goal.entitynum, &goal, BGU_HIGH );
			bs->lastLeaderMoveTime = level.time;
		}
	}*/

	//if we have changed class
	if ( bs->sess.playerType != PC_ENGINEER ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}

	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}

	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}

	// check for emergency targets
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}

	// is this constructible finished building?
	if ( !BotIsConstructible( bs->sess.sessionTeam, goal.entitynum ) ) {
		// finished building
		BotDefaultNode( bs );
		return qfalse;
	}

	//if there are 2 bots going for this goal, then we should abort if we are further away
	if ( ( numList = BotNumTeamMatesWithTarget( bs, goal.entitynum, list, 10 ) ) > botgoalMaxCloser[BFG_CONSTRUCT] ) {
		goalDist = VectorDistanceSquared( bs->origin, goal.origin );
		if ( goalDist > SQR( 1024 ) ) {
			goalDist = SQR( 1024 );   // only abort if one of our teammates is close to the goal
		}

		for ( i = 0; i < numList; i++ ) {
			if ( botstates[list[i]].ainode == AINode_MP_ConstructibleTarget ) {
				if ( VectorDistanceSquared( botstates[list[i]].origin, goal.origin ) < goalDist ) {
					// only abort if there is something else to do
					if ( BotFindSpecialGoals( bs ) ) {
						return qfalse;
					}
				}
			}
		}
	}

	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}

	// are we close enough to the goal?
	if ( ent->client->touchingTOI && ent->client->touchingTOI == trav ) {
		// if the construction has changed level, we should reset the construction flag so we wait for full power again
		if ( ( bs->flags & BFL_MISCFLAG ) && constructible->count2 && !constructible->s.angles2[0] ) {
			bs->flags &= ~BFL_MISCFLAG;
		}

		// if we haven't started yet, make sure we wait until we have reasonable power before starting
		if ( !( bs->flags & BFL_MISCFLAG ) ) {
			if ( bs->cur_ps.classWeaponTime > ( level.time - level.engineerChargeTime[bs->sess.sessionTeam - 1] / 4 ) ) {
				// just crouch, and look ahead
				trap_EA_Crouch( bs->client );
				bs->ideal_viewangles[PITCH] = 0;
				// look for things to attack
				BotFindAndAttackEnemy( bs );
				return qtrue;
			}
		}

		// we have started
		// if we are out of power, wait until it's full before starting again
		if ( !ReadyToConstruct( ent, constructible, qfalse ) ) {
			bs->flags &= ~BFL_MISCFLAG;
			return qtrue;
		}

		bs->flags |= BFL_MISCFLAG;
		// VOICE: cover me!
		BotVoiceChatAfterIdleTime( bs->client, "CoverMe", SAY_TEAM, 500 + rand() % 1000, qfalse, 10000, qfalse  );
		// look downwards
		bs->ideal_viewangles[PITCH] = 70;
		// select pliers (the all-in-one handyman tool)
		bs->weaponnum = WP_PLIERS;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );
		// hold fire
		if ( bs->cur_ps.weapon == WP_PLIERS ) {
			trap_EA_Attack( bs->client );
		}
		return qtrue;
	}

	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	//
	// MOVEMENT REQUIRED
	//
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}

	//if the movement failed
	goalDist = VectorDistanceSquared( bs->origin, goal.origin );
	if ( moveresult.failure || ( VectorLengthSquared( bs->cur_ps.velocity ) < SQR( 3 ) && goalDist < ( 128 * 128 ) ) ) {  // allow for boxes at construction site that may get in the way
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

		// jump randomly?
		trap_EA_Jump( bs->client );
		trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
	}

	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else if ( moveresult.flags & MOVERESULT_WAITING ) { // if waiting for something
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_PlantMine()
==================
*/
void AIEnter_MP_PlantMine( bot_state_t *bs ) {
	bs->target_goal.flags &= ~GFL_NOSLOWAPPROACH;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_PlantMine;
	bs->ainodeText = "AINode_MP_PlantMine";
}

/*
==================
AINode_MP_PlantMine()
==================
*/
int AINode_MP_PlantMine( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;

	goal = bs->target_goal;

	//if we have changed class
	if ( bs->sess.playerType != PC_ENGINEER ) {
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	// if I have no landmines, I can't do this
	if ( !BotGotEnoughAmmoForWeapon( bs, WP_LANDMINE ) ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	// are we close enough to the goal?
	if ( VectorDistanceSquared( bs->origin, goal.origin ) < SQR( 32 ) ) {
		// VOICE: cover me!
		BotVoiceChatAfterIdleTime( bs->client, "CoverMe", SAY_TEAM, 500 + rand() % 1000, qfalse, 60000, qfalse  );
		// crouch down
		trap_EA_Crouch( bs->client );
		// look downwards
		bs->ideal_viewangles[PITCH] = 70;
		// have we recently dropped a land mine?
		trav = NULL;
		while ( ( trav = G_FindLandmine( trav ) ) ) {
			if ( !trav->parent || trav->parent->s.number != bs->client ) {
				continue;
			}
			if ( VectorDistanceSquared( bs->target_goal.origin, trav->r.currentOrigin ) > SQR( 100 ) ) {
				continue;
			}
			if ( trav->awaitingHelpTime < level.time - 5000 ) {
				continue;
			}
			if ( G_LandmineArmed( trav ) ) {
				bs->last_dangerousgoal = 0; // Gordon: force a reset
				if ( BotDangerousGoal( bs, &goal ) ) {
					AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
					return qfalse;
				}
			}
			// found some!
			bs->flags |= BFL_MISCFLAG;
			VectorCopy( trav->r.currentOrigin, bs->target_goal.origin );
			bs->target_goal.origin[2] += 24;
			break;
		}
		if ( !trav ) {
			// select the land mine
			bs->weaponnum = WP_LANDMINE;
			trap_EA_SelectWeapon( bs->client, bs->weaponnum );
			// hold fire
			if ( !VectorLengthSquared( bs->velocity ) && fabs( bs->viewangles[PITCH] - bs->ideal_viewangles[PITCH] ) < 2 && bs->cur_ps.weapon == bs->weaponnum && !bs->cur_ps.grenadeTimeLeft ) {
				trap_EA_Attack( bs->client );
			}
		} else if ( trav->s.teamNum >= 4 ) {  // the dynamite is not armed
			// switch to pliers and arm the dynamite
			bs->weaponnum = WP_PLIERS;
			trap_EA_SelectWeapon( bs->client, bs->weaponnum );
			// aim directly at the dynamite
			VectorCopy( bs->origin, target );
			target[2] += bs->cur_ps.viewheight;
			VectorSubtract( trav->r.currentOrigin, target, dir );
			VectorNormalize( dir );
			vectoangles( dir, bs->ideal_viewangles );
			// hold fire
			if ( bs->cur_ps.weapon == WP_PLIERS ) {
				trap_EA_Attack( bs->client );
			}
		} else {
			// the dynamite is armed, get out of here!
			BotDefaultNode( bs );
			return qfalse;
		}
		return qtrue;
	}

	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	// MOVEMENT REQUIRED
	//
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		BotDefaultNode( bs );
		return qfalse;
	}

	BotAIBlocked( bs, &moveresult, qtrue );

	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else if ( moveresult.flags & MOVERESULT_WAITING ) { // if waiting for something
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
		BotFindAndAttackEnemy( bs );
		BotUpdateViewAngles( bs, &goal, moveresult );
	}

	return qtrue;
}

/*
==================
AIEnter_MP_DisarmDynamite()
==================
*/
void AIEnter_MP_DisarmDynamite( bot_state_t *bs ) {
	// VOICE: cover me!
	BotVoiceChatAfterIdleTime( bs->client, "CoverMe", SAY_TEAM, 500 + rand() % 1000, qfalse, 4000, qfalse  );
	bs->target_goal.flags &= ~GFL_NOSLOWAPPROACH;
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_DisarmDynamite;
	bs->ainodeText = "AINode_MP_DisarmDynamite";
}

/*
==================
AINode_MP_DisarmDynamite()
==================
*/
int AINode_MP_DisarmDynamite( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	gentity_t *trav;
	float goalDist;

	goal = bs->target_goal;
	//if we have changed class
	if ( bs->sess.playerType != PC_ENGINEER ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	//if there are 2 bots going for this goal, then we should abort if we are further away
	goalDist = VectorDistanceSquared( bs->origin, goal.origin );
/*	if (numList = BotNumTeamMatesWithTarget( bs, goal.entitynum, list, 10 )) {
		for (i=0; i<numList; i++) {
			if (list[i] == bs->client) continue;
			if (botstates[list[i]].ainode == AINode_MP_DisarmDynamite) {
				if (VectorDistanceSquared( botstates[list[i]].origin, goal.origin ) < goalDist) {
					// make sure other bots dont head for this target also
					bs->ignore_specialgoal_time = 0;
					g_entities[goal.entitynum].missionLevel = level.time + 5000;
					BotDefaultNode( bs );
					return qfalse;
				}
			}
		}
	}
*/                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                //
	trav = BotGetEntity( goal.entitynum );

	// FIXME: temp hack in dealing with NULL returns from BotGetEntity (??)
	if ( trav == NULL ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}

	if ( !trav->inuse || trav->s.eType != ET_MISSILE || trav->s.weapon != WP_DYNAMITE ) {
		// it's gone, so reset goal
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	if ( trav->s.teamNum >= 4 ) { // the dynamite is not armed
		// make sure other bots dont head for this target also
		bs->ignore_specialgoal_time = 0;
		g_entities[goal.entitynum].missionLevel = level.time + 5000;
		BotDefaultNode( bs );
		return qfalse;
	}
	// make sure other bots dont head for this target also
	trav->missionLevel = level.time + 1000;
	//

	//map specific code
	BotMapScripts( bs );
	//
	// are we close enough to the goal?
	if ( goalDist < SQR( 32 ) ) {
		// crouch down
		trap_EA_Crouch( bs->client );
		//
		// switch to pliers and disarm the dynamite
		bs->weaponnum = WP_PLIERS;
		trap_EA_SelectWeapon( bs->client, bs->weaponnum );
		// aim directly at the dynamite
		VectorCopy( bs->origin, target );
		target[2] += bs->cur_ps.viewheight;
		VectorSubtract( trav->r.currentOrigin, target, dir );
		VectorNormalize( dir );
		vectoangles( dir, bs->ideal_viewangles );
		// hold fire
		if ( bs->cur_ps.weapon == WP_PLIERS ) {
			trap_EA_Attack( bs->client );
		}
		//
		return qtrue;
	}

	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	//choose the best weapon to fight with
	BotChooseWeapon( bs );

	// MOVEMENT REQUIRED
	//
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

		// jump randomly?
		trap_EA_Jump( bs->client );
		trap_EA_Move( bs->client, tv( crandom(), crandom(), crandom() ), 100 + random() * 200 );
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_Battle_Fight()
==================
*/
void AIEnter_MP_Battle_Fight( bot_state_t *bs ) {
	BotClearGoal( &bs->target_goal );
	trap_BotResetLastAvoidReach( bs->ms );
	bs->ainode = AINode_MP_Battle_Fight;
	bs->ainodeText = "AINode_MP_Battle_Fight";
}

/*
==================
AINode_MP_Battle_Fight()
==================
*/
int AINode_MP_Battle_Fight( bot_state_t *bs ) {
	int areanum;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	bot_goal_t goal;
	float enemydist;

	memset( &moveresult, 0, sizeof( moveresult ) );

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	// if using MOBILE MG42, conditionally go into special prone mode
	if ( bs->weaponnum == WP_MOBILE_MG42 ) {
		if ( VectorDistanceSquared( bs->origin, BotGetOrigin( bs->enemy ) ) > SQR( 700 ) ) {
			vec3_t dir, ang, mountedWeaponAngles;
			trace_t tr;
			vec3_t end;
			// make sure they are within pitch range
			VectorSubtract( BotGetOrigin( bs->enemy ), bs->eye, dir );
			VectorNormalize( dir );
			vectoangles( dir, ang );
			//
			VectorCopy( bs->origin, end );
			end[2] -= 40;
			trap_Trace( &tr, bs->origin, vec3_origin, vec3_origin, end, bs->client, MASK_PLAYERSOLID );
			if ( tr.fraction < 1.f ) {
				vec3_t axis[3], forward, right;

				AngleVectors( bs->viewangles, forward, right, NULL );
				forward[2] = 0;
				right[2] = 0;
				PM_ClipVelocity( forward, tr.plane.normal, forward, OVERCLIP );
				PM_ClipVelocity( right, tr.plane.normal, right, OVERCLIP );
				//
				VectorNormalize( forward );
				VectorNormalize( right );
				//
				VectorCopy( forward, axis[0] );
				VectorCopy( right, axis[2] );
				CrossProduct( axis[0], axis[2], axis[1] );
				AxisToAngles( axis, mountedWeaponAngles );

				// make sure these angles are within view limits towards the enemy
				if ( fabs( AngleDifference( mountedWeaponAngles[PITCH], ang[PITCH] ) ) < 15.f ) {
					int oldviewheight;
					// check for obstruction at feet
					oldviewheight = level.clients[bs->client].ps.viewheight;
					level.clients[bs->client].ps.viewheight = PRONE_VIEWHEIGHT;
					if ( BotVisibleFromPos( bs->origin, bs->client, BotGetOrigin( bs->enemy ), bs->enemy, qtrue ) ) {
						AIEnter_MP_Battle_MobileMG42( bs );
						return qfalse;
					}
				}
			}
		}
	}
	// check for dangerous elements
	VectorCopy( bs->origin, goal.origin );
	goal.areanum = bs->areanum;
	goal.entitynum = bs->client;
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	enemydist = VectorDistanceSquared( bs->origin, entinfo.origin );
	//if the enemy is dead
	if ( bs->enemydeath_time ) {
		if ( bs->enemydeath_time < trap_AAS_Time() - 0.3 ) {
			bs->ignore_specialgoal_time = 0;
			bs->enemydeath_time = 0;

			bs->enemy = -1;
			BotDefaultNode( bs );
			return qfalse;
		}
	} else {
		if ( EntityIsDead( &entinfo ) ) {
			bs->enemydeath_time = trap_AAS_Time();
		}
	}
	//if the enemy is invisible and not shooting the bot looses track easily
	if ( EntityIsInvisible( &entinfo ) && !EntityIsShooting( &entinfo ) ) {
		if ( random() < 0.2 ) {
			BotDefaultNode( bs );
			return qfalse;
		}
	}
	//update the reachability area and origin if possible
	areanum = BotGetArea( bs->enemy ); //BotPointAreaNum(entinfo.number, entinfo.origin);
	if ( areanum ) {
		VectorCopy( entinfo.origin, bs->lastenemyorigin );
		bs->lastenemyareanum = areanum;
	}
	//update the attack inventory values
	BotUpdateBattleInventory( bs, bs->enemy );
	// get in real close to enemy flag carriers
	if ( BotCarryingFlag( bs->enemy ) && VectorDistanceSquared( bs->origin, bs->lastenemyorigin ) > SQR( 72 ) ) {
		if ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->lastenemyareanum, bs->tfl ) ) {
			AIEnter_MP_Battle_Chase( bs );
			return qfalse;
		}
	}
	//if the enemy is not visible, or is too far away
	if ( ( bs->weaponnum == WP_KNIFE ) || ( enemydist > SQR( BotWeaponRange( bs, bs->weaponnum ) ) ) || !BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
		if ( BotWantsToChase( bs ) ) {
			AIEnter_MP_Battle_Chase( bs );
			return qfalse;
		} else {
			bs->ignore_specialgoal_time = 0;
			BotDefaultNode( bs );
			return qtrue;   // wait until next frame, incase this triggers a loop
		}
	}
	//use holdable items
	BotBattleUseItems( bs );
	//

	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	if ( BotMoveWhileFiring( bs->weaponnum ) ) {
		//do attack movements
		moveresult = BotAttackMove( bs, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

	}
	//
	BotAIBlocked( bs, &moveresult, qfalse );
	//aim at the enemy
	BotAimAtEnemy( bs );
	//attack the enemy if possible
	if ( !BotCheckAttack( bs ) ) {
		if ( BotWantsToChase( bs ) ) {
			AIEnter_MP_Battle_Chase( bs );
			return qfalse;  // chase them immediately
		} else {
			BotDefaultNode( bs );
			return qtrue;   // prevent loop
		}
	}
	//if the bot wants to retreat
	if ( BotWantsToRetreat( bs ) ) {
		BotFindSpecialGoals( bs );
		return qtrue;
	}
	return qtrue;
}

/*
==================
AIEnter_MP_Battle_Chase()
==================
*/
void AIEnter_MP_Battle_Chase( bot_state_t *bs ) {
	bs->altenemy = -1;
	BotClearGoal( &bs->target_goal );
	bs->chase_time = trap_AAS_Time();
	bs->ainode = AINode_MP_Battle_Chase;
	bs->ainodeText = "AINode_MP_Battle_Chase";
}

/*
==================
AINode_MP_Battle_Chase()
==================
*/
int AINode_MP_Battle_Chase( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	aas_entityinfo_t entinfo;
	int oldenemy;
	float enemydist;

	memset( &moveresult, 0, sizeof( moveresult ) );

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// RF, if we have a leader, go back to protecting them
	if ( ( bs->leader > -1 ) && !BotCarryingFlag( bs->enemy ) ) {
		AIEnter_MP_DefendTarget( bs );
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( !BotCarryingFlag( bs->enemy ) && BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	//if the enemy is visible
	BotEntityInfo( bs->enemy, &entinfo );
	VectorCopy( entinfo.origin, bs->lastenemyorigin );
	enemydist = VectorDistanceSquared( bs->origin, bs->lastenemyorigin );
	if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
		// update chase org
		bs->lastenemyareanum = BotPointAreaNum( bs->enemy, entinfo.origin );
		bs->chase_time = trap_AAS_Time() + 5.0;
		// RF, if we are in the air, wait until we land (we may be climbing a ladder)
		if ( bs->cur_ps.groundEntityNum != ENTITYNUM_NONE || bs->cur_ps.velocity[2] < 0 ) {
			// get real close to flag carrier
			if ( ( bs->weaponnum != WP_KNIFE ) && ( ( !BotCarryingFlag( bs->enemy ) && ( enemydist < SQR( BotWeaponRange( bs, bs->weaponnum ) ) ) ) || ( enemydist < SQR( 64 ) ) ) ) {
				// make sure it is safe to attack
				if ( BotCheckAttack( bs ) ) {
					AIEnter_MP_Battle_Fight( bs );
					return qfalse;
				}
			} else {    // attack and keep moving towards them
				//choose the best weapon to fight with
				BotChooseWeapon( bs );
				//aim at the enemy
				BotAimAtEnemy( bs );
				//attack the enemy if possible
				if ( bs->weaponnum == bs->cur_ps.weapon ) {
					BotCheckAttack( bs );
				}
			}
		}
	}
	//
	if ( !BotCarryingFlag( bs->enemy ) ) {
		int oldEnemy;
		//if there is another enemy
		oldEnemy = bs->enemy;
		if ( ( bs->weaponnum != WP_KNIFE ) && BotFindEnemyMP( bs, -1, qfalse ) && ( bs->enemy != oldEnemy ) ) {
			AIEnter_MP_Battle_Fight( bs );
			return qfalse;
		}
	}
	//there is no last enemy area
	if ( ( bs->weaponnum != WP_KNIFE ) && !bs->lastenemyareanum ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//

	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//map specific code
	BotMapScripts( bs );
	//create the chase goal
	goal.entitynum = bs->enemy;
	goal.areanum = bs->lastenemyareanum;
	VectorCopy( bs->lastenemyorigin, goal.origin );
	VectorSet( goal.mins, -8, -8, -8 );
	VectorSet( goal.maxs, 8, 8, 8 );
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
	//if the last seen enemy spot is reached the enemy could not be found
	if ( ( bs->weaponnum != WP_KNIFE ) && trap_BotTouchingGoal( bs->origin, &goal ) ) {
		bs->chase_time = 0;
	}
	//if there's no chase time left
	if ( !bs->chase_time || bs->chase_time < trap_AAS_Time() - 10 ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	BotUpdateBattleInventory( bs, bs->enemy );
	//initialize the movement state
	BotSetupForMovement( bs );
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		bs->enemy = -1;
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);

		// try and find something else to do
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	BotAIBlocked( bs, &moveresult, qfalse );
	//
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else {
		// forget about our current enemy temporarily, so we can shoot at other while pursuing the flag carrier
		oldenemy = bs->enemy;
		if ( BotCarryingFlag( oldenemy ) ) {
			// check for enemies
			bs->enemy = -1;
			if ( bs->altenemy < 0 ) {
				BotFindEnemyMP( bs, -1, qfalse );
			} else {
				bs->enemy = bs->altenemy;
			}
			if ( bs->enemy > -1 ) {
				bs->altenemy = bs->enemy;
				// attack and keep moving
				if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
					//choose the best weapon to fight with
					BotChooseWeapon( bs );
					//aim at the enemy
					BotAimAtEnemy( bs );
					//attack the enemy if possible
					if ( bs->weaponnum == bs->cur_ps.weapon ) {
						BotCheckAttack( bs );
					}
				} else {
					bs->altenemy = -1;
				}
			} else {
				bs->altenemy = -1;
			}
		}
		//
		if ( ( bs->altenemy < 0 || !BotCarryingFlag( oldenemy ) ) && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
		// restore the enemy
		bs->enemy = oldenemy;
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//if the bot is in the area the enemy was last seen in
	if ( bs->areanum == bs->lastenemyareanum ) {
		bs->chase_time = 0;
	}
	//if the bot wants to retreat (the bot could have been damage during the chase)
	if ( BotWantsToRetreat( bs ) ) {
		AIEnter_MP_Battle_Retreat( bs );
		return qtrue;
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_Battle_Retreat()
==================
*/
void AIEnter_MP_Battle_Retreat( bot_state_t *bs ) {
	bs->ainode = AINode_MP_Battle_Retreat;
	bs->ainodeText = "AINode_MP_Battle_Retreat";
}

/*
==================
AINode_MP_Battle_Retreat()
==================
*/
int AINode_MP_Battle_Retreat( bot_state_t *bs ) {
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	int areanum;

	memset( &moveresult, 0, sizeof( moveresult ) );

	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	if ( EntityIsDead( &entinfo ) ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//

	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//map specific code
	BotMapScripts( bs );
	//update the attack inventory values
	BotUpdateBattleInventory( bs, bs->enemy );
	//if the bot doesn't want to retreat anymore... probably picked up some nice items
	if ( BotWantsToChase( bs ) ) {
		//empty the goal stack, when chasing, only the enemy is the goal
		trap_BotEmptyGoalStack( bs->gs );
		//go chase the enemy
		AIEnter_MP_Battle_Chase( bs );
		return qfalse;
	}
	//update the last time the enemy was visible
	if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy, NULL ) ) {
		bs->enemyvisible_time = trap_AAS_Time();
		//update the reachability area and origin if possible
		areanum = BotPointAreaNum( entinfo.number, entinfo.origin );
		if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
			VectorCopy( entinfo.origin, bs->lastenemyorigin );
			bs->lastenemyareanum = areanum;
		}
	}
	//if the enemy is NOT visible for 4 seconds
	if ( bs->enemyvisible_time < trap_AAS_Time() - 4 ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//else if the enemy is NOT visible
	else if ( bs->enemyvisible_time < trap_AAS_Time() ) {
		//if there is another enemy
		if ( BotFindEnemyMP( bs, -1, qfalse ) ) {
			AIEnter_MP_Battle_Fight( bs );
			return qfalse;
		}
	}
	//
	//use holdable items
	BotBattleUseItems( bs );
	//
	// !!! RF, we should try to move somewhere safe here
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//if the view is fixed for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEW
							  //|MOVERESULT_SWIMVIEW
							  ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else if ( !( moveresult.flags & MOVERESULT_MOVEMENTVIEWSET )
				&& !( bs->flags & BFL_IDEALVIEWSET ) ) {
		BotAimAtEnemy( bs );
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//attack the enemy if possible
	BotCheckAttack( bs );
	//
	return qtrue;
}

/*
==================
AIEnter_MP_Script_MoveToMarker()
==================
*/
void AIEnter_MP_Script_MoveToMarker( bot_state_t *bs ) {
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_Script_MoveToMarker;
	bs->ainodeText = "AINode_MP_Script_MoveToMarker";
}

/*
==================
AINode_MP_Script_MoveToMarker()
==================
*/
int AINode_MP_Script_MoveToMarker( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	g_serverEntity_t *marker;
	//
	goal = bs->target_goal;
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( !BotIsPOW( bs ) ) {
		if ( BotCheckEmergencyTargets( bs ) ) {
			return qfalse;
		}
	}
	// check for dangerous elements
	if ( !BotIsPOW( bs ) ) {
		if ( BotDangerousGoal( bs, &goal ) ) {
			AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
			return qfalse;
		}
	}
/*	// check for special class actions
	if (BotCheckClassActions(bs)) {
		return qfalse;
	}
*/                                                                                            // look for health/ammo packs
	if ( !BotIsPOW( bs ) ) {
		if ( BotFindNearbyGoal( bs ) ) {
			AIEnter_MP_Seek_NBG( bs );
			return qfalse;
		}
	}
	if ( !BotIsPOW( bs ) ) {
		if ( BotFindSpecialGoals( bs ) ) {
			return qfalse;
		}
	}
	//
	// should we stop pursuing this target?
	marker = GetServerEntity( bs->target_goal.entitynum );
	if ( !marker || !marker->inuse ) {
		bs->ignore_specialgoal_time = 0;
		BotDefaultNode( bs );
		return qfalse;
	}
	//
	VectorCopy( goal.origin, target );
	if ( fabs( target[2] - bs->origin[2] ) < 80 ) {
		target[2] = bs->origin[2];
	}
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//

	//map specific code
	BotMapScripts( bs );
	//initialize the movement state
	BotSetupForMovement( bs );
	//
	memset( &moveresult, 0, sizeof( moveresult ) );
	//
	if ( VectorDistanceSquared( bs->origin, target ) >= SQR( 12 ) ) {
		//try a direct movement
		if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
			//move towards the goal
			BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
		}
		//if the movement failed
		if ( moveresult.failure ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			if ( !( bs->flags & BFL_MISCFLAG ) ) {
				Bot_ScriptLog_Entry( bs, qfalse, NULL, "movement failure" );
				bs->flags |= BFL_MISCFLAG;
			}
			return qtrue;
		} else {
			if ( bs->flags & BFL_MISCFLAG ) {
				Bot_ScriptLog_Entry( bs, qfalse, NULL, "resumed movement" );
				bs->flags &= ~BFL_MISCFLAG;
			}
		}
		// set the movement type
		if ( bs->script.moveType == BSMT_WALKING ) {
			trap_EA_Walk( bs->client );
		} else if ( bs->script.moveType == BSMT_CROUCHING ) {
			trap_EA_Crouch( bs->client );
		}
		//
		BotAIBlocked( bs, &moveresult, qtrue );
	} else {
		// Gordon: TEMP: i doubt this should be here
		if ( BotIsPOW( bs ) ) {
			BotDefaultNode( bs );
			return qfalse;
		}
	}

	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
			// check for giving ourselves some ammo
			if ( bs->sess.playerType == PC_FIELDOPS && ClientNeedsAmmo( bs->client ) ) {
				// switch to regen and pump away
				bs->weaponnum = WP_AMMO;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				if ( bs->cur_ps.weapon == WP_AMMO && BotWeaponCharged( bs, WP_AMMO ) ) {
					trap_EA_Attack( bs->client );
				}
			}
			// check for giving ourselves some health
			if ( bs->sess.playerType == PC_MEDIC && BotHealthScale( bs->client ) < 1.0 ) {
				// switch to regen and pump away
				bs->weaponnum = WP_MEDKIT;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				if ( bs->cur_ps.weapon == WP_MEDKIT && BotWeaponCharged( bs, WP_MEDKIT ) ) {
					trap_EA_Attack( bs->client );
				}
			}
		}
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_MoveToAutonomyRange()
==================
*/
void AIEnter_MP_MoveToAutonomyRange( bot_state_t *bs ) {
	vec3_t pos;
	//
	if ( !BotGetMovementAutonomyPos( bs, pos ) ) {
		if ( g_developer.integer ) {
			G_Printf( "AIEnter_MP_MoveToAutonomyRange: autonomy pos unknown\n" );
		}
	}
	// make the current goal our autonomy spot
	BotClearGoal( &bs->target_goal );
	VectorCopy( pos, bs->target_goal.origin );
	bs->target_goal.areanum = BotPointAreaNum( bs->client, pos );
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->ainode = AINode_MP_MoveToAutonomyRange;
	bs->ainodeText = "AINode_MP_MoveToAutonomyRange";
}

/*
==================
AINode_MP_MoveToAutonomyRange()
==================
*/
int AINode_MP_MoveToAutonomyRange( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	//
	goal = bs->target_goal;
	//
	if ( BotIsObserver( bs ) ) {
		AIEnter_MP_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_MP_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_MP_Respawn( bs );
		return qfalse;
	}
	// check for emergency targets (flags, etc)
	if ( BotCheckEmergencyTargets( bs ) ) {
		return qfalse;
	}
	// check for dangerous elements
	if ( BotDangerousGoal( bs, &goal ) ) {
		AIEnter_MP_AvoidDanger( bs ); // avoid this danger until it passes
		return qfalse;
	}
/*	// check for special class actions
	if (BotCheckClassActions(bs)) {
		return qfalse;
	}
*/                                                                                            // look for health/ammo packs
	//if (BotFindNearbyGoal( bs )) {
	//	AIEnter_MP_Seek_NBG( bs );
	//	return qfalse;
	//}
	//if (BotFindSpecialGoals(bs)) {
	//	return qfalse;
	//}
	//
	// should we stop pursuing this target?
	//ent = BotGetEntity( bs->target_goal.entitynum );
	//if (!ent->inuse) {
	//	bs->ignore_specialgoal_time = 0;
	//	BotDefaultNode(bs);
	//	return qfalse;
	//}
	//
	VectorCopy( goal.origin, target );
	if ( fabs( target[2] - bs->origin[2] ) < 80 ) {
		target[2] = bs->origin[2];
	}
	//
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//

	//map specific code
	BotMapScripts( bs );
	//initialize the movement state
	BotSetupForMovement( bs );
	//
	memset( &moveresult, 0, sizeof( moveresult ) );
	//
	{
		float halfRange = ( 0.5 * BotGetMovementAutonomyRange( bs, NULL ) );

		if ( VectorDistanceSquared( bs->origin, target ) <= SQR( halfRange ) ) {
			// go back to standing
			AIEnter_MP_Stand( bs );
			return qfalse;
		}
	}
	//
	//try a direct movement
	if ( !BotDirectMoveToGoal( bs, &goal, &moveresult ) ) {
		//move towards the goal
		BotMP_MoveToGoal( bs, &moveresult, bs->ms, &goal, bs->tfl );
	}
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		if ( !( bs->flags & BFL_MISCFLAG ) ) {
			Bot_ScriptLog_Entry( bs, qfalse, NULL, "movement failure" );
			bs->flags |= BFL_MISCFLAG;
		}
		return qtrue;
	} else {
		if ( bs->flags & BFL_MISCFLAG ) {
			Bot_ScriptLog_Entry( bs, qfalse, NULL, "resumed movement" );
			bs->flags &= ~BFL_MISCFLAG;
		}
	}
	// set the movement type
	if ( bs->script.moveType == BSMT_WALKING ) {
		trap_EA_Walk( bs->client );
	} else if ( bs->script.moveType == BSMT_CROUCHING ) {
		trap_EA_Crouch( bs->client );
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else {
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
					//aim at the enemy
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
		//
		if ( bs->enemy < 0 && !( bs->flags & BFL_IDEALVIEWSET ) ) {
			if ( moveresult.flags & MOVERESULT_DIRECTMOVE ) {
				VectorSubtract( goal.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			}
			//FIXME: look at cluster portals?
			else if ( VectorLengthSquared( moveresult.movedir ) ) {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			} else if ( random() < bs->thinktime * 0.8 )     {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			bs->ideal_viewangles[2] *= 0.5;
			// check for giving ourselves some ammo
			if ( bs->sess.playerType == PC_FIELDOPS && ClientNeedsAmmo( bs->client ) ) {
				// switch to regen and pump away
				bs->weaponnum = WP_AMMO;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				if ( bs->cur_ps.weapon == WP_AMMO && BotWeaponCharged( bs, WP_AMMO ) ) {
					trap_EA_Attack( bs->client );
				}
			}
			// check for giving ourselves some health
			if ( bs->sess.playerType == PC_MEDIC && BotHealthScale( bs->client ) < 1.0 ) {
				// switch to regen and pump away
				bs->weaponnum = WP_MEDKIT;
				trap_EA_SelectWeapon( bs->client, bs->weaponnum );
				if ( bs->cur_ps.weapon == WP_MEDKIT && BotWeaponCharged( bs, WP_MEDKIT ) ) {
					trap_EA_Attack( bs->client );
				}
			}
		}
	}
	// reload?
	if ( ( bs->last_fire != level.time ) && ( bs->cur_ps.ammoclip[BG_FindClipForWeapon( bs->cur_ps.weapon )] < (int)( 0.8 * GetAmmoTableData( bs->cur_ps.weapon )->maxclip ) ) && bs->cur_ps.ammo[BG_FindAmmoForWeapon( bs->cur_ps.weapon )] ) {
		trap_EA_Reload( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_MP_NavigateFromVoid()
==================
*/
void AIEnter_MP_NavigateFromVoid( bot_state_t *bs ) {
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	bs->flags &= ~BFL_MISCFLAG;
	bs->last_SparseDefense = 0;
	bs->ainode = AINode_MP_NavigateFromVoid;
	bs->ainodeText = "AINode_MP_NavigateFromVoid";
}

/*
==================
AINode_MP_NavigateFromVoid()
==================
*/
int AINode_MP_NavigateFromVoid( bot_state_t *bs ) {
	const int checkFrames = 40;
	const float checkFrametime = 0.05;
	const int randomChecksPerSlice = 5;
	int i;
	vec3_t dir, angles;
	aas_clientmove_t move;
	qboolean success;
	float bestDist;
	vec3_t bestDir;

	// are we out of the void?
	if ( bs->areanum ) {
		BotDefaultNode( bs );
		return qfalse;
	}

	// try to navigate out of the void

	// if we have a current direction, validate it

	if ( bs->flags & BFL_MISCFLAG ) {

		// we are currently heading in a direction, verify to make sure it's still valid
		if ( bs->last_attackShout < level.time - 250 ) {
			VectorCopy( bs->lastLeaderPosition, dir );
			success = qfalse;
			if ( trap_AAS_PredictClientMovement( &move, bs->client, bs->origin, -1, qfalse, dir, vec3_origin, checkFrames, 0, checkFrametime, SE_HITGROUNDDAMAGE | SE_HITGROUNDAREA | SE_STUCK | SE_GAP, -1, qfalse ) ) {
				switch ( move.stopevent ) {
				case SE_HITGROUNDAREA:
					// success!
					success = qtrue;
					// keep it valid
					bs->last_SparseDefense = level.time + 500;
					break;
				}
			}
			//
			if ( !success ) {
				bs->flags &= ~BFL_MISCFLAG;
			}
		}

	}

	// do we need a new direction?

	if ( !( bs->flags & BFL_MISCFLAG ) ) {

		bestDist = -1.0;
		for ( i = 0; i < randomChecksPerSlice; i++ ) {
			// choose a random direction
			VectorClear( angles );
			angles[YAW] = rand() % 360;
			AngleVectors( angles, dir, NULL, NULL );

			// how far will this direction get us?
			success = qfalse;
			if ( trap_AAS_PredictClientMovement( &move, bs->client, bs->origin, -1, qfalse, dir, vec3_origin, checkFrames, 0, checkFrametime, SE_HITGROUNDDAMAGE | SE_HITGROUNDAREA | SE_STUCK | SE_GAP, -1, qfalse ) ) {
				switch ( move.stopevent ) {
				case SE_HITGROUNDAREA:
					// success!
					success = qtrue;
					break;
				}
			}
			//
			if ( success ) {
				bs->flags |= BFL_MISCFLAG;
				bs->last_attackShout = level.time;
				bs->last_SparseDefense = level.time + 200;
				VectorCopy( dir, bs->lastLeaderPosition );
				break;
			} else {
				switch ( move.stopevent ) {
				case SE_HITGROUNDDAMAGE:
				case SE_GAP:
					// bad direction
					break;
				default:
					// Gordon: OPT: bleh....
					if ( bestDist < 0 || VectorDistanceSquared( bs->origin, move.endpos ) < SQR( bestDist ) ) {
						bestDist = VectorDistance( bs->origin, move.endpos );
						VectorCopy( dir, bestDir );
					}
				}
			}
		}

		if ( !( bs->flags & BFL_MISCFLAG ) && ( bestDist > 0.0 ) ) {
			bs->last_SparseDefense = level.time + 200;
			VectorCopy( bestDir, bs->lastLeaderPosition );
		}

	}

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
				//aim at the enemy
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

	// do we have a current direction?
	if ( bs->last_SparseDefense > level.time ) {
		// continue in that direction
		trap_EA_Move( bs->client, bs->lastLeaderPosition, 400 );
		if ( bs->enemy < 0 ) {
			vectoangles( bs->lastLeaderPosition, angles );
			trap_EA_View( bs->client, angles );
		}
	}

	return qtrue;
}
