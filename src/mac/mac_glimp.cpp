/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Â“Wolf ET Source CodeÂ”).  

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

#include <math.h>

Q3DEF_BEGIN
	#include "../renderer/tr_local.h"
	#include "../client/client.h"
	#include "mac_local.h"
Q3DEF_END

#include "mac_glimp.h"

#include "CDrawSprocket.h"
#include "MacPrefs.h"
#include "AGLUtils.h"
#include "PickMonitor/pickmonitor.h"

//#define PTHREADS 1
#define MPLIB 1

#if PTHREADS
#include <pthread.h>
#include <semaphore.h>
#endif

extern long gSystemVersion;

// Mac-specific rendering cvars
cvar_t *r_device;
cvar_t *r_ext_transform_hint;
#if !MAC_WOLF
cvar_t *r_ati_fsaa_samples;
#endif
cvar_t *r_nv_fsaa_samples;
cvar_t *r_nv_quincunx;

#if MAC_MOHAA
cvar_t *r_stipplelines;
#endif

#if MAC_Q3_OLDSTUFF
// Newer Q3 variables that aren't in the older Q3 cores
cvar_t  *r_ext_preferred_tc_method;
#if !MAC_WOLF
cvar_t  *r_ext_texture_filter_anisotropic;
#endif
#endif

#if MAC_JKJA && MAC_Q3_MP
// Hack variable for deciding which kind of texture rectangle thing to do (for some
// reason it acts different on radeon! It's against the spec!).
bool g_bTextureRectangleHack = false;
#endif

#if MAC_WOLF
#define TC_S3TC_DXT TC_EXT_COMP_S3TC
int gl_NormalFontBase = 0;
#endif

#if !MAC_JK2 && !MAC_JK2_MP
glHardwareType_t sys_hardwareType;
#endif
aglstate_t sys_gl;

glconfig_mac_t glConfig_Mac;

int gClampMode = GL_CLAMP_TO_EDGE;

int vid_xpos;
int vid_ypos;
static Boolean sHasGameGamma = false;

qboolean GLimp_ChangeMode( int mode );
void GLimp_EndFrame( void );
static void GLimp_Extensions( void );


#pragma mark ¥ Mac-Specific Calls
/*
============
CheckErrors
============
*/
void CheckErrors( const char *inFile, int inLine ) {
	GLenum err;

	err = aglGetError();
	if ( err != AGL_NO_ERROR ) {
		Com_Printf( "aglGetError: %s\n", aglErrorString( err ) );
		Com_Error( ERR_FATAL, "aglGetError: %s at %s line %d", aglErrorString( err ), inFile, inLine );
	}
}

Boolean IsRadeon( void ) {
	if ( strstr( glConfig.renderer_string, "Radeon" ) || strstr( glConfig.renderer_string, "R-200" ) ) {
		return true;
	} else {
		return false;
	}
}

#if !MAC_JKJA || MAC_Q3_MP
/*
================
VID_Printf

DLL glue
================
*/
#define MAXPRINTMSG 4096
void VID_Printf( int print_level, const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start( argptr,fmt );
	vsprintf( msg,fmt,argptr );
	va_end( argptr );

	if ( print_level == PRINT_ALL ) {
		Com_Printf( "%s", msg );
	} else if ( print_level == PRINT_WARNING ) {
		Com_Printf( S_COLOR_YELLOW "%s", msg );       // yellow
	} else if ( print_level == PRINT_DEVELOPER ) {
		Com_DPrintf( S_COLOR_RED "%s", msg );
	}
}
#endif

void GLimp_InitGamma( void ) {
	// if we're already set up, bail
	if ( sys_gl.systemGammas ) {
		return;
	}

	{
		if ( 1 ) {
			CGDisplayErr cgErr;
			CGTableCount count = 0;
			CGGammaTable *cgGamma;

			cgGamma = (CGGammaTable *)malloc( sizeof( CGGammaTable ) );
			if ( !cgGamma ) {
				Com_Error( ERR_FATAL, "Could not initialize gamma table" );
			}
			cgErr = CGGetDisplayTransferByTable( NULL, 256, cgGamma->red, cgGamma->green, cgGamma->blue, &count );
			if ( cgErr ) {
				Com_Error( ERR_FATAL, "Could not read gamma table" );
			}

			glConfig.deviceSupportsGamma = qtrue;
			sys_gl.systemGammas = (Ptr) cgGamma;
			VID_Printf( PRINT_ALL, "CoreGraphics gamma table loaded\n" );
		} else
		{
			glConfig.deviceSupportsGamma = qfalse;
			sys_gl.systemGammas = NULL;
			VID_Printf( PRINT_ALL, "Could not call CoreGraphics gamma table API\n" );
		}
	}
}

//=======================================================================

/*
=====================
GLimp_ChangeDisplay
=====================
*/
void GLimp_ChangeDisplay( int *actualWidth, int *actualHeight ) {
}

//=======================================================================


