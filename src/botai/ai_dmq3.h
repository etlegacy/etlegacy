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
 * name:		ai_dmq3.h
 *
 * desc:		Wolf bot AI
 *
 *
 *****************************************************************************/



// How many targets we'll let the bots choose between?
#define BOT_MAX_POTENTIAL_TARGETS 10

// This value stands for an un-inited/bad target
#define BOT_INVALID_TARGET ( -1 )

// Penalties and bonusses used in choosing targets
#define BOT_TARGET_ALREADY_TARGETTED_PENALTY ( 5 )
#define BOT_TARGET_PERSISTENT_ENEMY_BONUS ( -2 )
#define BOT_TARGET_SHOT_ME_BONUS ( -9 )
#define BOT_TARGET_VISUAL_BONUS ( -2 )

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT BotTarget
//
typedef struct BotTarget_s
{
	// Who is this target?
	int m_target;

	// Distance from bot
	float m_distance;

	// Did he shoot us?
	qboolean m_thisGuyShotUsDammit;

	// Did we see him?
	qboolean m_canSeeTarget;

	// Did we hear him?
	qboolean m_canHearTarget;

	// Did we hear footsteps?
	qboolean m_heardFootSteps;

	// Did we hear shooting?
	qboolean m_heardShooting;

} BotTarget_t;
//
// STRUCT AI_Team
//
///////////////////////////////////////////////////////////////////////////////

// Did this bot just shoot me?
qboolean BotJustShotMe( bot_state_t *bs, int suspect );

void BotCountLandMines( void );
int BotBestTargetWeapon( bot_state_t *bs, int targetNum );
gentity_t *BotGetVisibleDamagableScriptMover( bot_state_t *bs );
qboolean BotEnemyCarryingFlag( int entnum );
float BotHealthScale( int entnum );
int BotGetNumVisibleSniperSpots( bot_state_t *bs );
int BotCanSnipe( bot_state_t *bs, qboolean checkAmmo );
qboolean BotIsConstructible( team_t botTeam, int toiNum );
qboolean BotGetReachableEntityArea( bot_state_t *bs, int entityNum, bot_goal_t *goal );
qboolean BotEntityTargetClassnameMatch( int entityNum, const char *classname );
qboolean BotDirectMoveToGoal( bot_state_t *bs, bot_goal_t *goal, bot_moveresult_t *moveresult );
int BotLastHurt( bot_state_t *bs );
qboolean BotBattleNewNode( bot_state_t *bs );
int BotLastAttacked( bot_state_t *bs );
int BotTravelTimeToEntity( bot_state_t *bs, int entnum );
int BotReduceListByTravelTime( int *list, int numList, vec3_t destpos, int destarea, int traveltime );
qboolean BotCanAttack( bot_state_t *bs );
float *BotGetOrigin( int entnum );
float *BotGetEye( int entnum );
int BotGetArea( int entnum );
qboolean BotSeekCover( bot_state_t *bs );
void BotShareLastAttacked( bot_state_t *bs );
char *BotStringForMovementAutonomy( int value );
char *BotStringForWeaponAutonomy( int value );
qboolean BotCanPursue( bot_state_t *bs, int enemy );
int BotGetMovementAutonomyLevel( bot_state_t *bs );
void BotEnemyFire( bot_state_t *bs );
void BotDefaultNode( bot_state_t *bs );
qboolean BotGetMovementAutonomyPos( bot_state_t *bs, vec3_t pos );
qboolean BotCheckEmergencyTargets( bot_state_t *bs );
qboolean BotFindSpecialGoals( bot_state_t *bs );
void BotCheckVoiceChats( bot_state_t *bs );
int BotWeaponAutonomyForString( char *string );
int BotMovementAutonomyForString( char *string );
int BotScriptAutonomyForString( char *string );
float BotGetRawMovementAutonomyRange( bot_state_t *bs );
float BotGetMovementAutonomyRange( bot_state_t *bs, bot_goal_t *goal );

// Start - TAT 9/18/2002
//	What distance should a bot be from its leader during a follow order?  Based on autonomy
float BotGetFollowAutonomyDist( bot_state_t *bs );

