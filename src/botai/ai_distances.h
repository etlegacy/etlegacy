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
 * name:		ai_distances.h
 *
 * desc:		Distance constants used by the AI in Single Player
 *
 *
 *****************************************************************************/

// Distance bots follow the leader
#define kBOT_FOLLOW_DIST 100

// When is a bot close enough to it's goal?
//		For when the goal is another bot
#define kBOT_CLOSE_ENOUGH 72

// For when we have a location goal
#define kBOT_LOC_CLOSE_ENOUGH 40

// When looking for nearby goals (to pick up health/ammo/flags), how far away can we look
#define kBOT_NEARBY_GOAL_DIST 384
// When looking for nearby goals (to pick up health/ammo/flags), how long can we take to get there
#define kBOT_NEARBY_GOAL_TIME 1000

// For goals where we need to touch our target - revive, get disguise, open door
#define kBOT_TOUCHING_DIST 50

// How far to retreat after reviving someone
#define kBOT_REVIVE_RETREAT_DIST 96

// How close we should be to call in an airstrike, or throw a smoke grenade
#define kBOT_AIRSTRIKE_DISTANCE 160

// The range we're using for the grenade launcher WP_M7
#define kBOT_M7_RANGE 1024

// Ok, so these aren't distances.  Oh well
// The health fractions at which bots start requesting a medic
#define kBOT_INJURED_LEVEL 0.4f
#define kBOT_REALLYINJURED_LEVEL 0.2f

// It a client is going faster than this, we think they're moving
#define kBOT_ENTITY_MOVING_UNSTEALTHY_SPEED 2

// If our best enemy is this close to us, and we're on an mg42, drop
// the gun...
#define kBOT_DROP_MG42_DISTANCE 240 // 20 feet roughly

// If our best enemy is this close, and out of our gun arc, drop
// the mg42
#define kBOT_DROP_MG42_DISTANCE_OUT_OF_ARC 480 // 40 feet roughly

// Really way too far for an mg42 to shoot
#define kBOT_MAX_MG42_TARGET_RANGE 2400

// Radius to call for help - idle friendly bots in this radius are expected to come help their teammates
#define kBOT_HELP_RADIUS 1000

// how close should player be to play the idle animation
#define kBOT_IDLE_ANIM_DISTANCE 384

// A distance which is much bigger than any we'd really ever need
#define kBOT_GIGANTIC_DISTANCE 999999

// The range out of which a bot will advance towards the player
#define kBOT_CHASE_RANGE 1200

// Don't go to a seek cover spot further than this
#define kBOT_MAX_SEEK_COVER_RANGE 1000

// Maximum distance enemies can be from you to be considered in the
// AAS_Retreat code
#define kBOT_MAX_RETREAT_ENEMY_DIST 2000

// How much away from our current pos we'd like to be on retreat
#define kBOT_RETREAT_FROM_CURRENT_POS_DIST 200

// How much away from any danger we'd like to be on retreat
#define kBOT_RETREAT_FROM_DANGER_DIST 800

// How close is close enough when retreating to the player
#define kBOT_RETREAT_TO_PLAYER_DIST 192

// How close is close enough to apply the "I'm near a leader" bonusses
#define kBOT_NEAR_LEADER_DISTANCE 800

// How far is far enough that we get max "No leader" penalty
#define kBOT_FAR_FROM_LEADER_DISTANCE 2000

// How long when following to change modes
#define kBOT_FOLLOW_DEFAULT_TIME 1500
#define kBOT_FOLLOW_DEFAULT_RAND_TIME 2500

// time to change follow stance
#define kBOT_FOLLOW_STANCE_TIME 1000
#define kBOT_FOLLOW_STANCE_RAND_TIME 4000

// Time after standing still to seek cover
#define kBOT_STAND_SEEKCOVER_TIME 5000
#define kBOT_STAND_SEEKCOVER_RAND_TIME 3000

// Distance can travel on stand to seek cover - should be pretty small
#define kBOT_STAND_SEEKCOVER_DIST 256

///////////////////////////////////////
//
// SPEED CONSTANTS
//
///////////////////////////////////////

#define kBOT_FOLLOW_SPEED_BONUS ( 1.25f )

///////////////////////////////////////
//
// TIME CONSTANTS
//
// Most of these are in milliseconds
//
///////////////////////////////////////


// A short period of time to indicate we were just shot by someone
#define kBOT_JUST_SHOT_TIME 1000

