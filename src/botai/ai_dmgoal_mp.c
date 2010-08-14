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
 * name:		ai_dmgoal_mp.c
 *
 * desc:		Wolf bot AI
 *
 *
 *****************************************************************************/

//
// MULTIPLAYER GOAL AI
//

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
//
#include "ai_dmnet_mp.h"
#include "ai_dmgoal_mp.h"

static bot_goal_t target;

/*
==================
BotMP_CheckClassActions()
==================
*/
qboolean BotMP_CheckClassActions( bot_state_t *bs ) {
	qboolean hasLeader;
	//
	// if carrying flag, screw others
	if ( BotCarryingFlag( bs->client ) && bs->enemy > -1 ) {
		return qfalse;
	}

	// if we are following a carrier, or a non-bot, then stay close
	hasLeader = qfalse;
	if ( bs->leader > -1 ) {
		if ( BotCarryingFlag( bs->leader ) || !( g_entities[bs->leader].r.svFlags & SVF_BOT ) ) {
			hasLeader = qtrue;
		}
	}

/*	if( bs->script.frameFlags & BSFFL_MOVETOTARGET ) {
		hasLeader = qtrue;	// dont go so far if we're trying to get to a marker
	}*/

	if ( bs->ainode == AINode_MP_MoveToAutonomyRange ) {
		hasLeader = qtrue;  // dont go too far off course
	}

	if ( BotClass_MedicCheckRevives( bs, ( hasLeader ? 300 : 600 ), &target, qtrue ) ) {
		if ( target.entitynum != bs->target_goal.entitynum || bs->ainode != AINode_MP_MedicRevive ) {
			bs->target_goal = target;
			AIEnter_MP_MedicRevive( bs );
		}
		return qtrue;
	}

	if ( BotCarryingFlag( bs->client ) ) {
		return qfalse;  // only revive if we have flag (Gordon: or skip other checks if we have flag to make more sense...)
	}

	if ( BotClass_MedicCheckGiveHealth( bs, ( hasLeader ? 200 : 800 ), &target ) ) {
		if ( target.entitynum != bs->target_goal.entitynum || bs->ainode != AINode_MP_MedicGiveHealth ) {
			bs->target_goal = target;
			AIEnter_MP_MedicGiveHealth( bs );
		}
		return qtrue;
	}

	if ( BotClass_LtCheckGiveAmmo( bs, ( hasLeader ? 200 : 800 ), &target ) ) {
		if ( target.entitynum != bs->target_goal.entitynum || bs->ainode != AINode_MP_GiveAmmo ) {
			bs->target_goal = target;
			AIEnter_MP_GiveAmmo( bs );
		}
		return qtrue;
	}

	return qfalse;
}

/*
===================
BotMP_CheckEmergencyGoals
===================
*/
qboolean BotMP_CheckEmergencyGoals( bot_state_t *bs ) {
	gentity_t *trav, *flag;
	int i, t;
	int list[32], numList;
	int oldest = 0, oldestTime, oldIgnoreTime, areanum, numTeammates;
	float dist, bestDist;
	vec3_t center, brushPos, vec;
	trace_t tr;
	//
	if ( bs->last_checkemergencytargets > level.time - 300 ) {
		return qfalse;
	}
	bs->last_checkemergencytargets = level.time + rand() % 200;

	//
	oldIgnoreTime = bs->ignore_specialgoal_time;
	bs->ignore_specialgoal_time = level.time + 2000;
	numTeammates = BotNumTeamMates( bs, NULL, 0 );

	// check for voice chats
	BotCheckVoiceChats( bs );

	// if a bot finds itself in a void, it should use DrirectMove() to get out of it
	if ( !bs->areanum ) {
		if ( bs->ainode == AINode_MP_NavigateFromVoid ) {
			return qfalse;
		}
		AIEnter_MP_NavigateFromVoid( bs );
		return qtrue;
	}

	// DEBUG MODE: bot_debug = 11
	trap_Cvar_Update( &bot_debug );
	if ( g_cheats.integer && bot_debug.integer == BOT_DEBUG_FOLLOW_PLAYER ) {
		bs->leader = 0;
		if ( BotGoalForEntity( bs, bs->leader, &target, BGU_LOW ) ) {
			if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
				return qfalse;
			}
			bs->target_goal = target;
			AIEnter_MP_DefendTarget( bs );
			return qtrue;
		} else {
			bs->leader = -1;
		}
		// DEBUG MODE: bot_debug = 12 (dont look for goals)
	} else if ( g_cheats.integer && bot_debug.integer == BOT_DEBUG_FOLLOW_PLAYER + 1 ) {
		return qfalse;
	}

	if ( !BotIsPOW( bs ) ) {
		// check for being outside autonomy range within reasonable cause
		if ( BotCheckMovementAutonomy( bs, &bs->target_goal ) ) {
			if ( bs->target_goal.entitynum == bs->leader && bs->ainode == AINode_MP_DefendTarget ) {
				return qfalse;
			}
			if ( bs->ainode == AINode_MP_MoveToAutonomyRange ) {
				return qfalse;
			}
			// get back to it
			AIEnter_MP_MoveToAutonomyRange( bs );
			return qtrue;
		}

		// check for dropped flag
		if ( BotFindDroppedFlag( &trav ) ) {
			// if we are the owner and we just dropped it, then ignore it
			if ( !( trav->r.ownerNum == bs->client && trav->botIgnoreTime > level.time ) ) {
				// if we are already going for it
				if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_TouchTarget ) {
					return qfalse;  //keep going for it
				}
				// GO GET IT
				// do we have a route to the enemy flag?
				if ( BotGoalForEntity( NULL, trav->s.number, &target, BGU_HIGH ) ) {
					target.flags = GFL_NOSLOWAPPROACH;
					//
					if ( VectorDistanceSquared( target.origin, bs->origin ) < ( 1600 * 1600 ) || trap_InPVS( target.origin, bs->origin ) ) {
						if ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl ) ) {
							bs->target_goal = target;
							AIEnter_MP_TouchTarget( bs );
							return qtrue;
						}
						//if (g_cheats.integer)
						//	G_Printf( "WARNING: dropped objective is unreachable\n" );
					}
				}
			}
		}
	} else {
		return qtrue;
	}

	// if we have a visible & damagable script_mover target, then we should try to blow it up
	if ( ( trav = BotGetVisibleDamagableScriptMover( bs ) ) ) {
		if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_AttackTarget ) {
			return qfalse;
		}

		if ( BotGoalForEntity( bs, trav->s.number, &target, BGU_MAXIMUM ) ) {
			bs->target_goal = target;
			AIEnter_MP_AttackTarget( bs );
			return qtrue;
		}
	}

	// if a checkpoint is nearby, touch it
	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_CHECKPOINT ) ) ) {
		// if the opposition team controls this checkpoint, or it hasnt been captured yet
		if ( trav->count == ( bs->sess.sessionTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS ) || ( trav->count < 0 ) ) {
			// if we can see it
			VectorAdd( trav->r.absmin, trav->r.absmax, center );
			VectorScale( center, 0.5, center );
			if ( VectorDistanceSquared( center, bs->origin ) > ( 1024 * 1024 ) ) {
				continue;
			}
			center[0] = trav->r.absmax[0];
			center[1] = trav->r.absmax[1];
			VectorSubtract( trav->r.absmax, trav->r.absmin, vec );
			vec[2] = 0;
			VectorNormalize( vec );
			VectorAdd( center, vec, brushPos );
			if ( !trap_InPVS(  brushPos, bs->origin ) ) {
				continue;
			}
			// GO GET IT
			// do we have a route to the enemy flag?
			if ( BotGoalForEntity( bs, trav->s.number, &target, BGU_MEDIUM ) ) {
				// if we are already going for it
				if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_TouchTarget ) {
					return qfalse;  //keep going for it
				}
				target.flags = GFL_NOSLOWAPPROACH;
				//
				if ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl ) ) {
					bs->target_goal = target;
					AIEnter_MP_TouchTarget( bs );
					return qtrue;
				}
			}
		}
	}

	// in GT_WOLF mode on defense, if the flag is not at base, then all units should help get it back
	if ( level.captureFlagMode ) {
		//
		if ( bs->sess.sessionTeam != level.attackingTeam ) {
			//
			if ( !BotFlagAtBase( bs->sess.sessionTeam, &flag ) ) {
				//
				// if we are pursuing the flag carrier, then continue
				if ( bs->enemy >= 0 && BotCarryingFlag( bs->enemy ) ) {
					return qfalse;
				}
				//
				// go to the transmission point?
				trav = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
				if ( !trav ) {
					trav = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
				}
				if ( trav ) {
					//
					if ( BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) < (int)ceil( 0.5 * numTeammates ) ) {
						//
						// if we are already heading for it, then continue
						if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_DefendTarget ) {
							return qfalse;
						}
						//
						VectorAdd( trav->r.absmin, trav->r.absmax, brushPos );
						VectorScale( brushPos, 0.5, brushPos );
						// find the best goal area
						numList = trap_AAS_BBoxAreas( trav->r.absmin, trav->r.absmax, list, 32 );
						if ( numList ) {
							oldestTime = -1;
							bestDist = -1;
							for ( i = 0; i < numList; i++ ) {
								if ( !trap_AAS_AreaReachability( list[i] ) ) {
									continue;
								}
								t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
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
							if ( bestDist > 0 && oldestTime < 4000 && bestDist < 2000 ) {
								BotClearGoal( &target );
								// use this as the goal origin
								VectorCopy( center, target.origin );
								VectorCopy( bs->cur_ps.mins, target.mins );
								VectorCopy( bs->cur_ps.maxs, target.maxs );
								target.areanum = oldest;
								target.entitynum = trav->s.number;
								target.flags = GFL_NOSLOWAPPROACH;
								//
								if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_HIGH ) ) {
									// if we are already heading there, continue
									if ( bs->target_goal.entitynum && bs->target_goal.entitynum == trav->s.number ) {
										return qfalse;
									}
									//
									bs->target_goal = target;
									BotFindSparseDefendArea( bs, &bs->target_goal, qtrue );
									AIEnter_MP_DefendTarget( bs );
									return qtrue;
								}
							}
						}

					}/* else {	// go for the document room

						BotFlagAtBase(bs->sess.sessionTeam, &flag);
						//
						// do we have a route to the flag?
						BotClearGoal(&target);
						target.entitynum = flag->s.number;
						VectorCopy( flag->r.currentOrigin, center );
						center[2] += 30;
						target.areanum = trap_AAS_PointAreaNum(center);
						target.flags = GFL_NOSLOWAPPROACH;
						VectorCopy( flag->r.mins, target.mins );
						VectorCopy( flag->r.maxs, target.maxs );
						VectorCopy( flag->r.currentOrigin, target.origin );
						//
						t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, target.areanum, bs->tfl);
						if (t) {
							// if we are already heading there, continue
							if (bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_TouchTarget) return qfalse;
							bs->target_goal = target;
							BotFindSparseDefendArea(bs, &bs->target_goal);
							AIEnter_MP_DefendTarget(bs);
							return qtrue;
						}
					}*/
				}
				// keep sending some troops to the flag carrier, so we have the most direct route covered
				flag = BotGetEnemyFlagCarrier( bs );
				if ( flag && ( VectorDistanceSquared( flag->r.currentOrigin, bs->eye ) < ( 1024 * 1024 ) || trap_InPVS( flag->r.currentOrigin, bs->eye ) || BotNumTeamMatesWithTarget( bs, flag->s.number, NULL, 0 ) < 1 ) ) {
					// HACK, err... lets just take a snapshot of where they are now, and go down there
					bs->enemyvisible_time = trap_AAS_Time();
					//update the reachability area and origin if possible
					areanum = BotPointAreaNum( flag->s.number, flag->r.currentOrigin );
					if ( areanum && trap_AAS_AreaReachability( areanum ) && trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl ) ) {
						VectorCopy( flag->r.currentOrigin, bs->lastenemyorigin );
						bs->lastenemyareanum = areanum;
						//
						if ( BotGoalForEntity( bs, flag->s.number, &target, BGU_HIGH ) ) {
							bs->enemy = flag->s.number;
							AIEnter_MP_Battle_Chase( bs );
							bs->chase_time += 20.0;
							return qtrue;
						}
					}
				}
			}
		} else    // if we are close to the flag, get it!
		{
			// is the enemy flag at base?
			if (    ( ( bs->leader == -1 ) || !trap_InPVS( bs->origin, g_entities[bs->leader].r.currentOrigin ) || ( VectorDistanceSquared( bs->origin, g_entities[bs->leader].r.currentOrigin ) > ( MAX_BOTLEADER_DIST * MAX_BOTLEADER_DIST ) ) )
					&&  ( !BotCarryingFlag( bs->client ) )
					&&  ( BotFlagAtBase( level.attackingTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS, &flag ) == qtrue ) ) {
				qboolean getflag = qtrue;
				//
				// check special circumstances in which we should not go for the flag
				if ( bs->ainode == AINode_MP_MedicRevive || bs->ainode == AINode_MP_MedicGiveHealth ) {
					// if we are close to them, continue
					if ( trap_InPVS( bs->origin, g_entities[bs->target_goal.entitynum].r.currentOrigin ) &&
						 VectorDistanceSquared( bs->origin, g_entities[bs->target_goal.entitynum].r.currentOrigin ) < ( 512 * 512 ) ) {
						getflag = qfalse;
					}
				}
				//
				if ( getflag ) {
					// GO GET IT
					// do we have a route to the enemy flag?
					if ( BotGoalForEntity( bs, flag->s.number, &target, BGU_MEDIUM ) ) {
						target.flags = GFL_NOSLOWAPPROACH;
						//
						t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
						if ( t ) {
							if ( trap_InPVS( bs->origin, target.origin ) ) {
								t = t / 2 - 300;
							}
							if ( t <= 0 ) {
								t = 1;
							}
							if ( t < 700 ) {
								if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_MEDIUM ) ) {
									if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_TouchTarget ) {
										return qfalse;
									}
									bs->target_goal = target;
									AIEnter_MP_TouchTarget( bs );
									return qtrue;
								}
							}
						}
					}
				}
			}

		}
	}

	// check for medic trying to give us health
	// NOTE: check that we havent just spawned, since this can cause congestion at the start
	if ( ( g_entities[bs->client].client->respawnTime < level.time - 12000 ) && g_entities[bs->client].awaitingHelpTime > level.time + 200 ) {
		if ( bs->enemy < 0 && bs->stand_time < trap_AAS_Time() && !BotCarryingFlag( bs->client ) ) {
			AIEnter_MP_Stand( bs );
			bs->stand_time = trap_AAS_Time() + 1.0;
			return qtrue;
		}
	}

	// if we are a medic, we should always revive nearby players
	if ( BotClass_MedicCheckRevives( bs, 300, &target, qtrue ) ) {
		if ( !BotCarryingFlag( bs->client ) || ( ( bs->enemy == -1 ) && ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, bs->target_goal.areanum, bs->tfl ) > 600 ) ) ) {
			if ( target.entitynum == bs->target_goal.entitynum && bs->ainode != AINode_MP_MedicRevive ) {
				return qfalse;
			}
			bs->target_goal = target;
			AIEnter_MP_MedicRevive( bs );
			return qtrue;
		}
	}

	// PANZER TARGETS
	if ( ( COM_BitCheck( bs->cur_ps.weapons, WP_PANZERFAUST ) ) ) {
		if ( ( BotWeaponWantScale( bs, WP_PANZERFAUST ) > 0 ) ) {
			team_t team = bs->sess.sessionTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
			// shoot towards gun emplacements after spawning in
			i = 0;
			trav = NULL;
			numList = 0;
			while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_MG42 ) ) ) {
				if ( ( trav->aiInactive && ( 1 << team ) ) ) {
					continue;
				}
				if ( trav->s.powerups != STATE_DEFAULT ) {
					continue;
				}
				if ( trav->health <= 0 ) {
					continue;
				}
				// make sure we can see it
				VectorCopy( trav->r.currentOrigin, center );
				center[2] += 12;
				if ( !trap_InPVS( bs->eye, center ) ) {
					continue;
				}
				trap_Trace( &tr, bs->eye, NULL, NULL, center, bs->client, MASK_MISSILESHOT );
				if ( tr.fraction < 1.0 && tr.entityNum != trav->s.number ) {
					continue;
				}
				//
				list[numList++] = trav->s.number;
				i++;
			}
			//
			if ( i ) {
				// pick a random mg42
				t = rand() % i;
				trav = BotGetEntity( list[t] );
				// trav is the mg42
				trav->missionLevel = level.time + 10000;
				//
				target.entitynum = trav->s.number;
				VectorCopy( trav->r.currentOrigin, center );
				center[2] += 12;
				VectorCopy( center, target.origin );
				//
				if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_HIGH ) ) {
					bs->target_goal = target;
					AIEnter_MP_PanzerTarget( bs );
					return qtrue;
				}
			}
		}
	}

	// defense engineers should look for dynamite placed nearby objectives to defuse
	{
		int list[10], numList;
		//
		if ( ( numList = BotGetTargetDynamite( list, 10, NULL ) ) ) {
			for ( i = 0; i < numList; i++ ) {
				trav = BotGetEntity( list[i] );
				if ( !trav ) {
					continue;
				}

				// Gordon: WTH is this for?
				if ( bs->sess.playerType != PC_ENGINEER ) {
					if ( BotCanSnipe( bs, qtrue ) ) {
						if ( level.explosiveTargets[bs->sess.sessionTeam == TEAM_AXIS ? 0 : 1] != BotGetTargetExplosives( bs->sess.sessionTeam, NULL, 0, qfalse )
							 || BotNumTeamMatesWithTarget( bs, list[i], NULL, 0 ) >= (int)( floor( 0.3 * numTeammates / numList ) ) ) {
							continue;
						}
					}
				}

				// if this dynamite has been laid by our team, and we are an engineer, no need to defend it
				if ( bs->sess.playerType == PC_ENGINEER && ( ( trav->s.teamNum % 4 ) == bs->sess.sessionTeam ) ) {
					continue;
				}

				//if (trav->missionLevel > level.time) continue;
				if ( BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) > (int)floor( (float)( numTeammates / numList ) ) ) {
					continue;
				}
				VectorAdd( trav->r.absmin, trav->r.absmax, center );
				VectorScale( center, 0.5, center );
				if ( ( areanum = BotPointAreaNum( trav->s.number, center ) ) ) {
					//if (areanum = BotReachableBBoxAreaNum( bs, trav->r.absmin, trav->r.absmax )) {
					if ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, areanum, bs->tfl ) ) {
						// make this our goal
						BotClearGoal( &target );
						target.entitynum = trav->s.number;
						target.areanum = areanum;
						VectorCopy( trav->r.mins, target.mins );
						VectorCopy( trav->r.maxs, target.maxs );
						VectorCopy( center, target.origin );
						//
						// if we are an engineer, and this is hostile dynamite, we should defuse it, otherwise defend it
						if ( bs->sess.playerType == PC_ENGINEER && ( ( trav->s.teamNum % 4 ) != bs->sess.sessionTeam ) ) {
							if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_HIGH ) ) {
								// if we are already heading there, continue
								if ( bs->target_goal.entitynum && bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_DisarmDynamite ) {
									return qfalse;
								}
								bs->target_goal = target;
								trav->missionLevel = level.time + 5000;
								AIEnter_MP_DisarmDynamite( bs );
								return qtrue;
							}
							// only "defend" the dynamite if they are yet to break through any of the defenses
						} else if ( ( BotNumTeamMatesWithTargetAndCloser( bs, target.entitynum, target.areanum, NULL, 0, -1 ) < (int)( ceil( 0.3 * numTeammates / numList ) ) ) ) {
							// if we are already heading there, continue
							if ( bs->target_goal.entitynum && bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_DefendTarget ) {
								return qfalse;
							}
							// just "defend" it
							bs->target_goal = target;
							BotFindSparseDefendArea( bs, &bs->target_goal, qtrue );
							AIEnter_MP_DefendTarget( bs );
							return qtrue;
						}
					}
				}
			}
		}
	}
	//
	// look for healing/giving ammo to others
	if ( BotMP_CheckClassActions( bs ) ) {
		return qtrue;
	}
	//
	bs->ignore_specialgoal_time = oldIgnoreTime;
	return qfalse;
}

