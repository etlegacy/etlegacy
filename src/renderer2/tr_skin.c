/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_skin.c
 */

#include "tr_local.h"

/*
============================================================================
SKINS
============================================================================
*/

/**
 * @brief This is unfortunate, but the skin files aren't
 * compatable with our normal parsing rules.
 *
 * @param[in,out] data_p
 */
static char *CommaParse(char **data_p)
{
	int         c = 0, len = 0;
	char        *data = *data_p;
	static char com_token[MAX_TOKEN_CHARS];

	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	while (1)
	{
		// skip whitespace
		while ((c = *data) <= ' ')
		{
			if (!c)
			{
				break;
			}
			data++;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if (c == 0)
	{
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p        = (char *)data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while (c > 32 && c != ',');

	if (len == MAX_TOKEN_CHARS)
	{
		//Ren_Print ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = (char *)data;
	return com_token;
}

/**
 * @brief RE_GetSkinModel
 * @param[in] skinid
 * @param[in] type
 * @param[out] name
 * @return
 */
qboolean RE_GetSkinModel(qhandle_t skinid, const char *type, char *name)
{
	int    i;
	int    hash;
	skin_t *skin = tr.skins[skinid];

	hash = Com_HashKey((char *)type, strlen(type));

	for (i = 0; i < skin->numModels; i++)
	{
		if (hash != skin->models[i]->hash)
		{
			continue;
		}
		if (!Q_stricmp(skin->models[i]->type, type))
		{
			// whoops, should've been this way
			Q_strncpyz(name, skin->models[i]->model, sizeof(skin->models[i]->model));
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief RE_GetShaderFromModel
 * @param[in] modelid
 * @param[in] surfnum
 * @param withlightmap set to '0' will create a new shader that is a copy of the one found
 * on the model, without the lighmap stage, if the shader has a lightmap stage - unused
 * @return a shader index for a given model's surface
 *
 * @note Only works for bmodels right now.  Could modify for other models (md3's etc.)
 */
qhandle_t RE_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap)
{
	model_t      *model;
	bspModel_t   *bmodel;
	bspSurface_t *surf;
	shader_t     *shd;

	if (surfnum < 0)
	{
		surfnum = 0;
	}

	model = R_GetModelByHandle(modelid);    // should be correct now

	if (model)
	{
		bmodel = model->bsp;
		if (bmodel && bmodel->firstSurface)
		{
			if (bmodel->numSurfaces == 0)
			{
				Ren_Print("RE_GetShaderFromModel warning: no surface was found.\n");
				return 0;
			}

			// if it's out of range, use the first surface
			if (surfnum >= bmodel->numSurfaces)
			{
				Ren_Print("RE_GetShaderFromModel warning: surface is out of range.\n");
				surfnum = 0;
			}

			surf = bmodel->firstSurface + surfnum;
			// RF, check for null shader (can happen on func_explosive's with botclips attached)
			if (!surf->shader)
			{
				Ren_Print("RE_GetShaderFromModel warning: missing first surface shader.\n");
				return 0;
			}
			//if(surf->shader->lightmapIndex != LIGHTMAP_NONE) {

			/*
			RB: FIXME ?
			if(surf->shader->lightmapIndex > LIGHTMAP_NONE)
			{
			    image_t        *image;
			    long            hash;
			    qboolean        mip = qtrue;	// mip generation on by default

			    // get mipmap info for original texture
			    hash = GenerateImageHashValue(surf->shader->name);
			    for(image = r_imageHashTable[hash]; image; image = image->next)
			    {
			        if(!strcmp(surf->shader->name, image->imgName))
			        {
			            mip = image->mipmap;
			            break;
			        }
			    }
			    shd = R_FindShader(surf->shader->name, LIGHTMAP_NONE, mip);
			    shd->stages[0]->rgbGen = CGEN_LIGHTING_DIFFUSE;	// new
			}
			else
			*/
			{
				shd = surf->shader;
			}

			return shd->index;
		}
	}

	Ren_Print("Warning RE_GetShaderFromModel:  no model for modelid '%i'.\n", modelid);

	return 0;
}

/**
 * @brief RE_RegisterSkin
 * @param[in] name
 * @return
 */
qhandle_t RE_RegisterSkin(const char *name)
{
	skinSurface_t parseSurfaces[MAX_SKIN_SURFACES];
	qhandle_t     hSkin;
	skin_t        *skin;
	skinSurface_t *surf;
	skinModel_t   *model;
	char          *text, *text_p;
	char          *token;
	char          surfName[MAX_QPATH];
	int           totalSurfaces = 0;

	if (!name || !name[0])
	{
		Ren_Print("Empty name passed to RE_RegisterSkin\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Print("Skin name exceeds MAX_QPATH\n");
		return 0;
	}

	// see if the skin is already loaded
	for (hSkin = 1; hSkin < tr.numSkins; hSkin++)
	{
		skin = tr.skins[hSkin];
		if (!Q_stricmp(skin->name, name))
		{
			if (skin->numSurfaces == 0)
			{
				return 0;       // default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if (tr.numSkins == MAX_SKINS)
	{
		Ren_Warning("WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name);
		return 0;
	}

	// - moved things around slightly to fix the problem where you restart
	// a map that has ai characters who had invalid skin names entered
	// in thier "skin" or "head" field

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

#if 0
	// If not a .skin file, load as a single shader
	if (strcmp(name + strlen(name) - 5, ".skin"))
	{
		skin->numSurfaces        = 1;
		skin->surfaces           = ri.Hunk_Alloc(sizeof(skinSurface_t), h_low);
		skin->surfaces[0].shader = R_FindShader(name, SHADER_3D_DYNAMIC, qtrue);
		return hSkin;
	}
#endif

	// load and parse the skin file
	ri.FS_ReadFile(name, (void **)&text);
	if (!text)
	{
		Ren_Developer("WARNING: RE_RegisterSkin '%s' - empty skin or file not in path\n", name);
		return 0;
	}

	tr.numSkins++;
	skin            = (skin_t *)ri.Hunk_Alloc(sizeof(skin_t), h_low);
	tr.skins[hSkin] = skin;
	Q_strncpyz(skin->name, name, sizeof(skin->name));
	skin->numSurfaces = 0;
	skin->numModels   = 0;

	text_p = text;
	while (text_p && *text_p)
	{
		// get surface name
		token = CommaParse(&text_p);
		Q_strncpyz(surfName, token, sizeof(surfName));

		if (!token[0])
		{
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr(surfName);

		if (*text_p == ',')
		{
			text_p++;
		}

		if (!Q_stricmpn(token, "tag_", 4))
		{
			continue;
		}

		if (!Q_stricmpn(token, "md3_", 4))
		{
			if (skin->numModels >= MAX_PART_MODELS)
			{
				Ren_Warning("WARNING: Ignoring models in '%s', the max is %d!\n", name, MAX_PART_MODELS);
				break;
			}

			// this is specifying a model
			model = skin->models[skin->numModels] = ri.Hunk_Alloc(sizeof(skinModel_t), h_low);
			Q_strncpyz(model->type, token, sizeof(model->type));
			model->hash = Com_HashKey(model->type, sizeof(model->type));

			// get the model name
			token = CommaParse(&text_p);

			Q_strncpyz(model->model, token, sizeof(model->model));

			skin->numModels++;
			continue;
		}

		// parse the shader name
		token = CommaParse(&text_p);

		if (skin->numSurfaces < MAX_SKIN_SURFACES)
		{
			surf = &parseSurfaces[skin->numSurfaces];
			Q_strncpyz(surf->name, surfName, sizeof(surf->name));
			// FIXME: bspSurface not not have ::hash yet
			//surf->hash = Com_HashKey(surf->name, sizeof(surf->name));
			surf->shader = R_FindShader(token, SHADER_3D_DYNAMIC, qtrue);
			skin->numSurfaces++;
		}
		totalSurfaces++;
	}

	ri.FS_FreeFile(text);

	if (totalSurfaces > MAX_SKIN_SURFACES)
	{
		ri.Printf(PRINT_WARNING, "WARNING: Ignoring excess surfaces (found %d, max is %d) in skin '%s'!\n",
		          totalSurfaces, MAX_SKIN_SURFACES, name);
	}

	// never let a skin have 0 shaders
	if (skin->numSurfaces == 0)
	{
		return 0;               // use default skin
	}

	// copy surfaces to skin
	skin->surfaces = ri.Hunk_Alloc(skin->numSurfaces * sizeof(skinSurface_t), h_low);
	Com_Memcpy(skin->surfaces, parseSurfaces, skin->numSurfaces * sizeof(skinSurface_t));

	return hSkin;
}

/**
 * @brief R_InitSkins
 */
void R_InitSkins(void)
{
	skin_t *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc(sizeof(skin_t), h_low);
	Q_strncpyz(skin->name, "<default skin>", sizeof(skin->name));
	skin->numSurfaces        = 1;
	skin->surfaces           = ri.Hunk_Alloc(sizeof(skinSurface_t), h_low);
	skin->surfaces[0].shader = tr.defaultShader;
}

/**
 * @brief R_GetSkinByHandle
 * @param[in] hSkin
 * @return
 */
skin_t *R_GetSkinByHandle(qhandle_t hSkin)
{
	if (hSkin < 1 || hSkin >= tr.numSkins)
	{
		return tr.skins[0];
	}
	return tr.skins[hSkin];
}

/**
 * @brief R_SkinList_f
 */
void R_SkinList_f(void)
{
	int    i, j;
	skin_t *skin;

	Ren_Print("------------------\n");

	for (i = 0; i < tr.numSkins; i++)
	{
		skin = tr.skins[i];

		Ren_Print("%3i:%s (%d surfaces)\n", i, skin->name, skin->numSurfaces);
		for (j = 0; j < skin->numSurfaces; j++)
		{
			Ren_Print("       %s = %s\n", skin->surfaces[j].name, skin->surfaces[j].shader->name);
		}
	}
	Ren_Print("------------------\n");
}
