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
// Name:			ai_script_actions.c
// Function:		Wolfenstein Bot Scripting
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../qcommon/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "inv.h"             //indexes into the inventory

#include "ai_main.h"
#include "ai_dmq3.h"

/*
Contains the code to handle the various commands available with an event script.

These functions will return true if the action has been performed, and the script
should proceed to the next item on the list.
*/

qboolean Bot_ScriptAction_SetSpeed( bot_state_t* bs, char *params ) {
	vec3_t speed;
	char* pString;
	int i;
	char* token;

	pString = params;
	for ( i = 0; i < 3; i++ ) {
		token = COM_Parse( &pString );
		if ( !token || !*token ) {
			G_Error( "G_Scripting: syntax: setspeed <x> <y> <z>\n" );
		}
		speed[i] = atoi( token );
	}

	VectorAdd( g_entities[bs->entitynum].client->ps.velocity, speed, g_entities[bs->entitynum].client->ps.velocity );

	return qtrue;
}

/*
=================
Bot_ScriptError
=================
*/
void Bot_ScriptError( bot_state_t *bs, char *fmt, ... ) {
	va_list ap;
	char text[512];
	//
	va_start( ap, fmt );
	Q_vsnprintf( text, sizeof( text ), fmt, ap );
	if ( strlen( text ) >= sizeof( text ) ) {
		text[sizeof( text ) - 1] = '\0';
	}
	//
	G_Error( "BotScript (line %i): %s", bs->script.status.currentItem->lineNum, text );
	//
	va_end( ap );
}

/*
=================
Bot_ScriptAction_Print

  syntax: print <text>

  Mostly for debugging purposes
=================
*/
qboolean Bot_ScriptAction_Print( bot_state_t *bs, char *params ) {
	char *pString, *token, *printThis;
	int printLevel = 0;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "print requires some text" );
	}

	// Start parsing at the beginning of the string
	pString = params;

	// Default to printing whole string
	printThis = params;

	// See if the first parameter is a /N, where N is a number
	if ( ( token = COM_ParseExt( &pString, qfalse ) ) && token[0] == '/' ) {
		// Get the integer version of the print debug level
		printLevel = atoi( &( token[1] ) );

		// Just print what's left
		printThis = pString;

	}

	// Only print if our debug level is as high as the print level
	if ( g_scriptDebugLevel.integer >= printLevel ) {
		// Print the statement
		G_Printf( "(BotScript) %s-> %s\n", g_entities[bs->entitynum].client->pers.netname, printThis );

	} // if (g_scriptDebugLevel.integer >= printLevel)...

	return qtrue;
}

/*
=================
Bot_ScriptAction_SetAccumToPlayerCount

  syntax: SetAccumToPlayerCount <accum_index> [[<condition> <value>] [<condition> <value>] ...]

  condition list: team [axis/allies], class [medic/engineer/etc], weapon [flamethrower/sniperrifle/etc], within_range <targetname> <distance>
=================
*/
qboolean Bot_ScriptAction_SetAccumToPlayerCount( bot_state_t *bs, char *params ) {
	char    *pStr, *pStrBackup, *token;
	int count, i, val, accum, weapons[2];
	gitem_t *item = NULL;
	gentity_t   *ent;
	byte validPlayers[MAX_CLIENTS];

	// setup the list of starting validPlayers
	memset( validPlayers, 0, sizeof( validPlayers ) );
	count = 0;
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( !g_entities[i].inuse ) {
			continue;
		}
		if ( !g_entities[i].client ) {
			continue;
		}
		if ( !g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		validPlayers[i] = 1;
		count++;
	}

	pStr = params;

	token = COM_ParseExt( &pStr, qfalse );
	if ( !token || !token[0] || ( token[0] < '0' ) || ( token[0] > '9' ) ) {
		Bot_ScriptError( bs, "accum buffer index expected, %s found: SetAccumToPlayerCount %s", token, params );
	}
	accum = atoi( token );
	if ( accum < 0 || accum >= MAX_SCRIPT_ACCUM_BUFFERS ) {
		Bot_ScriptError( bs, "accum buffer index out of range, %s found (range is 0 - %i): SetAccumToPlayerCount %s", token, MAX_SCRIPT_ACCUM_BUFFERS - 1, params );
	}

	// eliminate them with each condition not met
	while ( qtrue ) {
		val = 0;
		//
		token = COM_ParseExt( &pStr, qfalse );
		if ( !token || !token[0] ) {
			// we're done
			break;
		}
		//
		if ( token[0] != '/' ) {
			Bot_ScriptError( bs, "condition identifier expected, %s found: SetAccumToPlayerCount %s", token, params );
		}
		//
		if ( !Q_stricmp( token, "/team" ) ) {
			token = COM_ParseExt( &pStr, qfalse );
			if ( !token || !token[0] || token[0] == '/' ) {
				Bot_ScriptError( bs, "unexpected end of command: SetAccumToPlayerCount %s", params );
			}
			//
			if ( !Q_stricmp( token, "axis" ) ) {
				val = TEAM_AXIS;
			} else if ( !Q_stricmp( token, "allies" ) ) {
				val = TEAM_ALLIES;
			} else {
				Bot_ScriptError( bs, "unknown team \"%s\": SetAccumToPlayerCount %s", token, params );
			}
			// eliminate players
			for ( i = 0; i < level.maxclients; i++ ) {
				if ( !validPlayers[i] ) {
					continue;
				}
				if ( g_entities[i].client->sess.sessionTeam != val ) {
					validPlayers[i] = 0;
					count--;
				}
			}
		} else
		//
		if ( !Q_stricmp( token, "/class" ) ) {
			token = COM_ParseExt( &pStr, qfalse );
			if ( !token || !token[0] || token[0] == '/' ) {
				Bot_ScriptError( bs, "unexpected end of command: SetAccumToPlayerCount %s", params );
			}
			//
			val = Team_ClassForString( token );
			if ( val < 0 ) {
				Bot_ScriptError( bs, "unknown class \"%s\": SetAccumToPlayerCount %s", token, params );
			}
			// eliminate players
			for ( i = 0; i < level.maxclients; i++ ) {
				if ( !validPlayers[i] ) {
					continue;
				}
				if ( g_entities[i].client->sess.playerType != val ) {
					validPlayers[i] = 0;
					count--;
				}
			}
		} else
		//
		if ( !Q_stricmp( token, "/weapon" ) ) {
			memset( weapons, 0, sizeof( weapons ) );
			// for each weapon
			while ( qtrue ) {
				// read the weapon
				token = COM_ParseExt( &pStr, qfalse );
				if ( !token || !token[0] || token[0] == '/' ) {
					Bot_ScriptError( bs, "unexpected end of command: SetAccumToPlayerCount %s", params );
				}
				//
				if ( ( item = BG_FindItem( token ) ) ) {
					if ( !item->giTag ) {
						Bot_ScriptError( bs, "unknown weapon \"%s\": SetAccumToPlayerCount %s", token, params );
					}
					COM_BitSet( weapons, item->giTag );
				} else {
					Bot_ScriptError( bs, "unknown weapon \"%s\": SetAccumToPlayerCount %s", token, params );
				}
				//
				pStrBackup = pStr;
				token = COM_ParseExt( &pStr, qfalse );
				if ( !token[0] || ( Q_stricmp( token, "or" ) != 0 ) ) {
					// not OR, so drop out of here
					pStr = pStrBackup;
					break;
				}
			}
			// eliminate players
			for ( i = 0; i < level.maxclients; i++ ) {
				if ( !validPlayers[i] ) {
					continue;
				}
				if ( !( g_entities[i].client->ps.weapons[0] & weapons[0] ) && !( g_entities[i].client->ps.weapons[1] & weapons[1] ) ) {
					validPlayers[i] = 0;
					count--;
				}
			}
		} else
		//
		if ( !Q_stricmp( token, "/within_range" ) ) {
			// targetname
			token = COM_ParseExt( &pStr, qfalse );
			if ( !token || !token[0] || token[0] == '/' ) {
				Bot_ScriptError( bs, "unexpected end of command: SetAccumToPlayerCount %s", params );
			}
			ent = G_FindByTargetname( NULL, token );
			if ( !ent ) {
				Bot_ScriptError( bs, "unknown spawn point \"%s\": SetAccumToPlayerCount %s", token, params );
			}
			// range
			token = COM_ParseExt( &pStr, qfalse );
			if ( !token || !token[0] || token[0] == '/' ) {
				Bot_ScriptError( bs, "range expected, not found: SetAccumToPlayerCount %s", params );
			}
			//
			// eliminate players
			for ( i = 0; i < level.maxclients; i++ ) {
				if ( !validPlayers[i] ) {
					continue;
				}
				if ( VectorDistanceSquared( g_entities[i].r.currentOrigin, ent->s.origin ) > SQR( atof( token ) ) ) {
					validPlayers[i] = 0;
					count--;
				}
			}
		}
	}
	//
	bs->script.accumBuffer[accum] = count;
	//
	return qtrue;
}