int botgoalPriorities_Standard[BFG_NUMBOTGOALTYPES] = {
	0, // BFG_FOLLOW_LEADER
	-1, // BFG_CONSTRUCT
	2, // BFG_TRIGGER
	-1, // BFG_DESTRUCTION_EXPLOSIVE
	-1, // BFG_DESTRUCTION_BUILDING
	-1, // BFG_MG42_REPAIR
	-1, // BFG_MINE
	1, // BFG_ATTRACTOR
	-1, // BFG_SNIPERSPOT
	2, // BFG_MG42
	-1, // BFG_SCANFORMINES
	-1, // BFG_DESTRUCTION_SATCHEL
};

int botgoalPriorities_Engineer[BFG_NUMBOTGOALTYPES] = {
	1, // BFG_FOLLOW_LEADER
	5, // BFG_CONSTRUCT
	3, // BFG_TRIGGER
	5, // BFG_DESTRUCTION_EXPLOSIVE
	5, // BFG_DESTRUCTION_BUILDING
	4, // BFG_MG42_REPAIR
	4, // BFG_MINE
	2, // BFG_ATTRACTOR
	-1, // BFG_SNIPERSPOT
	0, // BFG_MG42
	-1, // BFG_SCANFORMINES
	-1, // BFG_DESTRUCTION_SATCHEL
};

int botgoalPriorities_CovertOps[BFG_NUMBOTGOALTYPES] = {
	1, // BFG_FOLLOW_LEADER
	-1, // BFG_CONSTRUCT
	3, // BFG_TRIGGER
	-1, // BFG_DESTRUCTION_EXPLOSIVE
	-1, // BFG_DESTRUCTION_BUILDING
	-1, // BFG_MG42_REPAIR
	-1, // BFG_MINE
	2, // BFG_ATTRACTOR
	4, // BFG_SNIPERSPOT
	0, // BFG_MG42
	4, // BFG_SCANFORMINES
	4, // BFG_DESTRUCTION_SATCHEL
};

int *botgoalPriorities_Class[NUM_PLAYER_CLASSES] = {
	botgoalPriorities_Standard, // PC_SOLDIER
	botgoalPriorities_Standard, // PC_MEDIC
	botgoalPriorities_Engineer, // PC_ENGINEER
	botgoalPriorities_Standard, // PC_FIELDOPS
	botgoalPriorities_CovertOps // PC_COVERTOPS
};

int botgoalMaxCloser[BFG_NUMBOTGOALTYPES] = {
	2, // BFG_FOLLOW_LEADER
	1, // BFG_CONSTRUCT
	2, // BFG_TRIGGER
	1, // BFG_DESTRUCTION_EXPLOSIVE
	1, // BFG_DESTRUCTION_BUILDING
	0, // BFG_MG42_REPAIR
	0, // BFG_MINE
	2, // BFG_ATTRACTOR
	0, // BFG_SNIPERSPOT
	0, // BFG_MG42
	0, // BFG_SCANFORMINES
	0, // BFG_DESTRUCTION_SATCHEL
};

typedef struct botgoalFind_s {
	gentity_t*          ent;
	botgoalFindType_t type;
	qboolean ignore;
	int priority;
	int defaultPriority;
	int numPlayers;
} botgoalFind_t;

#define MAX_FIND_BOTGOALS   32
#define BFG_CHECKMAX                if ( goalNum >= maxGoals ) { return maxGoals; }
#define BFG_CHECKDISABLED( ent,bs )   ent->aiInactive & ( 1 << bs->sess.sessionTeam )