/*
===================
GLimp_AglDescribe_f

===================
*/
void GLimp_AglDescribe_f( void ) {
	long value;
	long r,g,b,a;
	long stencil, depth;

	VID_Printf( PRINT_ALL, "Selected pixel format 0x%x\n", (int)sys_gl.fmt );

	VID_Printf( PRINT_ALL, "TEXTURE_MEMORY: %i\n", sys_gl.textureMemory );
	VID_Printf( PRINT_ALL, "VIDEO_MEMORY: %i\n", sys_gl.videoMemory );

	aglDescribePixelFormat( sys_gl.fmt, AGL_RED_SIZE, &r );
	aglDescribePixelFormat( sys_gl.fmt, AGL_GREEN_SIZE, &g );
	aglDescribePixelFormat( sys_gl.fmt, AGL_BLUE_SIZE, &b );
	aglDescribePixelFormat( sys_gl.fmt, AGL_ALPHA_SIZE, &a );
	aglDescribePixelFormat( sys_gl.fmt, AGL_STENCIL_SIZE, &stencil );
	aglDescribePixelFormat( sys_gl.fmt, AGL_DEPTH_SIZE, &depth );
	VID_Printf( PRINT_ALL, "red:%i green:%i blue:%i alpha:%i depth:%i stencil:%i\n",
				r, g, b, a, depth, stencil );

	aglDescribePixelFormat( sys_gl.fmt, AGL_BUFFER_SIZE, &value );
	VID_Printf( PRINT_ALL, "BUFFER_SIZE: %i\n", value );

	aglDescribePixelFormat( sys_gl.fmt, AGL_PIXEL_SIZE, &value );
	VID_Printf( PRINT_ALL, "PIXEL_SIZE: %i\n", value );

	aglDescribePixelFormat( sys_gl.fmt, AGL_RENDERER_ID, &value );
	VID_Printf( PRINT_ALL, "RENDERER_ID: %i\n", value );
}

/*
===================
GLimp_AglState_f

===================
*/
void GLimp_AglState_f( void ) {
	char    *cmd;
	int state, value;

	if ( Cmd_Argc() != 3 ) {
		VID_Printf( PRINT_ALL, "Usage: aglstate <parameter> <0/1>\n" );
		return;
	}

	cmd = Cmd_Argv( 1 );
	if ( !Q_stricmp( cmd, "rasterization" ) ) {
		state = AGL_RASTERIZATION;
	} else
	{
		VID_Printf( PRINT_ALL, "Unknown agl state: %s\n", cmd );
		return;
	}

	cmd = Cmd_Argv( 2 );
	value = atoi( cmd );

	if ( value ) {
		aglEnable( sys_gl.context, state );
	} else
	{
		aglDisable( sys_gl.context, state );
	}
}

Q3DEF void GLimp_pause( void ) {
	_aglSuspendGameContext();
}

Q3DEF void GLimp_resume( void ) {
	_aglResumeGameContext();
}

#if TARGET_API_MAC_CARBON
Q3DEF void GLimp_SetGameGamma( void ) {
	if ( ( gSystemVersion < 0x1000 ) || ( !sHasGameGamma ) ) {
		return;
	}

	{
		CGDisplayErr cgErr;

		cgErr = CGSetDisplayTransferByTable( NULL, 256, sys_gl.gameGamma.red, sys_gl.gameGamma.green, sys_gl.gameGamma.blue );

#if _DEBUG
		VID_Printf( PRINT_ALL, "CoreGraphics gamma table set, err: %d\n", cgErr );
#endif
	}

}
#endif

static void GLW_InitTextureCompression( void ) {
	qboolean newer_tc, old_tc;

	// Check for available tc methods.
	newer_tc = ( strstr( glConfig.extensions_string, "ARB_texture_compression" )
				 && strstr( glConfig.extensions_string, "EXT_texture_compression_s3tc" ) ) ? qtrue : qfalse;
	old_tc = ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) ) ? qtrue : qfalse;

	if ( old_tc ) {
		VID_Printf( PRINT_ALL, "...GL_S3_s3tc available\n" );
	}

	if ( newer_tc ) {
		VID_Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc available\n" );
	}

#if MAC_MOHAA || MAC_WOLF
	if ( !r_ext_compressed_textures->integer )
#else
	if ( !r_ext_compressed_textures->value )
