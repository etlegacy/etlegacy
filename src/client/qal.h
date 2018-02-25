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
 * @file qal.c
 * @brief Dynamically loads OpenAL
 */

#ifndef INCLUDE_QAL_H
#define INCLUDE_QAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifdef FEATURE_OPENAL_DLOPEN
#define AL_NO_PROTOTYPES
#define ALC_NO_PROTOTYPES
#else
#define AL_ALEXT_PROTOTYPES
#endif

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

// Hack to enable compiling both on OpenAL SDK and OpenAL-soft.
#ifndef ALC_ENUMERATE_ALL_EXT
#  define ALC_ENUMERATE_ALL_EXT 1
#  define ALC_DEFAULT_ALL_DEVICES_SPECIFIER        0x1012
#  define ALC_ALL_DEVICES_SPECIFIER                0x1013
#endif

#ifdef FEATURE_OPENAL_DLOPEN
extern LPALENABLE               qalEnable;
extern LPALDISABLE              qalDisable;
extern LPALISENABLED            qalIsEnabled;
extern LPALGETSTRING            qalGetString;
extern LPALGETBOOLEANV          qalGetBooleanv;
extern LPALGETINTEGERV          qalGetIntegerv;
extern LPALGETFLOATV            qalGetFloatv;
extern LPALGETDOUBLEV           qalGetDoublev;
extern LPALGETBOOLEAN           qalGetBoolean;
extern LPALGETINTEGER           qalGetInteger;
extern LPALGETFLOAT             qalGetFloat;
extern LPALGETDOUBLE            qalGetDouble;
extern LPALGETERROR             qalGetError;
extern LPALISEXTENSIONPRESENT   qalIsExtensionPresent;
extern LPALGETPROCADDRESS       qalGetProcAddress;
extern LPALGETENUMVALUE         qalGetEnumValue;
extern LPALLISTENERF            qalListenerf;
extern LPALLISTENER3F           qalListener3f;
extern LPALLISTENERFV           qalListenerfv;
extern LPALLISTENERI            qalListeneri;
extern LPALLISTENER3I           qalListener3i;
extern LPALLISTENERIV           qalListeneriv;
extern LPALGETLISTENERF         qalGetListenerf;
extern LPALGETLISTENER3F        qalGetListener3f;
extern LPALGETLISTENERFV        qalGetListenerfv;
extern LPALGETLISTENERI         qalGetListeneri;
extern LPALGETLISTENER3I        qalGetListener3i;
extern LPALGETLISTENERIV        qalGetListeneriv;
extern LPALGENSOURCES           qalGenSources;
extern LPALDELETESOURCES        qalDeleteSources;
extern LPALISSOURCE             qalIsSource;
extern LPALSOURCEF              qalSourcef;
extern LPALSOURCE3F             qalSource3f;
extern LPALSOURCEFV             qalSourcefv;
extern LPALSOURCEI              qalSourcei;
extern LPALSOURCE3I             qalSource3i;
extern LPALSOURCEIV             qalSourceiv;
extern LPALGETSOURCEF           qalGetSourcef;
extern LPALGETSOURCE3F          qalGetSource3f;
extern LPALGETSOURCEFV          qalGetSourcefv;
extern LPALGETSOURCEI           qalGetSourcei;
extern LPALGETSOURCE3I          qalGetSource3i;
extern LPALGETSOURCEIV          qalGetSourceiv;
extern LPALSOURCEPLAYV          qalSourcePlayv;
extern LPALSOURCESTOPV          qalSourceStopv;
extern LPALSOURCEREWINDV        qalSourceRewindv;
extern LPALSOURCEPAUSEV         qalSourcePausev;
extern LPALSOURCEPLAY           qalSourcePlay;
extern LPALSOURCESTOP           qalSourceStop;
extern LPALSOURCEREWIND         qalSourceRewind;
extern LPALSOURCEPAUSE          qalSourcePause;
extern LPALSOURCEQUEUEBUFFERS   qalSourceQueueBuffers;
extern LPALSOURCEUNQUEUEBUFFERS qalSourceUnqueueBuffers;
extern LPALGENBUFFERS           qalGenBuffers;
extern LPALDELETEBUFFERS        qalDeleteBuffers;
extern LPALISBUFFER             qalIsBuffer;
extern LPALBUFFERDATA           qalBufferData;
extern LPALBUFFERF              qalBufferf;
extern LPALBUFFER3F             qalBuffer3f;
extern LPALBUFFERFV             qalBufferfv;
extern LPALBUFFERF              qalBufferi;
extern LPALBUFFER3F             qalBuffer3i;
extern LPALBUFFERFV             qalBufferiv;
extern LPALGETBUFFERF           qalGetBufferf;
extern LPALGETBUFFER3F          qalGetBuffer3f;
extern LPALGETBUFFERFV          qalGetBufferfv;
extern LPALGETBUFFERI           qalGetBufferi;
extern LPALGETBUFFER3I          qalGetBuffer3i;
extern LPALGETBUFFERIV          qalGetBufferiv;
extern LPALDOPPLERFACTOR        qalDopplerFactor;
extern LPALDOPPLERVELOCITY      qalDopplerVelocity;
extern LPALSPEEDOFSOUND         qalSpeedOfSound;
extern LPALDISTANCEMODEL        qalDistanceModel;

