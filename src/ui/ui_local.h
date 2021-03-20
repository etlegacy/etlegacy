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
 * @file ui_local.h
 * @brief Local definitions for UI module
 */

#ifndef INCLUDE_UI_LOCAL_H
#define INCLUDE_UI_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/q_unicode.h"
#include "../renderercommon/tr_types.h"
#include "ui_public.h"
#include "keycodes.h"
#include "../game/bg_public.h"
#include "ui_shared.h"

extern vmCvar_t ui_brassTime;
extern vmCvar_t ui_drawCrosshair;
extern vmCvar_t ui_drawCrosshairNames;
extern vmCvar_t ui_drawCrosshairPickups;
extern vmCvar_t ui_drawSpectatorNames;
extern vmCvar_t ui_marks;

extern vmCvar_t ui_autoactivate;

extern vmCvar_t ui_selectedPlayer;
extern vmCvar_t ui_selectedPlayerName;
extern vmCvar_t ui_netSource;
extern vmCvar_t ui_menuFiles;
extern vmCvar_t ui_gameType;
extern vmCvar_t ui_netGameType;
extern vmCvar_t ui_joinGameType;
extern vmCvar_t ui_dedicated;

// multiplayer cvars
extern vmCvar_t ui_serverFilterType;
extern vmCvar_t ui_currentNetMap;
extern vmCvar_t ui_currentMap;
extern vmCvar_t ui_mapIndex;
extern vmCvar_t ui_browserShowEmptyOrFull;
extern vmCvar_t ui_browserShowPasswordProtected;
extern vmCvar_t ui_browserShowFriendlyFire;
extern vmCvar_t ui_browserShowMaxlives;
extern vmCvar_t ui_browserShowAntilag;
extern vmCvar_t ui_browserShowWeaponsRestricted;
extern vmCvar_t ui_browserShowTeamBalanced;
extern vmCvar_t ui_browserShowHumans;

extern vmCvar_t ui_browserModFilter;
extern vmCvar_t ui_browserMapFilter;
extern vmCvar_t ui_browserServerNameFilterCheckBox;

extern vmCvar_t ui_browserOssFilter;

extern vmCvar_t ui_serverStatusTimeOut;

extern vmCvar_t g_gameType;

extern vmCvar_t cl_profile;
extern vmCvar_t cl_defaultProfile;
extern vmCvar_t ui_profile;
extern vmCvar_t ui_currentNetCampaign;
extern vmCvar_t ui_currentCampaign;
extern vmCvar_t ui_campaignIndex;
extern vmCvar_t ui_currentCampaignCompleted;
extern vmCvar_t ui_blackout;
extern vmCvar_t ui_cg_crosshairAlpha;
extern vmCvar_t ui_cg_crosshairAlphaAlt;
extern vmCvar_t ui_cg_crosshairColor;
extern vmCvar_t ui_cg_crosshairColorAlt;
extern vmCvar_t ui_cg_crosshairSize;

extern vmCvar_t cl_bypassMouseInput;

extern vmCvar_t ui_serverBrowserSettings;

extern vmCvar_t ui_cg_shoutcastDrawPlayers;
extern vmCvar_t ui_cg_shoutcastDrawTeamNames;
extern vmCvar_t ui_cg_shoutcastTeamName1;
extern vmCvar_t ui_cg_shoutcastTeamName2;
extern vmCvar_t ui_cg_shoutcastDrawHealth;
extern vmCvar_t ui_cg_shoutcastGrenadeTrail;
extern vmCvar_t ui_cg_shoutcastDrawMinimap;

// ui_serverBrowserSettings flags
#define UI_BROWSER_ALLOW_REDIRECT     BIT(0)
#define UI_BROWSER_ALLOW_HUMANS_COUNT BIT(1)
#define UI_BROWSER_ALLOW_MAX_CLIENTS  BIT(2)

#define SLIDER_RANGE            10
#define MAX_EDIT_LINE           256

/**
 * @struct _tag_menuframework
 * @typedef menuframework_s
 * @brief
 */
typedef struct _tag_menuframework
{
	int cursor;
	int cursor_prev;

	int nitems;
	void *items[MAX_MENUITEMS];

	void (*draw)(void);
	sfxHandle_t (*key)(int key);

	qboolean wrapAround;
	qboolean fullscreen;
	qboolean showlogo;

	int specialmenutype;

} menuframework_s;

/**
 * @struct menucommon_s
 * @brief
 */