/*
======================
Bot_ScriptAction_SpawnBot

  see Svcmd_SpawnBot()
======================
*/
qboolean Bot_ScriptAction_SpawnBot( bot_state_t *bs, char *params ) {
	//trap_SendConsoleCommand( EXEC_APPEND, va("spawnbot %s\n", params) );
	G_SpawnBot( params );
	return qtrue;
}

/*
=================
Bot_ScriptAction_Accum

  syntax: accum <buffer_index> <command> <paramater>

  Commands:

	accum <n> inc <m>
	accum <n> abort_if_less_than <m>
	accum <n> abort_if_greater_than <m>
	accum <n> abort_if_not_equal <m>
	accum <n> abort_if_equal <m>
	accum <n> set_to <m>
	accum <n> random <m>
	accum <n> bitset <m>
	accum <n> bitclear <m>
	accum <n> abort_if_bitset <m>
	accum <n> abort_if_not_bitset <m>
=================
*/

qboolean Bot_ScriptAction_Trigger( bot_state_t *bs, char *params );

qboolean Bot_ScriptAction_Accum( bot_state_t *bs, char *params ) {
	char *pString, *token, lastToken[MAX_QPATH];
	int bufferIndex;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "accum: without a buffer index" );
	}

	bufferIndex = atoi( token );
	if ( bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS ) {
		Bot_ScriptError( bs, "accum: buffer is outside range (0 - %i)", MAX_SCRIPT_ACCUM_BUFFERS );
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "accum: without a command" );
	}

	Q_strncpyz( lastToken, token, sizeof( lastToken ) );
	token = COM_ParseExt( &pString, qfalse );

	if ( !Q_stricmp( lastToken, "inc" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum:: %s requires a parameter", lastToken );
		}
		bs->script.accumBuffer[bufferIndex] += atoi( token );
	} else if ( !Q_stricmp( lastToken, "abort_if_less_than" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( bs->script.accumBuffer[bufferIndex] < atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_greater_than" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( bs->script.accumBuffer[bufferIndex] > atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_equal" ) || !Q_stricmp( lastToken, "abort_if_not_equals" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( bs->script.accumBuffer[bufferIndex] != atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_equal" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( bs->script.accumBuffer[bufferIndex] == atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		bs->script.accumBuffer[bufferIndex] |= ( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "bitclear" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		bs->script.accumBuffer[bufferIndex] &= ~( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "abort_if_bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( bs->script.accumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( !( bs->script.accumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "set_to" ) || !Q_stricmp( lastToken, "set" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		bs->script.accumBuffer[bufferIndex] = atoi( token );
	} else if ( !Q_stricmp( lastToken, "random" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		bs->script.accumBuffer[bufferIndex] = rand() % atoi( token );
	} else if ( !Q_stricmp( lastToken, "trigger_if_equal" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "accum: %s requires a parameter", lastToken );
		}
		if ( bs->script.accumBuffer[bufferIndex] == atoi( token ) ) {
			return Bot_ScriptAction_Trigger( bs, pString );
		}
	} else {
		Bot_ScriptError( bs, "accum: %s: unknown command", params );
	}

	return qtrue;
}

/*
======================
Bot_ScriptAction_Wait
======================
*/
qboolean Bot_ScriptAction_Wait( bot_state_t *bs, char *params ) {
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "Wait requires a duration." );
	}
	if ( !atoi( params ) ) {
		Bot_ScriptError( bs, "Wait has invalid duration." );
	}
	//
	// Gordon: check <= 0 instead of == 0?
	return ( bs->script.status.stackChangeTime < level.time - atoi( params ) );
}

/*
======================
Bot_ScriptAction_MoveToMarker
======================
*/
qboolean Bot_ScriptAction_MoveToMarker( bot_state_t *bs, char *params ) {
	char *pString, *token;
	g_serverEntity_t *target;
	vec3_t vec;

	// Gordon: 24/10/02
	float radius = 24;


	// TAT 11/14/2002 - This causes a crash if you have guys on patrol routes
	//		Why did we need it?
	// Gordon: 27/11/02: dont want to teleport dead players, can move further down,
	// but i see no reason a general early out wouldn't be as good
	// why does it crash?
	/*
	// Gordon: 6/11/02
	// cant move if we're dead...
	if(g_entities[bs->entitynum].health <= 0) {
		return qtrue;
	}
	*/

	// TAT 8/20/2002
	//		If we have received a player command that overrides our movement scripts,
	//		we should end the script
	if ( bs->overrideMovementScripts ) {
		// We're done with this script
		return qtrue;
//		return qfalse;
	}

	// Did this move fail?
	if ( bs->movementFailedBadly ) {
//		Bot_ScriptError( bs, "MoveToMarker failed." );

		// Cancel the move
		bs->movementFailedBadly = qfalse;

		// We're done here
		return qtrue;

	} // if (bs->movementFailedBadly)...

	//
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "MoveToMarker requires a targetname." );
	}
	//
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	target = FindServerEntity( NULL, SE_FOFS( name ), token );
	if ( !target ) {
		Bot_ScriptError( bs, "MoveToMarker has unknown targetname: \"%s\"", token );
	}
	//
	bs->script.frameFlags |= BSFFL_MOVETOTARGET;
	bs->script.entityNum = target->number;
	bs->script.moveType = BSMT_DEFAULT;
	//
	while ( ( token = COM_ParseExt( &pString, qfalse ) ) && token[0] ) {
		if ( !Q_stricmp( token, "/WALKING" ) ) {
			bs->script.moveType = BSMT_WALKING;
		} else if ( !Q_stricmp( token, "/CROUCHING" ) ) {
			bs->script.moveType = BSMT_CROUCHING;
		} else if ( !Q_stricmp( token, "/DIRECT" ) ) {
			bs->script.frameFlags |= BSFFL_DIRECTMOVE;
			// Gordon: 24/10/02 Adding ability to set custom radius for movement goal
		} else if ( !Q_stricmp( token, "radius" ) ) {
			token = COM_ParseExt( &pString, qfalse );
			if ( !*token ) {
				Bot_ScriptError( bs, "MoveToMarker with radius has no value" );
			} else {
				radius = atof( token );
			}
			// Gordon: 24/10/02 Adding ability to teleport a bot somewhere
		} else if ( !Q_stricmp( token, "instant" ) ) {
			TeleportPlayer( &g_entities[bs->entitynum], target->origin, g_entities[bs->entitynum].client->ps.viewangles );
			return qtrue;
			// Gordon: end
		}
	}
	// START	Mad Doctor I changes, 8/19/2002
	// The check was too constrictive and failed too often.  I may have made it too loose, but it seems to work.
	// if we have passed or are close enough to the marker, then return qtrue
	// Gordon: added custom radius ability, default is 24 as Ian had set here
	if ( VectorDistanceSquared( bs->origin, target->origin ) < SQR( radius ) ) {
//	if (VectorDistanceSquared( bs->origin, target->s.origin ) < SQR(12)) {
		// END		Mad Doctor I changes, 8/19/2002
		return qtrue;
	} else if ( ( bs->script.status.stackChangeTime < level.time - 500 ) && VectorDistanceSquared( bs->origin, target->origin ) < SQR( 48 ) ) {
		VectorSubtract( target->origin, bs->origin, vec );
		if ( DotProduct( bs->cur_ps.velocity, vec ) < 0 ) {
			return qtrue;
		}
	}
	//
	return qfalse;
}

/*
=====================
Bot_ScriptAction_Trigger
=====================
*/
qboolean Bot_ScriptAction_Trigger( bot_state_t *bs, char *params ) {
	gentity_t *trent, *ent;
	char *pString, name[MAX_QPATH], trigger[MAX_QPATH], *token;
	int oldId, i;
	qboolean terminate, found;

	// get the cast name
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( name, token, sizeof( name ) );
	if ( !name[0] ) {
		G_Error( "G_Scripting: trigger must have a name and an identifier\n" );
	}

	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( trigger, token, sizeof( trigger ) );
	if ( !trigger[0] ) {
		G_Error( "G_Scripting: trigger must have a name and an identifier\n" );
	}

	ent = BotGetEntity( bs->client );

	// START	Mad Doctor I changes, 8/14/2002
	// Changes to fix bugs caused if you used "trigger Foo FooAction" within bot Foo's
	// scripts instead of using "trigger self FooAction".
	if ( ( !Q_stricmp( name, "self" ) ) || ( !Q_stricmp( name, ent->scriptName ) ) ) {
		trent = ent;
		oldId = bs->script.status.id;
		Bot_ScriptEvent( bs->client, "trigger", trigger );

		// See if we've popped back to the original script
		return ( oldId == bs->script.status.id );
//		return (oldId == trent->scriptStatus.scriptId);
		// END		Mad Doctor I changes, 8/14/2002
	} else if ( !Q_stricmp( name, "global" ) ) {
		terminate = qfalse;
		found = qfalse;
		// for all entities/bots
		trent = g_entities;
		for ( i = 0; i < level.num_entities; i++, trent++ ) {
			if ( !trent->inuse ) {
				continue;
			}
			if ( !trent->scriptName ) {
				continue;
			}
			if ( !trent->scriptName[0] ) {
				continue;
			}
			found = qtrue;
			if ( !( trent->r.svFlags & SVF_BOT ) ) {
				G_Script_ScriptEvent( trent, "trigger", trigger );
			} else {
				oldId = bs->script.status.id;
				Bot_ScriptEvent( bs->client, "trigger", trigger );
				// if the script changed, return false so we don't muck with it's variables
				if ( ( trent == ent ) && ( oldId != bs->script.status.id ) ) {
					terminate = qtrue;
				}
			}
		}
		//
		if ( terminate ) {
			return qfalse;
		}
		if ( found ) {
			return qtrue;
		}
	} else {
		terminate = qfalse;
		found = qfalse;
		// for all entities/bots with this scriptName
		trent = NULL;
		while ( ( trent = BotFindEntity( trent, FOFS( scriptName ), name ) ) ) {
			found = qtrue;
			if ( !( trent->r.svFlags & SVF_BOT ) ) {
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, "trigger", trigger );
				// if the script changed, return false so we don't muck with it's variables
				if ( ( trent == ent ) && ( oldId != trent->scriptStatus.scriptId ) ) {
					terminate = qtrue;
				}
			} else {
				Bot_ScriptEvent( trent->s.number, "trigger", trigger );
			}
		}
		//
		if ( terminate ) {
			return qfalse;
		}

		// Did we find a bot?
		if ( found ) {

			// We found one, and triggered the action, so keep going on with our script
			return qtrue;

		} // if (found) ...


	} // else...

//	G_Error( "G_Scripting: trigger has unknown name: %s\n", name );
	G_Printf( "G_Scripting: trigger has unknown name: %s\n", name );
	return qfalse;  // shutup the compiler
}

