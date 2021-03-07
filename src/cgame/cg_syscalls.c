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
 * @file cg_syscalls.c
 */

#include "cg_local.h"

static intptr_t(QDECL * syscall)(intptr_t arg, ...) = (intptr_t(QDECL *)(intptr_t, ...)) - 1;

/**
 * @brief dllEntry
 */
Q_EXPORT void dllEntry(intptr_t(QDECL * syscallptr)(intptr_t arg, ...))
{
	syscall = syscallptr;
}

/**
 * @brief PASSFLOAT
 * @param[in] x
 * @return
 */
int PASSFLOAT(float x)
{
	floatint_t fi;

	fi.f = x;
	return fi.i;
}

/**
 * @brief trap_PumpEventLoop
 */
void trap_PumpEventLoop(void)
{
}

/**
 * @brief trap_Print
 * @param[in] fmt
 */
void trap_Print(const char *fmt)
{
	SystemCall(CG_PRINT, fmt);
}

/**
 * @brief trap_Error
 * @param[in] fmt
 */
void trap_Error(const char *fmt)
{
	SystemCall(CG_ERROR, fmt);
	// shut up GCC warning about returning functions, because we know better
	exit(1);
}

/**
 * @brief trap_Milliseconds
 * @return
 */
int trap_Milliseconds(void)
{
	return SystemCall(CG_MILLISECONDS);
}

/**
 * @brief trap_Cvar_Register
 * @param[in] vmCvar
 * @param[in] varName
 * @param[in] defaultValue
 * @param[in] flags
 */
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags)
{
	SystemCall(CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags);
}

/**
 * @brief trap_Cvar_Update
 * @param[in] vmCvar
 */
void trap_Cvar_Update(vmCvar_t *vmCvar)
{
	SystemCall(CG_CVAR_UPDATE, vmCvar);
}

/**
 * @brief trap_Cvar_Set
 * @param[in] varName
 * @param[in] value
 */
void trap_Cvar_Set(const char *varName, const char *value)
{
	SystemCall(CG_CVAR_SET, varName, value);
}

/**
 * @brief trap_Cvar_VariableStringBuffer
 * @param[in] varName
 * @param[out] buffer
 * @param[in] bufsize
 */
void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize)
{
	SystemCall(CG_CVAR_VARIABLESTRINGBUFFER, varName, buffer, bufsize);
}

/**
 * @brief trap_Cvar_LatchedVariableStringBuffer
 * @param[in] varName
 * @param[out] buffer
 * @param[in] bufsize
 */
void trap_Cvar_LatchedVariableStringBuffer(const char *varName, char *buffer, int bufsize)
{
	SystemCall(CG_CVAR_LATCHEDVARIABLESTRINGBUFFER, varName, buffer, bufsize);
}

/**
 * @brief trap_Argc
 * @return
 */
int trap_Argc(void)
{
	return SystemCall(CG_ARGC);
}

/**
 * @brief trap_Argv
 * @param[in] n
 * @param[out] buffer
 * @param[in] bufferLength
 */
void trap_Argv(int n, char *buffer, int bufferLength)
{
	SystemCall(CG_ARGV, n, buffer, bufferLength);
}

/**
 * @brief trap_Args
 * @param[out] buffer
 * @param[in] bufferLength
 */
void trap_Args(char *buffer, int bufferLength)
{
	SystemCall(CG_ARGS, buffer, bufferLength);
}

/**
 * @brief trap_FS_FOpenFile
 * @param[in] qpath
 * @param[in,out] f
 * @param[in] mode
 * @return
 */
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	return SystemCall(CG_FS_FOPENFILE, qpath, f, mode);
}

/**
 * @brief trap_FS_Read
 * @param[out] buffer
 * @param[in] len
 * @param[in] f
 */
void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	SystemCall(CG_FS_READ, buffer, len, f);
}

/**
 * @brief trap_FS_Write
 * @param[in] buffer
 * @param[in] len
 * @param[in] f
 */
void trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	SystemCall(CG_FS_WRITE, buffer, len, f);
}

/**
 * @brief trap_FS_FCloseFile
 * @param[in] f
 */
void trap_FS_FCloseFile(fileHandle_t f)
{
	SystemCall(CG_FS_FCLOSEFILE, f);
}

/**
 * @brief trap_FS_GetFileList
 * @param[in] path
 * @param[in] extension
 * @param[out] listbuf
 * @param[in] bufsize
 * @return
 */
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return SystemCall(CG_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

/**
 * @brief trap_FS_Delete
 * @param[in] filename
 * @return
 */
int trap_FS_Delete(const char *filename)
{
	return SystemCall(CG_FS_DELETEFILE, filename);
}

/**
 * @brief trap_SendConsoleCommand
 * @param[in] text
 */
void trap_SendConsoleCommand(const char *text)
{
	SystemCall(CG_SENDCONSOLECOMMAND, text);
}

/**
 * @brief trap_AddCommand
 * @param[in] cmdName
 */
void trap_AddCommand(const char *cmdName)
{
	SystemCall(CG_ADDCOMMAND, cmdName);
}

/**
 * @brief
 * @details Although this trap is not actually used anywhere in baseq3 game source, a reasonable assumption, based on
 * function name and parameters, is that this trap removes a command name from the list of command completions.
 * That is, the inverse of trap_AddCommand().
 *
 * Unknown: case sensitivity (i.e. if capitalization matters for string matching)
 * @param[in] cmdName command name
 */
void trap_RemoveCommand(const char *cmdName)
{
	SystemCall(CG_REMOVECOMMAND, cmdName);
}

/**
 * @brief trap_SendClientCommand
 * @param[in] s
 */
void trap_SendClientCommand(const char *s)
{
	SystemCall(CG_SENDCLIENTCOMMAND, s);
}

/**
 * @brief trap_UpdateScreen
 */
void trap_UpdateScreen(void)
{
	SystemCall(CG_UPDATESCREEN);
}

/**
 * @brief trap_CM_NumInlineModels
 * @return
 */
int trap_CM_NumInlineModels(void)
{
	return SystemCall(CG_CM_NUMINLINEMODELS);
}

/**
 * @brief trap_CM_InlineModel
 * @param[in] index
 * @return
 */
clipHandle_t trap_CM_InlineModel(int index)
{
	return SystemCall(CG_CM_INLINEMODEL, index);
}

/**
 * @brief trap_CM_TempBoxModel
 * @param[in] mins
 * @param[in] maxs
 * @return
 */
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs)
{
	return SystemCall(CG_CM_TEMPBOXMODEL, mins, maxs);
}

