#include "cg_local.h"

// Runtime debug toggle (0 = off). Adjust via debugger/cheat if needed.
static int cg_iconfeed_debug = 0;

typedef struct
{
	uint8_t active;
	int timestamp;       // Actual spawn time used for expiry, fading and draw order.
	cgHudIconFeedType_t type;
	qhandle_t shader;
	vec4_t color;
} cg_hud_iconfeed_entry_t;

#define MAX_ENTRIES 16

// Target icon size (unscaled)
static const int   ICON_SIZE_W = 26;
static const int   ICON_SIZE_H = 26;
static const float ICON_ALPHA  = 1.0f;

static const float FADE_IN_SIZE      = 2.0f;
static const int   FADE_IN_DURATION  = 200;
static const float FADE_IN_ALPHA     = 0.01f;
static const int   FADE_OUT_DURATION = 300;
static const float FADE_OUT_ALPHA    = 0.0f;

// Negative values intentionally tighten the row by slightly overlapping icons.
static const int ICON_GAP            = -2;
static const int ICON_SHIFT_DURATION = 200;

typedef struct
{
	uint8_t active;
	float startX;
	float currentX;
	float targetX;
	int animationStart;
} cg_hud_iconfeed_anim_t;

static int                     idx                     = 0;
static cg_hud_iconfeed_entry_t entries[MAX_ENTRIES]    = { 0 };
static cg_hud_iconfeed_anim_t  entryAnims[MAX_ENTRIES] = { 0 };

// Draw an entry centered at (cx, cy) with scale and alpha applied.
static ID_INLINE void CG_Hud_IconFeed_Draw_EntryCentered(int i, int cx, int cy, float scale, float alpha)
{
	const cg_hud_iconfeed_entry_t *e = &entries[i];
	float                         col[4];
	int                           w, h, x, y;

	// width/height scale around center
	w = (int)((float)ICON_SIZE_W * scale + 0.5f);
	h = (int)((float)ICON_SIZE_H * scale + 0.5f);
	if (w < 1)
	{
		w = 1;
	}
	if (h < 1)
	{
		h = 1;
	}

	x = cx - (w >> 1);
	y = cy - (h >> 1);

	// use the entry's RGB; apply dynamic alpha
	col[0] = e->color[0];
	col[1] = e->color[1];
	col[2] = e->color[2];
	col[3] = alpha;


	trap_R_SetColor(col);
	CG_DrawPic(x, y, w, h, e->shader);
	trap_R_SetColor(NULL);
}

// Evaluate the current horizontal position for an icon animation track.
static ID_INLINE float CG_IconFeed_EvaluateAnimatedX(const cg_hud_iconfeed_anim_t *anim, int now)
{
	float frac;

	if (!anim->active)
	{
		return 0.0f;
	}
	if (now <= anim->animationStart)
	{
		return anim->startX;
	}
	if (now >= anim->animationStart + ICON_SHIFT_DURATION)
	{
		return anim->targetX;
	}

	frac = (float)(now - anim->animationStart) / (float)ICON_SHIFT_DURATION;
	return LERP(anim->startX, anim->targetX, frac);
}

// Update an icon animation track and return the x-position to draw this frame.
static ID_INLINE float CG_IconFeed_ResolveAnimatedX(int entryIndex, float targetX, int now)
{
	cg_hud_iconfeed_anim_t *anim = &entryAnims[entryIndex];
	float                  currentX;

	if (!anim->active)
	{
		anim->active         = 1;
		anim->startX         = targetX;
		anim->currentX       = targetX;
		anim->targetX        = targetX;
		anim->animationStart = now;

		return targetX;
	}

	currentX = CG_IconFeed_EvaluateAnimatedX(anim, now);
	if (Q_fabs(anim->targetX - targetX) > 0.5f)
	{
		anim->startX         = currentX;
		anim->targetX        = targetX;
		anim->animationStart = now;
		currentX             = anim->startX;
	}

	anim->currentX = currentX;
	return currentX;
}

