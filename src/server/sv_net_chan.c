/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 * @file sv_net_chan.c
 */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

/**
 * @brief SV_Netchan_Encode
 * @param[in] client
 * @param[in,out] msg
 * @param[in,out] commandString
 *
 * @note first four bytes of the data are always:
 * long reliableAcknowledge;
 */
static void SV_Netchan_Encode(client_t *client, msg_t *msg, char *commandString)
{
	long     i, index;
	byte     key, *string;
	int      srdc, sbit;
	qboolean soob;

	if (msg->cursize < SV_ENCODE_START)
	{
		return;
	}

	// NOTE: saving pos, reading reliableAck, restoring, not using it .. useless?
	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->bit       = 0;
	msg->readcount = 0;
	msg->oob       = qfalse;

	MSG_ReadLong(msg);

	msg->oob       = soob;
	msg->bit       = sbit;
	msg->readcount = srdc;

	string = (byte *)commandString;
	index  = 0;
	// xor the client challenge with the netchan sequence number
	key = client->challenge ^ client->netchan.outgoingSequence;
	for (i = SV_ENCODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last received and with this message acknowledged client command
		if (!string[index])
		{
			index = 0;
		}

		key ^= string[index] << (i & 1);

		index++;
		// encode the data with this key
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}

/**
 * @brief SV_Netchan_Decode
 * @param[in] client
 * @param[in,out] msg
 *
 * @note first 12 bytes of the data are always:
 * long serverId;
 * long messageAcknowledge;
 * long reliableAcknowledge;
 */
static void SV_Netchan_Decode(client_t *client, msg_t *msg)
{
	int      serverId, messageAcknowledge, reliableAcknowledge;
	int      i;
	int      index = 0;
	int      srdc = msg->readcount;
	int      sbit = msg->bit;
	qboolean soob = msg->oob;
	byte     key, *string;

	msg->oob = qfalse;

	serverId            = MSG_ReadLong(msg);
	messageAcknowledge  = MSG_ReadLong(msg);
	reliableAcknowledge = MSG_ReadLong(msg);

	msg->oob       = soob;
	msg->bit       = sbit;
	msg->readcount = srdc;

	string = (byte *)client->reliableCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)];

	key = client->challenge ^ serverId ^ messageAcknowledge;
	for (i = msg->readcount + SV_DECODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last sent and acknowledged server command
		if (!string[index])
		{
			index = 0;
		}

		if ((!Com_IsCompatible(&client->agent, 0x1) && (byte)string[index] > 127) || string[index] == '%')
		{
			string[index] = '.';
		}

		key ^= string[index] << (i & 1);

		index++;
		// decode the data with this key
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}

/**
 * @brief SV_Netchan_ClearQueue
 * @param[in,out] client
 */
void SV_Netchan_ClearQueue(client_t *client)
{
	netchan_buffer_t *netbuf, *next;

	for (netbuf = client->netchan_start_queue; netbuf; netbuf = next)
	{
		next = netbuf->next;
		Z_Free(netbuf);
	}

	client->netchan_start_queue = NULL;
	client->netchan_end_queue   = &client->netchan_start_queue;
}

/**
 * @brief SV_Netchan_TransmitNextInQueue
 * @param[in,out] client
 */
void SV_Netchan_TransmitNextInQueue(client_t *client)
{
	netchan_buffer_t *netbuf;

	Com_DPrintf("Netchan_TransmitNextFragment: popping a queued message for transmit\n");
	netbuf = client->netchan_start_queue;

	SV_Netchan_Encode(client, &netbuf->msg, netbuf->lastClientCommandString);

	Netchan_Transmit(&client->netchan, netbuf->msg.cursize, netbuf->msg.data);

	// pop from queue
	client->netchan_start_queue = netbuf->next;
	if (!client->netchan_start_queue)
	{
		Com_DPrintf("Netchan_TransmitNextFragment: emptied queue\n");
		client->netchan_end_queue = &client->netchan_start_queue;
	}
	else
	{
		Com_DPrintf("Netchan_TransmitNextFragment: remaining queued message\n");
	}

	Z_Free(netbuf);
}

/**
 * @brief SV_Netchan_TransmitNextFragment
 * @param[in] client
 * @return
 */
int SV_Netchan_TransmitNextFragment(client_t *client)
{
	if (client->netchan.unsentFragments)
	{
		Netchan_TransmitNextFragment(&client->netchan);
		return SV_RateMsec(client);
	}
	else if (client->netchan_start_queue)
	{
		SV_Netchan_TransmitNextInQueue(client);
		return SV_RateMsec(client);
	}

	return -1;
}

/**
 * @brief SV_WriteBinaryMessage
 * @param[in] msg
 * @param[in,out] cl
 */
static void SV_WriteBinaryMessage(msg_t *msg, client_t *cl)
{
	if (!cl->binaryMessageLength)
	{
		return;
	}

	MSG_Uncompressed(msg);

	if ((msg->cursize + cl->binaryMessageLength) >= msg->maxsize)
	{
		cl->binaryMessageOverflowed = qtrue;
		return;
	}

	MSG_WriteData(msg, cl->binaryMessage, cl->binaryMessageLength);
	cl->binaryMessageLength     = 0;
	cl->binaryMessageOverflowed = qfalse;
}

/**
 * @brief SV_Netchan_Transmit
 *
 * @details If there are some unsent fragments (which may happen if the snapshots
 * and the gamestate are fragmenting, and collide on send for instance)
 * then buffer them and make sure they get sent in correct order
 *
 * @param[in,out] client
 * @param[in] msg
 */
void SV_Netchan_Transmit(client_t *client, msg_t *msg)
{
	MSG_WriteByte(msg, svc_EOF);
	SV_WriteBinaryMessage(msg, client);

	if (client->netchan.unsentFragments || client->netchan_start_queue)
	{
		netchan_buffer_t *netbuf;
		Com_DPrintf("SV_Netchan_Transmit: unsent fragments, stacked\n");
		netbuf = (netchan_buffer_t *)Z_Malloc(sizeof(netchan_buffer_t));
		// store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending
		MSG_Copy(&netbuf->msg, netbuf->msgBuffer, sizeof(netbuf->msgBuffer), msg);
		Q_strncpyz(netbuf->lastClientCommandString, client->lastClientCommandString, sizeof(netbuf->lastClientCommandString));
		netbuf->next = NULL;
		// insert it in the queue, the message will be encoded and sent later
		*client->netchan_end_queue = netbuf;
		client->netchan_end_queue  = &(*client->netchan_end_queue)->next;
	}
	else
	{
		SV_Netchan_Encode(client, msg, client->lastClientCommandString);
		Netchan_Transmit(&client->netchan, msg->cursize, msg->data);
	}
}

/**
 * @brief SV_Netchan_Process
 * @param[in] client
 * @param[in] msg
 * @return
 */
qboolean SV_Netchan_Process(client_t *client, msg_t *msg)
{
	int ret = Netchan_Process(&client->netchan, msg);

	if (!ret)
	{
		return qfalse;
	}

	SV_Netchan_Decode(client, msg);

	return qtrue;
}