/**
 * @brief trap_CM_TempCapsuleModel
 * @param[in] mins
 * @param[in] maxs
 * @return
 */
clipHandle_t trap_CM_TempCapsuleModel(const vec3_t mins, const vec3_t maxs)
{
	return SystemCall(CG_CM_TEMPCAPSULEMODEL, mins, maxs);
}

/**
 * @brief trap_CM_PointContents
 * @param[in] p
 * @param[in] model
 * @return
 */
int trap_CM_PointContents(const vec3_t p, clipHandle_t model)
{
	return SystemCall(CG_CM_POINTCONTENTS, p, model);
}

/**
 * @brief trap_CM_TransformedPointContents
 * @param[in] p
 * @param[in] model
 * @param[in] origin
 * @param[in] angles
 * @return
 */
int trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles)
{
	return SystemCall(CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles);
}

/**
 * @brief trap_CM_BoxTrace
 * @param[out] results
 * @param[in] start
 * @param[in] end
 * @param[in] mins
 * @param[in] maxs
 * @param[in] model
 * @param[in] brushmask
 */
void trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
                      const vec3_t mins, const vec3_t maxs,
                      clipHandle_t model, int brushmask)
{
	SystemCall(CG_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask);
}

/**
 * @brief trap_CM_TransformedBoxTrace
 * @param[out] results
 * @param[in] start
 * @param[in] end
 * @param[in] mins
 * @param[in] maxs
 * @param[in] model
 * @param[in] brushmask
 * @param[in] origin
 * @param[in] angles
 */
void trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                 const vec3_t mins, const vec3_t maxs,
                                 clipHandle_t model, int brushmask,
                                 const vec3_t origin, const vec3_t angles)
{
	SystemCall(CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles);
}

/**
 * @brief trap_CM_CapsuleTrace
 * @param[out] results
 * @param[in] start
 * @param[in] end
 * @param[in] mins
 * @param[in] maxs
 * @param[in] model
 * @param[in] brushmask
 */
void trap_CM_CapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
                          const vec3_t mins, const vec3_t maxs,
                          clipHandle_t model, int brushmask)
{
	SystemCall(CG_CM_CAPSULETRACE, results, start, end, mins, maxs, model, brushmask);
}

/**
 * @brief trap_CM_TransformedCapsuleTrace
 * @param[out] results
 * @param[in] start
 * @param[in] end
 * @param[in] mins
 * @param[in] maxs
 * @param[in] model
 * @param[in] brushmask
 * @param[in] origin
 * @param[in] angles
 */
void trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                     const vec3_t mins, const vec3_t maxs,
                                     clipHandle_t model, int brushmask,
                                     const vec3_t origin, const vec3_t angles)
{
	SystemCall(CG_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs, model, brushmask, origin, angles);
}

/**
 * @brief trap_CM_MarkFragments
 * @param[in] numPoints
 * @param[in] points
 * @param[in] projection
 * @param[in] maxPoints
 * @param[in] pointBuffer
 * @param[in] maxFragments
 * @param[in] fragmentBuffer
 * @return
 */
int trap_CM_MarkFragments(int numPoints, const vec3_t *points,
                          const vec3_t projection,
                          int maxPoints, vec3_t pointBuffer,
                          int maxFragments, markFragment_t *fragmentBuffer)
{
	return SystemCall(CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer);
}

/**
 * @brief trap_R_ProjectDecal
 * @param[in] hShader
 * @param[in] numPoints
 * @param[in] points
 * @param[in] projection
 * @param[in] color
 * @param[in] lifeTime
 * @param[in] fadeTime
 */
void trap_R_ProjectDecal(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime)
{
	SystemCall(CG_R_PROJECTDECAL, hShader, numPoints, points, projection, color, lifeTime, fadeTime);
}

/**
 * @brief trap_R_ClearDecals
 */
void trap_R_ClearDecals(void)
{
	SystemCall(CG_R_CLEARDECALS);
}

/**
 * @brief Starts a sound
 * @param[in] origin position or NULL for position of player entity (see entityNum)
 * @param[in] entityNum
 * @param[in] entchannel
 * @param[in] sfx
 */
void trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx)
{
#ifdef FEATURE_EDV
	//explicitly respatialize all local sounds in freecam
	if ((cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam) && entityNum == cg.snap->ps.clientNum)
	{
		SystemCall(CG_S_STARTSOUND, cg.snap->ps.origin, -1, entchannel, sfx, 127 /* Gordon: default volume always for the moment*/);
	}
	else
	{
#endif
	SystemCall(CG_S_STARTSOUND, origin, entityNum, entchannel, sfx, 127 /* default volume always for the moment*/);
#ifdef FEATURE_EDV
}
#endif
}

/**
 * @brief trap_S_StartSoundVControl
 * @param[in] origin
 * @param[in] entityNum
 * @param[in] entchannel
 * @param[in] sfx
 * @param[in] volume
 */
void trap_S_StartSoundVControl(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int volume)
{
#ifdef FEATURE_EDV
	//explicitly respatialize all local sounds in freecam
	if ((cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam) && entityNum == cg.snap->ps.clientNum)
	{
		SystemCall(CG_S_STARTSOUND, cg.snap->ps.origin, -1, entchannel, sfx, 127 /* Gordon: default volume always for the moment*/);
	}
	else
	{
#endif
	SystemCall(CG_S_STARTSOUND, origin, entityNum, entchannel, sfx, volume);
#ifdef FEATURE_EDV
}
#endif
}

