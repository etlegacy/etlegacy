/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file g_popup.c
 * @brief The popups a client answers with 'vote yes|no [id]': the pending yes/no prompts of
 * each client (complaints, fireteam applications, invitations, propositions and the
 * autofireteam questions) and the team votes, which run next to the vote of all players.
 *
 * Each client has a queue of up to MAX_PENDING_PROMPTS prompts. A prompt is sent to the client
 * with 'prompt <id> <kind> <from> <from2> <msLeft>' (again when its time changes) and removed
 * with 'promptclose <id> <answer>' (answer 1 yes, 0 no, -1 withdrawn). The client shows the
 * prompts one at a time, the oldest first, then the team vote, then the vote of all players; an
 * answer without an id goes to the same popup (see Cmd_Vote_f()). Only the prompt on screen
 * counts down: the next one gets its full PROMPT_TIME when it comes to the front, so a prompt
 * never runs out while it waits.
 *
 * A team vote is sent only to the players of that team, with
 * 'tvote <id> <startTime> <yes> <no> <voted> <text>', and 'tvote 0' when it ends.
 */

#include "g_local.h"

/**
 * @brief G_NextPopupId hands out the id that ties a prompt or vote on a client to its answer
 * @return a running id, 1-32767 (0 means no id)
 */
int G_NextPopupId(void)
{
	level.popupSeq = level.popupSeq % 32767 + 1;
	return level.popupSeq;
}

/**
 * @brief G_PromptIsRequest
 * @param[in] kind
 * @return qtrue for the prompts another player sends (application, invitation, proposition):
 * these are refused when the list is full instead of pushing an older prompt out
 */
static qboolean G_PromptIsRequest(int kind)
{
	return kind == POPUP_APPLICATION || kind == POPUP_INVITATION || kind == POPUP_PROPOSITION;
}

/**
 * @brief G_PromptLegacyFields returns the clientPersistant_t fields that show the newest
 * pending prompt of a kind to Lua scripts
 * @param[in] cl
 * @param[in] kind
 * @param[out] client the client the prompt is about, NULL if the kind has none
 * @param[out] client2 the proposer of a proposition, NULL for the other kinds
 * @param[out] endTime NULL if the kind has no legacy fields
 */
static void G_PromptLegacyFields(gclient_t *cl, int kind, int **client, int **client2, int **endTime)
{
	*client  = NULL;
	*client2 = NULL;
	*endTime = NULL;

	switch (kind)
	{
	case POPUP_COMPLAINT:
		*client  = &cl->pers.complaintClient;
		*endTime = &cl->pers.complaintEndTime;
		break;
	case POPUP_APPLICATION:
		*client  = &cl->pers.applicationClient;
		*endTime = &cl->pers.applicationEndTime;
		break;
	case POPUP_INVITATION:
		*client  = &cl->pers.invitationClient;
		*endTime = &cl->pers.invitationEndTime;
		break;
	case POPUP_PROPOSITION:
		*client  = &cl->pers.propositionClient;
		*client2 = &cl->pers.propositionClient2;
		*endTime = &cl->pers.propositionEndTime;
		break;
	case POPUP_AFT:
		*endTime = &cl->pers.autofireteamEndTime;
		break;
	case POPUP_AFTC:
		*endTime = &cl->pers.autofireteamCreateEndTime;
		break;
	case POPUP_AFTJ:
		*endTime = &cl->pers.autofireteamJoinEndTime;
		break;
	default:
		break;
	}
}

/**
 * @brief G_PromptNewest
 * @param[in] cl
 * @param[in] kind
 * @return the slot of the newest pending prompt of the kind, -1 if there is none
 */
static int G_PromptNewest(gclient_t *cl, int kind)
{
	int i, best = -1;

	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] == kind && cl->pers.promptEndTime[i] > level.time &&
		    (best < 0 || cl->pers.promptSeq[i] > cl->pers.promptSeq[best]))
		{
			best = i;
		}
	}

	return best;
}

/**
 * @brief G_PromptUpdateLegacyKind writes the newest pending prompt of a kind into its old
 * single-prompt fields (client -1 and end time 0 when there is none)
 * @param[in,out] cl
 * @param[in] kind
 */
