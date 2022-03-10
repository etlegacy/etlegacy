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
 * @file ui_syscalls.c
 */

#include "ui_local.h"

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
 * @brief trap_Print
 * @param[in] fmt
 */
void trap_Print(const char *fmt)
{
	SystemCall(UI_PRINT, fmt);
}

/**
 * @brief trap_Error
 * @param[in] fmt
 */
void trap_Error(const char *fmt)
{
	SystemCall(UI_ERROR, fmt);
	// shut up GCC warning about returning functions, because we know better
	exit(1);
}

/**
 * @brief trap_Milliseconds
 * @return
 */
int trap_Milliseconds(void)
{
	return SystemCall(UI_MILLISECONDS);
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
	SystemCall(UI_CVAR_REGISTER, vmCvar, varName, defaultValue, flags);
}

/**
 * @brief trap_Cvar_Update
 * @param[in] vmCvar
 */
void trap_Cvar_Update(vmCvar_t *vmCvar)
{
	SystemCall(UI_CVAR_UPDATE, vmCvar);
}

/**
 * @brief trap_Cvar_Set
 * @param[in] varName
 * @param[in] value
 */
void trap_Cvar_Set(const char *varName, const char *value)
{
	SystemCall(UI_CVAR_SET, varName, value);
}

/**
 * @brief trap_Cvar_VariableValue
 * @param[in] varName
 * @return
 */
float trap_Cvar_VariableValue(const char *varName)
{
	floatint_t fi;

	fi.i = SystemCall(UI_CVAR_VARIABLEVALUE, varName);
	return fi.f;
}

/**
 * @brief trap_Cvar_VariableStringBuffer
 * @param[in] varName
 * @param[in] buffer
 * @param[in] bufsize
 */
void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize)
{
	SystemCall(UI_CVAR_VARIABLESTRINGBUFFER, varName, buffer, bufsize);
}

/**
 * @brief trap_Cvar_LatchedVariableStringBuffer
 * @param[in] varName
 * @param[in] buffer
 * @param[in] bufsize
 */
void trap_Cvar_LatchedVariableStringBuffer(const char *varName, char *buffer, int bufsize)
{
	SystemCall(UI_CVAR_LATCHEDVARIABLESTRINGBUFFER, varName, buffer, bufsize);
}

/**
 * @brief trap_Cvar_SetValue
 * @param[in] varName
 * @param[in] value
 */
void trap_Cvar_SetValue(const char *varName, float value)
{
	SystemCall(UI_CVAR_SETVALUE, varName, PASSFLOAT(value));
}

/**
 * @brief trap_Cvar_Reset
 * @param[in] name
 */
void trap_Cvar_Reset(const char *name)
{
	SystemCall(UI_CVAR_RESET, name);
}

/**
 * @brief trap_Cvar_Create
 * @param[in] varName
 * @param[in] var_value
 * @param[in] flags
 */
void trap_Cvar_Create(const char *varName, const char *var_value, int flags)
{
	SystemCall(UI_CVAR_CREATE, varName, var_value, flags);
}

/**
 * @brief trap_Cvar_InfoStringBuffer
 * @param[in] bit
 * @param[in] buffer
 * @param[in] bufsize
 */
void trap_Cvar_InfoStringBuffer(int bit, char *buffer, int bufsize)
{
	SystemCall(UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize);
}

/**
 * @brief trap_Argc
 * @return
 */
int trap_Argc(void)
{
	return SystemCall(UI_ARGC);
}

/**
 * @brief trap_Argv
 * @param[in] n
 * @param[in] buffer
 * @param[in] bufferLength
 */
void trap_Argv(int n, char *buffer, int bufferLength)
{
	SystemCall(UI_ARGV, n, buffer, bufferLength);
}

/**
 * @brief trap_Cmd_ExecuteText
 * @param[in] exec_when
 * @param[in] text
 */
void trap_Cmd_ExecuteText(int exec_when, const char *text)
{
	SystemCall(UI_CMD_EXECUTETEXT, exec_when, text);
}

/**
 * @brief trap_AddCommand
 * @param[in] cmdName
 */
void trap_AddCommand(const char *cmdName)
{
	SystemCall(UI_ADDCOMMAND, cmdName);
}

