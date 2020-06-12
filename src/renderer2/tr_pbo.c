
#include "tr_local.h"

/*
PBO_t* R_CreatePBO(const char* name, pboUsage_t usage)
{
	PBO_t *pbo;
	int   bufUsage, bufDataUsage;

	switch (usage)
	{
	case PBO_USAGE_WRITETOPBO:
		bufUsage = GL_PIXEL_UNPACK_BUFFER;
		bufDataUsage = GL_STREAM_DRAW;
		break;
	case PBO_USAGE_READFROMPBO:
		bufUsage = GL_PIXEL_PACK_BUFFER;
		bufDataUsage = GL_STREAM_READ;
		break;
	default:
		bufUsage = 0; //Prevents warning
		Ren_Fatal("bad pboUsage_t given: %i", usage);
		//break;
	}
	pbo->usage = bufUsage;

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreatePBO: \"%s\" is too long\n", name);
	}

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	pbo = (PBO_t *)ri.Hunk_Alloc(sizeof(*pbo), h_low);
	Com_AddToGrowList(&tr.pbos, pbo);

	Q_strncpyz(pbo->name, name, sizeof(pbo->name));

	glGenBuffers(1, &pbo->handle);

	glBindBuffer(pbo->usage, pbo->handle);
	glBufferData(pbo->usage, vertexesSize, vertexes, bufDataUsage);

	glBindBuffer(pbo->usage, 0);

	GL_CheckErrors();

	return vbo;
}
*/