/**
 * @brief trap_S_StartSoundEx
 * @param[in] origin
 * @param[in] entityNum
 * @param[in] entchannel
 * @param[in] sfx
 * @param[in] flags
 */
void trap_S_StartSoundEx(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags)
{
#ifdef FEATURE_EDV
	//explicitly respatialize all local sounds in freecam
	if ((cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam) && entityNum == cg.snap->ps.clientNum)
	{
		SystemCall(CG_S_STARTSOUND, cg.snap->ps.origin, -1, entchannel, sfx, 127 /* Gordon: default volume always for the moment*/);
	}
	else
	{
#endif
	SystemCall(CG_S_STARTSOUNDEX, origin, entityNum, entchannel, sfx, flags, 127 /* default volume always for the moment*/);
#ifdef FEATURE_EDV
}
#endif
}

/**
 * @brief trap_S_StartSoundExVControl
 * @param[in] origin
 * @param[in] entityNum
 * @param[in] entchannel
 * @param[in] sfx
 * @param[in] flags
 * @param[in] volume
 */
void trap_S_StartSoundExVControl(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags, int volume)
{
	SystemCall(CG_S_STARTSOUNDEX, origin, entityNum, entchannel, sfx, flags, volume);
}

/**
 * @brief trap_S_StartLocalSound
 * @param[in] sfx
 * @param[in] channelNum
 *
 * @note 127 Default volume always for the moment
 */
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum)
{
	SystemCall(CG_S_STARTLOCALSOUND, sfx, channelNum, 127);
}

/**
 * @brief trap_S_ClearLoopingSounds
 */
void trap_S_ClearLoopingSounds(void)
{
	SystemCall(CG_S_CLEARLOOPINGSOUNDS);
}

/**
 * @brief trap_S_ClearSounds
 * @param[in] killmusic
 */
void trap_S_ClearSounds(qboolean killmusic)
{
	SystemCall(CG_S_CLEARSOUNDS, killmusic);
}

/**
 * @brief trap_S_AddLoopingSound
 * @param[in] origin
 * @param[in] velocity
 * @param[in] sfx
 * @param[in] volume
 * @param[in] soundTime
 *
 * @note Volume was previously removed from CG_S_ADDLOOPINGSOUND.  I added 'range'
 */
void trap_S_AddLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume, int soundTime)
{
	SystemCall(CG_S_ADDLOOPINGSOUND, origin, velocity, 1250, sfx, volume, soundTime);
}

/**
 * @brief trap_S_AddRealLoopingSound
 * @param[in] origin
 * @param[in] velocity
 * @param[in] sfx
 * @param[in] range
 * @param[in] volume
 * @param[in] soundTime
 */
void trap_S_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range, int volume, int soundTime)
{
	SystemCall(CG_S_ADDREALLOOPINGSOUND, origin, velocity, range, sfx, volume, soundTime);
}

/**
 * @brief trap_S_StopStreamingSound
 * @param[in] entityNum
 */
void trap_S_StopStreamingSound(int entityNum)
{
	SystemCall(CG_S_STOPSTREAMINGSOUND, entityNum);
}

/**
 * @brief trap_S_UpdateEntityPosition
 * @param[in] entityNum
 * @param[in] origin
 */
void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin)
{
	SystemCall(CG_S_UPDATEENTITYPOSITION, entityNum, origin);
}

/**
 * @brief Talking animations
 * @param[in] entityNum
 * @return
 */
int trap_S_GetVoiceAmplitude(int entityNum)
{
	return SystemCall(CG_S_GETVOICEAMPLITUDE, entityNum);
}

/**
 * @brief trap_S_Respatialize
 * @param[in] entityNum
 * @param[in] origin
 * @param[in] axis
 * @param[in] inwater
 */
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater)
{
	SystemCall(CG_S_RESPATIALIZE, entityNum, origin, axis, inwater);
}

/**
 * @brief trap_S_GetSoundLength
 * @param[in] sfx
 * @return
 */
int trap_S_GetSoundLength(sfxHandle_t sfx)
{
	return SystemCall(CG_S_GETSOUNDLENGTH, sfx);
}

/**
 * @brief For timing looped sounds
 * @return
 */
int trap_S_GetCurrentSoundTime(void)
{
	return SystemCall(CG_S_GETCURRENTSOUNDTIME);
}

/**
 * @brief trap_S_StartBackgroundTrack
 * @param[in] intro
 * @param[in] loop
 * @param[in] fadeupTime
 */
void trap_S_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime)
{
	SystemCall(CG_S_STARTBACKGROUNDTRACK, intro, loop, fadeupTime);
}

/**
 * @brief trap_S_FadeBackgroundTrack
 * @param[in] targetvol
 * @param[in] time
 * @param[in] num
 *
 * @note yes, i know.  fadebackground coming in, fadestreaming going out.  will have to see where functionality leads...
 * @note 'num' is '0' if it's music, '1' if it's "all streaming sounds"
 */
void trap_S_FadeBackgroundTrack(float targetvol, int time, int num)
{
	SystemCall(CG_S_FADESTREAMINGSOUND, PASSFLOAT(targetvol), time, num);
}

/**
 * @brief trap_S_FadeAllSound
 * @param[in] targetvol
 * @param[in] time
 * @param[in] stopsounds
 */
void trap_S_FadeAllSound(float targetvol, int time, qboolean stopsounds)
{
	SystemCall(CG_S_FADEALLSOUNDS, PASSFLOAT(targetvol), time, stopsounds);
}

/**
 * @brief trap_S_StartStreamingSound
 * @param[in] intro
 * @param[in] loop
 * @param[in] entnum
 * @param[in] channel
 * @param[in] attenuation
 * @return
 */