/**
 * @brief trap_FS_FOpenFile
 * @param[in] qpath
 * @param[in] f
 * @param[in] mode
 * @return
 */
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	return SystemCall(UI_FS_FOPENFILE, qpath, f, mode);
}

/**
 * @brief trap_FS_Read
 * @param[in] buffer
 * @param[in] len
 * @param[in] f
 */
void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	SystemCall(UI_FS_READ, buffer, len, f);
}

/**
 * @brief trap_FS_Write
 * @param[in] buffer
 * @param[in] len
 * @param[in] f
 */
void trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	SystemCall(UI_FS_WRITE, buffer, len, f);
}

/**
 * @brief trap_FS_FCloseFile
 * @param[in] f
 */
void trap_FS_FCloseFile(fileHandle_t f)
{
	SystemCall(UI_FS_FCLOSEFILE, f);
}

/**
 * @brief trap_FS_GetFileList
 * @param[in] path
 * @param[in] extension
 * @param[in] listbuf
 * @param[in] bufsize
 * @return
 */
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return SystemCall(UI_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

/**
 * @brief trap_FS_Delete
 * @param[in] filename
 * @return
 */
int trap_FS_Delete(const char *filename)
{
	return SystemCall(UI_FS_DELETEFILE, filename);
}

/**
 * @brief trap_R_RegisterModel
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterModel(const char *name)
{
	return SystemCall(UI_R_REGISTERMODEL, name);
}

/**
 * @brief trap_R_RegisterSkin
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterSkin(const char *name)
{
	return SystemCall(UI_R_REGISTERSKIN, name);
}

/**
 * @brief trap_R_RegisterFont
 * @param[in] fontName
 * @param[in] pointSize
 * @param[in] font
 */
void trap_R_RegisterFont(const char *fontName, int pointSize, void *font)
{
	SystemCall(UI_R_REGISTERFONT, fontName, pointSize, font, IS_FUNC_SUPPORTED(UNICODE_SUPPORT_VERSION));
}

/**
 * @brief trap_R_RegisterShaderNoMip
 * @param[in] name
 * @return
 */
qhandle_t trap_R_RegisterShaderNoMip(const char *name)
{
	return SystemCall(UI_R_REGISTERSHADERNOMIP, name);
}

/**
 * @brief trap_R_ClearScene
 */
void trap_R_ClearScene(void)
{
	SystemCall(UI_R_CLEARSCENE);
}

/**
 * @brief trap_R_AddRefEntityToScene
 * @param[in]  re
 */
void trap_R_AddRefEntityToScene(const refEntity_t *re)
{
	SystemCall(UI_R_ADDREFENTITYTOSCENE, re);
}

/**
 * @brief trap_R_AddPolyToScene
 * @param[in] hShader
 * @param[in] numVerts
 * @param[in] verts
 */
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts)
{
	SystemCall(UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts);
}

// new dlight system
//% void    trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw ) {
//%     SystemCall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), overdraw );
//% }
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
	SystemCall(UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(radius), PASSFLOAT(intensity),
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
	SystemCall(UI_R_ADDCORONATOSCENE, org, PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), PASSFLOAT(scale), id, visible);
}

/**
 * @brief trap_R_RenderScene
 * @param[in] fd
 */
void trap_R_RenderScene(const refdef_t *fd)
{
	SystemCall(UI_R_RENDERSCENE, fd);
}

/**
 * @brief trap_R_SetColor
 * @param[in] rgba
 */
void trap_R_SetColor(const float *rgba)
{
	SystemCall(UI_R_SETCOLOR, rgba);
}

/**
 * @brief trap_R_Add2dPolys
 * @param[in] verts
 * @param[in] numverts
 * @param[in] hShader
 */
void trap_R_Add2dPolys(polyVert_t *verts, int numverts, qhandle_t hShader)
{
	SystemCall(UI_R_DRAW2DPOLYS, verts, numverts, hShader);
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
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader)
{
	SystemCall(UI_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader);
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
void trap_R_DrawRotatedPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle)
{
	SystemCall(UI_R_DRAWROTATEDPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader, PASSFLOAT(angle));
}

