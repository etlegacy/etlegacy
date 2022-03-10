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
 * @file ui_shared.h
 * @brief Shared between cgame and ui
 */

#ifndef INCLUDE_UI_SHARED_H
#define INCLUDE_UI_SHARED_H

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "keycodes.h"

#include "../../etmain/ui/menudef.h"

#define MAX_MENUDEFFILE   4096
#define MAX_MENUFILE     32768
#define MAX_MENUS          256
#define MAX_MENUDEPTH        8
#define MAX_MENUITEMS      128       ///< ioquake3 has 96
#define MAX_COLOR_RANGES    10
#define MAX_MODAL_MENUS     16

#define WINDOW_MOUSEOVER        0x00000001  ///< mouse is over it, non exclusive
#define WINDOW_HASFOCUS         0x00000002  ///< has cursor focus, exclusive
#define WINDOW_VISIBLE          0x00000004  ///< is visible
#define WINDOW_GREY             0x00000008  ///< is visible but grey ( non-active )
#define WINDOW_DECORATION       0x00000010  ///< for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT        0x00000020  ///< fading out, non-active
#define WINDOW_FADINGIN         0x00000040  ///< fading in
#define WINDOW_MOUSEOVERTEXT    0x00000080  ///< mouse is over it, non exclusive
#define WINDOW_INTRANSITION     0x00000100  ///< window is in transition
#define WINDOW_FORECOLORSET     0x00000200  ///< forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL       0x00000400  ///< for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW     0x00000800  ///< mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW    0x00001000  ///< mouse is over right/down arrow
#define WINDOW_LB_THUMB         0x00002000  ///< mouse is over thumb
#define WINDOW_LB_PGUP          0x00004000  ///< mouse is over page up
#define WINDOW_LB_PGDN          0x00008000  ///< mouse is over page down
#define WINDOW_ORBITING         0x00010000  ///< item is in orbit
#define WINDOW_OOB_CLICK        0x00020000  ///< close on out of bounds click
#define WINDOW_WRAPPED          0x00040000  ///< manually wrap text
#define WINDOW_AUTOWRAPPED      0x00080000  ///< auto wrap text
#define WINDOW_FORCED           0x00100000  ///< forced open
#define WINDOW_POPUP            0x00200000  ///< popup
#define WINDOW_BACKCOLORSET     0x00400000  ///< backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE     0x00800000  ///< visibility timing ( NOT implemented )
#define WINDOW_IGNORE_HUDALPHA  0x01000000  ///< window will apply cg_hudAlpha value to colors unless this flag is set
#define WINDOW_DRAWALWAYSONTOP  0x02000000
#define WINDOW_MODAL            0x04000000 ///< window is modal, the window to go back to is stored in a stack
#define WINDOW_FOCUSPULSE       0x08000000
#define WINDOW_TEXTASINT        0x10000000
#define WINDOW_TEXTASFLOAT      0x20000000
#define WINDOW_LB_SOMEWHERE     0x40000000

// CGAME cursor type bits
#define CURSOR_NONE             0x00000001
#define CURSOR_ARROW            0x00000002
#define CURSOR_SIZER            0x00000004

#define TEXTFIELD(x) (x == ITEM_TYPE_EDITFIELD || x == ITEM_TYPE_NUMERICFIELD)

#ifdef CGAME
#define STRING_POOL_SIZE    128 * 1024
#else
#define STRING_POOL_SIZE    384 * 1024
#endif

#define MAX_EDITFIELD       256

#define ART_FX_BASE         "menu/art/fx_base"
#define ART_FX_BLUE         "menu/art/fx_blue"
#define ART_FX_CYAN         "menu/art/fx_cyan"
#define ART_FX_GREEN        "menu/art/fx_grn"
#define ART_FX_RED          "menu/art/fx_red"
#define ART_FX_TEAL         "menu/art/fx_teal"
#define ART_FX_WHITE        "menu/art/fx_white"
#define ART_FX_YELLOW       "menu/art/fx_yel"