/*
=====================
Bot_ScriptAction_Logging
=====================
*/
qboolean Bot_ScriptAction_Logging( bot_state_t *bs, char *params ) {
	struct tm *localTime;
	time_t long_time;
	char filename[MAX_QPATH];

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "Logging requires an ON/OFF" );
	}
	if ( !Q_stricmp( params, "ON" ) ) {
		if ( bs->script.flags & BSFL_LOGGING ) {
			// logging already started
			return qtrue;
		}
		bs->script.flags |= BSFL_LOGGING;
		// get the time/date
		time( &long_time );
		localTime = localtime( &long_time );
		Q_strncpyz( filename, va( "BotLog_%s_[%i]_[%4i_%2i_%2i]_[%2i_%2i_%2i].txt", g_entities[bs->client].aiName, bs->client, 1900 + localTime->tm_year, 1 + localTime->tm_mon, 1 + localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec ), sizeof( filename ) );
		// open the log file
		if ( trap_FS_FOpenFile(  filename, &bs->script.logFile, FS_APPEND ) < 0 ) {
			Bot_ScriptError( bs, "Cannot open file for logging: %s", filename );
		}
	} else if ( !Q_stricmp( params, "OFF" ) )      {
		if ( !( bs->script.flags & BSFL_LOGGING ) ) {
			// logging already started
			return qtrue;
		}
		bs->script.flags &= ~BSFL_LOGGING;
		trap_FS_FCloseFile( bs->script.logFile );
		bs->script.logFile = 0;
	} else {
		Bot_ScriptError( bs, "Logging has unknown parameter (%s), expected ON/OFF", params );
	}

	return qtrue;
}

/*
=====================
Bot_ScriptAction_AbortIfWarmup
=====================
*/
qboolean Bot_ScriptAction_AbortIfWarmup( bot_state_t *bs, char *params ) {
	if ( level.warmupTime ) {
		// abort the current script
		bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
	}
	//
	return qtrue;
}

char *botAttributeStrings[] =
{



	"BOT_REACTION_TIME",
	"BOT_AIM_ACCURACY",





	"BOT_WIMP_FACTOR",
	NULL
};

/*
=============
Bot_ScriptAction_SetAttribute

  returns qfalse if error
=============
*/
qboolean Bot_ScriptAction_SetAttribute( bot_state_t *bs, char *params ) {
	int i;
	char *pString, *token;

	// get the attribString
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "attribute string required" );
	}

	for ( i = 0; botAttributeStrings[i]; i++ ) {
		if ( !Q_stricmp( botAttributeStrings[i], token ) ) {
			token = COM_ParseExt( &pString, qfalse );
			if ( !token[0] ) {
				Bot_ScriptError( bs, "attribute value required" );
			}
			bs->attribs[i] = atof( token );
			return qtrue;
		}
	}

	// Let's give a big old error right here!
	Bot_ScriptError( bs, "SetAttribute: Invalid attribute %s.", token );

	return qfalse;
}