int BotMP_FindGoal_BuildGoalList( bot_state_t* bs, botgoalFind_t* pGoals, int maxGoals ) {
	int goalNum = 0;
	int leader, numTargets;
	int k, c;
	int tlist[10];
	gentity_t *ent, *trav;

	BFG_CHECKMAX;

	leader = BotGetLeader( bs, ( bs->sess.playerType == PC_SOLDIER ) );
	if ( leader != -1 && g_entities[leader].r.svFlags & SVF_BOT ) {
		pGoals[goalNum].type =      BFG_FOLLOW_LEADER;
		pGoals[goalNum].ent =       &g_entities[leader];
		pGoals[goalNum].priority =  g_entities[leader].goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
		pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

		goalNum++;

		BFG_CHECKMAX;
	}

	// trigger_multiples
	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_TRIGGER_MULTIPLE ) ) ) {
		// is it disabled?
		if ( BFG_CHECKDISABLED( trav, bs ) ) {
			continue;
		}

		// is it active?
		if ( !trav->r.linked || ( trav->nextthink > level.time + 1000 ) ) {
			continue;   // if it's thinking, then it's not active (doesnt respond to touch)
		}

		pGoals[goalNum].type =      BFG_TRIGGER;
		pGoals[goalNum].ent =       trav;
		pGoals[goalNum].priority =  trav->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
		pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

		goalNum++;

		BFG_CHECKMAX;
	}

	if ( bs->sess.playerType == PC_ENGINEER ) {
		// Constructibles
		// find a constructible
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_OBJECTIVE_INFO ) ) ) {
			// is it disabled?
			if ( BFG_CHECKDISABLED( trav, bs ) ) {
				continue;
			}

			// find the constructible
			ent = G_ConstructionForTeam( trav, bs->sess.sessionTeam );
			if ( !ent ) {
				continue;
			}

			if ( !BotIsConstructible( bs->sess.sessionTeam, trav->s.number ) ) {
				continue;
			}

			pGoals[goalNum].type =      BFG_CONSTRUCT;
			pGoals[goalNum].ent =       trav;
			pGoals[goalNum].priority =  trav->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
			pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

			goalNum++;

			BFG_CHECKMAX;
		}

		// check for dynamite
		// look for things to blow up
		if ( BotWeaponCharged( bs, WP_DYNAMITE ) ) {
			numTargets = BotGetTargetExplosives( bs->sess.sessionTeam, tlist, 10, qfalse );
			for ( k = 0; k < numTargets; k++ ) {
				// see if this is a worthy goal
				ent = &g_entities[tlist[k]];

				if ( ent->s.eType == ET_OID_TRIGGER ) {
					pGoals[goalNum].type =  BFG_DESTRUCTION_EXPLOSIVE;
					if ( ent->target_ent ) {
						pGoals[goalNum].priority =  ent->target_ent->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
					} else {
						// Gordon: this shouldn't happen really....
						pGoals[goalNum].priority =  ent->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
					}
				} else {
					pGoals[goalNum].type =  BFG_DESTRUCTION_BUILDING;
					pGoals[goalNum].priority =  ent->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
				}
				pGoals[goalNum].ent =       ent;
				pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

				goalNum++;

				BFG_CHECKMAX;
			}
		}

		// look for a broken MG42 that isnt already being fixed
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_MG42 ) ) ) {
			// is it disabled?
			if ( BFG_CHECKDISABLED( trav, bs ) ) {
				continue;
			}

			if ( trav->health <= 0 ) {
				if ( trav->melee ) {
					ent = trav->melee;
					if ( ( ent->botIgnoreTime < level.time ) ) {
						pGoals[goalNum].type =      BFG_MG42_REPAIR;
						pGoals[goalNum].ent =       ent;
						pGoals[goalNum].priority =  trav->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
						pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

						goalNum++;

						BFG_CHECKMAX;
					}
				}
			}
		}

		// PLANT LAND MINES
		if ( G_CountTeamLandmines( bs->sess.sessionTeam ) < MAX_TEAM_LANDMINES ) {
			// look for a land mine area that isn't full
			trav = NULL;
			while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_LANDMINE_AREA ) ) ) {
				// is it disabled?
				if ( BFG_CHECKDISABLED( trav, bs ) ) {
					continue;
				}
				// is it for us?
				if ( ( trav->spawnflags & 1 ) && ( bs->sess.sessionTeam != TEAM_AXIS ) ) {
					continue;
				} else if ( ( trav->spawnflags & 2 ) && ( bs->sess.sessionTeam != TEAM_ALLIES ) ) {
					continue;
				}

				// has it got enough landmines?
				if ( trav->count2 >= trav->count ) {
					continue;
				}

				pGoals[goalNum].type =      BFG_MINE;
				pGoals[goalNum].ent =       trav;
				pGoals[goalNum].priority =  trav->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
				pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

				goalNum++;

				BFG_CHECKMAX;
			}
		}
	}

	trav = NULL;
	while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_ATTRACTOR ) ) ) {
		if ( BFG_CHECKDISABLED( trav, bs ) ) {
			continue;
		}

		pGoals[goalNum].type =      BFG_ATTRACTOR;
		pGoals[goalNum].ent =       trav;
		pGoals[goalNum].priority =  trav->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
		pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

		goalNum++;

		BFG_CHECKMAX;
	}

	// SNIPER SPOT
	if ( BotCanSnipe( bs, qtrue ) ) {
		c = BotBestSniperSpot( bs );
		if ( c != -1 ) {
			pGoals[goalNum].type =      BFG_SNIPERSPOT;
			pGoals[goalNum].ent =       &g_entities[c];
			pGoals[goalNum].priority =  g_entities[c].goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
			pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

			goalNum++;

			BFG_CHECKMAX;
		}
	}

	if ( bs->sess.playerType == PC_COVERTOPS ) {
		c = BotBestLandmineSpotingSpot( bs );
		if ( c != -1 ) {
			pGoals[goalNum].type =      BFG_SCANFORMINES;
			pGoals[goalNum].ent =       &g_entities[c];
			pGoals[goalNum].priority =  g_entities[c].goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
			pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

			goalNum++;

			BFG_CHECKMAX;
		}

		numTargets = BotGetTargetsForSatchelCharge( bs->sess.sessionTeam, tlist, 10, qfalse );
		for ( k = 0; k < numTargets; k++ ) {
			// see if this is a worthy goal
			ent = &g_entities[tlist[k]];

			pGoals[goalNum].type =              BFG_DESTRUCTION_SATCHEL;
			pGoals[goalNum].priority =          ent->goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
			pGoals[goalNum].ent =               ent;
			pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

			goalNum++;

			BFG_CHECKMAX;
		}
	}

	// MG42
	c = BotBestMG42Spot( bs, qfalse );
	if ( c != -1 ) {
		pGoals[goalNum].type =      BFG_MG42;
		pGoals[goalNum].ent =       &g_entities[c];
		pGoals[goalNum].priority =  g_entities[c].goalPriority[bs->sess.sessionTeam - TEAM_AXIS];
		pGoals[goalNum].defaultPriority =   botgoalPriorities_Class[bs->sess.playerType][pGoals[goalNum].type];

		goalNum++;

		BFG_CHECKMAX;
	}

	return goalNum;
}

int QDECL BotMP_FindGoals_Sort_Standard( const void *a, const void *b ) {
	botgoalFind_t* bg1 = (botgoalFind_t*)a;
	botgoalFind_t* bg2 = (botgoalFind_t*)b;

	if ( botgoalPriorities_Standard[bg1->type] > botgoalPriorities_Standard[bg2->type] ) {
		return -1;
	} else if ( botgoalPriorities_Standard[bg2->type] > botgoalPriorities_Standard[bg1->type] ) {
		return 1;
	}

	if ( bg1->priority > bg2->priority ) {
		return -1;
	} else if ( bg2->priority > bg1->priority ) {
		return 1;
	}

	return 0;
}

int QDECL BotMP_FindGoals_Sort_Engineer( const void *a, const void *b ) {
	botgoalFind_t* bg1 = (botgoalFind_t*)a;
	botgoalFind_t* bg2 = (botgoalFind_t*)b;

	if ( botgoalPriorities_Engineer[bg1->type] > botgoalPriorities_Engineer[bg2->type] ) {
		return -1;
	} else if ( botgoalPriorities_Engineer[bg2->type] > botgoalPriorities_Engineer[bg1->type] ) {
		return 1;
	}

	if ( bg1->priority > bg2->priority ) {
		return -1;
	} else if ( bg2->priority > bg1->priority ) {
		return 1;
	}

	return 0;
}

int QDECL BotMP_FindGoals_Sort_CovertOps( const void *a, const void *b ) {
	botgoalFind_t* bg1 = (botgoalFind_t*)a;
	botgoalFind_t* bg2 = (botgoalFind_t*)b;

	if ( botgoalPriorities_CovertOps[bg1->type] > botgoalPriorities_CovertOps[bg2->type] ) {
		return -1;
	} else if ( botgoalPriorities_CovertOps[bg2->type] > botgoalPriorities_CovertOps[bg1->type] ) {
		return 1;
	}

	if ( bg1->priority > bg2->priority ) {
		return -1;
	} else if ( bg2->priority > bg1->priority ) {
		return 1;
	}

	return 0;
}

typedef int QDECL sortFunc ( const void *a, const void *b );

sortFunc* botmp_sortFuncs[NUM_PLAYER_CLASSES] = {
	BotMP_FindGoals_Sort_Standard,  // PC_SOLDIER
	BotMP_FindGoals_Sort_Standard,  // PC_MEDIC
	BotMP_FindGoals_Sort_Engineer,  // PC_ENGINEER
	BotMP_FindGoals_Sort_Standard,  // PC_FIELDOPS
	BotMP_FindGoals_Sort_CovertOps  // PC_COVERTOPS
};


int BotMP_FindGoal_ClassForGoalType( botgoalFindType_t type ) {
	switch ( type ) {
	case BFG_CONSTRUCT:
	case BFG_DESTRUCTION_EXPLOSIVE:
	case BFG_DESTRUCTION_BUILDING:
	case BFG_MG42_REPAIR:
	case BFG_MINE:
		return PC_ENGINEER;

	case BFG_SNIPERSPOT:
	case BFG_SCANFORMINES:
	case BFG_DESTRUCTION_SATCHEL:
		return PC_COVERTOPS;
	default:
		break;
	}

	return -1;
}

#define BFG_RETURN_SUCCESS              return BPG_NEWGOAL
#define BFG_RETURN_FAILURE              return BPG_FAILED
#define BFG_RETURN_ALREADY              return BPG_ALREADYDOING
#define BFG_RETURN_FAILURE_AND_IGNORE   bg->ignore = qtrue; BFG_RETURN_FAILURE
#define BFG_GOAL_ENTITYNUM              bg->ent - g_entities

typedef enum botMPpg_e {
	BPG_FAILED,
	BPG_NEWGOAL,
	BPG_ALREADYDOING,
} botMPpg_t;