#define ASSET_GRADIENTBAR           "ui/assets/gradientbar2.tga"
#define ASSET_GRADIENTROUND         "ui/assets/gradientround.tga"
#define ASSET_SCROLLBAR             "ui/assets/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "ui/assets/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "ui/assets/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "ui/assets/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "ui/assets/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "ui/assets/scrollbar_thumb.tga"
#define ASSET_SLIDER_BAR            "ui/assets/slider2.tga"
#define ASSET_SLIDER_THUMB          "ui/assets/sliderbutt_1.tga"
#define ASSET_CHECKBOX_CHECK        "ui/assets/check.tga"
#define ASSET_CHECKBOX_CHECK_NOT    "ui/assets/check_not.tga"
#define ASSET_CHECKBOX_CHECK_NO     "ui/assets/check_no.tga"

#define SCROLLBAR_SIZE      16.0f
#define SLIDER_WIDTH        96.0f
#define SLIDER_HEIGHT       10.0f    ///< 16.0
#define SLIDER_THUMB_WIDTH  12.0f
#define SLIDER_THUMB_HEIGHT 12.0f    ///< 20.0
#define NUM_CROSSHAIRS      16

/**
 * @struct windowDef_s
 * @typedef Window
 * @brief
 *
 * @todo FIXME: do something to separate text vs window stuff
 */
typedef struct
{
	rectDef_t rect;                 ///< client coord rectangle
	rectDef_t rectClient;           ///< screen coord rectangle
	const char *name;               ///<
	const char *model;              ///<
	const char *group;              ///< if it belongs to a group
	const char *cinematicName;      ///< cinematic name
	int cinematic;                  ///< cinematic handle
	int style;                      ///<
	int border;                     ///<
	int ownerDraw;                  ///< ownerDraw style
	int ownerDrawFlags;             ///< show flags for ownerdraw items
	float borderSize;               ///<
	int flags;                      ///< visible, focus, mouseover, cursor
	rectDef_t rectEffects;          ///< for various effects
	rectDef_t rectEffects2;         ///< for various effects
	int offsetTime;                 ///< time based value for various effects
	int nextTime;                   ///< time next effect should cycle
	vec4_t foreColor;               ///< text color
	vec4_t backColor;               ///< border color
	vec4_t borderColor;             ///< border color
	vec4_t outlineColor;            ///< border color
	qhandle_t background;           ///< background asset
} windowDef_t;

typedef windowDef_t Window;

/**
 * @struct colorRangeDef_s
 * @brief
 */
typedef struct
{
	vec4_t color;
	int type;
	float low;
	float high;
} colorRangeDef_t;

// FIXME: combine flags into bitfields to save space
// FIXME: consolidate all of the common stuff in one structure for menus and items
// THINKABOUTME: is there any compelling reason not to have items contain items
// and do away with a menu per say.. major issue is not being able to dynamically allocate
// and destroy stuff.. Another point to consider is adding an alloc free call for vm's and have
// the engine just allocate the pool for it based on a cvar
// many of the vars are re-used for different item types, as such they are not always named appropriately
// the benefits of c++ in DOOM will greatly help crap like this
// FIXME: need to put a type ptr that points to specific type info per type
//
#define MAX_LB_COLUMNS 16

/**
 * @struct columnInfo_s
 * @typedef columnInfo_t
 * @brief
 */
typedef struct columnInfo_s
{
	int pos;
	int width;
	int maxChars;
} columnInfo_t;

/**
 * @struct listBoxDef_s
 * @typedef listBoxDef_t
 * @brief
 */
typedef struct listBoxDef_s
{
	int startPos;
	int endPos;
	int drawPadding;
	int cursorPos;
	float elementWidth;
	float elementHeight;
	int elementStyle;
	int numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char *doubleClick;
	const char *contextMenu;
	qboolean notselectable;
} listBoxDef_t;

/**
 * @struct editFieldDef_s
 * @typedef editFieldDef_t
 * @brief
 */
typedef struct editFieldDef_s
{
	float minVal;                   ///<  edit field limits
	float maxVal;                   ///<
	float defVal;                   ///<
	float range;                    ///<
	int maxChars;                   ///< for edit fields
	int maxPaintChars;              ///< for edit fields
	int paintOffset;                ///<
} editFieldDef_t;

#define MAX_MULTI_CVARS 32

/**
 * @struct multiDef_s
 * @typedef multiDef_t
 * @brief
 */
typedef struct multiDef_s
{
	const char *cvarList[MAX_MULTI_CVARS];
	const char *cvarStr[MAX_MULTI_CVARS];
	float cvarValue[MAX_MULTI_CVARS];
	int count;
	qboolean strDef;
	const char *undefinedStr;
} multiDef_t;