extern LPALCCREATECONTEXT      qalcCreateContext;
extern LPALCMAKECONTEXTCURRENT qalcMakeContextCurrent;
extern LPALCPROCESSCONTEXT     qalcProcessContext;
extern LPALCSUSPENDCONTEXT     qalcSuspendContext;
extern LPALCDESTROYCONTEXT     qalcDestroyContext;
extern LPALCGETCURRENTCONTEXT  qalcGetCurrentContext;
extern LPALCGETCONTEXTSDEVICE  qalcGetContextsDevice;
extern LPALCOPENDEVICE         qalcOpenDevice;
extern LPALCCLOSEDEVICE        qalcCloseDevice;
extern LPALCGETERROR           qalcGetError;
extern LPALCISEXTENSIONPRESENT qalcIsExtensionPresent;
extern LPALCGETPROCADDRESS     qalcGetProcAddress;
extern LPALCGETENUMVALUE       qalcGetEnumValue;
extern LPALCGETSTRING          qalcGetString;
extern LPALCGETINTEGERV        qalcGetIntegerv;
extern LPALCCAPTUREOPENDEVICE  qalcCaptureOpenDevice;
extern LPALCCAPTURECLOSEDEVICE qalcCaptureCloseDevice;
extern LPALCCAPTURESTART       qalcCaptureStart;
extern LPALCCAPTURESTOP        qalcCaptureStop;
extern LPALCCAPTURESAMPLES     qalcCaptureSamples;