/**
 * @brief trap_R_ModelBounds
 * @param[in] model
 * @param[in] mins
 * @param[in] maxs
 */
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs)
{
	SystemCall(UI_R_MODELBOUNDS, model, mins, maxs);
}

/**
 * @brief trap_UpdateScreen
 */
void trap_UpdateScreen(void)
{
	SystemCall(UI_UPDATESCREEN);
}

/**
 * @brief trap_CM_LerpTag
 * @param[in] tag
 * @param[in] refent
 * @param[in] tagName
 * @param[in] startIndex - unused
 * @return
 */
int trap_CM_LerpTag(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex)
{
	return SystemCall(UI_CM_LERPTAG, tag, refent, tagName, 0);
}

/**
 * @brief trap_S_StartLocalSound
 * @param[in] sfx
 * @param[in] channelNum
 */
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum)
{
	SystemCall(UI_S_STARTLOCALSOUND, sfx, channelNum, 127 /*default volume always for the moment*/);
}

/**
 * @brief trap_S_RegisterSound
 * @param[in] sample
 * @param[in] compressed
 * @return
 */
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed)
{
	int i = SystemCall(UI_S_REGISTERSOUND, sample, compressed);

#ifdef ETLEGACY_DEBUG
	if (i == 0)
	{
		Com_Printf("^1Warning: Failed to load sound: %s\n", sample);
	}
#endif
	return i;
}

/**
 * @brief trap_S_FadeBackgroundTrack
 * @param[in] targetvol
 * @param[in] time
 * @param[in] num
 */
void trap_S_FadeBackgroundTrack(float targetvol, int time, int num)       // yes, i know.  fadebackground coming in, fadestreaming going out.  will have to see where functionality leads...
{
	SystemCall(UI_S_FADESTREAMINGSOUND, PASSFLOAT(targetvol), time, num);     // 'num' is '0' if it's music, '1' if it's "all streaming sounds"
}

/**
 * @brief trap_S_FadeAllSound
 * @param[in] targetvol
 * @param[in] time
 * @param[in] stopsounds
 */
void trap_S_FadeAllSound(float targetvol, int time, qboolean stopsounds)
{
	SystemCall(UI_S_FADEALLSOUNDS, PASSFLOAT(targetvol), time, stopsounds);
}

/**
 * @brief trap_Key_KeynumToStringBuf
 * @param[in] keynum
 * @param[in] buf
 * @param[in] buflen
 */
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen)
{
	SystemCall(UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen);
}

/**
 * @brief trap_Key_GetBindingBuf
 * @param[in] keynum
 * @param[in] buf
 * @param[in] buflen
 */
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen)
{
	SystemCall(UI_KEY_GETBINDINGBUF, keynum, buf, buflen);
}

/**
 * @brief trap_Key_KeysForBinding
 * @param[in] binding
 * @param[in] key1
 * @param[in] key2
 *
 * @note binding MUST be lower case
 */
void trap_Key_KeysForBinding(const char *binding, int *key1, int *key2)
{
	SystemCall(UI_KEY_BINDINGTOKEYS, binding, key1, key2);
}

/**
 * @brief trap_Key_SetBinding
 * @param[in] keynum
 * @param[in] binding
 */
void trap_Key_SetBinding(int keynum, const char *binding)
{
	SystemCall(UI_KEY_SETBINDING, keynum, binding);
}

/**
 * @brief trap_Key_IsDown
 * @param[in] keynum
 * @return
 */
qboolean trap_Key_IsDown(int keynum)
{
	return (qboolean)(SystemCall(UI_KEY_ISDOWN, keynum));
}

/**
 * @brief trap_Key_GetOverstrikeMode
 * @return
 */
qboolean trap_Key_GetOverstrikeMode(void)
{
	return (qboolean)(SystemCall(UI_KEY_GETOVERSTRIKEMODE));
}

/**
 * @brief trap_Key_SetOverstrikeMode
 * @param[in] state
 */
void trap_Key_SetOverstrikeMode(qboolean state)
{
	SystemCall(UI_KEY_SETOVERSTRIKEMODE, state);
}

/**
 * @brief trap_Key_ClearStates
 */
void trap_Key_ClearStates(void)
{
	SystemCall(UI_KEY_CLEARSTATES);
}