/**
 * @struct modelDef_s
 * @typedef modelDef_t
 * @brief
 */
typedef struct modelDef_s
{
	int angle;
	vec3_t origin;
	float fov_x;
	float fov_y;
	int rotationSpeed;

	int animated;
	int startframe;
	int numframes;
	int loopframes;
	int fps;

	int frame;
	int oldframe;
	float backlerp;
	int frameTime;
} modelDef_t;

#define CVAR_ENABLE     0x00000001
#define CVAR_DISABLE    0x00000002
#define CVAR_SHOW       0x00000004
#define CVAR_HIDE       0x00000008
#define CVAR_NOTOGGLE   0x00000010

// "setting" flags for items
#define SVS_DISABLED_SHOW   0x01
#define SVS_ENABLED_SHOW    0x02

#define UI_MAX_TEXT_LINES 64

/**
 * @struct itemDef_s
 * @typedef itemDef_t
 * @brief
 */
typedef struct itemDef_s
{
	Window window;                  ///< common positional, border, style, layout info
	rectDef_t textRect;             ///< rectangle the text ( if any ) consumes
	int type;                       ///< text, button, radiobutton, checkbox, textfield, listbox, combo
	int alignment;                  ///< left center right
	int textalignment;              ///< ( optional ) alignment for text within rect based on text width
	float textalignx;               ///< ( optional ) text alignment x coord
	float textaligny;               ///< ( optional ) text alignment x coord
	float textscale;                ///< scale percentage from 72pts
	int font;                       ///<
	int textStyle;                  ///< ( optional ) style, normal and shadowed are it for now
	const char *text;               ///< display text
	void *parent;                   ///< menu owner
	qhandle_t asset;                ///< handle to asset
	const char *mouseEnterText;     ///< mouse enter script
	const char *mouseExitText;      ///< mouse exit script
	const char *mouseEnter;         ///< mouse enter script
	const char *mouseExit;          ///< mouse exit script
	const char *action;             ///< select script
	const char *onAccept;           ///< run when the users presses the enter key
	const char *onFocus;            ///< select script
	const char *leaveFocus;         ///< select script
	const char *cvar;               ///< associated cvar
	const char *cvarTest;           ///< associated cvar for enable actions
	const char *enableCvar;         ///< enable, disable, show, or hide based on value, this can contain a list
	int cvarFlags;                  ///<  what type of action to take on cvarenables
	sfxHandle_t focusSound;
	int numColors;                  ///< number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	int colorRangeType;             ///< either
	int special;                    ///< used for feeder id's etc.. diff per type
	int cursorPos;                  ///< cursor position in characters
	void *typeData;                 ///< type specific data ptr's

	//      For the bot menu, we have context sensitive menus
	//      the way it works, we could have multiple items in a menu with the same hotkey
	//      so in the mission pack, we search through all the menu items to find the one that is applicable to this key press
	//      so the item has to store both the hotkey and the command to execute
	int hotkey;
	const char *onKey;

	// on-the-fly enable/disable of items
	int settingTest;
	int settingFlags;
	int voteFlag;

	const char *onTab;
	const char *onEsc;
	const char *onEnter;
	const char *onPaste;

	struct itemDef_s *toolTipData;  ///< Tag an item to this item for auto-help popups

	/// ETL: checkbox for bitflags in an integer cvar. The value it has is the bitvalue (1,2,4,8 etc)
	int bitflag;

	vec4_t scrollColor;
	vec4_t sliderColor;
} itemDef_t;

/**
 * @struct menuDef_s
 * @brief
 */
typedef struct
{
	Window window;
	const char *font;               ///< font
	qboolean fullScreen;            ///< covers entire screen
	int itemCount;                  ///< number of items;
	int fontIndex;                  ///<
	int cursorItem;                 ///< which item as the cursor
	int fadeCycle;                  ///<
	float fadeClamp;                ///<
	float fadeAmount;               ///<
	const char *onOpen;             ///< run when the menu is first opened
	const char *onClose;            ///< run when the menu is closed
	const char *onESC;              ///< run when the escape key is hit
	const char *onEnter;            ///< run when the enter key is hit
	const char *onPaste;            ///< run when the paste action is activated

	int timeout;                    ///< milliseconds until menu times out
	int openTime;                   ///< time menu opened
	const char *onTimeout;          ///< run when menu times out

	const char *onKey[MAX_KEYS];    ///< execs commands when a key is pressed
	const char *soundName;          ///< background loop sound for menu

	vec4_t focusColor;              ///< focus color for items
	vec4_t disableColor;            ///< focus color for items
	itemDef_t *items[MAX_MENUITEMS]; ///< items this menu contains

	/// TODO: Should we search through all the items to find the hotkey instead of using the onKey array?
	/// The bot command menu needs to do this, see note above
	qboolean itemHotkeyMode;
} menuDef_t;