typedef struct
{
	int type;
	const char *name;
	int id;
	int x, y;
	int left;
	int top;
	int right;
	int bottom;
	menuframework_s *parent;
	int menuPosition;
	unsigned flags;

	void (*callback)(void *self, int event);
	void (*statusbar)(void *self);
	void (*ownerdraw)(void *self);
} menucommon_s;

/**
 * @struct mfield_s
 * @brief
 */
typedef struct
{
	int cursor;
	int scroll;
	int widthInChars;
	char buffer[MAX_EDIT_LINE];
	int maxchars;
} mfield_t;

/**
 * @struct menufield_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;
	mfield_t field;
} menufield_s;

/**
 * @struct menuslider_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_s;

/**
 * @struct menulist_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;

	int oldvalue;
	int curvalue;
	int numitems;
	int top;

	const char **itemnames;

	int width;
	int height;
	int columns;
	int seperation;
} menulist_s;

/**
 * @struct menuaction_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;
} menuaction_s;

/**
 * @struct menuradiobutton_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;
	int curvalue;
} menuradiobutton_s;

/**
 * @struct menubitmap_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;
	char *focuspic;
	char *errorpic;
	qhandle_t shader;
	qhandle_t focusshader;
	int width;
	int height;
	float *focuscolor;
} menubitmap_s;

/**
 * @struct menutext_s
 * @brief
 */
typedef struct
{
	menucommon_s generic;
	char *fmt;
	int style;
	float *color;
} menutext_s;

extern void         Menu_Cache(void);

extern vec4_t menu_text_color;
extern vec4_t menu_grayed_color;
extern vec4_t menu_dark_color;
extern vec4_t menu_highlight_color;
extern vec4_t menu_red_color;
extern vec4_t menu_black_color;
extern vec4_t menu_dim_color;
extern vec4_t color_black;

extern vec4_t color_halfblack;

extern vec4_t color_white;
extern vec4_t color_yellow;
extern vec4_t color_blue;
extern vec4_t color_orange;
extern vec4_t color_red;
extern vec4_t color_dim;
extern vec4_t name_color;
extern vec4_t list_color;
extern vec4_t listbar_color;
extern vec4_t listbar_color2;

extern vec4_t text_color_disabled;
extern vec4_t text_color_normal;
extern vec4_t text_color_highlight;

extern menuDef_t Menus[MAX_MENUS];      ///< defined menus
extern int       menuCount;             ///< how many menus

extern menuDef_t *modalMenuStack[MAX_MODAL_MENUS];
extern int       modalMenuCount;

extern qboolean  g_waitingForKey;
extern qboolean  g_editingField;
extern itemDef_t *g_editItem;
extern itemDef_t *g_bindItem;

extern void      (*captureFunc)(void *p);
extern void      *captureData;
extern itemDef_t *itemCapture;

#define DOUBLE_CLICK_DELAY 300
extern int lastListBoxClickTime;

extern qboolean debugMode;

#define SET_EDITITEM(x) g_editingField  = qtrue; g_editItem = x;
#define CLEAR_EDITITEM() g_editingField = qfalse; g_editItem = NULL;
#define IS_EDITMODE(x) ((x->window.flags & WINDOW_HASFOCUS) && g_editingField)
#define COMBO_SELECTORCHAR "V"

// ui_main.c
void UI_Report(void);
void UI_Load(void);
void UI_LoadMenus(const char *menuFile, qboolean reset);
void  UI_SetActiveMenu(uiMenuCommand_t menu);
uiMenuCommand_t UI_GetActiveMenu(void);
int UI_AdjustTimeByGame(int time);
void UI_ShowPostGame();
void UI_LoadArenas(void);
void UI_LoadCampaigns(void);
mapInfo *UI_FindMapInfoByMapname(const char *name);
void UI_ReadableSize(char *buf, int bufsize, int value);
void UI_PrintTime(char *buf, int bufsize, int time);
void Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font);

void UI_Campaign_f(void);
void UI_ListCampaigns_f(void);

void UI_ListFavourites_f(void);
void UI_RemoveAllFavourites_f(void);

#define GLINFO_LINES        256

// ui_atoms.c
void QDECL Com_DPrintf(const char *fmt, ...);

// ui_menu.c
extern void UI_RegisterCvars(void);
extern void UI_UpdateCvars(void);

// ui_connect.c
extern void UI_DrawConnectScreen(qboolean overlay);

// ui_loadpanel.c
extern void UI_DrawLoadPanel(qboolean ownerdraw, qboolean uihack);

