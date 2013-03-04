#include "tr_local.h"

#define LL(x) x = LittleLong(x)
#define LF(x) x = LittleFloat(x)

static qboolean R_LoadMDX(model_t *mod, void *buffer, const char *mod_name)
{
	int           i, j;
	mdxHeader_t   *pinmodel, *mdx;
	mdxFrame_t    *frame;
	short         *bframe;
	mdxBoneInfo_t *bi;
	int           version;
	int           size;
	int           frameSize;

	pinmodel = (mdxHeader_t *) buffer;

	version = LittleLong(pinmodel->version);
	if (version != MDX_VERSION)
	{
		ri.Printf(PRINT_WARNING, "R_LoadMDX: %s has wrong version (%i should be %i)\n", mod_name, version, MDX_VERSION);
		return qfalse;
	}

	mod->type      = MOD_MDX;
	size           = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mdx            = mod->model.mdx = ri.Hunk_Alloc(size, h_low);

	memcpy(mdx, buffer, LittleLong(pinmodel->ofsEnd));

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