/**
 * @brief trap_Key_GetCatcher
 * @return
 */
int trap_Key_GetCatcher(void)
{
	return SystemCall(UI_KEY_GETCATCHER);
}

/**
 * @brief trap_Key_SetCatcher
 * @param[in] catcher
 */
void trap_Key_SetCatcher(int catcher)
{
	SystemCall(UI_KEY_SETCATCHER, catcher);
}

/**
 * @brief trap_GetClipboardData
 * @param[in] buf
 * @param[in] bufsize
 */
void trap_GetClipboardData(char *buf, size_t bufsize)
{
	SystemCall(UI_GETCLIPBOARDDATA, buf, bufsize);
}

/**
 * @brief trap_GetClientState
 * @param[in] state
 */
void trap_GetClientState(uiClientState_t *state)
{
	SystemCall(UI_GETCLIENTSTATE, state);
}

/**
 * @brief trap_GetGlconfig
 * @param[in] glconfig
 */
void trap_GetGlconfig(glconfig_t *glconfig)
{
	SystemCall(UI_GETGLCONFIG, glconfig);
}

/**
 * @brief trap_GetConfigString
 * @param[in] index
 * @param[in] buff
 * @param[in] buffsize
 * @return
 */
int trap_GetConfigString(int index, char *buff, size_t buffsize)
{
	return SystemCall(UI_GETCONFIGSTRING, index, buff, buffsize);
}

/**
 * @brief trap_LAN_GetLocalServerCount
 * @return
 */
int trap_LAN_GetLocalServerCount(void)
{
	return SystemCall(UI_LAN_GETLOCALSERVERCOUNT);
}

/**
 * @brief trap_LAN_GetLocalServerAddressString
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 */
void trap_LAN_GetLocalServerAddressString(int n, char *buf, int buflen)
{
	SystemCall(UI_LAN_GETLOCALSERVERADDRESSSTRING, n, buf, buflen);
}

/**
 * @brief trap_LAN_GetGlobalServerCount
 * @return
 */
int trap_LAN_GetGlobalServerCount(void)
{
	return SystemCall(UI_LAN_GETGLOBALSERVERCOUNT);
}

/**
 * @brief trap_LAN_GetGlobalServerAddressString
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 */
void trap_LAN_GetGlobalServerAddressString(int n, char *buf, int buflen)
{
	SystemCall(UI_LAN_GETGLOBALSERVERADDRESSSTRING, n, buf, buflen);
}

/**
 * @brief trap_LAN_GetPingQueueCount
 * @return
 */
int trap_LAN_GetPingQueueCount(void)
{
	return SystemCall(UI_LAN_GETPINGQUEUECOUNT);
}

/**
 * @brief trap_LAN_ClearPing
 * @param[in] n
 */
void trap_LAN_ClearPing(int n)
{
	SystemCall(UI_LAN_CLEARPING, n);
}

/**
 * @brief trap_LAN_GetPing
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 * @param[in] pingtime
 */
void trap_LAN_GetPing(int n, char *buf, int buflen, int *pingtime)
{
	SystemCall(UI_LAN_GETPING, n, buf, buflen, pingtime);
}

/**
 * @brief trap_LAN_GetPingInfo
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 */
void trap_LAN_GetPingInfo(int n, char *buf, int buflen)
{
	SystemCall(UI_LAN_GETPINGINFO, n, buf, buflen);
}

/**
 * @brief trap_LAN_UpdateVisiblePings
 * @param[in] source
 * @return
 */
qboolean trap_LAN_UpdateVisiblePings(int source)
{
	return (qboolean)(SystemCall(UI_LAN_UPDATEVISIBLEPINGS, source));
}

/**
 * @brief trap_LAN_GetServerCount
 * @param[in] source
 * @return
 */
int trap_LAN_GetServerCount(int source)
{
	return SystemCall(UI_LAN_GETSERVERCOUNT, source);
}

/**
 * @brief trap_LAN_CompareServers
 * @param[in] source
 * @param[in] sortKey
 * @param[in] sortDir
 * @param[in] s1
 * @param[in] s2
 * @return
 */
int trap_LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	return SystemCall(UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2);
}

