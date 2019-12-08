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
 * @file cl_net_chan.c
 */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "client.h"

/**
 * @brief CL_Netchan_Encode
 * @details First 12 bytes of the data are always:
 * long serverId;
 * long messageAcknowledge;
 * long reliableAcknowledge;
 * @param[in,out] msg
 */
static void CL_Netchan_Encode(msg_t *msg)
{
	int      serverId, messageAcknowledge, reliableAcknowledge;
	int      i, index, srdc, sbit;
	qboolean soob;
	byte     key, *string;

	if (msg->cursize <= CL_ENCODE_START)
	{
		return;
	}

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->bit       = 0;
	msg->readcount = 0;
	msg->oob       = qfalse;

	serverId            = MSG_ReadLong(msg);
	messageAcknowledge  = MSG_ReadLong(msg);
	reliableAcknowledge = MSG_ReadLong(msg);

	msg->oob       = soob;
	msg->bit       = sbit;
	msg->readcount = srdc;

	string = (byte *)clc.serverCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)];
	index  = 0;

	key = clc.challenge ^ serverId ^ messageAcknowledge;
	for (i = CL_ENCODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last received now acknowledged server command
		if (!string[index])
		{
			index = 0;
		}

		key ^= string[index] << (i & 1);

		index++;
		// encode the data with this key
		*(msg->data + i) = (*(msg->data + i)) ^ key;
	}
}

/**
 * @brief CL_Netchan_Decode
 * @details First four bytes of the data are always:
 * long reliableAcknowledge;
 * @param[in,out] msg
 */
static void CL_Netchan_Decode(msg_t *msg)
{
	long     reliableAcknowledge, i, index;
	byte     key, *string;
	int      srdc, sbit;
	qboolean soob;

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->oob = qfalse;

	reliableAcknowledge = MSG_ReadLong(msg);

	msg->oob       = soob;
	msg->bit       = sbit;
	msg->readcount = srdc;

	string = (byte *) clc.reliableCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)];
	index  = 0;
	// xor the client challenge with the netchan sequence number (need something that changes every message)
	key = clc.challenge ^ LittleLong(*(unsigned *)msg->data);
	for (i = msg->readcount + CL_DECODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last sent and with this message acknowledged client command
		if (!string[index])
		{
			index = 0;
		}

		if ((!Com_IsCompatible(&clc.agent, 0x1) && (byte)string[index] > 127) || string[index] == '%')
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
 * @brief CL_Netchan_TransmitNextFragment
 * @param[in] chan
 */
void CL_Netchan_TransmitNextFragment(netchan_t *chan)
{
	Netchan_TransmitNextFragment(chan);
}

/**
 * @brief CL_WriteBinaryMessage
 * @param[in] msg
 */
static void CL_WriteBinaryMessage(msg_t *msg)
{
	if (!clc.binaryMessageLength)
	{
		return;
	}

	MSG_Uncompressed(msg);

	if ((msg->cursize + clc.binaryMessageLength) >= msg->maxsize)
	{
		clc.binaryMessageOverflowed = qtrue;
		return;
	}

	MSG_WriteData(msg, clc.binaryMessage, clc.binaryMessageLength);
	clc.binaryMessageLength     = 0;
	clc.binaryMessageOverflowed = qfalse;
}

/**
 * @brief CL_Netchan_Transmit
 * @param[in] chan
 * @param[in] msg
 */
void CL_Netchan_Transmit(netchan_t *chan, msg_t *msg)
{
	MSG_WriteByte(msg, clc_EOF);
	CL_WriteBinaryMessage(msg);

	CL_Netchan_Encode(msg);

	Netchan_Transmit(chan, msg->cursize, msg->data);
}

int newsize = 0;

/**
 * @brief CL_Netchan_Process
 * @param[in] chan
 * @param[in] msg
 * @return
 */
qboolean CL_Netchan_Process(netchan_t *chan, msg_t *msg)
{
	int ret;

	ret = Netchan_Process(chan, msg);
	if (!ret)
	{
		return qfalse;
	}

	CL_Netchan_Decode(msg);

	newsize += msg->cursize;
	return qtrue;
}
