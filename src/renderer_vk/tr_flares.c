/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 * @file renderer/tr_flares.c
 * @brief LIGHT FLARES
 *
 * A light flare is an effect that takes place inside the eye when bright light
 * sources are visible.  The size of the flare reletive to the screen is nearly
 * constant, irrespective of distance, but the intensity should be proportional
 * to the projected area of the light source.
 *
 * A surface that has been flagged as having a light flare will calculate the
 * depth buffer value that it's midpoint should have when the surface is added.
 *
 * After all opaque surfaces have been rendered, the depth buffer is read back
 * for each flare in view.  If the point has not been obscured by a closer
 * surface, the flare should be drawn.
 *
 * Surfaces that have a repeated texture should never be flagged as flaring,
 * because there will only be a single flare added at the midpoint of the
 * polygon.
 *
 * To prevent abrupt popping, the intensity of the flare is interpolated up and
 * down as it changes visibility.  This involves scene to scene state, unlike
 * almost all other aspects of the renderer, and is complicated by the fact that
 * a single frame may have multiple scenes.
 *
 * RB_RenderFlares() will be called once per view (twice in a mirrored scene,
 * potentially up to five or more times in a frame with 3D status bar icons).
 */

#include "tr_local.h"

/**
 * @struct flare_s
 * @typedef flare_t
 * @brief Flare states maintain visibility over multiple frames for fading layers: view, mirror, menu
 */
typedef struct flare_s
{
	struct flare_s *next;      ///< for active chain

	int addedFrame;

	qboolean inPortal;              ///< true if in a portal view of the scene
	int frameSceneNum;
	void *surface;
	int fogNum;

	int fadeTime;

	qboolean cgvisible;             ///< for coronas, the client determines current visibility, but it's still inserted so it will fade out properly
	qboolean visible;               ///< state of last test
	float drawIntensity;            ///< may be non 0 even if !visible due to fading

	int windowX, windowY;
	float eyeZ;

	vec3_t color;
	float scale;

	int id;
} flare_t;

#define MAX_FLARES      128

flare_t r_flareStructs[MAX_FLARES];
flare_t *r_activeFlares, *r_inactiveFlares;

/**
 * @brief R_ClearFlares
 */
void R_ClearFlares(void)
{
	int i;

	Com_Memset(r_flareStructs, 0, sizeof(r_flareStructs));
	r_activeFlares   = NULL;
	r_inactiveFlares = NULL;

	for (i = 0; i < MAX_FLARES; i++)
	{
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares       = &r_flareStructs[i];
	}
}

/**
 * @brief This is called at surface tesselation time
 * @param[in] surface
 * @param[in] fogNum
 * @param[in] point
 * @param[in] color
 * @param[in] scale
 * @param[in] normal
 * @param[in] id
 * @param[in] visible
 */
