/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * Copyright (C) 2010 COR Entertainment, LLC.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file irc_client.c
 * @brief irc ingame client
 */

// this is FEATURE_IRC_CLIENT and FEATURE_IRC_SERVER only

#include "../client/client.h"
#include "htable.h"

#ifdef DEDICATED
#include "../server/server.h"
#endif

#ifdef WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
# include <process.h>
typedef SOCKET irc_socket_t;
#else
# include <unistd.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sys/param.h>
# include <sys/ioctl.h>
# include <sys/uio.h>
# include <errno.h>
# include <pthread.h>
typedef int irc_socket_t;
# if !defined HAVE_CLOSESOCKET
#  define closesocket close
# endif
# if !defined INVALID_SOCKET
#  define INVALID_SOCKET (-1)
# endif
#endif

cvar_t *irc_mode;
cvar_t *irc_server;
cvar_t *irc_channel;
cvar_t *irc_port;
cvar_t *irc_nickname;
cvar_t *irc_kick_rejoin;
cvar_t *irc_reconnect_delay;

/*
 * Timing controls
 *
 * In order to avoid actively waiting like crazy, there are many parts of the
 * IRC client code that need to sleep or wait for a timeout. However, if the
 * wait is too long, it makes the whole thing unreactive to e.g. the irc_say
 * command; if the wait is too shot, it starts using CPU time.
 *
 * The constants below control the timeouts.
 */

#define IRC_TIMEOUT_MS      250
#define IRC_TIMEOUT_US      (IRC_TIMEOUT_MS * 1000)
#define IRC_TIMEOUTS_PER_SEC    (1000 / IRC_TIMEOUT_MS)

/* Ctype-like macros */
#define IS_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c) ((c) >= 'a' && (c) <= 'z')
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_CNTRL(c) ((c) >= 0 && (c) <= 31)
#define IS_ALPHA(c) (IS_UPPER(c) || IS_LOWER(c))
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c))
#define IS_SPECL(c) (((c) >= '[' && (c) <= '`') || ((c) >= '{' && (c) <= '}') || ((c) == '-'))
#define IS_CLEAN(c) (IS_ALNUM(c) || IS_SPECL(c))  // RFC 2812

/* IRC command status; used to determine if connection should be re-attempted or not */
#define IRC_CMD_SUCCESS     0   ///< Success
#define IRC_CMD_FATAL       1   ///< Fatal error, don't bother retrying
#define IRC_CMD_RETRY       2   ///< Recoverable error, command should be attempted again

/* Constants that indicate the state of the IRC thread. */
#define IRC_THREAD_DEAD     0       ///< Thread is dead or hasn't been started
#define IRC_THREAD_INITIALISING 1   ///< Thread is being initialised
#define IRC_THREAD_CONNECTING   2   ///< Thread is attempting to connect
#define IRC_THREAD_SETNICK  3       ///< Thread is trying to set the player's
///< nick
#define IRC_THREAD_CONNECTED    4   ///< Thread established a connection to
///< the server and will attempt to join
///< the channel
#define IRC_THREAD_JOINED   5       ///< Channel joined, ready to send or
///< receive messages
#define IRC_THREAD_QUITTING 6       ///< The thread is being killed

/* Function that sets the thread status when the thread dies. Since that is
 * system-dependent, it can't be done in the thread's main code.
 */
static void IRC_SetThreadDead();

/**
 * @var IRC_ThreadStatus
 * @brief Status of the IRC thread
 */
static int IRC_ThreadStatus = IRC_THREAD_DEAD;

/**
 * @var IRC_QuitRequested
 * @brief Quit requested?
 */
static qboolean IRC_QuitRequested;

/**
 * @var IRC_Socket
 * @brief Socket handler
 */
static irc_socket_t IRC_Socket;

/*
 * The protocol parser uses a finite state machine, here are the various
 * states' definitions as well as a variable containing the current state
 * and various other variables for message building.
 */
#define IRC_PARSER_RECOVERY     (-1)    ///< Error recovery
#define IRC_PARSER_START        0       ///< Start of a message
#define IRC_PARSER_PFX_NOS_START    1   ///< Prefix start
#define IRC_PARSER_PFX_NOS      2       ///< Prefix, server or nick name
#define IRC_PARSER_PFX_USER_START   3   ///< Prefix, start of user name
#define IRC_PARSER_PFX_USER     4       ///< Prefix, user name
#define IRC_PARSER_PFX_HOST_START   5   ///< Prefix, start of host name
#define IRC_PARSER_PFX_HOST     6       ///< Prefix, host name
#define IRC_PARSER_COMMAND_START    7   ///< Start of command after a prefix
#define IRC_PARSER_STR_COMMAND      8   ///< String command
#define IRC_PARSER_NUM_COMMAND_2    9   ///< Numeric command, second character
#define IRC_PARSER_NUM_COMMAND_3    10  ///< Numeric command, third character
#define IRC_PARSER_NUM_COMMAND_4    11  ///< Numeric command end
#define IRC_PARSER_PARAM_START      12  ///< Parameter start
#define IRC_PARSER_MID_PARAM        13  ///< "Middle" parameter
#define IRC_PARSER_TRAILING_PARAM   14  ///< Trailing parameter
#define IRC_PARSER_LF           15      ///< End of line

static int      IRC_ParserState;
static qboolean IRC_ParserInMessage;
static qboolean IRC_ParserError;

/*
 * According to RFC 1459, maximal message size is 512 bytes, including trailing
 * CRLF.
 */
#define IRC_MESSAGE_SIZE 512
#define IRC_SEND_BUF_SIZE IRC_MESSAGE_SIZE
#define IRC_RECV_BUF_SIZE (IRC_MESSAGE_SIZE * 2)

/*
 * IRC messages consist in:
 * 1) an optional prefix, which contains either a server name or a nickname,
 * 2) a command, which may be either a word or 3 numbers,
 * 3) any number of arguments.
 *
 * RFC 2812 says that there are at most 14 "middle" parameters and a trailing
 * parameter. However, UnrealIRCd does not respect this, and sends messages
 * that contain an extra parameter. While the message in question could be
 * ignored, it's better to avoid entering the error recovery state.
 *
 * Since we won't be handling messages in parallel, we will create a
 * static record and use that to store everything, as we can definitely
 * spare 130k of memory (note: we could have something smaller but it'd
 * probably be a pointless exercise).
 */

#define irc_string_t(len) struct { \
		unsigned int length; \
		char string[len]; \
}

#define IRC_MAX_NICK_LEN 64
#define IRC_MAX_ARG_LEN 509
#define IRC_MAX_PARAMS 16

/**
 * @struct irc_message_t
 * @brief
 */
struct irc_message_t
{
	// Prefix
	irc_string_t(IRC_MAX_NICK_LEN) pfx_nickOrServer;
	irc_string_t(IRC_MAX_NICK_LEN) pfx_user;
	irc_string_t(IRC_MAX_NICK_LEN) pfx_host;

	// Command
	irc_string_t(32) cmd_string;

	// Arguments
	irc_string_t(IRC_MAX_ARG_LEN) arg_values[IRC_MAX_PARAMS];
	unsigned int arg_count;
};

static struct irc_message_t IRC_ReceivedMessage;

// Macros to access the message's various fields
#define IRC_String(N) (IRC_ReceivedMessage.N.string)
#define IRC_Length(N) (IRC_ReceivedMessage.N.length)

/*
 * IRC command handlers are called when some command is received;
 * they are stored in hash tables.
 */
typedef int (*irc_handler_func_t)();
typedef int (*ctcp_handler_func_t)(qboolean is_channel, const char *message);

/**
 * @struct irc_handler_t
 * @brief
 */
struct irc_handler_t
{
	char cmd_string[33];
	void *handler;
};

static hashtable_t IRC_Handlers;
static hashtable_t IRC_CTCPHandlers;

/**
 * @struct irc_user_t
 * @brief Username, nickname, etc...
 */
struct irc_user_t
{
	char nick[16];
	int nickattempts;
	char username[16];
	char email[100];
};

static struct irc_user_t IRC_User;

/*
 * Events that can be displayed and flags that apply to them.
 */
#define IRC_EVT_SAY         0x00000000  ///< Standard message
#define IRC_EVT_ACT         0x00000001  ///< \/me message
#define IRC_EVT_JOIN        0x00000002  ///< Join
#define IRC_EVT_PART        0x00000003  ///< Part
#define IRC_EVT_QUIT        0x00000004  ///< Quit
#define IRC_EVT_KICK        0x00000005  ///< Kick
#define IRC_EVT_NICK_CHANGE 0x00000006  ///< Nick change
#define IRC_EVTF_SELF       0x00000100  ///< Event applies to current user

#define IRC_EventType(evt) (evt & 0xff)
#define IRC_EventIsSelf(evt) ((evt & IRC_EVTF_SELF) == IRC_EVTF_SELF)
#define IRC_MakeEvent(type, isself) (IRC_EVT_ ## type | ((isself) ? IRC_EVTF_SELF : 0))