// Is this diffevent in other systems? OSX?
#define K_CLIPBOARD(x) (tolower(x) == 'v' && (DC->keyIsDown(K_LCTRL) || DC->keyIsDown(K_RCTRL)))

// new ui stuff
#define MAX_HEADS 64
#define MAX_ALIASES 64
#define MAX_TEAMS 64

#define MAX_MAPS 512
#define MAX_ADDRESSLENGTH       64
#define MAX_DISPLAY_SERVERS     4096
#define MAX_SERVERSTATUS_LINES  128
#define MAX_SERVERSTATUS_TEXT   2048

#define MAX_MODS 64
#define MAX_DEMOS 256
#define MAX_MOVIES 256
#define MAX_PLAYERMODELS 256
#define MAX_SPAWNPOINTS 128
#define MAX_SPAWNDESC   128

#define MAX_PROFILES 64

#define SCROLL_TIME_START           500
#define SCROLL_TIME_ADJUST          150
#define SCROLL_TIME_ADJUSTOFFSET    40
#define SCROLL_TIME_FLOOR           20

/**
 * @struct scrollInfo_s
 * @typedef scrollInfo_t
 * @brief
 */
typedef struct scrollInfo_s
{
	int nextScrollTime;
	int nextAdjustTime;
	int adjustValue;
	int scrollKey;
	float xStart;
	float yStart;
	itemDef_t *item;
	qboolean scrollDir;
} scrollInfo_t;

extern scrollInfo_t scrollInfo;

/**
 * @struct characterInfo
 * @brief
 */
typedef struct
{
	const char *name;
	const char *imageName;
	qhandle_t headImage;
	qboolean female;
} characterInfo;

/**
 * @struct aliasInfo
 * @brief
 */
typedef struct
{
	const char *name;
	const char *ai;
	const char *action;
} aliasInfo;

/**
 * @struct teamInfo
 * @brief
 */
typedef struct
{
	const char *teamName;
	const char *imageName;
	qhandle_t teamIcon;
	qhandle_t teamIcon_Metal;
	qhandle_t teamIcon_Name;
	int cinematic;
} teamInfo;

/**
 * @struct gameTypeInfo
 * @brief
 */
typedef struct
{
	const char *gameType;
	const char *gameTypeShort;
	int gtEnum;
	const char *gameTypeDescription;
} gameTypeInfo;

/**
 * @struct profileInfo_s
 * @brief
 */
typedef struct
{
	const char *name;
	const char *dir;
} profileInfo_t;

/**
 * @struct serverFilter_s
 * @typedef serverFilter_t
 * @brief
 */
typedef struct serverFilter_s
{
	const char *description;
	const char *basedir;
} serverFilter_t;

/**
 * @struct serverStatus_s
 * @typedef serverStatus_t
 * @brief
 */
typedef struct serverStatus_s
{
	int refreshtime;
	int sortKey;
	int sortDir;
	qboolean refreshActive;
	int currentServer;
	int displayServers[MAX_DISPLAY_SERVERS];
	int numDisplayServers;
	int numIncompatibleServers;
	int numInvalidServers;
	int numPlayersOnServers;
	int numHumansOnServers;
	int nextDisplayRefresh;
	qhandle_t currentServerPreview;
	int currentServerCinematic;
	int motdLen;
	int motdWidth;
	int motdPaintX;
	int motdPaintX2;
	int motdOffset;
	int motdTime;
	char motd[MAX_STRING_CHARS];
} serverStatus_t;

/**
 * @struct serverStatusInfo_s
 * @brief
 */
typedef struct
{
	char address[MAX_ADDRESSLENGTH];
	char *lines[MAX_SERVERSTATUS_LINES][4];
	char text[MAX_SERVERSTATUS_TEXT];
	char pings[MAX_CLIENTS * 3];
	int numLines;
} serverStatusInfo_t;

/**
 * @struct modInfo_s
 * @brief
 */
typedef struct
{
	const char *modName;
	const char *modDescr;
} modInfo_t;

typedef struct
{
	const char *path;
	qboolean file;
} demoItem_t;

typedef struct
{
	char path[MAX_OSPATH];
	demoItem_t items[MAX_DEMOS];
	unsigned int count;
	unsigned int index;
} demoList_t;

/**
 * @struct uiInfo_s
 * @brief
 *
 * @todo Cleanup
 */