#define UI_FONT_COUNT 6

/**
 * @struct cachedAssets_s
 * @brief
 */
typedef struct
{
	const char *fontStr;
	const char *cursorStr;
	const char *gradientStr;
	fontHelper_t fonts[UI_FONT_COUNT];
	fontHelper_t bg_loadscreenfont1;
	fontHelper_t bg_loadscreenfont2;
	qhandle_t cursor;
	qhandle_t gradientBar;
	qhandle_t gradientRound;
	qhandle_t scrollBarArrowUp;
	qhandle_t scrollBarArrowDown;
	qhandle_t scrollBarArrowLeft;
	qhandle_t scrollBarArrowRight;
	qhandle_t scrollBar;
	qhandle_t scrollBarThumb;
	qhandle_t buttonMiddle;
	qhandle_t buttonInside;
	qhandle_t solidBox;
	qhandle_t sliderBar;
	qhandle_t sliderThumb;
	qhandle_t checkboxCheck;
	qhandle_t checkboxCheckNot;
	qhandle_t checkboxCheckNo;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float fadeClamp;
	int fadeCycle;
	float fadeAmount;
	float shadowX;
	float shadowY;
	vec4_t shadowColor;
	float shadowFadeClamp;
	qboolean fontRegistered;

	// player settings
	qhandle_t fxBasePic;
	qhandle_t fxPic[7];
	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t crosshairAltShader[NUM_CROSSHAIRS];

} cachedAssets_t;

/**
 * @struct commandDef_s
 * @brief
 */
typedef struct
{
	const char *name;
	void (*handler)(itemDef_t *item, qboolean *bAbort, char **args);
} commandDef_t;

/**
 * @struct displayContextDef_s
 * @brief
 */
