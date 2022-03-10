/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_animation.c
 *
 * @brief All bones should be an identity orientation to display the mesh exactly
 * as it is specified.
 *
 * For all other frames, the bones represent the transformation from the
 * orientation of the bone in the base frame to the orientation in this
 * frame.
 */

#include "tr_local.h"

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)

/**
 * @brief R_AllocAnimation
 * @return
 */
static skelAnimation_t *R_AllocAnimation(void)
{
	skelAnimation_t *anim;

	if (tr.numAnimations == MAX_ANIMATIONFILES)
	{
		return NULL;
	}

	anim                            = (skelAnimation_t *)ri.Hunk_Alloc(sizeof(*anim), h_low);
	anim->index                     = tr.numAnimations;
	tr.animations[tr.numAnimations] = anim;
	tr.numAnimations++;

	return anim;
}

/**
 * @brief R_InitAnimations
 */
void R_InitAnimations(void)
{
	skelAnimation_t *anim;

	// leave a space for NULL animation
	tr.numAnimations = 0;

	anim       = R_AllocAnimation();
	anim->type = AT_BAD;
	strcpy(anim->name, "<default animation>");
}

/**
 * @brief R_LoadMD5Anim
 * @param[out] skelAnim
 * @param[out] buffer
 * @param bufferSize - unused
 * @param[in] name
 * @return
 */
