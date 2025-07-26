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
 * @file bg_ebs.c
 */

#include "../qcommon/q_shared.h"
#include "bg_ebs.h"

// in total, an entityState_t struct provides:
// 766 generic bits, of which we use ebs_maxIntBits
// 24 floats

typedef struct
{
	int byteOffset;
	int numBits;
} intField_t;

// fields are in network order, same as the engine
// selected fields do not provide forward compatibility
#define ESF(Field, Bits) { offsetof(entityState_t, Field), Bits }
static const intField_t entityIntFields[] =
{
	//ESF(eType,         8),  // @NOTE: we can't override this! it's needed for the client to know what to do
	ESF(eFlags,          24),
	ESF(pos.trType,      8),
	ESF(pos.trTime,      32),
	ESF(pos.trDuration,  32),

	ESF(apos.trType,     8),
	ESF(apos.trTime,     32),
	ESF(apos.trDuration, 32),

	ESF(time,            32),
	ESF(time2,           32),

	ESF(otherEntityNum,  GENTITYNUM_BITS),
	ESF(otherEntityNum2, GENTITYNUM_BITS),
	ESF(groundEntityNum, GENTITYNUM_BITS),

	ESF(loopSound,       8),
	ESF(constantLight,   32),
	ESF(dl_intensity,    32),

	ESF(modelindex,      9),
	ESF(modelindex2,     9),
	ESF(frame,           16),
	ESF(clientNum,       8),
	ESF(solid,           24),
	ESF(event,           10),
	ESF(eventParm,       8),
	ESF(eventSequence,   8),

	ESF(events[0],       8),
	ESF(events[1],       8),
	ESF(events[2],       8),
	ESF(events[3],       8),

	ESF(eventParms[0],   8),
	ESF(eventParms[1],   8),
	ESF(eventParms[2],   8),
	ESF(eventParms[3],   8),

	ESF(powerups,        16),
	ESF(weapon,          8),
	ESF(legsAnim,        ANIM_BITS),
	ESF(torsoAnim,       ANIM_BITS),
	ESF(density,         10),
	ESF(dmgFlags,        32),
	ESF(onFireStart,     32),
	ESF(onFireEnd,       32),
	ESF(nextWeapon,      8),
	ESF(teamNum,         8),
	ESF(effect1Time,     32),
	ESF(effect2Time,     32),
	ESF(effect3Time,     32),
	ESF(animMovetype,    4),
	ESF(aiState,         2)
};
#undef ESF
const int ebs_fieldCount = ARRAY_LEN(entityIntFields);
const int ebs_maxIntBits = 758;

// fields are in network order, same as the engine
#define ESF(Field) (offsetof(entityState_t, Field))
static const int entityFloatFields[] =
{
	ESF(pos.trBase[0]),
	ESF(pos.trBase[1]),
	ESF(pos.trBase[2]),
	ESF(pos.trDelta[0]),
	ESF(pos.trDelta[1]),
	ESF(pos.trDelta[2]),
	ESF(apos.trBase[1]),
	ESF(apos.trBase[0]),
	ESF(apos.trBase[2]),
	ESF(apos.trDelta[0]),
	ESF(apos.trDelta[1]),
	ESF(apos.trDelta[2]),
	ESF(origin[0]),
	ESF(origin[1]),
	ESF(origin[2]),
	ESF(origin2[0]),
	ESF(origin2[1]),
	ESF(origin2[2]),
	ESF(angles[0]),
	ESF(angles[1]),
	ESF(angles[2]),
	ESF(angles2[0]),
	ESF(angles2[1]),
	ESF(angles2[2])
};
#undef ESF

void EBS_InitWrite(entityBitStream_t *ebs, entityState_t *es, qboolean clear)
{
	int i;

	ebs->es      = es;
	ebs->field   = 0;
	ebs->bit     = 0;
	ebs->count   = 0;
	ebs->read    = qfalse;
	ebs->cleared = clear;

	if (clear)
	{
		for ( i = 0; i < ARRAY_LEN(entityIntFields); ++i )
		{
			byte * const field = (byte *)es + entityIntFields[i].byteOffset;
			*(int *)field = 0;
		}
	}
}