/*
===============
Bot_ScriptAction_MountMG42
===============
*/
qboolean Bot_ScriptAction_MountMG42( bot_state_t *bs, char *params ) {
	gentity_t *mg42, *mg42Spot;
	//
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "MountMG42 requires a targetname" );
	}
	// find the mg42
	mg42 = NULL;
	while ( ( mg42 = BotFindNextStaticEntity( mg42, BOTSTATICENTITY_MG42 ) ) ) {
		if ( !Q_stricmp( mg42->targetname, params ) ) {
			break;
		}
	}
	//
	if ( !mg42 ) {
		Bot_ScriptError( bs, "MountMG42: targetname \"%s\" not found", params );
	}
	//
	mg42Spot = mg42->melee;
	//
	if ( !mg42Spot ) {
		Bot_ScriptError( bs, "MountMG42: (internal error) mg42 (\"%s\") has invalid mg42_spot", params );
	}
	//
	bs->script.flags |= BSFL_MOUNT_MG42;
	bs->script.mg42entnum = mg42Spot->s.number;
	//
	return qtrue;
}

// START	Mad Doctor I changes, 8/19/2002

/*
===============
Bot_ScriptAction_PlaySoundAtPlayer


===============
*/
qboolean Bot_ScriptAction_PlaySoundAtPlayer( bot_state_t *bs, char *params ) {
	// We need to find out who the player is
	gentity_t *player = NULL;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "PlaySound requires a soundname" );
	}

	// Look up the player entity by name
	// NOTE: This would need adjusting for COOP play.
	player = BotFindEntityForName( "player" );

	// Bail if no player exists
	if ( player == NULL ) {
		return qtrue;
	}

	//
	G_AddEvent( player, EV_GENERAL_SOUND, G_SoundIndex( params ) );
	//
	return qtrue;

} // qboolean Bot_ScriptAction_PlaySoundAtPlayer( bot_state_t *bs, char *params )...

// END		Mad Doctor I changes, 8/19/2002


extern qboolean AddWeaponToPlayer( gclient_t *client, weapon_t weapon, int ammo, int ammoclip, qboolean setcurrent );

/*
===============
Bot_ScriptAction_SetWeapon
===============
*/
qboolean Bot_ScriptAction_SetWeapon( bot_state_t *bs, char *params ) {
	char userinfo[MAX_INFO_STRING];

	// Which class are we?
	int playerClass = g_entities[bs->client].client->sess.playerType;

	int weapon;
	//
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetWeapon requires a weapon name" );
	}


	weapon = Bot_GetWeaponForClassAndTeam( playerClass, g_entities[bs->client].client->sess.sessionTeam, params );
	if ( weapon == -1 ) {
		// can't use this weapon
		Bot_ScriptError( bs, "Bot %s on team %s cannot use weapon %s\n", g_entities[bs->client].aiName, ( g_entities[bs->client].client->sess.sessionTeam == TEAM_AXIS ) ? "Axis" : "Allies", params );
	}

	if ( weapon == WP_NONE ) {
		trap_GetUserinfo( bs->client, userinfo, sizeof( userinfo ) );
		Info_SetValueForKey( userinfo, "pWeapon", "NONE" );
		trap_SetUserinfo( bs->client, userinfo );
		ClientUserinfoChanged( bs->client );
		// TAT 12/26/2002 - make sure the bot state knows we have no weapon
		bs->weaponnum = WP_NONE;
	} else
	{
		gentity_t *player;
		int i;
		// make sure we tell all the clients that we have a weapon
		for ( i = 0; i < level.numConnectedClients; i++ )
		{
			// send the noweapon command with who as the 1st param, and 0 meaning we don't have no weapon
			player = g_entities + level.sortedClients[i];
			if ( player->inuse && player->client->sess.sessionTeam == bs->mpTeam ) {
				trap_SendServerCommand( player->s.number, va( "nwp %i 0", bs->client ) );
			}
		}
/*
		trap_GetUserinfo( bs->client, userinfo, sizeof(userinfo) );
		Info_SetValueForKey( userinfo, "pWeapon", va("%i", weapon));
		trap_SetUserinfo( bs->client, userinfo );
		ClientUserinfoChanged(bs->client);
*/

	}

	// set the weapon
	g_entities[bs->client].client->sess.playerWeapon = weapon;
	g_entities[bs->client].client->ps.weapon = weapon;
	g_entities[bs->client].s.weapon = weapon;

	// use this new func: don't bother with the whole weaponSpawnNumber thing
	SetWolfSpawnWeapons( g_entities[bs->client].client );

	if ( weapon != WP_NONE ) {
//		AddWeaponToPlayer( g_entities[bs->client].client, weapon, 1, 0, qtrue );

		// Make sure this weapon is allowed
		COM_BitSet( g_entities[bs->client].client->ps.weapons, weapon );

		//@TODO set ammo with an extra toekn
//		client->ps.ammoclip[BG_FindClipForWeapon(weapon)] = ammoclip;
//		client->ps.ammo[BG_FindAmmoForWeapon(weapon)] = ammo;

		// Make this the current weapon
		g_entities[bs->client].client->ps.weapon = weapon;

	} // if (weapon != WP_NONE)...

	return qtrue;
}

/*
===============
Bot_ScriptAction_SetClass
===============
*/
qboolean Bot_ScriptAction_SetClass( bot_state_t *bs, char *params ) {
	int val = -1;
	char userinfo[MAX_INFO_STRING];
	//
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetClass requires a class name" );
	}
	//
	if ( !Q_stricmp( params, "ANY" ) ) {
		val = -1;
	} else if ( !Q_stricmp( params, "soldier" ) ) {
		val = PC_SOLDIER;
	} else if ( !Q_stricmp( params, "medic" ) ) {
		val = PC_MEDIC;
	} else if ( !Q_stricmp( params, "engineer" ) ) {
		val = PC_ENGINEER;
	} else if ( !Q_stricmp( params, "lieutenant" ) ) { // FIXME: remove this from missionpack? once all scripts have been updated
		val = PC_FIELDOPS;
	} else if ( !Q_stricmp( params, "fieldops" ) ) {
		val = PC_FIELDOPS;
	} else if ( !Q_stricmp( params, "covertops" ) ) {
		val = PC_COVERTOPS;
	} else {
		Bot_ScriptError( bs, "unknown class \"%s\"", params );
	}
	//
	trap_GetUserinfo( bs->client, userinfo, sizeof( userinfo ) );
	Info_SetValueForKey( userinfo, "pClass", va( "%i", val ) );
	trap_SetUserinfo( bs->client, userinfo );
	//
	return qtrue;
}

