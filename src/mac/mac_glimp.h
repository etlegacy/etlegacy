/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef mac_glimp_h
#define mac_glimp_h

#include <DrawSprocket/DrawSprocket.h>
#include <AGL/agl.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

// #include "ATIGL.h"

#ifndef  GL_MULTISAMPLE_FILTER_HINT_NV
#define GL_MULTISAMPLE_FILTER_HINT_NV                    0x8534
#endif

typedef struct {
	CGGammaValue red[256];
	CGGammaValue green[256];
	CGGammaValue blue[256];
} CGGammaTable;

#define MAX_DEVICES 32

typedef struct {
	GDHandle devices[MAX_DEVICES];
	int numDevices;

	GDHandle device;
	DisplayIDType displayID;

	AGLContext context;
	AGLContext secondaryContext;
	AGLPixelFormat fmt;
	//	WindowRef		window;

	DSpContextReference DSpContext;
	CGGammaTable gameGamma;
	Ptr systemGammas;

	GLint textureMemory;
	GLint videoMemory;

	FILE            *log_fp;
} aglstate_t;

extern aglstate_t sys_gl;

#ifdef __cplusplus
extern "C" {
#endif
extern Boolean QGL_Init( void );
#ifdef __cplusplus
}
#endif

#endif
