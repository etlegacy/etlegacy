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
 * @file cg_particles.c
 */

#include "cg_local.h"

#define MUSTARD     1
#define BLOODRED    2
#define EMISIVEFADE 3
#define GREY75      4
#define ZOMBIE      5

typedef struct particle_s
{
	struct particle_s *next;

	float time;
	float endtime;

	vec3_t org;
	vec3_t vel;
	vec3_t accel;
	int color;
	float alpha;
	float alphavel;
	int type;
	qhandle_t pshader;

	float height;
	float width;

	float endheight;
	float endwidth;

	float start;
	float end;

	float startfade;
	qboolean rotate;
	int snum;

	qboolean link;

	int shaderAnim;
	int roll;

	int accumroll;

} cparticle_t;

typedef enum
{
	P_NONE,
	P_WEATHER,
	P_FLAT,
	P_SMOKE,
	P_ROTATE, // unused
	P_WEATHER_TURBULENT,
	P_ANIM,
	P_DLIGHT_ANIM,
	P_BLEED,
	P_FLAT_SCALEUP,
	P_FLAT_SCALEUP_FADE,
	P_WEATHER_FLURRY,
	P_SMOKE_IMPACT,
	P_BUBBLE,
	P_BUBBLE_TURBULENT,
	P_SPRITE
} particle_type_t;

#define PARTICLES_CFG_NAME "particles/particles.cfg"
#define MAX_SHADER_ANIMS        8
#define MAX_SHADER_ANIM_FRAMES  64

typedef struct shaderAnim_s
{
	char names[MAX_QPATH];
	int counts;
	float STRatio;
	qhandle_t anims[MAX_SHADER_ANIM_FRAMES];

} shaderAnim_t;

static shaderAnim_t shaderAnims[MAX_SHADER_ANIMS];

#define MAX_PARTICLES   1024 * 8

static cparticle_t *active_particles, *free_particles;
static cparticle_t particles[MAX_PARTICLES];

static vec3_t vforward, vright, vup;
static vec3_t rforward, rright, rup;

static float oldtime;

/**
 * @brief CG_ParsePatriclesConfig
 */