typedef struct
{
	qhandle_t (*registerShaderNoMip)(const char *p);
	void (*setColor)(const vec4_t v);
	void (*drawHandlePic)(float x, float y, float w, float h, qhandle_t asset);
	void (*drawStretchPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
	void (*drawText)(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
	void (*drawTextExt)(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font);
	int (*textWidth)(const char *text, float scale, int limit);
	int (*textWidthExt)(const char *text, float scale, int limit, fontHelper_t *font);
	int (*multiLineTextWidth)(const char *text, float scale, int limit);
	int (*textHeight)(const char *text, float scale, int limit);
	int (*textHeightExt)(const char *text, float scale, int limit, fontHelper_t *font);
	int (*multiLineTextHeight)(const char *text, float scale, int limit);
	void (*textFont)(int font);
	qhandle_t (*registerModel)(const char *p);
	void (*modelBounds)(qhandle_t model, vec3_t min, vec3_t max);
	void (*fillRect)(float x, float y, float w, float h, const vec4_t color);
	void (*drawRect)(float x, float y, float w, float h, float size, const vec4_t color);
	void (*drawSides)(float x, float y, float w, float h, float size);
	void (*drawTopBottom)(float x, float y, float w, float h, float size);
	void (*clearScene)(void);
	void (*addRefEntityToScene)(const refEntity_t *re);
	void (*renderScene)(const refdef_t *fd);
	void (*registerFont)(const char *pFontname, int pointSize, void *font);
	void (*ownerDrawItem)(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, int special, float scale, vec4_t color, qhandle_t shader, int textStyle);
	float (*getValue)(int ownerDraw, int type);
	qboolean (*ownerDrawVisible)(int flags);
	void (*runScript)(char **p);
	void (*getTeamColor)(vec4_t *color);
	void (*getCVarString)(const char *cvar, char *buffer, int bufsize);
	float (*getCVarValue)(const char *cvar);
	void (*setCVar)(const char *cvar, const char *value);
	void (*drawTextWithCursor)(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style);
	void (*drawTextWithCursorExt)(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style, fontHelper_t *font);
	void (*setOverstrikeMode)(qboolean b);
	qboolean (*getOverstrikeMode)(void);
	void (*startLocalSound)(sfxHandle_t sfx, int channelNum);
	qboolean (*ownerDrawHandleKey)(int ownerDraw, int flags, int *special, int key);
	int (*feederCount)(int feederID);
	const char *(*feederItemText)(int feederID, int index, int column, qhandle_t *handles, int *numhandles);
	const char *(*fileText)(const char *fileName);
	qhandle_t (*feederItemImage)(int feederID, int index);
	void (*feederSelection)(int feederID, int index);
	qboolean (*feederSelectionClick)(itemDef_t *item);
	void (*feederAddItem)(int feederID, const char *name, int index);
	const char *(*translateString)(const char *string);
	void (*checkAutoUpdate)(void);
	void (*getAutoUpdate)(void);

	void (*keynumToStringBuf)(int keynum, char *buf, int buflen);
	void (*getBindingBuf)(int keynum, char *buf, int buflen);
	void (*getKeysForBinding)(const char *binding, int *key1, int *key2);

	qboolean (*keyIsDown)(int keynum);

	void (*getClipboardData)(char *buf, size_t bufsize);

	void (*setBinding)(int keynum, const char *binding);
	void (*executeText)(int exec_when, const char *text);
	void (*Error)(int level, const char *error, ...) _attribute((noreturn, format(printf, 2, 3)));
	void (*Print)(const char *msg, ...) _attribute((format(printf, 1, 2)));
	void (*Pause)(qboolean b);
	int (*ownerDrawWidth)(int ownerDraw, float scale);
	sfxHandle_t (*registerSound)(const char *name, qboolean compressed);
	void (*startBackgroundTrack)(const char *intro, const char *loop, int fadeupTime);
	void (*stopBackgroundTrack)(void);
	int (*playCinematic)(const char *name, float x, float y, float w, float h);
	void (*stopCinematic)(int handle);
	void (*drawCinematic)(int handle, float x, float y, float w, float h);
	void (*runCinematicFrame)(int handle);

	// campaign stuffs
	const char *(*descriptionForCampaign)(void);
	const char *(*nameForCampaign)(void);
	void (*add2dPolys)(polyVert_t *verts, int numverts, qhandle_t hShader);
	void (*updateScreen)(void);
	void (*getHunkData)(int *hunkused, int *hunkexpected);
	int (*getConfigString)(int index, char *buff, size_t buffsize);

	float yscale;
	float xscale;
	float bias;
	int realTime;
	int frameTime;
	int cursorx;
	int cursory;
	qboolean debug;

	cachedAssets_t Assets;

	glconfig_t glconfig;
	qhandle_t whiteShader;
	qhandle_t gradientImage;
	qhandle_t cursor;
	float FPS;
} displayContextDef_t;

void PC_SourceError(int handle, const char *format, ...);
//void PC_SourceWarning(int handle, const char *format, ...); // Unused
qboolean PC_Float_Parse(int handle, float *f);
qboolean PC_Color_Parse(int handle, vec4_t *c);
qboolean PC_Int_Parse(int handle, int *i);
qboolean PC_Rect_Parse(int handle, rectDef_t *r);
qboolean PC_String_Parse(int handle, const char **out);
qboolean PC_Script_Parse(int handle, const char **out);
qboolean PC_Char_Parse(int handle, char *out);

// Window
void Window_Init(Window *w);
void Window_CloseCinematic(windowDef_t *window);
void Window_Paint(Window *w, float fadeAmount, float fadeClamp, float fadeCycle);

void GradientBar_Paint(rectDef_t *rect, vec4_t color);
void GradientRound_Paint(float x, float y, float w, float h, vec4_t color);

// Display
void Init_Display(displayContextDef_t *dc);
int Display_VisibleMenuCount(void);
void Display_CloseCinematics(void);
displayContextDef_t *Display_GetContext(void);
void *Display_CaptureItem(int x, int y);
qboolean Display_MouseMove(void *p, int x, int y);
int Display_CursorType(int x, int y);
qboolean Display_KeyBindPending(void);
void Display_CacheAll(void);
void Display_HandleKey(int key, qboolean down, int x, int y);

// Script
qboolean Float_Parse(char **p, float *f);
qboolean Color_Parse(char **p, vec4_t *c);
qboolean Int_Parse(char **p, int *i);
qboolean Rect_Parse(char **p, rectDef_t *r);
qboolean String_Parse(char **p, const char **out);
qboolean Script_Parse(char **p, const char **out);

// Menu
void Menu_Init(menuDef_t *menu);
void Menu_PostParse(menuDef_t *menu);
menuDef_t *Menu_GetFocused(void);
void Menus_OpenByName(const char *p);
menuDef_t *Menus_FindByName(const char *p);
void Menus_ShowByName(const char *p);
void Menus_CloseByName(const char *p);
void Menu_HandleKey(menuDef_t *menu, int key, qboolean down);
void Menu_HandleMouseMove(menuDef_t *menu, float x, float y);
void Menu_ScrollFeeder(menuDef_t *menu, int feeder, qboolean down);
int Menu_Count(void);
menuDef_t *Menu_Get(int handle);
void Menu_New(int handle);
void Menu_PaintAll(void);
menuDef_t *Menus_ActivateByName(const char *p, qboolean modalStack);
void Menu_Reset(void);
qboolean Menus_AnyFullScreenVisible(void);
void  Menus_Activate(menuDef_t *menu);
qboolean Menus_CaptureFuncActive(void);
void Menu_UpdatePosition(menuDef_t *menu);
itemDef_t *Menu_SetPrevCursorItem(menuDef_t *menu);
itemDef_t *Menu_SetNextCursorItem(menuDef_t *menu);
void Menu_CloseCinematics(menuDef_t *menu);
void Menu_FadeMenuByName(const char *p, qboolean *bAbort, qboolean fadeOut);
void Menu_TransitionItemByName(menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt);
void Menu_OrbitItemByName(menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time);
void Menu_RunCloseScript(menuDef_t *menu);
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);
void Menu_FadeItemByName(menuDef_t *menu, const char *p, qboolean fadeOut);
itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *p);
itemDef_t *Menu_ClearFocus(menuDef_t *menu);
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);
void Menu_SetupKeywordHash(void);
qboolean Menu_OverActiveItem(menuDef_t *menu, float x, float y);
void Menus_CloseAll(void);
void Menu_Paint(menuDef_t *menu, qboolean forcePaint);
void Menu_SetFeederSelection(menuDef_t *menu, int feeder, int index, const char *name);
qboolean Menu_Parse(int handle, menuDef_t *menu);

