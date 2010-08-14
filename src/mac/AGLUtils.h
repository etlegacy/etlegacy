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

#ifndef aglutils_h
#define aglutils_h

#define NEW_CODE 1

#include <OpenGL/gl.h>
#include <AGL/agl.h>

#define TEST_SMP    1
#define TEST_FSAA   1

extern AGLPixelFormat gPixelFormat;
extern AGLContext gAGLContext;
extern Fixed aglFrequency;

OSStatus MySetWindowContentColor( WindowRef inWindow, RGBColor const *inRGB );
GLboolean _aglSetGameContext( short inWidth, short inHeight, short inDepth, short inTexDepth, short inStencilDepth,
							  Fixed inFreq, DisplayIDType inDevice, Boolean inWindow, int inFSAASamples );
void _aglDisposeGameContext( void );
GLboolean _aglSuspendGameContext( void );
GLboolean _aglResumeGameContext( void );
void _aglSwapBuffers( void );
WindowRef _aglGetGLWindow( void );
Boolean _aglUsingFullscreen( void );
AGLDrawable _aglGetDrawable( void );
CGrafPtr _aglGetCurrentPort( void );

void _aglUsePrimaryContext( void );
void _aglUseSecondaryContext( void );

#endif