int trap_S_StartStreamingSound(const char *intro, const char *loop, int entnum, int channel, int attenuation)
{
	return SystemCall(CG_S_STARTSTREAMINGSOUND, intro, loop, entnum, channel, attenuation);
}

/**
 * @brief trap_R_GetSkinModel
 * @param[in] skinid
 * @param[in] type
 * @param[out] name
 * @return
 */
qboolean trap_R_GetSkinModel(qhandle_t skinid, const char *type, char *name)
{
	return (qboolean)(SystemCall(CG_R_GETSKINMODEL, skinid, type, name));
}

/**
 * @brief trap_R_GetShaderFromModel
 * @param[in] modelid
 * @param[in] surfnum
 * @param[in] withlightmap
 * @return
 */
qhandle_t trap_R_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap)
{
	return SystemCall(CG_R_GETMODELSHADER, modelid, surfnum, withlightmap);
}

/**
 * @brief trap_R_ClearScene
 */
void trap_R_ClearScene(void)
{
	SystemCall(CG_R_CLEARSCENE);
}

/**
 * @brief trap_R_AddRefEntityToScene
 * @param re
 */
void trap_R_AddRefEntityToScene(const refEntity_t *re)
{
	SystemCall(CG_R_ADDREFENTITYTOSCENE, re);
}

/**
 * @brief trap_R_AddPolyToScene
 * @param[in] hShader
 * @param[in] numVerts
 * @param[in] verts
 */
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts)
{
	SystemCall(CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts);
}

/**
 * @brief trap_R_AddPolyBufferToScene
 * @param[in] pPolyBuffer
 */
void trap_R_AddPolyBufferToScene(polyBuffer_t *pPolyBuffer)
{
	SystemCall(CG_R_ADDPOLYBUFFERTOSCENE, pPolyBuffer);
}

/**
 * @brief trap_R_AddPolysToScene
 * @param[in] hShader
 * @param[in] numVerts
 * @param[in] verts
 * @param[in] numPolys
 */
void trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys)
{
	SystemCall(CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys);
}

/**
 * @brief New dlight system
 * @param[in] org
 * @param[in] radius
 * @param[in] intensity
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] hShader
 * @param[in] flags
 */
void trap_R_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags)
{
	SystemCall(CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT(radius), PASSFLOAT(intensity),
	        PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), hShader, flags);
}

/**
 * @brief trap_R_AddCoronaToScene
 * @param[in] org
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] scale
 * @param[in] id
 * @param[in] visible
 */
void trap_R_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible)
{
	SystemCall(CG_R_ADDCORONATOSCENE, org, PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), PASSFLOAT(scale), id, visible);
}

/**
 * @brief trap_R_SetFog
 * @param[in] fogvar
 * @param[in] var1
 * @param[in] var2
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] density
 */
void trap_R_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density)
{
	SystemCall(CG_R_SETFOG, fogvar, var1, var2, PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), PASSFLOAT(density));
}

/**
 * @brief trap_R_SetGlobalFog
 * @param[in] restore
 * @param[in] duration
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] depthForOpaque
 */
void trap_R_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque)
{
	SystemCall(CG_R_SETGLOBALFOG, restore, duration, PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), PASSFLOAT(depthForOpaque));
}

/**
 * @brief trap_R_RenderScene
 * @param[in] fd
 */
void trap_R_RenderScene(const refdef_t *fd)
{
	SystemCall(CG_R_RENDERSCENE, fd);
}

/**
 * @brief trap_R_SaveViewParms
 */
void trap_R_SaveViewParms()
{
	SystemCall(CG_R_SAVEVIEWPARMS);
}

/**
 * @brief trap_R_RestoreViewParms
 */
void trap_R_RestoreViewParms()
{
	SystemCall(CG_R_RESTOREVIEWPARMS);
}

/**
 * @brief trap_R_SetColor
 * @param[in] rgba
 */
void trap_R_SetColor(const float *rgba)
{
	SystemCall(CG_R_SETCOLOR, rgba);
}

/**
 * @brief trap_R_DrawStretchPic
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 */
void trap_R_DrawStretchPic(float x, float y, float w, float h,
                           float s1, float t1, float s2, float t2, qhandle_t hShader)
{
	SystemCall(CG_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader);
}

/**
 * @brief trap_R_DrawRotatedPic
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 * @param[in] angle
 */
void trap_R_DrawRotatedPic(float x, float y, float w, float h,
                           float s1, float t1, float s2, float t2, qhandle_t hShader, float angle)
{
	SystemCall(CG_R_DRAWROTATEDPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader, PASSFLOAT(angle));
}

/**
 * @brief trap_R_DrawStretchPicGradient
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 * @param[in] gradientColor
 * @param[in] gradientType
 */
void trap_R_DrawStretchPicGradient(float x, float y, float w, float h,
                                   float s1, float t1, float s2, float t2, qhandle_t hShader,
                                   const float *gradientColor, int gradientType)
{
	SystemCall(CG_R_DRAWSTRETCHPIC_GRADIENT, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader, gradientColor, gradientType);
}

/**
 * @brief trap_R_Add2dPolys
 * @param[in] verts
 * @param[in] numverts
 * @param[in] hShader
 */
void trap_R_Add2dPolys(polyVert_t *verts, int numverts, qhandle_t hShader)
{
	SystemCall(CG_R_DRAW2DPOLYS, verts, numverts, hShader);
}

/**
 * @brief trap_R_ModelBounds
 * @param[in] model
 * @param[in] mins
 * @param[in] maxs
 */
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs)
{
	SystemCall(CG_R_MODELBOUNDS, model, mins, maxs);
}

/**
 * @brief trap_R_LerpTag
 * @param[in] tag
 * @param[in] refent
 * @param[in] tagName
 * @param[in] startIndex
 * @return
 */
int trap_R_LerpTag(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex)
{
	return SystemCall(CG_R_LERPTAG, tag, refent, tagName, startIndex);
}