static void G_PromptUpdateLegacyKind(gclient_t *cl, int kind)
{
	int *client, *client2, *endTime;
	int slot = G_PromptNewest(cl, kind);

	G_PromptLegacyFields(cl, kind, &client, &client2, &endTime);
	if (!endTime)
	{
		return;
	}

	if (client)
	{
		*client = slot >= 0 ? cl->pers.promptFrom[slot] : -1;
	}
	if (client2)
	{
		*client2 = slot >= 0 ? cl->pers.promptFrom2[slot] : -1;
	}
	*endTime = slot >= 0 ? cl->pers.promptEndTime[slot] : 0;
}

/**
 * @brief G_PromptUpdateLegacy keeps the old single-prompt fields (complaintClient,
 * invitationEndTime, ...) showing the newest pending prompt of each kind, as Lua scripts
 * written for one prompt per kind expect
 * @param[in,out] cl
 */
void G_PromptUpdateLegacy(gclient_t *cl)
{
	int kind;

	for (kind = POPUP_COMPLAINT; kind <= POPUP_AFTJ; kind++)
	{
		G_PromptUpdateLegacyKind(cl, kind);
	}
}

/**
 * @brief G_PromptSend sends a prompt to its client, or again after its end time changed
 * @param[in] clientNum
 * @param[in] slot
 */
static void G_PromptSend(int clientNum, int slot)
{
	gclient_t *cl = level.clients + clientNum;

	trap_SendServerCommand(clientNum, va("prompt %i %i %i %i %i", cl->pers.promptId[slot], cl->pers.promptKind[slot],
	                                     cl->pers.promptFrom[slot], cl->pers.promptFrom2[slot],
	                                     MAX(0, cl->pers.promptEndTime[slot] - level.time - PROMPT_CLIENT_MARGIN)));
}

/**
 * @brief G_PromptClear removes a prompt from a client's list
 * @param[in] clientNum
 * @param[in] slot
 * @param[in] answer 1 yes, 0 no: the client shows the answer on the prompt; -1 the prompt was withdrawn
 * @param[in] notify qfalse when the client has dropped the prompt already (it timed out there)
 * @note doesn't update the legacy fields, see G_PromptUpdateLegacy()
 */
void G_PromptClear(int clientNum, int slot, int answer, qboolean notify)
{
	gclient_t *cl = level.clients + clientNum;

	if (notify && cl->pers.promptKind[slot] != POPUP_ANY)
	{
		trap_SendServerCommand(clientNum, va("promptclose %i %i", cl->pers.promptId[slot], answer));
	}

	cl->pers.promptKind[slot]    = POPUP_ANY;
	cl->pers.promptId[slot]      = 0;
	cl->pers.promptFrom[slot]    = -1;
	cl->pers.promptFrom2[slot]   = -1;
	cl->pers.promptEndTime[slot] = 0;
	cl->pers.promptSeq[slot]     = 0;
	cl->pers.promptShown[slot]   = 0;
}

/**
 * @brief G_PromptScheduleKeep starts the timer of the prompt that has come to the front (and
 * sends it again with its full time). The prompts behind it get the time they would end at if
 * every prompt before them timed out. Then the old single-prompt fields are updated.
 * @param[in] clientNum
 * @param[in] keepKind a kind whose old fields keep their values if no prompt of it is left
 * (a script's write that withdrew it), POPUP_ANY for none
 */
static void G_PromptScheduleKeep(int clientNum, int keepKind)
{
	gclient_t *cl = level.clients + clientNum;
	int       order[MAX_PENDING_PROMPTS];
	int       n = 0, i, j;

	// the pending prompts, oldest first
	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] == POPUP_ANY)
		{
			continue;
		}

		for (j = n; j > 0 && cl->pers.promptSeq[order[j - 1]] > cl->pers.promptSeq[i]; j--)
		{
			order[j] = order[j - 1];
		}
		order[j] = i;
		n++;
	}

	for (i = 0; i < n; i++)
	{
		int slot = order[i];

		if (i == 0)
		{
			if (!cl->pers.promptShown[slot])
			{
				cl->pers.promptShown[slot]   = 1;
				cl->pers.promptEndTime[slot] = level.time + PROMPT_TIME;
				G_PromptSend(clientNum, slot);
			}
		}
		else
		{
			cl->pers.promptShown[slot]   = 0;
			cl->pers.promptEndTime[slot] = cl->pers.promptEndTime[order[0]] + i * PROMPT_TIME;
		}
	}

	for (i = POPUP_COMPLAINT; i <= POPUP_AFTJ; i++)
	{
		if (i != keepKind || G_PromptNewest(cl, i) >= 0)
		{
			G_PromptUpdateLegacyKind(cl, i);
		}
	}
}