#endif
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		VID_Printf( PRINT_ALL, "...ignoring texture compression\n" );
	} else if ( !old_tc && !newer_tc )   {
		{
			// Requesting texture compression, but no method found
			glConfig.textureCompression = TC_NONE;
			VID_Printf( PRINT_ALL, "...no supported texture compression method found\n" );
			VID_Printf( PRINT_ALL, ".....ignoring texture compression\n" );
		}
	} else
	{
#if MAC_MOHAA
		qglCompressedTexImage2DARB = glCompressedTexImage2DARB;
#endif

		// some form of supported texture compression is avaiable, so see if the user has a preference
		if ( r_ext_preferred_tc_method->integer == TC_NONE ) {
			// No preference, so pick the best
			if ( newer_tc ) {
				VID_Printf( PRINT_ALL, "...no tc preference specified\n" );
				VID_Printf( PRINT_ALL, ".....using GL_EXT_texture_compression_s3tc\n" );
				glConfig.textureCompression = TC_S3TC_DXT;
			} else
			{
				VID_Printf( PRINT_ALL, "...no tc preference specified\n" );
				VID_Printf( PRINT_ALL, ".....using GL_S3_s3tc\n" );
				glConfig.textureCompression = TC_S3TC;
			}
		} else
		{
			// User has specified a preference, now see if this request can be honored
			if ( old_tc && newer_tc ) {
				// both are avaiable, so we can use the desired tc method
				if ( r_ext_preferred_tc_method->integer == TC_S3TC ) {
					VID_Printf( PRINT_ALL, "...using preferred tc method, GL_S3_s3tc\n" );
					glConfig.textureCompression = TC_S3TC;
				} else
				{
					VID_Printf( PRINT_ALL, "...using preferred tc method, GL_EXT_texture_compression_s3tc\n" );
					glConfig.textureCompression = TC_S3TC_DXT;
				}
			} else
			{
				// Both methods are not available, so this gets trickier
				if ( r_ext_preferred_tc_method->integer == TC_S3TC ) {
					// Preferring to user older compression
					if ( old_tc ) {
						VID_Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					} else
					{
						// Drat, preference can't be honored
						VID_Printf( PRINT_ALL, "...preferred tc method, GL_S3_s3tc not available\n" );
						VID_Printf( PRINT_ALL, ".....falling back to GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				} else
				{
					// Preferring to user newer compression
					if ( newer_tc ) {
						VID_Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					} else
					{
						// Drat, preference can't be honored
						VID_Printf( PRINT_ALL, "...preferred tc method, GL_EXT_texture_compression_s3tc not available\n" );
						VID_Printf( PRINT_ALL, ".....falling back to GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

/*
===================
GLimp_Extensions

===================
*/
static void GLimp_Extensions( void ) {
	if ( !r_allowExtensions->integer ) {
		VID_Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	VID_Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) ) {
		if ( r_ext_texture_env_add->integer ) {
			glConfig.textureEnvAddAvailable = qtrue;
			VID_Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		} else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			VID_Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_EXT_texture_filter_anisotropic
#if MAC_Q3_OLDSTUFF
	#if MAC_WOLF
	glConfig.anisotropicAvailable = qfalse;
	#else
	glConfig_Mac.textureFilterAnisotropicAvailable = qfalse;
	#endif
#else
	#if MAC_JKJA
	glConfig.maxTextureFilterAnisotropy = 0;
	#else
	glConfig.textureFilterAnisotropicAvailable = qfalse;
	#endif
#endif
	if ( strstr( glConfig.extensions_string, "EXT_texture_filter_anisotropic" ) ) {
#if MAC_Q3_OLDSTUFF
	#if MAC_WOLF
		glConfig.anisotropicAvailable = qtrue;
	#else
		glConfig_Mac.textureFilterAnisotropicAvailable = qtrue;
	#endif
#else
	#if MAC_JKJA
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy );
	#else
		glConfig.textureFilterAnisotropicAvailable = qtrue;
	#endif
#endif
		VID_Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic available\n" );

		if ( r_ext_texture_filter_anisotropic->integer ) {
			VID_Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
		} else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
#if MAC_JKJA
		Cvar_Set( "r_ext_texture_filter_anisotropic_avail", va( "%f",glConfig.maxTextureFilterAnisotropy ) );
		if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy ) {
			Cvar_Set( "r_ext_texture_filter_anisotropic", va( "%f",glConfig.maxTextureFilterAnisotropy ) );
		}
#else
		Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "1" );
#endif
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
		Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

	// GL_EXT_clamp_to_edge / GL_SGIS_clamp_to_edge
#if MAC_STVEF_HM || MAC_Q3_OLDSTUFF
	gClampMode = GL_CLAMP;
	glConfig_Mac.clampToEdgeAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) ||
		 strstr( glConfig.extensions_string, "GL_SGIS_texture_edge_clamp" ) ) {
		gClampMode = GL_CLAMP_TO_EDGE;
		glConfig_Mac.clampToEdgeAvailable = qtrue;
		VID_Printf( PRINT_ALL, "...Using GL_EXT_texture_edge_clamp\n" );
	}
#else
	glConfig.clampToEdgeAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) ||
		 strstr( glConfig.extensions_string, "GL_SGIS_texture_edge_clamp" ) ) {
		glConfig.clampToEdgeAvailable = qtrue;
		VID_Printf( PRINT_ALL, "...Using texture_edge_clamp\n" );
	}
#endif

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  ) {
		if ( r_ext_multitexture->integer ) {
			qglMultiTexCoord2fARB = glMultiTexCoord2fARB;
			qglActiveTextureARB = glActiveTextureARB;
			qglClientActiveTextureARB = glClientActiveTextureARB;

			if ( qglActiveTextureARB ) {
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, (long *)&glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 ) {
					VID_Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				} else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					VID_Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		} else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) ) {
		if ( r_ext_compiled_vertex_array->integer ) {
			qglLockArraysEXT = glLockArraysEXT;
			qglUnlockArraysEXT = glUnlockArraysEXT;

			VID_Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
		} else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

#if !MAC_Q3_OLDSTUFF
	// GL_ARB_point_parameters (10.2 and up)
	// LBO - the PC JK2 code checks for GL_EXT_point_parameters, but Apple supports the ARB version
	// which is identical.
	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_point_parameters" ) ) {
		VID_Printf( PRINT_ALL, "...using GL_ARB_point_parameters\n" );
		qglPointParameterfEXT = glPointParameterfARB;
		qglPointParameterfvEXT = glPointParameterfvARB;
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_ARB_point_parameters not found\n" );
	}
#endif

	// GL_ARB_multisample
	if ( strstr( glConfig.extensions_string, "GL_ARB_multisample" ) ||
		 strstr( glConfig.extensions_string, "GL_multisample" ) ) {
		VID_Printf( PRINT_ALL, "...using GL_ARB_multisample\n" );
		glConfig_Mac.multisampleAvailable = qtrue;
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_ARB_multisample not found\n" );
		glConfig_Mac.multisampleAvailable = qfalse;
	}

	// apple transform hint
	if ( strstr( glConfig.extensions_string, "GL_APPLE_transform_hint" ) ) {
		if ( r_ext_transform_hint->integer ) {
			glHint( GL_TRANSFORM_HINT_APPLE, GL_FASTEST );
			VID_Printf( PRINT_ALL, "...using GL_APPLE_transform_hint\n" );
		} else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_APPLE_transform_hint\n" );
		}
	} else
	{
		VID_Printf( PRINT_ALL, "...GL_APPLE_transform_hint not found\n" );
	}

	// APPLE_client_storage
	if ( strstr( glConfig.extensions_string, "GL_APPLE_client_storage" ) ) {
		glConfig_Mac.hasClientStorage = qtrue;
		VID_Printf( PRINT_ALL, "...using GL_APPLE_client_storage\n" );
	} else
	{
		glConfig_Mac.hasClientStorage = qfalse;
		VID_Printf( PRINT_ALL, "...GL_APPLE_client_storage not found\n" );
	}

#if MAC_WOLF
#if !MAC_WOLF_ET
	// GL_ATI_pn_triangles - ATI PN-Triangles
	if ( strstr( glConfig.extensions_string, "GL_ATIX_pn_triangles" ) ) {
		if ( r_ext_ATI_pntriangles->integer ) {
			ri.Printf( PRINT_ALL, "...using GL_ATI_pn_triangles\n" );

			qglPNTrianglesiATI = glPNTrianglesiATIX;
			qglPNTrianglesfATI = glPNTrianglesfATIX;

			if ( !qglPNTrianglesiATI || !qglPNTrianglesfATI ) {
				ri.Error( ERR_FATAL, "bad getprocaddress 0" );
			}
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ATI_pn_triangles\n" );
			ri.Cvar_Set( "r_ext_ATI_pntriangles", "0" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_ATI_pn_triangles not found\n" );
		ri.Cvar_Set( "r_ext_ATI_pntriangles", "0" );
	}
#endif

	// GL_NV_fog_distance
	if ( strstr( glConfig.extensions_string, "GL_NV_fog_distance" ) ) {
		if ( r_ext_NV_fog_dist->integer ) {
			glConfig.NVFogAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_NV_fog_distance\n" );
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_NV_fog_distance\n" );
			ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
		}
	} else
	{
		glConfig.NVFogAvailable = qfalse;
		ri.Printf( PRINT_ALL, "...GL_NV_fog_distance not found\n" );
		ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
	}
#endif

#if !MAC_JK2 && !MAC_JK2_MP & !MAC_MOHAA
	// determine ragePro status for rendering hacks
	if ( strstr( glConfig.renderer_string, "Rage Pro" )
		 || strstr( glConfig.renderer_string, "rage pro" ) ) {
		glConfig.hardwareType = GLHW_RAGEPRO;
	} else {
		glConfig.hardwareType = GLHW_GENERIC;
	}
#endif

	// Enable FSAA on ATi cards
	if ( IsRadeon() /*&& gSystemVersion >= 0x0922*/ ) {
		GLenum err;
		aglSetInteger( sys_gl.context, ATI_FSAA_SAMPLES, (const long*)&r_ati_fsaa_samples->integer );

		// Flush AGL_BAD_ENUM, which can occur if we're using older OS/drivers
		err = aglGetError();
	}
}

void GLimp_SetFSAA( void ) {
	if ( !glConfig_Mac.multisampleAvailable ) {
		return;
	}

	VID_Printf( PRINT_ALL, "Changing nVidia FSAA samples to %d\n", r_nv_fsaa_samples->integer );
	VID_Printf( PRINT_ALL, "...r_nv_quincunx is %d\n", r_nv_quincunx->integer );

	switch ( r_nv_fsaa_samples->integer )
	{
	case 0:
	case 1:
		glDisable( GL_MULTISAMPLE );
		break;
	case 2:     // 2 pixel, 2 tap
	case 4:     // 4 pixel, 4 tap
		glEnable( GL_MULTISAMPLE );
		if ( r_nv_quincunx->integer ) {
			glHint( GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST );
		} else {
			glHint( GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST );
		}
		break;
	}
}


#pragma mark -
#pragma mark ¥ Q3 core calls

extern "C" {
void PlayIntroMovies( void );
}
/*
===================
GLimp_Init

Don't return unless OpenGL has been properly initialized
===================
*/
Q3DEF void GLimp_Init( void ) {
	char buf[MAX_STRING_CHARS];
	GLint major, minor;
	static qboolean registered;
	cvar_t *lastValidRenderer = Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

	gDrawSprocket = new CDrawSprocket( macPrefs.displayID );
	if ( gDrawSprocket == NULL ) {
		ExitToShell();
	}

	VID_Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );

	aglResetLibrary();
	aglGetVersion( &major, &minor );
	QGL_Init();

	VID_Printf( PRINT_ALL, "aglVersion: %i.%i\n", (int)major, (int)minor );

	r_device = Cvar_Get( "r_device", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_ext_transform_hint = Cvar_Get( "r_ext_transform_hint", "1", CVAR_LATCH | CVAR_ARCHIVE );

	// Default FSAA options to off
	r_ati_fsaa_samples = Cvar_Get( "r_ati_fsaa_samples", "1", CVAR_ARCHIVE );
	r_nv_fsaa_samples = Cvar_Get( "r_nv_fsaa_samples", "0", CVAR_LATCH | CVAR_ARCHIVE );
	r_nv_quincunx = Cvar_Get( "r_nv_quincunx", "0", CVAR_ARCHIVE );
#if MAC_Q3_OLDSTUFF
	r_ext_preferred_tc_method = Cvar_Get( "r_ext_preferred_tc_method", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_texture_filter_anisotropic = Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE );
#endif

	if ( !registered ) {
		Cmd_AddCommand( "aglDescribe", GLimp_AglDescribe_f );
		Cmd_AddCommand( "aglState", GLimp_AglState_f );
	}

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glConfig_Mac, 0, sizeof( glConfig_Mac ) );

	r_swapInterval->modified = qtrue;   // force a set next frame

	// Get the display ID in case it isn't zero
	sys_gl.displayID = macPrefs.displayID = gDrawSprocket->GetDisplayID();

	GLimp_InitGamma();

	GLimp_ChangeMode( r_mode->integer );

	// Get the display ID in case it was zero before and the user picked a monitor
	sys_gl.displayID = macPrefs.displayID = gDrawSprocket->GetDisplayID();

	CheckErrors( __FILE__, __LINE__ );

	// get our config strings
#if MAC_JK2 || MAC_JKJA // LBO - starting with JK2, these are pointers to const strings
	glConfig.vendor_string = (const char *) qglGetString( GL_VENDOR );
	glConfig.renderer_string = (const char *) qglGetString( GL_RENDERER );
	glConfig.version_string = (const char *) qglGetString( GL_VERSION );
	glConfig.extensions_string = (const char *) qglGetString( GL_EXTENSIONS );
#else
	Q_strncpyz( glConfig.vendor_string, (const char *)qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (const char *)qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, (const char *)qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (const char *)qglGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );
#endif

	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, (long *)&glConfig.maxTextureSize );
	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) {
		glConfig.maxTextureSize = 0;
	}