/**
 * @brief trap_R_RemapShader
 * @param[in] oldShader
 * @param[in] newShader
 * @param[in] timeOffset
 */
void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset)
{
	SystemCall(CG_R_REMAP_SHADER, oldShader, newShader, timeOffset);
}

/**
 * @brief trap_GetGlconfig
 * @param[out] glconfig
 */
void trap_GetGlconfig(glconfig_t *glconfig)
{
	SystemCall(CG_GETGLCONFIG, glconfig);
}

/**
 * @brief trap_GetGameState
 * @param[out] gamestate
 */
void trap_GetGameState(gameState_t *gamestate)
{
	SystemCall(CG_GETGAMESTATE, gamestate);
}

#ifdef ETLEGACY_DEBUG
//#define FAKELAG
#ifdef FAKELAG
#define MAX_SNAPSHOT_BACKUP 256
#define MAX_SNAPSHOT_MASK   (MAX_SNAPSHOT_BACKUP - 1)

static snapshot_t snaps[MAX_SNAPSHOT_BACKUP];
static int        curSnapshotNumber;
int               snapshotDelayTime;
static qboolean   skiponeget;
#endif // FAKELAG
#endif // ETLEGACY_DEBUG

/**
 * @brief trap_GetCurrentSnapshotNumber
 * @param[out] snapshotNumber
 * @param[out] serverTime
 */
void trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime)
{
	SystemCall(CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime);

#ifdef FAKELAG
	{
		char s[MAX_STRING_CHARS];
		int  fakeLag;

		trap_Cvar_VariableStringBuffer("g_fakelag", s, sizeof(s));
		fakeLag = Q_atoi(s);
		if (fakeLag < 0)
		{
			fakeLag = 0;
		}

		if (fakeLag)
		{
			if (curSnapshotNumber < cg.latestSnapshotNum)
			{
				*snapshotNumber   = cg.latestSnapshotNum + 1;
				curSnapshotNumber = cg.latestSnapshotNum + 2;   // skip one ahead and we're good to go on the next frame
				skiponeget        = qtrue;
			}
			else
			{
				*snapshotNumber = curSnapshotNumber;
			}
		}
	}
#endif // FAKELAG
}

/**
 * @brief trap_GetSnapshot
 * @param[in] snapshotNumber
 * @param[out] snapshot
 * @return
 */
qboolean trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot)
{
#ifndef FAKELAG
	return (qboolean)(SystemCall(CG_GETSNAPSHOT, snapshotNumber, snapshot));
#else
	{
		char s[MAX_STRING_CHARS];
		int  fakeLag;

		if (skiponeget)
		{
			SystemCall(CG_GETSNAPSHOT, snapshotNumber, snapshot);
		}

		trap_Cvar_VariableStringBuffer("g_fakelag", s, sizeof(s));
		fakeLag = Q_atoi(s);
		if (fakeLag < 0)
		{
			fakeLag = 0;
		}

		if (fakeLag)
		{
			int i;
			int realsnaptime, thissnaptime;

			// store our newest usercmd
			curSnapshotNumber++;
			Com_Memcpy(&snaps[curSnapshotNumber & MAX_SNAPSHOT_MASK], snapshot, sizeof(snapshot_t));

			// find a usercmd that is fakeLag msec behind
			i            = curSnapshotNumber & MAX_SNAPSHOT_MASK;
			realsnaptime = snaps[i].serverTime;
			i--;
			do
			{
				thissnaptime = snaps[i & MAX_SNAPSHOT_MASK].serverTime;

				if (realsnaptime - thissnaptime > fakeLag)
				{
					// found the right one
					snapshotDelayTime = realsnaptime - thissnaptime;
					snapshot          = &snaps[i & MAX_SNAPSHOT_MASK];
					//*snapshotNumber = i & MAX_SNAPSHOT_MASK;
					return qtrue;
				}

				i--;
			}
			while ((i & MAX_SNAPSHOT_MASK) != (curSnapshotNumber & MAX_SNAPSHOT_MASK));

			// didn't find a proper one, just use the oldest one we have
			snapshotDelayTime = realsnaptime - thissnaptime;
			snapshot          = &snaps[(curSnapshotNumber - 1) & MAX_SNAPSHOT_MASK];
			//*snapshotNumber = (curSnapshotNumber - 1) & MAX_SNAPSHOT_MASK;
			return qtrue;
		}
		else
		{
			return SystemCall(CG_GETSNAPSHOT, snapshotNumber, snapshot);
		}
	}
#endif // FAKELAG
}

/**
 * @brief trap_GetServerCommand
 * @param[in] serverCommandNumber
 * @return
 */
qboolean trap_GetServerCommand(int serverCommandNumber)
{
	return (qboolean)(SystemCall(CG_GETSERVERCOMMAND, serverCommandNumber));
}

/**
 * @brief trap_GetCurrentCmdNumber
 * @return
 */
int trap_GetCurrentCmdNumber(void)
{
	return SystemCall(CG_GETCURRENTCMDNUMBER);
}

/**
 * @brief trap_GetUserCmd
 * @param[in] cmdNumber
 * @param[out] ucmd
 * @return
 */
qboolean trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd)
{
	return (qboolean)(SystemCall(CG_GETUSERCMD, cmdNumber, ucmd));
}

/**
 * @brief trap_SetUserCmdValue
 * @param[in] stateValue
 * @param[in] flags
 * @param[in] sensitivityScale
 * @param[in] mpIdentClient
 */
void trap_SetUserCmdValue(int stateValue, int flags, float sensitivityScale, int mpIdentClient)
{
	SystemCall(CG_SETUSERCMDVALUE, stateValue, flags, PASSFLOAT(sensitivityScale), mpIdentClient);
}