// Item
void Item_Init(itemDef_t *item);
qboolean Item_Bind_HandleKey(itemDef_t *item, int key, qboolean down);
qboolean Item_Combo_HandleKey(itemDef_t *item, int key);
void Item_ComboDeSelect(itemDef_t *item);
qboolean Item_TextField_HandleKey(itemDef_t *item, int key);
void Item_HandleTextFieldDeSelect(itemDef_t *item);
qboolean Item_HandleKey(itemDef_t *item, int key, qboolean down);
void Item_Action(itemDef_t *item);
qboolean Item_EnableShowViaCvar(itemDef_t *item, int flag);
void Item_HandleTextFieldSelect(itemDef_t *item);
rectDef_t *Item_CorrectedTextRect(itemDef_t *item);
qboolean Item_SettingShow(itemDef_t *item, qboolean fVoteTest);
void Item_MouseEnter(itemDef_t *item, float x, float y);
void Item_MouseLeave(itemDef_t *item);
void Item_SetMouseOver(itemDef_t *item, qboolean focus);
void Item_Paint(itemDef_t *item);
void Item_SetupKeywordHash(void);
void Item_SetScreenCoords(itemDef_t *item, float x, float y);
qboolean Item_SetFocus(itemDef_t *item, float x, float y);
void Item_HandleSaveValue(void);
qboolean Item_ListBox_HandleKey(itemDef_t *item, int key, qboolean down, qboolean force);
void Item_UpdatePosition(itemDef_t *item);
void Item_RunScript(itemDef_t *item, qboolean *bAbort, const char *s);
void Item_Tooltip_Initialize(itemDef_t *item);
void Item_MouseActivate(itemDef_t *item);
void Item_KeyboardActivate(itemDef_t *item);

// Generic
const char *String_Alloc(const char *p);
void String_Init(void);
void String_Report(void);
qboolean IsVisible(int flags);
void ToWindowCoords(float *x, float *y, windowDef_t *window);
void Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount);
qboolean FileExists(const char *filename);
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
qboolean Rect_ContainsPoint(rectDef_t *rect, float x, float y);
qboolean Rect_ContainsPointN(rectDef_t *rect, float x, float y);

