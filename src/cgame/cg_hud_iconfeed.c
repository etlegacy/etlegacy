#include "cg_local.h"

/* Runtime debug toggle (0 = off). Adjust via debugger/cheat if needed. */
static int cg_iconfeed_debug = 0;

typedef struct
{
	uint8_t active;
	int timestamp;       /* Actual spawn time used for expiry and fading. */
	int groupTimestamp;  /* Recency key used to order display groups. */
	int groupId;         /* Group identifier used to keep a window contiguous. */
	cgHudIconFeedType_t type;
	qhandle_t shader;
	vec4_t color;
} cg_hud_iconfeed_entry_t;

#define MAX_ENTRIES 16

/* Target icon size (unscaled) */
static const int   icon_size_w = 29;
static const int   icon_size_h = 29;
static const float icon_alpha  = 0.8f;

static const float fade_in_size      = 2.0f;
static const int   fade_in_duration  = 200;
static const float fade_in_alpha     = 0.01f;
static const int   fade_out_duration = 300;
static const float fade_out_alpha    = 0.0f;

/* Horizontal gap between icons that share a consecutive-kill window. */
static const int icon_consecutive_gap = 0;

/* Gap between separate consecutive-kill groups. */
static const int icon_group_gap      = 8;
static const int icon_shift_duration = 200;

/* Older groups are intentionally deemphasized relative to the current group. */
static const float icon_previous_group_scale = 0.85f;

/* Kills within this window keep extending the current consecutive group. */
static const int icon_kill_group_window = 3000;

typedef struct
{
	uint8_t active;
	float startX;
	float currentX;
	float targetX;
	float startY;
	float currentY;
	float targetY;
	int animationStart;
} cg_hud_iconfeed_anim_t;

static int                     idx                     = 0;
static cg_hud_iconfeed_entry_t entries[MAX_ENTRIES]    = { 0 };
static cg_hud_iconfeed_anim_t  entryAnims[MAX_ENTRIES] = { 0 };
static int                     s_nextGroupId           = 1;            /* Monotonic display-group ids. */
static int                     s_lastKillGroupId       = 0;            /* Active consecutive kill window id. */
static int                     s_lastKillTime          = -2147483647;  /* Last kill time used to extend the window. */

/* Draw an entry centered at (cx, cy) with scale and alpha applied. */
static ID_INLINE void CG_Hud_IconFeed_Draw_EntryCentered(int i, int cx, int cy, float scale, float alpha)
{
	const cg_hud_iconfeed_entry_t *e = &entries[i];
	float                         col[4];
	int                           w, h, x, y;

	/* width/height scale around center */
	w = (int)((float)icon_size_w * scale + 0.5f);
	h = (int)((float)icon_size_h * scale + 0.5f);
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

	/* use the entry's RGB; apply dynamic alpha */
	col[0] = e->color[0];
	col[1] = e->color[1];
	col[2] = e->color[2];
	col[3] = alpha;


	trap_R_SetColor(col);
	CG_DrawPic(x, y, w, h, e->shader);
	trap_R_SetColor(NULL);
}

/* Evaluate the current icon position for an animation track. */
static ID_INLINE void CG_IconFeed_EvaluateAnimatedPosition(const cg_hud_iconfeed_anim_t *anim, int now, float *x, float *y)
{
	float frac;

	if (!anim->active)
	{
		*x = 0.0f;
		*y = 0.0f;
		return;
	}
	if (now <= anim->animationStart)
	{
		*x = anim->startX;
		*y = anim->startY;
		return;
	}
	if (now >= anim->animationStart + icon_shift_duration)
	{
		*x = anim->targetX;
		*y = anim->targetY;
		return;
	}

	frac = (float)(now - anim->animationStart) / (float)icon_shift_duration;
	*x   = LERP(anim->startX, anim->targetX, frac);
	*y   = LERP(anim->startY, anim->targetY, frac);
}