/*
 * Rate limiters for various events.
 *
 * The rate limiter works on a per-event basis, it doesn't know nor care
 * about users.
 * Its threshold and increase constants (which will be scaled depending on
 * the timing controls) determine the amount of responses per second, while
 * also allowing "bursts".
 */

/// Rate limiter threshold - above that, no response
#define IRC_LIMIT_THRESHOLD 3

/// Rate limiter increase per check
#define IRC_LIMIT_INCREASE  1

#define IRC_RL_MESSAGE  0
#define IRC_RL_PING     1
#define IRC_RL_VERSION  2

static unsigned int IRC_RateLimiter[3];

/*--------------------------------------------------------------------------*/
/* FUNCTIONS THAT MANAGE IRC COMMAND HANDLERS                               */
/*--------------------------------------------------------------------------*/

/**
 * @brief Initialises the handler tables
 */
static ID_INLINE void IRC_InitHandlers()
{
	IRC_Handlers = HT_Create(100, HT_FLAG_INTABLE | HT_FLAG_CASE,
	                         sizeof(struct irc_handler_t),
	                         HT_OffsetOfField(struct irc_handler_t, cmd_string),
	                         32);
	IRC_CTCPHandlers = HT_Create(100, HT_FLAG_INTABLE | HT_FLAG_CASE,
	                             sizeof(struct irc_handler_t),
	                             HT_OffsetOfField(struct irc_handler_t, cmd_string),
	                             32);
}

/**
 * @brief Frees the list of handlers (used when the IRC thread dies).
 */
static void IRC_FreeHandlers()
{
	HT_Destroy(IRC_Handlers);
	HT_Destroy(IRC_CTCPHandlers);
}

/**
 * @brief Registers a new IRC command handler.
 * @param[in] command
 * @param[in] handler
 */
static ID_INLINE void IRC_AddHandler(const char *command, irc_handler_func_t handler)
{
	qboolean             created;
	struct irc_handler_t *rv;

	rv = HT_GetItem(IRC_Handlers, command, &created);
	etl_assert(created);
	rv->handler = handler;
}

/**
 * @brief Registers a new CTCP command handler.
 * @param[in] command
 * @param[in] handler
 */
static void IRC_AddCTCPHandler(const char *command, ctcp_handler_func_t handler)
{
	qboolean             created;
	struct irc_handler_t *rv;

	rv = HT_GetItem(IRC_CTCPHandlers, command, &created);
	etl_assert(created);
	rv->handler = handler;
}

/**
 * @brief Executes the command handler for the currently stored command. If there is
 * no registered handler matching the command, ignore it.
 * @return
 */
static int IRC_ExecuteHandler()
{
	struct irc_handler_t *handler;

	handler = HT_GetItem(IRC_Handlers, IRC_String(cmd_string), NULL);
	if (handler == NULL)
	{
		return IRC_CMD_SUCCESS;
	}
	return ((irc_handler_func_t)(handler->handler))();
}

/**
 * @brief Executes a CTCP command handler.
 * @param[in] command
 * @param[in] is_channel
 * @param[in] argument
 * @return
 */
static int IRC_ExecuteCTCPHandler(const char *command, qboolean is_channel, const char *argument)
{
	struct irc_handler_t *handler;

	handler = HT_GetItem(IRC_CTCPHandlers, command, NULL);
	if (handler == NULL)
	{
		return IRC_CMD_SUCCESS;
	}
	return ((ctcp_handler_func_t)(handler->handler))(is_channel, argument);
}

/*--------------------------------------------------------------------------*/
/* IRC DELAYED EXECUTION                                                    */
/*--------------------------------------------------------------------------*/

/**
 * @struct IRC_DEQueue
 * @brief Structure for the delayed execution queue
 */
struct irc_delayed_t
{
	irc_handler_func_t handler;         ///< Handler to call
	int time_left;                      ///< "Time" left before call
	struct irc_delayed_t *next;         ///< Next record
};

/**
 * @var IRC_DEQueue
 * @brief Delayed execution queue head & tail
 */
static struct irc_delayed_t *IRC_DEQueue = NULL;

/**
 * @brief This function sets an IRC handler function to be executed after some time.
 * @param[in] function
 * @param[in] time
 */
static void IRC_SetTimeout(irc_handler_func_t function, int time)
{
	struct irc_delayed_t *qe, *find;
	etl_assert(time > 0);

	// Create entry
	qe            = (struct irc_delayed_t *) Com_Allocate(sizeof(struct irc_delayed_t));
	qe->handler   = function;
	qe->time_left = time * IRC_TIMEOUTS_PER_SEC;

	// Find insert location
	if (IRC_DEQueue)
	{
		if (IRC_DEQueue->time_left >= time)
		{
			qe->next    = IRC_DEQueue;
			IRC_DEQueue = qe;
		}
		else
		{
			find = IRC_DEQueue;
			while (find->next && find->next->time_left < time)
			{
				find = find->next;
			}
			qe->next   = find->next;
			find->next = qe;
		}
	}
	else
	{
		qe->next    = NULL;
		IRC_DEQueue = qe;
	}
}

/**
 * @brief This function dequeues an entry from the delayed execution queue.
 * @return
 */
static qboolean IRC_DequeueDelayed()
{
	struct irc_delayed_t *found;

	if (!IRC_DEQueue)
	{
		return qfalse;
	}

	found       = IRC_DEQueue;
	IRC_DEQueue = found->next;
	Com_Dealloc(found);
	return qtrue;
}

/**
 * @brief This function deletes all remaining entries from the delayed execution queue
 */
static void IRC_FlushDEQueue()
{
	while (IRC_DequeueDelayed())
	{
		// PURPOSEDLY EMPTY
	}
}

/**
 * @brief This function processes the delayed execution queue.
 * @return
 */
static int IRC_ProcessDEQueue()
{
	struct irc_delayed_t *iter;
	int                  err_code;

	iter = IRC_DEQueue;
	while (iter)
	{
		if (iter->time_left == 1)
		{
			err_code = (iter->handler)();
			IRC_DequeueDelayed();
			if (err_code != IRC_CMD_SUCCESS)
			{
				return err_code;
			}
			iter = IRC_DEQueue;
		}
		else
		{
			iter->time_left--;
			iter = iter->next;
		}
	}

	return IRC_CMD_SUCCESS;
}

/*--------------------------------------------------------------------------*/
/* IRC MESSAGE PARSER                                                       */
/*--------------------------------------------------------------------------*/

/* Parser macros, 'cause I'm lazy */
#define P_SET_STATE(S) IRC_ParserState = IRC_PARSER_ ## S
#define P_INIT_MESSAGE(S) { \
		P_SET_STATE(S); \
		IRC_ParserInMessage = qtrue; \
		Com_Memset(&IRC_ReceivedMessage, 0, sizeof(struct irc_message_t)); \
}
#if defined DEBUG_DUMP_IRC
#define P_ERROR(S) { \
		if (!IRC_ParserError) { \
			Com_Printf("IRC PARSER ERROR (state: %d , received: %d)\n", IRC_ParserState, next); \
		} \
		P_SET_STATE(S); \
		IRC_ParserError = qtrue; \
}
#else // defined DEBUG_DUMP_IRC
#define P_ERROR(S) { \
		P_SET_STATE(S); \
		IRC_ParserError = qtrue; \
}
#endif // defined DEBUG_DUMP_IRC
#define P_AUTO_ERROR { \
		if (next == '\r') { \
			P_ERROR(LF); \
		} else { \
			P_ERROR(RECOVERY); \
		} \
}
#define P_INIT_STRING(S) { \
		IRC_ReceivedMessage.S.string[0] = next; \
		IRC_ReceivedMessage.S.length    = 1; \
}
#define P_ADD_STRING(S) { \
		if (IRC_ReceivedMessage.S.length == sizeof(IRC_ReceivedMessage.S.string) - 1) { \
			P_ERROR(RECOVERY); \
		} else { \
			IRC_ReceivedMessage.S.string[IRC_ReceivedMessage.S.length++] = next; \
		} \
}
#define P_NEXT_PARAM { \
		if ((++IRC_ReceivedMessage.arg_count) == IRC_MAX_PARAMS) { \
			P_ERROR(RECOVERY); \
		} \
}
#define P_START_PARAM { \
		if ((++IRC_ReceivedMessage.arg_count) == IRC_MAX_PARAMS) { \
			P_ERROR(RECOVERY); \
		} else { \
			P_INIT_STRING(arg_values[IRC_ReceivedMessage.arg_count - 1]) \
		} \
}
#define P_ADD_PARAM P_ADD_STRING(arg_values[IRC_ReceivedMessage.arg_count - 1])

/**
 * @brief Main parsing function that uses a FSM to parse one character at a time.
 * @param next
 * @return true when a full message is read and no error has occured.
 */