/**
 * @brief trap_SetClientLerpOrigin
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void trap_SetClientLerpOrigin(float x, float y, float z)
{
	SystemCall(CG_SETCLIENTLERPORIGIN, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(z));
}

/**
 * @brief testPrintInt
 * @param[out] string
 * @param[in] i
 */
void testPrintInt(char *string, int i)
{
	SystemCall(CG_TESTPRINTINT, string, i);
}

/**
 * @brief testPrintFloat
 * @param[out] string
 * @param[in] f
 */
void testPrintFloat(char *string, float f)
{
	SystemCall(CG_TESTPRINTFLOAT, string, PASSFLOAT(f));
}

/**
 * @brief trap_MemoryRemaining
 * @return
 */
int trap_MemoryRemaining(void)
{
	return SystemCall(CG_MEMORY_REMAINING);
}

/**
 * @brief trap_loadCamera
 * @param camNum - unused
 * @param name - unused
 * @return
 *
 * @note Unused
 */
qboolean trap_loadCamera(int camNum, const char *name)
{
	return 0;
}

/**
 * @brief trap_startCamera
 * @param camNum - unused
 * @param time - unused
 *
 * @note Unused
 */
void trap_startCamera(int camNum, int time)
{
}

/**
 * @brief trap_stopCamera
 * @param camNum - unused
 *
 * @note Unused
 */
void trap_stopCamera(int camNum)
{
}

/**
 * @brief trap_getCameraInfo
 * @param camNum - unused
 * @param time   - unused
 * @param origin - unused
 * @param angles - unused
 * @param fov  - unused
 * @return
 *
 * @note Unused
 */
qboolean trap_getCameraInfo(int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov)
{
	return 0;
}

/**
 * @brief trap_Key_IsDown
 * @param[in] keynum
 * @return
 */
qboolean trap_Key_IsDown(int keynum)
{
	return (qboolean)(SystemCall(CG_KEY_ISDOWN, keynum));
}

/**
 * @brief trap_Key_GetCatcher
 * @return
 */
int trap_Key_GetCatcher(void)
{
	return SystemCall(CG_KEY_GETCATCHER);
}

/**
 * @brief trap_Key_GetOverstrikeMode
 * @return
 */
qboolean trap_Key_GetOverstrikeMode(void)
{
	return (qboolean)(SystemCall(CG_KEY_GETOVERSTRIKEMODE));
}

/**
 * @brief trap_Key_SetOverstrikeMode
 * @param[in] state
 */
void trap_Key_SetOverstrikeMode(qboolean state)
{
	SystemCall(CG_KEY_SETOVERSTRIKEMODE, state);
}

/**
 * @brief trap_Key_KeysForBinding
 * @param[in] binding
 * @param[in,out] key1
 * @param[in,out] key2
 *
 * @note binding MUST be lower case
 */
void trap_Key_KeysForBinding(const char *binding, int *key1, int *key2)
{
	SystemCall(CG_KEY_BINDINGTOKEYS, binding, key1, key2);
}

/**
 * @brief trap_Key_SetCatcher
 * @param[in] catcher
 */
void trap_Key_SetCatcher(int catcher)
{
	SystemCall(CG_KEY_SETCATCHER, catcher);
}

/**
 * @brief trap_Key_GetKey
 * @param[in] binding
 * @return
 */
int trap_Key_GetKey(const char *binding)
{
	return SystemCall(CG_KEY_GETKEY, binding);
}

/**
 * @brief trap_PC_AddGlobalDefine
 * @param[in] define
 * @return
 */
int trap_PC_AddGlobalDefine(char *define)
{
	return SystemCall(CG_PC_ADD_GLOBAL_DEFINE, define);
}

/**
 * @brief trap_PC_LoadSource
 * @param[in] filename
 * @return
 */
int trap_PC_LoadSource(const char *filename)
{
	return SystemCall(CG_PC_LOAD_SOURCE, filename);
}

/**
 * @brief trap_PC_FreeSource
 * @param[in] handle
 * @return
 */
int trap_PC_FreeSource(int handle)
{
	return SystemCall(CG_PC_FREE_SOURCE, handle);
}

/**
 * @brief trap_PC_ReadToken
 * @param[in] handle
 * @param[out] pc_token
 * @return
 */
int trap_PC_ReadToken(int handle, pc_token_t *pc_token)
{
	return SystemCall(CG_PC_READ_TOKEN, handle, pc_token);
}

/**
 * @brief trap_PC_SourceFileAndLine
 * @param[in] handle
 * @param[out] filename
 * @param[out] line
 * @return
 */
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	return SystemCall(CG_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

/**
 * @brief trap_PC_UnReadToken
 * @param[in] handle
 * @return
 */
int trap_PC_UnReadToken(int handle)
{
	return SystemCall(CG_PC_UNREAD_TOKEN, handle);
}

/**
 * @brief trap_S_StopBackgroundTrack
 */
void trap_S_StopBackgroundTrack(void)
{
	SystemCall(CG_S_STOPBACKGROUNDTRACK);
}

/**
 * @brief trap_RealTime
 * @param[out] qtime
 * @return
 */
int trap_RealTime(qtime_t *qtime)
{
	return SystemCall(CG_REAL_TIME, qtime);
}

/**
 * @brief trap_SnapVector
 * @param[out] v
 */
void trap_SnapVector(float *v)
{
	SystemCall(CG_SNAPVECTOR, v);
}

/**
 * @brief This returns a handle.
 *
 * @details arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
 *
 * @param[in] arg0
 * @param[in] xpos
 * @param[in] ypos
 * @param[in] width
 * @param[in] height
 * @param[in] bits
 * @return
 */
int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits)
{
	return SystemCall(CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}


/**
 * @brief Stops playing the cinematic and ends it.  should always return FMV_EOF
 * cinematics must be stopped in reverse order of when they are started
 * @param[in] handle
 * @return
 */
e_status trap_CIN_StopCinematic(int handle)
{
	return (e_status)(SystemCall(CG_CIN_STOPCINEMATIC, handle));
}

/**
 * @brief Will run a frame of the cinematic but will not draw it.
 * Will return FMV_EOF if the end of the cinematic has been reached.
 * @param[in] handle
 * @return
 */
e_status trap_CIN_RunCinematic(int handle)
{
	return (e_status)(SystemCall(CG_CIN_RUNCINEMATIC, handle));
}

/**
 * @brief Draws the current frame
 * @param handle
 */
void trap_CIN_DrawCinematic(int handle)
{
	SystemCall(CG_CIN_DRAWCINEMATIC, handle);
}

/**
 * @brief Allows you to resize the animation dynamically
 * @param[in] handle
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 */
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h)
{
	SystemCall(CG_CIN_SETEXTENTS, handle, x, y, w, h);
}

