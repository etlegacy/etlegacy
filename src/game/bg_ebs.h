/*
 * Copyright (C) 2022 Gian 'myT' Schellenbaum.
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
 */
/**
 * @file bg_ebs.h
 */

typedef struct
{
	entityState_t *es;
	int field; // field index
	int bit;   // bit offset in the field
	int count; // total bits read/written
	qboolean read;
	qboolean cleared;
} entityBitStream_t;

extern const int ebs_maxIntBits;

// treats entityState_t as a generic float array and bit stream container
void        EBS_InitWrite(entityBitStream_t *ebs, entityState_t *es, qboolean clear);
void        EBS_InitRead(entityBitStream_t *ebs, const entityState_t *es);
void        EBS_WriteBits(entityBitStream_t *ebs, int data, int bits);
void        EBS_WriteBitsWithSign(entityBitStream_t *ebs, int data, int bitsLeftToRead);
int         EBS_ReadBits(entityBitStream_t *ebs, int bits);
int         EBS_ReadBitsWithSign(entityBitStream_t *ebs, int bitsLeftToRead);
void        EBS_Skip(entityBitStream_t *ebs, int bits);
void        EBS_WriteFloat(entityState_t *es, int index, float data);
float       EBS_ReadFloat(entityState_t *es, int index);

#ifdef ETLEGACY_DEBUG
#define EBS_TESTS 1
#endif

#ifdef EBS_TESTS
void        EBS_RunTests(void);
void        EBS_ValidateTables(void);
void        EBS_WriteTestPayload(entityBitStream_t *ebs);
void        EBS_ValidateTestPayload(entityBitStream_t *ebs);
#endif

// ET_EBS_SHOUTCAST fields sizes
#define EBS_SHOUTCAST_VERSION_SIZE 4
#define EBS_SHOUTCAST_CLIENTNUM_SIZE 6

#define EBS_SHOUTCAST_SLOTMASK_SIZE 6
#define EBS_SHOUTCAST_HEALTH_SIZE 9
#define EBS_SHOUTCAST_AMMOCLIP_SIZE 10
#define EBS_SHOUTCAST_AMMO_SIZE 10
#define EBS_SHOUTCAST_PLAYER_SIZE (EBS_SHOUTCAST_SLOTMASK_SIZE + EBS_SHOUTCAST_HEALTH_SIZE + EBS_SHOUTCAST_AMMOCLIP_SIZE + EBS_SHOUTCAST_AMMO_SIZE)