/**
 * @brief G_PromptSchedule brings the next prompt to the front, see G_PromptScheduleKeep()
 * @param[in] clientNum
 */
static void G_PromptSchedule(int clientNum)
{
	G_PromptScheduleKeep(clientNum, POPUP_ANY);
}

/**
 * @brief G_PromptAdd adds a prompt to a client's list and sends it. The same request again
 * (same kind, from and from2) replaces the pending one.
 * @param[in] clientNum
 * @param[in] kind
 * @param[in] from the client the prompt is about, -1 if none
 * @param[in] from2 the proposer of a proposition, else -1
 * @param[in] endTime level.time the prompt expires if it is on screen at once, 0 for the
 * default (PROMPT_TIME); a queued prompt gets PROMPT_TIME when it comes to the front
 * @return the prompt id, 0 if the list is full (only for requests from other players: other
 * prompts push the oldest one out)
 */
int G_PromptAdd(int clientNum, popupKind_t kind, int from, int from2, int endTime)
{
	gclient_t *cl = level.clients + clientNum;
	int       slot = -1, i;
	int       empty = -1, oldest = -1;

	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] == POPUP_ANY || cl->pers.promptEndTime[i] <= level.time)
		{
			if (empty < 0)
			{
				empty = i;
			}
			continue;
		}

		if (cl->pers.promptKind[i] == kind && cl->pers.promptFrom[i] == from && cl->pers.promptFrom2[i] == from2)
		{
			slot = i;
		}

		if (oldest < 0 || cl->pers.promptSeq[i] < cl->pers.promptSeq[oldest])
		{
			oldest = i;
		}
	}

	if (slot >= 0)
	{
		// replaced: the client drops the old one and queues the new one
		G_PromptClear(clientNum, slot, -1, qtrue);
	}
	else if (empty >= 0)
	{
		// an expired prompt timed out on the client already
		slot = empty;
		G_PromptClear(clientNum, slot, -1, qfalse);
	}
	else if (G_PromptIsRequest(kind))
	{
		return 0;
	}
	else
	{
		slot = oldest;
		G_PromptClear(clientNum, slot, -1, qtrue);
	}

	cl->pers.promptKind[slot]    = kind;
	cl->pers.promptId[slot]      = G_NextPopupId();
	cl->pers.promptFrom[slot]    = from;
	cl->pers.promptFrom2[slot]   = from2;
	cl->pers.promptEndTime[slot] = level.time + PROMPT_TIME;
	cl->pers.promptSeq[slot]     = ++level.promptSeq;
	cl->pers.promptShown[slot]   = 0;

	// at the front it is sent from here, with its timer running
	G_PromptSchedule(clientNum);

	if (cl->pers.promptShown[slot] && endTime > level.time)
	{
		// a script's end time
		cl->pers.promptEndTime[slot] = endTime;
		G_PromptSend(clientNum, slot);
		G_PromptUpdateLegacyKind(cl, kind);
	}
	else if (!cl->pers.promptShown[slot])
	{
		// queued
		G_PromptSend(clientNum, slot);
	}

	return cl->pers.promptId[slot];
}

/**
 * @brief G_PromptFindId
 * @param[in] cl
 * @param[in] id
 * @return the slot of the pending prompt with that id, -1 if it is gone
 */
int G_PromptFindId(gclient_t *cl, int id)
{
	int i;

	if (id <= 0)
	{
		return -1;
	}

	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] != POPUP_ANY && cl->pers.promptId[i] == id && cl->pers.promptEndTime[i] > level.time)
		{
			return i;
		}
	}

	return -1;
}