// Is a bot within the desired distance of its leader?
qboolean BotWithinLeaderFollowDist( bot_state_t *bs );
// End - TAT 9/18/2002

// Start	TAT 9/23/2002
// Update recon state information for a bot
void BotUpdateReconInfo( bot_state_t *bs );

qboolean BotPointWithinMovementAutonomy( bot_state_t *bs, bot_goal_t *goal, vec3_t point );
qboolean BotPointWithinRawMovementAutonomy( bot_state_t *bs, vec3_t point );
qboolean BotGoalWithinMovementAutonomy( bot_state_t *bs, bot_goal_t *goal, int urgency );
qboolean BotCheckMovementAutonomy( bot_state_t *bs, bot_goal_t *goal );
qboolean BotGoalForEntity( bot_state_t *bs, int entityNum, bot_goal_t *goal, int urgency );
qboolean BotDangerousGoal( bot_state_t *bs, bot_goal_t *goal );
qboolean BotCarryingFlag( int client );
qboolean BotFindNearbyGoal( bot_state_t *bs );

float BotWeaponWantScale( bot_state_t *bs, weapon_t weapon );
qboolean BotWeaponCharged( bot_state_t *bs, int weapon );
void BotRecordTeamChange( int client );
void BotRecordKill( int client, int enemy );
void BotRecordPain( int client, int enemy, int mod );
void BotRecordDeath( int client, int enemy );
void BotGetAimAccuracySkill( bot_state_t *bs, float *outAimAccuracy, float *outSkill );
int BotBestSniperSpot( bot_state_t *bs );
int BotBestLandmineSpotingSpot( bot_state_t *bs );
int BotGetRandomVisibleSniperSpot( bot_state_t *bs );
void BotClearGoal( bot_goal_t *goal );
qboolean BotGotEnoughAmmoForWeapon( bot_state_t *bs, int weapon );
int BotReachableBBoxAreaNum( bot_state_t *bs, vec3_t absmin, vec3_t absmax );
void BotDropFlag( bot_state_t *bs );
int BotBestMG42Spot( bot_state_t *bs, qboolean force );

//setup the deathmatch AI
void BotSetupDeathmatchAI( void );
//shutdown the deathmatch AI
void BotShutdownDeathmatchAI( void );
//let the bot live within it's deathmatch AI net
void BotDeathmatchAI( bot_state_t *bs, float thinktime );
//free waypoints
void BotFreeWaypoints( bot_waypoint_t *wp );
//choose a weapon
void BotChooseWeapon( bot_state_t *bs );

// TAT 11/14/2002 -Bot cycles to next weapon in inventory, and will use it until told to cycle again
void BotCycleWeapon( bot_state_t *bs );

// Gordon: set pow status
void BotSetPOW( int entityNum, qboolean isPOW );

//setup movement stuff
void BotSetupForMovement( bot_state_t *bs );
//update the inventory
void BotUpdateInventory( bot_state_t *bs );
//update the inventory during battle
void BotUpdateBattleInventory( bot_state_t *bs, int enemy );
//use holdable items during battle
void BotBattleUseItems( bot_state_t *bs );
//return true if the bot is dead
qboolean BotIsDead( bot_state_t *bs );
//returns true if the bot is in observer mode
qboolean BotIsPOW( bot_state_t *bs );
//Gordon: returns true if the bot is a prisoner of war
qboolean BotIsObserver( bot_state_t *bs );
//returns true if the bot is in the intermission
qboolean BotIntermission( bot_state_t *bs );
//returns true if the bot is in lava
qboolean BotInLava( bot_state_t *bs );
//returns true if the bot is in slime
qboolean BotInSlime( bot_state_t *bs );
//returns true if the entity is dead
qboolean EntityIsDead( aas_entityinfo_t *entinfo );
//returns true if the entity is in limbo
qboolean EntityInLimbo( aas_entityinfo_t *entinfo );
//returns true if the entity is invisible
qboolean EntityIsInvisible( aas_entityinfo_t *entinfo );
//returns true if the entity is shooting
qboolean EntityIsShooting( aas_entityinfo_t *entinfo );
//returns the name of the client
char *ClientName( int client, char *name, int size );
//returns an simplyfied client name
char *EasyClientName( int client, char *name, int size );
//returns the skin used by the client
char *ClientSkin( int client, char *skin, int size );
//returns the aggression of the bot in the range [0, 100]
float BotAggression( bot_state_t *bs );
//returns true if the bot wants to retreat
int BotWantsToRetreat( bot_state_t *bs );
//returns true if the bot wants to chase
int BotWantsToChase( bot_state_t *bs );
//returns true if the bot wants to help
int BotWantsToHelp( bot_state_t *bs );
//returns true if the bot can and wants to rocketjump
int BotCanAndWantsToRocketJump( bot_state_t *bs );
//returns true if the bot wants to and goes camping
int BotWantsToCamp( bot_state_t *bs );
//the bot will perform attack movements
bot_moveresult_t BotAttackMove( bot_state_t *bs, int tfl );
//returns true if the bot and the entity are in the same team
int BotSameTeam( bot_state_t *bs, int entnum );