/*
=================
Bot_ScriptAction_GlobalAccum

  syntax: globalAccum <buffer_index> <command> <paramater>

  Commands:

	globalAccum <n> inc <m>
	globalAccum <n> abort_if_less_than <m>
	globalAccum <n> abort_if_greater_than <m>
	globalAccum <n> abort_if_not_equal <m>
	globalAccum <n> abort_if_equal <m>
	globalAccum <n> set_to <m>
	globalAccum <n> random <m>
	globalAccum <n> bitset <m>
	globalAccum <n> bitclear <m>
	globalAccum <n> abort_if_bitset <m>
	globalAccum <n> abort_if_not_bitset <m>
=================
*/
qboolean Bot_ScriptAction_GlobalAccum( bot_state_t *bs, char *params ) {
	char *pString, *token, lastToken[MAX_QPATH];
	int bufferIndex;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "globalAccum: without a buffer index" );
	}

	bufferIndex = atoi( token );
	if ( bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS ) {
		Bot_ScriptError( bs, "globalAccum: buffer is outside range (0 - %i)", MAX_SCRIPT_ACCUM_BUFFERS );
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "globalAccum: without a command" );
	}

	Q_strncpyz( lastToken, token, sizeof( lastToken ) );
	token = COM_ParseExt( &pString, qfalse );

	if ( !Q_stricmp( lastToken, "inc" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum:: %s requires a parameter", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] += atoi( token );
	} else if ( !Q_stricmp( lastToken, "abort_if_less_than" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		if ( level.globalAccumBuffer[bufferIndex] < atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_greater_than" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		if ( level.globalAccumBuffer[bufferIndex] > atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_equal" ) || !Q_stricmp( lastToken, "abort_if_not_equals" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		if ( level.globalAccumBuffer[bufferIndex] != atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_equal" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		if ( level.globalAccumBuffer[bufferIndex] == atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] |= ( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "bitclear" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] &= ~( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "abort_if_bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		if ( level.globalAccumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		if ( !( level.globalAccumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "set_to" ) || !Q_stricmp( lastToken, "set" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] = atoi( token );
	} else if ( !Q_stricmp( lastToken, "random" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "globalAccum: %s requires a parameter", lastToken );
		}
		level.globalAccumBuffer[bufferIndex] = rand() % atoi( token );
	} else {
		Bot_ScriptError( bs, "globalAccum: %s: unknown command", params );
	}

	return qtrue;
}

/*
=================
Bot_ScriptAction_FollowLeader
=================
*/
qboolean Bot_ScriptAction_FollowLeader( bot_state_t *bs, char *params ) {
	char *pString, *token;
	gentity_t   *target;
//	vec3_t	vec;
	int duration;
	//
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "FollowLeader requires a name." );
	}

	// TAT 8/20/2002
	//		If we have received a player command that overrides our movement scripts,
	//		we should end the script
	if ( bs->overrideMovementScripts ) {
		return qfalse;
	}

	//
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	target = BotFindEntityForName( token );
	if ( !target ) {
		if ( bs->script.status.stackChangeTime != level.time ) {
			// Gordon: lets assume they have died...
			return qtrue;
		}
		Bot_ScriptError( bs, "FollowLeader has unknown name: \"%s\"", token );
	}

	if ( target->health <= 0 ) {
		// Gordon: dead, dont follow
		return qtrue;
	}
	// read the duration
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "FollowLeader requires a duration" );
	}
	if ( !Q_stricmp( token, "forever" ) ) {
		duration = (int)0x7fffffff;
	} else {
		duration = atoi( token );
	}
	//
	bs->script.frameFlags |= BSFFL_FOLLOW_LEADER;
	bs->script.entityNum = target->s.number;
	bs->script.moveType = BSMT_DEFAULT;
	//
	while ( ( token = COM_ParseExt( &pString, qfalse ) ) && token[0] ) {
		if ( !Q_stricmp( token, "/WALKING" ) ) {
			bs->script.moveType = BSMT_WALKING;
		} else if ( !Q_stricmp( token, "/CROUCHING" ) ) {
			bs->script.moveType = BSMT_CROUCHING;
		}
	}
	//
	return ( bs->script.status.stackChangeTime < level.time - duration );
}

/*
===================
Bot_ScriptAction_Cvar

  syntax: cvar <cvarName> <operation> <value>
===================
*/
qboolean Bot_ScriptAction_Cvar( bot_state_t *bs, char *params ) {
	char *pString, *token, lastToken[MAX_QPATH], name[MAX_QPATH], cvarName[MAX_QPATH];
	int cvarValue;
	qboolean terminate, found;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "cvar without a cvar name\n" );
	}

	cvarValue = trap_Cvar_VariableIntegerValue( cvarName );

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "cvar without a command\n" );
	}

	Q_strncpyz( lastToken, token, sizeof( lastToken ) );
	token = COM_ParseExt( &pString, qfalse );

	if ( !Q_stricmp( lastToken, "inc" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		trap_Cvar_Set( cvarName, va( "%i", cvarValue + 1 ) );
	} else if ( !Q_stricmp( lastToken, "abort_if_less_than" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue < atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_greater_than" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue > atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_equal" ) || !Q_stricmp( lastToken, "abort_if_not_equals" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue != atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_equal" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue == atoi( token ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		cvarValue |= ( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "bitreset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		cvarValue &= ~( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "abort_if_bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue & ( 1 << atoi( token ) ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_bitset" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( !( cvarValue & ( 1 << atoi( token ) ) ) ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "set" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		cvarValue = atoi( token );
	} else if ( !Q_stricmp( lastToken, "random" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		cvarValue = rand() % atoi( token );
	} else if ( !Q_stricmp( lastToken, "trigger_if_equal" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue == atoi( token ) ) {
			gentity_t* trent;
			int oldId;

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( lastToken, token, sizeof( lastToken ) );
			if ( !*lastToken ) {
				Bot_ScriptError( bs, "trigger must have a name and an identifier\n" );
			}

			token = COM_ParseExt( &pString, qfalse );
			Q_strncpyz( name, token, sizeof( name ) );
			if ( !*name ) {
				Bot_ScriptError( bs, "trigger must have a name and an identifier\n" );
			}

			terminate = qfalse;
			found = qfalse;
			trent = NULL;
			while ( ( trent = BotFindEntity( trent, FOFS( scriptName ), lastToken ) ) ) {
				found = qtrue;
				oldId = trent->scriptStatus.scriptId;
				G_Script_ScriptEvent( trent, "trigger", name );
				// if the script changed, return false so we don't muck with it's variables
				if ( ( trent->s.number == bs->client ) && ( oldId != trent->scriptStatus.scriptId ) ) {
					terminate = qtrue;
				}
			}

			if ( terminate ) {
				return qfalse;
			}
			if ( found ) {
				return qtrue;
			}

//			Bot_ScriptError( bs, "trigger has unknown name: %s\n", name );
			G_Printf( "trigger has unknown name: %s\n", name );
			return qfalse;
		}
	} else if ( !Q_stricmp( lastToken, "wait_while_equal" ) ) {
		if ( !token[0] ) {
			Bot_ScriptError( bs, "cvar %s requires a parameter\n", lastToken );
		}
		if ( cvarValue == atoi( token ) ) {
			return qfalse;
		}
	} else {
		Bot_ScriptError( bs, "cvar %s: unknown command\n", params );
	}

	return qtrue;
}

/*
=================
Bot_ScriptAction_SetMovementAutonomy
=================
*/
qboolean Bot_ScriptAction_SetMovementAutonomy( bot_state_t *bs, char *params ) {
	int mlevel;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetMovementAutonomy requires a parameter" );
	}
	//
	mlevel = BotMovementAutonomyForString( params );
	if ( mlevel < 0 ) {
		Bot_ScriptError( bs, "SetMovementAutonomy: unknown parameter \"%s\"", params );
	}
	bs->script.movementAutonomy = mlevel;
	// TAT - why are there 2 of these vars?  set both of them
	bs->movementAutonomy = mlevel;

	//
	if ( bs->leader < 0 ) {
		VectorCopy( level.clients[bs->client].ps.origin, bs->script.movementAutonomyPos );
		VectorCopy( level.clients[bs->client].ps.origin, bs->movementAutonomyPos );
	}
	//
	return qtrue;
}

/*
======================
Bot_ScriptAction_MovementAutonomy
======================
*/
qboolean Bot_ScriptAction_MovementAutonomy( bot_state_t *bs, char *params ) {
	char *pString, *token, *operand;
	int maLevel;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "MovementAutonomy requires a parameter" );
	}
	//
	pString = params;
	//
	// read the operand
	token = COM_ParseExt( &pString, qfalse );
	operand = va( "%s", token );   // RF, this is a cheap and nasty way of saving memory
	//
	if ( !operand[0] ) {
		Bot_ScriptError( bs, "MovementAutonomy requires an operand" );
	}
	//
	// read the level
	token = COM_ParseExt( &pString, qfalse );
	//
	if ( !token[0] ) {
		Bot_ScriptError( bs, "MovementAutonomy requires a level" );
	}
	//
	maLevel = BotMovementAutonomyForString( token );
	if ( maLevel < 0 ) {
		Bot_ScriptError( bs, "SetMovementAutonomy: unknown movementAutonomy \"%s\"", params );
	}
	//
	// apply the function
	if ( !Q_stricmp( operand, "set" ) ) {
		bs->script.movementAutonomy = maLevel;
		VectorCopy( level.clients[bs->client].ps.origin, bs->script.movementAutonomyPos );
	}
	//
	if ( !Q_stricmp( operand, "force" ) ) {
		bs->script.movementAutonomy = maLevel;
		VectorCopy( level.clients[bs->client].ps.origin, bs->script.movementAutonomyPos );
		bs->leader = -1;    // stop following others
		//
		bs->script.flags |= BSFL_FORCED_MOVEMENT_AUTONOMY;  // force this level
	}
	//
	if ( !Q_stricmp( operand, "unforce" ) ) {
		bs->script.flags &= ~BSFL_FORCED_MOVEMENT_AUTONOMY; // turn it off
	}
	//
	else if ( !Q_stricmp( operand, "abort_if_less_than" ) ) {
		if ( bs->movementAutonomy < maLevel ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	}
	//
	else if ( !Q_stricmp( operand, "abort_if_greater_than" ) ) {
		if ( bs->movementAutonomy > maLevel ) {
			// abort the current script
			bs->script.status.stackHead = bs->script.data->events[bs->script.status.eventIndex].stack.numItems;
		}
	}
	//
	return qtrue;
}

//
// START Mad Doctor I changes, 8/12/2002
// Adding some basic script functions for setting health, notarget, and accuracy
//

/*
=================
Bot_ScriptAction_NoTarget

  syntax: notarget ON/OFF
=================
*/
qboolean Bot_ScriptAction_NoTarget
(
	bot_state_t *bs,
	char *params
) {
	// The user needs to specify on or off
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "notarget requires ON or OFF as parameter" );

	} // if (!params || !params[0]) ...

	if ( !Q_stricmp( params, "ON" ) ) {
		g_entities[bs->client].flags |= FL_NOTARGET;
	} else if ( !Q_stricmp( params, "OFF" ) ) {
		g_entities[bs->client].flags &= ~FL_NOTARGET;
	} else
	{
		Bot_ScriptError( bs,  "notarget requires ON or OFF as parameter" );
	}

	return qtrue;

} // qboolean Bot_ScriptAction_NoTarget( cast_state_t *cs, char *params) ...