//GLimp_pause ();
//aglSetDrawable (gAGLContext, NULL);
//PlayIntroMovies ();
//ExitToShell();

	//
	// chipset specific configuration
	//
	strcpy( buf, glConfig.renderer_string );
	strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	extern qboolean Sys_LowPhysicalMemory();
	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) ) {
#if MAC_JK2
		if ( Sys_LowPhysicalMemory() ) {
			Cvar_Set( "s_khz", "11" ); // this will get called before S_Init
			Cvar_Set( "cg_VariantSoundCap", "2" );
			Cvar_Set( "s_allowDynamicMusic","0" );
		}
#endif
		//reset to defaults
		Cvar_Set( "r_picmip", "1" );

		if ( strstr( buf, "matrox" ) ) {
			Cvar_Set( "r_allowExtensions", "0" );
		} else
		// Savage3D and Savage4 should always have trilinear enabled
		if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) || strstr( buf, "geforce" ) ) {
			Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		} else
		{
			Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		}

		if ( strstr( buf, "kyro" ) ) {
			Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );   //KYROs have it avail, but suck at it!
			Cvar_Set( "r_ext_preferred_tc_method", "1" );          //(Use DXT1 instead of DXT5 - same quality but much better performance on KYRO)
		}

		GLimp_Extensions();
		CheckErrors( __FILE__, __LINE__ );

