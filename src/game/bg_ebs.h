/*
===========================================================================
Copyright (C) 2022 Gian 'myT' Schellenbaum. All rights reserved.

This file is part of Challenge ProMode Arena (CPMA).
===========================================================================
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
