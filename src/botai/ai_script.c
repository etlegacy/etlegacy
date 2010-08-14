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

//===========================================================================
//
// Name:			ai_script.c
// Function:		Wolfenstein BOT Scripting
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_main.h"
#include "ai_dmq3.h"

/*
Scripting that allows the designwers to control the behaviour of AI characters
according to each different scenario.
*/

// action functions need to be declared here so they can be accessed in the scriptAction table
qboolean Bot_ScriptAction_Print( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SetAccumToPlayerCount( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SpawnBot( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_Accum( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_GlobalAccum( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_Wait( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_MoveToMarker( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_Trigger( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_Logging( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_AbortIfWarmup( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SetAttribute( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_MountMG42( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SetWeapon( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SetClass( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SetMovementAutonomy( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_FollowLeader( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_Cvar( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_MovementAutonomy( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_ResetScript( bot_state_t *bs, char *params );

// Mad Doctor I, 8/12/2002. Add a script function to set a bot as untargettable
qboolean Bot_ScriptAction_NoTarget( bot_state_t *bs, char *params );

// Mad Doctor I, 8/15/2002. Allow changing the FOV
qboolean Bot_ScriptAction_SetFieldOfView( bot_state_t *bs, char *params );

// Mad Doctor I, 8/15/2002. Change the hearing range
qboolean Bot_ScriptAction_SetHearingRange( bot_state_t *bs, char *params );

// Mad Doctor I changes, 8/19/2002. Play a sound right where the player is
qboolean Bot_ScriptAction_PlaySoundAtPlayer( bot_state_t *bs, char *params );

// xkan, 8/20/2002
qboolean Bot_ScriptAction_SetCrouch( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_SetProne( bot_state_t *bs, char *params );

// Mad Doc - TDF
qboolean Bot_ScriptAction_PrintAccum( bot_state_t *bs, char *params );
qboolean Bot_ScriptAction_PrintGlobalAccum( bot_state_t *bs, char *params );

qboolean Bot_ScriptAction_SetSpeed( bot_state_t *bs, char *params );

// Mad Doc - TDF
qboolean Bot_ScriptAction_BotDebugging( bot_state_t *bs, char *params );

// Mad Doctor I, 10/23/2002.  Vision range settings.
qboolean Bot_ScriptAction_SetVisionRange( bot_state_t *bs, char *params );

// Mad Doctor I, 10/23/2002.  Range for "spot & report", not attack.
qboolean Bot_ScriptAction_SetFarSeeingRange( bot_state_t *bs, char *params );

// Mad Doctor I, 10/23/2002.  When an enemy is this close, you can sense them outside FOV.
qboolean Bot_ScriptAction_SetCloseHearingRange( bot_state_t *bs, char *params );

// Mad Doctor I, 11/26/2002.  Set an individual bot's speed
qboolean Bot_ScriptAction_SetSpeedCoefficient( bot_state_t *bs, char *params );

// xkan, 10/23/2002
qboolean Bot_ScriptAction_SetFireRate( bot_state_t *bs, char *params );
// xkan, 12/19/2002
qboolean Bot_ScriptAction_SetFireCycleTime( bot_state_t *bs, char *params );

// TAT 11/16/2002 - Set the selected weapon of the bot - does NOT change which weapons the bot is holding
qboolean Bot_ScriptAction_SetActiveWeapon( bot_state_t *bs, char *params );

// Gordon: 31/10/02, give players a centerprint message
qboolean Bot_ScriptAction_Announce( bot_state_t *bs, char *params );

// TAT 11/21/2002 - From SP, bot will fire at target for duration
qboolean Bot_ScriptAction_FireAtTarget( bot_state_t *bs, char *params );

// TAT 12/6/2002 - script autonomy is used to determine if bots will break their scripted actions to attack or chase
qboolean Bot_ScriptAction_SetScriptAutonomy( bot_state_t *bs, char *params );

// TAT 12/14/2002 - Set how much ammo we have for a particular weapon
//		Doesn't change the current weapon loadout (which weapons we carry)
qboolean Bot_ScriptAction_SetAmmoAmount( bot_state_t *bs, char *params );

qboolean Bot_ScriptAction_SetCivilian( bot_state_t *bs, char *params );

// these are the actions that each event can call
static bot_script_stack_action_t botScriptActions[] =
{
	{"print",                        Bot_ScriptAction_Print},
	{"SetAccumToPlayerCount",        Bot_ScriptAction_SetAccumToPlayerCount},
	{"SpawnBot",                 Bot_ScriptAction_SpawnBot},
	{"Accum",                        Bot_ScriptAction_Accum},
	{"GlobalAccum",                  Bot_ScriptAction_GlobalAccum},
	{"Wait",                     Bot_ScriptAction_Wait},
	{"MoveToMarker",             Bot_ScriptAction_MoveToMarker},
	{"Trigger",                      Bot_ScriptAction_Trigger},
	{"Logging",                      Bot_ScriptAction_Logging},
	{"AbortIfWarmup",                Bot_ScriptAction_AbortIfWarmup},
	{"SetAttribute",             Bot_ScriptAction_SetAttribute},
	{"MountMG42",                    Bot_ScriptAction_MountMG42},
	{"SetWeapon",                    Bot_ScriptAction_SetWeapon},
	{"SetClass",                 Bot_ScriptAction_SetClass},
	{"SetMovementAutonomy",          Bot_ScriptAction_SetMovementAutonomy},
	{"FollowLeader",             Bot_ScriptAction_FollowLeader},
	{"Cvar",                     Bot_ScriptAction_Cvar},
	{"MovementAutonomy",         Bot_ScriptAction_MovementAutonomy},
	{"NoTarget",                 Bot_ScriptAction_NoTarget},
	{"ResetScript",                  Bot_ScriptAction_ResetScript},

	// Mad Doctor I, 8/15/2002
	{"SetFieldOfView",               Bot_ScriptAction_SetFieldOfView},
	{"SetHearingRange",              Bot_ScriptAction_SetHearingRange},

	// Mad Doctor I, 8/19/2002
	{"PlaySoundAtPlayer",            Bot_ScriptAction_PlaySoundAtPlayer},

	// xkan 8/20/2002
	{"SetCrouch",                    Bot_ScriptAction_SetCrouch},
	{"SetProne",                 Bot_ScriptAction_SetProne},

	// Mad Doc - TDF
	{"PrintAccum",                   Bot_ScriptAction_PrintAccum},
	{"PrintGlobalAccum",         Bot_ScriptAction_PrintGlobalAccum},

	{"setspeed",                 Bot_ScriptAction_SetSpeed},

	// Mad Doc - TDF, toggle bot thought bubbles
	{"BotDebugging",             Bot_ScriptAction_BotDebugging},

	// Mad Doctor I, 10/23/2002.  Vision range settings.
	{"SetVisionRange",               Bot_ScriptAction_SetVisionRange},

	// Mad Doctor I, 10/23/2002.  Range for "spot & report", not attack.
	{"SetFarSeeingRange",            Bot_ScriptAction_SetFarSeeingRange},

	// Mad Doctor I, 10/23/2002.  When an enemy is this close, you can sense them outside FOV.
	{"SetCloseHearingRange",     Bot_ScriptAction_SetCloseHearingRange},

	// Mad Doctor I, 11/26/2002.  Set an individual bot's speed
	{"SetSpeedCoefficient",          Bot_ScriptAction_SetSpeedCoefficient},

	// xkan, 10/23/2002. Rate of slowdown/interruption to normally continuous firing
	{"SetFireRate",                  Bot_ScriptAction_SetFireRate},
	// xkan, 12/19/2002, fire cycle time to break up based on fire rate.
	{"SetFireCycleTime",         Bot_ScriptAction_SetFireCycleTime},

	// TAT 11/16/2002 - Set the selected weapon of the bot - does NOT change which weapons the bot is holding
	{"SetActiveWeapon",              Bot_ScriptAction_SetActiveWeapon},

	// Gordon: just need this temporarily to give the player some messages, should remove later
	{"wm_announce",                  Bot_ScriptAction_Announce},

	// TAT 11/21/2002 - From SP, bot will fire at target for duration
	{"FireAtTarget",             Bot_ScriptAction_FireAtTarget},

	// TAT 12/6/2002 - script autonomy is used to determine if bots will break their scripted actions to attack or chase
	{"SetScriptAutonomy",            Bot_ScriptAction_SetScriptAutonomy},

	// TAT 12/14/2002 - Set how much ammo we have for a particular weapon
	//		Doesn't change the current weapon loadout (which weapons we carry)
	{"SetAmmoAmount",                Bot_ScriptAction_SetAmmoAmount},

	// xkan, 1/6/2003 - mark this bot as whether he is civilian
	{"SetCivilian",                  Bot_ScriptAction_SetCivilian},

	{NULL,                          NULL}
};

qboolean Bot_EventMatch_StringEqual( bot_script_event_t *event, char *eventParm );
qboolean Bot_EventMatch_IntInRange( bot_script_event_t *event, char *eventParm );

// the list of events that can start an action sequence
// NOTE!!: only append to this list, DO NOT INSERT!!
static bot_script_event_define_t botScriptEvents[] =
{
	{"spawn",                        NULL},          // called as each character is spawned into the game
	{"trigger",                      Bot_EventMatch_StringEqual},    // something has triggered us (always followed by an identifier)
	{"pain",                     Bot_EventMatch_IntInRange}, // we've been hurt
	{"death",                        NULL},          // RIP
	{"activate",                 Bot_EventMatch_StringEqual},    // "param" has just activated us
	{"objective",                    Bot_EventMatch_StringEqual},    // something has occured involving the objective
	{"respawn",                      Bot_EventMatch_StringEqual},    // respawned at "param" spawnpoint
	{"enemysight",                   Bot_EventMatch_StringEqual},    //
	{"revived",                      NULL},                          // bot was revived by a comrade

	{NULL,                          NULL}
};

int numSecrets;

/*
=============================
STATIC SCRIPT DATA STRUCTURES

  Holds parsed information for each scripted character. Each bot that enters the game chooses
  their scripting data from one of the characters below.
=============================
*/
static int numScriptCharacters = 0;

bot_script_global_data_t botCharacterScriptData[MAX_BOT_SCRIPT_CHARACTERS];


/*
===============
Bot_FindSriptGlobalData
===============
*/
int Bot_FindSriptGlobalData( bot_script_data_t *data ) {
	int i;

	for ( i = 0; i < numScriptCharacters; i++ ) {
		if ( botCharacterScriptData[i].data == data ) {
			return i;
		}
	}

	return -1;
}

/*
===============
Bot_EventMatch_StringEqual
===============
*/
qboolean Bot_EventMatch_StringEqual( bot_script_event_t *event, char *eventParm ) {
	if ( !event->params || !event->params[0] || ( eventParm && !Q_stricmp( event->params, eventParm ) ) ) {
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
===============
Bot_EventMatch_IntInRange
===============
*/
qboolean Bot_EventMatch_IntInRange( bot_script_event_t *event, char *eventParm ) {
	char *pString, *token;
	int int1, int2, eInt;

	// get the cast name
	pString = eventParm;
	token = COM_ParseExt( &pString, qfalse );
	int1 = atoi( token );
	token = COM_ParseExt( &pString, qfalse );
	int2 = atoi( token );

	eInt = atoi( event->params );

	if ( eventParm && eInt > int1 && eInt <= int2 ) {
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
===============
Bot_EventForString
===============
*/
int Bot_EventForString( char *string ) {
	int i;

	for ( i = 0; botScriptEvents[i].eventStr; i++ )
	{
		if ( !Q_stricmp( string, botScriptEvents[i].eventStr ) ) {
			return i;
		}
	}

	return -1;
}

/*
===============
Bot_ActionForString
===============
*/
bot_script_stack_action_t *Bot_ActionForString( char *string ) {
	int i;

	for ( i = 0; botScriptActions[i].actionString; i++ )
	{
		if ( !Q_stricmp( string, botScriptActions[i].actionString ) ) {
			if ( !Q_stricmp( string, "foundsecret" ) ) {
				numSecrets++;
			}
			return &botScriptActions[i];
		}
	}

	return NULL;
}

void Bot_ScriptParseAllCharacters();

/*
=============
Bot_ScriptLoad

  Loads the script for the current level into the buffer
=============
*/
void Bot_ScriptLoad( void ) {
/*
	char			filename[MAX_QPATH];
	vmCvar_t		mapname;
	fileHandle_t	f;
	int				len;

	level.botScriptBuffer = NULL;

	trap_Cvar_VariableStringBuffer( "bot_scriptName", filename, sizeof(filename) );
	if (strlen( filename ) > 0) {
		trap_Cvar_Register( &mapname, "bot_scriptName", "", CVAR_ROM );
	} else {
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	}
	Q_strncpyz( filename, "maps/", sizeof(filename) );
	Q_strcat( filename, sizeof(filename), mapname.string );
	Q_strcat( filename, sizeof(filename), ".botscript" );

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	// make sure we clear out the temporary scriptname
	trap_Cvar_Set( "bot_scriptName", "" );

	if (len < 0)
		return;

	level.botScriptBuffer = G_Alloc( len );
	trap_FS_Read( level.botScriptBuffer, len, f );

	trap_FS_FCloseFile( f );

	// now parse the data for each of the characters
	Bot_ScriptParseAllCharacters();
*/
	Bot_ScriptParseAllCharacters();
	return;
}

/*
==============
Bot_ScriptParseAllCharacters
==============
*/
void Bot_ScriptParseAllCharacters() {
	char        *pScript;
	char        *token;
	bot_script_global_data_t *bsd;
	char params[MAX_TOKEN_CHARS];

	if ( !level.scriptEntity ) {
		return;
	}

	pScript = level.scriptEntity;
	COM_BeginParseSession( "Bot_ScriptParse" );
	numScriptCharacters = 0;
	memset( botCharacterScriptData, 0, sizeof( botCharacterScriptData ) );

	while ( 1 )
	{
		token = COM_Parse( &pScript );
		// we are expecting a name here
		if ( !token[0] ) {
			// end of script
			break;
		}
		if ( token[0] == '{' || token[0] == '}' ) {
			G_Error( "Bot_ScriptParse(), Error (line %d): entry identifier expected, '%s' found.\n", 1 + COM_GetCurrentParseLine(), token );
		}
		// is this a bot?
		if ( Q_stricmp( token, "BOT" ) != 0 ) {
			// not a bot, skip this whole entry
			SkipRestOfLine( &pScript );
			// skip this section
			SkipBracedSection( &pScript );
			//
			continue;
		}
		// this is the name
		if ( numScriptCharacters == MAX_BOT_SCRIPT_CHARACTERS ) {
			G_Error( "Bot_ScriptParse(), Error (line %d): MAX_BOT_SCRIPT_CHARACTERS exceeded (%i), too many bot script characters\n", 1 + COM_GetCurrentParseLine(), MAX_BOT_SCRIPT_CHARACTERS );
			break;
		}
		bsd = &botCharacterScriptData[numScriptCharacters++];
		bsd->lineNum = 1 + COM_GetCurrentParseLine();
		// read the name
		token = COM_Parse( &pScript );
		// we are expecting a name here
		if ( !token[0] ) {
			G_Error( "Bot_ScriptParse(), Error (line %d): name expected, end of line found.\n", 1 + COM_GetCurrentParseLine() );
		}
		if ( token[0] == '{' || token[0] == '}' ) {
			G_Error( "Bot_ScriptParse(), Error (line %d): name expected, '%s' found.\n", 1 + COM_GetCurrentParseLine(), token );
		}
		// allocate the name
		bsd->name = G_Alloc( strlen( token ) + 1 );
		Q_strncpyz( bsd->name, token, strlen( token ) + 1 );
		// read the params
		memset( params, 0, sizeof( params ) );
		while ( ( token = COM_ParseExt( &pScript, qfalse ) ) && token[0] ) {
			if ( strlen( params ) + strlen( token ) >= sizeof( params ) ) {
				G_Error( "Bot_ScriptParse(), Error (line %d): parameters exceed maximum size\n", 1 + COM_GetCurrentParseLine() );
			}
			if ( strlen( params ) > 0 ) {
				Q_strcat( params, sizeof( params ), " " );
			}
			Q_strcat( params, sizeof( params ), token );
		}
		// allocate the params
		bsd->params = G_Alloc( strlen( params ) + 1 );
		Q_strncpyz( bsd->params, params, strlen( params ) + 1 );
		// allocate memory for this character script
		bsd->data = G_Alloc( sizeof( bot_script_data_t ) );
		memset( bsd->data, 0, sizeof( bot_script_data_t ) );
		// now parse the script data for this character
		Bot_ScriptParse( bsd->data, &pScript );
	}
}

/*
==============
Bot_ScriptParse

  Parses the script for the given character
==============
*/
void Bot_ScriptParse( bot_script_data_t *bsd, char **text ) {
	char        *token;
	qboolean inScript;
	int eventNum;
	bot_script_event_t events[BOT_MAX_SCRIPT_EVENTS];
	int numEventItems;
	bot_script_event_t *curEvent;
	char params[512];
	//
	bot_script_stack_item_t items[BOT_MAX_SCRIPT_ITEMS];
	int numItems;
	//
	bot_script_stack_action_t   *action;
	int i;
	int bracketLevel;
	int strPoolCount;

	inScript = qfalse;      // not inside the given bot's script

	bracketLevel = 0;
	numEventItems = 0;
	numItems = 0;
	strPoolCount = 0;

	memset( events, 0, sizeof( events ) );
	memset( items, 0, sizeof( items ) );

	while ( 1 )
	{
		token = COM_Parse( text );

		if ( !token[0] ) {
			break;
		}

		// end of script
		if ( token[0] == '}' ) {
			bracketLevel--;
			if ( !bracketLevel ) { // we have already parsed the given bot, so get out of here
				break;
			}
			if ( bracketLevel < 0 ) {
				G_Error( "Bot_ScriptParse(), Error (line %d): '%s' found, name expected\n", 1 + COM_GetCurrentParseLine(), token );
			}
		} else if ( token[0] == '{' )    {
			bracketLevel++;
			if ( bracketLevel > 1 ) {
				G_Error( "Bot_ScriptParse(), Error (line %d): '%s' found, event name expected\n", 1 + COM_GetCurrentParseLine(), token );
			}
		} else if ( bracketLevel == 1 )   {
			eventNum = Bot_EventForString( token );
			if ( eventNum < 0 ) {
				G_Error( "Bot_ScriptParse(), Error (line %d): unknown event: %s.\n", 1 + COM_GetCurrentParseLine(), token );
			}
			if ( numEventItems >= BOT_MAX_SCRIPT_EVENTS ) {
				G_Error( "Bot_ScriptParse(), Error (line %d): BOT_MAX_SCRIPT_EVENTS reached (%d)\n", 1 + COM_GetCurrentParseLine(), BOT_MAX_SCRIPT_EVENTS );
			}

			curEvent = &events[numEventItems];
			curEvent->eventNum = eventNum;
			curEvent->stack.startIndex = numItems;
			memset( params, 0, sizeof( params ) );

			curEvent->lineNum = 1 + COM_GetCurrentParseLine();
			curEvent->text = *text - strlen( token );

			// parse any event params before the start of this event's actions
			while ( ( token = COM_Parse( text ) ) && ( token[0] != '{' ) )
			{
				if ( !token[0] ) {
					G_Error( "Bot_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", 1 + COM_GetCurrentParseLine() );
				}

				if ( strlen( params ) ) { // add a space between each param
					Q_strcat( params, sizeof( params ), " " );
				}
				Q_strcat( params, sizeof( params ), token );
			}

			if ( strlen( params ) ) { // copy the params into the event
				curEvent->params = &bsd->stringPool[strPoolCount];
				Q_strncpyz( curEvent->params, params, BOT_SIZE_STRING_POOL - strPoolCount );
				if ( ( strPoolCount += strlen( params ) + 1 ) >= BOT_SIZE_STRING_POOL ) {
					G_Error( "Bot_ScriptParse(), Error (line %d): string pool size exceeded (MAX = %i)\n", 1 + COM_GetCurrentParseLine(), BOT_SIZE_STRING_POOL );
				}
			}

			// parse the actions for this event
			while ( ( token = COM_Parse( text ) ) && ( token[0] != '}' ) )
			{
				if ( !token[0] ) {
					G_Error( "Bot_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", 1 + COM_GetCurrentParseLine() );
				}

				action = Bot_ActionForString( token );
				if ( !action ) {
					G_Error( "Bot_ScriptParse(), Error (line %d): unknown action: %s.\n", 1 + COM_GetCurrentParseLine(), token );
				}

				items[numItems].action = action;
				items[numItems].lineNum = 1 + COM_GetCurrentParseLine();
				items[numItems].text = *text - strlen( token );

				memset( params, 0, sizeof( params ) );
				token = COM_ParseExt( text, qfalse );
				for ( i = 0; token[0]; i++ )
				{
					if ( strlen( params ) ) { // add a space between each param
						Q_strcat( params, sizeof( params ), " " );
					}

					// Special case: playsound's need to be cached on startup to prevent in-game pauses
					if ( i == 0 ) {
						if ( !Q_stricmp( action->actionString, "playsound" ) ) {
							G_SoundIndex( token );
						}
					}

					if ( strrchr( token,' ' ) ) { // need to wrap this param in quotes since it has more than one word
						Q_strcat( params, sizeof( params ), "\"" );
					}

					Q_strcat( params, sizeof( params ), token );

					if ( strrchr( token,' ' ) ) { // need to wrap this param in quotes since it has more than one word
						Q_strcat( params, sizeof( params ), "\"" );
					}

					token = COM_ParseExt( text, qfalse );
				}

				if ( strlen( params ) ) { // copy the params into the event
					items[numItems].params = &bsd->stringPool[strPoolCount];
					Q_strncpyz( items[numItems].params, params, BOT_SIZE_STRING_POOL - strPoolCount );
					if ( ( strPoolCount += strlen( params ) + 1 ) >= BOT_SIZE_STRING_POOL ) {
						G_Error( "Bot_ScriptParse(), Error (line %d): string pool size exceeded (MAX = %i)\n", 1 + COM_GetCurrentParseLine(), BOT_SIZE_STRING_POOL );
					}
				}

				curEvent->stack.numItems++;
				numItems++;

				if ( numItems >= BOT_MAX_SCRIPT_ITEMS ) {
					G_Error( "Bot_ScriptParse(), Error (line %d): script exceeded BOT_MAX_SCRIPT_ITEMS (%d)\n", 1 + COM_GetCurrentParseLine(), BOT_MAX_SCRIPT_ITEMS );
				}
			}

			numEventItems++;
		} else {
			G_Error( "Bot_ScriptParse(), Error (line %d): '%s' found, '{' expected\n", 1 + COM_GetCurrentParseLine(), token );
		}
	}

	// alloc and copy the events into the structure for this bot
	if ( numEventItems > 0 ) {
		memcpy( bsd->events, events, sizeof( bot_script_event_t ) * numEventItems );
		bsd->numEvents = numEventItems;
		memcpy( bsd->items, items, sizeof( bot_script_stack_item_t ) * numItems );
	}

}

/*
================
Bot_ScriptInitBot
================
*/
qboolean Bot_ScriptInitBot( int entnum ) {
	gentity_t *ent, *trav;
	bot_state_t *bs;
	char userinfo[MAX_INFO_STRING];
	bot_script_global_data_t *bsgd;
	char    *token, *p, *pBackup;
	int i, val = 0;
	int weapons[2];
	gitem_t *item = NULL;
	char    *name;
	//
	bs = &botstates[entnum];
	if ( !bs->inuse ) {
		return qfalse;
	}
	if ( bs->script.data ) {
		return qtrue;
	}
	// set starting defaults
	bs->script.status.eventIndex = -1;
	bs->script.data = NULL;
	//
	ent = BotGetEntity( bs->entitynum );
	trap_GetUserinfo( bs->entitynum, userinfo, sizeof( userinfo ) );
	name = Info_ValueForKey( userinfo, "scriptName" );
	if ( !name || !name[0] ) {
		return qfalse;
	}

	// find the script data for this bot
	bsgd = botCharacterScriptData;
	for ( i = 0; i < numScriptCharacters; i++, bsgd++ ) {
		if ( Q_stricmp( name, bsgd->name ) != 0 ) {
			continue;
		}
		// check params
		p = bsgd->params;
		//
		// eliminate them with each condition not met
		while ( qtrue ) {
			token = COM_ParseExt( &p, qfalse );
			if ( !token || !token[0] ) {
				// we're done, we found a match
				break;
			}
			//
			if ( token[0] != '/' ) {
				G_Error( "BotScript, line %i: condition identifier expected, '%s' found\n", bsgd->lineNum, token );
			}
			//
			if ( !Q_stricmp( token, "/team" ) ) {
				token = COM_ParseExt( &p, qfalse );
				if ( !token || !token[0] || token[0] == '/' ) {
					G_Error( "BotScript, line %i: unexpected end of /team parameter", bsgd->lineNum );
				}
				//
				if ( !Q_stricmp( token, "axis" ) ) {
					val = TEAM_AXIS;
				} else if ( !Q_stricmp( token, "allies" ) ) {
					val = TEAM_ALLIES;
				} else {
					G_Error( "BotScript, line %i: unknown team \"%s\"", bsgd->lineNum, token );
				}
				// eliminate player
				if ( bs->mpTeam != val ) {
					break;
				}
			} else
			//
			if ( !Q_stricmp( token, "/class" ) ) {
				token = COM_ParseExt( &p, qfalse );
				if ( !token || !token[0] || token[0] == '/' ) {
					G_Error( "BotScript, line %i: unexpected end of /class parameter", bsgd->lineNum );
				}
				//
				val = Team_ClassForString( token );
				if ( val < 0 ) {
					G_Error( "BotScript, line %i: unknown class \"%s\"", bsgd->lineNum, token );
				}
				if ( bs->mpClass != val ) {
					break;
				}
			} else
			//
			if ( !Q_stricmp( token, "/weapon" ) ) {
				memset( weapons, 0, sizeof( weapons ) );
				// for each weapon
				while ( qtrue ) {
					// read the weapon
					token = COM_ParseExt( &p, qfalse );
					if ( !token || !token[0] || token[0] == '/' ) {
						G_Error( "BotScript, line %i: unexpected end of /weapon parameter", bsgd->lineNum );
					}
					//
					if ( ( item = BG_FindItem( token ) ) ) {
						if ( !item->giTag ) {
							G_Error( "BotScript, line %i: unknown weapon \"%s\"", bsgd->lineNum, token );
						}
						COM_BitSet( weapons, item->giTag );
					} else {
						G_Error( "BotScript, line %i: unknown weapon \"%s\"", bsgd->lineNum, token );
					}
					//
					pBackup = p;
					token = COM_ParseExt( &p, qfalse );
					if ( Q_stricmp( token, "or" ) != 0 ) {
						// not OR, so drop out of here
						p = pBackup;
						break;
					}
				}
				if ( !( ent->client->ps.weapons[0] & weapons[0] ) && !( ent->client->ps.weapons[1] & weapons[1] ) ) {
					break;
				}
			} else
			//
			if ( !Q_stricmp( token, "/within_range" ) ) {
				// targetname
				token = COM_ParseExt( &p, qfalse );
				if ( !token || !token[0] || token[0] == '/' ) {
					G_Error( "BotScript, line %i: unexpected end of /within_range parameter", bsgd->lineNum );
				}
				trav = G_FindByTargetname( NULL, token );
				if ( !trav ) {
					G_Error( "BotScript, line %i: unknown spawn point \"%s\"", bsgd->lineNum, token );
				}
				// range
				token = COM_ParseExt( &p, qfalse );
				if ( !token || !token[0] || token[0] == '/' ) {
					G_Error( "BotScript, line %i: range expected, not found", bsgd->lineNum );
				}
				//
				// eliminate players
				if ( VectorDistanceSquared( ent->r.currentOrigin, trav->s.origin ) > SQR( atof( token ) ) ) {
					break;
				}
			}
		}
		//
		// if there is a NOT a valid token waiting, then we passed all checks
		if ( !token[0] ) {
			break;
		}
	}
	//
	if ( i < numScriptCharacters ) {
		// we found a script for this character
		bs->script.data = bsgd->data;
		return qtrue;
	}
	//
	return qfalse;
}

/*
================
Bot_LineText
================
*/
char *Bot_LineText( char *text ) {
	static char lineText[MAX_TOKEN_CHARS];
	int len;
	//
	len = strstr( text, "\n" ) - text;
	if ( len <= 0 ) {
		return "";
	}
	if ( len >= sizeof( lineText ) - 1 ) {
		G_Error( "Bot_LineText: max line length exceed (%i)", (int)sizeof( lineText ) );
	}
	//
	memset( lineText, 0, sizeof( lineText ) );
	Q_strncpyz( lineText, text, len );
	//
	return lineText;
}

/*
================
Bot_ScriptChange
================
*/
void Bot_ScriptChange( bot_state_t *bs, int newScriptNum ) {
	bot_script_status_t statusBackup;

	bs->script.callIndex++;

	// backup the current scripting
	statusBackup = bs->script.status;

	// set the new script to this cast, and reset script status
	bs->script.status.stackHead = 0;
	bs->script.status.stackChangeTime = level.time;
	bs->script.status.eventIndex = newScriptNum;
	bs->script.status.id = statusBackup.id + 1;

	// first call
	bs->script.flags |= BSFL_FIRST_CALL;

	Bot_ScriptLog_Entry( bs, qfalse, Bot_LineText( bs->script.data->events[bs->script.status.eventIndex].text ), "** NEW EVENT **\r\n" );

	// try and run the script, if it doesn't finish, then abort the current script (discard backup)
	if ( Bot_ScriptRun( bs, qtrue ) ) {
		// completed successfully
		bs->script.status.stackHead             = statusBackup.stackHead;
		bs->script.status.stackChangeTime       = statusBackup.stackChangeTime;
		bs->script.status.eventIndex            = statusBackup.eventIndex;
		bs->script.status.id                    = statusBackup.id;
		//
		bs->script.flags                        &= ~BSFL_FIRST_CALL;
		// returned to previous event
		if ( statusBackup.eventIndex > -1 ) {
			Bot_ScriptLog_Entry( bs, qfalse, Bot_LineText( bs->script.data->events[statusBackup.eventIndex].text ), "**RESUMED**\r\n" );
		}
	} else {
		// still running, previous script is terminated
		if ( statusBackup.eventIndex > -1 && statusBackup.eventIndex != bs->script.status.eventIndex ) {
			Bot_ScriptLog_Entry( bs, qfalse, Bot_LineText( bs->script.data->events[statusBackup.eventIndex].text ), "**TERMINATED**\r\n" );
		}
	}
}

/*
================
Bot_ScriptEvent

  An event has occured, for which a script may exist
================
*/
void Bot_ScriptEvent( int entityNum, char *eventStr, char *params ) {
	int i, eventNum;
	bot_state_t *bs;

	if ( entityNum < 0 || entityNum >= MAX_CLIENTS ) {
		G_Error( "Bot_ScriptEvent: entityNum out of range (%i)", entityNum );
	}

	bs = &botstates[entityNum];
	if ( !bs->inuse ) {
		return;
	}
	if ( !bs->script.data ) {
		return;
	}

	eventNum = -1;

	// find out which event this is
	for ( i = 0; botScriptEvents[i].eventStr; i++ )
	{
		if ( !Q_stricmp( eventStr, botScriptEvents[i].eventStr ) ) { // match found
			eventNum = i;
			break;
		}
	}

	//
	// show debugging info
	if ( g_scriptDebug.integer ) {
		if ( g_entities[entityNum].scriptName ) {
			G_Printf( "%i : (%s) GScript event: %s %s\n", level.time, g_entities[entityNum].scriptName, eventStr, params ? params : "" );
		} else {
			G_Printf( "%i : (n/a) GScript event: %s %s\n", level.time, eventStr, params ? params : "" );
		}
	}
	//

	if ( eventNum < 0 ) {
		if ( g_cheats.integer ) { // dev mode
			G_Printf( "devmode-> Bot_ScriptEvent(), unknown event: %s\n", eventStr );
		}
	}

	// see if this cast has this event
	for ( i = 0; i < bs->script.data->numEvents; i++ )
	{
		if ( bs->script.data->events[i].eventNum == eventNum ) {
			if (    ( !bs->script.data->events[i].params )
					||  ( !botScriptEvents[eventNum].eventMatch || botScriptEvents[eventNum].eventMatch( &bs->script.data->events[i], params ) ) ) {
				Bot_ScriptChange( bs, i );
				break;
			}
		}
	}
}

/*
================
Bot_ForceScriptEvent

  Definately run this event now, overriding any paused state
================
*/
void Bot_ForceScriptEvent( int entityNum, char *eventStr, char *params ) {
	int oldPauseTime;
	bot_state_t *bs;

	if ( entityNum >= MAX_CLIENTS ) {
		return;
	}

	bs = &botstates[entityNum];
	if ( !bs->inuse ) {
		return;
	}
	if ( !bs->script.data ) {
		return;
	}

	oldPauseTime = bs->script.pauseTime;
	bs->script.pauseTime = 0;

	Bot_ScriptEvent( entityNum, eventStr, params );

	bs->script.pauseTime = oldPauseTime;
}

/*
================
Bot_TeamScriptEvent
================
*/
void Bot_TeamScriptEvent( int team, char *eventStr, char *params ) {
	int i;
	bot_state_t *bs;
	//
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		bs = &botstates[level.sortedClients[i]];
		if ( !bs->inuse ) {
			continue;
		}
		if ( bs->sess.sessionTeam != team ) {
			continue;
		}

		Bot_ScriptEvent( level.sortedClients[i], eventStr, params );
	}
}

/*
=============
Bot_ScriptRun

  returns qtrue if the script completed
=============
*/
qboolean Bot_ScriptRun( bot_state_t *bs, qboolean force ) {
	bot_script_stack_t          *stack;
	bot_script_stack_item_t     *item;
	int oldScriptId;

	if ( !bs->script.data ) {
		return qtrue;
	}

	// turn off flags that get set each frame while they are active
	bs->script.frameFlags = 0;

	if ( bs->script.status.eventIndex < 0 ) {
		return qtrue;
	}

	if ( !bs->script.data->events ) {
		bs->script.status.eventIndex = -1;
		return qtrue;
	}

	if ( !force && ( bs->script.pauseTime >= level.time ) ) {
		return qtrue;
	}

	stack = &bs->script.data->events[bs->script.status.eventIndex].stack;

	if ( !stack->numItems ) {
		bs->script.status.eventIndex = -1;
		return qtrue;
	}

	while ( bs->script.status.stackHead < stack->numItems )
	{
		item = &bs->script.data->items[ stack->startIndex + bs->script.status.stackHead ];
		bs->script.status.currentItem = item;
		//
		if ( bs->script.flags & BSFL_FIRST_CALL ) {
			Bot_ScriptLog_Entry( bs, qtrue, Bot_LineText( bs->script.data->events[bs->script.status.eventIndex].text ), "" );
		}
		//
		oldScriptId = bs->script.status.id;
		//

		// Mad Doc - TDf
		if ( G_IsSinglePlayerGame() ) {
			if ( bot_debug.integer ) {
				trap_SendServerCommand( 0, va( "botdebugprint %i \"Line: %i %s %s\"", bs->client, Bot_Script_GetCurrentLine( bs ), item->action->actionString, item->params ) );
			}
		}

		if ( !item->action->actionFunc( bs, item->params ) ) {
			bs->script.flags &= ~BSFL_FIRST_CALL;
			return qfalse;
		}
		// if our script changed, stop execution
		if ( oldScriptId != bs->script.status.id ) {
			return qfalse;
		}
		// move to the next action in the script
		bs->script.status.stackHead++;
		// record the time that this new item became active
		bs->script.status.stackChangeTime = level.time;
		// reset misc stuff
		bs->script.flags |= BSFL_FIRST_CALL;
	}

	Bot_ScriptLog_Entry( bs, qtrue, Bot_LineText( bs->script.data->events[bs->script.status.eventIndex].text ), "** FINISHED **" );

	bs->script.status.eventIndex = -1;

	return qtrue;
}

/*
=================
Bot_Script_GetCurrentLine
=================
*/
int Bot_Script_GetCurrentLine( bot_state_t *bs ) {
	if ( bs->script.status.eventIndex < 0 ) {
		return -1;
	}
	if ( !bs->script.status.currentItem ) {
		return -1;
	}
	return bs->script.status.currentItem->lineNum;
}

/*
=================
Bot_ScriptLog_Entry
=================
*/
void Bot_ScriptLog_Entry( bot_state_t *bs, qboolean showDetails, char *preText, char *fmt, ... ) {
	va_list ap;
	char text[1024], *pStr, *token;
	fileHandle_t f;
	int i;
	//
	if ( !( f = bs->script.logFile ) ) {
		return;
	}
	//
	// timestamp
	// get the time/date
	Q_strncpyz( text, va( "(%i) ", level.time ), sizeof( text ) );
	trap_FS_Write( text, strlen( text ), f );
	//
	i = 40; // padding for indentation
	// pretext
	if ( preText ) {
		trap_FS_Write( preText, strlen( preText ), f );
		i -= strlen( preText );
		if ( i < 0 ) {
			i = 0;
		}
	}
	// indentation
	while ( i-- ) trap_FS_Write( " ", 1, f );
	//
	if ( showDetails && ( Bot_Script_GetCurrentLine( bs ) > -1 ) ) {
		// show the current script line and text
		Q_strncpyz( text, va( "(line %i:", Bot_Script_GetCurrentLine( bs ) ), sizeof( text ) );
		trap_FS_Write( text, strlen( text ), f );
		// text
		pStr = bs->script.status.currentItem->text;
		while ( ( token = COM_ParseExt( &pStr, qfalse ) ) && token[0] ) {
			trap_FS_Write( " ", 1, f );
			trap_FS_Write( token, strlen( token ), f );
		}
		trap_FS_Write( ") ", 2, f );
	}
	//
	if ( fmt ) {
		va_start( ap, fmt );
		Q_vsnprintf( text, sizeof( text ), fmt, ap );
		if ( strlen( text ) >= sizeof( text ) ) {
			//G_Error( "Bot_ScriptLog_Entry: text exceeded buffer size" );
			// just cut it short
			text[sizeof( text ) - 1] = '\0';
		}
		va_end( ap );
		//
		trap_FS_Write( text, strlen( text ), f );
	}
	trap_FS_Write( "\r\n", 2, f );
}

/*
================
Bot_ScriptThink
================
*/
void Bot_ScriptThink( void ) {
	int i;
	bot_state_t *bs;

	for ( i = 0; i < level.maxclients; i++ ) {
		// get the bot for this entity num
		bs = &botstates[i];

		// if there's no bot here, skip it
		if ( !bs->inuse ) {
			continue;
		}

		// if the bot is dead, skip it
		if ( BotIsDead( bs ) ) {
			continue;
		}

		Bot_ScriptRun( bs, qfalse );
	}
}