void EBS_InitRead(entityBitStream_t *ebs, const entityState_t *es)
{
	ebs->es    = (entityState_t *)es;
	ebs->field = 0;
	ebs->bit   = 0;
	ebs->count = 0;
	ebs->read  = qtrue;
}

void EBS_Skip(entityBitStream_t *ebs, int bits)
{
	int fieldIndex    = ebs->field;
	int fieldBitIndex = ebs->bit;

	//etl_assert(ebs->read == qtrue);

	while (bits > 0)
	{
		const intField_t *f;
		int              bitsToSkip;

		if (fieldIndex >= ebs_fieldCount)
		{
			break;
		}

		f          = &entityIntFields[fieldIndex];
		bitsToSkip = MIN(bits, f->numBits - fieldBitIndex);

		fieldBitIndex += bitsToSkip;
		bits          -= bitsToSkip;
		ebs->count    += bitsToSkip;
		if (fieldBitIndex >= f->numBits)
		{
			fieldBitIndex = 0;
			fieldIndex++;
		}
	}

	ebs->field = fieldIndex;
	ebs->bit   = fieldBitIndex;
}

void EBS_WriteBits(entityBitStream_t *ebs, int data, int bitsLeftToWrite)
{
	int fieldIndex    = ebs->field;
	int fieldBitIndex = ebs->bit;

	etl_assert(ebs->read == qfalse);

	if (ebs->count + bitsLeftToWrite > ebs_maxIntBits)
	{
		etl_assert(0);
		return;
	}

	while (bitsLeftToWrite > 0)
	{
		const intField_t *f;
		int              *out;
		int              i, bitsToWrite;

		if (fieldIndex >= ebs_fieldCount)
		{
			break;
		}

		f   = &entityIntFields[fieldIndex];
		out = ( int * )((byte *)ebs->es + f->byteOffset);
		// out         = (int *)ebs->es + (f->byteOffset >> 2);
		bitsToWrite = MIN(bitsLeftToWrite, f->numBits - fieldBitIndex);

		for ( i = 0; i < bitsToWrite; ++i )
		{
			const int bitValue = data & 1;
			if (bitValue)
			{
				*out |= (bitValue << fieldBitIndex);
			}
			else if (!ebs->cleared)
			{
				*out &= ~(1 << fieldBitIndex);
			}
			fieldBitIndex++;
			data >>= 1;
		}

		bitsLeftToWrite -= bitsToWrite;
		ebs->count      += bitsToWrite;
		if (fieldBitIndex >= f->numBits)
		{
			fieldBitIndex = 0;
			fieldIndex++;
		}
	}

	ebs->field = fieldIndex;
	ebs->bit   = fieldBitIndex;
}

void EBS_WriteBitsWithSign(entityBitStream_t *ebs, int data, int bitsLeftToRead)
{
	etl_assert(bitsLeftToRead > 1);
	EBS_WriteBits(ebs, data < 0, 1);
	EBS_WriteBits(ebs, abs(data), bitsLeftToRead - 1);
}