/**
 * @brief G_PromptFront finds the prompt the client shows: the oldest pending one, whose timer
 * runs (so it is also the one with the least time left; the client uses the same order)
 * @param[in] cl
 * @return the slot, -1 if no prompt is pending
 */
int G_PromptFront(gclient_t *cl)
{
	int i, best = -1;

	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] == POPUP_ANY || cl->pers.promptEndTime[i] <= level.time)
		{
			continue;
		}

		if (best < 0 || cl->pers.promptSeq[i] < cl->pers.promptSeq[best])
		{
			best = i;
		}
	}

	return best;
}

/**
 * @brief G_PromptRemoveKinds withdraws a client's own prompts of the given kinds
 * @param[in] clientNum
 * @param[in] kindMask POPUP_MASK() of the kinds
 */
void G_PromptRemoveKinds(int clientNum, int kindMask)
{
	gclient_t *cl = level.clients + clientNum;
	int       i;

	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] != POPUP_ANY && (kindMask & POPUP_MASK(cl->pers.promptKind[i])))
		{
			G_PromptClear(clientNum, i, -1, cl->pers.promptEndTime[i] > level.time);
		}
	}

	G_PromptSchedule(clientNum);
}

/**
 * @brief G_PromptRemoveAbout withdraws the prompts of the other clients that are about a
 * client (it is their 'from' or 'from2'), of the given kinds
 * @param[in] clientNum
 * @param[in] kindMask POPUP_MASK() of the kinds
 */
void G_PromptRemoveAbout(int clientNum, int kindMask)
{
	int i, j;

	for (i = 0; i < level.maxclients; i++)
	{
		gclient_t *cl     = level.clients + i;
		qboolean  changed = qfalse;

		if (i == clientNum || cl->pers.connected == CON_DISCONNECTED)
		{
			continue;
		}

		for (j = 0; j < MAX_PENDING_PROMPTS; j++)
		{
			if (cl->pers.promptKind[j] != POPUP_ANY && (kindMask & POPUP_MASK(cl->pers.promptKind[j])) &&
			    (cl->pers.promptFrom[j] == clientNum || cl->pers.promptFrom2[j] == clientNum))
			{
				G_PromptClear(i, j, -1, cl->pers.promptEndTime[j] > level.time);
				changed = qtrue;
			}
		}

		if (changed)
		{
			G_PromptSchedule(i);
		}
	}
}

/**
 * @brief G_PromptJoinedFireteam withdraws the fireteam prompts that are void once a client is
 * on a fireteam: its invitations (their leaders are told), its autofireteam questions, and its
 * applications and propositions on the other clients
 * @param[in] clientNum the client that joined or created a fireteam
 * @param[in] leaderNum the leader of that fireteam
 */
void G_PromptJoinedFireteam(int clientNum, int leaderNum)
{
	gclient_t *cl = level.clients + clientNum;
	int       i;

	for (i = 0; i < MAX_PENDING_PROMPTS; i++)
	{
		if (cl->pers.promptKind[i] == POPUP_INVITATION && cl->pers.promptEndTime[i] > level.time)
		{
			int inviter = cl->pers.promptFrom[i];

			if (inviter != leaderNum && inviter >= 0 && inviter < level.maxclients && level.clients[inviter].pers.connected == CON_CONNECTED)
			{
				trap_SendServerCommand(inviter, va("cpm \"%s^7 is in another fireteam now\"", cl->pers.netname));
			}
		}
	}

	G_PromptRemoveKinds(clientNum, POPUP_MASK(POPUP_INVITATION) | POPUP_MASK(POPUP_AFTC) | POPUP_MASK(POPUP_AFTJ));
	G_PromptRemoveAbout(clientNum, POPUP_MASK(POPUP_APPLICATION) | POPUP_MASK(POPUP_PROPOSITION));
}

/**
 * @brief G_PromptSyncLegacy applies a Lua script's change of the old single-prompt fields
 * (complaintClient, invitationEndTime, ...) to the prompt list: an end time in the past
 * withdraws the newest prompt of that kind, a new end time or client changes or creates it.
 * A field written on its own (the client before the end time) waits until the prompt is complete.
 * @param[in,out] cl
 */