// Max travel time a bot will spend travelling to heal/give ammo on a give team health/ammo command
//		NOTE: also checks autonomy range, so this is just an additional check
#define kBOT_MAX_RESUPPLY_TRAVEL_TIME 3000


//////////////////////////////
// Combat time constants
//

// Time to give up on an enemy when you haven't been able to shoot at him
#define kBOT_ENEMY_GIVEUP_TIME 20000

// How long minimum to stay at a combat spot?
#define kBOT_MIN_COMBAT_SPOT_TIME 3500

// Random addition for staying at a combat spot
#define kBOT_RANDOM_COMBAT_SPOT_TIME 5500

// Time to stay at a cover spot if we're switching between it and an exposed spot
#define kBOT_MIN_COVERSPOT_TIME_WITH_EXPOSED 12000
//		and random addition
#define kBOT_RANDOM_COVERSPOT_TIME_WITH_EXPOSED 4000

// How long minimum to stay crouched down?
#define kBOT_HIDE_MIN_TIME 3500
// How much to vary the crouch time randomly
#define kBOT_HIDE_TIME_RANDOM 2500

// How long to stand up after hiding in a seek cover spot
#define kBOT_COVER_STAND_MIN_TIME 500
// and random addition
#define kBOT_COVER_STAND_TIME_RANDOM 1000

// For how long after we've been shot will we do evasive maneuvers (only with no seek cover spot!)
#define kBOT_EVASIVE_MANEUVER_TIME 5000

// How often do we check to see if we can fire
#define kBOT_MIN_FIRE_CYCLE_TIME 1500
#define kBOT_MAX_FIRE_CYCLE_TIME 2700

// For how long do we consider an axis cover spot claimed
#define kBOT_IGNORE_COMBAT_SPOT_TIME 500

// Don't keep replanning our retreat location too often.
#define kBOT_MIN_RETREAT_REPLAN_TIME 3000

// If we've retreated and not been shot for a while, go to Stand.
#define kBOT_MAX_RETREAT_TIME 45000

// Minimum time to wait between calling for help
#define kBOT_MIN_CALL_FOR_HELP_TIME 15000

// If an enemy is within this distance, bot won't go prone
#define kBOT_MIN_PRONE_DIST 256

////////////////////////////////////////////////
// Retreat to Player constants
//

// How frequently do we check to see if we should retreat?
#define kBOT_RETREAT_UPDATE_TIME 100
#define kBOT_RETREAT_UPDATE_RANDOM 500

// Damage level below which we never break - this gets modified by the wimp factor
#define kBOT_RETREAT_DAMAGE_THRESHOLD 0.75f
// Damage level at which we are considered to be about to die
#define kBOT_RETREAT_ABOUT_TO_DIE_DAMAGE 0.8f

// Multiplier to wimp factor to decide when we retreat to player
#define kBOT_RETREAT_MULTIPLIER 1.0
// when we retreat all out
#define kBOT_RETREAT_ALLOUT_MULTIPLIER 2.0

// Percent of damage ratio to add to likeliness to break
#define kBOT_RETREAT_DAMAGE_FACTOR 0.5f

// Multiplier to distance ratio to add to likeliness
#define kBOT_RETREAT_DISTANCE_FACTOR 0.3f

// Maximum distFactor we care about for retreating
#define kBOT_MAX_RETREAT_DIST_FACTOR 3.0f

// Amount to increase break likeliness if...
// can't see the player
#define kBOT_RETREAT_PLAYER_OUTOFSIGHT 0.2f

// player is dead
#define kBOT_RETREAT_PLAYER_DEAD 0.4f

// How long after retreating before we can engage again?
#define kBOT_RETREAT_TIMETOENGAGE 20000

////////////////////////////////////////////////
// Combat penalty constants
//

// How much to penalize damage ratio based on distance from leader
// NOTE: 1 == take full damage at max dist, 0 == use scripted ratio
#define NO_LEADER_DAMAGE_PENALTY ( 0.6f )

// How much to penalize the aiming accuracy based on distance from
// leader.  1 == can't hit at all, 0 == No penalty
#define NO_LEADER_MAX_AIM_PENALTY ( 0.5f )

// how much inaccuracy to remove. e.g. 0.5 = 50% less inaccuracy
#define AIM_ACCURACY_BONUS_PRONE    0.5
#define AIM_ACCURACY_BONUS_CROUCH   0.3
#define AIM_SKILL_BONUS_PRONE       0.5
#define AIM_ACCURACY_ENEMY_PENALTY_PRONE    0.5
#define AIM_ACCURACY_ENEMY_PENALTY_CROUCH   0.2