/* Update an icon animation track and return the draw position for this frame. */
static ID_INLINE void CG_IconFeed_ResolveAnimatedPosition(int entryIndex, float targetX, float targetY, int now, float *x, float *y)
{
	cg_hud_iconfeed_anim_t *anim = &entryAnims[entryIndex];
	float                  currentX;
	float                  currentY;

	if (!anim->active)
	{
		anim->active         = 1;
		anim->startX         = targetX;
		anim->currentX       = targetX;
		anim->targetX        = targetX;
		anim->startY         = targetY;
		anim->currentY       = targetY;
		anim->targetY        = targetY;
		anim->animationStart = now;

		*x = targetX;
		*y = targetY;
		return;
	}

	CG_IconFeed_EvaluateAnimatedPosition(anim, now, &currentX, &currentY);
	if (Q_fabs(anim->targetX - targetX) > 0.5f || Q_fabs(anim->targetY - targetY) > 0.5f)
	{
		anim->startX         = currentX;
		anim->startY         = currentY;
		anim->targetX        = targetX;
		anim->targetY        = targetY;
		anim->animationStart = now;
		currentX             = anim->startX;
		currentY             = anim->startY;
	}

	anim->currentX = currentX;
	anim->currentY = currentY;
	*x             = currentX;
	*y             = currentY;
}

/* Return qtrue when the entry participates in the kill-consecutive window logic. */
static ID_INLINE qboolean CG_IconFeed_IsKillType(cgHudIconFeedType_t type)
{
	return type == CG_HUD_ICONFEED_KILL
	       || type == CG_HUD_ICONFEED_KILL_SELF
	       || type == CG_HUD_ICONFEED_KILL_TEAM;
}