/**
 * @brief trap_GetEntityToken
 * @param[out] buffer
 * @param[in] bufferSize
 * @return
 */
qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	return (qboolean)(SystemCall(CG_GET_ENTITY_TOKEN, buffer, bufferSize));
}

/**
 * @brief trap_UI_Popup
 * @param[in] arg0
 */
void trap_UI_Popup(int arg0)
{
	SystemCall(CG_INGAME_POPUP, arg0);
}

/**
 * @brief trap_UI_ClosePopup
 * @param arg0 - unused
 *
 * @note TODO: this function is empty and is NEVER used
 */
void trap_UI_ClosePopup(const char *arg0)
{
}

/**
 * @brief trap_Key_GetBindingBuf
 * @param[in] keynum
 * @param[in] buf
 * @param[in] buflen
 */
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen)
{
	SystemCall(CG_KEY_GETBINDINGBUF, keynum, buf, buflen);
}

/**
 * @brief trap_Key_SetBinding
 * @param[in] keynum
 * @param[in] binding
 */
void trap_Key_SetBinding(int keynum, const char *binding)
{
	SystemCall(CG_KEY_SETBINDING, keynum, binding);
}

/**
 * @brief trap_Key_KeynumToStringBuf
 * @param[in] keynum
 * @param[out] buf
 * @param[in] buflen
 */
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen)
{
	SystemCall(CG_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen);
}

/**
 * @brief trap_TranslateString
 * @param[in] string
 * @param[out] buf
 */
void trap_TranslateString(const char *string, char *buf)
{
	SystemCall(CG_TRANSLATE_STRING, string, buf);
}

// Media register functions

// FIXME: unique debug macros
//#define ETLEGACY_DEBUG

#ifdef ETLEGACY_DEBUG
#define DEBUG_REGISTERPROFILE_INIT int dbgTime = trap_Milliseconds();
#define DEBUG_REGISTERPROFILE_EXEC(f, n) if (developer.integer) { CG_Printf("%s : loaded %s in %i msec\n", f, n, trap_Milliseconds() - dbgTime); }

/**
 * @brief trap_S_RegisterSound
 * @param[in] sample
 * @param[in] compressed
 * @return
 */
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed)
{
	sfxHandle_t snd;
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	snd = SystemCall(CG_S_REGISTERSOUND, sample, compressed);
	if (!sample || !sample[0])
	{
		Com_Printf("^1trap_S_RegisterSound: Null sample filename\n");
	}
	if (snd == 0)
	{
		// exclude false positive blank sound (sound index = 0)
		if (Q_stricmp(sample, "sound/player/default/blank.wav"))
		{
			Com_Printf("^1trap_S_RegisterSound: Failed to load sound: '%s'\n", sample);
		}
	}
	else
	{
		CG_DPrintf("^2trap_S_RegisterSound: register sound: '%s'\n", sample);
	}
	DEBUG_REGISTERPROFILE_EXEC("trap_S_RegisterSound", sample)
	return snd;
}

/**
 * @brief trap_R_RegisterModel
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterModel(const char *name)
{
	qhandle_t handle;
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	handle = SystemCall(CG_R_REGISTERMODEL, name);
	if (!name || !name[0])
	{
		Com_Printf("^1trap_R_RegisterModel: Null or empty name\n");
	}
	if (handle == 0)
	{
		Com_Printf("^1trap_R_RegisterModel: Failed to load model: '%s'\n", name);
	}
	else
	{
		CG_DPrintf("^2trap_R_RegisterModel: register model: '%s'\n", name);
	}
	DEBUG_REGISTERPROFILE_EXEC("trap_R_RegisterModel", name)
	return handle;
}

/**
 * @brief trap_R_RegisterSkin
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterSkin(const char *name)
{
	qhandle_t handle;
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	handle = SystemCall(CG_R_REGISTERSKIN, name);
	if (!name || !name[0])
	{
		Com_Printf("^1trap_R_RegisterSkin: Null or empty name\n");
	}
	if (handle == 0)
	{

		Com_Printf("^1trap_R_RegisterSkin: Failed to load skin: '%s'\n", name);
	}
	else
	{
		CG_DPrintf("^2trap_R_RegisterSkin: register skin: '%s'\n", name);
	}
	DEBUG_REGISTERPROFILE_EXEC("trap_R_RegisterSkin", name)
	return handle;
}

/**
 * @brief trap_R_RegisterShader
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterShader(const char *name)
{
	qhandle_t handle;
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	handle = SystemCall(CG_R_REGISTERSHADER, name);
	if (!name || !name[0])
	{
		Com_Printf("^1trap_R_RegisterShader: Null or empty name\n");
	}
	if (handle == 0)
	{
		Com_Printf("^1trap_R_RegisterShader: Failed to load shader: '%s'\n", name);
	}
	else
	{
		CG_DPrintf("^2trap_R_RegisterShader: register shader: '%s'\n", name);
	}
	DEBUG_REGISTERPROFILE_EXEC("trap_R_RegisterShader", name)
	return handle;
}

/**
 * @brief trap_R_RegisterShaderNoMip
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterShaderNoMip(const char *name)
{
	qhandle_t handle;
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	handle = SystemCall(CG_R_REGISTERSHADERNOMIP, name);
	if (!name || !name[0])
	{
		Com_Printf("^1trap_R_RegisterShaderNoMip: Null or empty name\n");
	}
	if (handle == 0)
	{
		Com_Printf("^1trap_R_RegisterShaderNoMip: Failed to load shader no mip: '%s'\n", name);
	}
	else
	{
		CG_DPrintf("^2trap_R_RegisterShaderNoMip: register shader no mip: '%s'\n", name);
	}
	DEBUG_REGISTERPROFILE_EXEC("trap_R_RegisterShaderNpMip", name);
	return handle;
}

/**
 * @brief trap_R_RegisterFont
 * @param[in] fontName
 * @param[in] pointSize
 * @param[in] font
 */