static qboolean CG_ParsePatriclesConfig(void)
{
	char         *text_p;
	int          len;
	int          i;
	char         *token;
	char         text[1024];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile(PARTICLES_CFG_NAME, &f, FS_READ);

	if (len <= 0)
	{
		CG_Printf("CG_ParseWeaponConfig: File not found: %s\n", PARTICLES_CFG_NAME);
		return qfalse;
	}

	if (len >= sizeof(text) - 1)
	{
		CG_Printf("CG_ParseWeaponConfig: File %s too long\n", PARTICLES_CFG_NAME);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;

	COM_BeginParseSession("CG_ParseParticlesConfig");

	for (i = 0 ; i < MAX_SHADER_ANIMS ; i++)
	{
		int j;

		token = COM_Parse(&text_p);     // shader name

		if (!token[0])
		{
			break;
		}

		Q_strncpyz(shaderAnims[i].names, token, MAX_QPATH);

		token = COM_Parse(&text_p);     // shader count

		if (!token[0])
		{
			break;
		}

		shaderAnims[i].counts = Q_atoi(token);

		token = COM_Parse(&text_p);     // ST Ratio

		if (!token[0])
		{
			break;
		}

		shaderAnims[i].STRatio = (float)atof(token);

		// parse shader animation
		for (j = 0; j < shaderAnims[i].counts; j++)
		{
			shaderAnims[i].anims[j] = trap_R_RegisterShader(va("%s%i", shaderAnims[i].names, j + 1));
		}
	}

	if (i == MAX_SHADER_ANIMS)
	{
		CG_Printf("CG_ParseParticlesConfig: Error parsing particles animation file: %s\n", PARTICLES_CFG_NAME);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_ClearParticles
 */
void CG_ClearParticles(void)
{
	int i;

	Com_Memset(particles, 0, sizeof(particles));

	free_particles   = &particles[0];
	active_particles = NULL;

	for (i = 0 ; i < MAX_PARTICLES ; i++)
	{
		particles[i].next = &particles[i + 1];
		particles[i].type = 0;
	}
	particles[MAX_PARTICLES - 1].next = NULL;

	oldtime = cg.time;
}

/**
 * @brief CG_InitParticles
 */
void CG_InitParticles(void)
{
	CG_ClearParticles();

	CG_ParsePatriclesConfig();
}

/**
 * @brief CG_AddParticleToScene
 * @param[in,out] p
 * @param[in] org
 * @param alpha - unused
 */
void CG_AddParticleToScene(cparticle_t *p, vec3_t org, float alpha)
{
	polyVert_t verts[4];
	polyVert_t TRIverts[3];

	switch (p->type)
	{
	case P_WEATHER:
	case P_WEATHER_TURBULENT:
	case P_WEATHER_FLURRY:
	case P_BUBBLE:
	case P_BUBBLE_TURBULENT:  // create a front facing polygon
	{
		if (!cg_visualEffects.integer)
		{
			return;
		}

		if (p->type != P_WEATHER_FLURRY)
		{
			if (p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT)
			{
				if (org[2] > p->end)
				{
					p->time = cg.time;
					VectorCopy(org, p->org);   // fixes rare snow flakes that flicker on the ground

					p->org[2] = (p->start + crandom() * 4);

					if (p->type == P_BUBBLE_TURBULENT)
					{
						p->vel[0] = crandom() * 4;
						p->vel[1] = crandom() * 4;
					}
				}
			}
			else
			{
				if (org[2] < p->end)
				{
					p->time = cg.time;
					VectorCopy(org, p->org);   // fixes rare snow flakes that flicker on the ground

					while (p->org[2] < p->end)
					{
						p->org[2] += (p->start - p->end);
					}

					if (p->type == P_WEATHER_TURBULENT)
					{
						p->vel[0] = crandom() * 16;
						p->vel[1] = crandom() * 16;
					}
				}
			}

			// snow pvs check
			if (!p->link)
			{
				return;
			}

			p->alpha = 1.f;
		}

		// had to do this or MAX_POLYS is being exceeded in village1.bsp
		if (VectorDistanceSquared(cg.snap->ps.origin, org) > Square(1024))
		{
			return;
		}

		if (p->type == P_BUBBLE || p->type == P_BUBBLE_TURBULENT)
		{
			vec3_t point;

			VectorMA(org, -p->height, vup, point);
			VectorMA(point, -p->width, vright, point);
			VectorCopy(point, verts[0].xyz);
			verts[0].st[0]       = 0;
			verts[0].st[1]       = 0;
			verts[0].modulate[0] = 255;
			verts[0].modulate[1] = 255;
			verts[0].modulate[2] = 255;
			verts[0].modulate[3] = (byte)(255 * p->alpha);

			VectorMA(org, -p->height, vup, point);
			VectorMA(point, p->width, vright, point);
			VectorCopy(point, verts[1].xyz);
			verts[1].st[0]       = 0;
			verts[1].st[1]       = 1;
			verts[1].modulate[0] = 255;
			verts[1].modulate[1] = 255;
			verts[1].modulate[2] = 255;
			verts[1].modulate[3] = (byte)(255 * p->alpha);

			VectorMA(org, p->height, vup, point);
			VectorMA(point, p->width, vright, point);
			VectorCopy(point, verts[2].xyz);
			verts[2].st[0]       = 1;
			verts[2].st[1]       = 1;
			verts[2].modulate[0] = 255;
			verts[2].modulate[1] = 255;
			verts[2].modulate[2] = 255;
			verts[2].modulate[3] = (byte)(255 * p->alpha);

			VectorMA(org, p->height, vup, point);
			VectorMA(point, -p->width, vright, point);
			VectorCopy(point, verts[3].xyz);
			verts[3].st[0]       = 1;
			verts[3].st[1]       = 0;
			verts[3].modulate[0] = 255;
			verts[3].modulate[1] = 255;
			verts[3].modulate[2] = 255;
			verts[3].modulate[3] = (byte)(255 * p->alpha);
		}
		else
		{
			vec3_t point;

			VectorMA(org, -p->height, vup, point);
			VectorMA(point, -p->width, vright, point);
			VectorCopy(point, TRIverts[0].xyz);
			TRIverts[0].st[0]       = 1;
			TRIverts[0].st[1]       = 0;
			TRIverts[0].modulate[0] = 255;
			TRIverts[0].modulate[1] = 255;
			TRIverts[0].modulate[2] = 255;
			TRIverts[0].modulate[3] = (byte)(255 * p->alpha);

			VectorMA(org, p->height, vup, point);
			VectorMA(point, -p->width, vright, point);
			VectorCopy(point, TRIverts[1].xyz);
			TRIverts[1].st[0]       = 0;
			TRIverts[1].st[1]       = 0;
			TRIverts[1].modulate[0] = 255;
			TRIverts[1].modulate[1] = 255;
			TRIverts[1].modulate[2] = 255;
			TRIverts[1].modulate[3] = (byte)(255 * p->alpha);

			VectorMA(org, p->height, vup, point);
			VectorMA(point, p->width, vright, point);
			VectorCopy(point, TRIverts[2].xyz);
			TRIverts[2].st[0]       = 0;
			TRIverts[2].st[1]       = 1;
			TRIverts[2].modulate[0] = 255;
			TRIverts[2].modulate[1] = 255;
			TRIverts[2].modulate[2] = 255;
			TRIverts[2].modulate[3] = (byte)(255 * p->alpha);
		}
	}
	break;
	case P_SPRITE:
	{
		vec3_t point, rr, ru, rotate_ang;
		float  time   = cg.time - p->time;
		float  time2  = p->endtime - p->time;
		float  ratio  = time / time2;
		float  width  = p->width + (ratio * (p->endwidth - p->width));
		float  height = p->height + (ratio * (p->endheight - p->height));

		if (!cg_visualEffects.integer)
		{
			return;
		}

		if (p->roll)
		{
			vectoangles(cg.refdef_current->viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			AngleVectors(rotate_ang, NULL, rr, ru);
		}

		if (p->roll)
		{
			VectorMA(org, -height, ru, point);
			VectorMA(point, -width, rr, point);
		}
		else
		{
			VectorMA(org, -height, vup, point);
			VectorMA(point, -width, vright, point);
		}
		VectorCopy(point, verts[0].xyz);
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;

		if (p->roll)
		{
			VectorMA(point, 2 * height, ru, point);
		}
		else
		{
			VectorMA(point, 2 * height, vup, point);
		}
		VectorCopy(point, verts[1].xyz);
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;

		if (p->roll)
		{
			VectorMA(point, 2 * width, rr, point);
		}
		else
		{
			VectorMA(point, 2 * width, vright, point);
		}
		VectorCopy(point, verts[2].xyz);
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;

		if (p->roll)
		{
			VectorMA(point, -2 * height, ru, point);
		}
		else
		{
			VectorMA(point, -2 * height, vup, point);
		}
		VectorCopy(point, verts[3].xyz);
		verts[3].st[0]       = 1;
		verts[3].st[1]       = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}
	break;
	case P_SMOKE:
	case P_SMOKE_IMPACT: // create a front rotating facing polygon
	{
		vec3_t point, rup2, rright2, color;
		float  invratio, time, time2, ratio, width, height;

		if (!cg_visualEffects.integer)
		{
			return;
		}

		if (p->type == P_SMOKE_IMPACT && VectorDistanceSquared(cg.snap->ps.origin, org) > Square(1024))
		{
			return;
		}

		if (p->color == MUSTARD)
		{
			VectorSet(color, 0.42f, 0.33f, 0.19f);
		}
		else if (p->color == BLOODRED)
		{
			VectorSet(color, 0.22f, 0, 0);
		}
		else if (p->color == ZOMBIE)
		{
			VectorSet(color, 0.4f, 0.28f, 0.23f);
		}
		else if (p->color == GREY75)
		{
			float len, greyit;

			len = Distance(cg.snap->ps.origin, org);
			if (len == 0.f)
			{
				len = 1;
			}

			//val    = 4096 / len;
			greyit = 0.25f * (4096 / len);
			if (greyit > 0.5f)
			{
				greyit = 0.5f;
			}

			VectorSet(color, greyit, greyit, greyit);
		}
		else
		{
			VectorSet(color, 1.0f, 1.0f, 1.0f);
		}

		time  = cg.time - p->time;
		time2 = p->endtime - p->time;
		ratio = time / time2;

		if (cg.time > p->startfade)
		{
			invratio = 1 - ((cg.time - p->startfade) / (p->endtime - p->startfade));

			if (p->color == EMISIVEFADE)
			{
				float fval = invratio * invratio;

				if (fval < 0)
				{
					fval = 0;
				}
				VectorSet(color, fval, fval, fval);
			}
			invratio *= p->alpha;
		}
		else
		{
			invratio = 1 * p->alpha;
		}

		if (invratio > 1)
		{
			invratio = 1;
		}

		width  = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		//if (p->type != P_SMOKE_IMPACT)
		{
			vec3_t temp;

			vectoangles(rforward, temp);
			p->accumroll += p->roll;
			temp[ROLL]   += p->accumroll * 0.1;
			//temp[ROLL] += p->roll * 0.1;
			AngleVectors(temp, NULL, rright2, rup2);
		}
		//else
		//{
		//VectorCopy (rright, rright2);
		//VectorCopy (rup, rup2);
		//}

		if (p->rotate)
		{
			VectorMA(org, -height, rup2, point);
			VectorMA(point, -width, rright2, point);
		}
		else
		{
			VectorMA(org, -p->height, vup, point);
			VectorMA(point, -p->width, vright, point);
		}
		VectorCopy(point, verts[0].xyz);
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = (byte)(255 * color[0]);
		verts[0].modulate[1] = (byte)(255 * color[1]);
		verts[0].modulate[2] = (byte)(255 * color[2]);
		verts[0].modulate[3] = (byte)(255 * invratio);

		if (p->rotate)
		{
			VectorMA(org, -height, rup2, point);
			VectorMA(point, width, rright2, point);
		}
		else
		{
			VectorMA(org, -p->height, vup, point);
			VectorMA(point, p->width, vright, point);
		}
		VectorCopy(point, verts[1].xyz);
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = (byte)(255 * color[0]);
		verts[1].modulate[1] = (byte)(255 * color[1]);
		verts[1].modulate[2] = (byte)(255 * color[2]);
		verts[1].modulate[3] = (byte)(255 * invratio);

		if (p->rotate)
		{
			VectorMA(org, height, rup2, point);
			VectorMA(point, width, rright2, point);
		}
		else
		{
			VectorMA(org, p->height, vup, point);
			VectorMA(point, p->width, vright, point);
		}
		VectorCopy(point, verts[2].xyz);
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = (byte)(255 * color[0]);
		verts[2].modulate[1] = (byte)(255 * color[1]);
		verts[2].modulate[2] = (byte)(255 * color[2]);
		verts[2].modulate[3] = (byte)(255 * invratio);

		if (p->rotate)
		{
			VectorMA(org, height, rup2, point);
			VectorMA(point, -width, rright2, point);
		}
		else
		{
			VectorMA(org, p->height, vup, point);
			VectorMA(point, -p->width, vright, point);
		}
		VectorCopy(point, verts[3].xyz);
		verts[3].st[0]       = 1;
		verts[3].st[1]       = 0;
		verts[3].modulate[0] = (byte)(255 * color[0]);
		verts[3].modulate[1] = (byte)(255 * color[1]);
		verts[3].modulate[2] = (byte)(255 * color[2]);
		verts[3].modulate[3] = (byte)(255 * invratio);

	}
	break;
	case P_BLEED:
	{
		vec3_t point, rr, ru, rotate_ang;

		if (!cg_visualEffects.integer)
		{
			return;
		}

		if (p->roll)
		{
			vectoangles(cg.refdef_current->viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			AngleVectors(rotate_ang, NULL, rr, ru);
		}
		else
		{
			VectorCopy(vup, ru);
			VectorCopy(vright, rr);
		}

		VectorMA(org, -p->height, ru, point);
		VectorMA(point, -p->width, rr, point);
		VectorCopy(point, verts[0].xyz);
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = 111;
		verts[0].modulate[1] = 19;
		verts[0].modulate[2] = 9;
		verts[0].modulate[3] = (byte)(255 * p->alpha);

		VectorMA(org, -p->height, ru, point);
		VectorMA(point, p->width, rr, point);
		VectorCopy(point, verts[1].xyz);
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = 111;
		verts[1].modulate[1] = 19;
		verts[1].modulate[2] = 9;
		verts[1].modulate[3] = (byte)(255 * p->alpha);

		VectorMA(org, p->height, ru, point);
		VectorMA(point, p->width, rr, point);
		VectorCopy(point, verts[2].xyz);
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = 111;
		verts[2].modulate[1] = 19;
		verts[2].modulate[2] = 9;
		verts[2].modulate[3] = (byte)(255 * p->alpha);

		VectorMA(org, p->height, ru, point);
		VectorMA(point, -p->width, rr, point);
		VectorCopy(point, verts[3].xyz);
		verts[3].st[0]       = 1;
		verts[3].st[1]       = 0;
		verts[3].modulate[0] = 111;
		verts[3].modulate[1] = 19;
		verts[3].modulate[2] = 9;
		verts[3].modulate[3] = (byte)(255 * p->alpha);
	}
	break;
	case P_FLAT_SCALEUP:
	{
		vec3_t color;
		float  width, height;
		float  sinR, cosR;
		float  time  = cg.time - p->time;
		float  time2 = p->endtime - p->time;
		float  ratio = time / time2;

		if (!cg_visualEffects.integer)
		{
			return;
		}

		if (p->color == BLOODRED)
		{
			VectorSet(color, 1, 1, 1);
		}
		else
		{
			VectorSet(color, 0.5f, 0.5f, 0.5f);
		}

		width  = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		if (width > p->endwidth)
		{
			width = p->endwidth;
		}

		if (height > p->endheight)
		{
			height = p->endheight;
		}

		sinR = height * (float)(sin(DEG2RAD(p->roll)) * M_SQRT2);
		cosR = width * (float)(cos(DEG2RAD(p->roll)) * M_SQRT2);

		VectorCopy(org, verts[0].xyz);
		verts[0].xyz[0]     -= sinR;
		verts[0].xyz[1]     -= cosR;
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = (byte)(255 * color[0]);
		verts[0].modulate[1] = (byte)(255 * color[1]);
		verts[0].modulate[2] = (byte)(255 * color[2]);
		verts[0].modulate[3] = 255;

		VectorCopy(org, verts[1].xyz);
		verts[1].xyz[0]     -= cosR;
		verts[1].xyz[1]     += sinR;
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = verts[0].modulate[0];
		verts[1].modulate[1] = verts[0].modulate[1];
		verts[1].modulate[2] = verts[0].modulate[2];
		verts[1].modulate[3] = verts[0].modulate[3];

		VectorCopy(org, verts[2].xyz);
		verts[2].xyz[0]     += sinR;
		verts[2].xyz[1]     += cosR;
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = verts[0].modulate[0];
		verts[2].modulate[1] = verts[0].modulate[1];
		verts[2].modulate[2] = verts[0].modulate[2];
		verts[2].modulate[3] = verts[0].modulate[3];

		VectorCopy(org, verts[3].xyz);
		verts[3].xyz[0]     += cosR;
		verts[3].xyz[1]     -= sinR;
		verts[3].st[0]       = 1;
		verts[3].st[1]       = 0;
		verts[3].modulate[0] = verts[0].modulate[0];
		verts[3].modulate[1] = verts[0].modulate[1];
		verts[3].modulate[2] = verts[0].modulate[2];
		verts[3].modulate[3] = verts[0].modulate[3];
	}
	break;
	case P_FLAT:
	{
		if (!cg_visualEffects.integer)
		{
			return;
		}

		VectorCopy(org, verts[0].xyz);
		verts[0].xyz[0]     -= p->height;
		verts[0].xyz[1]     -= p->width;
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = 255;

		VectorCopy(org, verts[1].xyz);
		verts[1].xyz[0]     -= p->height;
		verts[1].xyz[1]     += p->width;
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = 255;

		VectorCopy(org, verts[2].xyz);
		verts[2].xyz[0]     += p->height;
		verts[2].xyz[1]     += p->width;
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = 255;

		VectorCopy(org, verts[3].xyz);
		verts[3].xyz[0]     += p->height;
		verts[3].xyz[1]     -= p->width;
		verts[3].st[0]       = 1;
		verts[3].st[1]       = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = 255;
	}
	break;
	case P_ANIM:
	case P_DLIGHT_ANIM:
	{
		vec3_t point, rr, ru, rotate_ang;
		float  width, height;
		float  time  = cg.time - p->time;
		float  time2 = p->endtime - p->time;
		float  ratio = time / time2;
		double invratio;
		int    i, j;

		if (ratio >= 1)
		{
			ratio = 0.9999f;
		}
		else if (ratio < 0)
		{
			// make sure that ratio isn't negative or
			// we'll walk out of bounds when j is calculated below
			ratio = 0.0001f;
		}

		width  = p->width + (ratio * (p->endwidth - p->width));
		height = p->height + (ratio * (p->endheight - p->height));

		// add dlight if necessary
		if (p->type == P_DLIGHT_ANIM)
		{
			// fixme: support arbitrary color
			trap_R_AddLightToScene(org, 320,        //%	1.5 * (width > height ? width : height),
			                       1.25f * (1.0f - ratio), 1.0f, 0.95f, 0.85f, 0, 0);
		}

		// if we are "inside" this sprite, don't draw
		if (VectorDistanceSquared(cg.snap->ps.origin, org) < Square(width / 1.5f))
		{
			return;
		}

		i          = p->shaderAnim;
		j          = (int)floor((double)ratio * shaderAnims[p->shaderAnim].counts);
		p->pshader = shaderAnims[i].anims[j];

		if (cg.time > p->startfade)
		{
			//invratio = 1 - (cg.time - p->startfade) / (p->endtime - p->startfade);  // linear
			invratio = pow(0.01, (double)((cg.time - p->startfade) / (p->endtime - p->startfade)));

			if (invratio > 1)
			{
				invratio = 1;
			}
		}
		else
		{
			invratio = 1;
		}

		if (p->roll)
		{
			vectoangles(cg.refdef_current->viewaxis[0], rotate_ang);
			rotate_ang[ROLL] += p->roll;
			AngleVectors(rotate_ang, NULL, rr, ru);
		}

		if (p->roll)
		{
			VectorMA(org, -height, ru, point);
			VectorMA(point, -width, rr, point);
		}
		else
		{
			VectorMA(org, -height, vup, point);
			VectorMA(point, -width, vright, point);
		}
		VectorCopy(point, verts[0].xyz);
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = (byte)(255 * invratio);

		if (p->roll)
		{
			VectorMA(point, 2 * height, ru, point);
		}
		else
		{
			VectorMA(point, 2 * height, vup, point);
		}
		VectorCopy(point, verts[1].xyz);
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = (byte)(255 * invratio);

		if (p->roll)
		{
			VectorMA(point, 2 * width, rr, point);
		}
		else
		{
			VectorMA(point, 2 * width, vright, point);
		}
		VectorCopy(point, verts[2].xyz);
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = (byte)(255 * invratio);

		if (p->roll)
		{
			VectorMA(point, -2 * height, ru, point);
		}
		else
		{
			VectorMA(point, -2 * height, vup, point);
		}
		VectorCopy(point, verts[3].xyz);
		verts[3].st[0]       = 1;
		verts[3].st[1]       = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = (byte)(255 * invratio);
	}
	break;
	default:
		break;
	}

	if (!p->pshader)
	{
		CG_Printf("CG_AddParticleToScene type %d p->pshader == ZERO\n", p->type);
		return;
	}

	if (p->type == P_WEATHER || p->type == P_WEATHER_TURBULENT || p->type == P_WEATHER_FLURRY)
	{
		trap_R_AddPolyToScene(p->pshader, 3, TRIverts);
	}
	else
	{
		trap_R_AddPolyToScene(p->pshader, 4, verts);
	}
}

// made this static so it doesn't interfere with other files
static float roll = 0.0f;

/**
 * @brief CG_AddParticles
 */
void CG_AddParticles(void)
{
	cparticle_t *p, *next;
	float       alpha;
	float       time, time2;
	vec3_t      org;
	cparticle_t *active = NULL, *tail = NULL;
	vec3_t      rotate_ang;

	VectorCopy(cg.refdef_current->viewaxis[0], vforward);
	VectorCopy(cg.refdef_current->viewaxis[1], vright);
	VectorCopy(cg.refdef_current->viewaxis[2], vup);

	vectoangles(cg.refdef_current->viewaxis[0], rotate_ang);
	roll             += ((cg.time - oldtime) * 0.1f) ;
	rotate_ang[ROLL] += (roll * 0.9f);
	AngleVectors(rotate_ang, rforward, rright, rup);

	oldtime = cg.time;

	for (p = active_particles ; p ; p = next)
	{
		next = p->next;

		time = (cg.time - p->time) * 0.001f;

		alpha = p->alpha + time * p->alphavel;
		if (alpha <= 0)     // faded out
		{
			p->next        = free_particles;
			free_particles = p;
			p->type        = 0;
			p->color       = 0;
			p->alpha       = 0;
			continue;
		}

		switch (p->type)
		{
		case P_SMOKE:
		case P_ANIM:
		case P_DLIGHT_ANIM:
		case P_BLEED:
		case P_SMOKE_IMPACT:
		case P_WEATHER_FLURRY:
		case P_FLAT_SCALEUP_FADE:
			if (cg.time > p->endtime)
			{
				p->next        = free_particles;
				free_particles = p;
				p->type        = 0;
				p->color       = 0;
				p->alpha       = 0;
				continue;
			}
			break;
		case P_SPRITE:
			if (p->endtime < 0)
			{
				// temporary sprite
				CG_AddParticleToScene(p, p->org, alpha);
				p->next        = free_particles;
				free_particles = p;
				p->type        = 0;
				p->color       = 0;
				p->alpha       = 0;
				continue;
			}
			break;
		default:
			// P_NONE
			// P_WEATHER
			// P_FLAT
			// P_ROTATE
			// P_WEATHER_TURBULENT
			// P_FLAT_SCALEUP
			// P_BUBBLE
			// P_BUBBLE_TURBULENT
			break;
		}

		p->next = NULL;
		if (!tail)
		{
			active = tail = p;
		}
		else
		{
			tail->next = p;
			tail       = p;
		}

		if (alpha > 1.0f)
		{
			alpha = 1;
		}

		time2 = time * time;

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		CG_AddParticleToScene(p, org, alpha);
	}

	active_particles = active;
}

/**
 * @brief CG_ParticleSnowFlurry
 * @param[in] pshader
 * @param[in] cent
 */
void CG_ParticleSnowFlurry(qhandle_t pshader, centity_t *cent)
{
	cparticle_t *p;

	if (!pshader)
	{
		CG_Printf("CG_ParticleSnowFlurry pshader == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->color         = 0;
	p->alpha         = 0.9f;
	p->alphavel      = 0;

	p->start = cent->currentState.origin2[0];
	p->end   = cent->currentState.origin2[1];

	p->endtime   = cg.time + cent->currentState.time;
	p->startfade = cg.time + cent->currentState.time2;

	p->pshader = pshader;

	if (rand() % 100 > 90)
	{
		p->height = 32;
		p->width  = 32;
		p->alpha  = 0.1f;
	}
	else
	{
		p->height = 1;
		p->width  = 1;
	}

	p->vel[2] = -10;

	p->type = P_WEATHER_FLURRY;

	VectorCopy(cent->currentState.origin, p->org);

	p->vel[0] = p->vel[1] = 0;

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->vel[0] += cent->currentState.angles[0] * 32 + (crandom() * 16);
	p->vel[1] += cent->currentState.angles[1] * 32 + (crandom() * 16);
	p->vel[2] += cent->currentState.angles[2];

	p->accel[0] = crandom() * 16;
	p->accel[1] = crandom() * 16;
}

/**
 * @brief CG_ParticleSnow
 * @param[in] pshader
 * @param[in] origin
 * @param[in] origin2
 * @param[in] turb
 * @param[in] range
 * @param[in] snum
 */
void CG_ParticleSnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum)
{
	cparticle_t *p;

	if (!pshader)
	{
		CG_Printf("CG_ParticleSnow pshader == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->color         = 0;
	p->alpha         = 0.4f;
	p->alphavel      = 0;
	p->start         = origin[2];
	p->end           = origin2[2];
	p->pshader       = pshader;
	p->height        = 1;
	p->width         = 1;

	p->vel[2] = -50;

	if (turb)
	{
		p->type   = P_WEATHER_TURBULENT;
		p->vel[2] = -50 * 1.3;
	}
	else
	{
		p->type = P_WEATHER;
	}

	VectorCopy(origin, p->org);

	p->org[0] = p->org[0] + (crandom() * range);
	p->org[1] = p->org[1] + (crandom() * range);
	p->org[2] = p->org[2] + (crandom() * (p->start - p->end));

	p->vel[0] = p->vel[1] = 0;

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if (turb)
	{
		p->vel[0] = crandom() * 16;
		p->vel[1] = crandom() * 16;
	}

	// snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

/**
 * @brief CG_ParticleBubble
 * @param[in] pshader
 * @param[in] origin
 * @param[in] origin2
 * @param[in] turb
 * @param[in] range
 * @param[in] snum
 */
void CG_ParticleBubble(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum)
{
	cparticle_t *p;
	float       randsize;

	if (!pshader)
	{
		CG_Printf("CG_ParticleSnow pshader == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->color         = 0;
	p->alpha         = 0.4f;
	p->alphavel      = 0;
	p->start         = origin[2];
	p->end           = origin2[2];
	p->pshader       = pshader;

	randsize = 1 + (crandom() * 0.5f);

	p->height = randsize;
	p->width  = randsize;

	p->vel[2] = 50 + (crandom() * 10);

	if (turb)
	{
		p->type   = P_BUBBLE_TURBULENT;
		p->vel[2] = 50 * 1.3;
	}
	else
	{
		p->type = P_BUBBLE;
	}

	VectorCopy(origin, p->org);

	p->org[0] = p->org[0] + (crandom() * range);
	p->org[1] = p->org[1] + (crandom() * range);
	p->org[2] = p->org[2] + (crandom() * (p->start - p->end));

	p->vel[0] = p->vel[1] = 0;

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if (turb)
	{
		p->vel[0] = crandom() * 4;
		p->vel[1] = crandom() * 4;
	}

	// snow pvs check
	p->snum = snum;
	p->link = qtrue;
}

/**
 * @brief CG_ParticleSmoke
 * @param[in] pshader
 * @param[in] cent
 */
void CG_ParticleSmoke(qhandle_t pshader, centity_t *cent)
{
	// using cent->density = enttime
	//cent->frame = startfade
	cparticle_t *p;
	vec3_t      dir;

	if (!pshader)
	{
		CG_Printf("CG_ParticleSmoke == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;

	p->endtime   = cg.time + cent->currentState.time;
	p->startfade = cg.time + cent->currentState.time2;

	p->color    = 0;
	p->alpha    = 1.0;
	p->alphavel = 0;
	p->start    = cent->currentState.origin[2];
	p->end      = cent->currentState.origin2[2];
	p->pshader  = pshader;
	if (cent->currentState.density == 1 || cent->currentState.modelindex2)
	{
		p->rotate    = qfalse;
		p->height    = 8;
		p->width     = 8;
		p->endheight = 32;
		p->endwidth  = 32;
	}
	else if (cent->currentState.density == 2)
	{
		p->rotate    = qtrue;
		p->height    = 4;
		p->width     = 4;
		p->endheight = 8;
		p->endwidth  = 8;
	}
	else if (cent->currentState.density == 3)
	{
		p->rotate = qfalse;
		{
			float scale;

			scale        = 16 + (crandom() * 8);
			p->height    = 24 + scale;
			p->width     = 24 + scale;
			p->endheight = 64 + scale;
			p->endwidth  = 64 + scale;
		}
	}
	else if (cent->currentState.density == 4)           // white smoke
	{
		p->rotate    = qtrue;
		p->height    = cent->currentState.angles2[0];
		p->width     = cent->currentState.angles2[0];
		p->endheight = cent->currentState.angles2[1];
		p->endwidth  = cent->currentState.angles2[1];
		p->color     = GREY75;
	}
	else if (cent->currentState.density == 5)           // mustard gas
	{
		p->rotate    = qtrue;
		p->height    = cent->currentState.angles2[0];
		p->width     = cent->currentState.angles2[0];
		p->endheight = cent->currentState.angles2[1];
		p->endwidth  = cent->currentState.angles2[1];
		p->color     = MUSTARD;
		p->alpha     = 0.75f;
	}
	else   // black smoke
	{
		p->rotate    = qtrue;
		p->height    = cent->currentState.angles2[0];
		p->width     = cent->currentState.angles2[0];
		p->endheight = cent->currentState.angles2[1];
		p->endwidth  = cent->currentState.angles2[1];

		p->pshader = cgs.media.smokePuffShaderb[rand() % 5];
	}

	p->type = P_SMOKE;

	VectorCopy(cent->lerpOrigin, p->org);

	p->vel[0]   = p->vel[1] = 0;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	if (cent->currentState.density == 1)
	{
		p->vel[2] = 5;
	}
	else if (cent->currentState.density == 2)
	{
		p->vel[2] = 5;
	}
	else if (cent->currentState.density == 3)       // cannon
	{
		VectorCopy(cent->currentState.origin2, dir);
		p->vel[0] = dir[0] * 128 + (crandom() * 64);
		p->vel[1] = dir[1] * 128 + (crandom() * 64);
		p->vel[2] = 15 + (crandom() * 16);
	}
	else if (cent->currentState.density == 5)           // gas or cover smoke
	{
		VectorCopy(cent->currentState.origin2, dir);
		p->vel[0] = dir[0] * 32 + (crandom() * 16);
		p->vel[1] = dir[1] * 32 + (crandom() * 16);
		p->vel[2] = 4 + (crandom() * 2);
	}
	else   // smoke
	{
		VectorCopy(cent->currentState.origin2, dir);
		p->vel[0] = dir[0] + (crandom() * p->height);
		p->vel[1] = dir[1] + (crandom() * p->height);
		p->vel[2] = cent->currentState.angles2[2];
	}

	if (cent->currentState.frame == 1)     // reverse gravity
	{
		p->vel[2] *= -1;
	}

	p->roll = (int)(8 + (crandom() * 4));
}

/**
 * @brief CG_ParticleBulletDebris
 * @param[in] org
 * @param[in] vel
 * @param[in] duration
 */
void CG_ParticleBulletDebris(vec3_t org, vec3_t vel, int duration)
{
	cparticle_t *p;

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;

	p->endtime   = cg.time + duration;
	p->startfade = cg.time + duration / 2;

	p->color    = EMISIVEFADE;
	p->alpha    = 1.0f;
	p->alphavel = 0;

	p->height    = 0.5f;
	p->width     = 0.5f;
	p->endheight = 0.5f;
	p->endwidth  = 0.5f;

	p->pshader = cgs.media.tracerShader;

	p->type = P_SMOKE;

	VectorCopy(org, p->org);

	p->vel[0]   = vel[0];
	p->vel[1]   = vel[1];
	p->vel[2]   = vel[2];
	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->accel[2] = -60;
	p->vel[2]  += -20;
}

/**
 * @brief The core of the dirt explosion
 * @param[in] org
 * @param[in] vel
 * @param[in] duration
 * @param[in] width
 * @param[in] height
 * @param[in] alpha
 * @param[in] shader
 */
void CG_ParticleDirtBulletDebris_Core(vec3_t org, vec3_t vel, int duration, float width, float height, float alpha, qhandle_t shader)
{
	cparticle_t *p;

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;

	p->time      = cg.time;
	p->endtime   = cg.time + duration;
	p->startfade = cg.time + duration / 2;

	p->color    = EMISIVEFADE;
	p->alpha    = alpha;
	p->alphavel = 0;

	p->height    = width;
	p->width     = height;
	p->endheight = p->height;
	p->endwidth  = p->width;

	p->rotate = qfalse;

	p->type = P_SMOKE;

	p->pshader = shader;

	VectorCopy(org, p->org);
	VectorCopy(vel, p->vel);
	VectorSet(p->accel, 0, 0, -330);
}

/**
 * @brief CG_ParticleExplosion
 * @param[in] animStr
 * @param[in] origin
 * @param[in] vel
 * @param[in] duration
 * @param[in] sizeStart
 * @param[in] sizeEnd
 * @param[in] dlight
 */
void CG_ParticleExplosion(const char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd, qboolean dlight)
{
	cparticle_t *p;
	int         anim;

	// find the animation string
	for (anim = 0; anim < MAX_SHADER_ANIMS; anim++)
	{
		if (!Q_stricmp(animStr, shaderAnims[anim].names))
		{
			break;
		}
	}
	if (anim == MAX_SHADER_ANIMS)
	{
		CG_Error("CG_ParticleExplosion: unknown animation string: %s\n", animStr);
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->alpha         = 1.0f;
	p->alphavel      = 0;

	if (duration < 0)
	{
		duration *= -1;
		p->roll   = 0;
	}
	else
	{
		p->roll = (int)(crandom() * 179);
	}

	p->shaderAnim = anim;

	p->width  = sizeStart;
	p->height = sizeStart * shaderAnims[anim].STRatio;      // for sprites that are stretch in either direction

	p->endheight = sizeEnd;
	p->endwidth  = sizeEnd * shaderAnims[anim].STRatio;

	p->endtime   = cg.time + duration;
	p->startfade = cg.time;

	if (dlight)
	{
		p->type = P_DLIGHT_ANIM;
	}
	else
	{
		p->type = P_ANIM;
	}

	VectorCopy(origin, p->org);
	VectorCopy(vel, p->vel);
	VectorClear(p->accel);
}

/*
 * @brief CG_NewParticleArea
 * @param[in] num
 * @return
 *
 * @note Unused
 *
int CG_NewParticleArea(int num)
{
    char   *str;
    char   *token;
    int    type;
    vec3_t origin, origin2;
    int    i;
    float  range;
    int    turb;
    int    numparticles;
    int    snum;

    str = (char *) CG_ConfigString(num);
    if (!str[0])
    {
        return (0);
    }

    // returns type 128 64 or 32
    token = COM_Parse(&str);
    type  = Q_atoi(token);

    switch (type)
    {
    case 0:
        range = 256;
        break;
    case 1:
        range = 128;
        break;
    case 2:
    case 7:
        range = 64;
        break;
    case 3:
    case 6:
        range = 32;
        break;
    case 4:
        range = 8;
        break;
    case 5:
        range = 16;
        break;
    default:
        range = 0;
        break;
    }

    for (i = 0; i < 3; i++)
    {
        token     = COM_Parse(&str);
        origin[i] = (float)atof(token);
    }

    for (i = 0; i < 3; i++)
    {
        token      = COM_Parse(&str);
        origin2[i] = (float)atof(token);
    }

    token        = COM_Parse(&str);
    numparticles = Q_atoi(token);

    token = COM_Parse(&str);
    turb  = Q_atoi(token);

    token = COM_Parse(&str);
    snum  = Q_atoi(token);

    for (i = 0; i < numparticles; i++)
    {
        if (type >= 4)
        {
            CG_ParticleBubble(cgs.media.waterBubbleShader, origin, origin2, turb, range, snum);
        }
        else
        {
            CG_ParticleSnow(cgs.media.snowShader, origin, origin2, turb, range, snum);
        }
    }

    return 1;
}
*/

/**
 * @brief CG_ParticleImpactSmokePuffExtended
 * @param[in] pshader
 * @param[in] origin
 * @param[in] lifetime
 * @param[in] vel
 * @param[in] acc
 * @param[in] maxroll
 * @param[in] alpha
 * @param[in] size
 */
void CG_ParticleImpactSmokePuffExtended(qhandle_t pshader, vec3_t origin, int lifetime, int vel, int acc, int maxroll, float alpha, float size)
{
	cparticle_t *p;

	if (!pshader)
	{
		CG_Printf("CG_ParticleImpactSmokePuffExtended pshader == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->alpha         = alpha;
	p->alphavel      = 0;

	// roll either direction
	p->roll  = rand() % (2 * maxroll);
	p->roll -= maxroll;

	p->pshader = pshader;

	p->endtime   = cg.time + lifetime;
	p->startfade = cg.time + 100;

	// changed calculation to prevent division by 0 for small size
	p->width  = size * (1.0f + random() * 0.5f);  // rand()%(int)(size * .5f) + size;
	p->height = size * (1.0f + random() * 0.5f);   // rand()%(int)(size * .5f) + size;

	p->endheight = p->height * 2;
	p->endwidth  = p->width * 2;

	p->type = P_SMOKE_IMPACT;

	VectorCopy(origin, p->org);
	VectorSet(p->vel, 0, 0, vel);
	VectorSet(p->accel, 0, 0, acc);

	p->rotate = qtrue;
}

/**
 * @brief CG_ParticleImpactSmokePuff
 * @param[in] pshader
 * @param[in] origin
 */
void CG_ParticleImpactSmokePuff(qhandle_t pshader, vec3_t origin)
{
	CG_ParticleImpactSmokePuffExtended(pshader, origin, 500, 20, 20, 30, 0.25f, 8.f);
}

#ifdef BLOOD_PARTICLE_TRAIL
/**
 * @brief CG_Particle_Bleed
 * @param[in] pshader
 * @param[in] start
 * @param dir - unused
 * @param[in] fleshEntityNum
 * @param[in] duration
 */
void CG_Particle_Bleed(qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration)
{
	cparticle_t *p;

	if (!pshader)
	{
		CG_Printf("CG_Particle_Bleed pshader == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->alpha         = 1.0f;
	p->alphavel      = 0;
	p->roll          = 0;

	p->pshader = pshader;

	p->endtime = cg.time + duration;

	if (fleshEntityNum)
	{
		p->startfade = cg.time;
	}
	else
	{
		p->startfade = cg.time + 100;
	}

	p->width  = 4;
	p->height = 4;

	p->endheight = 4 + rand() % 3;
	p->endwidth  = p->endheight;

	p->type = P_SMOKE;

	VectorCopy(start, p->org);
	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = -20;
	VectorClear(p->accel);

	p->rotate = qfalse;

	p->roll = rand() % 179;

	if (fleshEntityNum)
	{
		p->color = MUSTARD;
	}
	else
	{
		p->color = BLOODRED;
	}
	p->alpha = 0.75f;
}
#endif

/**
 * @brief CG_Particle_OilParticle
 * @param[in] pshader
 * @param[in] origin
 * @param[in] dir
 * @param[in] ptime
 * @param[in] snum
 */
void CG_Particle_OilParticle(qhandle_t pshader, vec3_t origin, vec3_t dir, int ptime, int snum)      // snum is parent ent number?
{
	cparticle_t *p;
	int         time  = cg.time;
	int         time2 = cg.time + ptime;
	float       ratio = (float)1 - ((float)time / (float)time2);

	if (!pshader)
	{
		CG_Printf("CG_Particle_OilParticle == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;
	p->alphavel      = 0;
	p->roll          = 0;

	p->pshader = pshader;

	p->endtime = cg.time + 2000;

	p->startfade = p->endtime;

	p->width  = 2;
	p->height = 2;

	p->endwidth  = 1;
	p->endheight = 1;

	p->type = P_SMOKE;

	VectorCopy(origin, p->org);

	p->vel[0] = (dir[0] * (16 * ratio));
	p->vel[1] = (dir[1] * (16 * ratio));
	p->vel[2] = (dir[2] * (16 * ratio));

	p->snum = snum;

	VectorClear(p->accel);

	p->accel[2] = -20;

	p->rotate = qfalse;

	p->roll = rand() % 179;

	p->alpha = 0.5f;

	p->color = BLOODRED;
}

/**
 * @brief CG_Particle_OilSlick
 * @param[in] pshader
 * @param[in] cent
 */
void CG_Particle_OilSlick(qhandle_t pshader, centity_t *cent)
{
	cparticle_t *p;

	if (!pshader)
	{
		CG_Printf("CG_Particle_OilSlick == ZERO!\n");
	}

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;

	if (cent->currentState.angles2[2] != 0.f)
	{
		p->endtime = cg.time + cent->currentState.angles2[2];
	}
	else
	{
		p->endtime = cg.time + 60000;
	}

	p->startfade = p->endtime;

	p->alpha    = 1.0f;
	p->alphavel = 0;
	p->roll     = 0;

	p->pshader = pshader;

	if (cent->currentState.angles2[0] != 0.f || cent->currentState.angles2[1] != 0.f)
	{
		p->width  = cent->currentState.angles2[0];
		p->height = cent->currentState.angles2[0];

		p->endheight = cent->currentState.angles2[1];
		p->endwidth  = cent->currentState.angles2[1];
	}
	else
	{
		p->width  = 8;
		p->height = 8;

		p->endheight = 16;
		p->endwidth  = 16;
	}

	p->type = P_FLAT_SCALEUP;

	p->snum = cent->currentState.density;

	VectorCopy(cent->currentState.origin, p->org);

	p->org[2] += 0.55f + (crandom() * 0.5f);

	p->vel[0] = 0;
	p->vel[1] = 0;
	p->vel[2] = 0;
	VectorClear(p->accel);

	p->rotate = qfalse;

	p->roll = rand() % 179;

	p->alpha = 0.75f;
}

/**
 * @brief CG_OilSlickRemove
 * @param[in] cent
 */
void CG_OilSlickRemove(centity_t *cent)
{
	cparticle_t *p, *next;
	int         id = cent->currentState.density;

	if (!id)
	{
		CG_Printf("CG_OilSlickRemove NULL id\n");
	}

	for (p = active_particles ; p ; p = next)
	{
		next = p->next;

		if (p->type == P_FLAT_SCALEUP)
		{
			if (p->snum == id)
			{
				p->endtime   = cg.time + 100;
				p->startfade = p->endtime;
				p->type      = P_FLAT_SCALEUP_FADE;
			}
		}

	}
}

#define NORMALSIZE  16
#define LARGESIZE   32

/**
 * @brief CG_ParticleBloodCloud
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 */
void CG_ParticleBloodCloud(centity_t *cent, vec3_t origin, vec3_t dir)
{
	float       length;
	float       dist = 0;
	float       crittersize;
	vec3_t      angles, forward;
	vec3_t      point;
	cparticle_t *p;
	int         i;

	length = VectorLength(dir);
	vectoangles(dir, angles);
	AngleVectors(angles, forward, NULL, NULL);

	if (cent->currentState.density == 0)     // normal ai size
	{
		crittersize = NORMALSIZE;
	}
	else
	{
		crittersize = LARGESIZE;
	}

	if (length != 0.f)
	{
		dist = length / crittersize;
	}

	if (dist < 1)
	{
		dist = 1;
	}

	VectorCopy(origin, point);

	for (i = 0; i < dist; i++)
	{
		VectorMA(point, crittersize, forward, point);

		if (!free_particles)
		{
			return;
		}

		p                = free_particles;
		free_particles   = p->next;
		p->next          = active_particles;
		active_particles = p;

		p->time     = cg.time;
		p->alpha    = 1.0;
		p->alphavel = 0;
		p->roll     = 0;

		p->pshader = cgs.media.smokePuffShader;

		p->endtime = cg.time + 350 + (crandom() * 100);

		p->startfade = cg.time;

		if (cent->currentState.density == 0)     // normal ai size
		{
			p->width  = NORMALSIZE;
			p->height = NORMALSIZE;

			p->endheight = NORMALSIZE;
			p->endwidth  = NORMALSIZE;
		}
		else   // large frame
		{
			p->width  = LARGESIZE;
			p->height = LARGESIZE;

			p->endheight = LARGESIZE;
			p->endwidth  = LARGESIZE;
		}

		p->type = P_SMOKE;

		VectorCopy(origin, p->org);

		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = -1;

		VectorClear(p->accel);

		p->rotate = qfalse;
		p->roll   = rand() % 179;
		p->color  = BLOODRED;
		p->alpha  = 0.75;
	}
}

/**
 * @brief CG_ParticleSparks
 * @param[in] org
 * @param[in] vel
 * @param[in] duration
 * @param[in] x
 * @param[in] y
 * @param[in] speed
 */
void CG_ParticleSparks(vec3_t org, vec3_t vel, int duration, float x, float y, float speed)
{
	cparticle_t *p;

	if (!free_particles)
	{
		return;
	}

	p                = free_particles;
	free_particles   = p->next;
	p->next          = active_particles;
	active_particles = p;
	p->time          = cg.time;

	p->endtime   = cg.time + duration;
	p->startfade = cg.time + duration / 2;

	p->color    = EMISIVEFADE;
	p->alpha    = 0.4f;
	p->alphavel = 0;

	p->height    = 0.5f;
	p->width     = 0.5f;
	p->endheight = 0.5f;
	p->endwidth  = 0.5f;

	p->pshader = cgs.media.tracerShader;

	p->type = P_SMOKE;

	VectorCopy(org, p->org);

	p->org[0] += (crandom() * x);
	p->org[1] += (crandom() * y);

	p->vel[0] = vel[0];
	p->vel[1] = vel[1];
	p->vel[2] = vel[2];

	p->accel[0] = p->accel[1] = p->accel[2] = 0;

	p->vel[0] += (crandom() * 4);
	p->vel[1] += (crandom() * 4);
	p->vel[2] += (20 + (crandom() * 10)) * speed;

	p->accel[0] = crandom() * 4;
	p->accel[1] = crandom() * 4;
}

#define PARTICLE_GRAVITY 16

/**
 * @brief CG_ParticleDust
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 */
void CG_ParticleDust(centity_t *cent, vec3_t origin, vec3_t dir)
{
	float       length;
	float       dist = 0;
	float       crittersize;
	vec3_t      angles, forward;
	vec3_t      point;
	cparticle_t *p;
	int         i;

	VectorNegate(dir, dir);
	length = VectorLength(dir);
	vectoangles(dir, angles);
	AngleVectors(angles, forward, NULL, NULL);

	if (cent->currentState.density == 0)     // normal ai size
	{
		crittersize = NORMALSIZE;
	}
	else
	{
		crittersize = LARGESIZE;
	}

	if (length != 0.f)
	{
		dist = length / crittersize;
	}

	if (dist < 1)
	{
		dist = 1;
	}

	VectorCopy(origin, point);

	for (i = 0; i < dist; i++)
	{
		VectorMA(point, crittersize, forward, point);

		if (!free_particles)
		{
			return;
		}

		p                = free_particles;
		free_particles   = p->next;
		p->next          = active_particles;
		active_particles = p;

		p->time     = cg.time;
		p->alpha    = 5.0f;
		p->alphavel = 0;
		p->roll     = 0;

		p->pshader = cgs.media.bloodCloudShader;

		// stay around for long enough to expand and dissipate naturally
		if (length != 0.f)
		{
			p->endtime = cg.time + 4500 + (crandom() * 3500);
		}
		else
		{
			p->endtime = cg.time + 750 + (crandom() * 500);
		}

		p->startfade = cg.time;

		if (cent->currentState.density == 0)     // normal ai size
		{
			p->width  = NORMALSIZE;
			p->height = NORMALSIZE;

			// expand while falling
			p->endheight = NORMALSIZE * 4.0;
			p->endwidth  = NORMALSIZE * 4.0;
		}
		else   // large frame
		{
			p->width  = LARGESIZE;
			p->height = LARGESIZE;

			// expand while falling
			p->endheight = LARGESIZE * 3.0;
			p->endwidth  = LARGESIZE * 3.0;
		}

		if (length == 0.f)
		{
			p->width  *= 0.2;
			p->height *= 0.2;

			p->endheight = NORMALSIZE;
			p->endwidth  = NORMALSIZE;
		}

		p->type = P_SMOKE;

		VectorCopy(point, p->org);

		p->vel[0] = crandom() * 6;
		p->vel[1] = crandom() * 6;
		p->vel[2] = random() * 20;

		// add some gravity/randomness
		p->accel[0] = crandom() * 3;
		p->accel[1] = crandom() * 3;
		p->accel[2] = -PARTICLE_GRAVITY;

		VectorClear(p->accel);

		p->rotate = qfalse;

		p->roll = rand() % 179;

		if (cent->currentState.density)
		{
			p->color = GREY75;
		}
		else
		{
			p->color = MUSTARD;
		}

		p->alpha = 0.75f;
	}
}