#if MAC_JK2 || MAC_JKJA
		//this must be a really sucky card!
		if ( ( glConfig.textureCompression == TC_NONE ) || ( glConfig.maxActiveTextures < 2 )  || ( glConfig.maxTextureSize <= 512 ) ) {
			Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );

			Cvar_Set( "r_picmip", "2" );
			Cvar_Set( "r_colorbits", "16" );
			Cvar_Set( "r_texturebits", "16" );
			Cvar_Set( "r_mode", "3" ); //force 640
			Cmd_ExecuteString( "exec low.cfg\n" ); //get the rest which can be pulled in after init
		}
#endif
	}

	Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	GLimp_Extensions();
	CheckErrors( __FILE__, __LINE__ );

	GLimp_SetFSAA();

	// draw something to show that GL is alive
	glClearColor( 1, 0.5, 0.2, 0 );
	glClear( GL_COLOR_BUFFER_BIT );
	GLimp_EndFrame();

	GLimp_AglDescribe_f();

}

/*
===============
GLimp_EndFrame

===============
*/
Q3DEF void GLimp_EndFrame( void ) {

	// check for variable changes
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;
		VID_Printf( PRINT_ALL, "Changing AGL_SWAP_INTERVAL\n" );
		aglSetInteger( sys_gl.context, AGL_SWAP_INTERVAL, (long *)&r_swapInterval->integer );
	}

	if ( r_ati_fsaa_samples->modified ) { //DAJ added changed for ati fsaa
		r_ati_fsaa_samples->modified = qfalse;
		if ( IsRadeon() /*&& gSystemVersion >= 0x0922*/ ) {
			if ( r_ati_fsaa_samples->integer == 1 || r_ati_fsaa_samples->integer == 2 || r_ati_fsaa_samples->integer == 4 ) {
				GLenum err;
				VID_Printf( PRINT_ALL, "Changing ATI FSAA samples to %d\n", r_ati_fsaa_samples->integer );
				aglSetInteger( sys_gl.context, ATI_FSAA_SAMPLES,    (const long*)&r_ati_fsaa_samples->integer );

				// Flush AGL_BAD_ENUM, which can occur if we're using older OS/drivers
				err = aglGetError();
			} else
			{
				VID_Printf( PRINT_ALL, "Valid ATI FSAA samples are 1, 2, 4.  Resetting to 1\n" );
				Cvar_Set( "r_ati_fsaa_samples", "1" );
			}
		}
	}

	if ( r_nv_quincunx->modified ) {
		r_nv_quincunx->modified = qfalse;
		GLimp_SetFSAA();
	}

	// make sure the event loop is pumped
	Sys_SendKeyEvents();

	_aglSwapBuffers();