typedef struct
{
	displayContextDef_t uiDC;

	int etLegacyClient;

	int characterCount;
	characterInfo characterList[MAX_HEADS];

	int aliasCount;
	aliasInfo aliasList[MAX_ALIASES];

	int teamCount;
	teamInfo teamList[MAX_TEAMS];

	int numGameTypes;
	gameTypeInfo gameTypes[MAX_GAMETYPES];

	int numJoinGameTypes;
	gameTypeInfo joinGameTypes[MAX_GAMETYPES];

	int redBlue;
	int playerCount;
	int myTeamCount;
	int teamIndex;
	int playerRefresh;
	int playerIndex;
	int playerNumber;
	qboolean teamLeader;
	char playerNames[MAX_CLIENTS][MAX_NAME_LENGTH * 2];
	qboolean playerMuted[MAX_CLIENTS];
	int playerRefereeStatus[MAX_CLIENTS];
	int playerShoutcasterStatus[MAX_CLIENTS];
	char teamNames[MAX_CLIENTS][MAX_NAME_LENGTH];
	int teamClientNums[MAX_CLIENTS];

	int mapCount;
	mapInfo mapList[MAX_MAPS];

	int campaignCount;
	campaignInfo_t campaignList[MAX_CAMPAIGNS];

	cpsFile_t campaignStatus;

	profileInfo_t profileList[MAX_PROFILES];
	int profileCount;
	int profileIndex;

	modInfo_t modList[MAX_MODS];
	int modCount;
	int modIndex;

	/*
	const char demoPath[MAX_QPATH]
	const char *demoList[MAX_DEMOS];
	int demoCount;
	int demoIndex;
	*/
	demoList_t demos;

	const char *movieList[MAX_MOVIES];
	int movieCount;
	int movieIndex;
	int previewMovie;

	serverStatus_t serverStatus;

	// for the showing the status of a server
	char serverStatusAddress[MAX_ADDRESSLENGTH];
	serverStatusInfo_t serverStatusInfo;
	int nextServerStatusRefresh;

	int currentCrosshair;
	int startPostGameTime;

	int q3HeadCount;
	char q3HeadNames[MAX_PLAYERMODELS][64];
	qhandle_t q3HeadIcons[MAX_PLAYERMODELS];

	int effectsColor;

	int selectedObjective;

	int activeFont;

	const char *glInfoLines[GLINFO_LINES];
	int numGlInfoLines;

	vec4_t xhairColor;
	vec4_t xhairColorAlt;

	qhandle_t passwordFilter;
	qhandle_t friendlyFireFilter;
	qhandle_t maxLivesFilter;
	qhandle_t weaponRestrictionsFilter;
	qhandle_t antiLagFilter;
	qhandle_t teamBalanceFilter;

	// FIXME: put these into an array & sync order
	qhandle_t modFilter_legacy;
	qhandle_t modFilter_etpub;
	qhandle_t modFilter_jaymod;
	qhandle_t modFilter_nq;
	qhandle_t modFilter_nitmod;
	qhandle_t modFilter_silent;
	qhandle_t modFilter_tc;
	qhandle_t modFilter_etnam;
	qhandle_t modFilter_etrun;
	qhandle_t modFilter_etjump;
	qhandle_t modFilter_tjmod;
	qhandle_t modFilter_etmain;
	qhandle_t modFilter_unknown;

	qhandle_t campaignMap;
} uiInfo_t;

extern uiInfo_t uiInfo;

extern displayContextDef_t *DC;

// ui_atoms.c
extern qboolean UI_ConsoleCommand(int realTime);
extern void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader);
extern void UI_FillRect(float x, float y, float width, float height, const float *color);
extern void UI_DrawTopBottom(float x, float y, float w, float h);
extern void UI_AdjustFrom640(float *x, float *y, float *w, float *h);
extern char *UI_Argv(int arg);
extern char *UI_Cvar_VariableString(const char *varName);

// ui_shared.c
int Binding_IDFromName(const char *name);
void Binding_Set(int id, int b1, int b2);
void Binding_Unset(int id, int index);
qboolean Binding_Check(int id, qboolean b1, int key);
int Binding_Get(int id, qboolean b1);
int Binding_Count(void);
char *Binding_FromName(const char *cvar);

