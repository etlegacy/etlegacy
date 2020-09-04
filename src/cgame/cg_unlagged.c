/*
===========================================================================
Copyright (C) 2006 Neil Toronto.

This file is part of the Unlagged source code.

Unlagged source code is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Unlagged source code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Unlagged source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"

/**
 * @brief Draws a bounding box around a player.  Called from CG_Player.
 * @param[in] cent
 */
void CG_AddBoundingBox(centity_t *cent)
{
	polyVert_t   verts[4];
	clientInfo_t *ci;
	int          i;
	vec3_t       mins = { -15, -15, -24 };
	vec3_t       maxs = { 15, 15, 32 };
	float        extx, exty, extz;
	vec3_t       corners[8];
	qhandle_t    bboxShader, bboxShader_nocull;

	if (!cg_drawBBox.integer)
	{
		return;
	}

	// don't draw it if it's us in first-person
	if (cent->currentState.number == cg.predictedPlayerState.clientNum &&
	    !cg.renderingThirdPerson)
	{
		return;
	}

	// don't draw it for dead players
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	// get the shader handles
	bboxShader        = trap_R_RegisterShader("bbox");
	bboxShader_nocull = trap_R_RegisterShader("bbox_nocull");

	// if they don't exist, forget it
	if (!bboxShader || !bboxShader_nocull)
	{
		return;
	}

	// get the player's client info
	ci = &cgs.clientinfo[cent->currentState.clientNum];

	// if it's us
	if (cent->currentState.number == cg.predictedPlayerState.clientNum)
	{
		// use the view height
		maxs[2] = cg.predictedPlayerState.viewheight + 6;
	}
	else
	{
		int x, zd, zu;

		// otherwise grab the encoded bounding box
		x  = (cent->currentState.solid & 255);
		zd = ((cent->currentState.solid >> 8) & 255);
		zu = ((cent->currentState.solid >> 16) & 255) - 32;

		mins[0] = mins[1] = -x;
		maxs[0] = maxs[1] = x;
		mins[2] = -zd;
		maxs[2] = zu;
	}

	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];


	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	// set the polygon's vertex colors
	if (ci->team == TEAM_AXIS)
	{
		for (i = 0; i < 4; i++)
		{
			verts[i].modulate[0] = 160;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}
	else if (ci->team == TEAM_ALLIES)
	{
		for (i = 0; i < 4; i++)
		{
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 192;
			verts[i].modulate[3] = 255;
		}
	}
	else
	{
		for (i = 0; i < 4; i++)
		{
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 128;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}

	VectorAdd(cent->lerpOrigin, maxs, corners[3]);

	VectorCopy(corners[3], corners[2]);
	corners[2][0] -= extx;

	VectorCopy(corners[2], corners[1]);
	corners[1][1] -= exty;

	VectorCopy(corners[1], corners[0]);
	corners[0][0] += extx;

	for (i = 0; i < 4; i++)
	{
		VectorCopy(corners[i], corners[i + 4]);
		corners[i + 4][2] -= extz;
	}

	// top
	VectorCopy(corners[0], verts[0].xyz);
	VectorCopy(corners[1], verts[1].xyz);
	VectorCopy(corners[2], verts[2].xyz);
	VectorCopy(corners[3], verts[3].xyz);
	trap_R_AddPolyToScene(bboxShader, 4, verts);

	// bottom
	VectorCopy(corners[7], verts[0].xyz);
	VectorCopy(corners[6], verts[1].xyz);
	VectorCopy(corners[5], verts[2].xyz);
	VectorCopy(corners[4], verts[3].xyz);
	trap_R_AddPolyToScene(bboxShader, 4, verts);

	// top side
	VectorCopy(corners[3], verts[0].xyz);
	VectorCopy(corners[2], verts[1].xyz);
	VectorCopy(corners[6], verts[2].xyz);
	VectorCopy(corners[7], verts[3].xyz);
	trap_R_AddPolyToScene(bboxShader_nocull, 4, verts);

	// left side
	VectorCopy(corners[2], verts[0].xyz);
	VectorCopy(corners[1], verts[1].xyz);
	VectorCopy(corners[5], verts[2].xyz);
	VectorCopy(corners[6], verts[3].xyz);
	trap_R_AddPolyToScene(bboxShader_nocull, 4, verts);

	// right side
	VectorCopy(corners[0], verts[0].xyz);
	VectorCopy(corners[3], verts[1].xyz);
	VectorCopy(corners[7], verts[2].xyz);
	VectorCopy(corners[4], verts[3].xyz);
	trap_R_AddPolyToScene(bboxShader_nocull, 4, verts);

	// bottom side
	VectorCopy(corners[1], verts[0].xyz);
	VectorCopy(corners[0], verts[1].xyz);
	VectorCopy(corners[4], verts[2].xyz);
	VectorCopy(corners[5], verts[3].xyz);
	trap_R_AddPolyToScene(bboxShader_nocull, 4, verts);
}

/**
 * @brief Clamps a cvar between two integer values, returns qtrue if it had to.
 * @param[in] name
 * @param[in,out] vmCvar
 * @param[in] min
 * @param[in] max
 * @return
 */
qboolean CG_Cvar_ClampInt(const char *name, vmCvar_t *vmCvar, int min, int max)
{
	if (vmCvar->integer > max)
	{
		CG_Printf("Allowed values are %d to %d.\n", min, max);

		Com_sprintf(vmCvar->string, MAX_CVAR_VALUE_STRING, "%d", max);
		vmCvar->value   = max;
		vmCvar->integer = max;

		trap_Cvar_Set(name, vmCvar->string);
		return qtrue;
	}

	if (vmCvar->integer < min)
	{
		CG_Printf("Allowed values are %d to %d.\n", min, max);

		Com_sprintf(vmCvar->string, MAX_CVAR_VALUE_STRING, "%d", min);
		vmCvar->value   = min;
		vmCvar->integer = min;

		trap_Cvar_Set(name, vmCvar->string);
		return qtrue;
	}

	return qfalse;
}