#if MAC_TIME_LIMIT
	CheckTimeLimit();
#endif
}

/*
===============
GLimp_Shutdown

===============
*/
void GLimp_Shutdown( void ) {
	if ( sys_gl.systemGammas ) {
		{
			CGGammaTable *cgGamma = (CGGammaTable *) sys_gl.systemGammas;

			CGSetDisplayTransferByTable( NULL, 256, cgGamma->red, cgGamma->green, cgGamma->blue );
			free( sys_gl.systemGammas );
		}
		sys_gl.systemGammas = 0;
	}

	_aglDisposeGameContext();

	memset( &glConfig, 0, sizeof( glConfig ) );

	if ( gDrawSprocket ) {
		delete gDrawSprocket;
	}
}

#if MAC_WOLF
Q3DEF void GLimp_LogComment( char *comment )
#else
Q3DEF void GLimp_LogComment( const char *comment )
#endif
{
	if ( sys_gl.log_fp ) {
		fprintf( sys_gl.log_fp, "%s", comment );
	}
}

/*
===============
GLimp_SetGamma

===============
*/
void        GLimp_SetGamma( unsigned char red[256],
							unsigned char green[256],
							unsigned char blue[256] ) {
	int i;

	{
		// Convert values from range 0-255 to 0.0-1.0
		for ( i = 0 ; i < 256 ; i++ )
		{
			sys_gl.gameGamma.red[i] = (float)red[i] / 256.0;
			sys_gl.gameGamma.green[i] = (float)green[i] / 256.0;
			sys_gl.gameGamma.blue[i] = (float)blue[i] / 256.0;
		}
		sHasGameGamma = true;
		{
			CGDisplayErr cgErr;

			cgErr = CGSetDisplayTransferByTable( NULL, 256, sys_gl.gameGamma.red, sys_gl.gameGamma.green, sys_gl.gameGamma.blue );

#if _DEBUG
			VID_Printf( PRINT_ALL, "CoreGraphics gamma table set, err: %d\n", cgErr );
#endif
		}
	}
}

qboolean GLimp_ChangeMode( int mode ) {
	int colorDepth, zDepth;
	int fsaaSamples;
	long value;

	// get mode info
	mode = r_mode->integer;
	VID_Printf( PRINT_ALL, "...setting mode %d:", mode );

#if MAC_JKJA
	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, mode ) )