static qboolean R_LoadMD5Anim(skelAnimation_t *skelAnim, byte *buffer, int bufferSize, const char *name)
{
	int            i, j;
	md5Animation_t *anim;
	md5Frame_t     *frame;
	md5Channel_t   *channel;
	char           *token;
	int            version;
	char           *buf_p = (char *)buffer;

	skelAnim->type = AT_MD5;
	skelAnim->md5  = anim = (md5Animation_t *)ri.Hunk_Alloc(sizeof(*anim), h_low);

	// skip MD5Version indent string
	(void) COM_ParseExt2(&buf_p, qfalse);

	// check version
	token   = COM_ParseExt2(&buf_p, qfalse);
	version = Q_atoi(token);
	if (version != MD5_VERSION)
	{
		Ren_Warning("RE_RegisterAnimation: '%s' has wrong version (%i should be %i)\n", name, version, MD5_VERSION);
		return qfalse;
	}

	// skip commandline <arguments string>
	token = COM_ParseExt2(&buf_p, qtrue);
	token = COM_ParseExt2(&buf_p, qtrue);

	// parse numFrames <number>
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "numFrames"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'numFrames' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}
	token           = COM_ParseExt2(&buf_p, qfalse);
	anim->numFrames = Q_atoi(token);

	// parse numJoints <number>
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "numJoints"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'numJoints' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}
	token             = COM_ParseExt2(&buf_p, qfalse);
	anim->numChannels = Q_atoi(token);

	// parse frameRate <number>
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "frameRate"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'frameRate' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}
	token           = COM_ParseExt2(&buf_p, qfalse);
	anim->frameRate = Q_atoi(token);

	// parse numAnimatedComponents <number>
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "numAnimatedComponents"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'numAnimatedComponents' found '%s' in model '%s'\n", token,
		            name);
		return qfalse;
	}
	token                       = COM_ParseExt2(&buf_p, qfalse);
	anim->numAnimatedComponents = Q_atoi(token);

	// parse hierarchy {
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "hierarchy"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'hierarchy' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}
	token = COM_ParseExt2(&buf_p, qfalse);
	if (Q_stricmp(token, "{"))
	{
		Ren_Warning("RE_RegisterAnimation: expected '{' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}

	// parse all the channels
	anim->channels = (md5Channel_t *)ri.Hunk_Alloc(sizeof(md5Channel_t) * anim->numChannels, h_low);

	for (i = 0, channel = anim->channels; i < anim->numChannels; i++, channel++)
	{
		token = COM_ParseExt2(&buf_p, qtrue);
		Q_strncpyz(channel->name, token, sizeof(channel->name));

		//Ren_Print("RE_RegisterAnimation: '%s' has channel '%s'\n", name, channel->name);

		token                = COM_ParseExt2(&buf_p, qfalse);
		channel->parentIndex = Q_atoi(token);

		if (channel->parentIndex >= anim->numChannels)
		{
			Ren_Drop("RE_RegisterAnimation: '%s' has channel '%s' with bad parent index %i while numBones is %i\n",
			         name, channel->name, channel->parentIndex, anim->numChannels);
		}

		token                   = COM_ParseExt2(&buf_p, qfalse);
		channel->componentsBits = Q_atoi(token);

		token                     = COM_ParseExt2(&buf_p, qfalse);
		channel->componentsOffset = Q_atoi(token);
	}

	// parse }
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "}"))
	{
		Ren_Warning("RE_RegisterAnimation: expected '}' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}

	// parse bounds {
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "bounds"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'bounds' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}
	token = COM_ParseExt2(&buf_p, qfalse);
	if (Q_stricmp(token, "{"))
	{
		Ren_Warning("RE_RegisterAnimation: expected '{' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}

	anim->frames = (md5Frame_t *)ri.Hunk_Alloc(sizeof(md5Frame_t) * anim->numFrames, h_low);

	for (i = 0, frame = anim->frames; i < anim->numFrames; i++, frame++)
	{
		// skip (
		token = COM_ParseExt2(&buf_p, qtrue);
		if (Q_stricmp(token, "("))
		{
			Ren_Warning("RE_RegisterAnimation: expected '(' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		for (j = 0; j < 3; j++)
		{
			token               = COM_ParseExt2(&buf_p, qfalse);
			frame->bounds[0][j] = atof(token);
		}

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("RE_RegisterAnimation: expected ')' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		// skip (
		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, "("))
		{
			Ren_Warning("RE_RegisterAnimation: expected '(' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		for (j = 0; j < 3; j++)
		{
			token               = COM_ParseExt2(&buf_p, qfalse);
			frame->bounds[1][j] = atof(token);
		}

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("RE_RegisterAnimation: expected ')' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}
	}

	// parse }
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "}"))
	{
		Ren_Warning("RE_RegisterAnimation: expected '}' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}

	// parse baseframe {
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "baseframe"))
	{
		Ren_Warning("RE_RegisterAnimation: expected 'baseframe' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}
	token = COM_ParseExt2(&buf_p, qfalse);
	if (Q_stricmp(token, "{"))
	{
		Ren_Warning("RE_RegisterAnimation: expected '{' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}

	for (i = 0, channel = anim->channels; i < anim->numChannels; i++, channel++)
	{
		// skip (
		token = COM_ParseExt2(&buf_p, qtrue);
		if (Q_stricmp(token, "("))
		{
			Ren_Warning("RE_RegisterAnimation: expected '(' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		for (j = 0; j < 3; j++)
		{
			token                  = COM_ParseExt2(&buf_p, qfalse);
			channel->baseOrigin[j] = atof(token);
		}

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("RE_RegisterAnimation: expected ')' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		// skip (
		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, "("))
		{
			Ren_Warning("RE_RegisterAnimation: expected '(' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		for (j = 0; j < 3; j++)
		{
			token                = COM_ParseExt2(&buf_p, qfalse);
			channel->baseQuat[j] = atof(token);
		}
		QuatCalcW(channel->baseQuat);

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("RE_RegisterAnimation: expected ')' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}
	}

	// parse }
	token = COM_ParseExt2(&buf_p, qtrue);
	if (Q_stricmp(token, "}"))
	{
		Ren_Warning("RE_RegisterAnimation: expected '}' found '%s' in model '%s'\n", token, name);
		return qfalse;
	}

	for (i = 0, frame = anim->frames; i < anim->numFrames; i++, frame++)
	{
		// parse frame <number> {
		token = COM_ParseExt2(&buf_p, qtrue);
		if (Q_stricmp(token, "frame"))
		{
			Ren_Warning("RE_RegisterAnimation: expected 'baseframe' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, va("%i", i)))
		{
			Ren_Warning("RE_RegisterAnimation: expected '%i' found '%s' in model '%s'\n", i, token, name);
			return qfalse;
		}

		token = COM_ParseExt2(&buf_p, qfalse);
		if (Q_stricmp(token, "{"))
		{
			Ren_Warning("RE_RegisterAnimation: expected '{' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}

		frame->components = (float *)ri.Hunk_Alloc(sizeof(float) * anim->numAnimatedComponents, h_low);
		for (j = 0; j < anim->numAnimatedComponents; j++)
		{
			token                = COM_ParseExt2(&buf_p, qtrue);
			frame->components[j] = atof(token);
		}

		// parse }
		token = COM_ParseExt2(&buf_p, qtrue);
		if (Q_stricmp(token, "}"))
		{
			Ren_Warning("RE_RegisterAnimation: expected '}' found '%s' in model '%s'\n", token, name);
			return qfalse;
		}
	}

	// everything went ok
	return qtrue;
}

/**
 * @brief GetChunkHeader
 * @param[in] s
 * @param[out] chunkHeader
 */
static void GetChunkHeader(memStream_t *s, axChunkHeader_t *chunkHeader)
{
	int i;

	for (i = 0; i < 20; i++)
	{
		chunkHeader->ident[i] = MemStreamGetC(s);
	}

	chunkHeader->flags    = MemStreamGetLong(s);
	chunkHeader->dataSize = MemStreamGetLong(s);
	chunkHeader->numData  = MemStreamGetLong(s);
}

/**
 * @brief PrintChunkHeader
 * @param chunkHeader - unused
 * @note Disable
 */
static void PrintChunkHeader(axChunkHeader_t *chunkHeader)
{
#if 0
	Ren_Print("----------------------\n");
	Ren_Print("R_LoadPSA: chunk header ident: '%s'\n", chunkHeader->ident);
	Ren_Print("R_LoadPSA: chunk header flags: %i\n", chunkHeader->flags);
	Ren_Print("R_LoadPSA: chunk header data size: %i\n", chunkHeader->dataSize);
	Ren_Print("R_LoadPSA: chunk header num items: %i\n", chunkHeader->numData);
#endif
}

/**
 * @brief GetBone
 * @param[in] s
 * @param[out] bone
 */
static void GetBone(memStream_t *s, axBone_t *bone)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		bone->quat[i] = MemStreamGetFloat(s);
	}

	for (i = 0; i < 3; i++)
	{
		bone->position[i] = MemStreamGetFloat(s);
	}

	bone->length = MemStreamGetFloat(s);

	bone->xSize = MemStreamGetFloat(s);
	bone->ySize = MemStreamGetFloat(s);
	bone->zSize = MemStreamGetFloat(s);
}

/**
 * @brief R_LoadPSA
 * @param[in,out] skelAnim
 * @param[out] buffer
 * @param[in] bufferSize
 * @param[out] name
 * @return
 */
static qboolean R_LoadPSA(skelAnimation_t *skelAnim, byte *buffer, int bufferSize, const char *name)
{
	int               i, j, k;
	memStream_t       *stream;
	axChunkHeader_t   chunkHeader;
	int               numReferenceBones;
	axReferenceBone_t *refBone;
	axReferenceBone_t *refBones;
	int               numSequences;
	axAnimationInfo_t *animInfo;
	axAnimationKey_t  *key;
	psaAnimation_t    *psa;
	skelAnimation_t   *extraAnim;
	growList_t        extraAnims;

	stream = AllocMemStream(buffer, bufferSize);

	if (stream == NULL)
	{
		Ren_Warning("R_LoadPSA: can't allocate memory\n");
		return qfalse;
	}

	GetChunkHeader(stream, &chunkHeader);

	// check indent again
	if (Q_stricmpn(chunkHeader.ident, "ANIMHEAD", 8))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk indent ('%s' should be '%s')\n", name, chunkHeader.ident, "ANIMHEAD");
		FreeMemStream(stream);
		return qfalse;
	}

	PrintChunkHeader(&chunkHeader);

	// read reference bones
	GetChunkHeader(stream, &chunkHeader);
	if (Q_stricmpn(chunkHeader.ident, "BONENAMES", 9))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk indent ('%s' should be '%s')\n", name, chunkHeader.ident, "BONENAMES");
		FreeMemStream(stream);
		return qfalse;
	}

	if (chunkHeader.dataSize != sizeof(axReferenceBone_t))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk dataSize ('%i' should be '%lu')\n", name, chunkHeader.dataSize, (unsigned long) sizeof(axReferenceBone_t));
		FreeMemStream(stream);
		return qfalse;
	}

	PrintChunkHeader(&chunkHeader);

	numReferenceBones = chunkHeader.numData;
	if (numReferenceBones < 1)
	{
		Ren_Warning("R_LoadPSA: '%s' has no bones\n", name);
		FreeMemStream(stream);
		return qfalse;
	}
	if (numReferenceBones > MAX_BONES)
	{
		Ren_Warning("R_LoadPSA: '%s' has more than %i bones (%i)\n", name, MAX_BONES, numReferenceBones);
		FreeMemStream(stream);
		return qfalse;
	}

	refBones = (axReferenceBone_t *)ri.Hunk_Alloc(numReferenceBones * sizeof(axReferenceBone_t), h_low);
	for (i = 0, refBone = refBones; i < numReferenceBones; i++, refBone++)
	{
		(void) MemStreamRead(stream, refBone->name, sizeof(refBone->name));

		refBone->flags       = MemStreamGetLong(stream);
		refBone->numChildren = MemStreamGetLong(stream);
		refBone->parentIndex = MemStreamGetLong(stream);

		if (i == 0)
		{
			refBone->parentIndex = -1;
		}

		GetBone(stream, &refBone->bone);

#if 0
		Ren_Print("R_LoadPSA: axReferenceBone_t(%i):\n"
		          "axReferenceBone_t::name: '%s'\n"
		          "axReferenceBone_t::flags: %i\n"
		          "axReferenceBone_t::numChildren %i\n"
		          "axReferenceBone_t::parentIndex: %i\n"
		          "axReferenceBone_t::quat: %f %f %f %f\n"
		          "axReferenceBone_t::position: %f %f %f\n"
		          "axReferenceBone_t::length: %f\n"
		          "axReferenceBone_t::xSize: %f\n"
		          "axReferenceBone_t::ySize: %f\n"
		          "axReferenceBone_t::zSize: %f\n",
		          i,
		          refBone->name,
		          refBone->flags,
		          refBone->numChildren,
		          refBone->parentIndex,
		          refBone->bone.quat[0], refBone->bone.quat[1], refBone->bone.quat[2], refBone->bone.quat[3],
		          refBone->bone.position[0], refBone->bone.position[1], refBone->bone.position[2],
		          refBone->bone.length,
		          refBone->bone.xSize,
		          refBone->bone.ySize,
		          refBone->bone.zSize);
#endif
	}

	// load animation info
	GetChunkHeader(stream, &chunkHeader);
	if (Q_stricmpn(chunkHeader.ident, "ANIMINFO", 8))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk indent ('%s' should be '%s')\n", name, chunkHeader.ident, "ANIMINFO");
		FreeMemStream(stream);
		return qfalse;
	}

	if (chunkHeader.dataSize != sizeof(axAnimationInfo_t))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk dataSize ('%i' should be '%lu')\n", name, chunkHeader.dataSize, (unsigned long) sizeof(axAnimationInfo_t));
		FreeMemStream(stream);
		return qfalse;
	}

	PrintChunkHeader(&chunkHeader);

	numSequences = chunkHeader.numData;
	Com_InitGrowList(&extraAnims, numSequences - 1);
	for (i = 0; i < numSequences; i++)
	{
		if (i == 0)
		{
			Q_strncpyz(skelAnim->name, name, sizeof(skelAnim->name));
			skelAnim->type = AT_PSA;
			skelAnim->psa  = psa = (psaAnimation_t *)ri.Hunk_Alloc(sizeof(*psa), h_low);
		}
		else
		{
			// allocate a new skelAnimation_t
			if ((extraAnim = R_AllocAnimation()) == NULL)
			{
				Ren_Warning("R_LoadPSA: R_AllocAnimation() failed for '%s'\n", name);
				FreeMemStream(stream);
				return qfalse;
			}

			Q_strncpyz(extraAnim->name, name, sizeof(extraAnim->name));
			extraAnim->type = AT_PSA;
			extraAnim->psa  = psa = (psaAnimation_t *)ri.Hunk_Alloc(sizeof(*psa), h_low);

			Com_AddToGrowList(&extraAnims, extraAnim);
		}

		psa->numBones = numReferenceBones;
		psa->bones    = refBones;

		animInfo = &psa->info;

		(void) MemStreamRead(stream, animInfo->name, sizeof(animInfo->name));
		(void) MemStreamRead(stream, animInfo->group, sizeof(animInfo->group));

		animInfo->numBones = MemStreamGetLong(stream);

		if (animInfo->numBones != numReferenceBones)
		{
			FreeMemStream(stream);
			Ren_Drop("R_LoadPSA: axAnimationInfo_t contains different number than reference bones exist: %i != %i for anim '%s'", animInfo->numBones, numReferenceBones, name);
		}

		animInfo->rootInclude = MemStreamGetLong(stream);

		animInfo->keyCompressionStyle = MemStreamGetLong(stream);
		animInfo->keyQuotum           = MemStreamGetLong(stream);
		animInfo->keyReduction        = MemStreamGetFloat(stream);

		animInfo->trackTime = MemStreamGetFloat(stream);

		animInfo->frameRate = MemStreamGetFloat(stream);

		animInfo->startBoneIndex = MemStreamGetLong(stream);

		animInfo->firstRawFrame = MemStreamGetLong(stream);
		animInfo->numRawFrames  = MemStreamGetLong(stream);

#if 0
		Ren_Print("R_LoadPSA: axAnimationInfo_t(%i):\n"
		          "axAnimationInfo_t::name: '%s'\n"
		          "axAnimationInfo_t::group: '%s'\n"
		          "axAnimationInfo_t::numBones: %i\n"
		          "axAnimationInfo_t::rootInclude: %i\n"
		          "axAnimationInfo_t::keyCompressionStyle: %i\n"
		          "axAnimationInfo_t::keyQuotum: %i\n"
		          "axAnimationInfo_t::keyReduction: %f\n"
		          "axAnimationInfo_t::trackTime: %f\n"
		          "axAnimationInfo_t::frameRate: %f\n"
		          "axAnimationInfo_t::startBoneIndex: %i\n"
		          "axAnimationInfo_t::firstRawFrame: %i\n"
		          "axAnimationInfo_t::numRawFrames: %i\n",
		          i,
		          animInfo->name,
		          animInfo->group,
		          animInfo->numBones,
		          animInfo->rootInclude,
		          animInfo->keyCompressionStyle,
		          animInfo->keyQuotum,
		          animInfo->keyReduction,
		          animInfo->trackTime,
		          animInfo->frameRate,
		          animInfo->startBoneIndex,
		          animInfo->firstRawFrame,
		          animInfo->numRawFrames);
#endif
	}

	// load the animation frame keys
	GetChunkHeader(stream, &chunkHeader);
	if (Q_stricmpn(chunkHeader.ident, "ANIMKEYS", 8))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk indent ('%s' should be '%s')\n", name, chunkHeader.ident, "ANIMKEYS");
		FreeMemStream(stream);
		return qfalse;
	}

	if (chunkHeader.dataSize != sizeof(axAnimationKey_t))
	{
		Ren_Warning("R_LoadPSA: '%s' has wrong chunk dataSize ('%i' should be '%lu')\n", name, chunkHeader.dataSize, (unsigned long) sizeof(axAnimationKey_t));
		FreeMemStream(stream);
		return qfalse;
	}

	PrintChunkHeader(&chunkHeader);

	for (i = 0; i < numSequences; i++)
	{
		if (i == 0)
		{
			psa = skelAnim->psa;
		}
		else
		{
			extraAnim = (skelAnimation_t *)Com_GrowListElement(&extraAnims, i - 1);
			psa       = extraAnim->psa;
		}

		psa->numKeys = psa->info.numBones * psa->info.numRawFrames;
		psa->keys    = (axAnimationKey_t *)ri.Hunk_Alloc(psa->numKeys * sizeof(axAnimationKey_t), h_low);

		for (j = 0, key = &psa->keys[0]; j < psa->numKeys; j++, key++)
		{
			for (k = 0; k < 3; k++)
			{
				key->position[k] = MemStreamGetFloat(stream);
			}

			// see R_LoadPSK ...
			if ((j % psa->info.numBones) == 0)
			{
				key->quat[0] = MemStreamGetFloat(stream);
				key->quat[1] = -MemStreamGetFloat(stream);
				key->quat[2] = MemStreamGetFloat(stream);
				key->quat[3] = MemStreamGetFloat(stream);
			}
			else
			{
				key->quat[0] = -MemStreamGetFloat(stream);
				key->quat[1] = -MemStreamGetFloat(stream);
				key->quat[2] = -MemStreamGetFloat(stream);
				key->quat[3] = MemStreamGetFloat(stream);
			}

			key->time = MemStreamGetFloat(stream);
		}
	}

	Com_DestroyGrowList(&extraAnims);
	FreeMemStream(stream);

	return qtrue;
}

/**
 * @brief RE_RegisterAnimation
 * @param[in] name
 * @return
 */
qhandle_t RE_RegisterAnimation(const char *name)
{
	qhandle_t       hAnim;
	skelAnimation_t *anim;
	byte            *buffer;
	int             bufferLen;
	qboolean        loaded = qfalse;

	if (!name || !name[0])
	{
		Ren_Warning("Empty name passed to RE_RegisterAnimation\n");
		return 0;
	}

	//Ren_Print("RE_RegisterAnimation(%s)\n", name);

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Warning("Animation name exceeds MAX_QPATH\n");
		return 0;
	}

	// search the currently loaded animations
	for (hAnim = 1; hAnim < tr.numAnimations; hAnim++)
	{
		anim = tr.animations[hAnim];
		if (!Q_stricmp(anim->name, name))
		{
			if (anim->type == AT_BAD)
			{
				return 0;
			}
			return hAnim;
		}

		if (anim->type == AT_PSA && anim->psa)
		{
			const char *animName;

			animName = strstr(name, "::");

			//Ren_Print("animName = '%s'\n", animName ? (animName + 2) : NULL);
			if (animName && *(animName + 2) && !Q_stricmp(anim->psa->info.name, (animName + 2)))
			{
				return hAnim;
			}
		}
	}

	// allocate a new model_t
	if ((anim = R_AllocAnimation()) == NULL)
	{
		Ren_Warning("RE_RegisterAnimation: R_AllocAnimation() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the animation has been successfully allocated
	Q_strncpyz(anim->name, name, sizeof(anim->name));

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	// load and parse the .md5anim file
	bufferLen = ri.FS_ReadFile(name, (void **)&buffer);
	if (!buffer)
	{
		return 0;
	}

	if (!Q_stricmpn((const char *)buffer, "MD5Version", 10))
	{
		loaded = R_LoadMD5Anim(anim, buffer, bufferLen, name);
	}
	else if (!Q_stricmpn((const char *)buffer, "ANIMHEAD", 8))
	{
		loaded = R_LoadPSA(anim, buffer, bufferLen, name);
	}
	else
	{
		Ren_Warning("RE_RegisterAnimation: unknown fileid for '%s'\n", name);
	}

	ri.FS_FreeFile(buffer);

	if (!loaded)
	{
		Ren_Warning("couldn't load '%s'\n", name);

		// we still keep the model_t around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		anim->type = AT_BAD;
	}

	return anim->index;
}

/**
 * @brief R_GetAnimationByHandle
 * @param[in] hAnim
 * @return
 */
skelAnimation_t *R_GetAnimationByHandle(qhandle_t hAnim)
{
	skelAnimation_t *anim;

	// out of range gets the default animation
	if (hAnim < 1 || hAnim >= tr.numAnimations)
	{
		return tr.animations[0];
	}

	anim = tr.animations[hAnim];

	return anim;
}

/**
 * @brief R_AnimationList_f
 */
void R_AnimationList_f(void)
{
	int             i;
	skelAnimation_t *anim;

	for (i = 0; i < tr.numAnimations; i++)
	{
		anim = tr.animations[i];

		if (anim->type == AT_PSA && anim->psa)
		{
			Ren_Print("'%s' : '%s'\n", anim->name, anim->psa->info.name);
		}
		else
		{
			Ren_Print("'%s'\n", anim->name);
		}
	}
	Ren_Print("%8i : Total animations\n", tr.numAnimations);
}

/**
 * @brief R_CullMD5
 * @param[in,out] ent
 */
static void R_CullMD5(trRefEntity_t *ent)
{
	if (ent->e.skeleton.type == SK_INVALID)
	{
		// no properly set skeleton so use the bounding box by the model instead by the animations
		md5Model_t *model = tr.currentModel->md5;

		VectorCopy(model->bounds[0], ent->localBounds[0]);
		VectorCopy(model->bounds[1], ent->localBounds[1]);
	}
	else
	{
		int i;

		// copy a bounding box in the current coordinate system provided by skeleton
		for (i = 0; i < 3; i++)
		{
			ent->localBounds[0][i] = ent->e.skeleton.bounds[0][i]; // FIXME: * ent->e.skeleton.scale
			ent->localBounds[1][i] = ent->e.skeleton.bounds[1][i]; // FIXME: * ent->e.skeleton.scale
		}
	}

	// set up world bounds for light intersection tests
	R_SetupEntityWorldBounds(ent);

	switch (R_CullBox(ent->worldBounds))
	{
	case CULL_IN:
		tr.pc.c_box_cull_md5_in++;
		ent->cull = CULL_IN;
		return;
	case CULL_CLIP:
		tr.pc.c_box_cull_md5_clip++;
		ent->cull = CULL_CLIP;
		return;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md5_out++;
		ent->cull = CULL_OUT;
		return;
	}
}

/**
 * @brief R_AddMD5Surfaces
 * @param[in] ent
 */
void R_AddMD5Surfaces(trRefEntity_t *ent)
{
	md5Model_t   *model = tr.currentModel->md5;
	md5Surface_t *surface;
	shader_t     *shader;
	int          i;
	qboolean     personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal; // don't add third_person objects if not in a portal
	int          fogNum;

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum
	R_CullMD5(ent);

	if (ent->cull == CULL_OUT)
	{
		return;
	}

	// set up lighting now that we know we aren't culled
	if (!personalModel || r_shadows->integer > SHADOWING_BLOB)
	{
		R_SetupEntityLighting(&tr.refdef, ent, NULL);
	}

	// see if we are in a fog volume
	fogNum = R_FogWorldBox(ent->worldBounds);

	if (!r_vboModels->integer || !model->numVBOSurfaces ||
	    (!glConfig2.vboVertexSkinningAvailable && ent->e.skeleton.type == SK_ABSOLUTE))
	{
		// finally add surfaces
		for (i = 0, surface = model->surfaces; i < model->numSurfaces; i++, surface++)
		{
			if (ent->e.customShader)
			{
				shader = R_GetShaderByHandle(ent->e.customShader);
			}
			else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
			{
				skin_t *skin;

				skin = R_GetSkinByHandle(ent->e.customSkin);

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				if (i >= 0 && i < skin->numSurfaces && skin->surfaces)
				{
					shader = skin->surfaces[i].shader;
				}
				if (shader == tr.defaultShader)
				{
					Ren_Developer("WARNING: no shader for surface %i in skin %s\n", i, skin->name);
				}
				else if (shader->defaultShader)
				{
					Ren_Developer("WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
				}
			}
			else
			{
				shader = R_GetShaderByHandle(surface->shaderIndex);
			}

			// we will add shadows even if the main object isn't visible in the view

			// don't add third_person objects if not viewing through a portal
			if (!personalModel)
			{
				R_AddDrawSurf((surfaceType_t *)surface, shader, LIGHTMAP_NONE, fogNum);
			}
		}
	}
	else
	{
		int             i;
		srfVBOMD5Mesh_t *vboSurface;
		shader_t        *shader;

		for (i = 0; i < model->numVBOSurfaces; i++)
		{
			vboSurface = model->vboSurfaces[i];

			if (ent->e.customShader)
			{
				shader = R_GetShaderByHandle(ent->e.customShader);
			}
			else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
			{
				skin_t *skin;

				skin = R_GetSkinByHandle(ent->e.customSkin);

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				//if(i >= 0 && i < skin->numSurfaces && skin->surfaces[i])
				if (vboSurface->skinIndex >= 0 && vboSurface->skinIndex < skin->numSurfaces && skin->surfaces)
				{
					shader = skin->surfaces[vboSurface->skinIndex].shader;
				}

				if (shader == tr.defaultShader)
				{
					Ren_Developer("WARNING: no shader for surface %i in skin %s\n", i, skin->name);
				}
				else if (shader->defaultShader)
				{
					Ren_Developer("WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
				}
			}
			else
			{
				shader = vboSurface->shader;
			}

			// don't add third_person objects if not viewing through a portal
			if (!personalModel)
			{
				R_AddDrawSurf((surfaceType_t *)vboSurface, shader, LIGHTMAP_NONE, fogNum);
			}
		}
	}
}

/**
 * @brief R_AddMD5Interactions
 * @param[in] ent
 * @param[in] light
 */
void R_AddMD5Interactions(trRefEntity_t *ent, trRefLight_t *light)
{
	int               i;
	md5Model_t        *model;
	md5Surface_t      *surface;
	shader_t          *shader = 0;
	qboolean          personalModel;
	byte              cubeSideBits = CUBESIDE_CLIPALL;
	interactionType_t iaType       = IA_DEFAULT;

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum and we don't care about proper shadowing
	if (ent->cull == CULL_OUT)
	{
		if (r_shadows->integer <= SHADOWING_BLOB || light->l.noShadows)
		{
			return;
		}
		else
		{
			iaType = IA_SHADOWONLY;
		}
	}

	// avoid drawing of certain objects
#if defined(USE_REFENTITY_NOSHADOWID)
	if (light->l.inverseShadows)
	{
		if (iaType != IA_LIGHTONLY && (light->l.noShadowID && (light->l.noShadowID != ent->e.noShadowID)))
		{
			return;
		}
	}
	else
	{
		if (iaType != IA_LIGHTONLY && (light->l.noShadowID && (light->l.noShadowID == ent->e.noShadowID)))
		{
			return;
		}
	}
#endif

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	model = tr.currentModel->md5;

	// do a quick AABB cull
	if (!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], ent->worldBounds[0], ent->worldBounds[1]))
	{
		tr.pc.c_dlightSurfacesCulled += model->numSurfaces;
		return;
	}

	// do a more expensive and precise light frustum cull
	if (!r_noLightFrustums->integer)
	{
		if (R_CullLightWorldBounds(light, ent->worldBounds) == CULL_OUT)
		{
			tr.pc.c_dlightSurfacesCulled += model->numSurfaces;
			return;
		}
	}

	cubeSideBits = R_CalcLightCubeSideBits(light, ent->worldBounds);

	if (!r_vboModels->integer || !model->numVBOSurfaces ||
	    (!glConfig2.vboVertexSkinningAvailable && ent->e.skeleton.type == SK_ABSOLUTE))
	{
		// generate interactions with all surfaces
		for (i = 0, surface = model->surfaces; i < model->numSurfaces; i++, surface++)
		{
			if (ent->e.customShader)
			{
				shader = R_GetShaderByHandle(ent->e.customShader);
			}
			else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
			{
				skin_t *skin;

				skin = R_GetSkinByHandle(ent->e.customSkin);

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				if (i >= 0 && i < skin->numSurfaces && skin->surfaces)
				{
					shader = skin->surfaces[i].shader;
				}
				if (shader == tr.defaultShader)
				{
					Ren_Developer("R_AddMD5Interactions WARNING: no shader for surface %i in skin %s\n", i, skin->name);
				}
				else if (shader->defaultShader)
				{
					Ren_Developer("R_AddMD5Interactions WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
				}
			}
			else
			{
				shader = R_GetShaderByHandle(surface->shaderIndex);
			}

			// skip all surfaces that don't matter for lighting only pass
			if (shader->isSky || (!shader->interactLight && shader->noShadows))
			{
				continue;
			}

			// we will add shadows even if the main object isn't visible in the view

			// don't add third_person objects if not viewing through a portal
			if (!personalModel)
			{
				R_AddLightInteraction(light, (surfaceType_t *)surface, shader, cubeSideBits, iaType);
				tr.pc.c_dlightSurfaces++;
			}
		}
	}
	else
	{
		int             i;
		srfVBOMD5Mesh_t *vboSurface;
		shader_t        *shader;

		for (i = 0; i < model->numVBOSurfaces; i++)
		{
			vboSurface = model->vboSurfaces[i];

			if (ent->e.customShader)
			{
				shader = R_GetShaderByHandle(ent->e.customShader);
			}
			else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
			{
				skin_t *skin;

				skin = R_GetSkinByHandle(ent->e.customSkin);

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				if (i >= 0 && i < skin->numSurfaces && skin->surfaces)
				{
					shader = skin->surfaces[i].shader;
				}
				if (shader == tr.defaultShader)
				{
					Ren_Developer("R_AddMD5Interactions WARNING: no shader for surface %i in skin %s\n", i, skin->name);
				}
				else if (shader->defaultShader)
				{
					Ren_Developer("R_AddMD5Interactions WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
				}
			}
			else
			{
				shader = vboSurface->shader;
			}

			// skip all surfaces that don't matter for lighting only pass
			if (shader->isSky || (!shader->interactLight && shader->noShadows))
			{
				continue;
			}

			// don't add third_person objects if not viewing through a portal
			if (!personalModel)
			{
				R_AddLightInteraction(light, (surfaceType_t *)vboSurface, shader, cubeSideBits, iaType);
				tr.pc.c_dlightSurfaces++;
			}
		}
	}
}

/**
 * @brief Check if the skeleton bones are the same in the model and animation
 * and copy the parentIndex entries into the refSkeleton_t
 * @param[out] skel
 * @param[in] hModel
 * @param[in] hAnim
 * @return
 */
int RE_CheckSkeleton(refSkeleton_t *skel, qhandle_t hModel, qhandle_t hAnim)
{
	int             i;
	model_t         *model;
	md5Model_t      *md5Model;
	skelAnimation_t *skelAnim;

	model    = R_GetModelByHandle(hModel);
	skelAnim = R_GetAnimationByHandle(hAnim);

	if (model->type != MOD_MD5 || !model->md5)
	{
		Ren_Warning("RE_CheckSkeleton: '%s' is not a skeletal model\n", model->name);
		return qfalse;
	}

	md5Model = model->md5;
	if (md5Model->numBones < 1)
	{
		Ren_Warning("RE_CheckSkeleton: '%s' has no bones\n", model->name);
		return qfalse;
	}
	if (md5Model->numBones > MAX_BONES)
	{
		Ren_Warning("RE_CheckSkeleton: '%s' has more than %i bones (%i)\n", model->name, MAX_BONES, md5Model->numBones);
		return qfalse;
	}

	if (skelAnim->type == AT_MD5 && skelAnim->md5)
	{
		md5Animation_t *md5Animation = skelAnim->md5;
		md5Bone_t      *md5Bone;
		md5Channel_t   *md5Channel;

		if (md5Model->numBones != md5Animation->numChannels)
		{
			Ren_Warning("RE_CheckSkeleton: model '%s' has different number of bones than animation '%s': %d != %d\n", model->name, skelAnim->name, md5Model->numBones, md5Animation->numChannels);
			return qfalse;
		}

		// check bone names
		for (i = 0, md5Bone = md5Model->bones, md5Channel = md5Animation->channels; i < md5Model->numBones; i++, md5Bone++, md5Channel++)
		{
			if (Q_stricmp(md5Bone->name, md5Channel->name))
			{
				return qfalse;
			}

			skel->bones[i].parentIndex = md5Bone->parentIndex;
		}

		return qtrue;
	}
	else if (skelAnim->type == AT_PSA && skelAnim->psa)
	{
		psaAnimation_t    *psaAnimation = skelAnim->psa;
		axReferenceBone_t *refBone;
		md5Bone_t         *md5Bone;

		if (md5Model->numBones != psaAnimation->info.numBones)
		{
			Ren_Warning("RE_CheckSkeleton: model '%s' has different number of bones than animation '%s': %d != %d\n", model->name, skelAnim->name, md5Model->numBones, psaAnimation->info.numBones);
			return qfalse;
		}

		// check bone names
		for (i = 0, md5Bone = md5Model->bones, refBone = psaAnimation->bones; i < md5Model->numBones; i++, md5Bone++, refBone++)
		{
			if (Q_stricmp(md5Bone->name, refBone->name))
			{
				return qfalse;
			}

			skel->bones[i].parentIndex = md5Bone->parentIndex;
		}

		return qtrue;
	}

	Ren_Warning("RE_BuildSkeleton: bad animation '%s' with handle %i\n", skelAnim->name, hAnim);

	return qfalse;
}

/**
 * @brief RE_BuildSkeleton
 * @param[out] skel
 * @param[in] hAnim
 * @param[in] startFrame
 * @param[in] endFrame
 * @param[in] frac
 * @param[in] clearOrigin
 * @return
 */
int RE_BuildSkeleton(refSkeleton_t *skel, qhandle_t hAnim, int startFrame, int endFrame, float frac, qboolean clearOrigin)
{
	skelAnimation_t *skelAnim;

	skelAnim = R_GetAnimationByHandle(hAnim);

	if (skelAnim->type == AT_MD5 && skelAnim->md5)
	{
		int            i;
		md5Animation_t *anim = skelAnim->md5;
		md5Channel_t   *channel;
		md5Frame_t     *newFrame, *oldFrame;
		vec3_t         newOrigin, oldOrigin, lerpedOrigin;
		quat_t         newQuat, oldQuat, lerpedQuat;
		int            componentsApplied;

		// Validate the frames so there is no chance of a crash.
		// This will write directly into the entity structure, so
		// when the surfaces are rendered, they don't need to be
		// range checked again.
		//if((startFrame >= anim->numFrames) || (startFrame < 0) || (endFrame >= anim->numFrames) || (endFrame < 0))
		//{
		//  Ren_Developer( "RE_BuildSkeleton: no such frame %d to %d for '%s'\n", startFrame, endFrame, anim->name);
		//  //startFrame = 0;
		//  //endFrame = 0;
		//}

		Q_clamp(startFrame, 0, anim->numFrames - 1);
		Q_clamp(endFrame, 0, anim->numFrames - 1);

		// compute frame pointers
		oldFrame = &anim->frames[startFrame];
		newFrame = &anim->frames[endFrame];

		// calculate a bounding box in the current coordinate system
		for (i = 0; i < 3; i++)
		{
			skel->bounds[0][i] =
			    oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
			skel->bounds[1][i] =
			    oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
		}

		for (i = 0, channel = anim->channels; i < anim->numChannels; i++, channel++)
		{
			// set baseframe values
			VectorCopy(channel->baseOrigin, newOrigin);
			VectorCopy(channel->baseOrigin, oldOrigin);

			quat_copy(channel->baseQuat, newQuat);
			quat_copy(channel->baseQuat, oldQuat);

			componentsApplied = 0;

			// update tranlation bits
			if (channel->componentsBits & COMPONENT_BIT_TX)
			{
				oldOrigin[0] = oldFrame->components[channel->componentsOffset + componentsApplied];
				newOrigin[0] = newFrame->components[channel->componentsOffset + componentsApplied];
				componentsApplied++;
			}

			if (channel->componentsBits & COMPONENT_BIT_TY)
			{
				oldOrigin[1] = oldFrame->components[channel->componentsOffset + componentsApplied];
				newOrigin[1] = newFrame->components[channel->componentsOffset + componentsApplied];
				componentsApplied++;
			}

			if (channel->componentsBits & COMPONENT_BIT_TZ)
			{
				oldOrigin[2] = oldFrame->components[channel->componentsOffset + componentsApplied];
				newOrigin[2] = newFrame->components[channel->componentsOffset + componentsApplied];
				componentsApplied++;
			}

			// update quaternion rotation bits
			if (channel->componentsBits & COMPONENT_BIT_QX)
			{
				((vec_t *) oldQuat)[0] = oldFrame->components[channel->componentsOffset + componentsApplied];
				((vec_t *) newQuat)[0] = newFrame->components[channel->componentsOffset + componentsApplied];
				componentsApplied++;
			}

			if (channel->componentsBits & COMPONENT_BIT_QY)
			{
				((vec_t *) oldQuat)[1] = oldFrame->components[channel->componentsOffset + componentsApplied];
				((vec_t *) newQuat)[1] = newFrame->components[channel->componentsOffset + componentsApplied];
				componentsApplied++;
			}

			if (channel->componentsBits & COMPONENT_BIT_QZ)
			{
				((vec_t *) oldQuat)[2] = oldFrame->components[channel->componentsOffset + componentsApplied];
				((vec_t *) newQuat)[2] = newFrame->components[channel->componentsOffset + componentsApplied];
			}

			QuatCalcW(oldQuat);
			quat_norm(oldQuat);

			QuatCalcW(newQuat);
			quat_norm(newQuat);

#if 1
			VectorLerp(oldOrigin, newOrigin, frac, lerpedOrigin);
			quat_slerp(oldQuat, newQuat, frac, lerpedQuat);
#else
			VectorCopy(newOrigin, lerpedOrigin);
			quat_copy(newQuat, lerpedQuat);
#endif

			// copy lerped information to the bone + extra data
			skel->bones[i].parentIndex = channel->parentIndex;

			if (channel->parentIndex < 0 && clearOrigin)
			{
				VectorClear(skel->bones[i].origin);
				QuatClear(skel->bones[i].rotation);

				// move bounding box back
				VectorSubtract(skel->bounds[0], lerpedOrigin, skel->bounds[0]);
				VectorSubtract(skel->bounds[1], lerpedOrigin, skel->bounds[1]);
			}
			else
			{
				VectorCopy(lerpedOrigin, skel->bones[i].origin);
			}

			quat_copy(lerpedQuat, skel->bones[i].rotation);

#if defined(REFBONE_NAMES)
			Q_strncpyz(skel->bones[i].name, channel->name, sizeof(skel->bones[i].name));
#endif
		}

		skel->numBones = anim->numChannels;
		skel->type     = SK_RELATIVE;
		return qtrue;
	}
	else if (skelAnim->type == AT_PSA && skelAnim->psa)
	{
		int               i;
		psaAnimation_t    *anim = skelAnim->psa;
		axAnimationKey_t  *newKey, *oldKey;
		axReferenceBone_t *refBone;
		vec3_t            newOrigin, oldOrigin, lerpedOrigin;
		quat_t            newQuat, oldQuat, lerpedQuat;
		refSkeleton_t     skeleton;

		Q_clamp(startFrame, 0, anim->info.numRawFrames - 1);
		Q_clamp(endFrame, 0, anim->info.numRawFrames - 1);

		ClearBounds(skel->bounds[0], skel->bounds[1]);

		skel->numBones = anim->info.numBones;
		for (i = 0, refBone = anim->bones; i < anim->info.numBones; i++, refBone++)
		{
			oldKey = &anim->keys[startFrame * anim->info.numBones + i];
			newKey = &anim->keys[endFrame * anim->info.numBones + i];

			VectorCopy(newKey->position, newOrigin);
			VectorCopy(oldKey->position, oldOrigin);

			quat_copy(newKey->quat, newQuat);
			quat_copy(oldKey->quat, oldQuat);

			//QuatCalcW(oldQuat);
			//QuatNormalize(oldQuat);

			//QuatCalcW(newQuat);
			//QuatNormalize(newQuat);

			VectorLerp(oldOrigin, newOrigin, frac, lerpedOrigin);
			quat_slerp(oldQuat, newQuat, frac, lerpedQuat);

			// copy lerped information to the bone + extra data
			skel->bones[i].parentIndex = refBone->parentIndex;

			if (refBone->parentIndex < 0 && clearOrigin)
			{
				VectorClear(skel->bones[i].origin);
				QuatClear(skel->bones[i].rotation);

				// move bounding box back
				VectorSubtract(skel->bounds[0], lerpedOrigin, skel->bounds[0]);
				VectorSubtract(skel->bounds[1], lerpedOrigin, skel->bounds[1]);
			}
			else
			{
				VectorCopy(lerpedOrigin, skel->bones[i].origin);
			}

			quat_copy(lerpedQuat, skel->bones[i].rotation);

#if defined(REFBONE_NAMES)
			Q_strncpyz(skel->bones[i].name, refBone->name, sizeof(skel->bones[i].name));
#endif

			// calculate absolute values for the bounding box approximation
			VectorCopy(skel->bones[i].origin, skeleton.bones[i].origin);
			quat_copy(skel->bones[i].rotation, skeleton.bones[i].rotation);

			if (refBone->parentIndex >= 0)
			{
				vec3_t    rotated;
				quat_t    quat;
				refBone_t *bone   = &skeleton.bones[i];
				refBone_t *parent = &skeleton.bones[refBone->parentIndex];

				QuatTransformVector(parent->rotation, bone->origin, rotated);

				VectorAdd(parent->origin, rotated, bone->origin);

				QuatMultiply1(parent->rotation, bone->rotation, quat);
				quat_copy(quat, bone->rotation);

				AddPointToBounds(bone->origin, skel->bounds[0], skel->bounds[1]);
			}
		}

		skel->numBones = anim->info.numBones;
		skel->type     = SK_RELATIVE;
		return qtrue;
	}

	//Ren_Warning( "RE_BuildSkeleton: bad animation '%s' with handle %i\n", anim->name, hAnim);

	// FIXME: clear existing bones and bounds?
	return qfalse;
}

/**
 * @brief RE_BlendSkeleton
 * @param[in] skel
 * @param[in] blend
 * @param[in] frac
 * @return
 */
int RE_BlendSkeleton(refSkeleton_t *skel, const refSkeleton_t *blend, float frac)
{
	int    i;
	vec3_t lerpedOrigin;
	quat_t lerpedQuat;
	vec3_t bounds[2];

	if (skel->numBones != blend->numBones)
	{
		Ren_Warning("RE_BlendSkeleton: different number of bones %d != %d\n", skel->numBones, blend->numBones);
		return qfalse;
	}

	// lerp between the 2 bone poses
	for (i = 0; i < skel->numBones; i++)
	{
		VectorLerp(skel->bones[i].origin, blend->bones[i].origin, frac, lerpedOrigin);
		quat_slerp(skel->bones[i].rotation, blend->bones[i].rotation, frac, lerpedQuat);

		VectorCopy(lerpedOrigin, skel->bones[i].origin);
		quat_copy(lerpedQuat, skel->bones[i].rotation);
	}

	// calculate a bounding box in the current coordinate system
	for (i = 0; i < 3; i++)
	{
		bounds[0][i] = skel->bounds[0][i] < blend->bounds[0][i] ? skel->bounds[0][i] : blend->bounds[0][i];
		bounds[1][i] = skel->bounds[1][i] > blend->bounds[1][i] ? skel->bounds[1][i] : blend->bounds[1][i];
	}
	VectorCopy(bounds[0], skel->bounds[0]);
	VectorCopy(bounds[1], skel->bounds[1]);

	return qtrue;
}

/**
 * @brief RE_AnimNumFrames
 * @param[in] hAnim
 * @return
 *
 * @todo Unused ?
 */
int RE_AnimNumFrames(qhandle_t hAnim)
{
	skelAnimation_t *anim;

	anim = R_GetAnimationByHandle(hAnim);

	if (anim->type == AT_MD5 && anim->md5)
	{
		return anim->md5->numFrames;
	}

	if (anim->type == AT_PSA && anim->psa)
	{
		return anim->psa->info.numRawFrames;
	}

	return 0;
}

/**
 * @brief RE_AnimFrameRate
 * @param[in] hAnim
 * @return
 *
 * @todo Unused ?
 */
int RE_AnimFrameRate(qhandle_t hAnim)
{
	skelAnimation_t *anim;

	anim = R_GetAnimationByHandle(hAnim);

	if (anim->type == AT_MD5 && anim->md5)
	{
		return anim->md5->frameRate;
	}

	if (anim->type == AT_PSA && anim->psa)
	{
		return anim->psa->info.frameRate;
	}

	return 0;
}

#endif