// ui_syscalls.c
void trap_Print(const char *fmt);
void trap_Error(const char *fmt) _attribute((noreturn));
int trap_Milliseconds(void);
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void trap_Cvar_Update(vmCvar_t *vmCvar);
void trap_Cvar_Set(const char *varName, const char *value);
float trap_Cvar_VariableValue(const char *varName);
void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize);
void trap_Cvar_LatchedVariableStringBuffer(const char *varName, char *buffer, int bufsize);
void trap_Cvar_SetValue(const char *varName, float value);
void trap_Cvar_Reset(const char *name);
void trap_Cvar_Create(const char *varName, const char *var_value, int flags);
void trap_Cvar_InfoStringBuffer(int bit, char *buffer, int bufsize);
int trap_Argc(void);
void trap_Argv(int n, char *buffer, int bufferLength);
void trap_Cmd_ExecuteText(int exec_when, const char *text);      // don't use EXEC_NOW!
void trap_AddCommand(const char *cmdName);
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, int len, fileHandle_t f);
void trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int trap_FS_Delete(const char *filename);
qhandle_t trap_R_RegisterModel(const char *name);
qhandle_t trap_R_RegisterSkin(const char *name);
qhandle_t trap_R_RegisterShaderNoMip(const char *name);
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(const refEntity_t *re);
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void trap_R_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);
void trap_R_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
void trap_R_RenderScene(const refdef_t *fd);
void trap_R_SetColor(const float *rgba);
void trap_R_Add2dPolys(polyVert_t *verts, int numverts, qhandle_t hShader);
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
void trap_R_DrawRotatedPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
void trap_UpdateScreen(void);
int trap_CM_LerpTag(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex);
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed);
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen);
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen);
void trap_Key_KeysForBinding(const char *binding, int *key1, int *key2);
void trap_Key_SetBinding(int keynum, const char *binding);
qboolean trap_Key_IsDown(int keynum);
qboolean trap_Key_GetOverstrikeMode(void);
void trap_Key_SetOverstrikeMode(qboolean state);
void trap_Key_ClearStates(void);
int trap_Key_GetCatcher(void);
void trap_Key_SetCatcher(int catcher);
void trap_GetClipboardData(char *buf, size_t bufsize);
void trap_GetClientState(uiClientState_t *state);
void trap_GetGlconfig(glconfig_t *glconfig);
int trap_GetConfigString(int index, char *buff, size_t buffsize);
int trap_LAN_GetServerCount(int source);
int trap_LAN_GetLocalServerCount(void);
void trap_LAN_GetLocalServerAddressString(int n, char *buf, int buflen);
int trap_LAN_GetGlobalServerCount(void);
void trap_LAN_GetGlobalServerAddressString(int n, char *buf, int buflen);
int trap_LAN_GetPingQueueCount(void);
void trap_LAN_ClearPing(int n);
void trap_LAN_GetPing(int n, char *buf, int buflen, int *pingtime);
void trap_LAN_GetPingInfo(int n, char *buf, int buflen);
int trap_MemoryRemaining(void);

// multiplayer traps
qboolean trap_LAN_UpdateVisiblePings(int source);
void trap_LAN_MarkServerVisible(int source, int n, qboolean visible);
void trap_LAN_ResetPings(int n);
int trap_LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2);
void trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen);
void trap_LAN_GetServerInfo(int source, int n, char *buf, int buflen);
int trap_LAN_AddServer(int source, const char *name, const char *addr);
void trap_LAN_RemoveServer(int source, const char *addr);
int trap_LAN_GetServerPing(int source, int n);
int trap_LAN_ServerIsVisible(int source, int n);
int trap_LAN_ServerStatus(const char *serverAddress, char *serverStatus, int maxLen);
void trap_LAN_LoadCachedServers(void);
qboolean trap_LAN_ServerIsInFavoriteList(int source, int n);

void trap_R_RegisterFont(const char *fontName, int pointSize, void *font);
void trap_S_StopBackgroundTrack(void);
void trap_S_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime);
void trap_S_FadeAllSound(float targetvol, int time, qboolean stopsounds);
int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic(int handle);
void trap_CIN_DrawCinematic(int handle);
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h);
int  trap_RealTime(qtime_t *qtime);
void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);
qboolean trap_GetLimboString(int index, char *buf);
void trap_CheckAutoUpdate(void);
void trap_GetAutoUpdate(void);

void trap_openURL(const char *url);
void trap_GetHunkData(int *hunkused, int *hunkexpected);

// localization functions
const char *UI_TranslateString(const char *string);
void trap_TranslateString(const char *fmt, char *buffer);

const char *UI_DescriptionForCampaign(void);
const char *UI_NameForCampaign(void);

#define EDITFIELD_TEMP_CVAR         "ui_textfield_temp"

#endif