#else
	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
#endif
	{
		VID_Printf( PRINT_ALL, " invalid mode\n" );
		return qfalse;
	}
	VID_Printf( PRINT_ALL, " %dx%d\n", glConfig.vidWidth, glConfig.vidHeight );

#if MAC_DEBUG
	VID_Printf( PRINT_ALL, "...colorbits: %d\n", r_colorbits->integer );
	VID_Printf( PRINT_ALL, "...depthbits: %d\n", r_depthbits->integer );
	VID_Printf( PRINT_ALL, "...stencilbits: %d\n", r_stencilbits->integer );
#endif

	colorDepth = r_colorbits->integer;
	if ( colorDepth == 0 ) {
		colorDepth = 16;
	}
	zDepth = r_depthbits->integer;
	if ( zDepth == 0 ) {
		zDepth = 24;
	}

	switch ( r_nv_fsaa_samples->integer )
	{
	case 2:
		fsaaSamples = 2;
		break;
	case 4:
		fsaaSamples = 4;
		break;
	case 0:
	case 1:
	default:
		fsaaSamples = 0;
		break;
	}

	if ( !_aglSetGameContext( glConfig.vidWidth, glConfig.vidHeight, colorDepth, zDepth, r_stencilbits->integer,
							  gRefreshRate, sys_gl.displayID, !r_fullscreen->integer, fsaaSamples ) ) {
//		return qfalse;
		Com_Error( ERR_FATAL, "Couldn't start OpenGL!" );
	}

	sys_gl.context = aglGetCurrentContext();
	sys_gl.device = gDrawSprocket->GetGDevice();
	sys_gl.fmt = gPixelFormat;

	// glConfig updates
//	glConfig.displayFrequency = glInfo.freq;
//	glConfig.isFullscreen = glInfo.fFullscreen;
//	glConfig.vidHeight = glInfo.height;
//	glConfig.vidWidth = glInfo.width;

	aglDescribePixelFormat( sys_gl.fmt, AGL_RED_SIZE, &value );
	glConfig.colorBits = value;
	aglDescribePixelFormat( sys_gl.fmt, AGL_GREEN_SIZE, &value );
	glConfig.colorBits += value;
	aglDescribePixelFormat( sys_gl.fmt, AGL_BLUE_SIZE, &value );
	glConfig.colorBits += value;
	aglDescribePixelFormat( sys_gl.fmt, AGL_STENCIL_SIZE, &value );
	glConfig.stencilBits = value;
	aglDescribePixelFormat( sys_gl.fmt, AGL_DEPTH_SIZE, &value );
	glConfig.depthBits = value;

	CheckErrors( __FILE__, __LINE__ );

	glConfig.isFullscreen = (qboolean) r_fullscreen->integer;

	return qtrue;
}

Q3DEF qboolean GLimp_ChangeFullscreen( qboolean fullscreen ) {
	return qfalse;
}

#pragma mark -
#pragma mark ¥ SMP Calls

#if PTHREADS
#pragma mark * pthreads

sem_t renderCommandsEvent;
sem_t renderCompletedEvent;
sem_t renderActiveEvent;

void ( *glimpRenderThread )( void );

void *GLimp_RenderThreadWrapper( void *stub ) {
	glimpRenderThread();
	return NULL;
}


/*
=======================
GLimp_SpawnRenderThread
=======================
*/
pthread_t renderThreadHandle;
Q3DEF qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {
	sem_init( &renderCommandsEvent, 0, 0 );
	sem_init( &renderCompletedEvent, 0, 0 );
	sem_init( &renderActiveEvent, 0, 0 );

	glimpRenderThread = function;

	if ( pthread_create( &renderThreadHandle, NULL, GLimp_RenderThreadWrapper, NULL ) ) {
		return qfalse;
	}

	return qtrue;
}

static void    *smpData;
static int glXErrors;

Q3DEF void *GLimp_RendererSleep( void ) {
	void    *data;

	// after this, the front end can exit GLimp_FrontEndSleep
	sem_post( &renderCompletedEvent );

	sem_wait( &renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	sem_post( &renderActiveEvent );

	return data;
}


Q3DEF void GLimp_FrontEndSleep( void ) {
	sem_wait( &renderCompletedEvent );
}


Q3DEF void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	// after this, the renderer can continue through GLimp_RendererSleep
	sem_post( &renderCommandsEvent );

	sem_wait( &renderActiveEvent );
}

#elif MPLIB

#pragma mark * MP library

MPSemaphoreID renderCommandsEvent;
MPSemaphoreID renderCompletedEvent;
MPSemaphoreID renderActiveEvent;
MPSemaphoreID g2_frontend;
MPSemaphoreID g2_backend;
MPCriticalRegionID CritID = NULL;

extern AGLContext gAGLContext_Primary;
extern AGLContext gAGLContext_Secondary;

