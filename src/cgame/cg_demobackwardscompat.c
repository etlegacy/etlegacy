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
	if (CG_DemoVersionBelow(2, 83, 0))
	{
		Q_strncpyz(cg.demoBackwardsCompat.characterAnimationGroup, "animations/human_base_2_82_1.anim", sizeof(cg.demoBackwardsCompat.characterAnimationGroup));
		Q_strncpyz(cg.demoBackwardsCompat.characterAnimationScript, "animations/scripts/human_base_2_82_1.script", sizeof(cg.demoBackwardsCompat.characterAnimationScript));
	}
}