/*
====================
Bot_ScriptAction_ResetScript
====================
*/
qboolean Bot_ScriptAction_ResetScript( bot_state_t *bs, char *params ) {
	return qtrue;
}


// START	Mad Doctor I changes, 8/14/2002

/*
=================
Bot_ScriptAction_SetFieldOfView
=================
*/
qboolean Bot_ScriptAction_SetFieldOfView
(
	bot_state_t *bs,
	char *params
) {
	// Make sure we have a parameter to set the FOV to.
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetFieldOfView requires a FOV value" );

	} // if (!params || !params[0]) ...

	// Set the FOV
	bs->scriptedFieldOfView = atof( params );

	return qtrue;

} // qboolean Bot_ScriptAction_SetFieldOfView( cast_state_t *cs, char *params) ...



/*
=================
Bot_ScriptAction_SetHearingRange
=================
*/
qboolean Bot_ScriptAction_SetHearingRange
(
	bot_state_t *bs,
	char *params
) {
	// Make sure we have a parameter to set the hearing range to.
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetHearingRange requires a range value" );

	} // if (!params || !params[0]) ...

	// Set the hearing range
	bs->scriptedHearingRange = atof( params );

	return qtrue;

} // qboolean Bot_ScriptAction_SetHearingRange( cast_state_t *cs, char *params) ...

// START	xkan, 8/20/2002
/*
=================
Bot_ScriptAction_SetCrouch

  syntax: SetCrouch <OnOffFlag>

  OnOffFlag - 1 crouch; 0 don't crouch.
=================
*/
qboolean Bot_ScriptAction_SetCrouch
(
	bot_state_t *bs,
	char *params
) {
	char *pString, *token;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetCrouch: syntax: SetCrouch <On/Off>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] ) {
		G_Error( "Bot_ScriptAction_SetCrouch: syntax: SetCrouch <On/Off>\n" );
	}
	if ( !Q_stricmp( token, "on" ) ) {
		bs->script.flags |= BSFL_CROUCH;
	} else if ( !Q_stricmp( token, "off" ) )   {
		bs->script.flags &= ~BSFL_CROUCH;
	} else {
		G_Error( "Bot_ScriptAction_SetCrouch: syntax: SetCrouch <On/Off>\n" );
	}

	// We're done here!
	return qtrue;

} // Bot_ScriptAction_SetCrouch


qboolean Bot_ScriptAction_SetProne
(
	bot_state_t *bs,
	char *params
) {
	char *pString, *token;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetProne: syntax: SetProne <On/Off>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] ) {
		G_Error( "Bot_ScriptAction_SetProne: syntax: SetProne <On/Off>\n" );
	}
	if ( !Q_stricmp( token, "on" ) ) {
		bs->script.flags |= BSFL_PRONE;
	} else if ( !Q_stricmp( token, "off" ) )   {
		bs->script.flags &= ~BSFL_PRONE;
	} else {
		G_Error( "Bot_ScriptAction_SetProne: syntax: SetProne <On/Off>\n" );
	}

	// We're done here!
	return qtrue;
} // Bot_ScriptAction_SetProne

// Mad Doc - TDF
/*
===================
G_ScriptAction_PrintAccum

  syntax: printaccum <accumNumber>

  prints out the value of  accum 'accumNumber'
===================
*/
qboolean Bot_ScriptAction_PrintAccum( bot_state_t *bs, char *params ) {
	char *token, *pString;
	gentity_t *ent;
	int bufferIndex;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_PrintAccum: syntax: PrintAccum <accumNumber>\n" );
	}

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "Bot_ScriptAction_PrintAccum: syntax: PrintAccum <accumNumber>\n" );
	}


	bufferIndex = atoi( token );
	if ( ( bufferIndex < 0 ) || ( bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS ) ) {
		G_Error( "Bot_ScriptAction_PrintAccum: buffer is outside range (0 - %i)", MAX_SCRIPT_ACCUM_BUFFERS );
	}

	ent = BotGetEntity( bs->client );

	G_Printf( "(BotScript)  %s: Accum[%i] = %d\n", ent->scriptName, bufferIndex, ent->scriptAccumBuffer[bufferIndex] );

	return qtrue;
}


// Mad Doc - TDF
/*
===================
Bot_ScriptAction_PrintGlobalAccum

  syntax: printGlobalAccum <globalaccumnumber>

  prints out the value of global accum 'globalaccumnumber'
===================
*/
qboolean Bot_ScriptAction_PrintGlobalAccum( gentity_t *ent, char *params ) {
	char *token, *pString;
	int bufferIndex;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_PrintGlobalAccum: syntax: PrintGlobalAccum <globalAccumNumber>\n" );
	}

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "Bot_ScriptAction_PrintGlobalAccum: syntax: PrintGlobalAccum <globalAccumNumber>\n" );
	}


	bufferIndex = atoi( token );
	if ( ( bufferIndex < 0 ) || ( bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS ) ) {
		G_Error( "PrintGlobalAccum: buffer is outside range (0 - %i)", MAX_SCRIPT_ACCUM_BUFFERS );
	}


	G_Printf( "(BotScript) GlobalAccum[%i] = %d\n", bufferIndex, level.globalAccumBuffer[bufferIndex] );

	return qtrue;
}