// Sort active entries oldest-to-newest so the newest icon lands at the row end.
static ID_INLINE void CG_IconFeed_SortDrawOrder(uint8_t *sorted, int n)
{
	int i;

	for (i = 1; i < n; ++i)
	{
		int                 key     = sorted[i];
		int                 keyTime = entries[key].timestamp;
		cgHudIconFeedType_t keyType = entries[key].type;
		int                 k       = i - 1;

		while (k >= 0)
		{
			int                 cur     = sorted[k];
			int                 curTime = entries[cur].timestamp;
			cgHudIconFeedType_t curType = entries[cur].type;

			// Keep same-frame entries deterministic by using the icon type as a tie-breaker.
			if (curTime < keyTime
			    || (curTime == keyTime && curType <= keyType))
			{
				break;
			}
			sorted[k + 1] = (uint8_t)cur;
			--k;
		}
		sorted[k + 1] = (uint8_t)key;
	}
}

void CG_Hud_IconFeed_Add(cgHudIconFeedType_t type)
{
	static vec4_t color;

	cg_hud_iconfeed_entry_t e;
	int                     i;

	// Every obituary event becomes a standalone icon entry.
	e.active    = 1;
	e.timestamp = cg.time;
	e.type      = type;

	// default color: white, alpha will be set to fade-in start
	color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = FADE_IN_ALPHA;

	switch (e.type)
	{
	case CG_HUD_ICONFEED_KILL:
		e.shader = cgs.media.pmImages[PM_DEATH];
		vec4_copy(color, e.color);
		break;
	case CG_HUD_ICONFEED_KILL_SELF:
		e.active = 2;
		e.shader = cgs.media.pmImages[PM_DEATH];
		color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.0f; color[3] = FADE_IN_ALPHA;
		vec4_copy(color, e.color);
		break;
	case CG_HUD_ICONFEED_KILL_TEAM:
		e.active = 2;
		e.shader = cgs.media.pmImages[PM_DEATH];
		color[0] = 1.0f; color[1] = 0.0f; color[2] = 0.0f; color[3] = FADE_IN_ALPHA;
		vec4_copy(color, e.color);
		break;
	default:
		e.shader = cgs.media.spawnInvincibleShader;
		vec4_copy(color, e.color);
		break;
	}

	// for placement, pick first free slot scanning from rotating idx
	for (i = 0; i < MAX_ENTRIES; ++i)
	{
		int probe = (idx + i) % MAX_ENTRIES;

		if (cg_iconfeed_debug)
		{
			Com_Printf("tried: %d\n", probe);
		}

		if (entries[probe].active != 0)
		{
			continue;
		}

		idx                    = probe;
		entries[idx]           = e;
		entryAnims[idx].active = 0;

		if (cg_iconfeed_debug)
		{
			Com_Printf("took: %d\n", idx);
		}

		break;
	}
}

