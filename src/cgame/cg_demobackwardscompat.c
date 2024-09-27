#include "cg_local.h"

static qboolean CG_DemoVersionBelow(int major, int minor, int patch)
{
	if (cg.demoVersion.major == 0 && cg.demoVersion.minor == 0 && cg.demoVersion.patch == 0)
	{
		return qfalse;
	}

	if (cg.demoVersion.major < major)
	{
		return qtrue;
	}
	else if (cg.demoVersion.minor < minor)
	{
		return qtrue;
	}
	else if (cg.demoVersion.patch < patch)
	{
		return qtrue;
	}

	return qfalse;
}

void CG_DemoBackwardsCompatInit()
{
}
