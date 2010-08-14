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

/*
** MAC_QGL.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake3 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#include <float.h>
#include "../client/client.h"
#include "../renderer/tr_local.h"
#include "mac_local.h" // For VID_Printf
#include "mac_glimp.h"

void QGL_EnableLogging( qboolean enable );

#if MAC_QGL_LINKED
void ( APIENTRY * qglAccum )( GLenum op, GLfloat value ) = NULL;
void ( APIENTRY * qglAlphaFunc )( GLenum func, GLclampf ref ) = NULL;
GLboolean ( APIENTRY * qglAreTexturesResident )( GLsizei n, const GLuint *textures, GLboolean *residences ) = NULL;
void ( APIENTRY * qglArrayElement )( GLint i ) = NULL;
void ( APIENTRY * qglBegin )( GLenum mode ) = NULL;
void ( APIENTRY * qglBindTexture )( GLenum target, GLuint texture ) = NULL;
void ( APIENTRY * qglBitmap )( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) = NULL;
void ( APIENTRY * qglBlendFunc )( GLenum sfactor, GLenum dfactor ) = NULL;
void ( APIENTRY * qglCallList )( GLuint list ) = NULL;
void ( APIENTRY * qglCallLists )( GLsizei n, GLenum type, const GLvoid *lists ) = NULL;
void ( APIENTRY * qglClear )( GLbitfield mask ) = NULL;
void ( APIENTRY * qglClearAccum )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) = NULL;
void ( APIENTRY * qglClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) = NULL;
void ( APIENTRY * qglClearDepth )( GLclampd depth ) = NULL;
void ( APIENTRY * qglClearIndex )( GLfloat c ) = NULL;
void ( APIENTRY * qglClearStencil )( GLint s ) = NULL;
void ( APIENTRY * qglClipPlane )( GLenum plane, const GLdouble *equation ) = NULL;
void ( APIENTRY * qglColor3b )( GLbyte red, GLbyte green, GLbyte blue ) = NULL;
void ( APIENTRY * qglColor3bv )( const GLbyte *v ) = NULL;
void ( APIENTRY * qglColor3d )( GLdouble red, GLdouble green, GLdouble blue ) = NULL;
void ( APIENTRY * qglColor3dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglColor3f )( GLfloat red, GLfloat green, GLfloat blue ) = NULL;
void ( APIENTRY * qglColor3fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglColor3i )( GLint red, GLint green, GLint blue ) = NULL;
void ( APIENTRY * qglColor3iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglColor3s )( GLshort red, GLshort green, GLshort blue ) = NULL;
void ( APIENTRY * qglColor3sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglColor3ub )( GLubyte red, GLubyte green, GLubyte blue ) = NULL;
void ( APIENTRY * qglColor3ubv )( const GLubyte *v ) = NULL;
void ( APIENTRY * qglColor3ui )( GLuint red, GLuint green, GLuint blue ) = NULL;
void ( APIENTRY * qglColor3uiv )( const GLuint *v ) = NULL;
void ( APIENTRY * qglColor3us )( GLushort red, GLushort green, GLushort blue ) = NULL;
void ( APIENTRY * qglColor3usv )( const GLushort *v ) = NULL;
void ( APIENTRY * qglColor4b )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) = NULL;
void ( APIENTRY * qglColor4bv )( const GLbyte *v ) = NULL;
void ( APIENTRY * qglColor4d )( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) = NULL;
void ( APIENTRY * qglColor4dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglColor4f )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) = NULL;
void ( APIENTRY * qglColor4fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglColor4i )( GLint red, GLint green, GLint blue, GLint alpha ) = NULL;
void ( APIENTRY * qglColor4iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglColor4s )( GLshort red, GLshort green, GLshort blue, GLshort alpha ) = NULL;
void ( APIENTRY * qglColor4sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) = NULL;
void ( APIENTRY * qglColor4ubv )( const GLubyte *v ) = NULL;
void ( APIENTRY * qglColor4ui )( GLuint red, GLuint green, GLuint blue, GLuint alpha ) = NULL;
void ( APIENTRY * qglColor4uiv )( const GLuint *v ) = NULL;
void ( APIENTRY * qglColor4us )( GLushort red, GLushort green, GLushort blue, GLushort alpha ) = NULL;
void ( APIENTRY * qglColor4usv )( const GLushort *v ) = NULL;
void ( APIENTRY * qglColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) = NULL;
void ( APIENTRY * qglColorMaterial )( GLenum face, GLenum mode ) = NULL;
void ( APIENTRY * qglColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) = NULL;
void ( APIENTRY * qglCopyPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) = NULL;
void ( APIENTRY * qglCopyTexImage1D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border ) = NULL;
void ( APIENTRY * qglCopyTexImage2D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) = NULL;
void ( APIENTRY * qglCopyTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) = NULL;
void ( APIENTRY * qglCopyTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) = NULL;
void ( APIENTRY * qglCullFace )( GLenum mode ) = NULL;
void ( APIENTRY * qglDeleteLists )( GLuint list, GLsizei range ) = NULL;
void ( APIENTRY * qglDeleteTextures )( GLsizei n, const GLuint *textures ) = NULL;
void ( APIENTRY * qglDepthFunc )( GLenum func ) = NULL;
void ( APIENTRY * qglDepthMask )( GLboolean flag ) = NULL;
void ( APIENTRY * qglDepthRange )( GLclampd zNear, GLclampd zFar ) = NULL;
void ( APIENTRY * qglDisable )( GLenum cap ) = NULL;
void ( APIENTRY * qglDisableClientState )( GLenum array ) = NULL;
void ( APIENTRY * qglDrawArrays )( GLenum mode, GLint first, GLsizei count ) = NULL;
void ( APIENTRY * qglDrawBuffer )( GLenum mode ) = NULL;
void ( APIENTRY * qglDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) = NULL;
void ( APIENTRY * qglDrawPixels )( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) = NULL;
void ( APIENTRY * qglEdgeFlag )( GLboolean flag ) = NULL;
void ( APIENTRY * qglEdgeFlagPointer )( GLsizei stride, const GLboolean *pointer ) = NULL;
void ( APIENTRY * qglEdgeFlagv )( const GLboolean *flag ) = NULL;
void ( APIENTRY * qglEnable )( GLenum cap ) = NULL;
void ( APIENTRY * qglEnableClientState )( GLenum array ) = NULL;
void ( APIENTRY * qglEnd )( void ) = NULL;
void ( APIENTRY * qglEndList )( void ) = NULL;
void ( APIENTRY * qglEvalCoord1d )( GLdouble u ) = NULL;
void ( APIENTRY * qglEvalCoord1dv )( const GLdouble *u ) = NULL;
void ( APIENTRY * qglEvalCoord1f )( GLfloat u ) = NULL;
void ( APIENTRY * qglEvalCoord1fv )( const GLfloat *u ) = NULL;
void ( APIENTRY * qglEvalCoord2d )( GLdouble u, GLdouble v ) = NULL;
void ( APIENTRY * qglEvalCoord2dv )( const GLdouble *u ) = NULL;
void ( APIENTRY * qglEvalCoord2f )( GLfloat u, GLfloat v ) = NULL;
void ( APIENTRY * qglEvalCoord2fv )( const GLfloat *u ) = NULL;
void ( APIENTRY * qglEvalMesh1 )( GLenum mode, GLint i1, GLint i2 ) = NULL;
void ( APIENTRY * qglEvalMesh2 )( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) = NULL;
void ( APIENTRY * qglEvalPoint1 )( GLint i ) = NULL;
void ( APIENTRY * qglEvalPoint2 )( GLint i, GLint j ) = NULL;
void ( APIENTRY * qglFeedbackBuffer )( GLsizei size, GLenum type, GLfloat *buffer ) = NULL;
void ( APIENTRY * qglFinish )( void ) = NULL;
void ( APIENTRY * qglFlush )( void ) = NULL;
void ( APIENTRY * qglFogf )( GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglFogfv )( GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglFogi )( GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglFogiv )( GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglFrontFace )( GLenum mode ) = NULL;
void ( APIENTRY * qglFrustum )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) = NULL;
GLuint ( APIENTRY * qglGenLists )( GLsizei range ) = NULL;
void ( APIENTRY * qglGenTextures )( GLsizei n, GLuint *textures ) = NULL;
void ( APIENTRY * qglGetBooleanv )( GLenum pname, GLboolean *params ) = NULL;
void ( APIENTRY * qglGetClipPlane )( GLenum plane, GLdouble *equation ) = NULL;
void ( APIENTRY * qglGetDoublev )( GLenum pname, GLdouble *params ) = NULL;
GLenum ( APIENTRY * qglGetError )( void ) = NULL;
void ( APIENTRY * qglGetFloatv )( GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetIntegerv )( GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglGetLightfv )( GLenum light, GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetLightiv )( GLenum light, GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglGetMapdv )( GLenum target, GLenum query, GLdouble *v ) = NULL;
void ( APIENTRY * qglGetMapfv )( GLenum target, GLenum query, GLfloat *v ) = NULL;
void ( APIENTRY * qglGetMapiv )( GLenum target, GLenum query, GLint *v ) = NULL;
void ( APIENTRY * qglGetMaterialfv )( GLenum face, GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetMaterialiv )( GLenum face, GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglGetPixelMapfv )( GLenum map, GLfloat *values ) = NULL;
void ( APIENTRY * qglGetPixelMapuiv )( GLenum map, GLuint *values ) = NULL;
void ( APIENTRY * qglGetPixelMapusv )( GLenum map, GLushort *values ) = NULL;
void ( APIENTRY * qglGetPointerv )( GLenum pname, GLvoid* *params ) = NULL;
void ( APIENTRY * qglGetPolygonStipple )( GLubyte *mask ) = NULL;
const GLubyte * ( APIENTRY * qglGetString )(GLenum name) = NULL;
void ( APIENTRY * qglGetTexEnvfv )( GLenum target, GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetTexEnviv )( GLenum target, GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglGetTexGendv )( GLenum coord, GLenum pname, GLdouble *params ) = NULL;
void ( APIENTRY * qglGetTexGenfv )( GLenum coord, GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetTexGeniv )( GLenum coord, GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels ) = NULL;
void ( APIENTRY * qglGetTexLevelParameterfv )( GLenum target, GLint level, GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetTexLevelParameteriv )( GLenum target, GLint level, GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglGetTexParameterfv )( GLenum target, GLenum pname, GLfloat *params ) = NULL;
void ( APIENTRY * qglGetTexParameteriv )( GLenum target, GLenum pname, GLint *params ) = NULL;
void ( APIENTRY * qglHint )( GLenum target, GLenum mode ) = NULL;
void ( APIENTRY * qglIndexMask )( GLuint mask ) = NULL;
void ( APIENTRY * qglIndexPointer )( GLenum type, GLsizei stride, const GLvoid *pointer ) = NULL;
void ( APIENTRY * qglIndexd )( GLdouble c ) = NULL;
void ( APIENTRY * qglIndexdv )( const GLdouble *c ) = NULL;
void ( APIENTRY * qglIndexf )( GLfloat c ) = NULL;
void ( APIENTRY * qglIndexfv )( const GLfloat *c ) = NULL;
void ( APIENTRY * qglIndexi )( GLint c ) = NULL;
void ( APIENTRY * qglIndexiv )( const GLint *c ) = NULL;
void ( APIENTRY * qglIndexs )( GLshort c ) = NULL;
void ( APIENTRY * qglIndexsv )( const GLshort *c ) = NULL;
void ( APIENTRY * qglIndexub )( GLubyte c ) = NULL;
void ( APIENTRY * qglIndexubv )( const GLubyte *c ) = NULL;
void ( APIENTRY * qglInitNames )( void ) = NULL;
void ( APIENTRY * qglInterleavedArrays )( GLenum format, GLsizei stride, const GLvoid *pointer ) = NULL;
GLboolean ( APIENTRY * qglIsEnabled )( GLenum cap ) = NULL;
GLboolean ( APIENTRY * qglIsList )( GLuint list ) = NULL;
GLboolean ( APIENTRY * qglIsTexture )( GLuint texture ) = NULL;
void ( APIENTRY * qglLightModelf )( GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglLightModelfv )( GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglLightModeli )( GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglLightModeliv )( GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglLightf )( GLenum light, GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglLightfv )( GLenum light, GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglLighti )( GLenum light, GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglLightiv )( GLenum light, GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglLineStipple )( GLint factor, GLushort pattern ) = NULL;
void ( APIENTRY * qglLineWidth )( GLfloat width ) = NULL;
void ( APIENTRY * qglListBase )( GLuint base ) = NULL;
void ( APIENTRY * qglLoadIdentity )( void ) = NULL;
void ( APIENTRY * qglLoadMatrixd )( const GLdouble *m ) = NULL;
void ( APIENTRY * qglLoadMatrixf )( const GLfloat *m ) = NULL;
void ( APIENTRY * qglLoadName )( GLuint name ) = NULL;
void ( APIENTRY * qglLogicOp )( GLenum opcode ) = NULL;
void ( APIENTRY * qglMap1d )( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) = NULL;
void ( APIENTRY * qglMap1f )( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) = NULL;
void ( APIENTRY * qglMap2d )( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) = NULL;
void ( APIENTRY * qglMap2f )( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) = NULL;
void ( APIENTRY * qglMapGrid1d )( GLint un, GLdouble u1, GLdouble u2 ) = NULL;
void ( APIENTRY * qglMapGrid1f )( GLint un, GLfloat u1, GLfloat u2 ) = NULL;
void ( APIENTRY * qglMapGrid2d )( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) = NULL;
void ( APIENTRY * qglMapGrid2f )( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) = NULL;
void ( APIENTRY * qglMaterialf )( GLenum face, GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglMaterialfv )( GLenum face, GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglMateriali )( GLenum face, GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglMaterialiv )( GLenum face, GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglMatrixMode )( GLenum mode ) = NULL;
void ( APIENTRY * qglMultMatrixd )( const GLdouble *m ) = NULL;
void ( APIENTRY * qglMultMatrixf )( const GLfloat *m ) = NULL;
void ( APIENTRY * qglNewList )( GLuint list, GLenum mode ) = NULL;
void ( APIENTRY * qglNormal3b )( GLbyte nx, GLbyte ny, GLbyte nz ) = NULL;
void ( APIENTRY * qglNormal3bv )( const GLbyte *v ) = NULL;
void ( APIENTRY * qglNormal3d )( GLdouble nx, GLdouble ny, GLdouble nz ) = NULL;
void ( APIENTRY * qglNormal3dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz ) = NULL;
void ( APIENTRY * qglNormal3fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglNormal3i )( GLint nx, GLint ny, GLint nz ) = NULL;
void ( APIENTRY * qglNormal3iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglNormal3s )( GLshort nx, GLshort ny, GLshort nz ) = NULL;
void ( APIENTRY * qglNormal3sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglNormalPointer )( GLenum type, GLsizei stride, const GLvoid *pointer ) = NULL;
void ( APIENTRY * qglOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) = NULL;
void ( APIENTRY * qglPassThrough )( GLfloat token ) = NULL;
void ( APIENTRY * qglPixelMapfv )( GLenum map, GLsizei mapsize, const GLfloat *values ) = NULL;
void ( APIENTRY * qglPixelMapuiv )( GLenum map, GLsizei mapsize, const GLuint *values ) = NULL;
void ( APIENTRY * qglPixelMapusv )( GLenum map, GLsizei mapsize, const GLushort *values ) = NULL;
void ( APIENTRY * qglPixelStoref )( GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglPixelStorei )( GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglPixelTransferf )( GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglPixelTransferi )( GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglPixelZoom )( GLfloat xfactor, GLfloat yfactor ) = NULL;
void ( APIENTRY * qglPointSize )( GLfloat size ) = NULL;
void ( APIENTRY * qglPolygonMode )( GLenum face, GLenum mode ) = NULL;
void ( APIENTRY * qglPolygonOffset )( GLfloat factor, GLfloat units ) = NULL;
void ( APIENTRY * qglPolygonStipple )( const GLubyte *mask ) = NULL;
void ( APIENTRY * qglPopAttrib )( void ) = NULL;
void ( APIENTRY * qglPopClientAttrib )( void ) = NULL;
void ( APIENTRY * qglPopMatrix )( void ) = NULL;
void ( APIENTRY * qglPopName )( void ) = NULL;
void ( APIENTRY * qglPrioritizeTextures )( GLsizei n, const GLuint *textures, const GLclampf *priorities ) = NULL;
void ( APIENTRY * qglPushAttrib )( GLbitfield mask ) = NULL;
void ( APIENTRY * qglPushClientAttrib )( GLbitfield mask ) = NULL;
void ( APIENTRY * qglPushMatrix )( void ) = NULL;
void ( APIENTRY * qglPushName )( GLuint name ) = NULL;
void ( APIENTRY * qglRasterPos2d )( GLdouble x, GLdouble y ) = NULL;
void ( APIENTRY * qglRasterPos2dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglRasterPos2f )( GLfloat x, GLfloat y ) = NULL;
void ( APIENTRY * qglRasterPos2fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglRasterPos2i )( GLint x, GLint y ) = NULL;
void ( APIENTRY * qglRasterPos2iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglRasterPos2s )( GLshort x, GLshort y ) = NULL;
void ( APIENTRY * qglRasterPos2sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglRasterPos3d )( GLdouble x, GLdouble y, GLdouble z ) = NULL;
void ( APIENTRY * qglRasterPos3dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglRasterPos3f )( GLfloat x, GLfloat y, GLfloat z ) = NULL;
void ( APIENTRY * qglRasterPos3fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglRasterPos3i )( GLint x, GLint y, GLint z ) = NULL;
void ( APIENTRY * qglRasterPos3iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglRasterPos3s )( GLshort x, GLshort y, GLshort z ) = NULL;
void ( APIENTRY * qglRasterPos3sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglRasterPos4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) = NULL;
void ( APIENTRY * qglRasterPos4dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglRasterPos4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) = NULL;
void ( APIENTRY * qglRasterPos4fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglRasterPos4i )( GLint x, GLint y, GLint z, GLint w ) = NULL;
void ( APIENTRY * qglRasterPos4iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglRasterPos4s )( GLshort x, GLshort y, GLshort z, GLshort w ) = NULL;
void ( APIENTRY * qglRasterPos4sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglReadBuffer )( GLenum mode ) = NULL;
void ( APIENTRY * qglReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) = NULL;
void ( APIENTRY * qglRectd )( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) = NULL;
void ( APIENTRY * qglRectdv )( const GLdouble *v1, const GLdouble *v2 ) = NULL;
void ( APIENTRY * qglRectf )( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) = NULL;
void ( APIENTRY * qglRectfv )( const GLfloat *v1, const GLfloat *v2 ) = NULL;
void ( APIENTRY * qglRecti )( GLint x1, GLint y1, GLint x2, GLint y2 ) = NULL;
void ( APIENTRY * qglRectiv )( const GLint *v1, const GLint *v2 ) = NULL;
void ( APIENTRY * qglRects )( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) = NULL;
void ( APIENTRY * qglRectsv )( const GLshort *v1, const GLshort *v2 ) = NULL;
GLint ( APIENTRY * qglRenderMode )( GLenum mode ) = NULL;
void ( APIENTRY * qglRotated )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) = NULL;
void ( APIENTRY * qglRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) = NULL;
void ( APIENTRY * qglScaled )( GLdouble x, GLdouble y, GLdouble z ) = NULL;
void ( APIENTRY * qglScalef )( GLfloat x, GLfloat y, GLfloat z ) = NULL;
void ( APIENTRY * qglScissor )( GLint x, GLint y, GLsizei width, GLsizei height ) = NULL;
void ( APIENTRY * qglSelectBuffer )( GLsizei size, GLuint *buffer ) = NULL;
void ( APIENTRY * qglShadeModel )( GLenum mode ) = NULL;
void ( APIENTRY * qglStencilFunc )( GLenum func, GLint ref, GLuint mask ) = NULL;
void ( APIENTRY * qglStencilMask )( GLuint mask ) = NULL;
void ( APIENTRY * qglStencilOp )( GLenum fail, GLenum zfail, GLenum zpass ) = NULL;
void ( APIENTRY * qglTexCoord1d )( GLdouble s ) = NULL;
void ( APIENTRY * qglTexCoord1dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglTexCoord1f )( GLfloat s ) = NULL;
void ( APIENTRY * qglTexCoord1fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglTexCoord1i )( GLint s ) = NULL;
void ( APIENTRY * qglTexCoord1iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglTexCoord1s )( GLshort s ) = NULL;
void ( APIENTRY * qglTexCoord1sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglTexCoord2d )( GLdouble s, GLdouble t ) = NULL;
void ( APIENTRY * qglTexCoord2dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglTexCoord2f )( GLfloat s, GLfloat t ) = NULL;
void ( APIENTRY * qglTexCoord2fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglTexCoord2i )( GLint s, GLint t ) = NULL;
void ( APIENTRY * qglTexCoord2iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglTexCoord2s )( GLshort s, GLshort t ) = NULL;
void ( APIENTRY * qglTexCoord2sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglTexCoord3d )( GLdouble s, GLdouble t, GLdouble r ) = NULL;
void ( APIENTRY * qglTexCoord3dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglTexCoord3f )( GLfloat s, GLfloat t, GLfloat r ) = NULL;
void ( APIENTRY * qglTexCoord3fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglTexCoord3i )( GLint s, GLint t, GLint r ) = NULL;
void ( APIENTRY * qglTexCoord3iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglTexCoord3s )( GLshort s, GLshort t, GLshort r ) = NULL;
void ( APIENTRY * qglTexCoord3sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglTexCoord4d )( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) = NULL;
void ( APIENTRY * qglTexCoord4dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglTexCoord4f )( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) = NULL;
void ( APIENTRY * qglTexCoord4fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglTexCoord4i )( GLint s, GLint t, GLint r, GLint q ) = NULL;
void ( APIENTRY * qglTexCoord4iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglTexCoord4s )( GLshort s, GLshort t, GLshort r, GLshort q ) = NULL;
void ( APIENTRY * qglTexCoord4sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) = NULL;
void ( APIENTRY * qglTexEnvf )( GLenum target, GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglTexEnvfv )( GLenum target, GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglTexEnvi )( GLenum target, GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglTexEnviv )( GLenum target, GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglTexGend )( GLenum coord, GLenum pname, GLdouble param ) = NULL;
void ( APIENTRY * qglTexGendv )( GLenum coord, GLenum pname, const GLdouble *params ) = NULL;
void ( APIENTRY * qglTexGenf )( GLenum coord, GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglTexGeni )( GLenum coord, GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglTexGeniv )( GLenum coord, GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglTexImage1D )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) = NULL;
void ( APIENTRY * qglTexImage2D )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) = NULL;
void ( APIENTRY * qglTexParameterf )( GLenum target, GLenum pname, GLfloat param ) = NULL;
void ( APIENTRY * qglTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params ) = NULL;
void ( APIENTRY * qglTexParameteri )( GLenum target, GLenum pname, GLint param ) = NULL;
void ( APIENTRY * qglTexParameteriv )( GLenum target, GLenum pname, const GLint *params ) = NULL;
void ( APIENTRY * qglTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels ) = NULL;
void ( APIENTRY * qglTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) = NULL;
void ( APIENTRY * qglTranslated )( GLdouble x, GLdouble y, GLdouble z ) = NULL;
void ( APIENTRY * qglTranslatef )( GLfloat x, GLfloat y, GLfloat z ) = NULL;
void ( APIENTRY * qglVertex2d )( GLdouble x, GLdouble y ) = NULL;
void ( APIENTRY * qglVertex2dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglVertex2f )( GLfloat x, GLfloat y ) = NULL;
void ( APIENTRY * qglVertex2fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglVertex2i )( GLint x, GLint y ) = NULL;
void ( APIENTRY * qglVertex2iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglVertex2s )( GLshort x, GLshort y ) = NULL;
void ( APIENTRY * qglVertex2sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglVertex3d )( GLdouble x, GLdouble y, GLdouble z ) = NULL;
void ( APIENTRY * qglVertex3dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglVertex3f )( GLfloat x, GLfloat y, GLfloat z ) = NULL;
void ( APIENTRY * qglVertex3fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglVertex3i )( GLint x, GLint y, GLint z ) = NULL;
void ( APIENTRY * qglVertex3iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglVertex3s )( GLshort x, GLshort y, GLshort z ) = NULL;
void ( APIENTRY * qglVertex3sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglVertex4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) = NULL;
void ( APIENTRY * qglVertex4dv )( const GLdouble *v ) = NULL;
void ( APIENTRY * qglVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) = NULL;
void ( APIENTRY * qglVertex4fv )( const GLfloat *v ) = NULL;
void ( APIENTRY * qglVertex4i )( GLint x, GLint y, GLint z, GLint w ) = NULL;
void ( APIENTRY * qglVertex4iv )( const GLint *v ) = NULL;
void ( APIENTRY * qglVertex4s )( GLshort x, GLshort y, GLshort z, GLshort w ) = NULL;
void ( APIENTRY * qglVertex4sv )( const GLshort *v ) = NULL;
void ( APIENTRY * qglVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) = NULL;
void ( APIENTRY * qglViewport )( GLint x, GLint y, GLsizei width, GLsizei height ) = NULL;


static const char * BooleanToString( GLboolean b ) {
	if ( b == GL_FALSE ) {
		return "GL_FALSE";
	} else if ( b == GL_TRUE ) {
		return "GL_TRUE";
	} else {
		return "OUT OF RANGE FOR BOOLEAN";
	}
}

static const char * FuncToString( GLenum f ) {
	switch ( f )
	{
	case GL_ALWAYS:
		return "GL_ALWAYS";
	case GL_NEVER:
		return "GL_NEVER";
	case GL_LEQUAL:
		return "GL_LEQUAL";
	case GL_LESS:
		return "GL_LESS";
	case GL_EQUAL:
		return "GL_EQUAL";
	case GL_GREATER:
		return "GL_GREATER";
	case GL_GEQUAL:
		return "GL_GEQUAL";
	case GL_NOTEQUAL:
		return "GL_NOTEQUAL";
	default:
		return "!!! UNKNOWN !!!";
	}
}

static const char * PrimToString( GLenum mode ) {
	static char prim[1024];

	if ( mode == GL_TRIANGLES ) {
		strcpy( prim, "GL_TRIANGLES" );
	} else if ( mode == GL_TRIANGLE_STRIP ) {
		strcpy( prim, "GL_TRIANGLE_STRIP" );
	} else if ( mode == GL_TRIANGLE_FAN ) {
		strcpy( prim, "GL_TRIANGLE_FAN" );
	} else if ( mode == GL_QUADS ) {
		strcpy( prim, "GL_QUADS" );
	} else if ( mode == GL_QUAD_STRIP ) {
		strcpy( prim, "GL_QUAD_STRIP" );
	} else if ( mode == GL_POLYGON ) {
		strcpy( prim, "GL_POLYGON" );
	} else if ( mode == GL_POINTS ) {
		strcpy( prim, "GL_POINTS" );
	} else if ( mode == GL_LINES ) {
		strcpy( prim, "GL_LINES" );
	} else if ( mode == GL_LINE_STRIP ) {
		strcpy( prim, "GL_LINE_STRIP" );
	} else if ( mode == GL_LINE_LOOP ) {
		strcpy( prim, "GL_LINE_LOOP" );
	} else {
		sprintf( prim, "0x%x", mode );
	}

	return prim;
}

static const char * CapToString( GLenum cap ) {
	static char buffer[1024];

	switch ( cap )
	{
	case GL_TEXTURE_2D:
		return "GL_TEXTURE_2D";
	case GL_BLEND:
		return "GL_BLEND";
	case GL_DEPTH_TEST:
		return "GL_DEPTH_TEST";
	case GL_CULL_FACE:
		return "GL_CULL_FACE";
	case GL_CLIP_PLANE0:
		return "GL_CLIP_PLANE0";
	case GL_COLOR_ARRAY:
		return "GL_COLOR_ARRAY";
	case GL_TEXTURE_COORD_ARRAY:
		return "GL_TEXTURE_COORD_ARRAY";
	case GL_VERTEX_ARRAY:
		return "GL_VERTEX_ARRAY";
	case GL_ALPHA_TEST:
		return "GL_ALPHA_TEST";
	case GL_STENCIL_TEST:
		return "GL_STENCIL_TEST";
	default:
		sprintf( buffer, "0x%x", cap );
	}

	return buffer;
}

static const char * TypeToString( GLenum t ) {
	switch ( t )
	{
	case GL_BYTE:
		return "GL_BYTE";
	case GL_UNSIGNED_BYTE:
		return "GL_UNSIGNED_BYTE";
	case GL_SHORT:
		return "GL_SHORT";
	case GL_UNSIGNED_SHORT:
		return "GL_UNSIGNED_SHORT";
	case GL_INT:
		return "GL_INT";
	case GL_UNSIGNED_INT:
		return "GL_UNSIGNED_INT";
	case GL_FLOAT:
		return "GL_FLOAT";
	case GL_DOUBLE:
		return "GL_DOUBLE";
	default:
		return "!!! UNKNOWN !!!";
	}
}

static void APIENTRY logAccum( GLenum op, GLfloat value ) {
	fprintf( sys_gl.log_fp, "glAccum\n" );
	glAccum( op, value );
}

static void APIENTRY logAlphaFunc( GLenum func, GLclampf ref ) {
	fprintf( sys_gl.log_fp, "glAlphaFunc( 0x%x, %f )\n", func, ref );
	glAlphaFunc( func, ref );
}

static GLboolean APIENTRY logAreTexturesResident( GLsizei n, const GLuint *textures, GLboolean *residences ) {
	fprintf( sys_gl.log_fp, "glAreTexturesResident\n" );
	return glAreTexturesResident( n, textures, residences );
}

static void APIENTRY logArrayElement( GLint i ) {
	fprintf( sys_gl.log_fp, "glArrayElement\n" );
	glArrayElement( i );
}

static void APIENTRY logBegin( GLenum mode ) {
	fprintf( sys_gl.log_fp, "glBegin( %s )\n", PrimToString( mode ) );
	glBegin( mode );
}

static void APIENTRY logBindTexture( GLenum target, GLuint texture ) {
	fprintf( sys_gl.log_fp, "glBindTexture( 0x%x, %u )\n", target, texture );
	glBindTexture( target, texture );
}

static void APIENTRY logBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) {
	fprintf( sys_gl.log_fp, "glBitmap\n" );
	glBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
}

static void BlendToName( char *n, GLenum f ) {
	switch ( f )
	{
	case GL_ONE:
		strcpy( n, "GL_ONE" );
		break;
	case GL_ZERO:
		strcpy( n, "GL_ZERO" );
		break;
	case GL_SRC_ALPHA:
		strcpy( n, "GL_SRC_ALPHA" );
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		strcpy( n, "GL_ONE_MINUS_SRC_ALPHA" );
		break;
	case GL_DST_COLOR:
		strcpy( n, "GL_DST_COLOR" );
		break;
	case GL_ONE_MINUS_DST_COLOR:
		strcpy( n, "GL_ONE_MINUS_DST_COLOR" );
		break;
	case GL_DST_ALPHA:
		strcpy( n, "GL_DST_ALPHA" );
		break;
	default:
		sprintf( n, "0x%x", f );
	}
}
static void APIENTRY logBlendFunc( GLenum sfactor, GLenum dfactor ) {
	char sf[128], df[128];

	BlendToName( sf, sfactor );
	BlendToName( df, dfactor );

	fprintf( sys_gl.log_fp, "glBlendFunc( %s, %s )\n", sf, df );
	glBlendFunc( sfactor, dfactor );
}

static void APIENTRY logCallList( GLuint list ) {
	fprintf( sys_gl.log_fp, "glCallList( %u )\n", list );
	glCallList( list );
}

static void APIENTRY logCallLists( GLsizei n, GLenum type, const void *lists ) {
	fprintf( sys_gl.log_fp, "glCallLists\n" );
	glCallLists( n, type, lists );
}

static void APIENTRY logClear( GLbitfield mask ) {
	fprintf( sys_gl.log_fp, "glClear( 0x%x = ", mask );

	if ( mask & GL_COLOR_BUFFER_BIT ) {
		fprintf( sys_gl.log_fp, "GL_COLOR_BUFFER_BIT " );
	}
	if ( mask & GL_DEPTH_BUFFER_BIT ) {
		fprintf( sys_gl.log_fp, "GL_DEPTH_BUFFER_BIT " );
	}
	if ( mask & GL_STENCIL_BUFFER_BIT ) {
		fprintf( sys_gl.log_fp, "GL_STENCIL_BUFFER_BIT " );
	}
	if ( mask & GL_ACCUM_BUFFER_BIT ) {
		fprintf( sys_gl.log_fp, "GL_ACCUM_BUFFER_BIT " );
	}

	fprintf( sys_gl.log_fp, ")\n" );
	glClear( mask );
}

static void APIENTRY logClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
	fprintf( sys_gl.log_fp, "glClearAccum\n" );
	glClearAccum( red, green, blue, alpha );
}

static void APIENTRY logClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
	fprintf( sys_gl.log_fp, "glClearColor\n" );
	glClearColor( red, green, blue, alpha );
}

static void APIENTRY logClearDepth( GLclampd depth ) {
	fprintf( sys_gl.log_fp, "glClearDepth( %f )\n", ( float ) depth );
	glClearDepth( depth );
}

static void APIENTRY logClearIndex( GLfloat c ) {
	fprintf( sys_gl.log_fp, "glClearIndex\n" );
	glClearIndex( c );
}

static void APIENTRY logClearStencil( GLint s ) {
	fprintf( sys_gl.log_fp, "glClearStencil( %d )\n", s );
	glClearStencil( s );
}

static void APIENTRY logClipPlane( GLenum plane, const GLdouble *equation ) {
	fprintf( sys_gl.log_fp, "glClipPlane\n" );
	glClipPlane( plane, equation );
}

static void APIENTRY logColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
	fprintf( sys_gl.log_fp, "glColor3b\n" );
	glColor3b( red, green, blue );
}

static void APIENTRY logColor3bv( const GLbyte *v ) {
	fprintf( sys_gl.log_fp, "glColor3bv\n" );
	glColor3bv( v );
}

static void APIENTRY logColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
	fprintf( sys_gl.log_fp, "glColor3d\n" );
	glColor3d( red, green, blue );
}

static void APIENTRY logColor3dv( const GLdouble *v ) {
	fprintf( sys_gl.log_fp, "glColor3dv\n" );
	glColor3dv( v );
}

static void APIENTRY logColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
	fprintf( sys_gl.log_fp, "glColor3f\n" );
	glColor3f( red, green, blue );
}

static void APIENTRY logColor3fv( const GLfloat *v ) {
	fprintf( sys_gl.log_fp, "glColor3fv\n" );
	glColor3fv( v );
}

static void APIENTRY logColor3i( GLint red, GLint green, GLint blue ) {
	fprintf( sys_gl.log_fp, "glColor3i\n" );
	glColor3i( red, green, blue );
}

static void APIENTRY logColor3iv( const GLint *v ) {
	fprintf( sys_gl.log_fp, "glColor3iv\n" );
	glColor3iv( v );
}

static void APIENTRY logColor3s( GLshort red, GLshort green, GLshort blue ) {
	fprintf( sys_gl.log_fp, "glColor3s\n" );
	glColor3s( red, green, blue );
}

static void APIENTRY logColor3sv( const GLshort *v ) {
	fprintf( sys_gl.log_fp, "glColor3sv\n" );
	glColor3sv( v );
}

static void APIENTRY logColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
	fprintf( sys_gl.log_fp, "glColor3ub\n" );
	glColor3ub( red, green, blue );
}

static void APIENTRY logColor3ubv( const GLubyte *v ) {
	fprintf( sys_gl.log_fp, "glColor3ubv\n" );
	glColor3ubv( v );
}

#define SIG( x ) fprintf( sys_gl.log_fp, x "\n" )

static void APIENTRY logColor3ui( GLuint red, GLuint green, GLuint blue ) {
	SIG( "glColor3ui" );
	glColor3ui( red, green, blue );
}

static void APIENTRY logColor3uiv( const GLuint *v ) {
	SIG( "glColor3uiv" );
	glColor3uiv( v );
}

static void APIENTRY logColor3us( GLushort red, GLushort green, GLushort blue ) {
	SIG( "glColor3us" );
	glColor3us( red, green, blue );
}

static void APIENTRY logColor3usv( const GLushort *v ) {
	SIG( "glColor3usv" );
	glColor3usv( v );
}

static void APIENTRY logColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
	SIG( "glColor4b" );
	glColor4b( red, green, blue, alpha );
}

static void APIENTRY logColor4bv( const GLbyte *v ) {
	SIG( "glColor4bv" );
	glColor4bv( v );
}

static void APIENTRY logColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
	SIG( "glColor4d" );
	glColor4d( red, green, blue, alpha );
}
static void APIENTRY logColor4dv( const GLdouble *v ) {
	SIG( "glColor4dv" );
	glColor4dv( v );
}
static void APIENTRY logColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
	fprintf( sys_gl.log_fp, "glColor4f( %f,%f,%f,%f )\n", red, green, blue, alpha );
	glColor4f( red, green, blue, alpha );
}
static void APIENTRY logColor4fv( const GLfloat *v ) {
	fprintf( sys_gl.log_fp, "glColor4fv( %f,%f,%f,%f )\n", v[0], v[1], v[2], v[3] );
	glColor4fv( v );
}
static void APIENTRY logColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
	SIG( "glColor4i" );
	glColor4i( red, green, blue, alpha );
}
static void APIENTRY logColor4iv( const GLint *v ) {
	SIG( "glColor4iv" );
	glColor4iv( v );
}
static void APIENTRY logColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
	SIG( "glColor4s" );
	glColor4s( red, green, blue, alpha );
}
static void APIENTRY logColor4sv( const GLshort *v ) {
	SIG( "glColor4sv" );
	glColor4sv( v );
}
static void APIENTRY logColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
	SIG( "glColor4b" );
	glColor4b( red, green, blue, alpha );
}
static void APIENTRY logColor4ubv( const GLubyte *v ) {
	SIG( "glColor4ubv" );
	glColor4ubv( v );
}
static void APIENTRY logColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
	SIG( "glColor4ui" );
	glColor4ui( red, green, blue, alpha );
}
static void APIENTRY logColor4uiv( const GLuint *v ) {
	SIG( "glColor4uiv" );
	glColor4uiv( v );
}
static void APIENTRY logColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
	SIG( "glColor4us" );
	glColor4us( red, green, blue, alpha );
}
static void APIENTRY logColor4usv( const GLushort *v ) {
	SIG( "glColor4usv" );
	glColor4usv( v );
}
static void APIENTRY logColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
	SIG( "glColorMask" );
	glColorMask( red, green, blue, alpha );
}
static void APIENTRY logColorMaterial( GLenum face, GLenum mode ) {
	SIG( "glColorMaterial" );
	glColorMaterial( face, mode );
}

static void APIENTRY logColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) {
	fprintf( sys_gl.log_fp, "glColorPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
	glColorPointer( size, type, stride, pointer );
}

static void APIENTRY logCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
	SIG( "glCopyPixels" );
	glCopyPixels( x, y, width, height, type );
}

static void APIENTRY logCopyTexImage1D( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border ) {
	SIG( "glCopyTexImage1D" );
	glCopyTexImage1D( target, level, internalFormat, x, y, width, border );
}

static void APIENTRY logCopyTexImage2D( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
	SIG( "glCopyTexImage2D" );
	glCopyTexImage2D( target, level, internalFormat, x, y, width, height, border );
}

static void APIENTRY logCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
	SIG( "glCopyTexSubImage1D" );
	glCopyTexSubImage1D( target, level, xoffset, x, y, width );
}

static void APIENTRY logCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
	SIG( "glCopyTexSubImage2D" );
	glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

static void APIENTRY logCullFace( GLenum mode ) {
	fprintf( sys_gl.log_fp, "glCullFace( %s )\n", ( mode == GL_FRONT ) ? "GL_FRONT" : "GL_BACK" );
	glCullFace( mode );
}

static void APIENTRY logDeleteLists( GLuint list, GLsizei range ) {
	SIG( "glDeleteLists" );
	glDeleteLists( list, range );
}

static void APIENTRY logDeleteTextures( GLsizei n, const GLuint *textures ) {
	SIG( "glDeleteTextures" );
	glDeleteTextures( n, textures );
}

static void APIENTRY logDepthFunc( GLenum func ) {
	fprintf( sys_gl.log_fp, "glDepthFunc( %s )\n", FuncToString( func ) );
	glDepthFunc( func );
}

static void APIENTRY logDepthMask( GLboolean flag ) {
	fprintf( sys_gl.log_fp, "glDepthMask( %s )\n", BooleanToString( flag ) );
	glDepthMask( flag );
}

static void APIENTRY logDepthRange( GLclampd zNear, GLclampd zFar ) {
	fprintf( sys_gl.log_fp, "glDepthRange( %f, %f )\n", ( float ) zNear, ( float ) zFar );
	glDepthRange( zNear, zFar );
}

static void APIENTRY logDisable( GLenum cap ) {
	fprintf( sys_gl.log_fp, "glDisable( %s )\n", CapToString( cap ) );
	glDisable( cap );
}

static void APIENTRY logDisableClientState( GLenum array ) {
	fprintf( sys_gl.log_fp, "glDisableClientState( %s )\n", CapToString( array ) );
	glDisableClientState( array );
}

static void APIENTRY logDrawArrays( GLenum mode, GLint first, GLsizei count ) {
	SIG( "glDrawArrays" );
	glDrawArrays( mode, first, count );
}

static void APIENTRY logDrawBuffer( GLenum mode ) {
	SIG( "glDrawBuffer" );
	glDrawBuffer( mode );
}

static void APIENTRY logDrawElements( GLenum mode, GLsizei count, GLenum type, const void *indices ) {
	fprintf( sys_gl.log_fp, "glDrawElements( %s, %d, %s, MEM )\n", PrimToString( mode ), count, TypeToString( type ) );
	glDrawElements( mode, count, type, indices );
}

static void APIENTRY logDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) {
	SIG( "glDrawPixels" );
	glDrawPixels( width, height, format, type, pixels );
}

static void APIENTRY logEdgeFlag( GLboolean flag ) {
	SIG( "glEdgeFlag" );
	glEdgeFlag( flag );
}

static void APIENTRY logEdgeFlagPointer( GLsizei stride, const GLboolean *pointer ) {
	SIG( "glEdgeFlagPointer" );
	glEdgeFlagPointer( stride, pointer );
}

static void APIENTRY logEdgeFlagv( const GLboolean *flag ) {
	SIG( "glEdgeFlagv" );
	glEdgeFlagv( flag );
}

static void APIENTRY logEnable( GLenum cap ) {
	fprintf( sys_gl.log_fp, "glEnable( %s )\n", CapToString( cap ) );
	glEnable( cap );
}

static void APIENTRY logEnableClientState( GLenum array ) {
	fprintf( sys_gl.log_fp, "glEnableClientState( %s )\n", CapToString( array ) );
	glEnableClientState( array );
}

static void APIENTRY logEnd( void ) {
	SIG( "glEnd" );
	glEnd();
}

static void APIENTRY logEndList( void ) {
	SIG( "glEndList" );
	glEndList();
}

static void APIENTRY logEvalCoord1d( GLdouble u ) {
	SIG( "glEvalCoord1d" );
	glEvalCoord1d( u );
}

static void APIENTRY logEvalCoord1dv( const GLdouble *u ) {
	SIG( "glEvalCoord1dv" );
	glEvalCoord1dv( u );
}

static void APIENTRY logEvalCoord1f( GLfloat u ) {
	SIG( "glEvalCoord1f" );
	glEvalCoord1f( u );
}

static void APIENTRY logEvalCoord1fv( const GLfloat *u ) {
	SIG( "glEvalCoord1fv" );
	glEvalCoord1fv( u );
}
static void APIENTRY logEvalCoord2d( GLdouble u, GLdouble v ) {
	SIG( "glEvalCoord2d" );
	glEvalCoord2d( u, v );
}
static void APIENTRY logEvalCoord2dv( const GLdouble *u ) {
	SIG( "glEvalCoord2dv" );
	glEvalCoord2dv( u );
}
static void APIENTRY logEvalCoord2f( GLfloat u, GLfloat v ) {
	SIG( "glEvalCoord2f" );
	glEvalCoord2f( u, v );
}
static void APIENTRY logEvalCoord2fv( const GLfloat *u ) {
	SIG( "glEvalCoord2fv" );
	glEvalCoord2fv( u );
}

static void APIENTRY logEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
	SIG( "glEvalMesh1" );
	glEvalMesh1( mode, i1, i2 );
}
static void APIENTRY logEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
	SIG( "glEvalMesh2" );
	glEvalMesh2( mode, i1, i2, j1, j2 );
}
static void APIENTRY logEvalPoint1( GLint i ) {
	SIG( "glEvalPoint1" );
	glEvalPoint1( i );
}
static void APIENTRY logEvalPoint2( GLint i, GLint j ) {
	SIG( "glEvalPoint2" );
	glEvalPoint2( i, j );
}

static void APIENTRY logFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer ) {
	SIG( "glFeedbackBuffer" );
	glFeedbackBuffer( size, type, buffer );
}

static void APIENTRY logFinish( void ) {
	SIG( "glFinish" );
	glFinish();
}

static void APIENTRY logFlush( void ) {
	SIG( "glFlush" );
	glFlush();
}

static void APIENTRY logFogf( GLenum pname, GLfloat param ) {
	SIG( "glFogf" );
	glFogf( pname, param );
}

static void APIENTRY logFogfv( GLenum pname, const GLfloat *params ) {
	SIG( "glFogfv" );
	glFogfv( pname, params );
}

static void APIENTRY logFogi( GLenum pname, GLint param ) {
	SIG( "glFogi" );
	glFogi( pname, param );
}

static void APIENTRY logFogiv( GLenum pname, const GLint *params ) {
	SIG( "glFogiv" );
	glFogiv( pname, params );
}

static void APIENTRY logFrontFace( GLenum mode ) {
	SIG( "glFrontFace" );
	glFrontFace( mode );
}

static void APIENTRY logFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	SIG( "glFrustum" );
	glFrustum( left, right, bottom, top, zNear, zFar );
}

static GLuint APIENTRY logGenLists( GLsizei range ) {
	SIG( "glGenLists" );
	return glGenLists( range );
}

static void APIENTRY logGenTextures( GLsizei n, GLuint *textures ) {
	SIG( "glGenTextures" );
	glGenTextures( n, textures );
}

static void APIENTRY logGetBooleanv( GLenum pname, GLboolean *params ) {
	SIG( "glGetBooleanv" );
	glGetBooleanv( pname, params );
}

static void APIENTRY logGetClipPlane( GLenum plane, GLdouble *equation ) {
	SIG( "glGetClipPlane" );
	glGetClipPlane( plane, equation );
}

static void APIENTRY logGetDoublev( GLenum pname, GLdouble *params ) {
	SIG( "glGetDoublev" );
	glGetDoublev( pname, params );
}

static GLenum APIENTRY logGetError( void ) {
	SIG( "glGetError" );
	return glGetError();
}

static void APIENTRY logGetFloatv( GLenum pname, GLfloat *params ) {
	SIG( "glGetFloatv" );
	glGetFloatv( pname, params );
}

static void APIENTRY logGetIntegerv( GLenum pname, GLint *params ) {
	SIG( "glGetIntegerv" );
	glGetIntegerv( pname, params );
}

static void APIENTRY logGetLightfv( GLenum light, GLenum pname, GLfloat *params ) {
	SIG( "glGetLightfv" );
	glGetLightfv( light, pname, params );
}

static void APIENTRY logGetLightiv( GLenum light, GLenum pname, GLint *params ) {
	SIG( "glGetLightiv" );
	glGetLightiv( light, pname, params );
}

static void APIENTRY logGetMapdv( GLenum target, GLenum query, GLdouble *v ) {
	SIG( "glGetMapdv" );
	glGetMapdv( target, query, v );
}

static void APIENTRY logGetMapfv( GLenum target, GLenum query, GLfloat *v ) {
	SIG( "glGetMapfv" );
	glGetMapfv( target, query, v );
}

static void APIENTRY logGetMapiv( GLenum target, GLenum query, GLint *v ) {
	SIG( "glGetMapiv" );
	glGetMapiv( target, query, v );
}

static void APIENTRY logGetMaterialfv( GLenum face, GLenum pname, GLfloat *params ) {
	SIG( "glGetMaterialfv" );
	glGetMaterialfv( face, pname, params );
}

static void APIENTRY logGetMaterialiv( GLenum face, GLenum pname, GLint *params ) {
	SIG( "glGetMaterialiv" );
	glGetMaterialiv( face, pname, params );
}

static void APIENTRY logGetPixelMapfv( GLenum map, GLfloat *values ) {
	SIG( "glGetPixelMapfv" );
	glGetPixelMapfv( map, values );
}

static void APIENTRY logGetPixelMapuiv( GLenum map, GLuint *values ) {
	SIG( "glGetPixelMapuiv" );
	glGetPixelMapuiv( map, values );
}

static void APIENTRY logGetPixelMapusv( GLenum map, GLushort *values ) {
	SIG( "glGetPixelMapusv" );
	glGetPixelMapusv( map, values );
}

static void APIENTRY logGetPointerv( GLenum pname, GLvoid* *params ) {
	SIG( "glGetPointerv" );
	glGetPointerv( pname, params );
}

static void APIENTRY logGetPolygonStipple( GLubyte *mask ) {
	SIG( "glGetPolygonStipple" );
	glGetPolygonStipple( mask );
}

static const GLubyte * APIENTRY logGetString( GLenum name ) {
	SIG( "glGetString" );
	return glGetString( name );
}

static void APIENTRY logGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params ) {
	SIG( "glGetTexEnvfv" );
	glGetTexEnvfv( target, pname, params );
}

static void APIENTRY logGetTexEnviv( GLenum target, GLenum pname, GLint *params ) {
	SIG( "glGetTexEnviv" );
	glGetTexEnviv( target, pname, params );
}

static void APIENTRY logGetTexGendv( GLenum coord, GLenum pname, GLdouble *params ) {
	SIG( "glGetTexGendv" );
	glGetTexGendv( coord, pname, params );
}

static void APIENTRY logGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params ) {
	SIG( "glGetTexGenfv" );
	glGetTexGenfv( coord, pname, params );
}

static void APIENTRY logGetTexGeniv( GLenum coord, GLenum pname, GLint *params ) {
	SIG( "glGetTexGeniv" );
	glGetTexGeniv( coord, pname, params );
}

static void APIENTRY logGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, void *pixels ) {
	SIG( "glGetTexImage" );
	glGetTexImage( target, level, format, type, pixels );
}
static void APIENTRY logGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params ) {
	SIG( "glGetTexLevelParameterfv" );
	glGetTexLevelParameterfv( target, level, pname, params );
}

static void APIENTRY logGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params ) {
	SIG( "glGetTexLevelParameteriv" );
	glGetTexLevelParameteriv( target, level, pname, params );
}

static void APIENTRY logGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params ) {
	SIG( "glGetTexParameterfv" );
	glGetTexParameterfv( target, pname, params );
}

static void APIENTRY logGetTexParameteriv( GLenum target, GLenum pname, GLint *params ) {
	SIG( "glGetTexParameteriv" );
	glGetTexParameteriv( target, pname, params );
}

static void APIENTRY logHint( GLenum target, GLenum mode ) {
	fprintf( sys_gl.log_fp, "glHint( 0x%x, 0x%x )\n", target, mode );
	glHint( target, mode );
}

static void APIENTRY logIndexMask( GLuint mask ) {
	SIG( "glIndexMask" );
	glIndexMask( mask );
}

static void APIENTRY logIndexPointer( GLenum type, GLsizei stride, const void *pointer ) {
	SIG( "glIndexPointer" );
	glIndexPointer( type, stride, pointer );
}

static void APIENTRY logIndexd( GLdouble c ) {
	SIG( "glIndexd" );
	glIndexd( c );
}

static void APIENTRY logIndexdv( const GLdouble *c ) {
	SIG( "glIndexdv" );
	glIndexdv( c );
}

static void APIENTRY logIndexf( GLfloat c ) {
	SIG( "glIndexf" );
	glIndexf( c );
}

static void APIENTRY logIndexfv( const GLfloat *c ) {
	SIG( "glIndexfv" );
	glIndexfv( c );
}

static void APIENTRY logIndexi( GLint c ) {
	SIG( "glIndexi" );
	glIndexi( c );
}

static void APIENTRY logIndexiv( const GLint *c ) {
	SIG( "glIndexiv" );
	glIndexiv( c );
}

static void APIENTRY logIndexs( GLshort c ) {
	SIG( "glIndexs" );
	glIndexs( c );
}

static void APIENTRY logIndexsv( const GLshort *c ) {
	SIG( "glIndexsv" );
	glIndexsv( c );
}

static void APIENTRY logIndexub( GLubyte c ) {
	SIG( "glIndexub" );
	glIndexub( c );
}

static void APIENTRY logIndexubv( const GLubyte *c ) {
	SIG( "glIndexubv" );
	glIndexubv( c );
}

static void APIENTRY logInitNames( void ) {
	SIG( "glInitNames" );
	glInitNames();
}

static void APIENTRY logInterleavedArrays( GLenum format, GLsizei stride, const void *pointer ) {
	SIG( "glInterleavedArrays" );
	glInterleavedArrays( format, stride, pointer );
}

static GLboolean APIENTRY logIsEnabled( GLenum cap ) {
	SIG( "glIsEnabled" );
	return glIsEnabled( cap );
}
static GLboolean APIENTRY logIsList( GLuint list ) {
	SIG( "glIsList" );
	return glIsList( list );
}
static GLboolean APIENTRY logIsTexture( GLuint texture ) {
	SIG( "glIsTexture" );
	return glIsTexture( texture );
}

static void APIENTRY logLightModelf( GLenum pname, GLfloat param ) {
	SIG( "glLightModelf" );
	glLightModelf( pname, param );
}

static void APIENTRY logLightModelfv( GLenum pname, const GLfloat *params ) {
	SIG( "glLightModelfv" );
	glLightModelfv( pname, params );
}

static void APIENTRY logLightModeli( GLenum pname, GLint param ) {
	SIG( "glLightModeli" );
	glLightModeli( pname, param );

}

static void APIENTRY logLightModeliv( GLenum pname, const GLint *params ) {
	SIG( "glLightModeliv" );
	glLightModeliv( pname, params );
}

static void APIENTRY logLightf( GLenum light, GLenum pname, GLfloat param ) {
	SIG( "glLightf" );
	glLightf( light, pname, param );
}

static void APIENTRY logLightfv( GLenum light, GLenum pname, const GLfloat *params ) {
	SIG( "glLightfv" );
	glLightfv( light, pname, params );
}

static void APIENTRY logLighti( GLenum light, GLenum pname, GLint param ) {
	SIG( "glLighti" );
	glLighti( light, pname, param );
}

static void APIENTRY logLightiv( GLenum light, GLenum pname, const GLint *params ) {
	SIG( "glLightiv" );
	glLightiv( light, pname, params );
}

static void APIENTRY logLineStipple( GLint factor, GLushort pattern ) {
	SIG( "glLineStipple" );
	glLineStipple( factor, pattern );
}

static void APIENTRY logLineWidth( GLfloat width ) {
	SIG( "glLineWidth" );
	glLineWidth( width );
}

static void APIENTRY logListBase( GLuint base ) {
	SIG( "glListBase" );
	glListBase( base );
}

static void APIENTRY logLoadIdentity( void ) {
	SIG( "glLoadIdentity" );
	glLoadIdentity();
}

static void APIENTRY logLoadMatrixd( const GLdouble *m ) {
	SIG( "glLoadMatrixd" );
	glLoadMatrixd( m );
}

static void APIENTRY logLoadMatrixf( const GLfloat *m ) {
	SIG( "glLoadMatrixf" );
	glLoadMatrixf( m );
}

static void APIENTRY logLoadName( GLuint name ) {
	SIG( "glLoadName" );
	glLoadName( name );
}

static void APIENTRY logLogicOp( GLenum opcode ) {
	SIG( "glLogicOp" );
	glLogicOp( opcode );
}

static void APIENTRY logMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) {
	SIG( "glMap1d" );
	glMap1d( target, u1, u2, stride, order, points );
}

static void APIENTRY logMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) {
	SIG( "glMap1f" );
	glMap1f( target, u1, u2, stride, order, points );
}

static void APIENTRY logMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) {
	SIG( "glMap2d" );
	glMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void APIENTRY logMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) {
	SIG( "glMap2f" );
	glMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void APIENTRY logMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
	SIG( "glMapGrid1d" );
	glMapGrid1d( un, u1, u2 );
}

static void APIENTRY logMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
	SIG( "glMapGrid1f" );
	glMapGrid1f( un, u1, u2 );
}

static void APIENTRY logMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
	SIG( "glMapGrid2d" );
	glMapGrid2d( un, u1, u2, vn, v1, v2 );
}
static void APIENTRY logMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
	SIG( "glMapGrid2f" );
	glMapGrid2f( un, u1, u2, vn, v1, v2 );
}
static void APIENTRY logMaterialf( GLenum face, GLenum pname, GLfloat param ) {
	SIG( "glMaterialf" );
	glMaterialf( face, pname, param );
}
static void APIENTRY logMaterialfv( GLenum face, GLenum pname, const GLfloat *params ) {
	SIG( "glMaterialfv" );
	glMaterialfv( face, pname, params );
}

static void APIENTRY logMateriali( GLenum face, GLenum pname, GLint param ) {
	SIG( "glMateriali" );
	glMateriali( face, pname, param );
}

static void APIENTRY logMaterialiv( GLenum face, GLenum pname, const GLint *params ) {
	SIG( "glMaterialiv" );
	glMaterialiv( face, pname, params );
}

static void APIENTRY logMatrixMode( GLenum mode ) {
	SIG( "glMatrixMode" );
	glMatrixMode( mode );
}

static void APIENTRY logMultMatrixd( const GLdouble *m ) {
	SIG( "glMultMatrixd" );
	glMultMatrixd( m );
}

static void APIENTRY logMultMatrixf( const GLfloat *m ) {
	SIG( "glMultMatrixf" );
	glMultMatrixf( m );
}

static void APIENTRY logNewList( GLuint list, GLenum mode ) {
	SIG( "glNewList" );
	glNewList( list, mode );
}

static void APIENTRY logNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
	SIG( "glNormal3b" );
	glNormal3b( nx, ny, nz );
}

static void APIENTRY logNormal3bv( const GLbyte *v ) {
	SIG( "glNormal3bv" );
	glNormal3bv( v );
}

static void APIENTRY logNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
	SIG( "glNormal3d" );
	glNormal3d( nx, ny, nz );
}

static void APIENTRY logNormal3dv( const GLdouble *v ) {
	SIG( "glNormal3dv" );
	glNormal3dv( v );
}

static void APIENTRY logNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
	SIG( "glNormal3f" );
	glNormal3f( nx, ny, nz );
}

static void APIENTRY logNormal3fv( const GLfloat *v ) {
	SIG( "glNormal3fv" );
	glNormal3fv( v );
}
static void APIENTRY logNormal3i( GLint nx, GLint ny, GLint nz ) {
	SIG( "glNormal3i" );
	glNormal3i( nx, ny, nz );
}
static void APIENTRY logNormal3iv( const GLint *v ) {
	SIG( "glNormal3iv" );
	glNormal3iv( v );
}
static void APIENTRY logNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
	SIG( "glNormal3s" );
	glNormal3s( nx, ny, nz );
}
static void APIENTRY logNormal3sv( const GLshort *v ) {
	SIG( "glNormal3sv" );
	glNormal3sv( v );
}
static void APIENTRY logNormalPointer( GLenum type, GLsizei stride, const void *pointer ) {
	SIG( "glNormalPointer" );
	glNormalPointer( type, stride, pointer );
}
static void APIENTRY logOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	SIG( "glOrtho" );
	glOrtho( left, right, bottom, top, zNear, zFar );
}

static void APIENTRY logPassThrough( GLfloat token ) {
	SIG( "glPassThrough" );
	glPassThrough( token );
}

static void APIENTRY logPixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values ) {
	SIG( "glPixelMapfv" );
	glPixelMapfv( map, mapsize, values );
}

static void APIENTRY logPixelMapuiv( GLenum map, GLsizei mapsize, const GLuint *values ) {
	SIG( "glPixelMapuiv" );
	glPixelMapuiv( map, mapsize, values );
}

static void APIENTRY logPixelMapusv( GLenum map, GLsizei mapsize, const GLushort *values ) {
	SIG( "glPixelMapusv" );
	glPixelMapusv( map, mapsize, values );
}
static void APIENTRY logPixelStoref( GLenum pname, GLfloat param ) {
	SIG( "glPixelStoref" );
	glPixelStoref( pname, param );
}
static void APIENTRY logPixelStorei( GLenum pname, GLint param ) {
	SIG( "glPixelStorei" );
	glPixelStorei( pname, param );
}
static void APIENTRY logPixelTransferf( GLenum pname, GLfloat param ) {
	SIG( "glPixelTransferf" );
	glPixelTransferf( pname, param );
}

static void APIENTRY logPixelTransferi( GLenum pname, GLint param ) {
	SIG( "glPixelTransferi" );
	glPixelTransferi( pname, param );
}

static void APIENTRY logPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
	SIG( "glPixelZoom" );
	glPixelZoom( xfactor, yfactor );
}

static void APIENTRY logPointSize( GLfloat size ) {
	SIG( "glPointSize" );
	glPointSize( size );
}

static void APIENTRY logPolygonMode( GLenum face, GLenum mode ) {
	fprintf( sys_gl.log_fp, "glPolygonMode( 0x%x, 0x%x )\n", face, mode );
	glPolygonMode( face, mode );
}

static void APIENTRY logPolygonOffset( GLfloat factor, GLfloat units ) {
	SIG( "glPolygonOffset" );
	glPolygonOffset( factor, units );
}
static void APIENTRY logPolygonStipple( const GLubyte *mask ) {
	SIG( "glPolygonStipple" );
	glPolygonStipple( mask );
}
static void APIENTRY logPopAttrib( void ) {
	SIG( "glPopAttrib" );
	glPopAttrib();
}

static void APIENTRY logPopClientAttrib( void ) {
	SIG( "glPopClientAttrib" );
	glPopClientAttrib();
}

static void APIENTRY logPopMatrix( void ) {
	SIG( "glPopMatrix" );
	glPopMatrix();
}

static void APIENTRY logPopName( void ) {
	SIG( "glPopName" );
	glPopName();
}

static void APIENTRY logPrioritizeTextures( GLsizei n, const GLuint *textures, const GLclampf *priorities ) {
	SIG( "glPrioritizeTextures" );
	glPrioritizeTextures( n, textures, priorities );
}

static void APIENTRY logPushAttrib( GLbitfield mask ) {
	SIG( "glPushAttrib" );
	glPushAttrib( mask );
}

static void APIENTRY logPushClientAttrib( GLbitfield mask ) {
	SIG( "glPushClientAttrib" );
	glPushClientAttrib( mask );
}

static void APIENTRY logPushMatrix( void ) {
	SIG( "glPushMatrix" );
	glPushMatrix();
}

static void APIENTRY logPushName( GLuint name ) {
	SIG( "glPushName" );
	glPushName( name );
}

static void APIENTRY logRasterPos2d( GLdouble x, GLdouble y ) {
	SIG( "glRasterPot2d" );
	glRasterPos2d( x, y );
}

static void APIENTRY logRasterPos2dv( const GLdouble *v ) {
	SIG( "glRasterPos2dv" );
	glRasterPos2dv( v );
}

static void APIENTRY logRasterPos2f( GLfloat x, GLfloat y ) {
	SIG( "glRasterPos2f" );
	glRasterPos2f( x, y );
}
static void APIENTRY logRasterPos2fv( const GLfloat *v ) {
	SIG( "glRasterPos2dv" );
	glRasterPos2fv( v );
}
static void APIENTRY logRasterPos2i( GLint x, GLint y ) {
	SIG( "glRasterPos2if" );
	glRasterPos2i( x, y );
}
static void APIENTRY logRasterPos2iv( const GLint *v ) {
	SIG( "glRasterPos2iv" );
	glRasterPos2iv( v );
}
static void APIENTRY logRasterPos2s( GLshort x, GLshort y ) {
	SIG( "glRasterPos2s" );
	glRasterPos2s( x, y );
}
static void APIENTRY logRasterPos2sv( const GLshort *v ) {
	SIG( "glRasterPos2sv" );
	glRasterPos2sv( v );
}
static void APIENTRY logRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
	SIG( "glRasterPos3d" );
	glRasterPos3d( x, y, z );
}
static void APIENTRY logRasterPos3dv( const GLdouble *v ) {
	SIG( "glRasterPos3dv" );
	glRasterPos3dv( v );
}
static void APIENTRY logRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
	SIG( "glRasterPos3f" );
	glRasterPos3f( x, y, z );
}
static void APIENTRY logRasterPos3fv( const GLfloat *v ) {
	SIG( "glRasterPos3fv" );
	glRasterPos3fv( v );
}
static void APIENTRY logRasterPos3i( GLint x, GLint y, GLint z ) {
	SIG( "glRasterPos3i" );
	glRasterPos3i( x, y, z );
}
static void APIENTRY logRasterPos3iv( const GLint *v ) {
	SIG( "glRasterPos3iv" );
	glRasterPos3iv( v );
}
static void APIENTRY logRasterPos3s( GLshort x, GLshort y, GLshort z ) {
	SIG( "glRasterPos3s" );
	glRasterPos3s( x, y, z );
}
static void APIENTRY logRasterPos3sv( const GLshort *v ) {
	SIG( "glRasterPos3sv" );
	glRasterPos3sv( v );
}
static void APIENTRY logRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	SIG( "glRasterPos4d" );
	glRasterPos4d( x, y, z, w );
}
static void APIENTRY logRasterPos4dv( const GLdouble *v ) {
	SIG( "glRasterPos4dv" );
	glRasterPos4dv( v );
}
static void APIENTRY logRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	SIG( "glRasterPos4f" );
	glRasterPos4f( x, y, z, w );
}
static void APIENTRY logRasterPos4fv( const GLfloat *v ) {
	SIG( "glRasterPos4fv" );
	glRasterPos4fv( v );
}
static void APIENTRY logRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
	SIG( "glRasterPos4i" );
	glRasterPos4i( x, y, z, w );
}
static void APIENTRY logRasterPos4iv( const GLint *v ) {
	SIG( "glRasterPos4iv" );
	glRasterPos4iv( v );
}
static void APIENTRY logRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
	SIG( "glRasterPos4s" );
	glRasterPos4s( x, y, z, w );
}
static void APIENTRY logRasterPos4sv( const GLshort *v ) {
	SIG( "glRasterPos4sv" );
	glRasterPos4sv( v );
}
static void APIENTRY logReadBuffer( GLenum mode ) {
	SIG( "glReadBuffer" );
	glReadBuffer( mode );
}
static void APIENTRY logReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels ) {
	SIG( "glReadPixels" );
	glReadPixels( x, y, width, height, format, type, pixels );
}

static void APIENTRY logRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
	SIG( "glRectd" );
	glRectd( x1, y1, x2, y2 );
}

static void APIENTRY logRectdv( const GLdouble *v1, const GLdouble *v2 ) {
	SIG( "glRectdv" );
	glRectdv( v1, v2 );
}

static void APIENTRY logRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
	SIG( "glRectf" );
	glRectf( x1, y1, x2, y2 );
}

static void APIENTRY logRectfv( const GLfloat *v1, const GLfloat *v2 ) {
	SIG( "glRectfv" );
	glRectfv( v1, v2 );
}
static void APIENTRY logRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
	SIG( "glRecti" );
	glRecti( x1, y1, x2, y2 );
}
static void APIENTRY logRectiv( const GLint *v1, const GLint *v2 ) {
	SIG( "glRectiv" );
	glRectiv( v1, v2 );
}
static void APIENTRY logRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
	SIG( "glRects" );
	glRects( x1, y1, x2, y2 );
}
static void APIENTRY logRectsv( const GLshort *v1, const GLshort *v2 ) {
	SIG( "glRectsv" );
	glRectsv( v1, v2 );
}
static GLint APIENTRY logRenderMode( GLenum mode ) {
	SIG( "glRenderMode" );
	return glRenderMode( mode );
}
static void APIENTRY logRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
	SIG( "glRotated" );
	glRotated( angle, x, y, z );
}

static void APIENTRY logRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
	SIG( "glRotatef" );
	glRotatef( angle, x, y, z );
}

static void APIENTRY logScaled( GLdouble x, GLdouble y, GLdouble z ) {
	SIG( "glScaled" );
	glScaled( x, y, z );
}

static void APIENTRY logScalef( GLfloat x, GLfloat y, GLfloat z ) {
	SIG( "glScalef" );
	glScalef( x, y, z );
}

static void APIENTRY logScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
	fprintf( sys_gl.log_fp, "glScissor( %d, %d, %d, %d )\n", x, y, width, height );
	glScissor( x, y, width, height );
}

static void APIENTRY logSelectBuffer( GLsizei size, GLuint *buffer ) {
	SIG( "glSelectBuffer" );
	glSelectBuffer( size, buffer );
}

static void APIENTRY logShadeModel( GLenum mode ) {
	SIG( "glShadeModel" );
	glShadeModel( mode );
}

static void APIENTRY logStencilFunc( GLenum func, GLint ref, GLuint mask ) {
	SIG( "glStencilFunc" );
	glStencilFunc( func, ref, mask );
}

static void APIENTRY logStencilMask( GLuint mask ) {
	SIG( "glStencilMask" );
	glStencilMask( mask );
}

static void APIENTRY logStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
	SIG( "glStencilOp" );
	glStencilOp( fail, zfail, zpass );
}

static void APIENTRY logTexCoord1d( GLdouble s ) {
	SIG( "glTexCoord1d" );
	glTexCoord1d( s );
}

static void APIENTRY logTexCoord1dv( const GLdouble *v ) {
	SIG( "glTexCoord1dv" );
	glTexCoord1dv( v );
}

static void APIENTRY logTexCoord1f( GLfloat s ) {
	SIG( "glTexCoord1f" );
	glTexCoord1f( s );
}
static void APIENTRY logTexCoord1fv( const GLfloat *v ) {
	SIG( "glTexCoord1fv" );
	glTexCoord1fv( v );
}
static void APIENTRY logTexCoord1i( GLint s ) {
	SIG( "glTexCoord1i" );
	glTexCoord1i( s );
}
static void APIENTRY logTexCoord1iv( const GLint *v ) {
	SIG( "glTexCoord1iv" );
	glTexCoord1iv( v );
}
static void APIENTRY logTexCoord1s( GLshort s ) {
	SIG( "glTexCoord1s" );
	glTexCoord1s( s );
}
static void APIENTRY logTexCoord1sv( const GLshort *v ) {
	SIG( "glTexCoord1sv" );
	glTexCoord1sv( v );
}
static void APIENTRY logTexCoord2d( GLdouble s, GLdouble t ) {
	SIG( "glTexCoord2d" );
	glTexCoord2d( s, t );
}

static void APIENTRY logTexCoord2dv( const GLdouble *v ) {
	SIG( "glTexCoord2dv" );
	glTexCoord2dv( v );
}
static void APIENTRY logTexCoord2f( GLfloat s, GLfloat t ) {
	SIG( "glTexCoord2f" );
	glTexCoord2f( s, t );
}
static void APIENTRY logTexCoord2fv( const GLfloat *v ) {
	SIG( "glTexCoord2fv" );
	glTexCoord2fv( v );
}
static void APIENTRY logTexCoord2i( GLint s, GLint t ) {
	SIG( "glTexCoord2i" );
	glTexCoord2i( s, t );
}
static void APIENTRY logTexCoord2iv( const GLint *v ) {
	SIG( "glTexCoord2iv" );
	glTexCoord2iv( v );
}
static void APIENTRY logTexCoord2s( GLshort s, GLshort t ) {
	SIG( "glTexCoord2s" );
	glTexCoord2s( s, t );
}
static void APIENTRY logTexCoord2sv( const GLshort *v ) {
	SIG( "glTexCoord2sv" );
	glTexCoord2sv( v );
}
static void APIENTRY logTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
	SIG( "glTexCoord3d" );
	glTexCoord3d( s, t, r );
}
static void APIENTRY logTexCoord3dv( const GLdouble *v ) {
	SIG( "glTexCoord3dv" );
	glTexCoord3dv( v );
}
static void APIENTRY logTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
	SIG( "glTexCoord3f" );
	glTexCoord3f( s, t, r );
}
static void APIENTRY logTexCoord3fv( const GLfloat *v ) {
	SIG( "glTexCoord3fv" );
	glTexCoord3fv( v );
}
static void APIENTRY logTexCoord3i( GLint s, GLint t, GLint r ) {
	SIG( "glTexCoord3i" );
	glTexCoord3i( s, t, r );
}
static void APIENTRY logTexCoord3iv( const GLint *v ) {
	SIG( "glTexCoord3iv" );
	glTexCoord3iv( v );
}
static void APIENTRY logTexCoord3s( GLshort s, GLshort t, GLshort r ) {
	SIG( "glTexCoord3s" );
	glTexCoord3s( s, t, r );
}
static void APIENTRY logTexCoord3sv( const GLshort *v ) {
	SIG( "glTexCoord3sv" );
	glTexCoord3sv( v );
}
static void APIENTRY logTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
	SIG( "glTexCoord4d" );
	glTexCoord4d( s, t, r, q );
}
static void APIENTRY logTexCoord4dv( const GLdouble *v ) {
	SIG( "glTexCoord4dv" );
	glTexCoord4dv( v );
}
static void APIENTRY logTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
	SIG( "glTexCoord4f" );
	glTexCoord4f( s, t, r, q );
}
static void APIENTRY logTexCoord4fv( const GLfloat *v ) {
	SIG( "glTexCoord4fv" );
	glTexCoord4fv( v );
}
static void APIENTRY logTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
	SIG( "glTexCoord4i" );
	glTexCoord4i( s, t, r, q );
}
static void APIENTRY logTexCoord4iv( const GLint *v ) {
	SIG( "glTexCoord4iv" );
	glTexCoord4iv( v );
}
static void APIENTRY logTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
	SIG( "glTexCoord4s" );
	glTexCoord4s( s, t, r, q );
}
static void APIENTRY logTexCoord4sv( const GLshort *v ) {
	SIG( "glTexCoord4sv" );
	glTexCoord4sv( v );
}
static void APIENTRY logTexCoordPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) {
	fprintf( sys_gl.log_fp, "glTexCoordPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
	glTexCoordPointer( size, type, stride, pointer );
}

static void APIENTRY logTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
	fprintf( sys_gl.log_fp, "glTexEnvf( 0x%x, 0x%x, %f )\n", target, pname, param );
	glTexEnvf( target, pname, param );
}

static void APIENTRY logTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) {
	SIG( "glTexEnvfv" );
	glTexEnvfv( target, pname, params );
}

static void APIENTRY logTexEnvi( GLenum target, GLenum pname, GLint param ) {
	fprintf( sys_gl.log_fp, "glTexEnvi( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
	glTexEnvi( target, pname, param );
}
static void APIENTRY logTexEnviv( GLenum target, GLenum pname, const GLint *params ) {
	SIG( "glTexEnviv" );
	glTexEnviv( target, pname, params );
}

static void APIENTRY logTexGend( GLenum coord, GLenum pname, GLdouble param ) {
	SIG( "glTexGend" );
	glTexGend( coord, pname, param );
}

static void APIENTRY logTexGendv( GLenum coord, GLenum pname, const GLdouble *params ) {
	SIG( "glTexGendv" );
	glTexGendv( coord, pname, params );
}

static void APIENTRY logTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
	SIG( "glTexGenf" );
	glTexGenf( coord, pname, param );
}
static void APIENTRY logTexGenfv( GLenum coord, GLenum pname, const GLfloat *params ) {
	SIG( "glTexGenfv" );
	glTexGenfv( coord, pname, params );
}
static void APIENTRY logTexGeni( GLenum coord, GLenum pname, GLint param ) {
	SIG( "glTexGeni" );
	glTexGeni( coord, pname, param );
}
static void APIENTRY logTexGeniv( GLenum coord, GLenum pname, const GLint *params ) {
	SIG( "glTexGeniv" );
	glTexGeniv( coord, pname, params );
}
static void APIENTRY logTexImage1D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels ) {
	SIG( "glTexImage1D" );
	glTexImage1D( target, level, internalformat, width, border, format, type, pixels );
}
static void APIENTRY logTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels ) {
	SIG( "glTexImage2D" );
	glTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

static void APIENTRY logTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
	fprintf( sys_gl.log_fp, "glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param );
	glTexParameterf( target, pname, param );
}

static void APIENTRY logTexParameterfv( GLenum target, GLenum pname, const GLfloat *params ) {
	SIG( "glTexParameterfv" );
	glTexParameterfv( target, pname, params );
}
static void APIENTRY logTexParameteri( GLenum target, GLenum pname, GLint param ) {
	fprintf( sys_gl.log_fp, "glTexParameteri( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
	glTexParameteri( target, pname, param );
}
static void APIENTRY logTexParameteriv( GLenum target, GLenum pname, const GLint *params ) {
	SIG( "glTexParameteriv" );
	glTexParameteriv( target, pname, params );
}
static void APIENTRY logTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels ) {
	SIG( "glTexSubImage1D" );
	glTexSubImage1D( target, level, xoffset, width, format, type, pixels );
}
static void APIENTRY logTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels ) {
	SIG( "glTexSubImage2D" );
	glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}
static void APIENTRY logTranslated( GLdouble x, GLdouble y, GLdouble z ) {
	SIG( "glTranslated" );
	glTranslated( x, y, z );
}

static void APIENTRY logTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
	SIG( "glTranslatef" );
	glTranslatef( x, y, z );
}

static void APIENTRY logVertex2d( GLdouble x, GLdouble y ) {
	SIG( "glVertex2d" );
	glVertex2d( x, y );
}

static void APIENTRY logVertex2dv( const GLdouble *v ) {
	SIG( "glVertex2dv" );
	glVertex2dv( v );
}
static void APIENTRY logVertex2f( GLfloat x, GLfloat y ) {
	SIG( "glVertex2f" );
	glVertex2f( x, y );
}
static void APIENTRY logVertex2fv( const GLfloat *v ) {
	SIG( "glVertex2fv" );
	glVertex2fv( v );
}
static void APIENTRY logVertex2i( GLint x, GLint y ) {
	SIG( "glVertex2i" );
	glVertex2i( x, y );
}
static void APIENTRY logVertex2iv( const GLint *v ) {
	SIG( "glVertex2iv" );
	glVertex2iv( v );
}
static void APIENTRY logVertex2s( GLshort x, GLshort y ) {
	SIG( "glVertex2s" );
	glVertex2s( x, y );
}
static void APIENTRY logVertex2sv( const GLshort *v ) {
	SIG( "glVertex2sv" );
	glVertex2sv( v );
}
static void APIENTRY logVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
	SIG( "glVertex3d" );
	glVertex3d( x, y, z );
}
static void APIENTRY logVertex3dv( const GLdouble *v ) {
	SIG( "glVertex3dv" );
	glVertex3dv( v );
}
static void APIENTRY logVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
	SIG( "glVertex3f" );
	glVertex3f( x, y, z );
}
static void APIENTRY logVertex3fv( const GLfloat *v ) {
	SIG( "glVertex3fv" );
	glVertex3fv( v );
}
static void APIENTRY logVertex3i( GLint x, GLint y, GLint z ) {
	SIG( "glVertex3i" );
	glVertex3i( x, y, z );
}
static void APIENTRY logVertex3iv( const GLint *v ) {
	SIG( "glVertex3iv" );
	glVertex3iv( v );
}
static void APIENTRY logVertex3s( GLshort x, GLshort y, GLshort z ) {
	SIG( "glVertex3s" );
	glVertex3s( x, y, z );
}
static void APIENTRY logVertex3sv( const GLshort *v ) {
	SIG( "glVertex3sv" );
	glVertex3sv( v );
}
static void APIENTRY logVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	SIG( "glVertex4d" );
	glVertex4d( x, y, z, w );
}
static void APIENTRY logVertex4dv( const GLdouble *v ) {
	SIG( "glVertex4dv" );
	glVertex4dv( v );
}
static void APIENTRY logVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	SIG( "glVertex4f" );
	glVertex4f( x, y, z, w );
}
static void APIENTRY logVertex4fv( const GLfloat *v ) {
	SIG( "glVertex4fv" );
	glVertex4fv( v );
}
static void APIENTRY logVertex4i( GLint x, GLint y, GLint z, GLint w ) {
	SIG( "glVertex4i" );
	glVertex4i( x, y, z, w );
}
static void APIENTRY logVertex4iv( const GLint *v ) {
	SIG( "glVertex4iv" );
	glVertex4iv( v );
}
static void APIENTRY logVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
	SIG( "glVertex4s" );
	glVertex4s( x, y, z, w );
}
static void APIENTRY logVertex4sv( const GLshort *v ) {
	SIG( "glVertex4sv" );
	glVertex4sv( v );
}
static void APIENTRY logVertexPointer( GLint size, GLenum type, GLsizei stride, const void *pointer ) {
	fprintf( sys_gl.log_fp, "glVertexPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
	glVertexPointer( size, type, stride, pointer );
}
static void APIENTRY logViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
	fprintf( sys_gl.log_fp, "glViewport( %d, %d, %d, %d )\n", x, y, width, height );
	glViewport( x, y, width, height );
}
#endif // MAC_QGL_LINKED

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown( void ) {
	VID_Printf( PRINT_ALL, "...shutting down QGL\n" );

#if MAC_QGL_LINKED
	qglAccum                     = NULL;
	qglAlphaFunc                 = NULL;
	qglAreTexturesResident       = NULL;
	qglArrayElement              = NULL;
	qglBegin                     = NULL;
	qglBindTexture               = NULL;
	qglBitmap                    = NULL;
	qglBlendFunc                 = NULL;
	qglCallList                  = NULL;
	qglCallLists                 = NULL;
	qglClear                     = NULL;
	qglClearAccum                = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearIndex                = NULL;
	qglClearStencil              = NULL;
	qglClipPlane                 = NULL;
	qglColor3b                   = NULL;
	qglColor3bv                  = NULL;
	qglColor3d                   = NULL;
	qglColor3dv                  = NULL;
	qglColor3f                   = NULL;
	qglColor3fv                  = NULL;
	qglColor3i                   = NULL;
	qglColor3iv                  = NULL;
	qglColor3s                   = NULL;
	qglColor3sv                  = NULL;
	qglColor3ub                  = NULL;
	qglColor3ubv                 = NULL;
	qglColor3ui                  = NULL;
	qglColor3uiv                 = NULL;
	qglColor3us                  = NULL;
	qglColor3usv                 = NULL;
	qglColor4b                   = NULL;
	qglColor4bv                  = NULL;
	qglColor4d                   = NULL;
	qglColor4dv                  = NULL;
	qglColor4f                   = NULL;
	qglColor4fv                  = NULL;
	qglColor4i                   = NULL;
	qglColor4iv                  = NULL;
	qglColor4s                   = NULL;
	qglColor4sv                  = NULL;
	qglColor4ub                  = NULL;
	qglColor4ubv                 = NULL;
	qglColor4ui                  = NULL;
	qglColor4uiv                 = NULL;
	qglColor4us                  = NULL;
	qglColor4usv                 = NULL;
	qglColorMask                 = NULL;
	qglColorMaterial             = NULL;
	qglColorPointer              = NULL;
	qglCopyPixels                = NULL;
	qglCopyTexImage1D            = NULL;
	qglCopyTexImage2D            = NULL;
	qglCopyTexSubImage1D         = NULL;
	qglCopyTexSubImage2D         = NULL;
	qglCullFace                  = NULL;
	qglDeleteLists               = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDisableClientState        = NULL;
	qglDrawArrays                = NULL;
	qglDrawBuffer                = NULL;
	qglDrawElements              = NULL;
	qglDrawPixels                = NULL;
	qglEdgeFlag                  = NULL;
	qglEdgeFlagPointer           = NULL;
	qglEdgeFlagv                 = NULL;
	qglEnable                    = NULL;
	qglEnableClientState         = NULL;
	qglEnd                       = NULL;
	qglEndList                   = NULL;
	qglEvalCoord1d               = NULL;
	qglEvalCoord1dv              = NULL;
	qglEvalCoord1f               = NULL;
	qglEvalCoord1fv              = NULL;
	qglEvalCoord2d               = NULL;
	qglEvalCoord2dv              = NULL;
	qglEvalCoord2f               = NULL;
	qglEvalCoord2fv              = NULL;
	qglEvalMesh1                 = NULL;
	qglEvalMesh2                 = NULL;
	qglEvalPoint1                = NULL;
	qglEvalPoint2                = NULL;
	qglFeedbackBuffer            = NULL;
	qglFinish                    = NULL;
	qglFlush                     = NULL;
	qglFogf                      = NULL;
	qglFogfv                     = NULL;
	qglFogi                      = NULL;
	qglFogiv                     = NULL;
	qglFrontFace                 = NULL;
	qglFrustum                   = NULL;
	qglGenLists                  = NULL;
	qglGenTextures               = NULL;
	qglGetBooleanv               = NULL;
	qglGetClipPlane              = NULL;
	qglGetDoublev                = NULL;
	qglGetError                  = NULL;
	qglGetFloatv                 = NULL;
	qglGetIntegerv               = NULL;
	qglGetLightfv                = NULL;
	qglGetLightiv                = NULL;
	qglGetMapdv                  = NULL;
	qglGetMapfv                  = NULL;
	qglGetMapiv                  = NULL;
	qglGetMaterialfv             = NULL;
	qglGetMaterialiv             = NULL;
	qglGetPixelMapfv             = NULL;
	qglGetPixelMapuiv            = NULL;
	qglGetPixelMapusv            = NULL;
	qglGetPointerv               = NULL;
	qglGetPolygonStipple         = NULL;
	qglGetString                 = NULL;
	qglGetTexEnvfv               = NULL;
	qglGetTexEnviv               = NULL;
	qglGetTexGendv               = NULL;
	qglGetTexGenfv               = NULL;
	qglGetTexGeniv               = NULL;
	qglGetTexImage               = NULL;
	qglGetTexLevelParameterfv    = NULL;
	qglGetTexLevelParameteriv    = NULL;
	qglGetTexParameterfv         = NULL;
	qglGetTexParameteriv         = NULL;
	qglHint                      = NULL;
	qglIndexMask                 = NULL;
	qglIndexPointer              = NULL;
	qglIndexd                    = NULL;
	qglIndexdv                   = NULL;
	qglIndexf                    = NULL;
	qglIndexfv                   = NULL;
	qglIndexi                    = NULL;
	qglIndexiv                   = NULL;
	qglIndexs                    = NULL;
	qglIndexsv                   = NULL;
	qglIndexub                   = NULL;
	qglIndexubv                  = NULL;
	qglInitNames                 = NULL;
	qglInterleavedArrays         = NULL;
	qglIsEnabled                 = NULL;
	qglIsList                    = NULL;
	qglIsTexture                 = NULL;
	qglLightModelf               = NULL;
	qglLightModelfv              = NULL;
	qglLightModeli               = NULL;
	qglLightModeliv              = NULL;
	qglLightf                    = NULL;
	qglLightfv                   = NULL;
	qglLighti                    = NULL;
	qglLightiv                   = NULL;
	qglLineStipple               = NULL;
	qglLineWidth                 = NULL;
	qglListBase                  = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixd               = NULL;
	qglLoadMatrixf               = NULL;
	qglLoadName                  = NULL;
	qglLogicOp                   = NULL;
	qglMap1d                     = NULL;
	qglMap1f                     = NULL;
	qglMap2d                     = NULL;
	qglMap2f                     = NULL;
	qglMapGrid1d                 = NULL;
	qglMapGrid1f                 = NULL;
	qglMapGrid2d                 = NULL;
	qglMapGrid2f                 = NULL;
	qglMaterialf                 = NULL;
	qglMaterialfv                = NULL;
	qglMateriali                 = NULL;
	qglMaterialiv                = NULL;
	qglMatrixMode                = NULL;
	qglMultMatrixd               = NULL;
	qglMultMatrixf               = NULL;
	qglNewList                   = NULL;
	qglNormal3b                  = NULL;
	qglNormal3bv                 = NULL;
	qglNormal3d                  = NULL;
	qglNormal3dv                 = NULL;
	qglNormal3f                  = NULL;
	qglNormal3fv                 = NULL;
	qglNormal3i                  = NULL;
	qglNormal3iv                 = NULL;
	qglNormal3s                  = NULL;
	qglNormal3sv                 = NULL;
	qglNormalPointer             = NULL;
	qglOrtho                     = NULL;
	qglPassThrough               = NULL;
	qglPixelMapfv                = NULL;
	qglPixelMapuiv               = NULL;
	qglPixelMapusv               = NULL;
	qglPixelStoref               = NULL;
	qglPixelStorei               = NULL;
	qglPixelTransferf            = NULL;
	qglPixelTransferi            = NULL;
	qglPixelZoom                 = NULL;
	qglPointSize                 = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPolygonStipple            = NULL;
	qglPopAttrib                 = NULL;
	qglPopClientAttrib           = NULL;
	qglPopMatrix                 = NULL;
	qglPopName                   = NULL;
	qglPrioritizeTextures        = NULL;
	qglPushAttrib                = NULL;
	qglPushClientAttrib          = NULL;
	qglPushMatrix                = NULL;
	qglPushName                  = NULL;
	qglRasterPos2d               = NULL;
	qglRasterPos2dv              = NULL;
	qglRasterPos2f               = NULL;
	qglRasterPos2fv              = NULL;
	qglRasterPos2i               = NULL;
	qglRasterPos2iv              = NULL;
	qglRasterPos2s               = NULL;
	qglRasterPos2sv              = NULL;
	qglRasterPos3d               = NULL;
	qglRasterPos3dv              = NULL;
	qglRasterPos3f               = NULL;
	qglRasterPos3fv              = NULL;
	qglRasterPos3i               = NULL;
	qglRasterPos3iv              = NULL;
	qglRasterPos3s               = NULL;
	qglRasterPos3sv              = NULL;
	qglRasterPos4d               = NULL;
	qglRasterPos4dv              = NULL;
	qglRasterPos4f               = NULL;
	qglRasterPos4fv              = NULL;
	qglRasterPos4i               = NULL;
	qglRasterPos4iv              = NULL;
	qglRasterPos4s               = NULL;
	qglRasterPos4sv              = NULL;
	qglReadBuffer                = NULL;
	qglReadPixels                = NULL;
	qglRectd                     = NULL;
	qglRectdv                    = NULL;
	qglRectf                     = NULL;
	qglRectfv                    = NULL;
	qglRecti                     = NULL;
	qglRectiv                    = NULL;
	qglRects                     = NULL;
	qglRectsv                    = NULL;
	qglRenderMode                = NULL;
	qglRotated                   = NULL;
	qglRotatef                   = NULL;
	qglScaled                    = NULL;
	qglScalef                    = NULL;
	qglScissor                   = NULL;
	qglSelectBuffer              = NULL;
	qglShadeModel                = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoord1d                = NULL;
	qglTexCoord1dv               = NULL;
	qglTexCoord1f                = NULL;
	qglTexCoord1fv               = NULL;
	qglTexCoord1i                = NULL;
	qglTexCoord1iv               = NULL;
	qglTexCoord1s                = NULL;
	qglTexCoord1sv               = NULL;
	qglTexCoord2d                = NULL;
	qglTexCoord2dv               = NULL;
	qglTexCoord2f                = NULL;
	qglTexCoord2fv               = NULL;
	qglTexCoord2i                = NULL;
	qglTexCoord2iv               = NULL;
	qglTexCoord2s                = NULL;
	qglTexCoord2sv               = NULL;
	qglTexCoord3d                = NULL;
	qglTexCoord3dv               = NULL;
	qglTexCoord3f                = NULL;
	qglTexCoord3fv               = NULL;
	qglTexCoord3i                = NULL;
	qglTexCoord3iv               = NULL;
	qglTexCoord3s                = NULL;
	qglTexCoord3sv               = NULL;
	qglTexCoord4d                = NULL;
	qglTexCoord4dv               = NULL;
	qglTexCoord4f                = NULL;
	qglTexCoord4fv               = NULL;
	qglTexCoord4i                = NULL;
	qglTexCoord4iv               = NULL;
	qglTexCoord4s                = NULL;
	qglTexCoord4sv               = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexEnvfv                  = NULL;
	qglTexEnvi                   = NULL;
	qglTexEnviv                  = NULL;
	qglTexGend                   = NULL;
	qglTexGendv                  = NULL;
	qglTexGenf                   = NULL;
	qglTexGenfv                  = NULL;
	qglTexGeni                   = NULL;
	qglTexGeniv                  = NULL;
	qglTexImage1D                = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameterfv            = NULL;
	qglTexParameteri             = NULL;
	qglTexParameteriv            = NULL;
	qglTexSubImage1D             = NULL;
	qglTexSubImage2D             = NULL;
	qglTranslated                = NULL;
	qglTranslatef                = NULL;
	qglVertex2d                  = NULL;
	qglVertex2dv                 = NULL;
	qglVertex2f                  = NULL;
	qglVertex2fv                 = NULL;
	qglVertex2i                  = NULL;
	qglVertex2iv                 = NULL;
	qglVertex2s                  = NULL;
	qglVertex2sv                 = NULL;
	qglVertex3d                  = NULL;
	qglVertex3dv                 = NULL;
	qglVertex3f                  = NULL;
	qglVertex3fv                 = NULL;
	qglVertex3i                  = NULL;
	qglVertex3iv                 = NULL;
	qglVertex3s                  = NULL;
	qglVertex3sv                 = NULL;
	qglVertex4d                  = NULL;
	qglVertex4dv                 = NULL;
	qglVertex4f                  = NULL;
	qglVertex4fv                 = NULL;
	qglVertex4i                  = NULL;
	qglVertex4iv                 = NULL;
	qglVertex4s                  = NULL;
	qglVertex4sv                 = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;
#endif // MAC_QGL_LINKED
}

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to
** the appropriate GL stuff.  In Windows this means doing a
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
Boolean QGL_Init( void ) {
	VID_Printf( PRINT_ALL, "...initializing QGL\n" );

#if MAC_QGL_LINKED
	qglAccum                     = glAccum;
	qglAlphaFunc                 = glAlphaFunc;
	qglAreTexturesResident       = glAreTexturesResident;
	qglArrayElement              = glArrayElement;
	qglBegin                     = glBegin;
	qglBindTexture               = glBindTexture;
	qglBitmap                    = glBitmap;
	qglBlendFunc                 = glBlendFunc;
	qglCallList                  = glCallList;
	qglCallLists                 = glCallLists;
	qglClear                     = glClear;
	qglClearAccum                = glClearAccum;
	qglClearColor                = glClearColor;
	qglClearDepth                = glClearDepth;
	qglClearIndex                = glClearIndex;
	qglClearStencil              = glClearStencil;
	qglClipPlane                 = glClipPlane;
	qglColor3b                   = glColor3b;
	qglColor3bv                  = glColor3bv;
	qglColor3d                   = glColor3d;
	qglColor3dv                  = glColor3dv;
	qglColor3f                   = glColor3f;
	qglColor3fv                  = glColor3fv;
	qglColor3i                   = glColor3i;
	qglColor3iv                  = glColor3iv;
	qglColor3s                   = glColor3s;
	qglColor3sv                  = glColor3sv;
	qglColor3ub                  = glColor3ub;
	qglColor3ubv                 = glColor3ubv;
	qglColor3ui                  = glColor3ui;
	qglColor3uiv                 = glColor3uiv;
	qglColor3us                  = glColor3us;
	qglColor3usv                 = glColor3usv;
	qglColor4b                   = glColor4b;
	qglColor4bv                  = glColor4bv;
	qglColor4d                   = glColor4d;
	qglColor4dv                  = glColor4dv;
	qglColor4f                   = glColor4f;
	qglColor4fv                  = glColor4fv;
	qglColor4i                   = glColor4i;
	qglColor4iv                  = glColor4iv;
	qglColor4s                   = glColor4s;
	qglColor4sv                  = glColor4sv;
	qglColor4ub                  = glColor4ub;
	qglColor4ubv                 = glColor4ubv;
	qglColor4ui                  = glColor4ui;
	qglColor4uiv                 = glColor4uiv;
	qglColor4us                  = glColor4us;
	qglColor4usv                 = glColor4usv;
	qglColorMask                 = glColorMask;
	qglColorMaterial             = glColorMaterial;
	qglColorPointer              = glColorPointer;
	qglCopyPixels                = glCopyPixels;
	qglCopyTexImage1D            = glCopyTexImage1D;
	qglCopyTexImage2D            = glCopyTexImage2D;
	qglCopyTexSubImage1D         = glCopyTexSubImage1D;
	qglCopyTexSubImage2D         = glCopyTexSubImage2D;
	qglCullFace                  = glCullFace;
	qglDeleteLists               = glDeleteLists ;
	qglDeleteTextures            = glDeleteTextures ;
	qglDepthFunc                 = glDepthFunc ;
	qglDepthMask                 = glDepthMask ;
	qglDepthRange                = glDepthRange ;
	qglDisable                   = glDisable ;
	qglDisableClientState        = glDisableClientState ;
	qglDrawArrays                = glDrawArrays ;
	qglDrawBuffer                = glDrawBuffer ;
	qglDrawElements              = glDrawElements ;
	qglDrawPixels                = glDrawPixels ;
	qglEdgeFlag                  = glEdgeFlag ;
	qglEdgeFlagPointer           = glEdgeFlagPointer ;
	qglEdgeFlagv                 = glEdgeFlagv ;
	qglEnable                    =  glEnable                    ;
	qglEnableClientState         =  glEnableClientState         ;
	qglEnd                       =  glEnd                       ;
	qglEndList                   =  glEndList                   ;
	qglEvalCoord1d               =  glEvalCoord1d                ;
	qglEvalCoord1dv              =  glEvalCoord1dv              ;
	qglEvalCoord1f               =  glEvalCoord1f               ;
	qglEvalCoord1fv              =  glEvalCoord1fv              ;
	qglEvalCoord2d               =  glEvalCoord2d               ;
	qglEvalCoord2dv              =  glEvalCoord2dv              ;
	qglEvalCoord2f               =  glEvalCoord2f               ;
	qglEvalCoord2fv              =  glEvalCoord2fv              ;
	qglEvalMesh1                 =  glEvalMesh1                 ;
	qglEvalMesh2                 =  glEvalMesh2                 ;
	qglEvalPoint1                =  glEvalPoint1                ;
	qglEvalPoint2                =  glEvalPoint2                ;
	qglFeedbackBuffer            =  glFeedbackBuffer            ;
	qglFinish                    =  glFinish                    ;
	qglFlush                     =  glFlush                     ;
	qglFogf                      =  glFogf                      ;
	qglFogfv                     =  glFogfv                     ;
	qglFogi                      =  glFogi                      ;
	qglFogiv                     =  glFogiv                     ;
	qglFrontFace                 =  glFrontFace                 ;
	qglFrustum                   =  glFrustum                   ;
	qglGenLists                  =  glGenLists                  ;
	qglGenTextures               =  glGenTextures               ;
	qglGetBooleanv               =  glGetBooleanv               ;
	qglGetClipPlane              =  glGetClipPlane              ;
	qglGetDoublev                =  glGetDoublev                ;
	qglGetError                  =  glGetError                  ;
	qglGetFloatv                 =  glGetFloatv                 ;
	qglGetIntegerv               =  glGetIntegerv               ;
	qglGetLightfv                =  glGetLightfv                ;
	qglGetLightiv                =  glGetLightiv                ;
	qglGetMapdv                  =  glGetMapdv                  ;
	qglGetMapfv                  =  glGetMapfv                  ;
	qglGetMapiv                  =  glGetMapiv                  ;
	qglGetMaterialfv             =  glGetMaterialfv             ;
	qglGetMaterialiv             =  glGetMaterialiv             ;
	qglGetPixelMapfv             =  glGetPixelMapfv             ;
	qglGetPixelMapuiv            =  glGetPixelMapuiv            ;
	qglGetPixelMapusv            =  glGetPixelMapusv            ;
	qglGetPointerv               =  glGetPointerv               ;
	qglGetPolygonStipple         =  glGetPolygonStipple         ;
	qglGetString                 =  glGetString                 ;
	qglGetTexEnvfv               =  glGetTexEnvfv               ;
	qglGetTexEnviv               =  glGetTexEnviv               ;
	qglGetTexGendv               =  glGetTexGendv               ;
	qglGetTexGenfv               =  glGetTexGenfv               ;
	qglGetTexGeniv               =  glGetTexGeniv               ;
	qglGetTexImage               =  glGetTexImage               ;
	qglGetTexLevelParameterfv    =  glGetTexLevelParameterfv    ;
	qglGetTexLevelParameteriv    =  glGetTexLevelParameteriv    ;
	qglGetTexParameterfv         =  glGetTexParameterfv         ;
	qglGetTexParameteriv         =  glGetTexParameteriv         ;
	qglHint                      =  glHint                      ;
	qglIndexMask                 =  glIndexMask                 ;
	qglIndexPointer              =  glIndexPointer              ;
	qglIndexd                    =  glIndexd                    ;
	qglIndexdv                   =  glIndexdv                   ;
	qglIndexf                    =  glIndexf                    ;
	qglIndexfv                   =  glIndexfv                   ;
	qglIndexi                    =  glIndexi                    ;
	qglIndexiv                   =  glIndexiv                   ;
	qglIndexs                    =  glIndexs                    ;
	qglIndexsv                   =  glIndexsv                   ;
	qglIndexub                   =  glIndexub                   ;
	qglIndexubv                  =  glIndexubv                  ;
	qglInitNames                 =  glInitNames                 ;
	qglInterleavedArrays         =  glInterleavedArrays         ;
	qglIsEnabled                 =  glIsEnabled                 ;
	qglIsList                    =  glIsList                    ;
	qglIsTexture                 =  glIsTexture                 ;
	qglLightModelf               =  glLightModelf               ;
	qglLightModelfv              =  glLightModelfv              ;
	qglLightModeli               =  glLightModeli               ;
	qglLightModeliv              =  glLightModeliv              ;
	qglLightf                    =  glLightf                    ;
	qglLightfv                   =  glLightfv                   ;
	qglLighti                    =  glLighti                    ;
	qglLightiv                   =  glLightiv                   ;
	qglLineStipple               =  glLineStipple               ;
	qglLineWidth                 =  glLineWidth                 ;
	qglListBase                  =  glListBase                  ;
	qglLoadIdentity              =  glLoadIdentity              ;
	qglLoadMatrixd               =  glLoadMatrixd               ;
	qglLoadMatrixf               =  glLoadMatrixf               ;
	qglLoadName                  =  glLoadName                  ;
	qglLogicOp                   =  glLogicOp                   ;
	qglMap1d                     =  glMap1d                     ;
	qglMap1f                     =  glMap1f                     ;
	qglMap2d                     =  glMap2d                     ;
	qglMap2f                     =  glMap2f                     ;
	qglMapGrid1d                 =  glMapGrid1d                 ;
	qglMapGrid1f                 =  glMapGrid1f                 ;
	qglMapGrid2d                 =  glMapGrid2d                 ;
	qglMapGrid2f                 =  glMapGrid2f                 ;
	qglMaterialf                 =  glMaterialf                 ;
	qglMaterialfv                =  glMaterialfv                ;
	qglMateriali                 =  glMateriali                 ;
	qglMaterialiv                =  glMaterialiv                ;
	qglMatrixMode                =  glMatrixMode                ;
	qglMultMatrixd               =  glMultMatrixd               ;
	qglMultMatrixf               =  glMultMatrixf               ;
	qglNewList                   =  glNewList                   ;
	qglNormal3b                  =  glNormal3b                  ;
	qglNormal3bv                 =  glNormal3bv                 ;
	qglNormal3d                  =  glNormal3d                  ;
	qglNormal3dv                 =  glNormal3dv                 ;
	qglNormal3f                  =  glNormal3f                  ;
	qglNormal3fv                 =  glNormal3fv                 ;
	qglNormal3i                  =  glNormal3i                  ;
	qglNormal3iv                 =  glNormal3iv                 ;
	qglNormal3s                  =  glNormal3s                  ;
	qglNormal3sv                 =  glNormal3sv                 ;
	qglNormalPointer             =  glNormalPointer             ;
	qglOrtho                     =  glOrtho                     ;
	qglPassThrough               =  glPassThrough               ;
	qglPixelMapfv                =  glPixelMapfv                ;
	qglPixelMapuiv               =  glPixelMapuiv               ;
	qglPixelMapusv               =  glPixelMapusv               ;
	qglPixelStoref               =  glPixelStoref               ;
	qglPixelStorei               =  glPixelStorei               ;
	qglPixelTransferf            =  glPixelTransferf            ;
	qglPixelTransferi            =  glPixelTransferi            ;
	qglPixelZoom                 =  glPixelZoom                 ;
	qglPointSize                 =  glPointSize                 ;
	qglPolygonMode               =  glPolygonMode               ;
	qglPolygonOffset             =  glPolygonOffset             ;
	qglPolygonStipple            =  glPolygonStipple            ;
	qglPopAttrib                 =  glPopAttrib                 ;
	qglPopClientAttrib           =  glPopClientAttrib           ;
	qglPopMatrix                 =  glPopMatrix                 ;
	qglPopName                   =  glPopName                   ;
	qglPrioritizeTextures        =  glPrioritizeTextures        ;
	qglPushAttrib                =  glPushAttrib                ;
	qglPushClientAttrib          =  glPushClientAttrib          ;
	qglPushMatrix                =  glPushMatrix                ;
	qglPushName                  =  glPushName                  ;
	qglRasterPos2d               =  glRasterPos2d               ;
	qglRasterPos2dv              =  glRasterPos2dv              ;
	qglRasterPos2f               =  glRasterPos2f               ;
	qglRasterPos2fv              =  glRasterPos2fv              ;
	qglRasterPos2i               =  glRasterPos2i               ;
	qglRasterPos2iv              =  glRasterPos2iv              ;
	qglRasterPos2s               =  glRasterPos2s               ;
	qglRasterPos2sv              =  glRasterPos2sv              ;
	qglRasterPos3d               =  glRasterPos3d               ;
	qglRasterPos3dv              =  glRasterPos3dv              ;
	qglRasterPos3f               =  glRasterPos3f               ;
	qglRasterPos3fv              =  glRasterPos3fv              ;
	qglRasterPos3i               =  glRasterPos3i               ;
	qglRasterPos3iv              =  glRasterPos3iv              ;
	qglRasterPos3s               =  glRasterPos3s               ;
	qglRasterPos3sv              =  glRasterPos3sv              ;
	qglRasterPos4d               =  glRasterPos4d               ;
	qglRasterPos4dv              =  glRasterPos4dv              ;
	qglRasterPos4f               =  glRasterPos4f               ;
	qglRasterPos4fv              =  glRasterPos4fv              ;
	qglRasterPos4i               =  glRasterPos4i               ;
	qglRasterPos4iv              =  glRasterPos4iv              ;
	qglRasterPos4s               =  glRasterPos4s               ;
	qglRasterPos4sv              =  glRasterPos4sv              ;
	qglReadBuffer                =  glReadBuffer                ;
	qglReadPixels                =  glReadPixels                ;
	qglRectd                     =  glRectd                     ;
	qglRectdv                    =  glRectdv                    ;
	qglRectf                     =  glRectf                     ;
	qglRectfv                    =  glRectfv                    ;
	qglRecti                     =  glRecti                     ;
	qglRectiv                    =  glRectiv                    ;
	qglRects                     =  glRects                     ;
	qglRectsv                    =  glRectsv                    ;
	qglRenderMode                =  glRenderMode                ;
	qglRotated                   =  glRotated                   ;
	qglRotatef                   =  glRotatef                   ;
	qglScaled                    =  glScaled                    ;
	qglScalef                    =  glScalef                    ;
	qglScissor                   =  glScissor                   ;
	qglSelectBuffer              =  glSelectBuffer              ;
	qglShadeModel                =  glShadeModel                ;
	qglStencilFunc               =  glStencilFunc               ;
	qglStencilMask               =  glStencilMask               ;
	qglStencilOp                 =  glStencilOp                 ;
	qglTexCoord1d                =  glTexCoord1d                ;
	qglTexCoord1dv               =  glTexCoord1dv               ;
	qglTexCoord1f                =  glTexCoord1f                ;
	qglTexCoord1fv               =  glTexCoord1fv               ;
	qglTexCoord1i                =  glTexCoord1i                ;
	qglTexCoord1iv               =  glTexCoord1iv               ;
	qglTexCoord1s                =  glTexCoord1s                ;
	qglTexCoord1sv               =  glTexCoord1sv               ;
	qglTexCoord2d                =  glTexCoord2d                ;
	qglTexCoord2dv               =  glTexCoord2dv               ;
	qglTexCoord2f                =  glTexCoord2f                ;
	qglTexCoord2fv               =  glTexCoord2fv               ;
	qglTexCoord2i                =  glTexCoord2i                ;
	qglTexCoord2iv               =  glTexCoord2iv               ;
	qglTexCoord2s                =  glTexCoord2s                ;
	qglTexCoord2sv               =  glTexCoord2sv               ;
	qglTexCoord3d                =  glTexCoord3d                ;
	qglTexCoord3dv               =  glTexCoord3dv               ;
	qglTexCoord3f                =  glTexCoord3f                ;
	qglTexCoord3fv               =  glTexCoord3fv               ;
	qglTexCoord3i                =  glTexCoord3i                ;
	qglTexCoord3iv               =  glTexCoord3iv               ;
	qglTexCoord3s                =  glTexCoord3s                ;
	qglTexCoord3sv               =  glTexCoord3sv               ;
	qglTexCoord4d                =  glTexCoord4d                ;
	qglTexCoord4dv               =  glTexCoord4dv               ;
	qglTexCoord4f                =  glTexCoord4f                ;
	qglTexCoord4fv               =  glTexCoord4fv               ;
	qglTexCoord4i                =  glTexCoord4i                ;
	qglTexCoord4iv               =  glTexCoord4iv               ;
	qglTexCoord4s                =  glTexCoord4s                ;
	qglTexCoord4sv               =  glTexCoord4sv               ;
	qglTexCoordPointer           =  glTexCoordPointer           ;
	qglTexEnvf                   =  glTexEnvf                   ;
	qglTexEnvfv                  =  glTexEnvfv                  ;
	qglTexEnvi                   =  glTexEnvi                   ;
	qglTexEnviv                  =  glTexEnviv                  ;
	qglTexGend                   =  glTexGend                   ;
	qglTexGendv                  =  glTexGendv                  ;
	qglTexGenf                   =  glTexGenf                   ;
	qglTexGenfv                  =  glTexGenfv                  ;
	qglTexGeni                   =  glTexGeni                   ;
	qglTexGeniv                  =  glTexGeniv                  ;
	qglTexImage1D                =  glTexImage1D                ;
	qglTexImage2D                =  glTexImage2D                ;
	qglTexParameterf             =  glTexParameterf             ;
	qglTexParameterfv            =  glTexParameterfv            ;
	qglTexParameteri             =  glTexParameteri             ;
	qglTexParameteriv            =  glTexParameteriv            ;
	qglTexSubImage1D             =  glTexSubImage1D             ;
	qglTexSubImage2D             =  glTexSubImage2D             ;
	qglTranslated                =  glTranslated                ;
	qglTranslatef                =  glTranslatef                ;
	qglVertex2d                  =  glVertex2d                  ;
	qglVertex2dv                 =  glVertex2dv                 ;
	qglVertex2f                  =  glVertex2f                  ;
	qglVertex2fv                 =  glVertex2fv                 ;
	qglVertex2i                  =  glVertex2i                  ;
	qglVertex2iv                 =  glVertex2iv                 ;
	qglVertex2s                  =  glVertex2s                  ;
	qglVertex2sv                 =  glVertex2sv                 ;
	qglVertex3d                  =  glVertex3d                  ;
	qglVertex3dv                 =  glVertex3dv                 ;
	qglVertex3f                  =  glVertex3f                  ;
	qglVertex3fv                 =  glVertex3fv                 ;
	qglVertex3i                  =  glVertex3i                  ;
	qglVertex3iv                 =  glVertex3iv                 ;
	qglVertex3s                  =  glVertex3s                  ;
	qglVertex3sv                 =  glVertex3sv                 ;
	qglVertex4d                  =  glVertex4d                  ;
	qglVertex4dv                 =  glVertex4dv                 ;
	qglVertex4f                  =  glVertex4f                  ;
	qglVertex4fv                 =  glVertex4fv                 ;
	qglVertex4i                  =  glVertex4i                  ;
	qglVertex4iv                 =  glVertex4iv                 ;
	qglVertex4s                  =  glVertex4s                  ;
	qglVertex4sv                 =  glVertex4sv                 ;
	qglVertexPointer             =  glVertexPointer             ;
	qglViewport                  =  glViewport                  ;

	qglActiveTextureARB = 0;
	qglClientActiveTextureARB = 0;
	qglMultiTexCoord2fARB = 0;
	qglLockArraysEXT = 0;
	qglUnlockArraysEXT = 0;
	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;

#ifdef _NPATCH
	qglPNTrianglesiATI = NULL;
#endif // _NPATCH

	// check logging
	QGL_EnableLogging( (qboolean)r_logFile->integer );
#endif // MAC_QGL_LINKED

	return true;
}

void QGL_EnableLogging( qboolean enable ) {
#if MAC_QGL_LINKED
	static qboolean isEnabled;

	// return if we're already active
	if ( isEnabled && enable ) {
		// decrement log counter and stop if it has reached 0
		ri.Cvar_Set( "r_logFile", va( "%d", r_logFile->integer - 1 ) );
		if ( r_logFile->integer ) {
			return;
		}
		enable = qfalse;
	}

	// return if we're already disabled
	if ( !enable && !isEnabled ) {
		return;
	}

	isEnabled = enable;

	if ( enable ) {
		if ( !sys_gl.log_fp ) {
			struct tm *newtime;
			time_t aclock;
			char buffer[1024];
			cvar_t  *basedir;

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );

			basedir = ri.Cvar_Get( "fs_basepath", "", 0 );
			Com_sprintf( buffer, sizeof( buffer ), "%s:gl.log", basedir->string );
			sys_gl.log_fp = fopen( buffer, "wt" );

			fprintf( sys_gl.log_fp, "%s\n", asctime( newtime ) );
		}

		qglAccum                     = logAccum;
		qglAlphaFunc                 = logAlphaFunc;
		qglAreTexturesResident       = logAreTexturesResident;
		qglArrayElement              = logArrayElement;
		qglBegin                     = logBegin;
		qglBindTexture               = logBindTexture;
		qglBitmap                    = logBitmap;
		qglBlendFunc                 = logBlendFunc;
		qglCallList                  = logCallList;
		qglCallLists                 = logCallLists;
		qglClear                     = logClear;
		qglClearAccum                = logClearAccum;
		qglClearColor                = logClearColor;
		qglClearDepth                = logClearDepth;
		qglClearIndex                = logClearIndex;
		qglClearStencil              = logClearStencil;
		qglClipPlane                 = logClipPlane;
		qglColor3b                   = logColor3b;
		qglColor3bv                  = logColor3bv;
		qglColor3d                   = logColor3d;
		qglColor3dv                  = logColor3dv;
		qglColor3f                   = logColor3f;
		qglColor3fv                  = logColor3fv;
		qglColor3i                   = logColor3i;
		qglColor3iv                  = logColor3iv;
		qglColor3s                   = logColor3s;
		qglColor3sv                  = logColor3sv;
		qglColor3ub                  = logColor3ub;
		qglColor3ubv                 = logColor3ubv;
		qglColor3ui                  = logColor3ui;
		qglColor3uiv                 = logColor3uiv;
		qglColor3us                  = logColor3us;
		qglColor3usv                 = logColor3usv;
		qglColor4b                   = logColor4b;
		qglColor4bv                  = logColor4bv;
		qglColor4d                   = logColor4d;
		qglColor4dv                  = logColor4dv;
		qglColor4f                   = logColor4f;
		qglColor4fv                  = logColor4fv;
		qglColor4i                   = logColor4i;
		qglColor4iv                  = logColor4iv;
		qglColor4s                   = logColor4s;
		qglColor4sv                  = logColor4sv;
		qglColor4ub                  = logColor4ub;
		qglColor4ubv                 = logColor4ubv;
		qglColor4ui                  = logColor4ui;
		qglColor4uiv                 = logColor4uiv;
		qglColor4us                  = logColor4us;
		qglColor4usv                 = logColor4usv;
		qglColorMask                 = logColorMask;
		qglColorMaterial             = logColorMaterial;
		qglColorPointer              = logColorPointer;
		qglCopyPixels                = logCopyPixels;
		qglCopyTexImage1D            = logCopyTexImage1D;
		qglCopyTexImage2D            = logCopyTexImage2D;
		qglCopyTexSubImage1D         = logCopyTexSubImage1D;
		qglCopyTexSubImage2D         = logCopyTexSubImage2D;
		qglCullFace                  = logCullFace;
		qglDeleteLists               = logDeleteLists ;
		qglDeleteTextures            = logDeleteTextures ;
		qglDepthFunc                 = logDepthFunc ;
		qglDepthMask                 = logDepthMask ;
		qglDepthRange                = logDepthRange ;
		qglDisable                   = logDisable ;
		qglDisableClientState        = logDisableClientState ;
		qglDrawArrays                = logDrawArrays ;
		qglDrawBuffer                = logDrawBuffer ;
		qglDrawElements              = logDrawElements ;
		qglDrawPixels                = logDrawPixels ;
		qglEdgeFlag                  = logEdgeFlag ;
		qglEdgeFlagPointer           = logEdgeFlagPointer ;
		qglEdgeFlagv                 = logEdgeFlagv ;
		qglEnable                    =  logEnable                    ;
		qglEnableClientState         =  logEnableClientState         ;
		qglEnd                       =  logEnd                       ;
		qglEndList                   =  logEndList                   ;
		qglEvalCoord1d               =  logEvalCoord1d               ;
		qglEvalCoord1dv              =  logEvalCoord1dv              ;
		qglEvalCoord1f               =  logEvalCoord1f               ;
		qglEvalCoord1fv              =  logEvalCoord1fv              ;
		qglEvalCoord2d               =  logEvalCoord2d               ;
		qglEvalCoord2dv              =  logEvalCoord2dv              ;
		qglEvalCoord2f               =  logEvalCoord2f               ;
		qglEvalCoord2fv              =  logEvalCoord2fv              ;
		qglEvalMesh1                 =  logEvalMesh1                 ;
		qglEvalMesh2                 =  logEvalMesh2                 ;
		qglEvalPoint1                =  logEvalPoint1                ;
		qglEvalPoint2                =  logEvalPoint2                ;
		qglFeedbackBuffer            =  logFeedbackBuffer            ;
		qglFinish                    =  logFinish                    ;
		qglFlush                     =  logFlush                     ;
		qglFogf                      =  logFogf                      ;
		qglFogfv                     =  logFogfv                     ;
		qglFogi                      =  logFogi                      ;
		qglFogiv                     =  logFogiv                     ;
		qglFrontFace                 =  logFrontFace                 ;
		qglFrustum                   =  logFrustum                   ;
		qglGenLists                  =  logGenLists                  ;
		qglGenTextures               =  logGenTextures               ;
		qglGetBooleanv               =  logGetBooleanv               ;
		qglGetClipPlane              =  logGetClipPlane              ;
		qglGetDoublev                =  logGetDoublev                ;
		qglGetError                  =  logGetError                  ;
		qglGetFloatv                 =  logGetFloatv                 ;
		qglGetIntegerv               =  logGetIntegerv               ;
		qglGetLightfv                =  logGetLightfv                ;
		qglGetLightiv                =  logGetLightiv                ;
		qglGetMapdv                  =  logGetMapdv                  ;
		qglGetMapfv                  =  logGetMapfv                  ;
		qglGetMapiv                  =  logGetMapiv                  ;
		qglGetMaterialfv             =  logGetMaterialfv             ;
		qglGetMaterialiv             =  logGetMaterialiv             ;
		qglGetPixelMapfv             =  logGetPixelMapfv             ;
		qglGetPixelMapuiv            =  logGetPixelMapuiv            ;
		qglGetPixelMapusv            =  logGetPixelMapusv            ;
		qglGetPointerv               =  logGetPointerv               ;
		qglGetPolygonStipple         =  logGetPolygonStipple         ;
		qglGetString                 =  logGetString                 ;
		qglGetTexEnvfv               =  logGetTexEnvfv               ;
		qglGetTexEnviv               =  logGetTexEnviv               ;
		qglGetTexGendv               =  logGetTexGendv               ;
		qglGetTexGenfv               =  logGetTexGenfv               ;
		qglGetTexGeniv               =  logGetTexGeniv               ;
		qglGetTexImage               =  logGetTexImage               ;
		qglGetTexLevelParameterfv    =  logGetTexLevelParameterfv    ;
		qglGetTexLevelParameteriv    =  logGetTexLevelParameteriv    ;
		qglGetTexParameterfv         =  logGetTexParameterfv         ;
		qglGetTexParameteriv         =  logGetTexParameteriv         ;
		qglHint                      =  logHint                      ;
		qglIndexMask                 =  logIndexMask                 ;
		qglIndexPointer              =  logIndexPointer              ;
		qglIndexd                    =  logIndexd                    ;
		qglIndexdv                   =  logIndexdv                   ;
		qglIndexf                    =  logIndexf                    ;
		qglIndexfv                   =  logIndexfv                   ;
		qglIndexi                    =  logIndexi                    ;
		qglIndexiv                   =  logIndexiv                   ;
		qglIndexs                    =  logIndexs                    ;
		qglIndexsv                   =  logIndexsv                   ;
		qglIndexub                   =  logIndexub                   ;
		qglIndexubv                  =  logIndexubv                  ;
		qglInitNames                 =  logInitNames                 ;
		qglInterleavedArrays         =  logInterleavedArrays         ;
		qglIsEnabled                 =  logIsEnabled                 ;
		qglIsList                    =  logIsList                    ;
		qglIsTexture                 =  logIsTexture                 ;
		qglLightModelf               =  logLightModelf               ;
		qglLightModelfv              =  logLightModelfv              ;
		qglLightModeli               =  logLightModeli               ;
		qglLightModeliv              =  logLightModeliv              ;
		qglLightf                    =  logLightf                    ;
		qglLightfv                   =  logLightfv                   ;
		qglLighti                    =  logLighti                    ;
		qglLightiv                   =  logLightiv                   ;
		qglLineStipple               =  logLineStipple               ;
		qglLineWidth                 =  logLineWidth                 ;
		qglListBase                  =  logListBase                  ;
		qglLoadIdentity              =  logLoadIdentity              ;
		qglLoadMatrixd               =  logLoadMatrixd               ;
		qglLoadMatrixf               =  logLoadMatrixf               ;
		qglLoadName                  =  logLoadName                  ;
		qglLogicOp                   =  logLogicOp                   ;
		qglMap1d                     =  logMap1d                     ;
		qglMap1f                     =  logMap1f                     ;
		qglMap2d                     =  logMap2d                     ;
		qglMap2f                     =  logMap2f                     ;
		qglMapGrid1d                 =  logMapGrid1d                 ;
		qglMapGrid1f                 =  logMapGrid1f                 ;
		qglMapGrid2d                 =  logMapGrid2d                 ;
		qglMapGrid2f                 =  logMapGrid2f                 ;
		qglMaterialf                 =  logMaterialf                 ;
		qglMaterialfv                =  logMaterialfv                ;
		qglMateriali                 =  logMateriali                 ;
		qglMaterialiv                =  logMaterialiv                ;
		qglMatrixMode                =  logMatrixMode                ;
		qglMultMatrixd               =  logMultMatrixd               ;
		qglMultMatrixf               =  logMultMatrixf               ;
		qglNewList                   =  logNewList                   ;
		qglNormal3b                  =  logNormal3b                  ;
		qglNormal3bv                 =  logNormal3bv                 ;
		qglNormal3d                  =  logNormal3d                  ;
		qglNormal3dv                 =  logNormal3dv                 ;
		qglNormal3f                  =  logNormal3f                  ;
		qglNormal3fv                 =  logNormal3fv                 ;
		qglNormal3i                  =  logNormal3i                  ;
		qglNormal3iv                 =  logNormal3iv                 ;
		qglNormal3s                  =  logNormal3s                  ;
		qglNormal3sv                 =  logNormal3sv                 ;
		qglNormalPointer             =  logNormalPointer             ;
		qglOrtho                     =  logOrtho                     ;
		qglPassThrough               =  logPassThrough               ;
		qglPixelMapfv                =  logPixelMapfv                ;
		qglPixelMapuiv               =  logPixelMapuiv               ;
		qglPixelMapusv               =  logPixelMapusv               ;
		qglPixelStoref               =  logPixelStoref               ;
		qglPixelStorei               =  logPixelStorei               ;
		qglPixelTransferf            =  logPixelTransferf            ;
		qglPixelTransferi            =  logPixelTransferi            ;
		qglPixelZoom                 =  logPixelZoom                 ;
		qglPointSize                 =  logPointSize                 ;
		qglPolygonMode               =  logPolygonMode               ;
		qglPolygonOffset             =  logPolygonOffset             ;
		qglPolygonStipple            =  logPolygonStipple            ;
		qglPopAttrib                 =  logPopAttrib                 ;
		qglPopClientAttrib           =  logPopClientAttrib           ;
		qglPopMatrix                 =  logPopMatrix                 ;
		qglPopName                   =  logPopName                   ;
		qglPrioritizeTextures        =  logPrioritizeTextures        ;
		qglPushAttrib                =  logPushAttrib                ;
		qglPushClientAttrib          =  logPushClientAttrib          ;
		qglPushMatrix                =  logPushMatrix                ;
		qglPushName                  =  logPushName                  ;
		qglRasterPos2d               =  logRasterPos2d               ;
		qglRasterPos2dv              =  logRasterPos2dv              ;
		qglRasterPos2f               =  logRasterPos2f               ;
		qglRasterPos2fv              =  logRasterPos2fv              ;
		qglRasterPos2i               =  logRasterPos2i               ;
		qglRasterPos2iv              =  logRasterPos2iv              ;
		qglRasterPos2s               =  logRasterPos2s               ;
		qglRasterPos2sv              =  logRasterPos2sv              ;
		qglRasterPos3d               =  logRasterPos3d               ;
		qglRasterPos3dv              =  logRasterPos3dv              ;
		qglRasterPos3f               =  logRasterPos3f               ;
		qglRasterPos3fv              =  logRasterPos3fv              ;
		qglRasterPos3i               =  logRasterPos3i               ;
		qglRasterPos3iv              =  logRasterPos3iv              ;
		qglRasterPos3s               =  logRasterPos3s               ;
		qglRasterPos3sv              =  logRasterPos3sv              ;
		qglRasterPos4d               =  logRasterPos4d               ;
		qglRasterPos4dv              =  logRasterPos4dv              ;
		qglRasterPos4f               =  logRasterPos4f               ;
		qglRasterPos4fv              =  logRasterPos4fv              ;
		qglRasterPos4i               =  logRasterPos4i               ;
		qglRasterPos4iv              =  logRasterPos4iv              ;
		qglRasterPos4s               =  logRasterPos4s               ;
		qglRasterPos4sv              =  logRasterPos4sv              ;
		qglReadBuffer                =  logReadBuffer                ;
		qglReadPixels                =  logReadPixels                ;
		qglRectd                     =  logRectd                     ;
		qglRectdv                    =  logRectdv                    ;
		qglRectf                     =  logRectf                     ;
		qglRectfv                    =  logRectfv                    ;
		qglRecti                     =  logRecti                     ;
		qglRectiv                    =  logRectiv                    ;
		qglRects                     =  logRects                     ;
		qglRectsv                    =  logRectsv                    ;
		qglRenderMode                =  logRenderMode                ;
		qglRotated                   =  logRotated                   ;
		qglRotatef                   =  logRotatef                   ;
		qglScaled                    =  logScaled                    ;
		qglScalef                    =  logScalef                    ;
		qglScissor                   =  logScissor                   ;
		qglSelectBuffer              =  logSelectBuffer              ;
		qglShadeModel                =  logShadeModel                ;
		qglStencilFunc               =  logStencilFunc               ;
		qglStencilMask               =  logStencilMask               ;
		qglStencilOp                 =  logStencilOp                 ;
		qglTexCoord1d                =  logTexCoord1d                ;
		qglTexCoord1dv               =  logTexCoord1dv               ;
		qglTexCoord1f                =  logTexCoord1f                ;
		qglTexCoord1fv               =  logTexCoord1fv               ;
		qglTexCoord1i                =  logTexCoord1i                ;
		qglTexCoord1iv               =  logTexCoord1iv               ;
		qglTexCoord1s                =  logTexCoord1s                ;
		qglTexCoord1sv               =  logTexCoord1sv               ;
		qglTexCoord2d                =  logTexCoord2d                ;
		qglTexCoord2dv               =  logTexCoord2dv               ;
		qglTexCoord2f                =  logTexCoord2f                ;
		qglTexCoord2fv               =  logTexCoord2fv               ;
		qglTexCoord2i                =  logTexCoord2i                ;
		qglTexCoord2iv               =  logTexCoord2iv               ;
		qglTexCoord2s                =  logTexCoord2s                ;
		qglTexCoord2sv               =  logTexCoord2sv               ;
		qglTexCoord3d                =  logTexCoord3d                ;
		qglTexCoord3dv               =  logTexCoord3dv               ;
		qglTexCoord3f                =  logTexCoord3f                ;
		qglTexCoord3fv               =  logTexCoord3fv               ;
		qglTexCoord3i                =  logTexCoord3i                ;
		qglTexCoord3iv               =  logTexCoord3iv               ;
		qglTexCoord3s                =  logTexCoord3s                ;
		qglTexCoord3sv               =  logTexCoord3sv               ;
		qglTexCoord4d                =  logTexCoord4d                ;
		qglTexCoord4dv               =  logTexCoord4dv               ;
		qglTexCoord4f                =  logTexCoord4f                ;
		qglTexCoord4fv               =  logTexCoord4fv               ;
		qglTexCoord4i                =  logTexCoord4i                ;
		qglTexCoord4iv               =  logTexCoord4iv               ;
		qglTexCoord4s                =  logTexCoord4s                ;
		qglTexCoord4sv               =  logTexCoord4sv               ;
		qglTexCoordPointer           =  logTexCoordPointer           ;
		qglTexEnvf                   =  logTexEnvf                   ;
		qglTexEnvfv                  =  logTexEnvfv                  ;
		qglTexEnvi                   =  logTexEnvi                   ;
		qglTexEnviv                  =  logTexEnviv                  ;
		qglTexGend                   =  logTexGend                   ;
		qglTexGendv                  =  logTexGendv                  ;
		qglTexGenf                   =  logTexGenf                   ;
		qglTexGenfv                  =  logTexGenfv                  ;
		qglTexGeni                   =  logTexGeni                   ;
		qglTexGeniv                  =  logTexGeniv                  ;
		qglTexImage1D                =  logTexImage1D                ;
		qglTexImage2D                =  logTexImage2D                ;
		qglTexParameterf             =  logTexParameterf             ;
		qglTexParameterfv            =  logTexParameterfv            ;
		qglTexParameteri             =  logTexParameteri             ;
		qglTexParameteriv            =  logTexParameteriv            ;
		qglTexSubImage1D             =  logTexSubImage1D             ;
		qglTexSubImage2D             =  logTexSubImage2D             ;
		qglTranslated                =  logTranslated                ;
		qglTranslatef                =  logTranslatef                ;
		qglVertex2d                  =  logVertex2d                  ;
		qglVertex2dv                 =  logVertex2dv                 ;
		qglVertex2f                  =  logVertex2f                  ;
		qglVertex2fv                 =  logVertex2fv                 ;
		qglVertex2i                  =  logVertex2i                  ;
		qglVertex2iv                 =  logVertex2iv                 ;
		qglVertex2s                  =  logVertex2s                  ;
		qglVertex2sv                 =  logVertex2sv                 ;
		qglVertex3d                  =  logVertex3d                  ;
		qglVertex3dv                 =  logVertex3dv                 ;
		qglVertex3f                  =  logVertex3f                  ;
		qglVertex3fv                 =  logVertex3fv                 ;
		qglVertex3i                  =  logVertex3i                  ;
		qglVertex3iv                 =  logVertex3iv                 ;
		qglVertex3s                  =  logVertex3s                  ;
		qglVertex3sv                 =  logVertex3sv                 ;
		qglVertex4d                  =  logVertex4d                  ;
		qglVertex4dv                 =  logVertex4dv                 ;
		qglVertex4f                  =  logVertex4f                  ;
		qglVertex4fv                 =  logVertex4fv                 ;
		qglVertex4i                  =  logVertex4i                  ;
		qglVertex4iv                 =  logVertex4iv                 ;
		qglVertex4s                  =  logVertex4s                  ;
		qglVertex4sv                 =  logVertex4sv                 ;
		qglVertexPointer             =  logVertexPointer             ;
		qglViewport                  =  logViewport                  ;
	} else
	{
		if ( sys_gl.log_fp ) {
			fprintf( sys_gl.log_fp, "*** CLOSING LOG ***\n" );
			fclose( sys_gl.log_fp );
			sys_gl.log_fp = NULL;
		}
		qglAccum                     = glAccum;
		qglAlphaFunc                 = glAlphaFunc;
		qglAreTexturesResident       = glAreTexturesResident;
		qglArrayElement              = glArrayElement;
		qglBegin                     = glBegin;
		qglBindTexture               = glBindTexture;
		qglBitmap                    = glBitmap;
		qglBlendFunc                 = glBlendFunc;
		qglCallList                  = glCallList;
		qglCallLists                 = glCallLists;
		qglClear                     = glClear;
		qglClearAccum                = glClearAccum;
		qglClearColor                = glClearColor;
		qglClearDepth                = glClearDepth;
		qglClearIndex                = glClearIndex;
		qglClearStencil              = glClearStencil;
		qglClipPlane                 = glClipPlane;
		qglColor3b                   = glColor3b;
		qglColor3bv                  = glColor3bv;
		qglColor3d                   = glColor3d;
		qglColor3dv                  = glColor3dv;
		qglColor3f                   = glColor3f;
		qglColor3fv                  = glColor3fv;
		qglColor3i                   = glColor3i;
		qglColor3iv                  = glColor3iv;
		qglColor3s                   = glColor3s;
		qglColor3sv                  = glColor3sv;
		qglColor3ub                  = glColor3ub;
		qglColor3ubv                 = glColor3ubv;
		qglColor3ui                  = glColor3ui;
		qglColor3uiv                 = glColor3uiv;
		qglColor3us                  = glColor3us;
		qglColor3usv                 = glColor3usv;
		qglColor4b                   = glColor4b;
		qglColor4bv                  = glColor4bv;
		qglColor4d                   = glColor4d;
		qglColor4dv                  = glColor4dv;
		qglColor4f                   = glColor4f;
		qglColor4fv                  = glColor4fv;
		qglColor4i                   = glColor4i;
		qglColor4iv                  = glColor4iv;
		qglColor4s                   = glColor4s;
		qglColor4sv                  = glColor4sv;
		qglColor4ub                  = glColor4ub;
		qglColor4ubv                 = glColor4ubv;
		qglColor4ui                  = glColor4ui;
		qglColor4uiv                 = glColor4uiv;
		qglColor4us                  = glColor4us;
		qglColor4usv                 = glColor4usv;
		qglColorMask                 = glColorMask;
		qglColorMaterial             = glColorMaterial;
		qglColorPointer              = glColorPointer;
		qglCopyPixels                = glCopyPixels;
		qglCopyTexImage1D            = glCopyTexImage1D;
		qglCopyTexImage2D            = glCopyTexImage2D;
		qglCopyTexSubImage1D         = glCopyTexSubImage1D;
		qglCopyTexSubImage2D         = glCopyTexSubImage2D;
		qglCullFace                  = glCullFace;
		qglDeleteLists               = glDeleteLists ;
		qglDeleteTextures            = glDeleteTextures ;
		qglDepthFunc                 = glDepthFunc ;
		qglDepthMask                 = glDepthMask ;
		qglDepthRange                = glDepthRange ;
		qglDisable                   = glDisable ;
		qglDisableClientState        = glDisableClientState ;
		qglDrawArrays                = glDrawArrays ;
		qglDrawBuffer                = glDrawBuffer ;
		qglDrawElements              = glDrawElements ;
		qglDrawPixels                = glDrawPixels ;
		qglEdgeFlag                  = glEdgeFlag ;
		qglEdgeFlagPointer           = glEdgeFlagPointer ;
		qglEdgeFlagv                 = glEdgeFlagv ;
		qglEnable                    =  glEnable                    ;
		qglEnableClientState         =  glEnableClientState         ;
		qglEnd                       =  glEnd                       ;
		qglEndList                   =  glEndList                   ;
		qglEvalCoord1d               =  glEvalCoord1d                ;
		qglEvalCoord1dv              =  glEvalCoord1dv              ;
		qglEvalCoord1f               =  glEvalCoord1f               ;
		qglEvalCoord1fv              =  glEvalCoord1fv              ;
		qglEvalCoord2d               =  glEvalCoord2d               ;
		qglEvalCoord2dv              =  glEvalCoord2dv              ;
		qglEvalCoord2f               =  glEvalCoord2f               ;
		qglEvalCoord2fv              =  glEvalCoord2fv              ;
		qglEvalMesh1                 =  glEvalMesh1                 ;
		qglEvalMesh2                 =  glEvalMesh2                 ;
		qglEvalPoint1                =  glEvalPoint1                ;
		qglEvalPoint2                =  glEvalPoint2                ;
		qglFeedbackBuffer            =  glFeedbackBuffer            ;
		qglFinish                    =  glFinish                    ;
		qglFlush                     =  glFlush                     ;
		qglFogf                      =  glFogf                      ;
		qglFogfv                     =  glFogfv                     ;
		qglFogi                      =  glFogi                      ;
		qglFogiv                     =  glFogiv                     ;
		qglFrontFace                 =  glFrontFace                 ;
		qglFrustum                   =  glFrustum                   ;
		qglGenLists                  =  glGenLists                  ;
		qglGenTextures               =  glGenTextures               ;
		qglGetBooleanv               =  glGetBooleanv               ;
		qglGetClipPlane              =  glGetClipPlane              ;
		qglGetDoublev                =  glGetDoublev                ;
		qglGetError                  =  glGetError                  ;
		qglGetFloatv                 =  glGetFloatv                 ;
		qglGetIntegerv               =  glGetIntegerv               ;
		qglGetLightfv                =  glGetLightfv                ;
		qglGetLightiv                =  glGetLightiv                ;
		qglGetMapdv                  =  glGetMapdv                  ;
		qglGetMapfv                  =  glGetMapfv                  ;
		qglGetMapiv                  =  glGetMapiv                  ;
		qglGetMaterialfv             =  glGetMaterialfv             ;
		qglGetMaterialiv             =  glGetMaterialiv             ;
		qglGetPixelMapfv             =  glGetPixelMapfv             ;
		qglGetPixelMapuiv            =  glGetPixelMapuiv            ;
		qglGetPixelMapusv            =  glGetPixelMapusv            ;
		qglGetPointerv               =  glGetPointerv               ;
		qglGetPolygonStipple         =  glGetPolygonStipple         ;
		qglGetString                 =  glGetString                 ;
		qglGetTexEnvfv               =  glGetTexEnvfv               ;
		qglGetTexEnviv               =  glGetTexEnviv               ;
		qglGetTexGendv               =  glGetTexGendv               ;
		qglGetTexGenfv               =  glGetTexGenfv               ;
		qglGetTexGeniv               =  glGetTexGeniv               ;
		qglGetTexImage               =  glGetTexImage               ;
		qglGetTexLevelParameterfv    =  glGetTexLevelParameterfv    ;
		qglGetTexLevelParameteriv    =  glGetTexLevelParameteriv    ;
		qglGetTexParameterfv         =  glGetTexParameterfv         ;
		qglGetTexParameteriv         =  glGetTexParameteriv         ;
		qglHint                      =  glHint                      ;
		qglIndexMask                 =  glIndexMask                 ;
		qglIndexPointer              =  glIndexPointer              ;
		qglIndexd                    =  glIndexd                    ;
		qglIndexdv                   =  glIndexdv                   ;
		qglIndexf                    =  glIndexf                    ;
		qglIndexfv                   =  glIndexfv                   ;
		qglIndexi                    =  glIndexi                    ;
		qglIndexiv                   =  glIndexiv                   ;
		qglIndexs                    =  glIndexs                    ;
		qglIndexsv                   =  glIndexsv                   ;
		qglIndexub                   =  glIndexub                   ;
		qglIndexubv                  =  glIndexubv                  ;
		qglInitNames                 =  glInitNames                 ;
		qglInterleavedArrays         =  glInterleavedArrays         ;
		qglIsEnabled                 =  glIsEnabled                 ;
		qglIsList                    =  glIsList                    ;
		qglIsTexture                 =  glIsTexture                 ;
		qglLightModelf               =  glLightModelf               ;
		qglLightModelfv              =  glLightModelfv              ;
		qglLightModeli               =  glLightModeli               ;
		qglLightModeliv              =  glLightModeliv              ;
		qglLightf                    =  glLightf                    ;
		qglLightfv                   =  glLightfv                   ;
		qglLighti                    =  glLighti                    ;
		qglLightiv                   =  glLightiv                   ;
		qglLineStipple               =  glLineStipple               ;
		qglLineWidth                 =  glLineWidth                 ;
		qglListBase                  =  glListBase                  ;
		qglLoadIdentity              =  glLoadIdentity              ;
		qglLoadMatrixd               =  glLoadMatrixd               ;
		qglLoadMatrixf               =  glLoadMatrixf               ;
		qglLoadName                  =  glLoadName                  ;
		qglLogicOp                   =  glLogicOp                   ;
		qglMap1d                     =  glMap1d                     ;
		qglMap1f                     =  glMap1f                     ;
		qglMap2d                     =  glMap2d                     ;
		qglMap2f                     =  glMap2f                     ;
		qglMapGrid1d                 =  glMapGrid1d                 ;
		qglMapGrid1f                 =  glMapGrid1f                 ;
		qglMapGrid2d                 =  glMapGrid2d                 ;
		qglMapGrid2f                 =  glMapGrid2f                 ;
		qglMaterialf                 =  glMaterialf                 ;
		qglMaterialfv                =  glMaterialfv                ;
		qglMateriali                 =  glMateriali                 ;
		qglMaterialiv                =  glMaterialiv                ;
		qglMatrixMode                =  glMatrixMode                ;
		qglMultMatrixd               =  glMultMatrixd               ;
		qglMultMatrixf               =  glMultMatrixf               ;
		qglNewList                   =  glNewList                   ;
		qglNormal3b                  =  glNormal3b                  ;
		qglNormal3bv                 =  glNormal3bv                 ;
		qglNormal3d                  =  glNormal3d                  ;
		qglNormal3dv                 =  glNormal3dv                 ;
		qglNormal3f                  =  glNormal3f                  ;
		qglNormal3fv                 =  glNormal3fv                 ;
		qglNormal3i                  =  glNormal3i                  ;
		qglNormal3iv                 =  glNormal3iv                 ;
		qglNormal3s                  =  glNormal3s                  ;
		qglNormal3sv                 =  glNormal3sv                 ;
		qglNormalPointer             =  glNormalPointer             ;
		qglOrtho                     =  glOrtho                     ;
		qglPassThrough               =  glPassThrough               ;
		qglPixelMapfv                =  glPixelMapfv                ;
		qglPixelMapuiv               =  glPixelMapuiv               ;
		qglPixelMapusv               =  glPixelMapusv               ;
		qglPixelStoref               =  glPixelStoref               ;
		qglPixelStorei               =  glPixelStorei               ;
		qglPixelTransferf            =  glPixelTransferf            ;
		qglPixelTransferi            =  glPixelTransferi            ;
		qglPixelZoom                 =  glPixelZoom                 ;
		qglPointSize                 =  glPointSize                 ;
		qglPolygonMode               =  glPolygonMode               ;
		qglPolygonOffset             =  glPolygonOffset             ;
		qglPolygonStipple            =  glPolygonStipple            ;
		qglPopAttrib                 =  glPopAttrib                 ;
		qglPopClientAttrib           =  glPopClientAttrib           ;
		qglPopMatrix                 =  glPopMatrix                 ;
		qglPopName                   =  glPopName                   ;
		qglPrioritizeTextures        =  glPrioritizeTextures        ;
		qglPushAttrib                =  glPushAttrib                ;
		qglPushClientAttrib          =  glPushClientAttrib          ;
		qglPushMatrix                =  glPushMatrix                ;
		qglPushName                  =  glPushName                  ;
		qglRasterPos2d               =  glRasterPos2d               ;
		qglRasterPos2dv              =  glRasterPos2dv              ;
		qglRasterPos2f               =  glRasterPos2f               ;
		qglRasterPos2fv              =  glRasterPos2fv              ;
		qglRasterPos2i               =  glRasterPos2i               ;
		qglRasterPos2iv              =  glRasterPos2iv              ;
		qglRasterPos2s               =  glRasterPos2s               ;
		qglRasterPos2sv              =  glRasterPos2sv              ;
		qglRasterPos3d               =  glRasterPos3d               ;
		qglRasterPos3dv              =  glRasterPos3dv              ;
		qglRasterPos3f               =  glRasterPos3f               ;
		qglRasterPos3fv              =  glRasterPos3fv              ;
		qglRasterPos3i               =  glRasterPos3i               ;
		qglRasterPos3iv              =  glRasterPos3iv              ;
		qglRasterPos3s               =  glRasterPos3s               ;
		qglRasterPos3sv              =  glRasterPos3sv              ;
		qglRasterPos4d               =  glRasterPos4d               ;
		qglRasterPos4dv              =  glRasterPos4dv              ;
		qglRasterPos4f               =  glRasterPos4f               ;
		qglRasterPos4fv              =  glRasterPos4fv              ;
		qglRasterPos4i               =  glRasterPos4i               ;
		qglRasterPos4iv              =  glRasterPos4iv              ;
		qglRasterPos4s               =  glRasterPos4s               ;
		qglRasterPos4sv              =  glRasterPos4sv              ;
		qglReadBuffer                =  glReadBuffer                ;
		qglReadPixels                =  glReadPixels                ;
		qglRectd                     =  glRectd                     ;
		qglRectdv                    =  glRectdv                    ;
		qglRectf                     =  glRectf                     ;
		qglRectfv                    =  glRectfv                    ;
		qglRecti                     =  glRecti                     ;
		qglRectiv                    =  glRectiv                    ;
		qglRects                     =  glRects                     ;
		qglRectsv                    =  glRectsv                    ;
		qglRenderMode                =  glRenderMode                ;
		qglRotated                   =  glRotated                   ;
		qglRotatef                   =  glRotatef                   ;
		qglScaled                    =  glScaled                    ;
		qglScalef                    =  glScalef                    ;
		qglScissor                   =  glScissor                   ;
		qglSelectBuffer              =  glSelectBuffer              ;
		qglShadeModel                =  glShadeModel                ;
		qglStencilFunc               =  glStencilFunc               ;
		qglStencilMask               =  glStencilMask               ;
		qglStencilOp                 =  glStencilOp                 ;
		qglTexCoord1d                =  glTexCoord1d                ;
		qglTexCoord1dv               =  glTexCoord1dv               ;
		qglTexCoord1f                =  glTexCoord1f                ;
		qglTexCoord1fv               =  glTexCoord1fv               ;
		qglTexCoord1i                =  glTexCoord1i                ;
		qglTexCoord1iv               =  glTexCoord1iv               ;
		qglTexCoord1s                =  glTexCoord1s                ;
		qglTexCoord1sv               =  glTexCoord1sv               ;
		qglTexCoord2d                =  glTexCoord2d                ;
		qglTexCoord2dv               =  glTexCoord2dv               ;
		qglTexCoord2f                =  glTexCoord2f                ;
		qglTexCoord2fv               =  glTexCoord2fv               ;
		qglTexCoord2i                =  glTexCoord2i                ;
		qglTexCoord2iv               =  glTexCoord2iv               ;
		qglTexCoord2s                =  glTexCoord2s                ;
		qglTexCoord2sv               =  glTexCoord2sv               ;
		qglTexCoord3d                =  glTexCoord3d                ;
		qglTexCoord3dv               =  glTexCoord3dv               ;
		qglTexCoord3f                =  glTexCoord3f                ;
		qglTexCoord3fv               =  glTexCoord3fv               ;
		qglTexCoord3i                =  glTexCoord3i                ;
		qglTexCoord3iv               =  glTexCoord3iv               ;
		qglTexCoord3s                =  glTexCoord3s                ;
		qglTexCoord3sv               =  glTexCoord3sv               ;
		qglTexCoord4d                =  glTexCoord4d                ;
		qglTexCoord4dv               =  glTexCoord4dv               ;
		qglTexCoord4f                =  glTexCoord4f                ;
		qglTexCoord4fv               =  glTexCoord4fv               ;
		qglTexCoord4i                =  glTexCoord4i                ;
		qglTexCoord4iv               =  glTexCoord4iv               ;
		qglTexCoord4s                =  glTexCoord4s                ;
		qglTexCoord4sv               =  glTexCoord4sv               ;
		qglTexCoordPointer           =  glTexCoordPointer           ;
		qglTexEnvf                   =  glTexEnvf                   ;
		qglTexEnvfv                  =  glTexEnvfv                  ;
		qglTexEnvi                   =  glTexEnvi                   ;
		qglTexEnviv                  =  glTexEnviv                  ;
		qglTexGend                   =  glTexGend                   ;
		qglTexGendv                  =  glTexGendv                  ;
		qglTexGenf                   =  glTexGenf                   ;
		qglTexGenfv                  =  glTexGenfv                  ;
		qglTexGeni                   =  glTexGeni                   ;
		qglTexGeniv                  =  glTexGeniv                  ;
		qglTexImage1D                =  glTexImage1D                ;
		qglTexImage2D                =  glTexImage2D                ;
		qglTexParameterf             =  glTexParameterf             ;
		qglTexParameterfv            =  glTexParameterfv            ;
		qglTexParameteri             =  glTexParameteri             ;
		qglTexParameteriv            =  glTexParameteriv            ;
		qglTexSubImage1D             =  glTexSubImage1D             ;
		qglTexSubImage2D             =  glTexSubImage2D             ;
		qglTranslated                =  glTranslated                ;
		qglTranslatef                =  glTranslatef                ;
		qglVertex2d                  =  glVertex2d                  ;
		qglVertex2dv                 =  glVertex2dv                 ;
		qglVertex2f                  =  glVertex2f                  ;
		qglVertex2fv                 =  glVertex2fv                 ;
		qglVertex2i                  =  glVertex2i                  ;
		qglVertex2iv                 =  glVertex2iv                 ;
		qglVertex2s                  =  glVertex2s                  ;
		qglVertex2sv                 =  glVertex2sv                 ;
		qglVertex3d                  =  glVertex3d                  ;
		qglVertex3dv                 =  glVertex3dv                 ;
		qglVertex3f                  =  glVertex3f                  ;
		qglVertex3fv                 =  glVertex3fv                 ;
		qglVertex3i                  =  glVertex3i                  ;
		qglVertex3iv                 =  glVertex3iv                 ;
		qglVertex3s                  =  glVertex3s                  ;
		qglVertex3sv                 =  glVertex3sv                 ;
		qglVertex4d                  =  glVertex4d                  ;
		qglVertex4dv                 =  glVertex4dv                 ;
		qglVertex4f                  =  glVertex4f                  ;
		qglVertex4fv                 =  glVertex4fv                 ;
		qglVertex4i                  =  glVertex4i                  ;
		qglVertex4iv                 =  glVertex4iv                 ;
		qglVertex4s                  =  glVertex4s                  ;
		qglVertex4sv                 =  glVertex4sv                 ;
		qglVertexPointer             =  glVertexPointer             ;
		qglViewport                  =  glViewport                  ;
	}
#endif // MAC_QGL_LINKED
}


