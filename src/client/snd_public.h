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
 * @file snd_public.h
 */

#ifndef INCLUDE_SND_PUBLIC_H
#define INCLUDE_SND_PUBLIC_H

// background track queuing
#define QUEUED_PLAY_ONCE    -1
#define QUEUED_PLAY_LOOPED  -2
#define QUEUED_PLAY_ONCE_SILENT -3  ///< when done it goes quiet

void S_Init(void);
void S_Shutdown(void);
void S_Reload(void);

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound(vec3_t origin, int entNum, int entchannel, sfxHandle_t sfxHandle, int volume);
void S_StartSoundEx(vec3_t origin, int entNum, int entchannel, sfxHandle_t sfxHandle, int flags, int volume);
void S_StartLocalSound(sfxHandle_t sfxHandle, int channelNum, int volume);

void S_StartBackgroundTrack(const char *intro, const char *loop, int fadeUpTime);
void S_StopBackgroundTrack(void);

float S_StartStreamingSound(const char *intro, const char *loop, int entNum, int channel, int attenuation);
void S_StopEntStreamingSound(int entNum);
void S_FadeStreamingSound(float targetvol, int time, int stream);

// cinematics and voice-over-network will send raw samples
// 1.0 volume will be direct output of source samples
void S_RawSamples(int stream, int samples, int rate, int width, int channels,
                  const byte *data, float lvol, float rvol);

// stop all sounds and the background track
void S_ClearSounds(qboolean clearStreaming, qboolean clearMusic);
void S_StopAllSounds(void);
void S_FadeAllSounds(float targetVol, int time, qboolean stopSounds);

// all continuous looping sounds must be added before calling S_Update
void S_ClearLoopingSounds(void);
void S_AddLoopingSound(const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfxHandle, int volume, int soundTime);
void S_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, int range, sfxHandle_t sfxHandle, int volume, int soundTime);

// recompute the reletive volumes for all running sounds
// reletive to the given entNum / orientation
void S_Respatialize(int entNum, const vec3_t origin, vec3_t axis[3], int inwater);

// let the sound system know where an entity currently is
void S_UpdateEntityPosition(int entNum, const vec3_t origin);

void S_Update(void);

void S_DisableSounds(void);

void S_BeginRegistration(void);

// RegisterSound will allways return a valid sample, even if it
// has to create a placeholder.  This prevents continuous filesystem
// checks for missing files
sfxHandle_t S_RegisterSound(const char *name, qboolean compressed);

void S_DisplayFreeMemory(void);

void S_ClearSoundBuffer(qboolean killStreaming);

int S_GetVoiceAmplitude(int entNum);

int S_GetSoundLength(sfxHandle_t sfxHandle);
int S_GetCurrentSoundTime(void);

#ifdef USE_VOIP
void S_StartCapture(void);
int S_AvailableCaptureSamples(void);
void S_Capture(int samples, byte *data);
void S_StopCapture(void);
void S_MasterGain(float gain);
#endif

#endif // #ifndef INCLUDE_SND_PUBLIC_H