botMPpg_t BotMP_FindGoal_ProcessGoal( bot_state_t* bs, botgoalFind_t* bg, bot_goal_t* target_goal ) {
	int t, i;
	vec3_t mins, maxs;
	vec3_t center, brushPos, vec, end;
	vec_t dist;
	int list[32];
	int numList;
	trace_t tr;
	weapon_t weap;

	switch ( bg->type ) {
	case BFG_FOLLOW_LEADER:
		// we have a leader, follow them
		if ( BotGoalForEntity( bs, BFG_GOAL_ENTITYNUM, target_goal, BGU_LOW ) ) {
			target_goal->flags = GFL_LEADER;

			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target_goal->areanum, bs->tfl );
			if ( t && t <= MAX_BOTLEADER_TRAVEL ) {
				BotFindSparseDefendArea( bs, target_goal, qtrue );
				if ( bs->target_goal.entitynum == target_goal->entitynum && bs->ainode == AINode_MP_DefendTarget ) {
					BFG_RETURN_ALREADY;
				}
				BFG_RETURN_SUCCESS;
			}
		}

		bs->leader = -1;
		BFG_RETURN_FAILURE_AND_IGNORE;

	case BFG_CONSTRUCT:
		if ( !BotGetReachableEntityArea( bs, BFG_GOAL_ENTITYNUM, target_goal ) ) {
			BFG_RETURN_FAILURE_AND_IGNORE;      // not reachable
		}

		if ( bs->target_goal.entitynum == BFG_GOAL_ENTITYNUM && bs->ainode == AINode_MP_ConstructibleTarget ) {
			BFG_RETURN_ALREADY;     // already going for it
		}

		// Gordon: if this is a new goal, check pliers are half-full before heading off (optimist ;))
		if ( bs->cur_ps.classWeaponTime > ( level.time - ( level.engineerChargeTime[bs->sess.sessionTeam - 1] * 0.5f ) ) ) {
			BFG_RETURN_FAILURE_AND_IGNORE;
		}

		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target_goal->areanum, bs->tfl );
		if ( !t ) {
			BFG_RETURN_FAILURE_AND_IGNORE;     // failed for whatever reason
		}
		BFG_RETURN_SUCCESS;



	case BFG_TRIGGER:
		if ( !BotGetReachableEntityArea( bs, BFG_GOAL_ENTITYNUM, target_goal ) ) {
			BFG_RETURN_FAILURE_AND_IGNORE;      // not reachable
		}

		if ( !BotGoalWithinMovementAutonomy( bs, target_goal, BGU_LOW ) ) {
			BFG_RETURN_FAILURE_AND_IGNORE;          // outside autonomy range
		}

		// if the current destination is not touching the brush, abort
		VectorAdd( target_goal->origin, bs->cur_ps.mins, mins );
		VectorAdd( target_goal->origin, bs->cur_ps.maxs, maxs );
		if ( !trap_EntityContactCapsule( mins, maxs, bg->ent ) ) {
			BFG_RETURN_FAILURE_AND_IGNORE;
		}

		// get the travel time
		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target_goal->areanum, bs->tfl );
		if ( !t ) {
			BFG_RETURN_FAILURE_AND_IGNORE;
		}

		// is it for us only?
		if ( ( ( bg->ent->spawnflags & 1 ) && ( bs->sess.sessionTeam == TEAM_AXIS ) ) || ( ( bg->ent->spawnflags & 2 ) && ( bs->sess.sessionTeam == TEAM_ALLIES ) ) ) {
			if ( bs->target_goal.entitynum == target_goal->entitynum && bs->ainode == AINode_MP_TouchTarget ) {
				BFG_RETURN_ALREADY;
			}
			BFG_RETURN_SUCCESS;
		} else {        // defend it
			if ( bs->target_goal.entitynum == target_goal->entitynum && bs->ainode == AINode_MP_DefendTarget ) {
				BFG_RETURN_ALREADY;
			}
			BFG_RETURN_SUCCESS;
		}

	case BFG_DESTRUCTION_EXPLOSIVE:
		// calc center
		VectorAdd( bg->ent->r.absmin, bg->ent->r.absmax, brushPos );
		VectorScale( brushPos, 0.5, brushPos );

		// find the best goal area
		numList = trap_AAS_BBoxAreas( bg->ent->r.absmin, bg->ent->r.absmax, list, 32 );
		if ( numList ) {
			int oldestTime =    -1;
			int bestDist =      -1;
			int oldest =        0;

			for ( i = 0; i < numList; i++ ) {
				if ( !trap_AAS_AreaReachability( list[i] ) ) {
					continue;
				}

				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
				if ( t > 0 && t < 400 ) {
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

			if ( oldestTime > 0 ) {
				// now trace from this area towards the func_explosive
				trap_AAS_AreaCenter( oldest, center );
				VectorCopy( center, end );
				end[2] -= 512;
				trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, end, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
				VectorCopy( tr.endpos, center );
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, oldest, bs->tfl );
				if ( t ) {
					bg->ent->lastHintCheckTime = level.time + 5000;
					// use this as the goal origin
					VectorCopy( center, target_goal->origin );
					VectorCopy( bs->cur_ps.mins, target_goal->mins );
					VectorCopy( bs->cur_ps.maxs, target_goal->maxs );
					target_goal->areanum = oldest;
					target_goal->entitynum = BFG_GOAL_ENTITYNUM;

					if ( bs->target_goal.entitynum == target_goal->entitynum && bs->ainode == AINode_MP_DynamiteTarget ) {
						BFG_RETURN_ALREADY;
					}
					BFG_RETURN_SUCCESS;
				}
			}
		}
		BFG_RETURN_FAILURE_AND_IGNORE;



	case BFG_DESTRUCTION_BUILDING:
	case BFG_DESTRUCTION_SATCHEL:
		if ( bg->type == BFG_DESTRUCTION_BUILDING ) {
			weap = WP_DYNAMITE;
		} else {
			weap = WP_SATCHEL;
		}

		VectorAdd( bg->ent->r.absmin, bg->ent->r.absmax, brushPos );
		VectorScale( brushPos, 0.5, brushPos );

		VectorCopy( bg->ent->r.absmin, mins );
		VectorCopy( bg->ent->r.absmax, maxs );
		for ( i = 0; i < 2; i++ ) {
			mins[i] -= G_GetWeaponDamage( weap ) * 0.2f;
			maxs[i] += G_GetWeaponDamage( weap ) * 0.2f;
		}

		// find the best goal area
		numList = trap_AAS_BBoxAreas( mins, maxs, list, 32 );
		if ( numList ) {
			int oldestTime = -1;
			int bestDist = -1;
			int oldest = 0;

			for ( i = 0; i < numList; i++ ) {
				if ( !trap_AAS_AreaReachability( list[i] ) ) {
					continue;
				}

				// RF, make sure this is within range
				trap_AAS_AreaCenter( list[i], center );
				VectorCopy( center, end );
				end[2] -= 512;
				trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, end, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
				VectorCopy( tr.endpos, center );
				VectorSubtract( center, brushPos, vec );

				G_AdjustedDamageVec( bg->ent, center, vec );
				if ( ( VectorLengthSquared( vec ) <= SQR( G_GetWeaponDamage( weap ) ) ) && CanDamage( bg->ent, center ) ) {
					t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
					if ( t ) {
						oldestTime = t;
						oldest = list[i];
						bestDist = t;
					}
				}
			}

			if ( oldestTime != -1 ) {
				// now trace from this area towards the func_explosive
				trap_AAS_AreaCenter( oldest, center );
				VectorCopy( center, end );
				end[2] -= 512;
				trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, end, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
				VectorCopy( tr.endpos, center );

				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, oldest, bs->tfl );
				if ( t ) {
					bg->ent->lastHintCheckTime = level.time + 5000;
					// use this as the goal origin
					VectorCopy( center, target_goal->origin );
					VectorCopy( bs->cur_ps.mins, target_goal->mins );
					VectorCopy( bs->cur_ps.maxs, target_goal->maxs );
					target_goal->areanum = oldest;
					target_goal->entitynum = BFG_GOAL_ENTITYNUM;

					if ( BotGoalWithinMovementAutonomy( bs, target_goal, BGU_LOW ) ) {
						if ( bs->target_goal.entitynum == target_goal->entitynum ) {
							if ( bs->ainode == AINode_MP_DynamiteTarget ) {
								BFG_RETURN_ALREADY;
							}
							// Gordon: FIXME: insert satchel node here
/*								if( bs->ainode == AINode_MP_DynamiteTarget ) {
									BFG_RETURN_ALREADY;
								}*/
						}
						BFG_RETURN_SUCCESS;
					}
					BFG_RETURN_SUCCESS;
				}
			}
		}
		BFG_RETURN_FAILURE_AND_IGNORE;


	case BFG_MG42_REPAIR:
		if ( BotGoalForEntity( bs, BFG_GOAL_ENTITYNUM, target_goal, BGU_LOW ) ) {
			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target_goal->areanum, bs->tfl );
			if ( t < 500 ) {
				if ( bs->target_goal.entitynum == target_goal->entitynum && bs->ainode == AINode_MP_FixMG42 ) {
					BFG_RETURN_ALREADY;
				}

				// go fix it!!
				BFG_RETURN_SUCCESS;
			}
		}
		BFG_RETURN_FAILURE_AND_IGNORE;


	case BFG_MINE:
		if ( !BotGoalForEntity( bs, BFG_GOAL_ENTITYNUM, target_goal, BGU_LOW ) ) {
			BFG_RETURN_FAILURE_AND_IGNORE;
		}

		if ( bs->target_goal.entitynum == BFG_GOAL_ENTITYNUM && bs->ainode == AINode_MP_PlantMine ) {
			BFG_RETURN_ALREADY;
		}

		//
		t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target_goal->areanum, bs->tfl );
		if ( !t ) {
			BFG_RETURN_FAILURE_AND_IGNORE;
		}

		// find a spot inside the area to plant the mine
		i = 0;
		do {
			VectorAdd( BotGetOrigin( BFG_GOAL_ENTITYNUM ), bg->ent->pos3, vec );
			vec[0] += crandom() * bg->ent->r.absmax[0] * 0.25;
			vec[1] += crandom() * bg->ent->r.absmax[1] * 0.25;

			target_goal->areanum = BotPointAreaNum( bs->client, vec );
			VectorCopy( vec, target_goal->origin );
		} while ( ( ++i < 5 ) && ( ( target_goal->areanum < 0 ) || !PointInBounds( vec, bg->ent->r.absmin, bg->ent->r.absmax ) ) );

		if ( i < 5 ) {      // we found a valid spot
			BFG_RETURN_SUCCESS;
		}
		BFG_RETURN_FAILURE_AND_IGNORE;

	case BFG_ATTRACTOR:
		if ( BotGoalForEntity( bs, BFG_GOAL_ENTITYNUM, target_goal, BGU_MAXIMUM ) ) {
			// get the distance, halve it if there is noone one going for this attractor
			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target_goal->areanum, bs->tfl );
			if ( t ) {
				BotFindSparseDefendArea( bs, target_goal, qtrue );
				if ( bs->target_goal.entitynum == target_goal->entitynum && bs->ainode == AINode_MP_DefendTarget ) {
					BFG_RETURN_ALREADY;
				}
				BFG_RETURN_SUCCESS;
			}
		}
		BFG_RETURN_FAILURE_AND_IGNORE;

	case BFG_SNIPERSPOT:
		if ( bs->target_goal.entitynum == BFG_GOAL_ENTITYNUM && bs->ainode == AINode_MP_SniperSpot ) {
			BFG_RETURN_ALREADY;
		}

		target_goal->entitynum = BFG_GOAL_ENTITYNUM;
		VectorCopy( bg->ent->s.origin, target_goal->origin );
		target_goal->areanum = BotPointAreaNum( -1, target_goal->origin );
		VectorCopy( bs->cur_ps.mins, target_goal->mins );
		VectorCopy( bs->cur_ps.maxs, target_goal->maxs );

		BFG_RETURN_SUCCESS;

	case BFG_SCANFORMINES:
		if ( bs->target_goal.entitynum == BFG_GOAL_ENTITYNUM && bs->ainode == AINode_MP_ScanForLandmines ) {
			BFG_RETURN_ALREADY;
		}

		target_goal->entitynum = BFG_GOAL_ENTITYNUM;
		VectorCopy( bg->ent->s.origin, target_goal->origin );
		target_goal->areanum = BotPointAreaNum( -1, target_goal->origin );
		VectorCopy( bs->cur_ps.mins, target_goal->mins );
		VectorCopy( bs->cur_ps.maxs, target_goal->maxs );

		BFG_RETURN_SUCCESS;

	case BFG_MG42:
		if ( ( bs->target_goal.entitynum == BFG_GOAL_ENTITYNUM ) && ( bs->ainode == AINode_MP_MG42Mount || bs->ainode == AINode_MP_MG42Scan ) ) {
			BFG_RETURN_ALREADY;
		}

		target_goal->entitynum = BFG_GOAL_ENTITYNUM;
		VectorCopy( bg->ent->s.origin, target_goal->origin );
		target_goal->areanum = BotPointAreaNum( -1, target_goal->origin );
		VectorCopy( bs->cur_ps.mins, target_goal->mins );
		VectorCopy( bs->cur_ps.maxs, target_goal->maxs );

		BFG_RETURN_SUCCESS;
	default:
		break;
	}

	BFG_RETURN_FAILURE;
}

void BotMP_FindGoal_PostProcessGoal( bot_state_t* bs, botgoalFind_t* bg, bot_goal_t* goal ) {
	bs->target_goal = *goal;

	switch ( bg->type ) {
	case BFG_FOLLOW_LEADER:
		bs->leader = goal->entitynum;
		AIEnter_MP_DefendTarget( bs );
		break;
	case BFG_CONSTRUCT:
		AIEnter_MP_ConstructibleTarget( bs );
		break;
	case BFG_TRIGGER:
		if ( ( ( bg->ent->spawnflags & 1 ) && ( bs->sess.sessionTeam == TEAM_AXIS ) ) || ( ( bg->ent->spawnflags & 2 ) && ( bs->sess.sessionTeam == TEAM_ALLIES ) ) ) {
			AIEnter_MP_TouchTarget( bs );
		} else {
			AIEnter_MP_DefendTarget( bs );
		}
		break;
	case BFG_DESTRUCTION_EXPLOSIVE:
	case BFG_DESTRUCTION_BUILDING:
		AIEnter_MP_DynamiteTarget( bs );
		break;
	case BFG_DESTRUCTION_SATCHEL:
		AIEnter_MP_SatchelChargeTarget( bs );
		break;
	case BFG_MG42_REPAIR:
		AIEnter_MP_FixMG42( bs );
		break;
	case BFG_MINE:
		AIEnter_MP_PlantMine( bs );
		break;
	case BFG_ATTRACTOR:
		AIEnter_MP_DefendTarget( bs );
		break;
	case BFG_SNIPERSPOT:
		AIEnter_MP_SniperSpot( bs );
		break;
	case BFG_SCANFORMINES:
		AIEnter_MP_ScanForLandmines( bs );
		break;
	case BFG_MG42:
		AIEnter_MP_MG42Mount( bs );
		break;
	default:
		break;
	}
}

qboolean BotMP_AlreadyDoing_FastOut( bot_state_t *bs, botgoalFind_t *bg ) {
	// return qtrue if the given goal, which we are already doing, is ok to continue doing, without looking for other goals

	switch ( bg->type ) {
	case BFG_SNIPERSPOT:
	case BFG_MG42:
	case BFG_SCANFORMINES:
		return qtrue;

	case BFG_FOLLOW_LEADER:
	case BFG_CONSTRUCT:
	case BFG_TRIGGER:
	case BFG_DESTRUCTION_EXPLOSIVE:
	case BFG_DESTRUCTION_BUILDING:
	case BFG_DESTRUCTION_SATCHEL:
	case BFG_MG42_REPAIR:
	case BFG_MINE:
	case BFG_ATTRACTOR:
	default:
		return qfalse;
	}
}

qboolean BotMP_FindGoal_New( bot_state_t* bs ) {
	int numGoals, i, numCloser;
	botgoalFind_t findGoals[MAX_FIND_BOTGOALS];
	botMPpg_t res, bestGoal_result = 0;
	bot_goal_t goal, bestGoal;
	int bestGoal_numCloser, bestGoal_InList;

#ifdef _DEBUG
//	int t = trap_Milliseconds();
#endif // _DEBUG

	if ( bs->last_findspecialgoals > level.time - 1600 ) {
		return qfalse;
	}
	bs->last_findspecialgoals = level.time + rand() % 400;

	if ( bs->ignore_specialgoal_time > level.time ) {
		return qfalse;
	}

	numGoals = BotMP_FindGoal_BuildGoalList( bs, findGoals, MAX_FIND_BOTGOALS );
	if ( !numGoals ) {
		// Gordon: didnt find any goals :(
#ifdef _DEBUG
/*		if(bot_profile.integer == 1) {
			t = trap_Milliseconds() - t;
			G_Printf( "Findgoal: %s: %i\n", BG_ClassnameForNumber( bs->sess.playerType ), t );
		}*/
#endif // _DEBUG
		return qfalse;
	}

	for ( i = 0; i < numGoals; i++ ) {
		if ( findGoals[i].defaultPriority == -1 ) {
			findGoals[i].ignore = qtrue;
		} else {
			findGoals[i].ignore = qfalse;
		}
	}

	qsort( findGoals, numGoals, sizeof( botgoalFind_t ), botmp_sortFuncs[bs->cur_ps.stats[STAT_PLAYER_CLASS]] );

	bestGoal_numCloser =    -1;
	bestGoal_InList =       -1;

	for ( i = 0; i < numGoals; i++ ) {
		if ( findGoals[i].ignore ) {
			continue;
		}

#ifdef _DEBUG
/*		if(bot_profile.integer == 1) {
			G_Printf( "Processing goaltype %i\n", findGoals[i].type );
		}*/
#endif // _DEBUG
		res = BotMP_FindGoal_ProcessGoal( bs, &findGoals[i], &goal );
		switch ( res ) {
		case BPG_FAILED:
			continue;
		case BPG_ALREADYDOING:      // we should check for range also if we are already doing it, otherwise they may all get stuck at the same goal
			if ( BotMP_AlreadyDoing_FastOut( bs, &findGoals[i] ) ) {
#ifdef _DEBUG
/*					if(bot_profile.integer == 1) {
						t = trap_Milliseconds() - t;
						G_Printf( "Findgoal: %s: %i\n", BG_ClassnameForNumber( bs->sess.playerType ), t );
					}*/
#endif // _DEBUG
				return qfalse;
			}
			// else, drop down so we do a numCloser check
		case BPG_NEWGOAL:
			// Possible optimization here: record the travel time within each bot's AI node, so we can use that time inside BotNumTeamMatesWithTargetAndCloser() rather than calculating the time for all bots
			findGoals[i].numPlayers = numCloser = BotNumTeamMatesWithTargetAndCloser( bs, goal.entitynum, goal.areanum, NULL, 0, BotMP_FindGoal_ClassForGoalType( findGoals[i].type ) );
			if ( bestGoal_numCloser >= 0 ) {
				// First, check against maxCloser lookup
				if ( numCloser > botgoalMaxCloser[findGoals[i].type] ) {
					continue;
				}

				// TODO: scale it down according to priority (?)
				if ( numCloser > bestGoal_numCloser ) {
					continue;
				}

				// if this has the same numCloser, but a lower priority, then skip it
				if ( numCloser == bestGoal_numCloser ) {
					// if the default priority is lower, ignore
					if ( findGoals[i].defaultPriority < findGoals[bestGoal_InList].defaultPriority ) {
						continue;
					}

					// if the default priority is the same, check script
					if ( ( findGoals[i].defaultPriority == findGoals[bestGoal_InList].defaultPriority ) &&
						 ( findGoals[i].priority <= findGoals[bestGoal_InList].priority ) ) {
						continue;
					}
				}
			}
			bestGoal = goal;
			bestGoal_result = res;
			bestGoal_numCloser = numCloser;
			bestGoal_InList = i;
			break;
		}
	}

#ifdef _DEBUG
/*			if(bot_profile.integer == 1) {
				t = trap_Milliseconds() - t;
				G_Printf( "Findgoal: %s: %i\n", BG_ClassnameForNumber( bs->sess.playerType ), t );
			}*/
#endif // _DEBUG

	if ( bestGoal_numCloser >= 0 ) {
		switch ( bestGoal_result ) {
		case BPG_NEWGOAL:
			BotMP_FindGoal_PostProcessGoal( bs, &findGoals[bestGoal_InList], &bestGoal );
			return qtrue;
		case BPG_ALREADYDOING:
			return qfalse;
		default:
			break;
		}
	}

	return qfalse;
}