// Mad Doc - TDF
/*
===================
Bot_ScriptAction_BotDebugging

  syntax: BotDebugging ON/OFF

  toggles bot debugging
===================
*/
qboolean Bot_ScriptAction_BotDebugging( gentity_t *ent, char *params ) {
	char *token, *pString;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_BotDebugging: syntax: BotDebugging <ON/OFF>\n" );
	}

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "Bot_ScriptAction_BotDebugging: syntax: BotDebugging <ON/OFF>\n" );
	}

	if ( !Q_stricmp( token, "ON" ) ) {
		trap_Cvar_Set( "bot_debug", "1" );
	} else if ( !Q_stricmp( token, "OFF" ) )        {
		trap_Cvar_Set( "bot_debug", "0" );
	} else
	{
		G_Error( "Bot_ScriptAction_BotDebugging: syntax: BotDebugging <ON/OFF>\n" );
	}

	return qtrue;
}


// xkan, sets the fire rate for the bot
/*
=======================
Bot_ScriptAction_SetFireRate

   Sets the fire rate. fire rate 1 is normal (continuous) firing. fire rate 0 means do not fire
   at all. fire rate in between makes the bot fire a few bullets, wait a little bit, and then fire
   a few more bullets.
=======================
*/
qboolean Bot_ScriptAction_SetFireRate( bot_state_t *bs, char *params ) {
	char *pString, *token;
	float fireRate;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetFireRate: syntax: SetFireRate <0-1>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] ) {
		G_Error( "Bot_ScriptAction_SetFireRate: syntax: SetFireRate <0-1>\n" );
	}
	fireRate = atof( token );
	if ( fireRate < 0.0 || fireRate > 1.0 ) {
		G_Error( "Bot_ScriptAction_SetFireRate: syntax: SetFireRate <0-1>\n" );
	}

	bs->fireRate = fireRate;
	// We're done here!
	return qtrue;
}

// xkan, sets the fire cycle time for the bot
/*
=======================
Bot_ScriptAction_SetFireCycleTime

   Sets the minimum/maximum fire cycle time. Actual fire cycle time is a random number between
   minimum cycle time and maximum cycle time.

   during a fire cycle, the bot will fire for a duration equal to fireRate * cycleTime,
   and hold fire for the rest of the cycle (which is equal to (1-fireRate)*cycleTime.)

=======================
*/
qboolean Bot_ScriptAction_SetFireCycleTime( bot_state_t *bs, char *params ) {
	char *pString, *token;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetFireCycleTime: syntax: SetFireCycleTime <minimum time in msec> <maximum time in msec>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] || token[0] < '0' || token[0] > '9' ) {
		G_Error( "Bot_ScriptAction_SetFireCycleTime: syntax: SetFireCycleTime <minimum time in msec> <maximum time in msec>\n" );
	}
	bs->minFireRateCycleTime = atoi( token );

	token = COM_Parse( &pString );
	if ( !token || !token[0] || token[0] < '0' || token[0] > '9' ) {
		G_Error( "Bot_ScriptAction_SetFireCycleTime: syntax: SetFireCycleTime <minimum time in msec> <maximum time in msec>\n" );
	}
	bs->maxFireRateCycleTime = atoi( token );

	// We're done here!
	return qtrue;
}


/*
=======================
Bot_ScriptAction_SetVisionRange

Mad Doctor I, 10/23/2002
Set a maximum vision range for bots to see you
=======================
*/
qboolean Bot_ScriptAction_SetVisionRange( bot_state_t *bs, char *params ) {
	char *pString, *token;
	float visionRange = 0;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetVisionRange: syntax: SetVisionRange <range>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] ) {
		G_Error( "Bot_ScriptAction_SetVisionRange: syntax: SetVisionRange <range>\n" );
	}
	visionRange = atof( token );

	// Set the range for the bot
	bs->visionRange = visionRange;

	// We're done here!
	return qtrue;
}



/*
=======================
Bot_ScriptAction_SetFarSeeingRange

Mad Doctor I, 10/23/2002
Set a maximum vision range for bots to "spot and report" not attack
=======================
*/
qboolean Bot_ScriptAction_SetFarSeeingRange( bot_state_t *bs, char *params ) {
	char *pString, *token;
	float farSeeingRange = 0;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetFarSeeingRange: syntax: SetFarSeeingRange <range>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] ) {
		G_Error( "Bot_ScriptAction_SetFarSeeingRange: syntax: SetFarSeeingRange <range>\n" );
	}
	farSeeingRange = atof( token );

	// Set the range for the bot
	bs->farSeeingRange = farSeeingRange;

	// We're done here!
	return qtrue;
}



/*
=======================
Bot_ScriptAction_SetCloseHearingRange

Mad Doctor I, 10/23/2002
When an enemy is this close, you can sense them outside FOV
=======================
*/
qboolean Bot_ScriptAction_SetCloseHearingRange( bot_state_t *bs, char *params ) {
	char *pString, *token;
	float closeHearingRange = 0;

	if ( !params || !params[0] ) {
		G_Error( "Bot_ScriptAction_SetCloseHearingRange: syntax: SetCloseHearingRange <range>\n" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token || !token[0] ) {
		G_Error( "Bot_ScriptAction_SetCloseHearingRange: syntax: SetCloseHearingRange <range>\n" );
	}
	closeHearingRange = atof( token );

	// Set the range for the bot
	bs->closeHearingRange = closeHearingRange;

	// We're done here!
	return qtrue;
}


/*
=======================
Bot_ScriptAction_SetSpeedCoefficient

Mad Doctor I, 11/26/2002.  Set an individual bot's speed
=======================
*/
qboolean Bot_ScriptAction_SetSpeedCoefficient( bot_state_t *bs, char *params ) {
	return qtrue;
}

// TAT 2/4/2003 - just force an update of current bot selection - used when who is selectable has changed
extern void UpdateSelectedBots( gentity_t *ent );

// TAT 11/16/2002 - Set the selected weapon of the bot - does NOT change which weapons the bot is holding
qboolean Bot_ScriptAction_SetActiveWeapon( bot_state_t *bs, char *params ) {
	// Which class are we?
	int playerClass = g_entities[bs->client].client->sess.playerType;

	int weapon;
	//
	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetActiveWeapon requires a weapon name" );
	}

	weapon = Bot_GetWeaponForClassAndTeam( playerClass, g_entities[bs->client].client->sess.sessionTeam, params );
	if ( weapon == -1 ) {
		Bot_ScriptError( bs, "Bot %s on team %s cannot use weapon %s\n", g_entities[bs->client].aiName, ( g_entities[bs->client].client->sess.sessionTeam == TEAM_AXIS ) ? "Axis" : "Allies", params );
	}

	// if the bot doesn't have the weapon in question, error
	if ( !COM_BitCheck( bs->cur_ps.weapons, weapon ) ) {
		Bot_ScriptError( bs, "Bot %s on team %s doesn't have weapon %s\n", g_entities[bs->client].aiName, ( g_entities[bs->client].client->sess.sessionTeam == TEAM_AXIS ) ? "Axis" : "Allies", params );
	}

	// otherwise, set the commanded weapon
	bs->commandedWeapon = weapon;
	bs->weaponnum = weapon;
	// and switch to it
	trap_EA_SelectWeapon( bs->client, weapon );

	// done
	return qtrue;
}



/*
===================
Bot_ScriptAction_Announce

	Gordon: same as the equivilant G_ScriptAction_Announce
	syntax: wm_announce <"text to send to all clients">
===================
*/
qboolean Bot_ScriptAction_Announce( bot_state_t *bs, char *params ) {
	char *pString, *token;

	if ( level.intermissiontime ) {
		return qtrue;
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "Bot_ScriptAction_Announce: statement parameter required\n" );
	}

	trap_SendServerCommand( -1, va( "cp \"%s\" 2", token ) );

	return qtrue;
}