int EBS_ReadBits(entityBitStream_t *ebs, int bitsLeftToRead)
{
	const int fieldCount    = ARRAY_LEN(entityIntFields);
	int       fieldIndex    = ebs->field;
	int       fieldBitIndex = ebs->bit;
	int       data          = 0;
	int       dataBitIndex  = 0;

	etl_assert(ebs->read == qtrue);

	if (ebs->count + bitsLeftToRead > ebs_maxIntBits)
	{
		return 0;
	}

	while (bitsLeftToRead > 0)
	{
		const intField_t *f;
		int              in, i, bitsToRead;

		if (fieldIndex >= fieldCount)
		{
			break;
		}

		f = &entityIntFields[fieldIndex];
		// in         = *((const int *)ebs->es + (f->byteOffset >> 2)) >> fieldBitIndex;
		in         = *(int *)((byte *)ebs->es + f->byteOffset);
		bitsToRead = MIN(bitsLeftToRead, f->numBits - fieldBitIndex);

		for ( i = 0; i < bitsToRead; ++i )
		{
			const int bitValue = (in >> (fieldBitIndex + i)) & 1;
			data |= bitValue << (dataBitIndex + i);
		}

		fieldBitIndex  += bitsToRead;
		bitsLeftToRead -= bitsToRead;
		ebs->count     += bitsToRead;
		dataBitIndex   += bitsToRead;
		if (fieldBitIndex >= f->numBits)
		{
			fieldBitIndex = 0;
			fieldIndex++;
		}
	}

	ebs->field = fieldIndex;
	ebs->bit   = fieldBitIndex;

	return data;
}

int EBS_ReadBitsWithSign(entityBitStream_t *ebs, int bitsLeftToRead)
{
	int sign;

	etl_assert(bitsLeftToRead > 1);

	sign = EBS_ReadBits(ebs, 1) ? -1 : 1;
	return EBS_ReadBits(ebs, bitsLeftToRead - 1) * sign;
}


void EBS_WriteFloat(entityState_t *es, int index, float data)
{
	if ((unsigned int)index >= ARRAY_LEN(entityFloatFields))
	{
		etl_assert(0);
		return;
	}

	*(float *)((byte *)es + entityFloatFields[index]) = data;
}

float EBS_ReadFloat(entityState_t *es, int index)
{
	if ((unsigned int)index >= ARRAY_LEN(entityFloatFields))
	{
		etl_assert(0);
		return 0.0f;
	}

	return *(float *)((byte *)es + entityFloatFields[index]);
}

#ifdef EBS_TESTS

static struct
{
	unsigned int table[256];
	qboolean created;
} crc32_data = { { 0 }, qfalse };

void CRC32_Begin(unsigned int *crc)
{
	int          i, j;
	unsigned int c;
	if (!crc32_data.created)
	{
		for (i = 0; i < 256; i++)
		{
			c = i;
			for (j = 0; j < 8; j++)
				c = c & 1 ? (c >> 1) ^ 0xEDB88320UL : c >> 1;
			crc32_data.table[i] = c;
		}
		crc32_data.created = qtrue;
	}
	*crc = 0xFFFFFFFFUL;
}

void CRC32_ProcessBlock(unsigned int *crc, const void *buffer, unsigned int length)
{
	unsigned int        hash = *crc;
	const unsigned char *buf = (const unsigned char *)buffer;
	while (length--)
	{
		hash = crc32_data.table[(hash ^ *buf++) & 0xFF] ^ (hash >> 8);
	}
	*crc = hash;
}

void CRC32_End(unsigned int *crc)
{
	*crc ^= 0xFFFFFFFFUL;
}

static void EBS_SimpleTest()
{
	entityBitStream_t s;
	entityState_t     es = { 0 };
	int               i, maxCount;

	maxCount = (ebs_maxIntBits - 7) / 11;

	EBS_InitWrite(&s, &es, qtrue);
	for ( i = 0; i < maxCount; ++i )
	{
		EBS_WriteBits(&s, i % 2, 1);
		EBS_WriteBits(&s, i, 10);
	}
	EBS_InitRead(&s, &es);
	for ( i = 0; i < maxCount; ++i )
	{
		if (i % 10 == 0)
		{
			EBS_Skip(&s, 11);
			continue;
		}
		etl_assert(EBS_ReadBits(&s, 1) == i % 2);
		etl_assert(EBS_ReadBits(&s, 10) == i);
	}
}

void EBS_RunTests()
{
	entityBitStream_t s;
	entityState_t     es;

	EBS_ValidateTables();

	EBS_SimpleTest();

	EBS_InitWrite(&s, &es, qtrue);
	EBS_WriteTestPayload(&s);
	EBS_InitRead(&s, &es);
	EBS_ValidateTestPayload(&s);
}