void G_PromptSyncLegacy(gclient_t *cl)
{
	int clientNum = cl - level.clients;
	int kind;

	for (kind = POPUP_COMPLAINT; kind <= POPUP_AFTJ; kind++)
	{
		int *client, *client2, *endTime;
		int slot = G_PromptNewest(cl, kind);
		int want, want2, wantEnd, has, has2, hasEnd;

		G_PromptLegacyFields(cl, kind, &client, &client2, &endTime);
		if (!endTime)
		{
			continue;
		}

		want    = client ? *client : -1;
		want2   = client2 ? *client2 : -1;
		wantEnd = *endTime;
		has     = slot >= 0 ? cl->pers.promptFrom[slot] : -1;
		has2    = slot >= 0 ? cl->pers.promptFrom2[slot] : -1;
		hasEnd  = slot >= 0 ? cl->pers.promptEndTime[slot] : 0;

		if (want == has && want2 == has2 && wantEnd == hasEnd)
		{
			continue;
		}

		if (wantEnd > level.time)
		{
			if (client && (want < 0 || want >= level.maxclients))
			{
				// not complete yet
				continue;
			}

			if (slot >= 0 && want == has && want2 == has2)
			{
				// same prompt, new end time: the client restarts its timer
				cl->pers.promptEndTime[slot] = wantEnd;
				G_PromptSend(clientNum, slot);
			}
			else
			{
				if (slot >= 0)
				{
					G_PromptClear(clientNum, slot, -1, qtrue);
				}
				G_PromptAdd(clientNum, (popupKind_t)kind, want, want2, wantEnd);
			}
		}
		else if (slot >= 0)
		{
			G_PromptClear(clientNum, slot, -1, qtrue);
		}
		else
		{
			continue;
		}

		// the next prompt comes to the front; only a kind with a prompt left shows it: the
		// fields of a withdrawn one keep what the script wrote
		G_PromptScheduleKeep(clientNum, kind);
	}
}

/**
 * @brief G_PromptRunFrame drops the prompt on screen when its time is up (the client times it
 * out itself) and starts the timer of the next one
 */
void G_PromptRunFrame(void)
{
	int i, j;

	for (i = 0; i < level.maxclients; i++)
	{
		gclient_t *cl     = level.clients + i;
		qboolean  changed = qfalse;

		if (cl->pers.connected == CON_DISCONNECTED)
		{
			continue;
		}

		for (j = 0; j < MAX_PENDING_PROMPTS; j++)
		{
			if (cl->pers.promptKind[j] != POPUP_ANY && cl->pers.promptEndTime[j] <= level.time)
			{
				G_PromptClear(i, j, -1, qfalse);
				changed = qtrue;
			}
		}

		if (changed)
		{
			G_PromptSchedule(i);
		}
	}
}

/**
 * @brief G_PromptAnswered removes a prompt the client answered and brings the next one to the front
 * @param[in] clientNum
 * @param[in] slot
 * @param[in] yes
 */
void G_PromptAnswered(int clientNum, int slot, qboolean yes)
{
	G_PromptClear(clientNum, slot, yes ? 1 : 0, qtrue);
	G_PromptSchedule(clientNum);
}

/**
 * @brief G_TeamVote
 * @param[in] team
 * @return the team vote of Axis or Allies, NULL for the other teams
 */
voteInfo_t *G_TeamVote(team_t team)
{
	if (team == TEAM_AXIS)
	{
		return &level.teamVoteInfo[0];
	}
	if (team == TEAM_ALLIES)
	{
		return &level.teamVoteInfo[1];
	}
	return NULL;
}

/**
 * @brief G_TeamVoteSendTo sends a client the state of its team's vote ('tvote 0' if none runs)
 * @param[in] clientNum
 */