extern LPALGENEFFECTS              qalGenEffects;
extern LPALEFFECTI                 qalEffecti;
extern LPALEFFECTF                 qalEffectf;
extern LPALGENAUXILIARYEFFECTSLOTS qalGenAuxiliaryEffectSlots;
extern LPALAUXILIARYEFFECTSLOTI    qalAuxiliaryEffectSloti;
#else
#define qalEnable alEnable
#define qalDisable alDisable
#define qalIsEnabled alIsEnabled
#define qalGetString alGetString
#define qalGetBooleanv alGetBooleanv
#define qalGetIntegerv alGetIntegerv
#define qalGetFloatv alGetFloatv
#define qalGetDoublev alGetDoublev
#define qalGetBoolean alGetBoolean
#define qalGetInteger alGetInteger
#define qalGetFloat alGetFloat
#define qalGetDouble alGetDouble
#define qalGetError alGetError
#define qalIsExtensionPresent alIsExtensionPresent
#define qalGetProcAddress alGetProcAddress
#define qalGetEnumValue alGetEnumValue
#define qalListenerf alListenerf
#define qalListener3f alListener3f
#define qalListenerfv alListenerfv
#define qalListeneri alListeneri
#define qalListener3i alListener3i
#define qalListeneriv alListeneriv
#define qalGetListenerf alGetListenerf
#define qalGetListener3f alGetListener3f
#define qalGetListenerfv alGetListenerfv
#define qalGetListeneri alGetListeneri
#define qalGetListener3i alGetListener3i
#define qalGetListeneriv alGetListeneriv
#define qalGenSources alGenSources
#define qalDeleteSources alDeleteSources
#define qalIsSource alIsSource
#define qalSourcef alSourcef
#define qalSource3f alSource3f
#define qalSourcefv alSourcefv
#define qalSourcei alSourcei
#define qalSource3i alSource3i
#define qalSourceiv alSourceiv
#define qalGetSourcef alGetSourcef
#define qalGetSource3f alGetSource3f
#define qalGetSourcefv alGetSourcefv
#define qalGetSourcei alGetSourcei
#define qalGetSource3i alGetSource3i
#define qalGetSourceiv alGetSourceiv
#define qalSourcePlayv alSourcePlayv
#define qalSourceStopv alSourceStopv
#define qalSourceRewindv alSourceRewindv
#define qalSourcePausev alSourcePausev
#define qalSourcePlay alSourcePlay
#define qalSourceStop alSourceStop
#define qalSourceRewind alSourceRewind
#define qalSourcePause alSourcePause
#define qalSourceQueueBuffers alSourceQueueBuffers
#define qalSourceUnqueueBuffers alSourceUnqueueBuffers
#define qalGenBuffers alGenBuffers
#define qalDeleteBuffers alDeleteBuffers
#define qalIsBuffer alIsBuffer
#define qalBufferData alBufferData
#define qalBufferf alBufferf
#define qalBuffer3f alBuffer3f
#define qalBufferfv alBufferfv
#define qalBufferi alBufferi
#define qalBuffer3i alBuffer3i
#define qalBufferiv alBufferiv
#define qalGetBufferf alGetBufferf
#define qalGetBuffer3f alGetBuffer3f
#define qalGetBufferfv alGetBufferfv
#define qalGetBufferi alGetBufferi
#define qalGetBuffer3i alGetBuffer3i
#define qalGetBufferiv alGetBufferiv
#define qalDopplerFactor alDopplerFactor
#define qalDopplerVelocity alDopplerVelocity
#define qalSpeedOfSound alSpeedOfSound
#define qalDistanceModel alDistanceModel

#define qalcCreateContext alcCreateContext
#define qalcMakeContextCurrent alcMakeContextCurrent
#define qalcProcessContext alcProcessContext
#define qalcSuspendContext alcSuspendContext
#define qalcDestroyContext alcDestroyContext
#define qalcGetCurrentContext alcGetCurrentContext
#define qalcGetContextsDevice alcGetContextsDevice
#define qalcOpenDevice alcOpenDevice
#define qalcCloseDevice alcCloseDevice
#define qalcGetError alcGetError
#define qalcIsExtensionPresent alcIsExtensionPresent
#define qalcGetProcAddress alcGetProcAddress
#define qalcGetEnumValue alcGetEnumValue
#define qalcGetString alcGetString
#define qalcGetIntegerv alcGetIntegerv
#define qalcCaptureOpenDevice alcCaptureOpenDevice
#define qalcCaptureCloseDevice alcCaptureCloseDevice
#define qalcCaptureStart alcCaptureStart
#define qalcCaptureStop alcCaptureStop
#define qalcCaptureSamples alcCaptureSamples

#define qalGenEffects alGenEffects
#define qalEffecti alEffecti
#define qalEffectf alEffectf
#define qalGenAuxiliaryEffectSlots alGenAuxiliaryEffectSlots
#define qalAuxiliaryEffectSloti alAuxiliaryEffectSloti
#endif

qboolean QAL_Init(const char *libname);
void QAL_Shutdown(void);

#endif  // #ifndef INCLUDE_QAL_H
