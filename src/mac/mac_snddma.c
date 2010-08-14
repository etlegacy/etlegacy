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


// mac_snddma.c
// all other sound mixing is portable

#include "../client/snd_local.h"
#include "../qcommon/qcommon.h"
#include <Carbon/Carbon.h>

extern long gSystemVersion;

#define MAX_MIXED_SAMPLES   0x8000
#define SUBMISSION_CHUNK    0x100

static short s_mixedSamples[MAX_MIXED_SAMPLES];
static int s_chunkCount;                    // number of chunks submitted
static SndChannel      *s_sndChan;
static ExtSoundHeader s_sndHeader;
cvar_t *s_chunksize;

static int s_chunkLocal;
static UInt32 s_sampleRate;

/*
===============
S_Callback
===============
*/
pascal void S_Callback( SndChannel *sc, SndCommand *cmd ) {
	SndCommand mySndCmd;
	SndCommand mySndCmd2;
	int offset;

	offset = ( s_chunkCount * s_chunkLocal ) & ( MAX_MIXED_SAMPLES - 1 );

	// queue up another sound buffer
	memset( &s_sndHeader, 0, sizeof( s_sndHeader ) );
	s_sndHeader.samplePtr = ( char * )( s_mixedSamples + offset );
	s_sndHeader.numChannels = 2;
	s_sndHeader.sampleRate = s_sampleRate;
	s_sndHeader.loopStart = 0;
	s_sndHeader.loopEnd = 0;
	s_sndHeader.encode = extSH;
	s_sndHeader.baseFrequency = 1;
	s_sndHeader.numFrames = s_chunkLocal / 2;
	s_sndHeader.markerChunk = NULL;
	s_sndHeader.instrumentChunks = NULL;
	s_sndHeader.AESRecording = NULL;
	s_sndHeader.sampleSize = 16;

	mySndCmd.cmd = bufferCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (int)&s_sndHeader;
	SndDoCommand( sc, &mySndCmd, true );

	// and another callback
	mySndCmd2.cmd = callBackCmd;
	mySndCmd2.param1 = 0;
	mySndCmd2.param2 = 0;
	SndDoCommand( sc, &mySndCmd2, true );

	s_chunkCount++;     // this is the next buffer we will submit
}

/*
===============
S_MakeTestPattern
===============
*/
void S_MakeTestPattern( void ) {
	int i;
	float v;
	int sample;

	for ( i = 0 ; i < dma.samples / 2 ; i++ )
	{
		v = sin( M_PI * 2 * i / 64 );
		sample = v * 0x4000;
		( (short *)dma.buffer )[i * 2] = sample;
		( (short *)dma.buffer )[i * 2 + 1] = sample;
	}
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init( void ) {
	int err;

	s_chunksize = Cvar_Get( "s_chunksize", "2048", CVAR_ARCHIVE | CVAR_LATCH );
	s_chunkLocal = s_chunksize->integer;

	// create a sound channel
	s_sndChan = NULL;
	err = SndNewChannel( &s_sndChan, sampledSynth, initStereo, NewSndCallBackUPP( S_Callback ) );
	if ( err ) {
		return qfalse;
	}

	memset( (void *)&dma, 0, sizeof( dma ) );

	if ( s_khz->integer == 44 ) {
		dma.speed = 44100;
		s_sampleRate = rate44khz;
	} else if ( s_khz->integer == 22 )     {
		dma.speed = 22050;
		s_sampleRate = rate22050hz;
	} else
	{
		dma.speed = 11025;
		s_sampleRate = rate11025hz;
	}

	dma.channels = 2;
	dma.samples = MAX_MIXED_SAMPLES;
	dma.submission_chunk = s_chunkLocal;
	dma.samplebits = 16;
	dma.buffer = (byte *)s_mixedSamples;

	// que up the first submission-chunk sized buffer
	s_chunkCount = 0;

	S_Callback( s_sndChan, NULL );

	return qtrue;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos( void ) {
	return s_chunkCount * s_chunkLocal;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown( void ) {
	if ( s_sndChan ) {
		SndDisposeChannel( s_sndChan, true );
		s_sndChan = NULL;
	}
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting( void ) {
}

/*
===============
SNDDMA_Submit
===============
*/
void SNDDMA_Submit( void ) {
}
