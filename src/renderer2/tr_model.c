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
 * @file renderer2/tr_model.c
 * @brief Model loading and caching
 */

#include "tr_local.h"

qboolean R_LoadMD3(model_t *mod, int lod, void *buffer, int bufferSize, const char *name);

qboolean R_LoadMDC(model_t *mod, int lod, void *buffer, int bufferSize, const char *name);
qboolean R_LoadMDM(model_t *mod, void *buffer, const char *name);
static qboolean R_LoadMDX(model_t *mod, void *buffer, const char *name);

qboolean R_LoadMD5(model_t *mod, byte *buffer, int bufferSize, const char *name);
qboolean R_LoadPSK(model_t *mod, byte *buffer, int bufferSize, const char *name);

/**
 * @brief R_GetModelByHandle
 * @param[in] hModel
 * @return
 */
model_t *R_GetModelByHandle(qhandle_t hModel)
{
	model_t *mod;

	// out of range gets the default model
	if (hModel < 1 || hModel >= tr.numModels)
	{
		return tr.models[0];
	}

	mod = tr.models[hModel];

	return mod;
}

//===============================================================================

/**
 * @brief R_AllocModel
 * @return
 */
model_t *R_AllocModel(void)
{
	model_t *mod;

	if (tr.numModels == MAX_MOD_KNOWN)
	{
		Ren_Print("WARNING R_AllocModel: MAX_MOD_KNOWN reached - returning NULL\n");
		return NULL;
	}

	mod                     = (model_t *)ri.Hunk_Alloc(sizeof(*tr.models[tr.numModels]), h_low);
	mod->index              = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/**
 * @brief Loads in a model for the given name
 * @param[in] name
 * @return Zero will be returned if the model fails to load.
 * An entry will be retained for failed models as an
 * optimization to prevent disk rescanning if they are
 * asked for again.
 */
qhandle_t RE_RegisterModel(const char *name)
{
	model_t   *mod;
	byte      *buffer;
	int       bufferLen = 0;
	int       lod;
	int       ident;
	qboolean  loaded = qfalse;
	qhandle_t hModel;
	int       numLoaded;

	if (!name || !name[0])
	{
		Ren_Developer("RE_RegisterModel: NULL name\n");
		return 0;
	}
	else
	{
		Ren_Developer("RE_RegisterModel model: %s\n", name);
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Print("Model name exceeds MAX_QPATH\n");
		return 0;
	}

	// search the currently loaded models
	for (hModel = 1; hModel < tr.numModels; hModel++)
	{
		mod = tr.models[hModel];
		if (!strcmp(mod->name, name))
		{
			if (mod->type == MOD_BAD)
			{
				Ren_Warning("RE_RegisterModel: bad model '%s' - already registered but in bad condition - returning 0\n", name);
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t
	if ((mod = R_AllocModel()) == NULL)
	{
		Ren_Warning("RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz(mod->name, name, sizeof(mod->name));

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	mod->numLods = 0;

	// load the files
	numLoaded = 0;

	if (strstr(name, ".mds") || strstr(name, ".mdm") || strstr(name, ".mdx") || strstr(name, ".md5mesh") || strstr(name, ".psk"))
	{
		// try loading skeletal file

		loaded    = qfalse;
		bufferLen = ri.FS_ReadFile(name, (void **)&buffer);
		if (buffer)
		{
			ident = LittleLong(*(unsigned *)buffer);
#if 0
			if (ident == MDS_IDENT)
			{
				loaded = R_LoadMDS(mod, buffer, name);
			}
			else
#endif
			if (ident == MDM_IDENT)
			{
				loaded = R_LoadMDM(mod, buffer, name);
			}
			else if (ident == MDX_IDENT)
			{
				loaded = R_LoadMDX(mod, buffer, name);
			}

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
			if (!Q_stricmpn((const char *)buffer, "MD5Version", 10))
			{
				loaded = R_LoadMD5(mod, buffer, bufferLen, name);
			}
			else if (!Q_stricmpn((const char *)buffer, PSK_IDENTSTRING, PSK_IDENTLEN))
			{
				loaded = R_LoadPSK(mod, buffer, bufferLen, name);
			}
#endif
			ri.FS_FreeFile(buffer);
		}

		if (loaded)
		{
			// make sure the VBO glState entries are save
			R_BindNullVBO();
			R_BindNullIBO();

			return mod->index;
		}
	}

	for (lod = MD3_MAX_LODS - 1; lod >= 0; lod--)
	{
		char filename[1024];
		buffer = NULL;

		strcpy(filename, name);
		if (lod != 0)
		{
			char namebuf[80];

			if (strrchr(filename, '.'))
			{
				*strrchr(filename, '.') = 0;
			}
			sprintf(namebuf, "_%d.md3", lod);
			strcat(filename, namebuf);
		}

		filename[strlen(filename) - 1] = '3';   // try MD3 first (changed order for 2.76)
		if (ri.FS_FOpenFileRead(filename, NULL, qfalse) > 0)
		{
			ri.FS_ReadFile(filename, (void **)&buffer);
		}
		if (!buffer)
		{
			filename[strlen(filename) - 1] = 'c';   // try MDC  second
			if (ri.FS_FOpenFileRead(filename, NULL, qfalse) > 0)
			{
				ri.FS_ReadFile(filename, (void **)&buffer);
			}
			if (!buffer)
			{
				if (lod == 0 && mod->numLods > 1)
				{
					if (mod->type == MOD_MESH)
					{
						mod->mdv[0] = Com_AnyOf((void **)mod->mdv, Com_Nelem(mod->mdv));
					}
					Ren_Warning("RE_RegisterModel: found lods, but main model '%s' is missing\n", name);
				}
				continue;
			}
		}

		ident = LittleLong(*(unsigned *)buffer);
		if (ident != MD3_IDENT && ident != MDC_IDENT)
		{
			Ren_Warning("RE_RegisterModel: unknown fileid for %s\n", name);
			ri.FS_FreeFile(buffer);
			goto fail;
		}

		if (ident == MD3_IDENT)
		{
			loaded = R_LoadMD3(mod, lod, buffer, bufferLen, name);
		}
		else if (ident == MDC_IDENT)
		{
			loaded = R_LoadMDC(mod, lod, buffer, bufferLen, name);
		}

		ri.FS_FreeFile(buffer);

		if (!loaded)
		{
			if (lod == 0)
			{
				goto fail;
			}
			else
			{
				break;
			}
		}
		else
		{
			// make sure the VBO glState entries are save
			R_BindNullVBO();
			R_BindNullIBO();

			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
			//if ( lod <= r_lodbias->integer ) {
			//break;
			//}
		}
	}

	// make sure the VBO glState entries are save
	R_BindNullVBO();
	R_BindNullIBO();

	if (numLoaded)
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for (lod--; lod >= 0; lod--)
		{
			mod->numLods++;
			mod->mdv[lod] = mod->mdv[lod + 1];
		}
		return mod->index;
	}
#ifdef ETLEGACY_DEBUG
	else
	{
		Ren_Warning("couldn't load '%s'\n", name);
	}
#endif

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;

	// make sure the VBO glState entries are save
	R_BindNullVBO();
	R_BindNullIBO();

	return 0;
}

/**
 * @brief R_LoadMDX
 * @param[in,out] mod
 * @param[out] buffer
 * @param[in] name
 * @return
 */
static qboolean R_LoadMDX(model_t *mod, void *buffer, const char *name)
{
	int           i, j;
	mdxHeader_t   *pinmodel = (mdxHeader_t *) buffer, *mdx;
	mdxFrame_t    *frame;
	short         *bframe;
	mdxBoneInfo_t *bi;
	int           version;
	int           size;
	int           frameSize;

	version = LittleLong(pinmodel->version);
	if (version != MDX_VERSION)
	{
		Ren_Warning("R_LoadMDX: %s has wrong version (%i should be %i)\n", name, version, MDX_VERSION);
		return qfalse;
	}

	mod->type      = MOD_MDX;
	size           = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mdx            = mod->mdx = (mdxHeader_t *)ri.Hunk_Alloc(size, h_low);

	Com_Memcpy(mdx, buffer, LittleLong(pinmodel->ofsEnd));

	LL(mdx->ident);
	LL(mdx->version);
	LL(mdx->numFrames);
	LL(mdx->numBones);
	LL(mdx->ofsFrames);
	LL(mdx->ofsBones);
	LL(mdx->ofsEnd);
	LL(mdx->torsoParent);

	if (LittleLong(1) != 1)
	{
		// swap all the frames
		frameSize = (int)(sizeof(mdxBoneFrameCompressed_t)) * mdx->numBones;
		for (i = 0; i < mdx->numFrames; i++)
		{
			frame         = (mdxFrame_t *) ((byte *) mdx + mdx->ofsFrames + i * frameSize + i * sizeof(mdxFrame_t));
			frame->radius = LittleFloat(frame->radius);
			for (j = 0; j < 3; j++)
			{
				frame->bounds[0][j]    = LittleFloat(frame->bounds[0][j]);
				frame->bounds[1][j]    = LittleFloat(frame->bounds[1][j]);
				frame->localOrigin[j]  = LittleFloat(frame->localOrigin[j]);
				frame->parentOffset[j] = LittleFloat(frame->parentOffset[j]);
			}

			bframe = (short *)((byte *) mdx + mdx->ofsFrames + i * frameSize + ((i + 1) * sizeof(mdxFrame_t)));
			for (j = 0; j < mdx->numBones * sizeof(mdxBoneFrameCompressed_t) / sizeof(short); j++)
			{
				((short *)bframe)[j] = LittleShort(((short *)bframe)[j]);
			}
		}

		// swap all the bones
		for (i = 0; i < mdx->numBones; i++)
		{
			bi = (mdxBoneInfo_t *) ((byte *) mdx + mdx->ofsBones + i * sizeof(mdxBoneInfo_t));
			LL(bi->parent);
			bi->torsoWeight = LittleFloat(bi->torsoWeight);
			bi->parentDist  = LittleFloat(bi->parentDist);
			LL(bi->flags);
		}
	}

	return qtrue;
}

//=============================================================================

/*
 * @brief R_XMLError
 * @param ctx - unused
 * @param[in] fmt
 *
 * @note Unused
void R_XMLError(void *ctx, const char *fmt, ...)
{
    va_list     argptr;
    static char msg[4096];

    va_start(argptr, fmt);
    Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Ren_Warning("%s", msg);
}
*/

/*
 * @brief R_LoadDAE
 * @param mod - unused
 * @param[out] buffer
 * @param[in] bufferLen
 * @param[in] modName
 * @return
 *
 * @note Unused
static qboolean R_LoadDAE(model_t * mod, void *buffer, int bufferLen, const char *modName)
{
    xmlDocPtr       doc;
    xmlNodePtr      node;

    // setup error function handler
    xmlInitParser();
    xmlSetGenericErrorFunc(NULL, R_XMLError);

    Ren_Print("...loading DAE '%s'\n", modName);

    doc = xmlParseMemory(buffer, bufferLen);
    if(doc == NULL)
    {
        Ren_Warning( "R_LoadDAE: '%s' xmlParseMemory returned NULL\n", modName);
        return qfalse;
    }
    node = xmlDocGetRootElement(doc);

    if(node == NULL)
    {
        Ren_Warning( "R_LoadDAE: '%s' empty document\n", modName);
        xmlFreeDoc(doc);
        return qfalse;
    }

    if(xmlStrcmp(node->name, (const xmlChar *) "COLLADA"))
    {
        Ren_Warning( "R_LoadDAE: '%s' document of the wrong type, root node != COLLADA", modName);
        xmlFreeDoc(doc);
        return qfalse;
    }

    //TODO

    xmlFreeDoc(doc);

    Ren_Print("...finished DAE '%s'\n", modName);

    return qfalse;
}
*/

//=============================================================================

/**
 * @brief RE_BeginRegistration
 * @param[out] glconfigOut
 */
void RE_BeginRegistration(glconfig_t *glconfigOut)
{
	int i;

	R_Init();

	*glconfigOut = glConfig;

	R_IssuePendingRenderCommands();

	tr.visIndex = 0;

	// force markleafs to regenerate
	for (i = 0; i < MAX_VISCOUNTS; i++)
	{
		tr.visClusters[i] = -2;
	}

	R_ClearFlares();

	RE_ClearScene();

	// HACK: give world entity white color for "colored" shader keyword
	tr.worldEntity.e.shaderRGBA[0] = 255;
	tr.worldEntity.e.shaderRGBA[1] = 255;
	tr.worldEntity.e.shaderRGBA[2] = 255;
	tr.worldEntity.e.shaderRGBA[3] = 255;

	tr.worldEntity.e.nonNormalizedAxes = qfalse;

	// RB: world will be never ignored by occusion query test
	tr.worldEntity.occlusionQuerySamples = 1;

	tr.registered = qtrue;

	// NOTE: this sucks, for some reason the first stretch pic is never drawn
	// without this we'd see a white flash on a level load because the very
	// first time the level shot would not be drawn
	RE_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0);
}

//=============================================================================

/**
 * @brief R_ModelInit
 */
void R_ModelInit(void)
{
	model_t *mod;

	// leave a space for NULL model
	tr.numModels = 0;

	mod = R_AllocModel();
	if (mod)
	{
		mod->type = MOD_BAD;
	}
	else
	{
		Ren_Drop("R_ModelInit: R_AllocModel failed");
	}
}

/**
 * @brief R_Modellist_f
 */
void R_Modellist_f(void)
{
	int      i, j, k;
	model_t  *mod;
	int      total         = 0;
	int      totalDataSize = 0;
	qboolean showFrames;

	if (!strcmp(ri.Cmd_Argv(1), "frames"))
	{
		showFrames = qtrue;
	}
	else
	{
		showFrames = qfalse;
	}

	for (i = 1; i < tr.numModels; i++)
	{
		mod = tr.models[i];


		if (mod->type == MOD_MESH)
		{
			for (j = 0; j < MD3_MAX_LODS; j++)
			{
				if (mod->mdv[j] && mod->mdv[j] != mod->mdv[j - 1])
				{
					mdvModel_t   *mdvModel;
					mdvSurface_t *mdvSurface;
					mdvTagName_t *mdvTagName;

					mdvModel = mod->mdv[j];

					total++;
					Ren_Print("%d.%02d MB '%s' LOD = %i\n", mod->dataSize / (1024 * 1024),
					          (mod->dataSize % (1024 * 1024)) * 100 / (1024 * 1024),
					          mod->name, j);

					if (showFrames && mdvModel->numFrames > 1)
					{
						Ren_Print("\tnumSurfaces = %i\n", mdvModel->numSurfaces);
						Ren_Print("\tnumFrames = %i\n", mdvModel->numFrames);

						for (k = 0, mdvSurface = mdvModel->surfaces; k < mdvModel->numSurfaces; k++, mdvSurface++)
						{
							Ren_Print("\t\tmesh = '%s'\n", mdvSurface->name);
							Ren_Print("\t\t\tnumVertexes = %i\n", mdvSurface->numVerts);
							Ren_Print("\t\t\tnumTriangles = %i\n", mdvSurface->numTriangles);
						}
					}

					Ren_Print("\t\tnumTags = %i\n", mdvModel->numTags);
					for (k = 0, mdvTagName = mdvModel->tagNames; k < mdvModel->numTags; k++, mdvTagName++)
					{
						Ren_Print("\t\t\ttagName = '%s'\n", mdvTagName->name);
					}
				}
			}
		}
		else
		{
			Ren_Print("%d.%02d MB '%s'\n", mod->dataSize / (1024 * 1024),
			          (mod->dataSize % (1024 * 1024)) * 100 / (1024 * 1024),
			          mod->name);
			total++;
		}

		totalDataSize += mod->dataSize;
	}

	Ren_Print(" %d.%02d MB total model memory\n", totalDataSize / (1024 * 1024),
	          (totalDataSize % (1024 * 1024)) * 100 / (1024 * 1024));
	Ren_Print(" %i total models\n\n", total);

#if 0                           // not working right with new hunk
	if (tr.world)
	{
		Ren_Print("\n%8i : %s\n", tr.world->dataSize, tr.world->name);
	}
#endif
}

//=============================================================================

/**
 * @brief R_GetTag
 * @param[in] model
 * @param[in] frame
 * @param[in] _tagName
 * @param[in] startTagIndex
 * @param[out] outTag
 * @return
 */
static int R_GetTag(mdvModel_t *model, int frame, const char *_tagName, int startTagIndex, mdvTag_t **outTag)
{
	int          i;
	mdvTag_t     *tag;
	mdvTagName_t *tagName;

	// it is possible to have a bad frame while changing models, so don't error
	frame = Q_bound(0, frame, model->numFrames - 1);

	if (startTagIndex > model->numTags)
	{
		*outTag = NULL;
		return -1;
	}

#if 1
	tag     = model->tags + frame * model->numTags;
	tagName = model->tagNames;
	for (i = 0; i < model->numTags; i++, tag++, tagName++)
	{
		if ((i >= startTagIndex) && !strcmp(tagName->name, _tagName))
		{
			*outTag = tag;
			return i;
		}
	}
#endif

	*outTag = NULL;
	return -1;
}

/**
 * @brief RE_LerpTagQ3A
 * @param[in,out] tag
 * @param[in] handle
 * @param[in] startFrame
 * @param[in] endFrame
 * @param[in] frac
 * @param[in] tagNameIn
 * @return
 *
 * @note Unused
 */
int RE_LerpTagQ3A(orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagNameIn)
{
	mdvTag_t *start, *end;
	int      i;
	float    frontLerp, backLerp;
	model_t  *model;
	char     tagName[MAX_QPATH];
	int      retval;

	Q_strncpyz(tagName, tagNameIn, MAX_QPATH);

	model = R_GetModelByHandle(handle);
	if (!model->mdv[0])
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return -1;
	}

	start = end = NULL;

	// FIXME: retval is reassigned before used, does it was intended ?
	retval = R_GetTag(model->mdv[0], startFrame, tagName, 0, &start);
	retval = R_GetTag(model->mdv[0], endFrame, tagName, 0, &end);
	if (!start || !end)
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return -1;
	}

	frontLerp = frac;
	backLerp  = 1.0f - frac;

	for (i = 0; i < 3; i++)
	{
		tag->origin[i]  = start->origin[i] * backLerp + end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp + end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp + end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp + end->axis[2][i] * frontLerp;
	}
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);
	return retval;
}

/**
 * @brief RE_LerpTagET
 * @param[in,out] tag
 * @param[in] refent
 * @param[in] tagNameIn
 * @param[in] startIndex
 * @return
 */
int RE_LerpTagET(orientation_t *tag, const refEntity_t *refent, const char *tagNameIn, int startIndex)
{
	mdvTag_t  *start, *end;
	int       i;
	float     frontLerp, backLerp;
	model_t   *model;
	char      tagName[MAX_QPATH];       //, *ch;
	int       retval = 0;
	qhandle_t handle;
	int       startFrame, endFrame;
	float     frac;

	handle     = refent->hModel;
	startFrame = refent->oldframe;
	endFrame   = refent->frame;
	frac       = 1.0f - refent->backlerp;

	Q_strncpyz(tagName, tagNameIn, MAX_QPATH);
/*
    // if the tagName has a space in it, then it is passing through the starting tag number
    if (ch = strrchr(tagName, ' ')) {
        *ch = 0;
        ch++;
        startIndex = Q_atoi(ch);
    }
*/
	model = R_GetModelByHandle(handle);
	/*
	if(!model->mdv[0]) //if(!model->model.md3[0] && !model->model.mdc[0] && !model->model.mds)
	{
	    AxisClear(tag->axis);
	    VectorClear(tag->origin);
	    return -1;
	}
	*/

	frontLerp = frac;
	backLerp  = 1.0f - frac;

	start = end = NULL;

	if (model->type == MOD_MESH)
	{
		// old MD3 style
		// FIXME: retval is reassigned before used, does it was intended ?
		retval = R_GetTag(model->mdv[0], startFrame, tagName, startIndex, &start);
		retval = R_GetTag(model->mdv[0], endFrame, tagName, startIndex, &end);

	}
/*
    else if(model->type == MOD_MDS)
    {
        // use bone lerping
        retval = R_GetBoneTag(tag, model->model.mds, startIndex, refent, tagNameIn);

        if(retval >= 0)
        {
            return retval;
        }

        // failed
        return -1;

    }
    */
	else if (model->type == MOD_MDM)
	{
		// use bone lerping
		retval = R_MDM_GetBoneTag(tag, model->mdm, startIndex, refent, tagNameIn);

		if (retval >= 0)
		{
			return retval;
		}

		// failed
		return -1;

	}
#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	else if (model->type == MOD_MD5)
	{
		// Dushan: VS need this first
		vec3_t tmp;

		retval = RE_BoneIndex(handle, tagName);
		if (retval <= 0)
		{
			return -1;
		}
		VectorCopy(refent->skeleton.bones[retval].origin, tag->origin);
		quat_to_axis(refent->skeleton.bones[retval].rotation, tag->axis);
		VectorCopy(tag->axis[2], tmp);
		VectorCopy(tag->axis[1], tag->axis[2]);
		VectorCopy(tag->axis[0], tag->axis[1]);
		VectorCopy(tmp, tag->axis[0]);
		return retval;
	}
#endif
	/*
	else
	{
	    // psuedo-compressed MDC tags
	    mdcTag_t       *cstart, *cend;

	    retval = R_GetMDCTag((byte *) model->model.mdc[0], startFrame, tagName, startIndex, &cstart);
	    retval = R_GetMDCTag((byte *) model->model.mdc[0], endFrame, tagName, startIndex, &cend);

	    // uncompress the MDC tags into MD3 style tags
	    if(cstart && cend)
	    {
	        for(i = 0; i < 3; i++)
	        {
	            ustart.origin[i] = (float)cstart->xyz[i] * MD3_XYZ_SCALE;
	            uend.origin[i] = (float)cend->xyz[i] * MD3_XYZ_SCALE;
	            sangles[i] = (float)cstart->angles[i] * MDC_TAG_ANGLE_SCALE;
	            eangles[i] = (float)cend->angles[i] * MDC_TAG_ANGLE_SCALE;
	        }

	        AnglesToAxis(sangles, ustart.axis);
	        AnglesToAxis(eangles, uend.axis);

	        start = &ustart;
	        end = &uend;
	    }
	    else
	    {
	        start = NULL;
	        end = NULL;
	    }
	}
	*/

	if (!start || !end)
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return -1;
	}

	for (i = 0; i < 3; i++)
	{
		tag->origin[i]  = start->origin[i] * backLerp + end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp + end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp + end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp + end->axis[2][i] * frontLerp;
	}

	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);

	return retval;
}