void *UI_Alloc(int size);
void UI_InitMemory(void);
qboolean UI_OutOfMemory(void);

void Controls_GetConfig(void);
void Controls_SetConfig(qboolean restart);
void Controls_SetDefaults(qboolean lefthanded);

int trap_PC_AddGlobalDefine(char *define);
int trap_PC_RemoveAllGlobalDefines(void);
int trap_PC_LoadSource(const char *filename);
int trap_PC_FreeSource(int handle);
int trap_PC_ReadToken(int handle, pc_token_t *pc_token);
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line);
int trap_PC_UnReadToken(int handle);

// panelhandling

typedef struct panel_button_s panel_button_t;

/**
 * @struct panel_button_text_s
 * @typedef panel_button_text_t
 * @brief
 */
typedef struct panel_button_text_s
{
	float scalex, scaley;
	vec4_t colour;
	int style;
	int align;
	fontHelper_t *font;
} panel_button_text_t;

typedef qboolean (*panel_button_key_down)(panel_button_t *, int);
typedef qboolean (*panel_button_key_up)(panel_button_t *, int);
typedef void (*panel_button_render)(panel_button_t *);
typedef void (*panel_button_postprocess)(panel_button_t *);

/**
 * @struct panel_button_s
 * @typedef panel_button_t
 * @brief Button struct
 */
struct panel_button_s
{
	// compile time stuff
	// ======================
	const char *shaderNormal;

	/// text
	char *text;

	/// rect
	rectDef_t rect;

	/// data
	int data[8];

	/// "font"
	panel_button_text_t *font;

	// functions
	panel_button_key_down onKeyDown;
	panel_button_key_up onKeyUp;
	panel_button_render onDraw;
	panel_button_postprocess onFinish;

	// run-time stuff
	// ======================
	qhandle_t hShaderNormal;
};

void BG_PanelButton_RenderEdit(panel_button_t *button);
qboolean BG_PanelButton_EditClick(panel_button_t *button, int key);
qboolean BG_PanelButtonsKeyEvent(int key, qboolean down, panel_button_t **buttons);
void BG_PanelButtonsSetup(panel_button_t **buttons);
void BG_PanelButtonsRender(panel_button_t **buttons);
void BG_PanelButtonsRender_Text(panel_button_t *button);
void BG_PanelButtonsRender_TextExt(panel_button_t *button, const char *text);
void BG_PanelButtonsRender_Img(panel_button_t *button);
panel_button_t *BG_PanelButtonsGetHighlightButton(panel_button_t **buttons);
void BG_PanelButtons_SetFocusButton(panel_button_t *button);
panel_button_t *BG_PanelButtons_GetFocusButton(void);

qboolean BG_RectContainsPoint(float x, float y, float w, float h, float px, float py);
qboolean BG_CursorInRect(rectDef_t *rect);

void BG_FitTextToWidth_Ext(char *instr, float scale, float w, size_t size, fontHelper_t *font);

void AdjustFrom640(float *x, float *y, float *w, float *h);

void Cui_WideRect(rectDef_t *rect);
float Cui_WideX(float x);
float Cui_WideXoffset(void);
void C_PanelButtonsSetup(panel_button_t **buttons, float xoffset);      // called from UI & CGAME

//A simple macro to check if a certain functionality is available in the engine version
#ifdef CGAMEDLL
#define IS_FUNC_SUPPORTED(x) (cg.etLegacyClient >= x)
#else
#define IS_FUNC_SUPPORTED(x) (uiInfo.etLegacyClient >= x)
#endif

#define RegisterFont(fontName, pointSize, font) Q_UTF8_RegisterFont(fontName, pointSize, font, IS_FUNC_SUPPORTED(UNICODE_SUPPORT_VERSION), &trap_R_RegisterFont)
#define Q_UTF8_GlyphScale(font) ((fontInfo_t *)font->fontData)->glyphScale
#define Q_UTF8_GetGlyph(font, string) font->GetGlyph(font->fontData, Q_UTF8_CodePoint(string))
//#define Q_UTF8_GetGlyph(font, string) Q_UTF8_GetGlyphSafe(font, IS_FUNC_SUPPORTED(UNICODE_SUPPORT_VERSION), string)

#endif // #ifndef INCLUDE_UI_SHARED_H