void CG_DrawIconFeed(hudComponent_t *comp)
{
	int     i;
	int     drawCount;
	uint8_t drawItems[MAX_ENTRIES];
	int     anchorCX;
	int     anchorCY;
	int     stayTime;
	int     totalDrawn;

	anchorCX = (int)(comp->location.x + comp->location.w * 0.5f + 0.5f);
	anchorCY = (int)(comp->location.y + comp->location.h * 0.5f + 0.5f);
	// Prevent negative values from extending icon lifetime due to signed math.
	stayTime   = cg_iconfeedStayTime.integer < 0 ? 0 : cg_iconfeedStayTime.integer;
	totalDrawn = 0;

	// 1) Expire timed-out entries first
	for (i = 0; i < MAX_ENTRIES; ++i)
	{
		if (entries[i].active != 0 && cg.time > (entries[i].timestamp + stayTime))
		{
			if (cg_iconfeed_debug)
			{
				Com_Printf("free: %d\n", i);
			}

			entries[i].active    = 0;
			entryAnims[i].active = 0;
		}
		else if (entries[i].active == 0)
		{
			entryAnims[i].active = 0;
		}
	}

	// 2) Collect active entries and sort them into one row.
	drawCount = 0;
	for (i = 0; i < MAX_ENTRIES; ++i)
	{
		if (entries[i].active != 0)
		{
			drawItems[drawCount++] = (uint8_t)i;
		}
	}

	if (drawCount > 1)
	{
		CG_IconFeed_SortDrawOrder(drawItems, drawCount);
	}

	if (cg_iconfeed_debug)
	{
		for (i = 0; i < drawCount; ++i)
		{
			Com_Printf("drawItems[%d]=%d ts=%d type=%d\n",
			           i,
			           drawItems[i],
			           entries[drawItems[i]].timestamp,
			           (int)entries[drawItems[i]].type);
		}
	}

	// 3) Draw a single centered row with uniform spacing and sizing for every icon.
	if (drawCount > 0)
	{
		float iconScale;
		int   innerStep;
		int   now;

		iconScale = Com_Clamp(0.25f, 3.0f, cg_iconfeedScale.value);
		innerStep = (int)((float)(ICON_SIZE_W + ICON_GAP) * iconScale + 0.5f);
		now       = cg.time;
		if (innerStep < 1)
		{
			innerStep = 1;
		}

		if (cg_iconfeed_debug)
		{
			Com_Printf("row draw: count=%d iconScale=%.3f innerStep=%d\n",
			           drawCount, iconScale, innerStep);
		}

		for (i = 0; i < drawCount; ++i)
		{
			int   idxEntry;
			int   born;
			int   timeLeft;
			int   dt;
			int   doFadeOut;
			int   cx;
			float alpha;
			float alphaOut;
			float currentX;
			float p;
			float scale;
			float targetX;

			idxEntry  = drawItems[i];
			born      = entries[idxEntry].timestamp;
			timeLeft  = (born + stayTime) - now;
			doFadeOut = 0;
			alphaOut  = ICON_ALPHA;

			// Center the entire row and keep every icon on the same spacing grid.
			targetX = (float)anchorCX
			          - ((float)(drawCount - 1) * (float)innerStep) * 0.5f
			          + (float)i * (float)innerStep;
			currentX = CG_IconFeed_ResolveAnimatedX(idxEntry, targetX, now);
			cx       = (int)(currentX + 0.5f);

			dt = now - born;
			if (dt < 0)
			{
				dt = 0;
			}
			if (dt >= FADE_IN_DURATION)
			{
				p = 1.0f;
			}
			else
			{
				p = (float)dt / (float)FADE_IN_DURATION;
			}

			// Preserve the existing pop-in animation while honoring the cvar scale.
			scale = iconScale * (FADE_IN_SIZE + (1.0f - FADE_IN_SIZE) * p);
			alpha = FADE_IN_ALPHA + (ICON_ALPHA - FADE_IN_ALPHA) * p;

			if (timeLeft <= FADE_OUT_DURATION)
			{
				if (timeLeft < 0)
				{
					timeLeft = 0;
				}
				doFadeOut = 1;
				{
					float q = 1.0f - ((float)timeLeft / (float)FADE_OUT_DURATION);
					alphaOut = ICON_ALPHA + (FADE_OUT_ALPHA - ICON_ALPHA) * q;
				}
				if (alpha > alphaOut)
				{
					alpha = alphaOut;
				}
			}

			// Store the resolved alpha so debug output matches the final draw state.
			entries[idxEntry].color[3] = alpha;

			if (cg_iconfeed_debug)
			{
				Com_Printf("    idx=%d cx=%d target=%.2f p=%.3f s=%.3f a=%.3f%s type=%d\n",
				           idxEntry, cx, targetX, p, scale, alpha,
				           doFadeOut ? " (fadeOut)" : "", (int)entries[idxEntry].type);
			}

			CG_Hud_IconFeed_Draw_EntryCentered(idxEntry, cx, anchorCY, scale, alpha);
			totalDrawn++;
		}
	}

	if (cg_iconfeed_debug)
	{
		Com_Printf("total drawn: %d\n", totalDrawn);
	}
}