/* Sort active entries by display-group recency, then keep group members contiguous. */
static ID_INLINE void CG_IconFeed_SortDrawOrder(uint8_t *sorted, int n)
{
	int i;

	for (i = 1; i < n; ++i)
	{
		int                 key        = sorted[i];
		int                 keyGroupId = entries[key].groupId;
		int                 keyTime    = entries[key].groupTimestamp;
		cgHudIconFeedType_t keyType    = entries[key].type;
		int                 k          = i - 1;

		while (k >= 0)
		{
			int                 cur        = sorted[k];
			int                 curGroupId = entries[cur].groupId;
			int                 curTime    = entries[cur].groupTimestamp;
			cgHudIconFeedType_t curType    = entries[cur].type;

			if (curTime < keyTime
			    || (curTime == keyTime && (curGroupId < keyGroupId
			                               || (curGroupId == keyGroupId && curType <= keyType))))
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

	int                     i;
	int                     groupId;
	int                     groupTimestamp;
	int                     oldGroupId;
	qboolean                extendKillGroup;
	qboolean                isKillEntry;
	cg_hud_iconfeed_entry_t e;

	isKillEntry     = CG_IconFeed_IsKillType(type);
	extendKillGroup = qfalse;
	groupId         = 0;
	groupTimestamp  = cg.time;
	oldGroupId      = 0;

	if (isKillEntry
	    && s_lastKillGroupId != 0
	    && (cg.time - s_lastKillTime) <= icon_kill_group_window)
	{
		extendKillGroup = qtrue;
		oldGroupId      = s_lastKillGroupId;
		groupId         = s_nextGroupId;
	}
	else
	{
		groupId = s_nextGroupId;
	}

	e.active         = 1;
	e.timestamp      = cg.time;
	e.groupTimestamp = groupTimestamp;
	e.groupId        = groupId;
	e.type           = type;

	/* default color: white, alpha will be set to fade-in start */
	color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = fade_in_alpha;

	switch (e.type)
	{
	case CG_HUD_ICONFEED_KILL:
		e.shader = cgs.media.pmImages[PM_DEATH];
		vec4_copy(color, e.color);
		break;
	case CG_HUD_ICONFEED_KILL_SELF:
		e.active = 2;
		e.shader = cgs.media.pmImages[PM_DEATH];
		color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.0f; color[3] = fade_in_alpha;
		vec4_copy(color, e.color);
		break;
	case CG_HUD_ICONFEED_KILL_TEAM:
		e.active = 2;
		e.shader = cgs.media.pmImages[PM_DEATH];
		color[0] = 1.0f; color[1] = 0.0f; color[2] = 0.0f; color[3] = fade_in_alpha;
		vec4_copy(color, e.color);
		break;
	default:
		e.shader = cgs.media.spawnInvincibleShader;
		vec4_copy(color, e.color);
		break;
	}

	/* for placement, pick first free slot scanning from rotating idx */
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

		if (extendKillGroup)
		{
			for (i = 0; i < MAX_ENTRIES; ++i)
			{
				if (entries[i].active != 0 && entries[i].groupId == oldGroupId)
				{
					entries[i].groupId        = groupId;
					entries[i].groupTimestamp = groupTimestamp;
				}
			}
		}

		s_nextGroupId++;

		if (isKillEntry)
		{
			s_lastKillGroupId = groupId;
			s_lastKillTime    = cg.time;
		}

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
	/* Prevent negative values from extending icon lifetime due to signed math. */
	stayTime   = cg_iconfeedStayTime.integer < 0 ? 0 : cg_iconfeedStayTime.integer;
	totalDrawn = 0;

	/* 1) Expire timed-out entries first */
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

	/* 2) Collect active entries and sort them into display groups. */
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
			Com_Printf("drawItems[%d]=%d ts=%d groupTs=%d groupId=%d type=%d\n",
			           i,
			           drawItems[i],
			           entries[drawItems[i]].timestamp,
			           entries[drawItems[i]].groupTimestamp,
			           entries[drawItems[i]].groupId,
			           (int)entries[drawItems[i]].type);
		}
	}

	/* 3) Lay out each display group either leftward or downward from the anchor. */
	if (drawCount > 0)
	{
		float    iconScale;
		float    groupCenterX[MAX_ENTRIES];
		float    groupCenterY[MAX_ENTRIES];
		float    groupScale[MAX_ENTRIES];
		int      baseIconWidth;
		int      baseIconHeight;
		int      groupCount;
		int      groupIndex;
		int      groupInnerStep[MAX_ENTRIES];
		int      groupHeight[MAX_ENTRIES];
		int      groupLen[MAX_ENTRIES];
		int      groupStart[MAX_ENTRIES];
		int      groupWidth[MAX_ENTRIES];
		int      innerStep;
		int      groupGap;
		int      now;
		qboolean verticalLayout;

		iconScale      = Com_Clamp(0.25f, 3.0f, cg_iconfeedScale.value);
		baseIconWidth  = (int)((float)icon_size_w * iconScale + 0.5f);
		baseIconHeight = (int)((float)icon_size_h * iconScale + 0.5f);
		innerStep      = (int)((float)(icon_size_w + icon_consecutive_gap) * iconScale + 0.5f);
		groupGap       = (int)((float)icon_group_gap * iconScale + 0.5f);
		now            = cg.time;
		verticalLayout = (comp->style & ICONFEED_VERTICAL) != 0;

		if (baseIconWidth < 1)
		{
			baseIconWidth = 1;
		}
		if (baseIconHeight < 1)
		{
			baseIconHeight = 1;
		}
		if (innerStep < 1)
		{
			innerStep = 1;
		}
		if (groupGap < 0)
		{
			groupGap = 0;
		}

		/* Build display groups so the newest group can stay centered. */
		groupCount = 0;
		for (i = 0; i < drawCount; ++i)
		{
			if (i == 0 || entries[drawItems[i]].groupId != entries[drawItems[i - 1]].groupId)
			{
				groupStart[groupCount]   = i;
				groupLen[groupCount]     = 0;
				groupWidth[groupCount]   = 0;
				groupHeight[groupCount]  = 0;
				groupCenterX[groupCount] = (float)anchorCX;
				groupCenterY[groupCount] = (float)anchorCY;
				groupCount++;
			}

			groupLen[groupCount - 1]++;
		}

		for (i = 0; i < groupCount; ++i)
		{
			groupScale[i]     = (i == groupCount - 1) ? 1.0f : icon_previous_group_scale;
			groupInnerStep[i] = (int)((float)innerStep * groupScale[i] + 0.5f);
			if (groupInnerStep[i] < 1)
			{
				groupInnerStep[i] = 1;
			}

			groupWidth[i] = (int)((float)baseIconWidth * groupScale[i] + 0.5f)
			                + (groupLen[i] - 1) * groupInnerStep[i];
			groupHeight[i] = (int)((float)baseIconHeight * groupScale[i] + 0.5f);
			if (groupHeight[i] < 1)
			{
				groupHeight[i] = 1;
			}
		}

		if (verticalLayout)
		{
			/* Keep the newest group at the anchor and flow older groups downward. */
			groupCenterX[groupCount - 1] = (float)anchorCX;
			groupCenterY[groupCount - 1] = (float)anchorCY;
			for (i = groupCount - 2; i >= 0; --i)
			{
				groupCenterX[i] = (float)anchorCX;
				groupCenterY[i] = groupCenterY[i + 1]
				                  + ((float)groupHeight[i] + (float)groupHeight[i + 1]) * 0.5f
				                  + (float)groupGap;
			}
		}
		else
		{
			/* Keep the newest group centered and push older groups left. */
			groupCenterX[groupCount - 1] = (float)anchorCX;
			groupCenterY[groupCount - 1] = (float)anchorCY;
			for (i = groupCount - 2; i >= 0; --i)
			{
				groupCenterX[i] = groupCenterX[i + 1]
				                  - ((float)groupWidth[i] + (float)groupWidth[i + 1]) * 0.5f
				                  - (float)groupGap;
				groupCenterY[i] = (float)anchorCY;
			}
		}

		if (cg_iconfeed_debug)
		{
			Com_Printf("%s draw: count=%d groups=%d iconScale=%.3f innerStep=%d groupGap=%d\n",
			           verticalLayout ? "column" : "row",
			           drawCount, groupCount, iconScale, innerStep, groupGap);
		}

		for (groupIndex = 0; groupIndex < groupCount; ++groupIndex)
		{
			if (cg_iconfeed_debug)
			{
				Com_Printf("  group[%d] ts=%d len=%d center=(%.2f, %.2f) width=%d\n",
				           groupIndex,
				           entries[drawItems[groupStart[groupIndex]]].groupTimestamp,
				           groupLen[groupIndex],
				           groupCenterX[groupIndex],
				           groupCenterY[groupIndex],
				           groupWidth[groupIndex]);
			}

			for (i = 0; i < groupLen[groupIndex]; ++i)
			{
				int   drawIndex;
				int   idxEntry;
				int   born;
				int   timeLeft;
				int   dt;
				int   doFadeOut;
				int   cx;
				int   cy;
				float alpha;
				float alphaOut;
				float currentX;
				float currentY;
				float p;
				float scale;
				float targetX;
				float targetY;

				drawIndex = groupStart[groupIndex] + i;
				idxEntry  = drawItems[drawIndex];
				born      = entries[idxEntry].timestamp;
				timeLeft  = (born + stayTime) - now;
				doFadeOut = 0;
				alphaOut  = icon_alpha;

				targetX = groupCenterX[groupIndex]
				          - ((float)(groupLen[groupIndex] - 1) * (float)groupInnerStep[groupIndex]) * 0.5f
				          + (float)i * (float)groupInnerStep[groupIndex];
				targetY = groupCenterY[groupIndex];
				CG_IconFeed_ResolveAnimatedPosition(idxEntry, targetX, targetY, now, &currentX, &currentY);
				cx = (int)(currentX + 0.5f);
				cy = (int)(currentY + 0.5f);

				dt = now - born;
				if (dt < 0)
				{
					dt = 0;
				}
				if (dt >= fade_in_duration)
				{
					p = 1.0f;
				}
				else
				{
					p = (float)dt / (float)fade_in_duration;
				}

				/* Preserve the existing pop-in animation while honoring the cvar scale. */
				scale = iconScale * groupScale[groupIndex] * (fade_in_size + (1.0f - fade_in_size) * p);
				alpha = fade_in_alpha + (icon_alpha - fade_in_alpha) * p;

				if (timeLeft <= fade_out_duration)
				{
					if (timeLeft < 0)
					{
						timeLeft = 0;
					}
					doFadeOut = 1;
					{
						float q = 1.0f - ((float)timeLeft / (float)fade_out_duration);
						alphaOut = icon_alpha + (fade_out_alpha - icon_alpha) * q;
					}
					if (alpha > alphaOut)
					{
						alpha = alphaOut;
					}
				}

				/* Store the resolved alpha so debug output matches the final draw state. */
				entries[idxEntry].color[3] = alpha;

				if (cg_iconfeed_debug)
				{
					Com_Printf("    idx=%d cx=%d cy=%d target=(%.2f, %.2f) p=%.3f s=%.3f a=%.3f%s type=%d\n",
					           idxEntry, cx, cy, targetX, targetY, p, scale, alpha,
					           doFadeOut ? " (fadeOut)" : "", (int)entries[idxEntry].type);
				}

				CG_Hud_IconFeed_Draw_EntryCentered(idxEntry, cx, cy, scale, alpha);
				totalDrawn++;
			}
		}
	}

	if (cg_iconfeed_debug)
	{
		Com_Printf("total drawn: %d\n", totalDrawn);
	}
}