/**
 * @brief trap_LAN_GetServerAddressString
 * @param[in] source
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 */
void trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen)
{
	SystemCall(UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen);
}

/**
 * @brief trap_LAN_GetServerInfo
 * @param[in] source
 * @param[in] n
 * @param[in] buf
 * @param[in] buflen
 */
void trap_LAN_GetServerInfo(int source, int n, char *buf, int buflen)
{
	SystemCall(UI_LAN_GETSERVERINFO, source, n, buf, buflen);
}

/**
 * @brief trap_LAN_AddServer
 * @param[in] source
 * @param[in] name
 * @param[in] addr
 * @return
 */
int trap_LAN_AddServer(int source, const char *name, const char *addr)
{
	return SystemCall(UI_LAN_ADDSERVER, source, name, addr);
}

/**
 * @brief trap_LAN_RemoveServer
 * @param[in] source
 * @param[in] addr
 */
void trap_LAN_RemoveServer(int source, const char *addr)
{
	SystemCall(UI_LAN_REMOVESERVER, source, addr);
}

/**
 * @brief trap_LAN_GetServerPing
 * @param[in] source
 * @param[in] n
 * @return
 */
int trap_LAN_GetServerPing(int source, int n)
{
	return SystemCall(UI_LAN_GETSERVERPING, source, n);
}

/**
 * @brief trap_LAN_ServerIsVisible
 * @param[in] source
 * @param[in] n
 * @return
 */
int trap_LAN_ServerIsVisible(int source, int n)
{
	return SystemCall(UI_LAN_SERVERISVISIBLE, source, n);
}

/**
 * @brief trap_LAN_ServerStatus
 * @param[in] serverAddress
 * @param[in] serverStatus
 * @param[in] maxLen
 * @return
 */
int trap_LAN_ServerStatus(const char *serverAddress, char *serverStatus, int maxLen)
{
	return SystemCall(UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen);
}

/**
 * @brief trap_LAN_ServerIsInFavoriteList
 * @param[in] source
 * @param[in] n
 * @return
 */
qboolean trap_LAN_ServerIsInFavoriteList(int source, int n)
{
	return (qboolean)(SystemCall(UI_LAN_SERVERISINFAVORITELIST, source, n));
}

/*
 * @brief trap_LAN_SaveCachedServers
 *
 * @obsolete kept as reminder - ETL saves on add/remove
 *
void trap_LAN_SaveCachedServers(void)
{
    SystemCall(UI_LAN_SAVECACHEDSERVERS);
}
*/

/**
 * @brief trap_LAN_LoadCachedServers
 */
void trap_LAN_LoadCachedServers(void)
{
	SystemCall(UI_LAN_LOADCACHEDSERVERS);
}

/**
 * @brief trap_LAN_MarkServerVisible
 * @param[in] source
 * @param[in] n
 * @param[in] visible
 */
void trap_LAN_MarkServerVisible(int source, int n, qboolean visible)
{
	SystemCall(UI_LAN_MARKSERVERVISIBLE, source, n, visible);
}

/**
 * @brief trap_LAN_ResetPings
 * @param[in] n
 */
void trap_LAN_ResetPings(int n)
{
	SystemCall(UI_LAN_RESETPINGS, n);
}

/**
 * @brief trap_MemoryRemaining
 * @return
 */
int trap_MemoryRemaining(void)
{
	return SystemCall(UI_MEMORY_REMAINING);
}

/**
 * @brief trap_PC_AddGlobalDefine
 * @param[in] define
 * @return
 */
int trap_PC_AddGlobalDefine(char *define)
{
	return SystemCall(UI_PC_ADD_GLOBAL_DEFINE, define);
}

/**
 * @brief trap_PC_RemoveAllGlobalDefines
 * @return
 */
int trap_PC_RemoveAllGlobalDefines(void)
{
	return SystemCall(UI_PC_REMOVE_ALL_GLOBAL_DEFINES);
}

/**
 * @brief trap_PC_LoadSource
 * @param[in] filename
 * @return
 */
int trap_PC_LoadSource(const char *filename)
{
	return SystemCall(UI_PC_LOAD_SOURCE, filename);
}

/**
 * @brief trap_PC_FreeSource
 * @param[in] handle
 * @return
 */