void RB_AddFlare(void *surface, int fogNum, vec3_t point, vec3_t color, float scale, vec3_t normal, int id, qboolean visible) // added scale. added id.  added visible
{
	int     i;
	flare_t *f;
	vec3_t  local;
	vec4_t  eye, clip, normalized, window;

	backEnd.pc.c_flareAdds++;

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip(point, backEnd.orientation.modelMatrix,
	                       backEnd.viewParms.projectionMatrix, eye, clip);

	//Ren_Print("src:  %f  %f  %f  \n", point[0], point[1], point[2]);
	//Ren_Print("eye:  %f  %f  %f  %f\n", eye[0], eye[1], eye[2], eye[3]);

	// check to see if the point is completely off screen
	for (i = 0; i < 3; i++)
	{
		if (clip[i] >= clip[3] || clip[i] <= -clip[3])
		{
			return;
		}
	}

	R_TransformClipToWindow(clip, &backEnd.viewParms, normalized, window);

	//Ren_Print("window:  %f  %f  %f  \n", window[0], window[1], window[2]);

	if (window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth ||
	    window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight)
	{
		return; // shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	for (f = r_activeFlares ; f ; f = f->next)
	{
		// added back in more checks for different scenes
		if (f->id == id && f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal)
		{
			break;
		}
	}

	// allocate a new one
	if (!f)
	{
		if (!r_inactiveFlares)
		{
			// the list is completely full
			return;
		}
		f                = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next          = r_activeFlares;
		r_activeFlares   = f;

		f->surface       = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal      = backEnd.viewParms.isPortal;
		f->addedFrame    = -1;
		f->id            = id;
	}

	f->cgvisible = visible;

	if (f->addedFrame != backEnd.viewParms.frameCount - 1)
	{
		f->visible  = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum     = fogNum;

	VectorCopy(color, f->color);

	f->scale = scale;

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	if (normal)
	{
		float d;

		VectorSubtract(backEnd.viewParms.orientation.origin, point, local);
		VectorNormalizeFast(local);
		d = DotProduct(local, normal);
		VectorScale(f->color, d, f->color);
	}

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];

	f->eyeZ = eye[2];
}

/**
 * @brief RB_AddDlightFlares
 */
void RB_AddDlightFlares(void)
{
	dlight_t *l;
	int      i, j, k;
	int      id = 0;
	fog_t    *fog;

	if (r_flares->integer < 2)
	{
		return;
	}

	l = backEnd.refdef.dlights;

	for (i = 0 ; i < backEnd.refdef.num_dlights ; i++, l++)
	{
		// find which fog volume the light is in
		for (j = 1 ; j < tr.world->numfogs ; j++)
		{
			fog = &tr.world->fogs[j];
			for (k = 0 ; k < 3 ; k++)
			{
				if (l->origin[k] < fog->bounds[0][k] || l->origin[k] > fog->bounds[1][k])
				{
					break;
				}
			}
			if (k == 3)
			{
				break;
			}
		}
		if (j == tr.world->numfogs)
		{
			j = 0;
		}

		RB_AddFlare((void *)l, j, l->origin, l->color, 1.0f, NULL, id++, qtrue); // also set scale
	}
}

/**
 * @brief RB_AddCoronaFlares
 */
void RB_AddCoronaFlares(void)
{
	corona_t *cor;
	int      i, j, k;
	fog_t    *fog;

	if (r_flares->integer != 1 && r_flares->integer != 3)
	{
		return;
	}

	if (!(tr.world)) // possible currently at the player model selection menu
	{
		return;
	}

	cor = backEnd.refdef.coronas;

	for (i = 0 ; i < backEnd.refdef.num_coronas ; i++, cor++)
	{
		// find which fog volume the corona is in
		for (j = 1 ; j < tr.world->numfogs ; j++)
		{
			fog = &tr.world->fogs[j];
			for (k = 0 ; k < 3 ; k++)
			{
				if (cor->origin[k] < fog->bounds[0][k] || cor->origin[k] > fog->bounds[1][k])
				{
					break;
				}
			}
			if (k == 3)
			{
				break;
			}
		}
		if (j == tr.world->numfogs)
		{
			j = 0;
		}
		RB_AddFlare((void *)cor, j, cor->origin, cor->color, cor->scale, NULL, cor->id, cor->visible);
	}
}

/*
===============================================================================
FLARE BACK END
===============================================================================
*/

/**
 * @brief RB_TestFlare
 * @param[in,out] f
 */
void RB_TestFlare(flare_t *f)
{
	//float           depth;
	qboolean visible;
	float    fade;
	//float           screenZ;

	backEnd.pc.c_flareTests++;

	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	//glState.finishCalled = qfalse;
	//glState.finishCalled = qtrue;   // (SA) Hmm, shouldn't this be true?

	// read back the z buffer contents
	//glReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );
	//screenZ = backEnd.viewParms.projectionMatrix[14] /
	//  ( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

	// 24 was way to low tolerance.  It gave Dan problems with free standing light fixtures
	// I will monitor to see if changing this screws up any other situations
	// and 2 was way to high tolerance
	//visible = ( -f->eyeZ - -screenZ ) < 2;
	//visible = ( -f->eyeZ - -screenZ ) < 24;
	//visible = ( -f->eyeZ - -screenZ ) < 6;
	//visible = qtrue;
	visible = f->cgvisible;

	if (visible)
	{
		if (!f->visible)
		{
			f->visible  = qtrue;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;
	}
	else
	{
		if (f->visible)
		{
			f->visible  = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;
	}

	if (fade < 0)
	{
		fade = 0;
	}
	if (fade > 1)
	{
		fade = 1;
	}

	f->drawIntensity = fade;
}

/**
 * @brief RB_RenderFlare
 * @param[in] f
 */
void RB_RenderFlare(flare_t *f)
{
	float  size;
	vec3_t color;
	int    iColor[3];

	backEnd.pc.c_flareRenders++;

	// changed to use alpha blend rather than additive blend
	// this is to accomidate the fact we can't right now do
	// additive blends and have them fog correctly with our distance fog.
	//      /when/ we fix the blend problems with distance fog, this should
	//      be changed back to additive since there's nearly no hit for that
	//      but the alpha blend is noticably slower.

	VectorScale(f->color, tr.identityLight, color);         // mod for alpha blend rather than additive

	iColor[0] = (byte)(color[0] * 255);
	iColor[1] = (byte)(color[1] * 255);
	iColor[2] = (byte)(color[2] * 255);

	size = backEnd.viewParms.viewportWidth * ((r_flareSize->value * f->scale) / 640.0f + 8 / -f->eyeZ);

	RB_BeginSurface(tr.flareShader, f->fogNum);

	// FIXME: use quadstamp?
	tess.xyz[tess.numVertexes][0]          = f->windowX - size;
	tess.xyz[tess.numVertexes][1]          = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = (byte)(iColor[0]);
	tess.vertexColors[tess.numVertexes][1] = (byte)(iColor[1]);
	tess.vertexColors[tess.numVertexes][2] = (byte)(iColor[2]);
	tess.vertexColors[tess.numVertexes][3] = (byte)(f->drawIntensity * 255); // mod for alpha blend rather than additive
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]          = f->windowX - size;
	tess.xyz[tess.numVertexes][1]          = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = (byte)(iColor[0]);
	tess.vertexColors[tess.numVertexes][1] = (byte)(iColor[1]);
	tess.vertexColors[tess.numVertexes][2] = (byte)(iColor[2]);
	tess.vertexColors[tess.numVertexes][3] = (byte)(f->drawIntensity * 255); // mod for alpha blend rather than additive
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]          = f->windowX + size;
	tess.xyz[tess.numVertexes][1]          = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 1;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = (byte)(iColor[0]);
	tess.vertexColors[tess.numVertexes][1] = (byte)(iColor[1]);
	tess.vertexColors[tess.numVertexes][2] = (byte)(iColor[2]);
	tess.vertexColors[tess.numVertexes][3] = (byte)(f->drawIntensity * 255); // mod for alpha blend rather than additive
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0]          = f->windowX + size;
	tess.xyz[tess.numVertexes][1]          = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 1;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = (byte)(iColor[0]);
	tess.vertexColors[tess.numVertexes][1] = (byte)(iColor[1]);
	tess.vertexColors[tess.numVertexes][2] = (byte)(iColor[2]);
	tess.vertexColors[tess.numVertexes][3] = (byte)(f->drawIntensity * 255); // mod for alpha blend rather than additive
	//tess.vertexColors[tess.numVertexes][3] = 255; // mod for alpha blend rather than additive
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_EndSurface();
}

