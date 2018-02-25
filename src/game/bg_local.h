/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 * @file bg_local.h
 * @brief Local definitions for the bg (both games) files
 */

#ifndef INCLUDE_BG_LOCAL_H
#define INCLUDE_BG_LOCAL_H

#define MIN_WALK_NORMAL 0.7f     ///< Can't walk on very steep slopes

#define STEPSIZE        18

#define JUMP_VELOCITY   270

/**
 * @struct pml_s
 * @brief All of the locals will be zeroed before each
 * pmove, just to make damn sure we don't have
 * any differences when running on client or server
 */
typedef struct
{
	vec3_t forward, right, up;
	float frametime;

	int msec;

	qboolean walking;
	qboolean groundPlane;
	trace_t groundTrace;

	float impactSpeed;

	vec3_t previous_origin;
	vec3_t previous_velocity;
	int previous_waterlevel;

	// ladders
	qboolean ladder;
} pml_t;

extern pmove_t *pm;
extern pml_t   pml;

// movement parameters
extern float pm_stopspeed;

extern float pm_waterSwimScale;
//extern float pm_waterWadeScale;
extern float pm_slagSwimScale;
//extern float pm_slagWadeScale;

extern float pm_accelerate;
extern float pm_airaccelerate;
extern float pm_wateraccelerate;
extern float pm_slagaccelerate;
extern float pm_flyaccelerate;

extern float pm_friction;
extern float pm_waterfriction;
extern float pm_slagfriction;
//extern float pm_flightfriction;

extern int c_pmove;

void PM_AddTouchEnt(int entityNum);
void PM_AddEvent(int newEvent);

qboolean PM_SlideMove(qboolean gravity);
void PM_StepSlideMove(qboolean gravity);

#endif // #ifndef INCLUDE_BG_LOCAL_H