/*
===================
BotMP_FindGoal
===================
*/
qboolean BotMP_FindGoal( bot_state_t *bs ) {
	int i, k, t  = 0, c;
	int oldest = -1, oldestTime = -1, closestTime = -1;
	int tlist[10], numTargets;
	int teamlist[64], numTeammates;
	float dist, bestDist = -1, targetTime = 0, f = 0.0;
	gentity_t *ent, *trav;
	trace_t tr;
	vec3_t center, end, brushPos, vec;
	qboolean gotTarget, defendTarget = qfalse;
	bot_goal_t bestTarget, secondary;
	aas_altroutegoal_t altroutegoals[40];
	vec3_t mins, maxs;
	int list[MAX_CLIENTS], numList;
	float ourDist, *distances;
	int targetPriority = 0, bestPriority = 0, bestGoalPriority = 0;

	if ( bs->last_findspecialgoals > level.time - 1600 ) {
		return qfalse;
	}
	bs->last_findspecialgoals = level.time + rand() % 400;

	if ( bs->ignore_specialgoal_time > level.time ) {
		return qfalse;
	}

	BotClearGoal( &target );
	BotClearGoal( &secondary );
	numTeammates = BotNumTeamMates( bs, teamlist, 64 );
	gotTarget = qfalse;

	// LEADER

	//
	// look for a (better?) leader
	bs->leader = BotGetLeader( bs, ( bs->sess.playerType == PC_SOLDIER ) );
	if ( bs->leader > -1 ) {
		// we have a leader, follow them
		if ( BotGoalForEntity( bs, bs->leader, &secondary, BGU_LOW ) || !( g_entities[bs->leader].r.svFlags & SVF_BOT ) ) {
			secondary.flags = GFL_LEADER;

			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, secondary.areanum, bs->tfl );

			if ( !t || t > MAX_BOTLEADER_TRAVEL ) {
				bs->leader = -1;
			} else {
				targetTime = t;
				gotTarget = qtrue;
				defendTarget = qtrue;
				bestTarget = secondary;
			}
		} else {
			bs->leader = -1;
		}
	}

	// IMPORTANT CONSTRUCTIBLES
	if ( bs->sess.playerType == PC_ENGINEER ) {
		int pass;
		//
		for ( pass = 0; pass < 2; pass++ ) {
			// find a constructible
			trav = NULL;
			ent = NULL;
			closestTime = 0;
			while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_OBJECTIVE_INFO ) ) ) {
				// find the constructible
				// is it disabled?
				if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
					continue;
				}

				// is it not of highest priority?
				if ( trav->goalPriority[bs->sess.sessionTeam - 1] < 9 ) {
					continue;
				}

				ent = trav->target_ent;
				if ( !ent ) {
					continue;
				}

				if ( bs->sess.sessionTeam == TEAM_ALLIES ) {
					if ( ent->spawnflags & AXIS_CONSTRUCTIBLE ) {
						if ( ent->chain ) {
							ent = ent->chain;
						} else {
							continue;
						}
					}
				} else if ( bs->sess.sessionTeam == TEAM_AXIS ) {
					if ( ent->spawnflags & ALLIED_CONSTRUCTIBLE ) {
						if ( ent->chain ) {
							ent = ent->chain;
						} else {
							continue;
						}
					}
				}

				if ( ent->s.eType != ET_CONSTRUCTIBLE ) {
					continue;
				}

				//
				if ( !BotIsConstructible( bs->sess.sessionTeam, trav->s.number ) ) {
					continue;
				}
				if ( ( pass == 0 ) && BotNumTeamMatesWithTargetByClass( bs, trav->s.number, NULL, 0, PC_ENGINEER ) ) {
					continue;                                                                                                   // someone else is going for it
				}
				if ( !BotGetReachableEntityArea( bs, trav->s.number, &target ) ) {
					continue;                                                               // not reachable
				}
				//
				// we can build it
				//
				// if we are already heading for this goal, stay with it, otherwise look only for closest targets
				//
				if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_ConstructibleTarget ) {
					return qfalse;
				}
				//
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
				if ( closestTime && t > closestTime ) {
					continue;
				}
				//
				// if our weapon wont be ready in time
				//if (bs->cur_ps.classWeaponTime + t*10 < (level.time - level.lieutenantChargeTime[bs->sess.sessionTeam-1])) continue;
				//
				bestTarget = target;
				closestTime = t;
			}
			//
			if ( closestTime ) {
				bs->target_goal = bestTarget;
				AIEnter_MP_ConstructibleTarget( bs );
				return qtrue;
			}
		}
	}

	// IMPORTANT TRIGGER MULTIPLE

	{
		// find a trigger_multiple
		trav = NULL;
		ent = NULL;
		closestTime = 0;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_TRIGGER_MULTIPLE ) ) ) {
			// is it disabled?
			if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
				continue;
			}
			// is it active?
			if ( !trav->r.linked || ( trav->nextthink > level.time + 1000 ) ) {
				continue;                                                           // if it's thinking, then it's not active (doesnt respond to touch)
			}
			// is it not of highest priority?
			if ( trav->goalPriority[bs->sess.sessionTeam - 1] < 9 ) {
				continue;
			}
			//
			if ( !BotGetReachableEntityArea( bs, trav->s.number, &target ) ) {
				continue;                                                               // not reachable
			}
			if ( !BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
				continue;                                                               // outside autonomy range
			}
			// if the current destination is not touching the brush, abort
			VectorAdd( target.origin, bs->cur_ps.mins, mins );
			VectorAdd( target.origin, bs->cur_ps.maxs, maxs );
			if ( !trap_EntityContactCapsule( mins, maxs, trav ) ) {
				continue;
			}
			//
			// get the travel time
			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
			if ( !t ) {
				continue;
			}
			//
			targetPriority = 2;
			//
			// if there are too many going for this goal, then ignore it if we are the furthest away
			if ( ( numList = BotNumTeamMatesWithTargetAndCloser( bs, trav->s.number, target.areanum, list, MAX_CLIENTS, -1 ) ) > (int)floor( 0.3 * numTeammates ) ) {
				distances = BotSortPlayersByTraveltime( BotGetArea( trav->s.number ), list, numList );
				if ( distances[numList - 1] < (float)t ) {
					// we are the furthest
					targetPriority = 1;
				}
			}
			// only go for it if our current best goal is of lesser priority
			if ( targetPriority < bestPriority ) {
				continue;
			}
			//
			// we should touch it
			//
			if ( ( targetPriority <= bestPriority ) && ( closestTime && t > closestTime ) ) {
				continue;
			}
			//
			bestTarget = target;
			closestTime = t;
			bestPriority = targetPriority;
		}
		//
		if ( closestTime ) {
			trav = &g_entities[bestTarget.entitynum];
			// is it for us only?
			if ( ( ( trav->spawnflags & 1 ) && ( bs->sess.sessionTeam == TEAM_AXIS ) ) ||
				 ( ( trav->spawnflags & 2 ) && ( bs->sess.sessionTeam == TEAM_ALLIES ) ) ) {
				if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_TouchTarget ) {
					return qfalse;
				}
				bs->target_goal = bestTarget;
				AIEnter_MP_TouchTarget( bs );
				return qtrue;
			} else {    // defend it
				if ( targetPriority > 1 ) {
					if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
						return qfalse;
					}
					bs->target_goal = bestTarget;
					AIEnter_MP_DefendTarget( bs );
					return qtrue;
				} else {
					targetTime = closestTime;
					gotTarget = -1;
					defendTarget = qtrue;
				}
			}
		}
	}

	// CONSTRUCTIBLES

	if ( bs->sess.playerType == PC_ENGINEER ) {
		int pass;
		//
		for ( pass = 0; pass < 2; pass++ ) {
			// find a constructible
			trav = NULL;
			ent = NULL;
			closestTime = 0;
			while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_OBJECTIVE_INFO ) ) ) {
				// find the constructible
				// is it disabled?
				if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
					continue;
				}

				ent = trav->target_ent;
				if ( !ent ) {
					continue;
				}

				if ( bs->sess.sessionTeam == TEAM_ALLIES ) {
					if ( ent->spawnflags & AXIS_CONSTRUCTIBLE ) {
						if ( ent->chain ) {
							ent = ent->chain;
						} else {
							continue;
						}
					}
				} else if ( bs->sess.sessionTeam == TEAM_AXIS ) {
					if ( ent->spawnflags & ALLIED_CONSTRUCTIBLE ) {
						if ( ent->chain ) {
							ent = ent->chain;
						} else {
							continue;
						}
					}
				}

				if ( ent->s.eType != ET_CONSTRUCTIBLE ) {
					continue;
				}

				//
				if ( !BotIsConstructible( bs->sess.sessionTeam, trav->s.number ) ) {
					continue;
				}
				if ( BotNumTeamMatesWithTargetByClass( bs, trav->s.number, NULL, 0, PC_ENGINEER ) > pass ) {
					continue;                                                                                           // someone else is going for it
				}
				if ( !BotGetReachableEntityArea( bs, trav->s.number, &target ) ) {
					continue;                                                               // not reachable
				}
				if ( bestGoalPriority > trav->goalPriority[bs->sess.sessionTeam - 1] ) {
					continue;
				}
				//
				// we can build it
				//
				// if we are already heading for this goal, stay with it, otherwise look only for closest targets
				//
				if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_ConstructibleTarget ) {
					return qfalse;
				}
				//
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
				if ( bestGoalPriority == trav->goalPriority[bs->sess.sessionTeam - 1] ) {
					// only check the distance if the priorities are the same
					if ( closestTime && t > closestTime ) {
						continue;
					}
				}
				//
				// if our weapon wont be ready in time
				//if (bs->cur_ps.classWeaponTime + t*10 < (level.time - level.lieutenantChargeTime[bs->sess.sessionTeam-1])) continue;
				//
				bestTarget = target;
				closestTime = t;
				bestGoalPriority = trav->goalPriority[bs->sess.sessionTeam - 1];
			}
			//
			if ( closestTime ) {
				bs->target_goal = bestTarget;
				AIEnter_MP_ConstructibleTarget( bs );
				return qtrue;
			}
		}
	}

	// check for dynamite
	if ( bs->sess.playerType == PC_ENGINEER ) {
		// look for things to blow up
		numTargets = BotGetTargetExplosives( bs->sess.sessionTeam, tlist, 10, qfalse );
		if ( numTargets ) {
			closestTime = -1;
			// pick the closest target
			for ( k = 0; k < numTargets; k++ ) {
				//
				oldest = tlist[k];
				//
				// see if this is a worthy goal
				ent = BotGetEntity( oldest );
				// is it disabled?
				// Gordon: forest script was setup to use the entity itself, NOT the toi (toi for building, entity itself for destruction)
//				if (ent->aiInactive & (1<<bs->sess.sessionTeam)) continue;
//				if (ent->parent && (ent->parent->aiInactive & (1<<bs->sess.sessionTeam))) continue;
				// is the target disabled?
//				if (ent->target_ent && (ent->target_ent->aiInactive & (1<<bs->sess.sessionTeam))) continue;
				//
				if ( ( numList = BotNumTeamMatesWithTarget( bs, oldest, list, 10 ) ) ) {
					continue;
				}
				//
				if ( ent->s.eType == ET_OID_TRIGGER ) {
					VectorAdd( ent->r.absmin, ent->r.absmax, brushPos );
					VectorScale( brushPos, 0.5, brushPos );
					// find the best goal area
					numList = trap_AAS_BBoxAreas( ent->r.absmin, ent->r.absmax, list, 32 );
					if ( numList ) {
						oldestTime = -1;
						bestDist = -1;
						for ( i = 0; i < numList; i++ ) {
							if ( !trap_AAS_AreaReachability( list[i] ) ) {
								continue;
							}
							t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
							if ( t > 0 && ( !gotTarget || t < 400 ) ) {
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
						if ( oldestTime > 0 ) {
							// now trace from this area towards the func_explosive
							trap_AAS_AreaCenter( oldest, center );
							//trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, brushPos, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
							//VectorCopy( tr.endpos, center );
							VectorCopy( center, end );
							end[2] -= 512;
							trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, end, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
							VectorCopy( tr.endpos, center );
							//center[2] += 24;
							//oldest = trap_AAS_PointAreaNum(center);
							t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, oldest, bs->tfl );
							if ( t && ( closestTime < 0 || closestTime > t ) ) {
								ent->lastHintCheckTime = level.time + 5000;
								// use this as the goal origin
								VectorCopy( center, target.origin );
								VectorCopy( bs->cur_ps.mins, target.mins );
								VectorCopy( bs->cur_ps.maxs, target.maxs );
								target.areanum = oldest;
								target.entitynum = ent->s.number;
								//
								closestTime = t;
							}
						}
					}
				} else {

					VectorAdd( ent->r.absmin, ent->r.absmax, brushPos );
					VectorScale( brushPos, 0.5, brushPos );
					VectorCopy( ent->r.absmin, mins );
					VectorCopy( ent->r.absmax, maxs );
					for ( i = 0; i < 2; i++ ) {
						mins[i] -= G_GetWeaponDamage( WP_DYNAMITE ) * 0.25f;
						maxs[i] += G_GetWeaponDamage( WP_DYNAMITE ) * 0.25f;
					}
					// find the best goal area
					numList = trap_AAS_BBoxAreas( mins, maxs, list, 32 );
					if ( numList ) {
						oldestTime = -1;
						bestDist = -1;
						for ( i = 0; i < numList; i++ ) {
							if ( !trap_AAS_AreaReachability( list[i] ) ) {
								continue;
							}

							// RF, make sure this is within range
							trap_AAS_AreaCenter( list[i], center );
							VectorCopy( center, end );
							end[2] -= 512;
							trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, end, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
							VectorCopy( tr.endpos, center );
							VectorSubtract( center, brushPos, vec );
							G_AdjustedDamageVec( ent, center, vec );
							if ( ( VectorLengthSquared( vec ) <= SQR( G_GetWeaponDamage( WP_DYNAMITE ) ) ) && CanDamage( ent, center ) ) {
								t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
								if ( t > 0 && ( !gotTarget || t < 400 ) ) {
									if ( bestDist < 0 || t < bestDist ) {
										oldestTime = t;
										oldest = list[i];
										bestDist = t;
									}
								}
							}
						}
						if ( oldestTime > 0 ) {
							// now trace from this area towards the func_explosive
							trap_AAS_AreaCenter( oldest, center );
							//trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, brushPos, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
							//VectorCopy( tr.endpos, center );
							VectorCopy( center, end );
							end[2] -= 512;
							trap_Trace( &tr, center, bs->cur_ps.mins, bs->cur_ps.maxs, end, -1, MASK_PLAYERSOLID & ~CONTENTS_BODY );
							VectorCopy( tr.endpos, center );
							//center[2] += 24;
							//oldest = trap_AAS_PointAreaNum(center);
							t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, oldest, bs->tfl );
							if ( t && ( closestTime < 0 || closestTime > t ) ) {
								ent->lastHintCheckTime = level.time + 5000;
								// use this as the goal origin
								VectorCopy( center, target.origin );
								VectorCopy( bs->cur_ps.mins, target.mins );
								VectorCopy( bs->cur_ps.maxs, target.maxs );
								target.areanum = oldest;
								target.entitynum = ent->s.number;
								//
								closestTime = t;
							}
						}
					}
				}
			}

			// if we have a closestTime, we have a target!
			if ( closestTime > 0 ) {
				if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
					if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_DynamiteTarget ) {
						return qfalse;
					}
					bs->target_goal = target;
					AIEnter_MP_DynamiteTarget( bs );
					return qtrue;
				}
			}
		}
	}

	// BROKEN MG42
	if ( bs->sess.playerType == PC_ENGINEER ) {
		// look for a broken MG42 that isnt already being fixed
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_MG42 ) ) ) {
			// is it disabled?
			if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
				continue;
			}
			if ( trav->health <= 0 ) {
				if ( trav->melee ) {
					ent = trav->melee;
					if ( ( ent->aiTeam == bs->sess.sessionTeam ) && ( ent->botIgnoreTime < level.time ) && ( BotNumTeamMatesWithTargetByClass( bs, trav->s.number, NULL, -1, PC_ENGINEER ) == 0 ) ) {
						// go fix it?
						if ( BotGoalForEntity( bs, ent->s.number, &target, BGU_LOW ) ) {
							//
							t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
							//
							if ( t < 500 && ( !gotTarget || t < 100 ) ) {
								if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_FixMG42 ) {
									return qfalse;
								}
								// go fix it!!
								bs->target_goal = target;
								AIEnter_MP_FixMG42( bs );
								return qtrue;
							}
						}
					}
				}
			}
		}
	}

	// PLANT LAND MINES
	if ( bs->sess.playerType == PC_ENGINEER && ( G_CountTeamLandmines( bs->sess.sessionTeam ) < MAX_TEAM_LANDMINES ) ) {
		// look for a land mine area that isn't full
		closestTime = 0;
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_LANDMINE_AREA ) ) ) {
			// is it disabled?
			//if (trav->aiInactive & (1<<bs->sess.sessionTeam)) continue;	// FIXME: djbob needs to add the etype of the landmine hint to the filter
			// is it for us?
			if ( ( trav->spawnflags & 1 ) && ( bs->sess.sessionTeam != TEAM_AXIS ) ) {
				continue;
			}
			if ( ( trav->spawnflags & 2 ) && ( bs->sess.sessionTeam != TEAM_ALLIES ) ) {
				continue;
			}
			// has it got enough landmines?
			if ( trav->count2 >= trav->count ) {
				continue;
			}
			// already someone else heading for it?
			if ( BotNumTeamMatesWithTargetByClass( bs, trav->s.number, NULL, 0, PC_ENGINEER ) ) {
				continue;                                                                                   // someone else is going for it
			}
			// valid goal towards it?
			if ( !BotGoalForEntity( bs, trav->s.number, &target, BGU_LOW ) ) {
				continue;
			}
			//
			// if we are already heading for this goal, stay with it, otherwise look only for closest targets
			//
			if ( bs->target_goal.entitynum == trav->s.number && bs->ainode == AINode_MP_PlantMine ) {
				return qfalse;
			}
			//
			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
			if ( !t ) {
				continue;
			}
			if ( closestTime && t > closestTime ) {
				continue;
			}
			//
			bestTarget = target;
			closestTime = t;
		}
		//
		if ( closestTime ) {
			// find a spot inside the area to plant the mine
			trav = &g_entities[bestTarget.entitynum];
			i = 0;
			do {
				VectorAdd( BotGetOrigin( trav->s.number ), trav->pos3, vec );
				vec[0] += crandom() * trav->r.absmax[0] * 0.25;
				vec[1] += crandom() * trav->r.absmax[1] * 0.25;
				//
				bestTarget.areanum = BotPointAreaNum( bs->client, vec );
				VectorCopy( vec, bestTarget.origin );
			} while ( ( ++i < 5 ) && ( ( bestTarget.areanum < 0 ) || !PointInBounds( vec, trav->r.absmin, trav->r.absmax ) ) );
			//
			if ( i < 5 ) {    // we found a valid spot
				bs->target_goal = bestTarget;
				AIEnter_MP_PlantMine( bs );
				return qtrue;
			}
		}
	}

	// TRIGGER MULTIPLE
	{
		// find a trigger_multiple
		trav = NULL;
		ent = NULL;
		closestTime = 0;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_TRIGGER_MULTIPLE ) ) ) {
			// is it disabled?
			if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
				continue;
			}
			// is it active?
			if ( !trav->r.linked || ( trav->nextthink > level.time + 1000 ) ) {
				continue;                                                           // if it's thinking, then it's not active (doesnt respond to touch)
			}
			//
			if ( !BotGetReachableEntityArea( bs, trav->s.number, &target ) ) {
				continue;                                                               // not reachable
			}
			if ( !BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
				continue;                                                               // outside autonomy range
			}
			// if the current destination is not touching the brush, abort
			VectorAdd( target.origin, bs->cur_ps.mins, mins );
			VectorAdd( target.origin, bs->cur_ps.maxs, maxs );
			if ( !trap_EntityContactCapsule( mins, maxs, trav ) ) {
				continue;
			}
			//
			// get the travel time
			t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
			if ( !t ) {
				continue;
			}
			//
			targetPriority = 2;
			//
			// if there are too many going for this goal, then ignore it if we are the furthest away
			if ( ( numList = BotNumTeamMatesWithTargetAndCloser( bs, trav->s.number, target.areanum, list, MAX_CLIENTS, -1 ) ) > (int)floor( 0.3 * numTeammates ) ) {
				distances = BotSortPlayersByTraveltime( BotGetArea( trav->s.number ), list, numList );
				if ( distances[numList - 1] < (float)t ) {
					// we are the furthest
					targetPriority = 1;
					// also consider other goals
					t += 5000;
				}
			}
			// only go for it if our current best goal is of lesser priority
			if ( targetPriority < bestPriority ) {
				continue;
			}
			//
			// we should touch it
			//
			if ( ( targetPriority <= bestPriority ) && ( closestTime && t > closestTime ) ) {
				continue;
			}
			//
			bestTarget = target;
			closestTime = t;
			bestPriority = targetPriority;
		}
		//
		if ( closestTime ) {
			trav = &g_entities[bestTarget.entitynum];
			// is it for us only?
			if ( ( ( trav->spawnflags & 1 ) && ( bs->sess.sessionTeam == TEAM_AXIS ) ) ||
				 ( ( trav->spawnflags & 2 ) && ( bs->sess.sessionTeam == TEAM_ALLIES ) ) ) {
				if ( targetPriority > 1 ) {
					if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_TouchTarget ) {
						return qfalse;
					}
					bs->target_goal = bestTarget;
					AIEnter_MP_TouchTarget( bs );
					return qtrue;
				} else {    // enough people going for it, so only go for it if there's nothing else to do
					targetTime = closestTime;
					gotTarget = -1;
					defendTarget = qfalse;
				}
			} else {    // defend it
				if ( targetPriority > 1 ) {
					if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
						return qfalse;
					}
					bs->target_goal = bestTarget;
					AIEnter_MP_DefendTarget( bs );
					return qtrue;
				} else {
					targetTime = closestTime;
					gotTarget = -1;
					defendTarget = qtrue;
				}
			}
		}
	}

	//================================================================================================
	// SECONDARY TARGETS()

	// LEADER

	if ( !gotTarget ) {
		if ( bs->leader > -1 ) {
			if ( bs->lead_time > trap_AAS_Time() ) {
				// we must follow them
				goto secondaryTarget;
			}
		}
	}

	// CHECKPOINTS

	if ( !gotTarget ) {

		// Defend checkpoints if they are owned by us. Make sure we disperse troops amongst checkpoints, and actively
		// pursue checkpoints not owned by us more aggressively.

		t = 0;
		gotTarget = qfalse;
		// count the checkpoints
		i = 0;
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_CHECKPOINT ) ) ) {
			// is it disabled?
			if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
				continue;
			}
			i++;
		}
		if ( i ) {
			f = 1.0f / i;
		}
		//if (level.captureFlagMode) {
		//	c = (int)ceil(0.35*f*numTeammates);
		//} else {
		c = (int)ceil( f * numTeammates );
		//}

		// see if we should get/defend one
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_CHECKPOINT ) ) ) {
			// is it disabled?
			if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
				continue;
			}
			// if the opposition team controls this checkpoint, or it hasnt been captured yet
			if ( trav->count == ( bs->sess.sessionTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS ) || ( trav->count < 0 ) ) {
				if ( BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) < c ) {
					// GO GET IT
					// do we have a route to the enemy flag?
					secondary.entitynum = trav->s.number;
					VectorAdd( trav->r.absmin, trav->r.absmax, center );
					VectorScale( center, 0.5, center );
					secondary.areanum = BotReachableBBoxAreaNum( bs, trav->r.absmin, trav->r.absmax );
					if ( !secondary.areanum ) {
						secondary.areanum = BotPointAreaNum( -1, center );
					}
					//secondary.areanum = trap_AAS_PointAreaNum(center);
					secondary.flags = GFL_NOSLOWAPPROACH;
					VectorCopy( trav->r.mins, secondary.mins );
					VectorCopy( trav->r.maxs, secondary.maxs );
					VectorCopy( center, secondary.origin );
					//
					t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, secondary.areanum, bs->tfl );
					if ( t && ( !gotTarget || t < targetTime ) ) {
						targetTime = t;
						gotTarget = -1; // use this if nothing else is around
						defendTarget = qfalse;
						bestTarget = secondary;
						if ( t < 500 && BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) < 1 ) {
							if ( BotGoalWithinMovementAutonomy( bs, &bestTarget, BGU_LOW ) ) {
								if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_TouchTarget ) {
									return qfalse;
								}
								bs->target_goal = bestTarget;
								AIEnter_MP_TouchTarget( bs );
								return qtrue;
							}
						}
					}
				}
			} else {
				// defend the checkpoint?
				if ( !level.captureFlagMode && BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) < ( c > 1 ? c / 2 : c ) ) {
					// GO GET IT
					// do we have a route to the enemy flag?
					secondary.entitynum = trav->s.number;
					VectorAdd( trav->r.absmin, trav->r.absmax, center );
					VectorScale( center, 0.5, center );
					//VectorCopy( trav->r.currentOrigin, center );
					//center[2] += 30;
					secondary.areanum = trap_AAS_PointAreaNum( center );
					if ( !secondary.areanum ) {
						secondary.areanum = BotPointAreaNum( -1, center );
					}
					secondary.flags = GFL_NOSLOWAPPROACH;
					VectorCopy( trav->r.mins, secondary.mins );
					VectorCopy( trav->r.maxs, secondary.maxs );
					VectorCopy( trav->r.currentOrigin, secondary.origin );
					//
					// don't worry about our flag as much
					t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, secondary.areanum, bs->tfl );
					if ( !t ) {
						t = BotFindSparseDefendArea( bs, &secondary, qfalse );
					}
					if ( t ) {
						t = 200 + 2 * t;
						t += bs->client * 100;    // random element to spread bots out
						if ( !gotTarget || t < targetTime ) {
							if ( BotGoalWithinMovementAutonomy( bs, &secondary, BGU_LOW ) ) {
								targetTime = t;
								gotTarget = -1;
								defendTarget = qtrue;
								bestTarget = secondary;
							}
						}
					}
				}
			}
		}
	}

	// ATTRACTORS
	if ( !gotTarget ) {
		trav = NULL;
		while ( ( trav = BotFindNextStaticEntity( trav, BOTSTATICENTITY_BOT_ATTRACTOR ) ) ) {
			if ( trav->aiInactive & ( 1 << bs->sess.sessionTeam ) ) {
				continue;
			}
			if ( BotGoalForEntity( bs, trav->s.number, &target, BGU_MAXIMUM ) ) {
				// get the distance, halve it if there is noone one going for this attractor
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
				if ( BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) == 0 ) {
					t /= 2;
				}
				if ( !gotTarget || ( t < targetTime ) ) {
					targetTime = t;
					gotTarget = -1;
					defendTarget = qtrue;
					bestTarget = target;
				}
			}
		}
	}

	// OBJECTIVE LOCATIONS

	// defend target objectives (both teams should try and "occupy" objectives)