static qboolean IRC_Parser(char next)
{
	qboolean has_msg = qfalse;

	switch (IRC_ParserState)
	{

	/* Initial state; clear the message, then check input. ':'
	 * indicates there is a prefix, a digit indicates a numeric
	 * command, an upper-case letter indicates a string command.
	 * It's also possible we received an empty line - just skip
	 * it. Anything else is an error.
	 */
	case IRC_PARSER_START:
		IRC_ParserError     = qfalse;
		IRC_ParserInMessage = qfalse;
		if (next == ':')
		{
			P_INIT_MESSAGE(PFX_NOS_START);
		}
		else if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		else if (IS_DIGIT(next))
		{
			P_INIT_MESSAGE(NUM_COMMAND_2);
			P_INIT_STRING(cmd_string);
		}
		else if (IS_UPPER(next))
		{
			P_INIT_MESSAGE(STR_COMMAND);
			P_INIT_STRING(cmd_string);
		}
		else
		{
			P_ERROR(RECOVERY);
		}
		break;
	/*
	 * Start of prefix; anything is accepted, except for '!', '@', ' '
	 * and control characters which all cause an error recovery.
	 */
	case IRC_PARSER_PFX_NOS_START:
		if (next == '!' || next == '@' || next == ' ' || IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else
		{
			P_SET_STATE(PFX_NOS);
			P_INIT_STRING(pfx_nickOrServer);
		}
		break;
	/*
	 * Prefix, server or nick name. Control characters cause an error,
	 * ' ', '!' and '@' cause state changes.
	 */
	case IRC_PARSER_PFX_NOS:
		if (next == '!')
		{
			P_SET_STATE(PFX_USER_START);
		}
		else if (next == '@')
		{
			P_SET_STATE(PFX_HOST_START);
		}
		else if (next == ' ')
		{
			P_SET_STATE(COMMAND_START);
		}
		else if (IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else
		{
			P_ADD_STRING(pfx_nickOrServer);
		}
		break;
	/*
	 * Start of user name; anything goes, except for '!', '@', ' '
	 * and control characters which cause an error.
	 */
	case IRC_PARSER_PFX_USER_START:
		if (next == '!' || next == '@' || next == ' ' || IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else
		{
			P_SET_STATE(PFX_USER);
			P_INIT_STRING(pfx_user);
		}
		break;
	/*
	 * User name; '@' will cause state changes, '!' , ' ' and
	 * control characters will cause errors.
	 */
	case IRC_PARSER_PFX_USER:
		if (next == '@')
		{
			P_SET_STATE(PFX_HOST_START);
		}
		else if (next == '!' || next == ' ' || IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else
		{
			P_ADD_STRING(pfx_user);
		}
		break;
	/*
	 * Start of host name; anything goes, except for '!', '@', ' '
	 * and control characters which cause an error.
	 */
	case IRC_PARSER_PFX_HOST_START:
		if (next == '!' || next == '@' || next == ' ' || IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else
		{
			P_SET_STATE(PFX_HOST);
			P_INIT_STRING(pfx_host);
		}
		break;
	/*
	 * Host name; ' ' will cause state changes, '!' and control
	 * characters will cause errors.
	 */
	case IRC_PARSER_PFX_HOST:
		if (next == ' ')
		{
			P_SET_STATE(COMMAND_START);
		}
		else if (next == '!' || next == '@' || IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else
		{
			P_ADD_STRING(pfx_host);
		}
		break;
	/*
	 * Start of command, will accept start of numeric and string
	 * commands; anything else is an error.
	 */
	case IRC_PARSER_COMMAND_START:
		if (IS_DIGIT(next))
		{
			P_SET_STATE(NUM_COMMAND_2);
			P_INIT_STRING(cmd_string);
		}
		else if (IS_UPPER(next))
		{
			P_SET_STATE(STR_COMMAND);
			P_INIT_STRING(cmd_string);
		}
		else
		{
			P_AUTO_ERROR;
		}
		break;
	/*
	 * String command. Uppercase letters will cause the parser
	 * to continue on string commands, ' ' indicates a parameter
	 * is expected, '\r' means we're done. Anything else is an
	 * error.
	 */
	case IRC_PARSER_STR_COMMAND:
		if (next == ' ')
		{
			P_SET_STATE(PARAM_START);
		}
		else if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		else if (IS_UPPER(next))
		{
			P_ADD_STRING(cmd_string);
		}
		else
		{
			P_ERROR(RECOVERY);
		}
		break;
	/*
	 * Second/third digit of numeric command; anything but a digit
	 * is an error.
	 */
	case IRC_PARSER_NUM_COMMAND_2:
	case IRC_PARSER_NUM_COMMAND_3:
		if (IS_DIGIT(next))
		{
			IRC_ParserState++;
			P_ADD_STRING(cmd_string);
		}
		else
		{
			P_AUTO_ERROR;
		}
		break;
	/*
	 * End of numeric command, could be a ' ' or a '\r'.
	 */
	case IRC_PARSER_NUM_COMMAND_4:
		if (next == ' ')
		{
			P_SET_STATE(PARAM_START);
		}
		else if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		else
		{
			P_ERROR(RECOVERY);
		}
		break;
	/*
	 * Start of parameter. ':' means it's a trailing parameter,
	 * spaces and control characters shouldn't be here, and
	 * anything else is a "middle" parameter.
	 */
	case IRC_PARSER_PARAM_START:
		if (next == ':')
		{
			P_SET_STATE(TRAILING_PARAM);
			P_NEXT_PARAM;
		}
		else if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		else if (IS_CNTRL(next))
		{
			P_AUTO_ERROR;
		}
		else if (next != ' ')
		{
			if (next & 0x80)
			{
				next = '?';
			}
			P_SET_STATE(MID_PARAM);
			P_START_PARAM;
		}
		break;
	/*
	 * "Middle" parameter; ' ' means there's another parameter coming,
	 * '\r' means the end of the message, control characters are not
	 * accepted, anything else is part of the parameter.
	 */
	case IRC_PARSER_MID_PARAM:
		if (next == ' ')
		{
			P_SET_STATE(PARAM_START);
		}
		else if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		else if (IS_CNTRL(next))
		{
			P_ERROR(RECOVERY);
		}
		else
		{
			if (next & 0x80)
			{
				next = '?';
			}
			P_ADD_PARAM;
		}
		break;
	/*
	 * Trailing parameter; '\r' means the end of the command,
	 * and anything else is just added to the string.
	 */
	case IRC_PARSER_TRAILING_PARAM:
		if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		else
		{
			if (next & 0x80)
			{
				next = '?';
			}
			P_ADD_PARAM;
		}
		break;
	/*
	 * End of line, expect '\n'. If found, we may have a message
	 * to handle (unless there were errors). Anything else is an
	 * error.
	 */
	case IRC_PARSER_LF:
		if (next == '\n')
		{
			has_msg = IRC_ParserInMessage;
			P_SET_STATE(START);
		}
		else
		{
			P_AUTO_ERROR;
		}
		break;
	/*
	 * Error recovery: wait for an '\r'.
	 */
	case IRC_PARSER_RECOVERY:
		if (next == '\r')
		{
			P_SET_STATE(LF);
		}
		break;
	}

	return has_msg && !IRC_ParserError;
}

#ifdef DEBUG_DUMP_IRC
/**
 * @brief Debugging function that dumps the IRC message.
 */
static void IRC_DumpMessage()
{
	int i;

	Com_Printf("----------- IRC MESSAGE RECEIVED -----------\n");
	Com_Printf(" (pfx) nick/server .... [%.3d]%s\n", IRC_Length(pfx_nickOrServer), IRC_String(pfx_nickOrServer));
	Com_Printf(" (pfx) user ........... [%.3d]%s\n", IRC_Length(pfx_user), IRC_String(pfx_user));
	Com_Printf(" (pfx) host ........... [%.3d]%s\n", IRC_Length(pfx_host), IRC_String(pfx_host));
	Com_Printf(" command string ....... [%.3d]%s\n", IRC_Length(cmd_string), IRC_String(cmd_string));
	Com_Printf(" arguments ............  %.3d\n", IRC_ReceivedMessage.arg_count);
	for (i = 0 ; i < IRC_ReceivedMessage.arg_count ; i++)
	{
		Com_Printf(" ARG %d = [%.3d]%s\n", i + 1, IRC_Length(arg_values[i]), IRC_String(arg_values[i]));
	}
}
#endif // DEBUG_DUMP_IRC

/*--------------------------------------------------------------------------*/
/* "SYSTEM" FUNCTIONS                                                       */
/*--------------------------------------------------------------------------*/

#ifdef WIN32
/**
 * @brief IRC_HandleError
 */
static void IRC_HandleError(void)
{
	switch (WSAGetLastError())
	{
	case 0:     // No error
		return;

	case WSANOTINITIALISED:
		Com_Printf("Unable to initialise socket.\n");
		break;
	case WSAEAFNOSUPPORT:
		Com_Printf("The specified address family is not supported.\n");
		break;
	case WSAEADDRNOTAVAIL:
		Com_Printf("Specified address is not available from the local machine.\n");
		break;
	case WSAECONNREFUSED:
		Com_Printf("The attempt to connect was forcefully rejected.\n");
		break;
	case WSAEDESTADDRREQ:
		Com_Printf("address destination address is required.\n");
		break;
	case WSAEFAULT:
		Com_Printf("The namelen argument is incorrect.\n");
		break;
	case WSAEINVAL:
		Com_Printf("The socket is not already bound to an address.\n");
		break;
	case WSAEISCONN:
		Com_Printf("The socket is already connected.\n");
		break;
	case WSAEADDRINUSE:
		Com_Printf("The specified address is already in use.\n");
		break;
	case WSAEMFILE:
		Com_Printf("No more file descriptors are available.\n");
		break;
	case WSAENOBUFS:
		Com_Printf("No buffer space available. The socket cannot be created.\n");
		break;
	case WSAEPROTONOSUPPORT:
		Com_Printf("The specified protocol is not supported.\n");
		break;
	case WSAEPROTOTYPE:
		Com_Printf("The specified protocol is the wrong type for this socket.\n");
		break;
	case WSAENETUNREACH:
		Com_Printf("The network can't be reached from this host at this time.\n");
		break;
	case WSAENOTSOCK:
		Com_Printf("The descriptor is not a socket.\n");
		break;
	case WSAETIMEDOUT:
		Com_Printf("Attempt timed out without establishing a connection.\n");
		break;
	case WSAESOCKTNOSUPPORT:
		Com_Printf("Socket type is not supported in this address family.\n");
		break;
	case WSAENETDOWN:
		Com_Printf("Network subsystem failure.\n");
		break;
	case WSAHOST_NOT_FOUND:
		Com_Printf("Authoritative Answer Host not found.\n");
		break;
	case WSATRY_AGAIN:
		Com_Printf("Non-Authoritative Host not found or SERVERFAIL.\n");
		break;
	case WSANO_RECOVERY:
		Com_Printf("Non recoverable errors, FORMERR, REFUSED, NOTIMP.\n");
		break;
	case WSANO_DATA:
		Com_Printf("Valid name, no data record of requested type.\n");
		break;
	case WSAEINPROGRESS:
		Com_Printf("address blocking Windows Sockets operation is in progress.\n");
		break;
	default:
		Com_Printf("Unknown connection error.\n");
		break;
	}

	WSASetLastError(0);
}
#else
static void IRC_HandleError(void)
{
	Com_Printf("IRC socket connection error: %s\n", strerror(errno));
}
#endif

#if defined MSG_NOSIGNAL
# define IRC_SEND_FLAGS MSG_NOSIGNAL
#else
# define IRC_SEND_FLAGS 0
#endif

/**
 * @brief Attempt to format then send a message to the IRC server. Will return
 * true on success, and false if an overflow occurred or if send() failed.
 * @param[in] format
 * @return
 */
static _attribute((format(printf, 1, 2))) int IRC_Send(const char *format, ...)
{
	char    buffer[IRC_SEND_BUF_SIZE + 1];
	va_list args;
	int     len, sent;

	// Format message
	va_start(args, format);
	len = vsnprintf(buffer, IRC_SEND_BUF_SIZE - 1, format, args);
	va_end(args);
	if (len >= IRC_SEND_BUF_SIZE - 1)
	{
		// This is a bug, return w/ a fatal error
		Com_Printf("...IRC: send buffer overflow (%d characters)\n", len);
		return IRC_CMD_FATAL;
	}

	// Add CRLF terminator
#if defined DEBUG_DUMP_IRC
	Com_Printf("SENDING IRC MESSAGE: %s\n", buffer);
#endif
	buffer[len++] = '\r';
	buffer[len++] = '\n';

	Com_DPrintf("IRC Send: %s\n", buffer);

	// Send message
	sent = send(IRC_Socket, buffer, len, IRC_SEND_FLAGS);
	if (sent < len)
	{
		IRC_HandleError();
		return IRC_CMD_RETRY;
	}

	return IRC_CMD_SUCCESS;
}

/*
 * This function is used to prevent the IRC thread from turning the CPU into
 * a piece of molten silicium while it waits for the server to send data.
 *
 * If data is received, SUCCESS is returned; otherwise, RETRY will be returned
 * on timeout and FATAL on error.
 */

#ifdef WIN32
# define SELECT_ARG 0
# define SELECT_CHECK (rv == -1 && WSAGetLastError() == WSAEINTR)
#else // defined __linux__ || defined __FreeBSD__ || defined __APPLE__
# define SELECT_ARG (IRC_Socket + 1)
# define SELECT_CHECK (rv == -1 && errno == EINTR)
#endif

/**
 * @brief IRC_Wait
 * @return
 */
static int IRC_Wait()
{
	struct timeval timeout;
	fd_set         read_set;
	int            rv;

	// Wait for data to be available
	do
	{
		FD_ZERO(&read_set);
		FD_SET(IRC_Socket, &read_set);
		timeout.tv_sec  = 0;
		timeout.tv_usec = IRC_TIMEOUT_US;
		rv              = select(SELECT_ARG, &read_set, NULL, NULL, &timeout);
	}
	while (SELECT_CHECK);

	// Something wrong happened
	if (rv < 0)
	{
		IRC_HandleError();
		return IRC_CMD_FATAL;
	}

	return (rv == 0) ? IRC_CMD_RETRY : IRC_CMD_SUCCESS;
}

/**
 * @brief Wait for some seconds.
 * @param[in] seconds
 */
static void IRC_Sleep(int seconds)
{
	int i;

	etl_assert(seconds > 0);
	for (i = 0 ; i < seconds * IRC_TIMEOUTS_PER_SEC && !IRC_QuitRequested ; i++)
	{
#ifdef WIN32
		Sleep(IRC_TIMEOUT_MS);
#else // defined __linux__
		usleep(IRC_TIMEOUT_US);
#endif
	}
}

/*--------------------------------------------------------------------------*/
/* RATE LIMITS                                                              */
/*--------------------------------------------------------------------------*/

/**
 * @brief Checks if some action can be effected using the rate limiter. If it can,
 * the rate limiter's status will be updated.
 * @param event_type
 * @return
 */
static ID_INLINE qboolean IRC_CheckEventRate(int event_type)
{
	if (IRC_RateLimiter[event_type] >= IRC_LIMIT_THRESHOLD * IRC_TIMEOUTS_PER_SEC)
	{
		return qfalse;
	}
	IRC_RateLimiter[event_type] += IRC_LIMIT_INCREASE * IRC_TIMEOUTS_PER_SEC;
	return qtrue;
}

/**
 * @brief Decrease all non-zero rate limiter entries.
 */
static ID_INLINE void IRC_UpdateRateLimiter()
{
	unsigned int i;

	for (i = 0 ; i < sizeof(IRC_RateLimiter) / sizeof(unsigned int) ; i++)
	{
		if (IRC_RateLimiter[i])
		{
			IRC_RateLimiter[i]--;
		}
	}
}

/**
 * @brief Initialise the rate limiter.
 */
static ID_INLINE void IRC_InitRateLimiter()
{
	unsigned int i;

	for (i = 0 ; i < sizeof(IRC_RateLimiter) / sizeof(unsigned int) ; i++)
	{
		IRC_RateLimiter[i] = 0;
	}
}

/*--------------------------------------------------------------------------*/
/* DISPLAY CODE                                                             */
/*--------------------------------------------------------------------------*/

/**
 * @brief IRC_NeutraliseString
 * @param[out] buffer
 * @param[in] source
 */
static void IRC_NeutraliseString(char *buffer, const char *source)
{
	char c;

	while (*source)
	{
		c = *source;

		if (IS_CNTRL(c))
		{
			*(buffer++) = ' ';
		}
		else if (c & 0x80)
		{
			*(buffer++) = '?';
		}
		else if (c == Q_COLOR_ESCAPE)
		{
			*(buffer++) = Q_COLOR_ESCAPE;
			*(buffer++) = Q_COLOR_ESCAPE;
		}
		else
		{
			*(buffer++) = c;
		}
		source++;
	}
	*buffer = 0;
}

/**
 * @brief IRC_Display
 * @param[in] event
 * @param[in] nick
 * @param[in] message
 */
static void IRC_Display(int event, const char *nick, const char *message)
{
	char       buffer[IRC_RECV_BUF_SIZE * 2];
	char       nick_copy[IRC_MAX_NICK_LEN * 2];
	char       message_copy[IRC_MAX_ARG_LEN * 2];
	const char *fmt_string;
	qboolean   has_nick;
	qboolean   has_message;

	// If we're quitting, just skip this
	if (IRC_QuitRequested)
	{
		return;
	}

	if (irc_mode->integer & IRCM_MUTE_CHANNEL)
	{
		return;
	}

	// Determine message format
	switch (IRC_EventType(event))
	{
	case IRC_EVT_SAY:
		has_nick = has_message = qtrue;
		if (IRC_EventIsSelf(event))
		{
			fmt_string = "^2<^7%s^2> %s";
		}
		else if (strstr(message, IRC_User.nick))
		{
			fmt_string = "^3<^7%s^3> %s";
		}
		else
		{
			fmt_string = "^1<^7%s^1> %s";
		}
		break;
	case IRC_EVT_ACT:
		has_nick = has_message = qtrue;
		if (IRC_EventIsSelf(event))
		{
			fmt_string = "^2* ^7%s^2 %s";
		}
		else if (strstr(message, IRC_User.nick))
		{
			fmt_string = "^3* ^7%s^3 %s";
		}
		else
		{
			fmt_string = "^1* ^7%s^1 %s";
		}
		break;
	case IRC_EVT_JOIN:
		has_message = qfalse;
		has_nick    = !IRC_EventIsSelf(event);
		if (has_nick)
		{
			fmt_string = "^5-> ^7%s^5 has entered the channel.";
		}
		else
		{
			fmt_string = "^2Joined IRC chat.";
		}
		break;
	case IRC_EVT_PART:
		// The AlienArena IRC client never parts, so it's
		// someone else.
		has_nick    = qtrue;
		has_message = (message[0] != 0);
		if (has_message)
		{
			fmt_string = "^5<- ^7%s^5 has left the channel: %s.";
		}
		else
		{
			fmt_string = "^5<- ^7%s^5 has left the channel.";
		}
		break;
	case IRC_EVT_QUIT:
		has_nick = !IRC_EventIsSelf(event);
		if (has_nick)
		{
			has_message = (message[0] != 0);
			if (has_message)
			{
				fmt_string = "^5<- ^7%s^5 has quit: %s.";
			}
			else
			{
				fmt_string = "^5<- ^7%s^5 has quit.";
			}
		}
		else
		{
			has_message = qtrue;
			fmt_string  = "^2Quit IRC chat: %s.";
		}
		break;
	case IRC_EVT_KICK:
		has_nick = has_message = qtrue;
		if (IRC_EventIsSelf(event))
		{
			fmt_string = "^2Kicked by ^7%s^2: %s.";
		}
		else
		{
			fmt_string = "^5<- ^7%s^5 has been kicked: %s.";
		}
		break;
	case IRC_EVT_NICK_CHANGE:
		has_nick = has_message = qtrue;
		if (IRC_EventIsSelf(event))
		{
			fmt_string = "^2** ^7%s^2 is now known as ^7%s^2.";
		}
		else
		{
			fmt_string = "^5** ^7%s^5 is now known as ^7%s^5.";
		}
		break;
	default:
		has_nick   = has_message = qfalse;
		fmt_string = "unknown message received\n";
		break;
	}

	// Neutralise required strings
	if (has_nick)
	{
		IRC_NeutraliseString(nick_copy, nick);
	}
	if (has_message)
	{
		IRC_NeutraliseString(message_copy, message);
	}

	// Format message
	if (has_nick && has_message)
	{
		sprintf(buffer, fmt_string, nick_copy, message_copy);
	}
	else if (has_nick)
	{
		sprintf(buffer, fmt_string, nick_copy);
	}
	else if (has_message)
	{
		sprintf(buffer, fmt_string, message_copy);
	}
	else
	{
		strncpy(buffer, fmt_string, IRC_RECV_BUF_SIZE * 2 - 1);
	}
	buffer[IRC_RECV_BUF_SIZE * 2 - 1] = 0;

#ifdef DEDICATED
	// FIXME: add filters for IRC_EventType?
	if (irc_mode->integer & IRCM_CHANNEL_TO_CHAT)
	{
		SV_SendServerCommand(NULL, "chat \"%s\"", buffer);
	}
#endif

	Com_Printf("^1IRC: %s\n", buffer);
}

/*--------------------------------------------------------------------------*/
/* IRC MESSAGE HANDLERS                                                     */
/*--------------------------------------------------------------------------*/

/**
 * @brief Send the user's nickname.
 * @return
 */
int IRC_SendNickname()
{
	return IRC_Send("NICK %s", IRC_User.nick);
}

/**
 * @brief Join the channel
 * @return
 */
static int IRC_JoinChannel()
{
	return IRC_Send("JOIN #%s", irc_channel->string);
}

/**
 * @brief Handles a PING by replying with a PONG.
 * @return
 */
static int IRCH_Ping()
{
	if (IRC_ReceivedMessage.arg_count == 1)
	{
		return IRC_Send("PONG :%s", IRC_String(arg_values[0]));
	}
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Handles server errors
 * @return
 */
static int IRCH_ServerError()
{
	if (IRC_ThreadStatus == IRC_THREAD_QUITTING)
	{
		return IRC_CMD_SUCCESS;
	}

	if (IRC_ReceivedMessage.arg_count == 1)
	{
		Com_Printf("IRC: server error - %s\n", IRC_String(arg_values[0]));
	}
	else
	{
		Com_Printf("IRC: server error\n");
	}
	return IRC_CMD_RETRY;
}

/*
 * @brief Some fatal error was received, the IRC thread must die.
 * @note unused
static int IRCH_FatalError()
{
    IRC_Display(IRC_MakeEvent(QUIT, 1), "", "fatal error");
    IRC_Send("QUIT :Something went wrong\n");
    return IRC_CMD_RETRY;
}
*/

#define RANDOM_NUMBER_CHAR ('0' + rand() % 10)

/**
 * @brief Nickname error.
 *
 * If received while the thread is in the SETNICK state, we might
 * want to try again. Otherwise, we ignore the error as it should
 * not have been received anyway.
 */
static int IRCH_NickError()
{
	int nicklen = strlen(IRC_User.nick);

	if (IRC_ThreadStatus == IRC_THREAD_SETNICK)
	{
		if (++IRC_User.nickattempts == 4)
		{
			IRC_Send("QUIT :Could not set nickname");
			return IRC_CMD_FATAL;
		}

		if (nicklen < 15)
		{
			IRC_User.nick[nicklen++] = RANDOM_NUMBER_CHAR;
		}
		else
		{
			int i;

			for (i = nicklen - 3 ; i < nicklen ; i++)
			{
				IRC_User.nick[i] = RANDOM_NUMBER_CHAR;
			}
		}

		IRC_SetTimeout(IRC_SendNickname, 2);
	}
	else
	{
		Com_Printf("...IRC: got spurious nickname error\n");
	}

	return IRC_CMD_SUCCESS;
}

/**
 * @brief Connection established, we will be able to join a channel
 * @return
 */
static int IRCH_Connected()
{
	if (IRC_ThreadStatus != IRC_THREAD_SETNICK)
	{
		IRC_Display(IRC_MakeEvent(QUIT, 1), "", "IRC client bug\n");
		IRC_Send("QUIT :ET: Legacy IRC bug!");
		return IRC_CMD_RETRY;
	}
	IRC_ThreadStatus = IRC_THREAD_CONNECTED;
	IRC_SetTimeout(&IRC_JoinChannel, 1);
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Received JOIN
 * @return
 */
static int IRCH_Joined()
{
	int event;

	if (IRC_ThreadStatus < IRC_THREAD_CONNECTED)
	{
		IRC_Display(IRC_MakeEvent(QUIT, 1), "", "IRC client bug\n");
		IRC_Send("QUIT :ET: Legacy IRC bug!");
		return IRC_CMD_RETRY;
	}

	if (!strcmp(IRC_String(pfx_nickOrServer), IRC_User.nick))
	{
		IRC_ThreadStatus = IRC_THREAD_JOINED;
		event            = IRC_MakeEvent(JOIN, 1);
	}
	else
	{
		event = IRC_MakeEvent(JOIN, 0);
	}
	IRC_Display(event, IRC_String(pfx_nickOrServer), NULL);
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Received PART
 * @return
 */
static int IRCH_Part()
{
	IRC_Display(IRC_MakeEvent(PART, 0), IRC_String(pfx_nickOrServer), IRC_String(arg_values[1]));
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Received QUIT
 * @return
 */
static int IRCH_Quit()
{
	IRC_Display(IRC_MakeEvent(QUIT, 0), IRC_String(pfx_nickOrServer), IRC_String(arg_values[0]));
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Received KICK
 * @return
 */
static int IRCH_Kick()
{
	if (!strcmp(IRC_String(arg_values[1]), IRC_User.nick))
	{
		IRC_Display(IRC_MakeEvent(KICK, 1), IRC_String(pfx_nickOrServer), IRC_String(arg_values[2]));
		if (irc_kick_rejoin->integer > 0)
		{
			IRC_ThreadStatus = IRC_THREAD_CONNECTED;
			IRC_SetTimeout(&IRC_JoinChannel, irc_kick_rejoin->integer);
		}
		else
		{
			IRC_Display(IRC_MakeEvent(QUIT, 1), "", "kicked from channel..\n");
			IRC_Send("QUIT :b&!");
			return IRC_CMD_FATAL;
		}
	}
	else
	{
		IRC_Display(IRC_MakeEvent(KICK, 0), IRC_String(arg_values[1]), IRC_String(arg_values[2]));
	}
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Received NICK
 * @details While the AA client does not support changing the current nickname,
 * it is still possible to receive a NICK applying to the connected user
 * because of e.g. OperServ's SVSNICK command.
 * @return
 */
static int IRCH_Nick()
{
	int event;

	if (IRC_ReceivedMessage.arg_count != 1)
	{
		return IRC_CMD_SUCCESS;
	}

	if (!strcmp(IRC_String(pfx_nickOrServer), IRC_User.nick))
	{
		strncpy(IRC_User.nick, IRC_String(arg_values[0]), 15);
		Com_Printf("%s\n", IRC_User.nick);
		event = IRC_MakeEvent(NICK_CHANGE, 1);
	}
	else
	{
		event = IRC_MakeEvent(NICK_CHANGE, 0);
	}
	IRC_Display(event, IRC_String(pfx_nickOrServer), IRC_String(arg_values[0]));
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Handles an actual message.
 * @param[in] is_channel
 * @param[in] string
 * @return
 */
static int IRC_HandleMessage(qboolean is_channel, const char *string)
{
	if (is_channel)
	{
		IRC_Display(IRC_MakeEvent(SAY, 0), IRC_String(pfx_nickOrServer), string);
		return IRC_CMD_SUCCESS;
	}

	if (IRC_CheckEventRate(IRC_RL_MESSAGE))
	{
		return IRC_Send("PRIVMSG %s :Sorry, the ET: Legacy IRC client does not support private messages", IRC_String(pfx_nickOrServer));
	}
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Splits a CTCP message into action and argument, then call
 * its handler (if there is one).
 * @param[in] is_channel
 * @param[in,out] string
 * @param[in] string_len
 * @return
 */
static int IRC_HandleCTCP(qboolean is_channel, char *string, int string_len)
{
	char *end_of_action;

	end_of_action = strchr(string, ' ');
	if (end_of_action == NULL)
	{
		end_of_action  = string + string_len - 1;
		*end_of_action = 0;
	}
	else
	{
		*(string + string_len - 1) = 0;
		*end_of_action             = 0;
		end_of_action++;
	}

#if defined DEBUG_DUMP_IRC
	Com_Printf("--- IRC/CTCP ---\n");
	Com_Printf(" Command:     %s\n Argument(s): %s\n", string, end_of_action);
#endif

	return IRC_ExecuteCTCPHandler(string, is_channel, end_of_action);
}

/**
 * @brief Received PRIVMSG.
 *
 * This is either an actual message (to the channel or to the user) or a
 * CTCP command (action, version, etc...)
 *
 * @return
 */
static int IRCH_PrivMsg()
{
	qboolean is_channel;

	if (IRC_ReceivedMessage.arg_count != 2)
	{
		return IRC_CMD_SUCCESS;
	}

	// Check message to channel (bail out if it isn't our channel)
	is_channel = IRC_String(arg_values[0])[0] == '#';
	if (is_channel && strcmp(&(IRC_String(arg_values[0])[1]), irc_channel->string))
	{
		return IRC_CMD_SUCCESS;
	}

	if (IRC_Length(arg_values[1]) > 2
	    && IRC_String(arg_values[1])[0] == 1
	    && IRC_String(arg_values[1])[IRC_Length(arg_values[1]) - 1] == 1)
	{
		return IRC_HandleCTCP(is_channel, IRC_String(arg_values[1]) + 1, IRC_Length(arg_values[1]) - 1);
	}

	return IRC_HandleMessage(is_channel, IRC_String(arg_values[1]));
}

/**
 * @brief User is banned. Leave and do not come back.
 * @return
 */
static int IRCH_Banned()
{
	IRC_Display(IRC_MakeEvent(QUIT, 1), "", "banned from channel..\n");
	IRC_Send("QUIT :b&!");
	return IRC_CMD_FATAL;
}

/*--------------------------------------------------------------------------*/
/* CTCP COMMAND HANDLERS                                                    */
/*--------------------------------------------------------------------------*/

/**
 * @brief Action command aka "/me"
 * @param[in] is_channel
 * @param[in] argument
 * @return
 */
static int CTCP_Action(qboolean is_channel, const char *argument)
{
	if (!*argument)
	{
		return IRC_CMD_SUCCESS;
	}

	if (is_channel)
	{
		IRC_Display(IRC_MakeEvent(ACT, 0), IRC_String(pfx_nickOrServer), argument);
		return IRC_CMD_SUCCESS;
	}

	if (IRC_CheckEventRate(IRC_RL_MESSAGE))
	{
		return IRC_Send("PRIVMSG %s :Sorry, the ET: Legacy IRC client does not support private messages", IRC_String(pfx_nickOrServer));
	}
	return IRC_CMD_SUCCESS;
}

/**
 * @brief PING requests
 * @param[in] is_channel
 * @param[in] argument
 * @return
 */
static int CTCP_Ping(qboolean is_channel, const char *argument)
{
	if (is_channel || !IRC_CheckEventRate(IRC_RL_PING))
	{
		return IRC_CMD_SUCCESS;
	}

	if (*argument)
	{
		return IRC_Send("NOTICE %s :\001PING %s\001", IRC_String(pfx_nickOrServer), argument);
	}

	return IRC_Send("NOTICE %s :\001PING\001", IRC_String(pfx_nickOrServer));
}

/**
 * @brief VERSION requests, let's advertise AA a lil'.
 * @param[in] is_channel
 * @param argument - unused
 * @return
 */
static int CTCP_Version(qboolean is_channel, const char *argument)
{
	if (is_channel || !IRC_CheckEventRate(IRC_RL_VERSION))
	{
		return IRC_CMD_SUCCESS;
	}

	return IRC_Send("NOTICE %s :\001VERSION " Q3_VERSION "\001", IRC_String(pfx_nickOrServer));
}

/*--------------------------------------------------------------------------*/
/* MESSAGE SENDING                                                          */
/*--------------------------------------------------------------------------*/

/**
 * @def IRC_MAX_SEND_LEN
 * @brief Maximal message length
 */
#define IRC_MAX_SEND_LEN    400

/**
 * @struct irc_sendqueue_t
 * @brief The message sending queue is used to avoid having to send stuff from the
 * game's main thread, as it could block or cause mix-ups in the printing
 * function.
 */
struct irc_sendqueue_t
{
	qboolean has_content;
	qboolean is_action;
	char string[IRC_MAX_SEND_LEN];
};

/**
 * @def IRC_SendQueue_Process
 * @brief Length of the IRC send queue
 */
#define IRC_SENDQUEUE_SIZE  16

/**
 * @var IRC_SendQueue_Process
 * @brief Index of the next message to process
 */
static int IRC_SendQueue_Process = 0;

/**
 * @var IRC_SendQueue_Write
 * @brief Index of the next message to write
 */
static int IRC_SendQueue_Write = 0;

/**
 * @var IRC_SendQueue
 * @brief The queue
 */
static struct irc_sendqueue_t IRC_SendQueue[IRC_SENDQUEUE_SIZE];

/**
 * @brief Initialise the send queue.
 */
static ID_INLINE void IRC_InitSendQueue()
{
	Com_Memset(&IRC_SendQueue, 0, sizeof(IRC_SendQueue));
}

/**
 * @brief Writes an entry to the send queue.
 * @param[in] is_action
 * @param[in] string
 * @return
 */
static qboolean IRC_AddSendItem(qboolean is_action, const char *string)
{
	if (IRC_SendQueue[IRC_SendQueue_Write].has_content)
	{
		return qfalse;
	}

	Q_strncpyz(IRC_SendQueue[IRC_SendQueue_Write].string, string, IRC_MAX_SEND_LEN);
	IRC_SendQueue[IRC_SendQueue_Write].is_action   = is_action;
	IRC_SendQueue[IRC_SendQueue_Write].has_content = qtrue;
	IRC_SendQueue_Write                            = (IRC_SendQueue_Write + 1) % IRC_SENDQUEUE_SIZE;
	return qtrue;
}

/**
 * @brief Sends an IRC message (console command).
 */
void IRC_Say()
{
	char     m_sendstring[480];
	qboolean send_result;

	if (Cmd_Argc() < 2)
	{
		Com_Printf("usage: irc_say <text>\n");
		return;
	}

	if (IRC_ThreadStatus != IRC_THREAD_JOINED)
	{
		Com_Printf("IRC: Not connected\n");
		return;
	}

	Com_Memset(m_sendstring, 0, sizeof(m_sendstring));
	strncpy(m_sendstring, Cmd_Args(), 479);
	if (m_sendstring[0] == 0)
	{
		return;
	}

	if ((m_sendstring[0] == '/' || m_sendstring[0] == '.') && !Q_stricmpn(m_sendstring + 1, "me ", 3) && m_sendstring[4] != 0)
	{
		send_result = IRC_AddSendItem(qtrue, m_sendstring + 4);
	}
	else
	{
		send_result = IRC_AddSendItem(qfalse, m_sendstring);
	}

	if (!send_result)
	{
		Com_Printf("IRC: flood detected, message not sent\n");
	}
}

/**
 * @brief Processes the next item on the send queue, if any.
 * @return
 */
static qboolean IRC_ProcessSendQueue()
{
	const char *fmt_string;
	int        event, rv;

	if (!IRC_SendQueue[IRC_SendQueue_Process].has_content)
	{
		return qtrue;
	}

	if (IRC_SendQueue[IRC_SendQueue_Process].is_action)
	{
		fmt_string = "PRIVMSG #%s :\001ACTION %s\001\n";
		event      = IRC_MakeEvent(ACT, 1);
	}
	else
	{
		fmt_string = "PRIVMSG #%s :%s\n";
		event      = IRC_MakeEvent(SAY, 1);
	}

	rv = IRC_Send(fmt_string, irc_channel->string, IRC_SendQueue[IRC_SendQueue_Process].string);
	if (rv == IRC_CMD_SUCCESS)
	{
		IRC_Display(event, IRC_User.nick, IRC_SendQueue[IRC_SendQueue_Process].string);
	}
	IRC_SendQueue[IRC_SendQueue_Process].has_content = qfalse;
	IRC_SendQueue_Process                            = (IRC_SendQueue_Process + 1) % IRC_SENDQUEUE_SIZE;
	return (rv == IRC_CMD_SUCCESS);
}

/**
 * @brief Attempts to receive data from the server. If data is received, parse it
 * and attempt to execute a handler for each complete message.
 * @return
 */
static int IRC_ProcessData(void)
{
	char buffer[IRC_RECV_BUF_SIZE];
	int  i, len, err_code;

	len = recv(IRC_Socket, buffer, IRC_RECV_BUF_SIZE, 0);

	// Handle errors / remote disconnects
	if (len <= 0)
	{
		if (len < 0)
		{
			IRC_HandleError();
		}
		IRC_ThreadStatus = IRC_THREAD_QUITTING;
		return IRC_CMD_RETRY;
	}

	for (i = 0 ; i < len ; i++)
	{
		if (IRC_Parser(buffer[i]))
		{
#ifdef DEBUG_DUMP_IRC
			IRC_DumpMessage();
#endif // DEBUG_DUMP_IRC
			err_code = IRC_ExecuteHandler();
			if (err_code != IRC_CMD_SUCCESS)
			{
				return err_code;
			}
		}
	}

	return IRC_CMD_SUCCESS;
}

/**
 * @brief IRC_GetName
 * @param[in] name
 * @return
 */
char *IRC_GetName(const char *name)
{
	int  i       = 0, j = 0, k = 0;
	int  namelen = strlen(name);
	char c;
	char *retName;

	retName = (char *) Com_Allocate((sizeof(char) * namelen) + 1);
	Com_Memset(retName, 0, (sizeof(char) * namelen) + 1);

	for (; j < namelen; j++)
	{
		if (!name[i])
		{
			continue;
		}
		if (name[i] == Q_COLOR_ESCAPE)
		{
			i++;
			if (name[i] != Q_COLOR_ESCAPE)
			{
				if (name[i])
				{
					i++;
				}
				continue;
			}
		}

		c = name[i++];

		// First letter in nickname cannot be a special char
		if (k == 0 && (!IS_ALPHA(c) || IS_SPECL(c)))
		{
			continue;
		}

		if (!(IS_CLEAN(c)))
		{
			c = '_';
		}
		retName[k++] = c;
	}

	return retName;
}

/**
 * @brief Prepares the user record which is used when issuing the USER command.
 * @param[in] name
 * @return
 */
static qboolean IRC_InitialiseUser(const char *name)
{
	char *source;

	// Strip color chars for the player's name, and remove special
	// characters
	if (irc_mode->integer & IRCM_AUTO_OVERRIDE_NICKNAME)
	{
		source = IRC_GetName(irc_nickname->string);
	}
	else
	{
		source = IRC_GetName(name);
	}

	IRC_User.nickattempts = 1;

	Q_strncpyz(IRC_User.nick, source, sizeof(IRC_User.nick));

	Q_strncpyz(IRC_User.username, source, sizeof(IRC_User.username));

	// Set static address FIXME: add user authentication and cvar for this?
	strcpy(IRC_User.email, "mymail@mail.com");

	Com_DPrintf("IRC nick: %s username %s\n", IRC_User.nick, IRC_User.username);

	Com_Dealloc(source);

	return (strlen(IRC_User.nick) > 0);
}

#define CHECK_SHUTDOWN { if (IRC_QuitRequested) { return IRC_CMD_FATAL; } }
#define CHECK_SHUTDOWN_CLOSE { if (IRC_QuitRequested) { closesocket(IRC_Socket); return IRC_CMD_FATAL; } }

/**
 * @brief Establishes the IRC connection, sets the nick, etc...
 * @return
 */
static int IRC_AttemptConnection()
{
	struct sockaddr_in address;               // socket address
	struct addrinfo    hint;                  // provides hints about the type of socket the caller supports
	struct addrinfo    *res = NULL;           // contains response information about the host
	char               hostName[128];        // host name
	char               name[MAX_NAME_LENGTH]; // player's name
	int                err_code;
	char               port[5] = "6667";

	Com_Memset(&hint, 0, sizeof(hint));

	hint.ai_family   = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	CHECK_SHUTDOWN;
	Com_Memset(&address.sin_zero, 0, sizeof(address.sin_zero));
	Com_Printf("IRC: connecting to server %s:%i\n", irc_server->string, irc_port->integer);

#ifdef DEDICATED
	Q_strncpyz(name, Cvar_VariableString("sv_hostname"), sizeof(name));
#else
	// Force players to use a non-default name
	Q_strncpyz(name, Cvar_VariableString("name"), sizeof(name));
#endif
	// FIXME: add default player and server name
	if (!Q_stricmpn(name, "player", 7) || name[0] == '\0')
	{
		Com_Printf("...IRC: rejected due to unusable player name '%s'\n", name);
		return IRC_CMD_FATAL;
	}

	// Prepare USER record
	if (!IRC_InitialiseUser(name))
	{
		Com_Printf("...IRC: rejected due to mostly unusable player name '%s'\n", name);
		return IRC_CMD_FATAL;
	}

	// Check socket address
	if (irc_port->integer <= 0 || irc_port->integer >= 65536)
	{
		Com_Printf("...IRC: invalid port number, defaulting to 6667\n");
	}
	else
	{
		Q_strncpyz(port, irc_port->string, sizeof(port));
	}

	// Find server address
	Q_strncpyz(hostName, irc_server->string, sizeof(hostName));
	if (getaddrinfo(hostName, port, &hint, &res) != 0)
	{
		Com_Printf("...IRC: unknown server\n");
		return IRC_CMD_FATAL;
	}

	// Create socket
	CHECK_SHUTDOWN;
	if ((IRC_Socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == INVALID_SOCKET)
	{
		freeaddrinfo(res);
		IRC_HandleError();
		return IRC_CMD_FATAL;
	}

	// Attempt connection
	if ((connect(IRC_Socket, res->ai_addr, res->ai_addrlen)) != 0)
	{
		freeaddrinfo(res);
		closesocket(IRC_Socket);
		Com_Printf("...IRC connection refused.\n");
		return IRC_CMD_RETRY;
	}

	freeaddrinfo(res);

	// Send username and nick name
	CHECK_SHUTDOWN_CLOSE;
	err_code = IRC_Send("USER %s %s %s :%s", IRC_User.username, IRC_User.email, hostName, IRC_User.nick);
	if (err_code == IRC_CMD_SUCCESS)
	{
		err_code = IRC_SendNickname();
	}
	if (err_code != IRC_CMD_SUCCESS)
	{
		closesocket(IRC_Socket);
		return err_code;
	}

	// Initialise parser and set thread state
	IRC_ParserState  = IRC_PARSER_START;
	IRC_ThreadStatus = IRC_THREAD_SETNICK;

	CHECK_SHUTDOWN_CLOSE;
	Com_Printf("IRC: connected to server\n");
	return IRC_CMD_SUCCESS;
}

/**
 * @brief Attempt to connect to the IRC server for the first time.
 * Only retry a few times and assume the server's dead/does not exist if
 * connection can't be established.
 * @return
 */
static qboolean IRC_InitialConnect()
{
	int err_code = IRC_CMD_SUCCESS;
	int retries  = 3;
	int rc_delay = irc_reconnect_delay->integer;

	IRC_ThreadStatus = IRC_THREAD_CONNECTING;

	if (rc_delay < 5)
	{
		rc_delay = 5;
	}

	do
	{
		// If we're re-attempting a connection, wait a little bit,
		// or we might just piss the server off.
		if (err_code == IRC_CMD_RETRY)
		{
			IRC_Sleep(rc_delay);
		}
		else if (IRC_QuitRequested)
		{
			return qfalse;
		}

		err_code = IRC_AttemptConnection();
	}
	while (err_code == IRC_CMD_RETRY && --retries > 0);

	return (err_code == IRC_CMD_SUCCESS);
}

/**
 * @brief Attempt to reconnect to the IRC server. Only stop trying on fatal errors
 * or if the thread's status is set to QUITTING.
 * @return
 */
static int IRC_Reconnect()
{
	int err_code = IRC_CMD_SUCCESS;
	int rc_delay = irc_reconnect_delay->integer;

	IRC_ThreadStatus = IRC_THREAD_CONNECTING;

	if (rc_delay < 5)
	{
		rc_delay = 5;
	}

	do
	{
		IRC_Sleep((err_code == IRC_CMD_SUCCESS) ? (rc_delay >> 1) : rc_delay);
		if (IRC_QuitRequested)
		{
			return IRC_CMD_FATAL;
		}
		err_code = IRC_AttemptConnection();
	}
	while (err_code == IRC_CMD_RETRY);

	return err_code;
}

/**
 * @brief Once the initial connection has been established, either
 * 1) pump messages or 2) handle delayed functions. Try re-connecting if
 * connection is lost.
 */
static void IRC_MainLoop()
{
	int err_code;

	// Connect to server
	if (!IRC_InitialConnect())
	{
		return;
	}

	do
	{
		do
		{
			// If we must quit, send the command.
			if (IRC_QuitRequested && IRC_ThreadStatus != IRC_THREAD_QUITTING)
			{
				IRC_ThreadStatus = IRC_THREAD_QUITTING;
				IRC_Display(IRC_MakeEvent(QUIT, 1), "", "quit from menu\n");
				err_code = IRC_Send("QUIT : %s", Q3_VERSION);
			}
			else
			{
				// Wait for data or 1s timeout
				err_code = IRC_Wait();
				if (err_code == IRC_CMD_SUCCESS)
				{
					// We have some data, process it
					err_code = IRC_ProcessData();
				}
				else if (err_code == IRC_CMD_RETRY)
				{
					// Timed out, handle timers and update rate limiter
					err_code = IRC_ProcessDEQueue();
					IRC_UpdateRateLimiter();
				}
				else
				{
					// Disconnected, but reconnection should be attempted
					err_code = IRC_CMD_RETRY;
				}

				if (err_code == IRC_CMD_SUCCESS && !IRC_QuitRequested)
				{
					err_code = IRC_ProcessSendQueue() ? IRC_CMD_SUCCESS : IRC_CMD_RETRY;
				}
			}
		}
		while (err_code == IRC_CMD_SUCCESS);
		closesocket(IRC_Socket);

		// If we must quit, let's skip trying to reconnect
		if (IRC_QuitRequested || err_code == IRC_CMD_FATAL)
		{
			return;
		}

		// Reconnect to server
		do
		{
			err_code = IRC_Reconnect();
		}
		while (err_code == IRC_CMD_RETRY);
	}
	while (err_code != IRC_CMD_FATAL);
}

/**
 * @brief Main function of the IRC thread: initialise command handlers,
 * start the main loop, and uninitialise handlers after the loop
 * exits.
 */
static void IRC_Thread()
{
	// Init. send queue & rate limiter
	IRC_InitSendQueue();
	IRC_InitRateLimiter();
	IRC_InitHandlers();

	// Init. IRC handlers
	IRC_AddHandler("PING", &IRCH_Ping);                         // Ping request
	IRC_AddHandler("ERROR", &IRCH_ServerError);                 // Server error
	IRC_AddHandler("JOIN", &IRCH_Joined);                       // Channel join
	IRC_AddHandler("PART", &IRCH_Part);                         // Channel part
	IRC_AddHandler("QUIT", &IRCH_Quit);                         // Client quit
	IRC_AddHandler("PRIVMSG", &IRCH_PrivMsg);                   // Message or CTCP
	IRC_AddHandler("KICK", &IRCH_Kick);                         // Kick
	IRC_AddHandler("NICK", &IRCH_Nick);                         // Nick change
	IRC_AddHandler("001", &IRCH_Connected);                     // Connection established
	IRC_AddHandler("404", &IRCH_Banned);                        // Banned (when sending message)
	IRC_AddHandler("432", &IRCH_NickError);                     // Erroneous nick name
	IRC_AddHandler("468", &IRCH_NickError);                     // Erroneous nick name
	IRC_AddHandler("433", &IRCH_NickError);                     // Nick name in use
	IRC_AddHandler("474", &IRCH_Banned);                        // Banned (when joining)

	// Init. CTCP handlers
	IRC_AddCTCPHandler("ACTION", &CTCP_Action);                 // "/me"
	IRC_AddCTCPHandler("PING", &CTCP_Ping);
	IRC_AddCTCPHandler("VERSION", &CTCP_Version);

	// Enter loop
	IRC_MainLoop();

	// Clean up
	Com_Printf("IRC: disconnected from server\n");
	IRC_FlushDEQueue();
	IRC_FreeHandlers();
	IRC_SetThreadDead();
}

/*
 * Caution: IRC_SystemThreadProc(), IRC_StartThread() and IRC_WaitThread()
 *  have separate "VARIANTS".
 *
 * Note different prototypes for IRC_SystemThreadProc() and completely
 * different IRC_StartThread()/IRC_WaitThread() implementations.
 */
#ifdef WIN32

/****** THREAD HANDLING - WINDOWS VARIANT ******/

static HANDLE IRC_ThreadHandle = NULL;

/**
 * @brief IRC_SystemThreadProc
 * @param dummy - unused
 * @return
 */
static DWORD WINAPI IRC_SystemThreadProc(LPVOID dummy)
{
	IRC_Thread();
	return 0;
}

/**
 * @brief IRC_StartThread
 */
static void IRC_StartThread()
{
	if (IRC_ThreadHandle == NULL)
	{
		IRC_ThreadHandle = CreateThread(NULL, 0, IRC_SystemThreadProc, NULL, 0, NULL);
	}
}

/**
 * @brief IRC_SetThreadDead
 */
static void IRC_SetThreadDead()
{
	IRC_ThreadStatus = IRC_THREAD_DEAD;
	IRC_ThreadHandle = NULL;
}

/**
 * @brief IRC_WaitThread
 */
static void IRC_WaitThread()
{
	if (IRC_ThreadHandle != NULL)
	{
		if (IRC_ThreadStatus != IRC_THREAD_DEAD)
		{
			WaitForSingleObject(IRC_ThreadHandle, 10000);
			CloseHandle(IRC_ThreadHandle);
		}
		IRC_ThreadHandle = NULL;
	}
}

#else // defined __linux__ || defined __APPLE__ || defined __FreeBSD__

/****** THREAD HANDLING - UNIX VARIANT ******/

static pthread_t IRC_ThreadHandle = (pthread_t) NULL;

/**
 * @brief IRC_SystemThreadProc
 * @param dummy
 * @return
 */
static void *IRC_SystemThreadProc(void *dummy)
{
	IRC_Thread();
	return NULL;
}

/**
 * @brief IRC_StartThread
 */
static void IRC_StartThread(void)
{
	if (IRC_ThreadHandle == (pthread_t) NULL)
	{
		pthread_create(&IRC_ThreadHandle, NULL, IRC_SystemThreadProc, NULL);
	}
}

/**
 * @brief IRC_SetThreadDead
 */
static void IRC_SetThreadDead()
{
	IRC_ThreadStatus = IRC_THREAD_DEAD;
	IRC_ThreadHandle = (pthread_t) NULL;
}

/**
 * @brief IRC_WaitThread
 */
static void IRC_WaitThread()
{
	if (IRC_ThreadHandle != (pthread_t) NULL)
	{
		if (IRC_ThreadStatus != IRC_THREAD_DEAD)
		{
			pthread_join(IRC_ThreadHandle, NULL);
		}
		IRC_ThreadHandle = (pthread_t) NULL;
	}
}

#endif

/**
 * @brief IRC_Connect
 */
void IRC_Connect(void)
{
	if (IRC_ThreadStatus != IRC_THREAD_DEAD)
	{
		Com_Printf("...IRC thread is already running\n");
		return;
	}
	IRC_QuitRequested = qfalse;
	IRC_ThreadStatus  = IRC_THREAD_INITIALISING;
	IRC_StartThread();
}

/**
 * @brief IRC_Init
 */
void IRC_Init(void)
{
	irc_mode            = Cvar_Get("irc_mode", "0", CVAR_ARCHIVE);
	irc_server          = Cvar_Get("irc_server", "irc.freenode.net", CVAR_ARCHIVE);
	irc_channel         = Cvar_Get("irc_channel", "etlegacy", CVAR_ARCHIVE);
	irc_port            = Cvar_Get("irc_port", "6667", CVAR_ARCHIVE);
	irc_nickname        = Cvar_Get("irc_nickname", "ETLClient", CVAR_ARCHIVE);
	irc_kick_rejoin     = Cvar_Get("irc_kick_rejoin", "0", CVAR_ARCHIVE);
	irc_reconnect_delay = Cvar_Get("irc_reconnect_delay", "100", CVAR_ARCHIVE);
}

/**
 * @brief IRC_InitiateShutdown
 */
void IRC_InitiateShutdown(void)
{
	IRC_QuitRequested = qtrue;
}

/**
 * @brief IRC_WaitShutdown
 */
void IRC_WaitShutdown(void)
{
	IRC_WaitThread();
}

/**
 * @brief IRC_IsConnected
 * @return
 */
qboolean IRC_IsConnected(void)
{
	// get IRC status
	return (IRC_ThreadStatus == IRC_THREAD_JOINED);
}

/**
 * @brief IRC_IsRunning
 * @return
 */
qboolean IRC_IsRunning(void)
{
	// return IRC status
	return (IRC_ThreadStatus != IRC_THREAD_DEAD);
}