void ( *glimpRenderThread )( void );

OSStatus GLimp_RenderThreadWrapper( void *stub ) {
	GLboolean ok;

	// Disable the primary and set up the secondary context for rendering
	_aglUseSecondaryContext();

	GL_SetDefaultState();

	GLimp_SetFSAA();

	// draw something to show that the second context is alive
	glClearColor( 0.0, 1.0, 0.0, 0 );
	glClear( GL_COLOR_BUFFER_BIT );
	GLimp_EndFrame();

	// get the extensions
//	GLimp_Extensions();

//	CheckErrors();

//	GLimp_AglDescribe_f();

	glimpRenderThread();
	return noErr;
}


/*
=======================
GLimp_SpawnRenderThread
=======================
*/
MPTaskID renderThreadTask;
Q3DEF qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {
	OSStatus status = noErr;
	ItemCount numCPU = 0;

	if ( gSystemVersion < 0x1010 ) {
		VID_Printf( PRINT_ALL, "SMP support requires MacOS 10.1 or higher.\n" );
		return qfalse;
	}

	if ( !MPLibraryIsLoaded() ) {
		return qfalse;
	}

	numCPU = MPProcessors();
	VID_Printf( PRINT_ALL, "Number of CPUs: %d\n", numCPU );

	// Disable SMP support if only 1 CPU
	if ( numCPU < 2 ) {
		return qfalse;
	}

#if MAC_JK2
	VID_Printf( PRINT_ALL, "SMP is not supported in this game.\n" );
	return qfalse;
#endif

	// We don't use MPCreateBinary Semaphore because it sets the inital value
	// of the semaphore to 1 (i.e. fired), which is not what we want.
	MPCreateSemaphore( 1, 0, &renderCommandsEvent );
	MPCreateSemaphore( 1, 0, &renderCompletedEvent );
	MPCreateSemaphore( 1, 0, &renderActiveEvent );
	MPCreateSemaphore( 1, 0, &g2_backend );
	MPCreateSemaphore( 1, 0, &g2_frontend );

	MPCreateCriticalRegion( &CritID );

	glimpRenderThread = function;

	// Flush the primary context - we're about to get rid of it
	glFinish();
	GLimp_EndFrame();

	status = MPCreateTask( GLimp_RenderThreadWrapper,(void *) NULL,0,NULL,0,0,kNilOptions, &renderThreadTask );
	if ( status != noErr ) {
		VID_Printf( PRINT_ALL, "Can't create MP task, err #%d\n", status );
		return qfalse;
	}

	return qtrue;
}

static void    *smpData;
static int glXErrors;
static Boolean asleep = false;

Q3DEF void *GLimp_RendererSleep( void ) {
	void    *data;

	// after this, the front end can exit GLimp_FrontEndSleep
	MPSignalSemaphore( renderCompletedEvent );

	// Loop indefinitely until we are asked to wake up via a renderCommandEvent
	MPWaitOnSemaphore( renderCommandsEvent, kDurationForever );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	MPSignalSemaphore( renderActiveEvent );

	return data;
}


Q3DEF void GLimp_FrontEndSleep( void ) {
	// If we're already asleep, then we don't need to wait
	// The q3 core can call this routine 2 or more times without a subsequent call
	// to WakeRenderer, which leaves the semaphores in a deadlock.
	MPEnterCriticalRegion( CritID, kDurationForever );    //DAJ
	if ( asleep ) {
		MPExitCriticalRegion( CritID );
		return;
	}

	MPWaitOnSemaphore( renderCompletedEvent, kDurationForever );
	asleep = true;
	MPExitCriticalRegion( CritID );

	// We walk a fine line here. The Q3 core will issue OpenGL commands
	// in the main thread once it has determined that the back-end renderer
	// has completed. It makes this determination by having this routine
	// return. Because of this, we're going to point the main thread context
	// to the AGL context that is set up for the second thread right now
	// and hope for the best.
	aglSetCurrentContext( gAGLContext_Secondary );
}


Q3DEF void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	// We're getting ready to return control to the renderer thread, so
	// we're going to switch the main thread context back to the primary
	// while the backend is running. We can't have the same AGL context
	// active in two different threads at once, or we risk confusing the
	// OpenGL command pipeline for that context and a potential kernel panic.
	aglSetCurrentContext( gAGLContext_Primary );

	// after this, the renderer can continue through GLimp_RendererSleep
	MPSignalSemaphore( renderCommandsEvent );

	MPWaitOnSemaphore( renderActiveEvent, kDurationForever );

	MPEnterCriticalRegion( CritID, kDurationForever );    //DAJ
	asleep = false;
	MPExitCriticalRegion( CritID );
}

#else

#pragma mark * no threading

Q3DEF qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {
	return qfalse;
}

Q3DEF void *GLimp_RendererSleep( void ) {
	return NULL;
}

Q3DEF void GLimp_FrontEndSleep( void ) {
}

Q3DEF void GLimp_WakeRenderer( void * data ) {
}

#endif