int trap_PC_FreeSource(int handle)
{
	return SystemCall(UI_PC_FREE_SOURCE, handle);
}

/**
 * @brief trap_PC_ReadToken
 * @param handle
 * @param pc_token
 * @return
 */
int trap_PC_ReadToken(int handle, pc_token_t *pc_token)
{
	return SystemCall(UI_PC_READ_TOKEN, handle, pc_token);
}

/**
 * @brief trap_PC_SourceFileAndLine
 * @param[in] handle
 * @param[in] filename
 * @param[in] line
 * @return
 */
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	return SystemCall(UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

/**
 * @brief trap_PC_UnReadToken
 * @param[in] handle
 * @return
 */
int trap_PC_UnReadToken(int handle)
{
	return SystemCall(UI_PC_UNREAD_TOKEN, handle);
}

/**
 * @brief trap_S_StopBackgroundTrack
 */
void trap_S_StopBackgroundTrack(void)
{
	SystemCall(UI_S_STOPBACKGROUNDTRACK);
}

/**
 * @brief trap_S_StartBackgroundTrack
 * @param[in] intro
 * @param[in] loop
 * @param[in] fadeupTime
 */
void trap_S_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime)
{
	SystemCall(UI_S_STARTBACKGROUNDTRACK, intro, loop, fadeupTime);
}

/**
 * @brief trap_RealTime
 * @param[in] qtime
 * @return
 */
int trap_RealTime(qtime_t *qtime)
{
	return SystemCall(UI_REAL_TIME, qtime);
}

/**
 * @brief This returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
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
	return SystemCall(UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}


/**
 * @brief stops playing the cinematic and ends it.  should always return FMV_EOF
 * cinematics must be stopped in reverse order of when they are started
 * @param[in] handle
 * @return
 */
e_status trap_CIN_StopCinematic(int handle)
{
	return (e_status)(SystemCall(UI_CIN_STOPCINEMATIC, handle));
}

/**
 * @brief Will run a frame of the cinematic but will not draw it.
 * Will return FMV_EOF if the end of the cinematic has been reached.
 * @param[in] handle
 * @return
 */
e_status trap_CIN_RunCinematic(int handle)
{
	return (e_status)(SystemCall(UI_CIN_RUNCINEMATIC, handle));
}

/**
 * @brief Draws the current frame
 * @param[in] handle
 */
void trap_CIN_DrawCinematic(int handle)
{
	SystemCall(UI_CIN_DRAWCINEMATIC, handle);
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
	SystemCall(UI_CIN_SETEXTENTS, handle, x, y, w, h);
}

/**
 * @brief trap_R_RemapShader
 * @param[in] oldShader
 * @param[in] newShader
 * @param[in] timeOffset
 */
void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset)
{
	SystemCall(UI_R_REMAP_SHADER, oldShader, newShader, timeOffset);
}

/**
 * @brief trap_GetLimboString
 * @param[in] index
 * @param[in] buf
 * @return
 */
qboolean trap_GetLimboString(int index, char *buf)
{
	return (qboolean)(SystemCall(UI_CL_GETLIMBOSTRING, index, buf));
}

/**
 * @brief trap_TranslateString
 * @param[in] fmt
 * @param[in] buffer
 */
void trap_TranslateString(const char *fmt, char *buffer)
{
	SystemCall(UI_CL_TRANSLATE_STRING, fmt, buffer);
}

/**
 * @brief trap_CheckAutoUpdate
 */
void trap_CheckAutoUpdate(void)
{
	SystemCall(UI_CHECKAUTOUPDATE);
}

/**
 * @brief trap_GetAutoUpdate
 */
void trap_GetAutoUpdate(void)
{
	SystemCall(UI_GET_AUTOUPDATE);
}

/**
 * @brief trap_openURL
 * @param[in] url
 */
void trap_openURL(const char *url)
{
	SystemCall(UI_OPENURL, url);
}

/**
 * @brief trap_GetHunkData
 * @param[in] hunkused
 * @param[in] hunkexpected
 */
void trap_GetHunkData(int *hunkused, int *hunkexpected)
{
	SystemCall(UI_GETHUNKDATA, hunkused, hunkexpected);
}