void EBS_ValidateTables(void)
{
	int i, numBits;

	// verify all int offsets are multiples of 4
	numBits = 0;
	for ( i = 0; i < ARRAY_LEN(entityIntFields); ++i )
	{
		etl_assert(entityIntFields[i].byteOffset % 4 == 0);
		numBits += entityIntFields[i].numBits;
	}

	// verify we have the expected number of int bits
	etl_assert(numBits == ebs_maxIntBits);

	// verify all float offsets are multiples of 4
	for ( i = 0; i < ARRAY_LEN(entityFloatFields); ++i )
	{
		etl_assert(entityFloatFields[i] % 4 == 0);
	}
}

#define RAWDATASIZE 100

void EBS_WriteTestPayload(entityBitStream_t *ebs)
{
	// leave space for the CRC32 checksum
	const int    dataBitCount      = ebs_maxIntBits - 32;
	const int    dataByteCount     = (dataBitCount + 7) / 8;
	const int    dataFullByteCount = dataBitCount / 8;
	const int    trailingBits      = dataBitCount % 8;
	byte         rawData[RAWDATASIZE];
	unsigned int checksum;
	int          b;

	etl_assert(sizeof(rawData) >= dataByteCount);
	etl_assert(ebs->count == 0);

	// generate the payload
	for ( b = 0; b < dataByteCount; ++b )
	{
		rawData[b] = rand();
	}

	// nuke the top bits of the last byte if necessary
	if (trailingBits > 0)
	{
		rawData[dataByteCount - 1] <<= (byte)(8 - trailingBits);
		rawData[dataByteCount - 1] >>= (byte)(8 - trailingBits);
	}

	// compute the checksum
	CRC32_Begin(&checksum);
	for ( b = 0; b < dataByteCount; ++b )
	{
		CRC32_ProcessBlock(&checksum, &rawData[b], 1);
	}
	CRC32_End(&checksum);

	// write the generic data
	for ( b = 0; b < dataFullByteCount; ++b )
	{
		EBS_WriteBits(ebs, rawData[b], 8);
	}
	if (trailingBits > 0)
	{
		EBS_WriteBits(ebs, rawData[b], trailingBits);
	}
	EBS_WriteBits(ebs, checksum, 32);
	etl_assert(ebs->count == ebs_maxIntBits);

	// write the float data
	for ( b = 0; b < ARRAY_LEN(entityFloatFields); ++b )
	{
		float data;

		rawData[0] = rand();
		rawData[1] = rand();
		rawData[2] = rand();
		rawData[3] = rand();

		data = *(float *)rawData;
		EBS_WriteFloat(ebs->es, b, data);
	}
}

void EBS_ValidateTestPayload(entityBitStream_t *ebs)
{
	// leave space for the CRC32 checksum
	const int    dataBitCount      = ebs_maxIntBits - 32;
	const int    dataByteCount     = (dataBitCount + 7) / 8;
	const int    dataFullByteCount = dataBitCount / 8;
	const int    trailingBits      = dataBitCount % 8;
	byte         rawData[RAWDATASIZE];
	unsigned int netChecksum, compChecksum;
	int          b;

	etl_assert(sizeof(rawData) >= dataByteCount);
	etl_assert(ebs->count == 0);

	// read the data
	for ( b = 0; b < dataFullByteCount; ++b )
	{
		rawData[b] = EBS_ReadBits(ebs, 8);
	}
	if (trailingBits > 0)
	{
		rawData[b] = EBS_ReadBits(ebs, trailingBits);
	}
	netChecksum = EBS_ReadBits(ebs, 32);
	etl_assert(ebs->count == ebs_maxIntBits);

	// compute the checksum
	CRC32_Begin(&compChecksum);
	for ( b = 0; b < dataByteCount; ++b )
	{
		CRC32_ProcessBlock(&compChecksum, &rawData[b], 1);
	}
	CRC32_End(&compChecksum);

	// validate the checksum!
	etl_assert(compChecksum == netChecksum);
}

#endif