/**
 * @brief RE_BoneIndex
 * @param[in] hModel
 * @param[in] boneName
 * @return
 */
int RE_BoneIndex(qhandle_t hModel, const char *boneName)
{
	int        i;
	md5Bone_t  *bone;
	md5Model_t *md5;
	model_t    *model;

	model = R_GetModelByHandle(hModel);
	if (!model->md5)
	{
		return -1;
	}
	else
	{
		md5 = model->md5;
	}

	for (i = 0, bone = md5->bones; i < md5->numBones; i++, bone++)
	{
		if (!Q_stricmp(bone->name, boneName))
		{
			return i;
		}
	}

	return -1;
}

/**
 * @brief R_ModelBounds
 * @param[in] handle
 * @param[out] mins
 * @param[out] maxs
 */
void R_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs)
{
	model_t    *model;
	mdvModel_t *header;
	mdvFrame_t *frame;

	model = R_GetModelByHandle(handle);

	if (model->bsp)
	{
		VectorCopy(model->bsp->bounds[0], mins);
		VectorCopy(model->bsp->bounds[1], maxs);
	}
	else if (model->mdv[0])
	{
		header = model->mdv[0];

		frame = header->frames;

		VectorCopy(frame->bounds[0], mins);
		VectorCopy(frame->bounds[1], maxs);
	}
	else if (model->md5)
	{
		VectorCopy(model->md5->bounds[0], mins);
		VectorCopy(model->md5->bounds[1], maxs);
	}
	else
	{
		VectorClear(mins);
		VectorClear(maxs);
	}
}