/*	if (gotTarget != qtrue) {
		qboolean useObjective = qfalse;
		// count the objectives
		i = 0;
		c = 999;
		oldest = -1;
		trav = NULL;
		while ( trav = BotFindNextStaticEntity(trav, BOTSTATICENTITY_OBJECTIVE_INFO) ) {
			// is it disabled?
			if (trav->aiInactive & (1<<bs->sess.sessionTeam)) continue;
			//
			// only guard objectives that are attached to constructibles or destructibles
			//
			useObjective = qfalse;
			//
			if (trav->target_ent) {
				if (trav->target_ent->s.eType == ET_CONSTRUCTIBLE) {
					// has it been built?
					// all constructibles are considered important, whether built or not, unless they are disabled in scripting
					//if (BotIsConstructible( bs->sess.sessionTeam, trav->s.number )) {
						useObjective = qtrue;
					//}
				} else if (trav->target_ent->s.eType == ET_EXPLOSIVE) {
					// is it still valid?
					if (trav->target_ent->r.linked) {
						useObjective = qtrue;
					}
				}
				}
				//
			if (!useObjective) continue;	// ignore this objective
			//
			i++;
			if ((k = BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 )) < c) {
				if (!BotGetReachableEntityArea( bs, trav->s.number, &secondary )) continue;	// not reachable
				c = k;
				oldest = trav->s.number;
			}
		}
		if (i) {
			f = 1.0f / i;
		}
		trav = NULL;
		while (trav = BotFindNextStaticEntity(trav, BOTSTATICENTITY_OBJECTIVE_INFO)) {
			// is it disabled?
			if (trav->aiInactive & (1<<bs->sess.sessionTeam)) continue;
			//
			if (BotNumTeamMatesWithTarget( bs, trav->s.number, NULL, 0 ) >= (int)ceil(f*numTeammates)) {
				if (bestPriority >= 2) continue;	// we already have an important goal
				targetPriority = 1;
			} else {
				targetPriority = 2;
			}
			//
			// only guard objectives that are attached to constructibles or destructibles
			//
			useObjective = qfalse;
			//
			if (trav->target_ent) {
				if (trav->target_ent->s.eType == ET_CONSTRUCTIBLE) {
					// has it been built?
					//if (BotIsConstructible( bs->sess.sessionTeam, trav->s.number )) {
						useObjective = qtrue;
					//}
				} else if (trav->target_ent->s.eType == ET_EXPLOSIVE) {
					// is it still valid?
					if (trav->target_ent->r.linked) {
						useObjective = qtrue;
					}
				}
				//
				if (!useObjective) continue;	// ignore this objective
				//
				// GO GET IT
				// do we have a route to the objective?
				if (!BotGetReachableEntityArea( bs, trav->s.number, &secondary )) continue;	// not reachable
				//
				t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, secondary.areanum, bs->tfl);
				if (t) {
					if (!gotTarget || (targetPriority > bestPriority) || (oldest != -1 ? trav->s.number == oldest : t < targetTime)) {
						if (BotGoalWithinMovementAutonomy( bs, &secondary, BGU_LOW )) {
							targetTime = t;
							gotTarget = qtrue;
							defendTarget = qtrue;
							bestTarget = secondary;
							bestPriority = targetPriority;
						}
					}
				}
			}
		}
	}*/

	//================================================================================================
	// PRIMARY TARGETS()

	// FLAGS
	if ( level.captureFlagMode ) {

		if ( bs->sess.sessionTeam == level.attackingTeam ) {
			// ATTACKING

			// do we have the flag?
			if ( BotCarryingFlag( bs->client ) ) {  // we have it!!
				ent = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
				if ( !ent ) {
					ent = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
				}
				if ( ent ) {
					vec3_t bestAreaCenter;
					//
					VectorAdd( ent->r.absmin, ent->r.absmax, brushPos );
					VectorScale( brushPos, 0.5, brushPos );
					// find the best goal area
					numList = trap_AAS_BBoxAreas( ent->r.absmin, ent->r.absmax, list, 32 );
					if ( numList ) {
						oldestTime = -1;
						bestDist = -1;
						for ( i = 0; i < numList; i++ ) {
							if ( !trap_AAS_AreaReachability( list[i] ) ) {
								continue;
							}
							t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
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
									VectorCopy( center, bestAreaCenter );
								}
							}
						}
					}
					if ( bestDist > 0 ) {
						if ( bs->target_goal.entitynum == ent->s.number && bs->ainode == AINode_MP_TouchTarget
							 && bs->target_goal.areanum == oldest ) {
							return qfalse;
						}
						//
						BotClearGoal( &target );
						// use this as the goal origin
						VectorCopy( bestAreaCenter, target.origin );
						VectorCopy( bs->cur_ps.mins, target.mins );
						VectorCopy( bs->cur_ps.maxs, target.maxs );
						target.areanum = oldest;
						target.entitynum = ent->s.number;
						target.flags = GFL_NOSLOWAPPROACH;
						//
						if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
							//
							bs->target_goal = target;
							//
							// if we are close to the static flag, then look for an alternate route
							BotFlagAtBase( level.attackingTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS, &ent );
							if ( ent && VectorDistanceSquared( bs->origin, ent->r.currentOrigin ) < ( 384 * 384 ) ) {
								// check for an alternate route
								if ( ( t > 1000 ) && ( numList = trap_AAS_AlternativeRouteGoals( bs->origin, bestAreaCenter, bs->tfl, altroutegoals, 40, 0 ) ) ) {
									//pick one at random
									i = rand() % numList;
									//make this the altrouetgoal
									bs->alt_goal = bs->target_goal;
									trap_AAS_AreaWaypoint( altroutegoals[i].areanum, bs->alt_goal.origin );
									bs->alt_goal.areanum = trap_AAS_PointAreaNum( bs->alt_goal.origin );
									bs->alt_goal.number = level.time;
									bs->target_goal.number = level.time;
								}
							}
							//
							AIEnter_MP_TouchTarget( bs );
							return qtrue;
						}
					} else {
						// can't get to the destination point
						if ( bs->ainode == AINode_MP_Stand ) {
							return qfalse;
						}
						AIEnter_MP_Stand( bs );
						return qfalse;
					}
				}
			}

			// SNIPER AI
			// note: snipers only operate if flag is at base
			if ( BotCanSnipe( bs, qtrue ) ) {
				// already sniping
				if ( bs->ainode == AINode_MP_SniperSpot ) {
					return qfalse;
				}
				//
				c = BotBestSniperSpot( bs );
				if ( c > -1 ) {
					ent = BotGetEntity( c );
					// do we have a route to the flag?
					BotClearGoal( &target );
					target.entitynum = ent->s.number;
					VectorCopy( ent->s.origin, target.origin );
					target.areanum = BotPointAreaNum( -1, target.origin );
					VectorCopy( bs->cur_ps.mins, target.mins );
					VectorCopy( bs->cur_ps.maxs, target.maxs );
					//
					if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
						bs->target_goal = target;
						AIEnter_MP_SniperSpot( bs );
						return qtrue;
					}
				}
			}

			// MG42 AI
			// note: mg42 only operate if flag is at base
			if ( bs->sess.playerType == PC_SOLDIER ) {
				//
				c = BotBestMG42Spot( bs, !gotTarget );
				if ( c > -1 ) {
					// are we already mounting it?
					if ( ( bs->target_goal.entitynum == c ) && ( bs->ainode == AINode_MP_MG42Mount || bs->ainode == AINode_MP_MG42Scan ) ) {
						return qfalse;
					}
					//
					ent = BotGetEntity( c );
					// do we have a route to the flag?
					BotClearGoal( &target );
					target.entitynum = ent->s.number;
					VectorCopy( ent->s.origin, target.origin );
					target.areanum = BotPointAreaNum( -1, target.origin );
					VectorCopy( bs->cur_ps.mins, target.mins );
					VectorCopy( bs->cur_ps.maxs, target.maxs );
					//
					if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
						bs->target_goal = target;
						AIEnter_MP_MG42Mount( bs );
						return qtrue;
					}
				}
			}

			// is the enemy flag at base?
			if ( BotFlagAtBase( level.attackingTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS, &ent ) == qtrue ) {
				// GO GET IT
				// do we have a route to the enemy flag?
				BotClearGoal( &target );
				target.entitynum = ent->s.number;
				VectorCopy( ent->r.currentOrigin, center );
				center[2] += 30;
				target.areanum = trap_AAS_PointAreaNum( center );
				target.flags = GFL_NOSLOWAPPROACH;
				VectorCopy( ent->r.mins, target.mins );
				VectorCopy( ent->r.maxs, target.maxs );
				VectorCopy( center, target.origin );
				//
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
				if ( t && ( ( t < 500 ) || !gotTarget || defendTarget || ( ( t / 2 ) < targetTime ) ) ) {
					//
					if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_MEDIUM ) ) {
						//
						if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_TouchTarget ) {
							return qfalse;
						}
						bs->target_goal = target;
						//
						// check for an alternate route
						if ( t > 600 && ( numList = trap_AAS_AlternativeRouteGoals( bs->origin, bs->target_goal.origin, bs->tfl, altroutegoals, 40, 0 ) ) ) {
							//pick one at random
							i = rand() % numList;
							//make this the altrouetgoal
							bs->alt_goal = bs->target_goal;
							trap_AAS_AreaWaypoint( altroutegoals[i].areanum, bs->alt_goal.origin );
							bs->alt_goal.areanum = trap_AAS_PointAreaNum( bs->alt_goal.origin );
							bs->alt_goal.number = level.time;
							bs->target_goal.number = level.time;
						}
						//
						AIEnter_MP_TouchTarget( bs );
						return qtrue;
					}
				}
			} else {
				qboolean protect = qtrue;

				// who has it?
				BotClearGoal( &target );
				target.entitynum = BotGetTeamFlagCarrier( bs );
				//
				if ( ( target.entitynum > -1 && target.entitynum != bs->client ) &&
					 ( VectorDistanceSquared( g_entities[target.entitynum].r.currentOrigin, bs->origin ) < ( 1600 * 1600 ) ) /*&&
					(trap_InPVS(g_entities[target.entitynum].r.currentOrigin, bs->origin))*/) {

					// TODO: if there are too many defenders, and we are the furthest away, stop defending
					// !!!
					if ( ( numList = BotNumTeamMatesWithTarget( bs, target.entitynum, list, MAX_CLIENTS ) ) >= BOT_FLAG_CARRIER_DEFENDERS ) {
						ourDist = VectorDistanceSquared( bs->origin, g_entities[target.entitynum].r.currentOrigin );
						//if (!trap_InPVS( bs->origin, g_entities[target.entitynum].r.currentOrigin )) ourDist += 2048;
						distances = BotSortPlayersByDistance( g_entities[target.entitynum].r.currentOrigin, list, numList );
						if ( distances[numList - 1] < ourDist ) {
							// we are the furthest
							protect = qfalse;
						}
					}

					if ( protect ) {
						// protect the carrier
						ent = BotGetEntity( target.entitynum );
						VectorCopy( ent->r.currentOrigin, center );
						target.areanum = BotPointAreaNum( target.entitynum, center );
						target.flags = GFL_NOSLOWAPPROACH;
						VectorCopy( ent->r.mins, target.mins );
						VectorCopy( ent->r.maxs, target.maxs );
						VectorCopy( ent->r.currentOrigin, target.origin );
						//
						t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
						if ( !t ) {
							t = BotFindSparseDefendArea( bs, &target, qfalse );
						}
						if ( t && ( !gotTarget || defendTarget || t < targetTime ) ) {
							if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_HIGH ) ) {
								if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
									return qfalse;
								}
								bs->target_goal = target;
								BotFindSparseDefendArea( bs, &bs->target_goal, qtrue );
								AIEnter_MP_DefendTarget( bs );
								return qtrue;
							}
						}
					}
				}

				// head to the flag destination?
				ent = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
				if ( !ent ) {
					ent = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
				}
				if ( target.entitynum != bs->client && ent && ( BotNumTeamMatesWithTarget( bs, ent->s.number, NULL, 0 ) < (int)ceil( 0.3 * numTeammates ) ) ) {
					VectorAdd( ent->r.absmin, ent->r.absmax, brushPos );
					VectorScale( brushPos, 0.5, brushPos );
					// find the best goal area
					numList = trap_AAS_BBoxAreas( ent->r.absmin, ent->r.absmax, list, 32 );
					if ( numList ) {
						oldestTime = -1;
						bestDist = -1;
						for ( i = 0; i < numList; i++ ) {
							if ( !trap_AAS_AreaReachability( list[i] ) ) {
								continue;
							}
							t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
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
					if ( bestDist > 0 ) {
						BotClearGoal( &target );
						// use this as the goal origin
						VectorCopy( center, target.origin );
						VectorCopy( bs->cur_ps.mins, target.mins );
						VectorCopy( bs->cur_ps.maxs, target.maxs );
						target.areanum = oldest;
						target.entitynum = ent->s.number;
						target.flags = GFL_NOSLOWAPPROACH;
						//
						if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
							if ( bs->target_goal.entitynum == ent->s.number && bs->ainode == AINode_MP_DefendTarget ) {
								return qfalse;
							}
							bs->target_goal = target;
							BotFindSparseDefendArea( bs, &bs->target_goal, qtrue );
							AIEnter_MP_DefendTarget( bs );
							return qtrue;
						}
					}
				}

				// HEAD FOR IT, THEN WAIT FOR IT
				BotFlagAtBase( level.attackingTeam == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS, &ent );
				// do we have a route to the enemy flag?
				BotClearGoal( &target );
				target.entitynum = ent->s.number;
				VectorCopy( ent->r.currentOrigin, center );
				center[2] += 30;
				target.areanum = trap_AAS_PointAreaNum( center );
				target.flags = GFL_NOSLOWAPPROACH;
				VectorCopy( ent->r.mins, target.mins );
				VectorCopy( ent->r.maxs, target.maxs );
				VectorCopy( ent->r.currentOrigin, target.origin );
				//
				t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
				if ( !t ) {
					t = BotFindSparseDefendArea( bs, &target, qfalse );
				}
				if ( t && ( !gotTarget || defendTarget || t < targetTime ) ) {
					if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
						if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
							return qfalse;
						}
						bs->target_goal = target;
						BotFindSparseDefendArea( bs, &bs->target_goal, qtrue );
						AIEnter_MP_DefendTarget( bs );
						return qtrue;
					}
				}
			}

		} else {    // ================================================================
			// DEFENDING()

			// SNIPER AI
			// note: snipers only operate if flag is at base
			if ( BotCanSnipe( bs, qtrue ) ) {
				// already sniping
				if ( bs->ainode == AINode_MP_SniperSpot ) {
					return qfalse;
				}

				c = BotBestSniperSpot( bs );
				if ( c > -1 ) {
					ent = BotGetEntity( c );
					// do we have a route to the flag?
					BotClearGoal( &target );
					target.entitynum = ent->s.number;
					VectorCopy( ent->s.origin, target.origin );
					target.areanum = BotPointAreaNum( -1, target.origin );
					VectorCopy( bs->cur_ps.mins, target.mins );
					VectorCopy( bs->cur_ps.maxs, target.maxs );
					//
					if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
						bs->target_goal = target;
						AIEnter_MP_SniperSpot( bs );
						return qtrue;
					}
				}
			}

			// MG42 AI
			// note: mg42 only operate if flag is at base
			if ( ( bs->sess.playerType == PC_SOLDIER ) && ( BotFlagAtBase( bs->sess.sessionTeam, &ent ) == qtrue ) ) {
				// already sniping
				if ( bs->ainode == AINode_MP_MG42Mount || bs->ainode == AINode_MP_MG42Scan ) {
					return qfalse;
				}
				//
				c = BotBestMG42Spot( bs, !gotTarget );
				if ( c > -1 ) {
					ent = BotGetEntity( c );
					// do we have a route to the flag?
					BotClearGoal( &target );
					target.entitynum = ent->s.number;
					VectorCopy( ent->s.origin, target.origin );
					target.areanum = BotPointAreaNum( -1, target.origin );
					VectorCopy( bs->cur_ps.mins, target.mins );
					VectorCopy( bs->cur_ps.maxs, target.maxs );
					//
					if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
						bs->target_goal = target;
						AIEnter_MP_MG42Mount( bs );
						return qtrue;
					}
				}
			}

			if ( !gotTarget || !level.explosiveTargets[bs->sess.sessionTeam == TEAM_AXIS ? 0 : 1] || ( level.explosiveTargets[bs->sess.sessionTeam == TEAM_AXIS ? 0 : 1] - BotGetTargetExplosives( bs->sess.sessionTeam, NULL, 0, qtrue ) > 0 ) ) {
				// at least one target has been breached, so start defending objective

				// defend the objective!
				if ( BotFlagAtBase( bs->sess.sessionTeam, &ent ) == qtrue ) {
					if ( !gotTarget || BotNumTeamMatesWithTarget( bs, ent->s.number, NULL, 0 ) < (int)ceil( 0.4 * numTeammates ) ) {
						// do we have a route to the flag?
						BotClearGoal( &target );
						target.entitynum = ent->s.number;
						VectorCopy( ent->r.currentOrigin, center );
						center[2] += 30;
						target.areanum = trap_AAS_PointAreaNum( center );
						target.flags = GFL_NOSLOWAPPROACH;
						VectorCopy( ent->r.mins, target.mins );
						VectorCopy( ent->r.maxs, target.maxs );
						VectorCopy( ent->r.currentOrigin, target.origin );
						//
						t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
						if ( t && ( !gotTarget || defendTarget || t < targetTime ) ) {
							if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_MEDIUM ) ) {
								//
								BotFindSparseDefendArea( bs, &target, qtrue );
								//
								if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_MEDIUM ) ) {
									if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
										return qfalse;
									}
									// we are on defense
									BotVoiceChatAfterIdleTime( bs->client, "OnDefense", SAY_TEAM, 1000, qfalse, 10000 + rand() % 5000, qfalse );
									bs->target_goal = target;
									AIEnter_MP_DefendTarget( bs );
									return qtrue;
								}
							}
						}
					}
				} else {    // defend the enemy destination

					ent = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY );
					if ( !ent ) {
						ent = BotFindNextStaticEntity( NULL, BOTSTATICENTITY_FLAGONLY_MULTIPLE );
					}
					if ( ent /*&& (BotNumTeamMatesWithTarget( bs, ent->s.number, NULL, 0 ) < (int)ceil(0.5*numTeammates))*/ ) {
						VectorAdd( ent->r.absmin, ent->r.absmax, brushPos );
						VectorScale( brushPos, 0.5, brushPos );
						bestDist = -1;
						// find the best goal area
						numList = trap_AAS_BBoxAreas( ent->r.absmin, ent->r.absmax, list, 32 );
						if ( numList ) {
							oldestTime = -1;
							for ( i = 0; i < numList; i++ ) {
								if ( !trap_AAS_AreaReachability( list[i] ) ) {
									continue;
								}
								t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, list[i], bs->tfl );
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
						if ( bestDist > 0 ) {
							BotClearGoal( &target );
							// use this as the goal origin
							VectorCopy( center, target.origin );
							VectorCopy( bs->cur_ps.mins, target.mins );
							VectorCopy( bs->cur_ps.maxs, target.maxs );
							target.areanum = oldest;
							target.entitynum = ent->s.number;
							target.flags = GFL_NOSLOWAPPROACH;
							//
							BotFindSparseDefendArea( bs, &target, qtrue );
							if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_MEDIUM ) ) {
								if ( bs->target_goal.entitynum == ent->s.number && bs->ainode == AINode_MP_DefendTarget ) {
									return qfalse;
								}
								bs->target_goal = target;
								AIEnter_MP_DefendTarget( bs );
								return qtrue;
							}
						}
					}
					//
					// go to the flag room
					BotFlagAtBase( bs->sess.sessionTeam, &ent );
					// do we have a route to the flag?
					BotClearGoal( &target );
					target.entitynum = ent->s.number;
					VectorCopy( ent->r.currentOrigin, center );
					center[2] += 30;
					target.areanum = trap_AAS_PointAreaNum( center );
					target.flags = GFL_NOSLOWAPPROACH;
					VectorCopy( ent->r.mins, target.mins );
					VectorCopy( ent->r.maxs, target.maxs );
					VectorCopy( ent->r.currentOrigin, target.origin );
					//
					t = trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin, target.areanum, bs->tfl );
					if ( t && ( !gotTarget || defendTarget || t < targetTime ) ) {
						BotFindSparseDefendArea( bs, &target, qtrue );
						if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_MEDIUM ) ) {
							if ( bs->target_goal.entitynum == target.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
								return qfalse;
							}
							bs->target_goal = target;
							AIEnter_MP_DefendTarget( bs );
							return qtrue;
						}
					}

				}

			}

		}

	} else {    // NOT CTF MODE

		// SNIPER AI
		// note: snipers only operate if flag is at base
		if ( BotCanSnipe( bs, qtrue ) ) {
			// already sniping
			if ( bs->ainode == AINode_MP_SniperSpot ) {
				return qfalse;
			}
			//
			c = BotBestSniperSpot( bs );
			if ( c > -1 ) {
				ent = BotGetEntity( c );
				// do we have a route to the flag?
				BotClearGoal( &target );
				target.entitynum = ent->s.number;
				VectorCopy( ent->s.origin, target.origin );
				target.areanum = BotPointAreaNum( -1, target.origin );
				VectorCopy( bs->cur_ps.mins, target.mins );
				VectorCopy( bs->cur_ps.maxs, target.maxs );
				//
				if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
					bs->target_goal = target;
					AIEnter_MP_SniperSpot( bs );
					return qtrue;
				}
			}
		}

		// MG42 AI
		// note: mg42 only operate if flag is at base
		if ( qtrue ) { //bs->sess.playerType == PC_SOLDIER) {
			// already sniping
			if ( bs->ainode == AINode_MP_MG42Mount || bs->ainode == AINode_MP_MG42Scan ) {
				return qfalse;
			}
			//
			c = BotBestMG42Spot( bs, !gotTarget );
			if ( c > -1 ) {
				ent = BotGetEntity( c );
				// do we have a route to the flag?
				BotClearGoal( &target );
				target.entitynum = ent->s.number;
				VectorCopy( ent->s.origin, target.origin );
				target.areanum = BotPointAreaNum( -1, target.origin );
				VectorCopy( bs->cur_ps.mins, target.mins );
				VectorCopy( bs->cur_ps.maxs, target.maxs );
				//
				if ( BotGoalWithinMovementAutonomy( bs, &target, BGU_LOW ) ) {
					bs->target_goal = target;
					AIEnter_MP_MG42Mount( bs );
					return qtrue;
				}
			}
		}

	}

	//================================================================================================

secondaryTarget:

	// if we found a secondary target (checkpoint), go for it
	if ( gotTarget ) {
		if ( defendTarget ) {
			BotFindSparseDefendArea( bs, &bestTarget, qtrue );
			if ( BotGoalWithinMovementAutonomy( bs, &bestTarget, BGU_LOW ) ) {
				if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_DefendTarget ) {
					return qfalse;
				}
				bs->target_goal = bestTarget;
				AIEnter_MP_DefendTarget( bs );
				return qtrue;
			}
		} else {
			if ( BotGoalWithinMovementAutonomy( bs, &bestTarget, BGU_LOW ) ) {
				if ( bs->target_goal.entitynum == bestTarget.entitynum && bs->ainode == AINode_MP_TouchTarget ) {
					return qfalse;
				}
				bs->target_goal = bestTarget;
				AIEnter_MP_TouchTarget( bs );
				return qtrue;
			}
		}
	}

	return qfalse;
}