/*
=================
Bot_ScriptAction_FireAtTarget

  syntax: fireattarget <targetname> [duration]
=================
*/
qboolean Bot_ScriptAction_FireAtTarget( bot_state_t *bs, char *params ) {
	gentity_t   *ent;
	vec3_t vec, org, src;
	char *pString, *token;
	float diff;
	int i;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "AI Scripting: fireattarget without a targetname\n" );
	}

	// find this targetname
	ent = BotFindEntityForName( token );
	if ( !ent ) {
		ent = G_FindByTargetname( NULL, token );
		if ( !ent ) {
			Bot_ScriptError( bs,  "AI Scripting: fireattarget cannot find targetname/aiName \"%s\"\n", token );
		}
	}

	// if this is our first call for this fireattarget, record the ammo count
	//if (bs->script.flags & BSFL_FIRST_CALL)
	//{
	//	bs->last_fire = 0;
	//}

	// make sure we don't move or shoot while turning to our target
	//if (bs->script.status.scriptNoAttackTime < level.time) {
	//	bs->script.status.scriptNoAttackTime = level.time + 500;
	//}
	// dont reload prematurely
	//bs->noReloadTime = level.time + 1000;
	// don't move while firing at all
	//bs->castScriptStatus.scriptNoMoveTime = level.time + 500;

	// stand still
	//bs->script.frameFlags |= BSFFL_STAND;

	// let us move our view, whether it looks bad or not
	//bs->castScriptStatus.playAnimViewlockTime = 0;

	// set the view angle manually
	BG_EvaluateTrajectory( &ent->s.pos, level.time, org, qfalse, -1 );
	VectorCopy( bs->origin, src );
	src[2] += bs->cur_ps.viewheight;

	VectorSubtract( org, src, vec );
	VectorNormalize( vec );
	vectoangles( vec, bs->ideal_viewangles );

	if ( bs->weaponnum == WP_MORTAR_SET ) {
/*		vec_t x, y, u, d, b, g, a;
		vec3_t diff;
		VectorSubtract( org, src, diff );
		diff[2] = 0;

		g = g_gravity.value;
		x = VectorLength( diff );
		y = org[2] - src[2];
		u = MORTAR_SP_BOTSPEED;

		d = (g * SQR(x)) / (2 * SQR(u));

		b = (SQR(x) - (4 * d * (y + d)));

		if(b < 0) {
			return qfalse;
		}

		a = (vec_t)atan((-x - b) / (-2 * d));
		bs->ideal_viewangles[PITCH] = (AngleMod(RAD2DEG(a)- 180) + 60);*/

		float g = -g_gravity.value;

		float uz = sqrt( -2 * 3072 * g );
		float t = ( ( -uz ) / g ) * 2;
		float ux = ( org[0] - src[0] ) / t;
		float uy = ( org[1] - src[1] ) / t;

		VectorSet( g_entities[bs->entitynum].gDelta, ux, uy, uz );
	} else //if (bs->weaponnum != WP_MORTAR_SET)
	{
		for ( i = 0; i < 2; i++ ) {
			diff = abs( AngleDifference( bs->cur_ps.viewangles[i], bs->ideal_viewangles[i] ) );
			if ( VectorCompare( vec3_origin, ent->s.pos.trDelta ) ) {
				if ( diff ) {
					return qfalse;  // not facing yet
				}
			} else {
				if ( diff > 25 ) {    // allow some slack when target is moving
					return qfalse;
				}
			}
		}
	}

	// force fire
	trap_EA_Attack( bs->client );
	//
	bs->flags |= BFL_ATTACKED;

	// if we haven't fired yet
	//if (!bs->last_fire) {
	//	return qfalse;
	//}

	// do we need to stay and fire for a duration?
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		return qtrue;   // no need to wait around

	}
	if ( !Q_stricmp( token, "forever" ) ) {
		return qfalse;
	}

	// only return true if we've been firing for long enough
	//		TAT 1/29/2003 - the token in question was parsed right above this
	//		so can't just move this up to the top
	//		plus, if we don't pass in a duration, that means fire once, which that check above for no token will handle,
	//		but we WILL fire once at least
	return ( ( bs->script.status.stackChangeTime + atoi( token ) ) < level.time );
}

/*
=======================
Bot_ScriptAction_SetScriptAutonomy
=======================
*/
qboolean Bot_ScriptAction_SetScriptAutonomy( bot_state_t *bs, char *params ) {
	scriptAutonomy_t level;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "SetScriptAutonomy requires a parameter" );
	}
	//
	level = BotScriptAutonomyForString( params );
	if ( level < 0 ) {
		Bot_ScriptError( bs, "SetScriptAutonomy: unknown parameter \"%s\"", params );
	}

	bs->scriptAutonomy = level;

	//
	return qtrue;
}

// TAT 12/14/2002 - Set how much ammo we have for a particular weapon
//		Doesn't change the current weapon loadout (which weapons we carry)
qboolean Bot_ScriptAction_SetAmmoAmount( bot_state_t *bs, char *params ) {
	char *pString, *token;
	int weapon, amount;
	qboolean clipOnly = qfalse;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "Bot_ScriptAction_SetAmmoAmmount: syntax: SetAmmoAmount <weaponname> <number>" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "Bot_ScriptAction_SetAmmoAmmount: syntax: SetAmmoAmount <weaponname> <number>" );
	}

	weapon = Bot_GetWeaponForClassAndTeam( g_entities[bs->client].client->sess.playerType, g_entities[bs->client].client->sess.sessionTeam, token );
	if ( weapon == -1 ) {
		// can't use this weapon
		Bot_ScriptError( bs, "Bot %s on team %s cannot use weapon %s\n", g_entities[bs->client].aiName, ( g_entities[bs->client].client->sess.sessionTeam == TEAM_AXIS ) ? "Axis" : "Allies", token );
	}

	// Do we have this weapon?
	if ( !COM_BitCheck( bs->cur_ps.weapons, weapon ) ) {
		Bot_ScriptError( bs, "Bot_ScriptAction_SetAmmoAmount: Bot %s does not have weapon %s", g_entities[bs->client].aiName, token );
	}

	// what are we setting the ammo amount to?
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		Bot_ScriptError( bs, "Bot_ScriptAction_SetAmmoAmmount: syntax: SetAmmoAmount <weaponname> <number>" );
	}

	amount = atoi( token );

	// Some specials use the clip only
	switch ( weapon )
	{
	case WP_AMMO:
	case WP_MEDKIT:
	case WP_LANDMINE:
	case WP_MEDIC_SYRINGE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_FLAMETHROWER:
	case WP_MORTAR:
	case WP_MORTAR_SET:
	case WP_DYNAMITE:
		clipOnly = qtrue;
		break;
	}

	if ( clipOnly ) {
		g_entities[bs->client].client->ps.ammoclip[BG_FindAmmoForWeapon( weapon )] = amount;
	} else
	{
		g_entities[bs->client].client->ps.ammo[BG_FindAmmoForWeapon( weapon )] = amount;
	}

	// done
	return qtrue;
}

qboolean Bot_ScriptAction_SetCivilian( bot_state_t *bs, char *params ) {
	char *pString, *token;

	if ( !params || !params[0] ) {
		Bot_ScriptError( bs, "Bot_ScriptAction_SetCivilian: syntax: SetCivilian <Yes/No>" );
	}

	pString = params;
	token = COM_Parse( &pString );
	if ( token[0] && !Q_stricmp( token, "yes" ) ) {
		g_entities[bs->client].client->isCivilian = qtrue;
	} else if ( token[0] && !Q_stricmp( token, "no" ) )   {
		g_entities[bs->client].client->isCivilian = qfalse;
	} else {
		Bot_ScriptError( bs, "Bot_ScriptAction_SetCivilian: syntax: SetCivilian <Yes/No>" );
	}

	// done
	return qtrue;
}