void trap_R_RegisterFont(const char *fontName, int pointSize, void *font)
{
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	SystemCall(CG_R_REGISTERFONT, fontName, pointSize, font, IS_FUNC_SUPPORTED(UNICODE_SUPPORT_VERSION));
	DEBUG_REGISTERPROFILE_EXEC("trap_R_RegisterFont", fontName)
}

/**
 * @brief trap_CM_LoadMap
 * @param[in] mapname
 */
void trap_CM_LoadMap(const char *mapname)
{
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	SystemCall(CG_CM_LOADMAP, mapname);
	DEBUG_REGISTERPROFILE_EXEC("trap_CM_LoadMap", mapname)
}

/**
 * @brief trap_R_LoadWorldMap
 * @param[in] mapname
 */
void trap_R_LoadWorldMap(const char *mapname)
{
	DEBUG_REGISTERPROFILE_INIT
	CG_DrawInformation(qtrue);
	SystemCall(CG_R_LOADWORLDMAP, mapname);
	DEBUG_REGISTERPROFILE_EXEC("trap_R_LoadWorldMap", mapname)
}
#else
/**
 * @brief trap_S_RegisterSound
 * @param[in] sample
 * @param[in] compressed
 * @return
 */
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed)
{
	CG_DrawInformation(qtrue);
	return SystemCall(CG_S_REGISTERSOUND, sample, compressed);
}

/**
 * @brief trap_R_RegisterModel
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterModel(const char *name)
{
	CG_DrawInformation(qtrue);
	return SystemCall(CG_R_REGISTERMODEL, name);
}

/**
 * @brief trap_R_RegisterSkin
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterSkin(const char *name)
{
	CG_DrawInformation(qtrue);
	return SystemCall(CG_R_REGISTERSKIN, name);
}

/**
 * @brief trap_R_RegisterShader
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterShader(const char *name)
{
	CG_DrawInformation(qtrue);
	return SystemCall(CG_R_REGISTERSHADER, name);
}

/**
 * @brief trap_R_RegisterShaderNoMip
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterShaderNoMip(const char *name)
{
	CG_DrawInformation(qtrue);
	return SystemCall(CG_R_REGISTERSHADERNOMIP, name);
}

/**
 * @brief trap_R_RegisterFont
 * @param[in] fontName
 * @param[in] pointSize
 * @param[in] font
 */
void trap_R_RegisterFont(const char *fontName, int pointSize, void *font)
{
	CG_DrawInformation(qtrue);
	SystemCall(CG_R_REGISTERFONT, fontName, pointSize, font, IS_FUNC_SUPPORTED(UNICODE_SUPPORT_VERSION));
}

/**
 * @brief trap_CM_LoadMap
 * @param[in] mapname
 */
void trap_CM_LoadMap(const char *mapname)
{
	CG_DrawInformation(qtrue);
	SystemCall(CG_CM_LOADMAP, mapname);
}

/**
 * @brief trap_R_LoadWorldMap
 * @param[in] mapname
 */
void trap_R_LoadWorldMap(const char *mapname)
{
	CG_DrawInformation(qtrue);
	SystemCall(CG_R_LOADWORLDMAP, mapname);
}
#endif // ETLEGACY_DEBUG

/**
 * @brief trap_R_inPVS
 * @param[in] p1
 * @param[in] p2
 * @return
 */
qboolean trap_R_inPVS(const vec3_t p1, const vec3_t p2)
{
	return (qboolean)(SystemCall(CG_R_INPVS, p1, p2));
}

/**
 * @brief trap_GetHunkData
 * @param[out] hunkused
 * @param[out] hunkexpected
 */
void trap_GetHunkData(int *hunkused, int *hunkexpected)
{
	SystemCall(CG_GETHUNKDATA, hunkused, hunkexpected);
}

/**
 * @brief Binary message channel
 * @param[out] buf
 * @param[in] buflen
 * @return result
 */
qboolean trap_SendMessage(char *buf, int buflen)
{
	return SystemCall(CG_SENDMESSAGE, buf, buflen);
}

/**
 * @brief trap_MessageStatus
 * @return
 */
messageStatus_t trap_MessageStatus(void)
{
	return (messageStatus_t)(SystemCall(CG_MESSAGESTATUS));
}

/**
 * @brief Dynamic shaders
 * @param[in] shadername
 * @param[in] shadertext
 * @return
 */
qboolean trap_R_LoadDynamicShader(const char *shadername, const char *shadertext)
{
	return (qboolean)(SystemCall(CG_R_LOADDYNAMICSHADER, shadername, shadertext));
}

/**
 * @brief Render to texture
 * @param[in] textureid
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 */
void trap_R_RenderToTexture(int textureid, int x, int y, int w, int h)
{
	SystemCall(CG_R_RENDERTOTEXTURE, textureid, x, y, w, h);
}

/**
 * @brief trap_R_GetTextureId
 * @param[in] name
 * @return
 */
int trap_R_GetTextureId(const char *name)
{
	return SystemCall(CG_R_GETTEXTUREID, name);
}

/**
 * @brief Sync rendering
 */
void trap_R_Finish(void)
{
	SystemCall(CG_R_FINISH);
}
