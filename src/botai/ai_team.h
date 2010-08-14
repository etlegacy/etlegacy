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
 * name:		ai_team.h
 *
 * desc:		Wolf bot AI
 *
 *
 *****************************************************************************/

#ifndef BOT_SHOWTEXT
#define BOT_SHOWTEXT    2

typedef enum {
	VCHAT_MEDIC,
	VCHAT_NEEDAMMO,
	VCHAT_NEEDBACKUP,
	VCHAT_DYNAMITEPLANTED,
	VCHAT_GREATSHOT,
	VCHAT_HI,
	VCHAT_THANKS,
	VCHAT_FOLLOWME,
	VCHAT_DROPFLAG,
} vchat_id_t;


#endif


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT AI_Team
//
typedef struct AI_Team_s
{
	// Last time we had a vo from this team
	int last_voice_chat;

} AI_Team_t;
//
// STRUCT AI_Team
//
///////////////////////////////////////////////////////////////////////////////



qboolean BotCheckNeedEngineer( bot_state_t *bs, team_t team  );
int BotSuggestClass( bot_state_t *bs, team_t team );
int BotFlagAtBase( int team, gentity_t **returnEnt );
qboolean BotCheckEmergencyTargets( bot_state_t *bs );
int BotFindSparseDefendArea( bot_state_t *bs, bot_goal_t *goal, qboolean force );
int BotNumTeamMatesWithTarget( bot_state_t *bs, int targetEntity, int *list, int maxList );
int BotNumTeamMatesWithTargetAndCloser( bot_state_t *bs, int targetEntity, int targetArea, int *list, int maxList, int playerType );
int BotNumTeamMatesWithTargetByClass( bot_state_t *bs, int targetEntity, int *list, int maxList, int playerType );
int BotNumTeamMates( bot_state_t *bs, int *list, int maxList );
gentity_t *BotGetEnemyFlagCarrier( bot_state_t *bs );
// Start - TAT 9/20/2002
// covert ops bot looks for nearby corpses
qboolean BotClass_CovertOpsCheckDisguises( bot_state_t *bs, int maxTravel, bot_goal_t *goal );
// End - TAT 9/20/2002
qboolean BotClass_MedicCheckRevives( bot_state_t *bs, int maxtravel, bot_goal_t *goal, qboolean lookForBots );
qboolean BotClass_MedicCheckGiveHealth( bot_state_t *bs, int maxTravelTime, bot_goal_t *goal );
qboolean BotClass_LtCheckGiveAmmo( bot_state_t *bs, int maxTravelTime, bot_goal_t *goal );
qboolean BotFindDroppedFlag( gentity_t **returnEnt );
void BotSendVoiceChat( bot_state_t *bs, const char *id, int mode, int delay, qboolean voiceonly, qboolean forceIfDead );
void BotCheckVoiceChatResponse( bot_state_t *bs );
void BotSpawnSpecialEntities( void );
int BotSuggestWeapon( bot_state_t *bs, team_t team );
int BotGetTeamFlagCarrier( bot_state_t *bs );
float *BotSortPlayersByDistance( vec3_t target, int *list, int numList );
float *BotSortPlayersByTraveltime( int areanum, int *list, int numList );
int BotGetLeader( bot_state_t *bs, qboolean onlyRequested );
void BotVoiceChatAfterIdleTime( int client, const char *id, int mode, int delay, qboolean voiceonly, int idleTime, qboolean forceIfDead );
void BotVoiceChatAfterTeamIdleTime( int client, const char *id, int mode, int delay, qboolean voiceonly, int idleTime, qboolean forceIfDead );
void BotSetLeaderTagEnt( bot_state_t *bs );
int BotEngagementFunc( bot_state_t *bs );
int BotBehaviourFunc( bot_state_t *bs );
int BotObjectiveFunc( bot_state_t *bs );
qboolean G_RequestedAmmo( bot_state_t *bs, int client, qboolean clear );
int BotGetConstructibles( team_t team, int *list, int listSize, qboolean ignoreBuilt );
int BotClosestSeekCoverSpot( bot_state_t *bs );

// OK, this one looks in the specified direction (retreat/advance) and
// find the next available spot.
// Returns the entity number of the spot, -1 if N/A.
int BotSquadGetNextAvailableSeekCoverSpot
(
	// The info for the bot
	bot_state_t *bs,
	// Moving forwards or backwards?
	qboolean advance,
	// entity id of the player we should check being in a seek_cover_sequence
	int entityId
);
//	TAT 12/19/2002 - When a bot sees an enemy, he tells his squadmates
void BotSquadEnemySight( bot_state_t *bs, int enemy );


// TAT 12/10/2002
//		If possible, find a seek cover spot marked as exposed that is a parent or child of the current seek cover spot
int BotGetAdjacentExposedCoverSpot( bot_state_t *bs );

// TAT 1/8/2003
//	Get the next cover spot towards the retreatTo entity - used when retreating
int BotGetRetreatingCoverSpot( bot_state_t *bs, int retreatTo );

void AI_Team_Init_All_Teams();