/**
 * @brief Because flares are simulating an occular effect, they should be drawn after
 * everything (all views) in the entire frame has been drawn.
 *
 * Because of the way portals use the depth buffer to mark off areas, the
 * needed information would be lost after each view, so we are forced to draw
 * flares after each view.
 *
 * The resulting artifact is that flares in mirrors or portals don't dim properly
 * when occluded by something in the main view, and portal flares that should
 * extend past the portal edge will be overwritten.
 */
void RB_RenderFlares(void)
{
	flare_t  *f;
	flare_t  **prev;
	qboolean draw;

	if (!r_flares->integer)
	{
		return;
	}

	// turned light flares back on.  must evaluate problem id had with this
	RB_AddDlightFlares();
	RB_AddCoronaFlares();

	// perform z buffer readback on each flare in this view
	draw = qfalse;
	prev = &r_activeFlares;
	while ((f = *prev) != NULL)
	{
		// throw out any flares that weren't added last frame
		if (f->addedFrame < backEnd.viewParms.frameCount - 1)
		{
			*prev            = f->next;
			f->next          = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;
		if (f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal)
		{
			RB_TestFlare(f);
			if (f->drawIntensity != 0.f)
			{
				draw = qtrue;
			}
			else
			{
				// this flare has completely faded out, so remove it from the chain
				*prev            = f->next;
				f->next          = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		prev = &f->next;
	}

	if (!draw)
	{
		// none visible
		return;
	}

	if (backEnd.viewParms.isPortal)
	{
		glDisable(GL_CLIP_PLANE0);
	}

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(backEnd.viewParms.viewportX, backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	        backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	        -99999, 99999);

	for (f = r_activeFlares ; f ; f = f->next)
	{
		if (f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal && f->drawIntensity != 0.f)
		{
			RB_RenderFlare(f);
		}
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