void G_TeamVoteSendTo(int clientNum)
{
	gclient_t  *cl   = level.clients + clientNum;
	voteInfo_t *vote = G_TeamVote(cl->sess.sessionTeam);
	char       desc[VOTE_MAXSTRING];
	char       *s;

	if (cl->pers.connected != CON_CONNECTED || (g_entities[clientNum].r.svFlags & SVF_BOT))
	{
		return;
	}

	if (!vote || !vote->voteTime)
	{
		trap_SendServerCommand(clientNum, "tvote 0");
		return;
	}

	// the text goes in quotes
	Q_strncpyz(desc, vote->voteDesc, sizeof(desc));
	for (s = desc; *s; s++)
	{
		if (*s == '"')
		{
			*s = '\'';
		}
	}

	trap_SendServerCommand(clientNum, va("tvote %i %i %i %i %i \"%s\"", vote->voteId, vote->voteTime, vote->voteYes, vote->voteNo,
	                                     cl->pers.teamVoteVoted == vote->voteId, desc));
}

/**
 * @brief G_TeamVoteBroadcast sends the state of a team's vote to the players of that team
 * @param[in] team
 */
void G_TeamVoteBroadcast(team_t team)
{
	int i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		int clientNum = level.sortedClients[i];

		if (level.clients[clientNum].sess.sessionTeam == team)
		{
			G_TeamVoteSendTo(clientNum);
		}
	}
}

/**
 * @brief G_VoteCancelForCaller cancels the votes a client called (on disconnect or team change)
 * @param[in] clientNum
 * @param[in] reason "disconnected", "switched team"
 */
void G_VoteCancelForCaller(int clientNum, const char *reason)
{
	gentity_t *ent = g_entities + clientNum;
	int       t;

	if (level.voteInfo.voteTime && level.voteInfo.voteCaller == clientNum)
	{
		AP(va("cpm \"^1Vote CANCELED! Caller %s\n\"", reason));
		G_LogPrintf("Vote canceled: %s (caller %s %s)\n", level.voteInfo.voteString, ent->client->pers.netname, reason);
		level.voteInfo.voteTime     = 0;
		level.voteInfo.voteCanceled = 1;
		trap_SetConfigstring(CS_VOTE_TIME, ""); // so the counter goes away
	}

	for (t = 0; t < 2; t++)
	{
		voteInfo_t *vote = &level.teamVoteInfo[t];
		team_t     team  = t ? TEAM_ALLIES : TEAM_AXIS;

		if (vote->voteTime && vote->voteCaller == clientNum)
		{
			G_TeamCommand(team, va("cpm \"^1Team vote CANCELED! Caller %s\n\"", reason));
			G_LogPrintf("Team vote canceled: %s (caller %s %s)\n", vote->voteString, ent->client->pers.netname, reason);
			vote->voteTime     = 0;
			vote->voteCanceled = 0;
			G_TeamVoteBroadcast(team);
		}
	}
}

/**
 * @brief G_VoteExecute runs a passed vote. The vote functions read their argument from
 * level.voteInfo, so a team vote is put there for the call.
 * @param[in] vote
 */
void G_VoteExecute(voteInfo_t *vote)
{
	char value[VOTE_MAXSTRING];
	char string[MAX_STRING_CHARS];
	int  team, caller;

	if (vote == &level.voteInfo)
	{
		vote->vote_fn(NULL, 0, NULL, NULL, qfalse);
		return;
	}

	Q_strncpyz(value, level.voteInfo.vote_value, sizeof(value));
	Q_strncpyz(string, level.voteInfo.voteString, sizeof(string));
	team   = level.voteInfo.voteTeam;
	caller = level.voteInfo.voteCaller;

	Q_strncpyz(level.voteInfo.vote_value, vote->vote_value, sizeof(level.voteInfo.vote_value));
	Q_strncpyz(level.voteInfo.voteString, vote->voteString, sizeof(level.voteInfo.voteString));
	level.voteInfo.voteTeam   = vote->voteTeam;
	level.voteInfo.voteCaller = vote->voteCaller;

	vote->vote_fn(NULL, 0, NULL, NULL, qfalse);

	Q_strncpyz(level.voteInfo.vote_value, value, sizeof(level.voteInfo.vote_value));
	Q_strncpyz(level.voteInfo.voteString, string, sizeof(level.voteInfo.voteString));
	level.voteInfo.voteTeam   = team;
	level.voteInfo.voteCaller = caller;
}