//returns true if teamplay is on
int TeamPlayIsOn( void );

// Set up our danger spots
void BotFindEnemies
(
	bot_state_t *bs,
	int *dangerSpots,
	int *dangerSpotCount
);

// Returns true if we can do the battle fight.
// Returns false if enemy is too far or not visible
qboolean EnemyIsCloseEnoughToFight( bot_state_t *bs );

// TAT 11/21/2002
// Find an enemy and try to attack it
void BotFindAndAttackEnemy( bot_state_t *bs );
// Update the viewangle
void BotUpdateViewAngles( bot_state_t *bs, bot_goal_t *goal, bot_moveresult_t moveresult );

//returns true and sets the .enemy field when an enemy is found
int BotFindEnemyMP( bot_state_t *bs, int curenemy, qboolean ignoreViewRestrictions );

//returns true if the entity is within our view restrictions
qboolean BotEntityWithinView( bot_state_t *bs, int viewEnt );

float BotWeaponRange( bot_state_t *bs, int weaponnum );
qboolean BotScopedWeapon( int weapon );

//returns a roam goal
void BotRoamGoal( bot_state_t *bs, vec3_t goal );
//returns entity visibility in the range [0, 1]
float BotEntityVisible( int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent, vec3_t entorigin );
//the bot will aim at the current enemy
void BotAimAtEnemy( bot_state_t *bs );
//the bot will aim at the current enemy
void BotAimAtEnemySP( bot_state_t *bs );
//check if the bot should attack
qboolean BotCheckAttack( bot_state_t *bs );
//AI when the bot is blocked
void BotAIBlocked( bot_state_t *bs, bot_moveresult_t *moveresult, int activate );
//create a new waypoint
bot_waypoint_t *BotCreateWayPoint( char *name, vec3_t origin, int areanum );
//find a waypoint with the given name
bot_waypoint_t *BotFindWayPoint( bot_waypoint_t *waypoints, char *name );
//strstr but case insensitive
char *stristr( char *str, char *charset );
//
int BotPointAreaNum( int entnum, vec3_t origin );
//
void BotMapScripts( bot_state_t *bs );
//
qboolean BotMoveWhileFiring( int weapon );

qboolean ChangeBotAlertState( bot_state_t *bs, aistateEnum_t newAlertState, qboolean force );

//ctf flags
#define CTF_FLAG_NONE       0
#define CTF_FLAG_RED        1
#define CTF_FLAG_BLUE       2
//CTF skins
#define CTF_SKIN_REDTEAM    "red"
#define CTF_SKIN_BLUETEAM   "blue"
//CTF teams
#define CTF_TEAM_NONE       0
#define CTF_TEAM_AXIS       1
#define CTF_TEAM_ALLIES     2

extern int gametype;        //game type

// Rafael gameskill
extern int gameskill;
// done

extern vmCvar_t bot_grapple;
extern vmCvar_t bot_rocketjump;
extern vmCvar_t bot_fastchat;
extern vmCvar_t bot_nochat;
extern vmCvar_t bot_testrchat;

extern bot_goal_t ctf_redflag;
extern bot_goal_t ctf_blueflag;
