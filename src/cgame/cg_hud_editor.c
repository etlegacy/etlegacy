/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file cg_hud_editor.c
 * @brief Sets up and draws in-game HUD editor
 *
 */

#include "cg_local.h"

#define SOUNDEVENT(sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND)

#define SOUND_SELECT    SOUNDEVENT(cgs.media.sndLimboSelect)
#define SOUND_FILTER    SOUNDEVENT(cgs.media.sndLimboFilter)

// grouping hud editing fields
#define INPUT_WIDTH 50
#define INPUT_HEIGHT 16
#define CHECKBOX_SIZE 16
#define SLIDERS_WIDTH 170
#define SLIDERS_HEIGHT 16
#define BUTTON_WIDTH 55
#define BUTTON_HEIGHT 16

#define HUDEDITOR_CONTROLS_SPACER_XY 4
#define HUDEDITOR_TITLE_SPACER_Y (BUTTON_HEIGHT + 8)
#define HUDEDITOR_CATEGORY_SPACER_Y 8

#define HUDEDITOR_SELECTHUD_Y 6

#define HUDEDITOR_HUD_NAME_Y (HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y + (BUTTON_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 2))

#define HUDEDITOR_SIZEPOS_Y (HUDEDITOR_SELECTHUD_Y + BUTTON_HEIGHT + HUDEDITOR_TITLE_SPACER_Y + (BUTTON_HEIGHT * 4) + \
							 HUDEDITOR_CONTROLS_SPACER_XY * 2 + HUDEDITOR_CATEGORY_SPACER_Y)

#define HUDEDITOR_TAB_Y (HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y * 2 + HUDEDITOR_CATEGORY_SPACER_Y + \
						 (INPUT_HEIGHT * 2) + HUDEDITOR_CONTROLS_SPACER_XY)

#define HUDEDITOR_OPTION_Y (HUDEDITOR_TAB_Y + HUDEDITOR_CATEGORY_SPACER_Y + \
							(INPUT_HEIGHT * 2) + HUDEDITOR_CONTROLS_SPACER_XY)

#define HUDEDITOR_BOTTOM_Y (SCREEN_HEIGHT * HUD_EDITOR_SIZE_COEFF) - BUTTON_HEIGHT

enum
{
	HUD_COLOR_SELECTION_MAIN,
	HUD_COLOR_SELECTION_SECONDARY,
	HUD_COLOR_SELECTION_BACKGROUND,
	HUD_COLOR_SELECTION_BORDER,
};

typedef enum hudShowLayout_e
{
	HUD_SHOW_LAYOUT_OFF,
	HUD_SHOW_LAYOUT_VISIBLE_ONLY,
	HUD_SHOW_LAYOUT_ALL,
} hudShowLayout_t;

typedef enum hudShowGrid_e
{
	HUD_SHOW_GRID_OFF,
	HUD_SHOW_GRID_OCD_LEVEL1,
	HUD_SHOW_GRID_OCD_LEVEL2,
	HUD_SHOW_GRID_OCD_LEVEL3,
	HUD_SHOW_GRID_MAX,
} hudShowGrid_t;

typedef enum hudGridScale_e
{
	HUD_GRID_SCALE_1,
	HUD_GRID_SCALE_2,
	HUD_GRID_SCALE_3,
	HUD_GRID_SCALE_MAX,
} hudGridScale_t;

char *barFlagsString[BAR_MAX] =
{
	"Left",
	"Center",
	"Vertical",
	"No Alpha",
	"Bar Bckgrnd",
	"X0 Y5",
	"X0 Y0",
	"Lerp Color",
	"Bar Border",
	"Border Tiny",
	"Decor",
	"Icon",
	"Needle",
	"Circular",
	NULL,
};

float    HUDEditorX;
float    HUDEditorWidth;
float    HUDEditorCenterX;
qboolean wsAdjusted = qfalse;

static panel_button_t  *lastFocusComponent;
static qboolean        lastFocusComponentMoved;
static int             elementColorSelection;
static panel_button_t  *lastButtonTabSelected;
static panel_button_t  *lastButtonColorSelected;
static qboolean        forceGridAlignment = qfalse;
static hudShowLayout_t showLayout         = HUD_SHOW_LAYOUT_OFF;
static qboolean        showMicroGrid      = qfalse;
static hudShowGrid_t   showGrid           = HUD_SHOW_GRID_OFF;
static hudGridScale_t  gridScale          = HUD_GRID_SCALE_1;

static void CG_HudEditorUpdateFields(panel_button_t *button);
static qboolean CG_HudEditor_Dropdown_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditor_HudDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_HudRenderDropdown(panel_button_t *button);
static qboolean CG_HudEditor_StyleTextDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_StyleTextRenderDropdown(panel_button_t *button);
static qboolean CG_HudEditor_AlignTextDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_AlignTextRenderDropdown(panel_button_t *button);
static void CG_HudEditor_SetupTitleText(panel_button_t *button);
static qboolean CG_HudEditor_EditKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorPanel_EditKeyUp(panel_button_t *button, int key);
static qboolean CG_HudEditorPanel_KeyUp(panel_button_t *button, int key);
static qboolean CG_HudEditor_ParentDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_ParentRenderDropdown(panel_button_t *button);
static void CG_HudEditor_RenderEditName(panel_button_t *button);
static void CG_HudEditorName_Finish(panel_button_t *button);
static void CG_HudEditor_RenderEdit(panel_button_t *button);
static void CG_HudEditorX_Finish(panel_button_t *button);
static void CG_HudEditorY_Finish(panel_button_t *button);
static void CG_HudEditorWidth_Finish(panel_button_t *button);
static void CG_HudEditorHeight_Finish(panel_button_t *button);
static void CG_HudEditorEdit_Finish(panel_button_t *button);
static qboolean CG_HudEditoTabSelection_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditoColorSelection_KeyDown(panel_button_t *button, int key);
static void CG_HudEditorRender_Button(panel_button_t *button);
static void CG_HudEditorRender_HelpButton(panel_button_t *button);
static qboolean CG_HudEditorVisible_CheckboxKeyDown(panel_button_t *button, int key);
static void CG_HudEditor_RenderCheckbox(panel_button_t *button);
static qboolean CG_HudEditorStyle_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorBarStyle_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorShowBackground_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorShowBorder_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorAutoAdjust_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorButton_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorButton_KeyUp(panel_button_t *button, int key);
static qboolean CG_HudEditorHelpButton_KeyDown(panel_button_t *button, int key);
static void CG_DrawHudEditor_ComponentLists(panel_button_t *button);
static qboolean CG_HudEditor_ComponentLists_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditor_ComponentLists_KeyUp(panel_button_t *button, int key);
static qboolean CG_HudEditorColor_KeyDown(panel_button_t *button, int key);
static void CG_HudEditorColor_Render(panel_button_t *button);
static void CG_HudEditor_Slider_Render(panel_button_t *button);

// Font declaration

static panel_button_text_t hudEditorHeaderFont =
{
	0.2f,                  0.2f,
	{ 1.f,                 1.f, 1.f,  0.5f },
	ITEM_TEXTSTYLE_NORMAL, 0,
	&cgs.media.limboFont1,
};


static panel_button_text_t hudEditorTextFont =
{
	0.24f,                   0.24f,
	{ 1.f,                   1.f,  1.f,0.75f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorButtonSelectedFont =
{
	0.24f,                   0.24f,
	{ 1.f,                   1.f,  0.f,0.75f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorTextTitleFont =
{
	0.3f,                    0.3f,
	{ 1.f,                   1.f, 1.f,  1.f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorTextWarningFont =
{
	0.3f,                  0.3f,
	{ 1.f,                 0.f, 0.f,  1.f },
	ITEM_TEXTSTYLE_BLINK,  0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorFont_Dropdown =
{
	0.24f,                   0.24f,
	{ 1.f,                   1.f,  1.f,0.5f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

// HUD Editor panel button

static panel_button_t hudEditorHudDropdown =
{
	NULL,
	"hudeditor_huds",
	{ 0,                           HUDEDITOR_SELECTHUD_Y,BUTTON_WIDTH * 2.5f, BUTTON_HEIGHT },
	{ 0,                           0,         0,                   0, 0, 0, 0, 1 },
	&hudEditorFont_Dropdown,       // font
	CG_HudEditor_Dropdown_KeyDown, // keyDown
	CG_HudEditor_HudDropdown_KeyUp,// keyUp
	CG_HudEditor_HudRenderDropdown,
	NULL,
	0,
};

static panel_button_t hudEditorHudName =
{
	NULL,
	"hudeditor_name",
	{ 0,                        HUDEDITOR_HUD_NAME_Y,INPUT_WIDTH * 2, INPUT_HEIGHT },
	{ 0,                        0,        0,               0, 0, 0, 0, 1},
	&hudEditorTextFont,         // font
	CG_HudEditor_EditKeyDown,   // keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEditName,
	CG_HudEditorName_Finish,
	0,
};

static panel_button_t hudEditorHudParent =
{
	NULL,
	"hudeditor_parent",
	{ 0,                              HUDEDITOR_HUD_NAME_Y + INPUT_HEIGHT + HUDEDITOR_CONTROLS_SPACER_XY,INPUT_WIDTH * 2.2f, INPUT_HEIGHT },
	{ 0,                              0,                                                    0,                  0, 0, 0, 0, 1},
	&hudEditorFont_Dropdown,          // font
	CG_HudEditor_Dropdown_KeyDown,    // keyDown
	CG_HudEditor_ParentDropdown_KeyUp,// keyUp
	CG_HudEditor_ParentRenderDropdown,
	NULL,
	0,
};

static panel_button_t hudEditorPositionSizeTitle =
{
	NULL,
	"Position & Size",
	{ 0,                        HUDEDITOR_SIZEPOS_Y,70, 14 },
	{ 0,                        0,      0,  0, 0, 0, 0, 1},
	&hudEditorTextTitleFont,    // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_HudEditor_SetupTitleText,
	NULL,
	0
};

static panel_button_t hudEditorX =
{
	NULL,
	"hudeditor_X",
	{ 0,                    HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y - INPUT_HEIGHT,INPUT_WIDTH, INPUT_HEIGHT },
	// [0] used by ui_shared EditClick, [1] link to hud, [2] used by ui_shared EditClick [3] additional data like colorRGB, [4] differentiate between hud editor element and hud element
	{ 0,                    0,                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorX_Finish,
	0
};

static panel_button_t hudEditorY =
{
	NULL,
	"hudeditor_Y",
	{ 0,                    HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y - INPUT_HEIGHT,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorY_Finish,
	0
};

static panel_button_t hudEditorW =
{
	NULL,
	"hudeditor_W",
	{ 0,                     HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CONTROLS_SPACER_XY,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorWidth_Finish,
	0
};

static panel_button_t hudEditorH =
{
	NULL,
	"hudeditor_H",
	{ 0,                      HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CONTROLS_SPACER_XY,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                      0,                                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,       // font
	CG_HudEditor_EditKeyDown, // keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorHeight_Finish,
	0
};

static panel_button_t hudEditorVisible =
{
	NULL,
	"Visible",
	{ 0,                        HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + CHECKBOX_SIZE + HUDEDITOR_CONTROLS_SPACER_XY * 2,CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                            0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorVisible_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorAutoAdjust =
{
	NULL,
	"AutoAdj",
	{ 0,                        HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + CHECKBOX_SIZE + HUDEDITOR_CONTROLS_SPACER_XY * 2,CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                            0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorAutoAdjust_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorShowBackground =
{
	NULL,
	"Background",
	{ 0,                        HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + CHECKBOX_SIZE * 2 + HUDEDITOR_CONTROLS_SPACER_XY * 3,CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                             0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorShowBackground_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorShowBorder =
{
	NULL,
	"Border",
	{ 0,                        HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + CHECKBOX_SIZE * 2 + HUDEDITOR_CONTROLS_SPACER_XY * 3,CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                                 0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorShowBorder_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorColorButton =
{
	NULL,
	"Color",
	{ 0,                      HUDEDITOR_TAB_Y,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,            0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoTabSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorStyleButton =
{
	NULL,
	"Style",
	{ 0,                      HUDEDITOR_TAB_Y,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,            0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoTabSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorFontButton =
{
	NULL,
	"Font",
	{ 0,                      HUDEDITOR_TAB_Y + BUTTON_HEIGHT + HUDEDITOR_CONTROLS_SPACER_XY,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,                                                            0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoTabSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorBarButton =
{
	NULL,
	"Bar",
	{ 0,                      HUDEDITOR_TAB_Y + BUTTON_HEIGHT + HUDEDITOR_CONTROLS_SPACER_XY,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,                                                             0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoTabSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorFeedButton =
{
	NULL,
	"Feed",
	{ 0,                      HUDEDITOR_TAB_Y + BUTTON_HEIGHT + HUDEDITOR_CONTROLS_SPACER_XY,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,                                                            0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoTabSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorColorSelectionMain =
{
	NULL,
	"Main",
	{ 0,                      HUDEDITOR_OPTION_Y,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,                0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorColorSelectionSecondary =
{
	NULL,
	"Second",
	{ 0,                      HUDEDITOR_OPTION_Y,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,              0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};


static panel_button_t hudEditorColorSelectionBorder =
{
	NULL,
	"Border",
	{ 0,                      HUDEDITOR_OPTION_Y + HUDEDITOR_CONTROLS_SPACER_XY + BUTTON_HEIGHT,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,                                                             0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorColorSelectionBackground =
{
	NULL,
	"Backgrnd",
	{ 0,                      HUDEDITOR_OPTION_Y + HUDEDITOR_CONTROLS_SPACER_XY + BUTTON_HEIGHT,BUTTON_WIDTH * 1.5f, BUTTON_HEIGHT },
	{ 0,                      0,                                                           0,                   5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static void CG_HudEditorColor_Finish(panel_button_t *button);

static panel_button_t hudEditorColorRed =
{
	NULL,
	"hudeditor_Red",
	{ 0,                     HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 2),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                   0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorSliderR =
{
	NULL,
	"hudeditor_colorsliderRed",
	{ 0,                       HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 3),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                       0,                                                        0,             0, 0, 0, 0, 1  },
	&hudEditorTextFont,        // font
	CG_HudEditorColor_KeyDown, // keyDown
	CG_HudEditorPanel_KeyUp,   // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorColorGreen =
{
	NULL,
	"hudeditor_Green",
	{ 0,                     HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 4) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                 0,           1, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorSliderG =
{
	NULL,
	"hudeditor_colorsliderGreen",
	{ 0,                         HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 5) + (HUDEDITOR_CONTROLS_SPACER_XY * 5),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                         0,                                                      0,             1, 0, 0, 0, 1  },
	&hudEditorTextFont,          // font
	CG_HudEditorColor_KeyDown,   // keyDown
	CG_HudEditorPanel_KeyUp,     // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorColorBlue =
{
	NULL,
	"hudeditor_Blue",
	{ 0,                     HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 6) + (HUDEDITOR_CONTROLS_SPACER_XY * 6),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                  0,           2, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorSliderB =
{
	NULL,
	"hudeditor_colorsliderBlue",
	{ 0,                        HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 7) + (HUDEDITOR_CONTROLS_SPACER_XY * 7),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                        0,                                                       0,             2, 0, 0, 0, 1  },
	&hudEditorTextFont,         // font
	CG_HudEditorColor_KeyDown,  // keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorColorAlpha =
{
	NULL,
	"hudeditor_Alpha",
	{ 0,                     HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 8) + (HUDEDITOR_CONTROLS_SPACER_XY * 8),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                 0,           3, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorSliderA =
{
	NULL,
	"hudeditor_colorsliderAlpha",
	{ 0,                         HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 9) + (HUDEDITOR_CONTROLS_SPACER_XY * 9),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                         0,                                                      0,             3, 0, 0, 0, 1  },
	&hudEditorTextFont,          // font
	CG_HudEditorColor_KeyDown,   // keyDown
	CG_HudEditorPanel_KeyUp,     // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorFontScale =
{
	NULL,
	"hudeditor_Scale",
	{ 0,                    HUDEDITOR_OPTION_Y,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,     0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorFontScaleSlider =
{
	NULL,
	"hudeditor_Scale",
	{ 0,                       HUDEDITOR_OPTION_Y + (BUTTON_HEIGHT * 1) + (HUDEDITOR_CONTROLS_SPACER_XY * 1),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                       0,                                                                0,             0, 1, 300, 0, 1},
	&hudEditorTextFont,        // font
	CG_HudEditorColor_KeyDown, // keyDown, borrowing color keydown func
	CG_HudEditorPanel_KeyUp,   // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};

static panel_button_t hudEditorFontStyle =
{
	NULL,
	"hudeditor_StyleText",
	{ 0,                                 HUDEDITOR_OPTION_Y + (BUTTON_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 2),100, BUTTON_HEIGHT },
	{ 0,                                 0,                                                            0,   0, 0, 0, 0, 1 },
	&hudEditorFont_Dropdown,             // font
	CG_HudEditor_Dropdown_KeyDown,       // keyDown
	CG_HudEditor_StyleTextDropdown_KeyUp,// keyUp
	CG_HudEditor_StyleTextRenderDropdown,
	NULL,
	0,
};

static panel_button_t hudEditorFontAlign =
{
	NULL,
	"hudeditor_Align",
	{ 0,                                 HUDEDITOR_OPTION_Y + (BUTTON_HEIGHT * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 3),60, BUTTON_HEIGHT },
	{ 0,                                 0,                                                                0,  0, 0, 0, 0, 1 },
	&hudEditorFont_Dropdown,             // font
	CG_HudEditor_Dropdown_KeyDown,       // keyDown
	CG_HudEditor_AlignTextDropdown_KeyUp,// keyUp
	CG_HudEditor_AlignTextRenderDropdown,
	NULL,
	0,
};

static panel_button_t hudEditorStyle =
{
	NULL,
	"Style",
	{ 0,                        HUDEDITOR_OPTION_Y,CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,               0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorStyle_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorCircleDensity =
{
	NULL,
	"hudeditor_Density",
	{ 0,                    HUDEDITOR_OPTION_Y,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,   0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorCircleSliderD =
{
	NULL,
	"hudeditor_CircleSliderDensity",
	{ 0,                            HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 1) + (HUDEDITOR_CONTROLS_SPACER_XY * 1),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                            0,                                                   0,             0, 1, 2, 0, 1  },
	&hudEditorTextFont,             // font
	CG_HudEditorColor_KeyDown,      // keyDown
	CG_HudEditorPanel_KeyUp,        // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};

static panel_button_t hudEditorCircleStartAngle =
{
	NULL,
	"hudeditor_Start Angle",
	{ 0,                    HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 2),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                           0,           1, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorCircleSliderStarAngle =
{
	NULL,
	"hudeditor_CircleSliderStart",
	{ 0,                          HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 3),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                          0,                                                     0,             0, 1, 360, 0, 1},
	&hudEditorTextFont,           // font
	CG_HudEditorColor_KeyDown,    // keyDown
	CG_HudEditorPanel_KeyUp,      // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};


static panel_button_t hudEditorCircleEndAngle =
{
	NULL,
	"hudeditor_End Angle",
	{ 0,                    HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 4) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                             0,           2, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorCircleSliderEndAngle =
{
	NULL,
	"hudeditor_CircleSliderEnd",
	{ 0,                        HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 5) + (HUDEDITOR_CONTROLS_SPACER_XY * 5),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                        0,                                                       0,             0, 1, 360, 0, 1},
	&hudEditorTextFont,         // font
	CG_HudEditorColor_KeyDown,  // keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};

static panel_button_t hudEditorCircleThickness =
{
	NULL,
	"hudeditor_Thickness",
	{ 0,                    HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 6) + (HUDEDITOR_CONTROLS_SPACER_XY * 6),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                             0,           3, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorCircleSliderThickness =
{
	NULL,
	"hudeditor_CircleSliderThickness",
	{ 0,                              HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 7) + (HUDEDITOR_CONTROLS_SPACER_XY * 7),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                              0,                                                 0,             0, 1, 8, 0, 1  },
	&hudEditorTextFont,               // font
	CG_HudEditorColor_KeyDown,        // keyDown
	CG_HudEditorPanel_KeyUp,          // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};

static panel_button_t hudEditorBarStyle =
{
	NULL,
	"Bar Style",
	{ 0,                        HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 8) + (HUDEDITOR_CONTROLS_SPACER_XY * 8),CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                       0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorBarStyle_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorFeedTime =
{
	NULL,
	"hudeditor_Time",
	{ 0,                    HUDEDITOR_OPTION_Y,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,      0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorFeedTimeSlider =
{
	NULL,
	"hudEditor_FeedTimeSlider",
	{ 0,                       HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 1) + (HUDEDITOR_CONTROLS_SPACER_XY * 1),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                       0,                                                        0,             0, 1, 6000, 0, 1},
	&hudEditorTextFont,        // font
	CG_HudEditorColor_KeyDown, // keyDown
	CG_HudEditorPanel_KeyUp,   // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};

static panel_button_t hudEditorFeedStayTime =
{
	NULL,
	"hudeditor_Stay Time",
	{ 0,                    HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 2),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                             0,           1, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorFeedStayTimeSlider =
{
	NULL,
	"hudEditor_FeedStayTimeSlider",
	{ 0,                           HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 3),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                           0,                                                    0,             0, 1, 6000, 0, 1},
	&hudEditorTextFont,            // font
	CG_HudEditorColor_KeyDown,     // keyDown
	CG_HudEditorPanel_KeyUp,       // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};


static panel_button_t hudEditorFeedFadeTime =
{
	NULL,
	"hudeditor_Fade Time",
	{ 0,                    HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 4) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                             0,           2, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditKeyDown,// keyDown
	CG_HudEditorPanel_EditKeyUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorEdit_Finish,
	0
};

static panel_button_t hudEditorFeedFadeTimeSlider =
{
	NULL,
	"hudEditor_FeedFadeTimeSlider",
	{ 0,                           HUDEDITOR_OPTION_Y + (SLIDERS_HEIGHT * 5) + (HUDEDITOR_CONTROLS_SPACER_XY * 5),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                           0,                                                    0,             0, 1, 6000, 0, 1},
	&hudEditorTextFont,            // font
	CG_HudEditorColor_KeyDown,     // keyDown
	CG_HudEditorPanel_KeyUp,       // keyUp
	CG_HudEditor_Slider_Render,
	NULL,
	0
};

static panel_button_t hudEditorSave =
{
	NULL,
	"Save",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                              0,            0, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorClone =
{
	NULL,
	"Clone",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                             0,            1, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorDelete =
{
	NULL,
	"Delete",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                            0,            2, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorResetComp =
{
	NULL,
	"Reset Component",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y + BUTTON_HEIGHT + HUDEDITOR_CONTROLS_SPACER_XY,(BUTTON_WIDTH * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 2) - 1, BUTTON_HEIGHT },
	{ 0,                      0,                                                                                  0,                                                           3, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorHelp =
{
	NULL,
	"Toggle Help",
	{ 0,                          HUDEDITOR_BOTTOM_Y,(BUTTON_WIDTH * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 4) - 1, BUTTON_HEIGHT },
	{ 0,                          0,         0,                                                           1, 0, 0, 0, 1 },
	&hudEditorTextFont,           // font
	CG_HudEditorHelpButton_KeyDown,// keyDown
	NULL,                         // keyUp
	CG_HudEditorRender_HelpButton,
	NULL,
	0
};

static panel_button_t hudEditorWarningLabel =
{
	NULL,
	"CANNOT MODIFY DEFAULT HUDS",
	{ 0,                         HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y + BUTTON_HEIGHT * 2,(BUTTON_WIDTH * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 2), BUTTON_HEIGHT },
	{ 0,                         0,                                            0,                                                       0, 0, 0, 0, 1 },
	&hudEditorTextWarningFont,   // font
	NULL,                        // keyDown
	NULL,                        // keyUp
	CG_HudEditor_SetupTitleText,
	NULL,
	0
};

static panel_button_t hudEditorComponentsList =
{
	NULL,
	"hudeditor_componentsList",
	{ 3,                            SCREEN_HEIGHT + 6,SCREEN_WIDTH, SCREEN_HEIGHT_SAFE * 0.28 },
	{ 0,                            0,  0,            0, 0, 0, 0, 1             },
	&hudEditorFont_Dropdown,        // font
	CG_HudEditor_ComponentLists_KeyDown,// keyDown
	CG_HudEditor_ComponentLists_KeyUp,// keyUp
	CG_DrawHudEditor_ComponentLists,
	NULL,
	0
};

static panel_button_t *hudEditor[] =
{
	&hudEditorSave,           &hudEditorClone,       &hudEditorDelete,      &hudEditorResetComp, &hudEditorWarningLabel,
	&hudEditorFontButton,     &hudEditorColorButton, &hudEditorStyleButton, &hudEditorBarButton, &hudEditorFeedButton,
	&hudEditorComponentsList, &hudEditorHudName,
	&hudEditorHelp,

	// Below here all components that should draw on top
	&hudEditorHudParent,      &hudEditorHudDropdown,
	NULL,
};

static panel_button_t *hudEditorGenericTab[] =
{
	&hudEditorPositionSizeTitle, &hudEditorX,          &hudEditorY,
	&hudEditorW,                 &hudEditorH,
	&hudEditorVisible,           &hudEditorAutoAdjust, &hudEditorShowBackground,&hudEditorShowBorder,
	NULL,
};

static panel_button_t *hudEditorFontTab[] =
{
	&hudEditorFontScale, &hudEditorFontScaleSlider,
	&hudEditorFontAlign, &hudEditorFontStyle,
	NULL,
};

static panel_button_t *hudEditorColorTab[] =
{
	&hudEditorColorSelectionMain, &hudEditorColorSelectionSecondary, &hudEditorColorSelectionBorder, &hudEditorColorSelectionBackground,
	&hudEditorColorRed,           &hudEditorColorGreen,              &hudEditorColorBlue,            &hudEditorColorAlpha,
	&hudEditorColorSliderR,       &hudEditorColorSliderG,            &hudEditorColorSliderB,         &hudEditorColorSliderA,
	NULL,
};

static panel_button_t *hudEditorBarTab[] =
{
	&hudEditorCircleDensity, &hudEditorCircleStartAngle,      &hudEditorCircleEndAngle,       &hudEditorCircleThickness,
	&hudEditorCircleSliderD, &hudEditorCircleSliderStarAngle, &hudEditorCircleSliderEndAngle, &hudEditorCircleSliderThickness,
	NULL,
};

static panel_button_t *hudEditorFeedTab[] =
{
	&hudEditorFeedTime,     &hudEditorFeedTimeSlider,
	&hudEditorFeedStayTime, &hudEditorFeedStayTimeSlider,
	&hudEditorFeedFadeTime, &hudEditorFeedFadeTimeSlider,
	NULL,
};

/**
 * @brief CG_HudSave
 * @param[in] HUDToDuplicate
 * @param[in] HUDToDelete
 */
qboolean CG_HudSave(int HUDToDuplicate, int HUDToDelete)
{
	hudStucture_t *hud, *hud2;

	if (HUDToDelete > 0 && !CG_GetHudByNumber(HUDToDelete)->isEditable)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: can't delete defaults HUDs\n");
		return qfalse;
	}

	if (HUDToDuplicate >= 0)
	{
		int attemptedSpot;
		int hudNumber;

		if (hudData.count == MAXHUDS)
		{
			CG_Printf(S_COLOR_RED "ERROR CG_HudSave: no more free HUD slots for clone\n");
			return qfalse;
		}

		hud       = CG_GetHudByNumber(HUDToDuplicate);
		hud2      = CG_GetFreeHud();
		hudNumber = hud2->hudnumber;

		CG_CloneHud(hud2, hud);

		// determine a new name
		for (attemptedSpot = 1; attemptedSpot < MAXHUDS; attemptedSpot++)
		{
			int      i;
			qboolean collision = qfalse;

			Q_strncpyz(hud2->name,
			           (attemptedSpot == 1) ? va("%s_copy", hud->name) : va("%s_copy%d", hud->name, attemptedSpot),
			           sizeof(hud2->name));

			for (i = 0; i < hudData.count; i++)
			{
				hudStucture_t *otherHud = hudData.list[i];

				if (!Q_stricmp(otherHud->name, hud2->name))
				{
					CG_Printf("Hud name clone collision with '%s', trying higher suffixes...\n", otherHud->name);
					collision = qtrue;
				}

			}

			if (!collision)
			{
				goto successfully_determined_new_name;
			}
		}

		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: tried to create a new duplicate, but found no free spot\n");
		return qfalse;

successfully_determined_new_name:

		Q_strncpyz(hud2->parent, hud->name, sizeof(hud2->parent));
		hud2->parentNumber = hud->hudnumber;
		hud2->hudnumber    = hudNumber;
		hud2->isEditable   = qtrue;

		CG_RegisterHud(hud2);

		hudData.active = hud2;
		//cg_altHud.integer = hud2->hudnumber;
		trap_Cvar_Set("cg_altHud", hud2->name);

		CG_Printf("Clone hud %d on number %d\n", HUDToDuplicate, hud2->hudnumber);
	}

	if (HUDToDelete > 0 && CG_GetHudByNumber(HUDToDelete)->isEditable)
	{
		while ((hud = CG_GetHudByNumber(HUDToDelete)))
		{
			// ensure to update parent as well
			CG_UpdateParentHUD(hud->name, hud->parent, hud->hudnumber);

			if (hud == hudData.active)
			{
				trap_Cvar_Set("cg_altHud", "0");
				cg_altHud.integer = 0;
				hudData.active    = CG_GetHudByNumber(0);
			}

			CG_FreeHud(hud);
		}
	}

	return CG_WriteHudsToFile();
}

/**
* @brief CG_HudEditor_SetupTitleText
* @param button
*/
static void CG_HudEditor_SetupTitleText(panel_button_t *button)
{
	float textWidth;

	// draw warning label only if HUD 0 is set
	if (button == &hudEditorWarningLabel && hudData.active->isEditable)
	{
		return;
	}
	textWidth      = CG_Text_Width_Ext(button->text, button->font->scalex, 0, button->font->font);
	button->rect.x = HUDEditorCenterX - (textWidth * 0.5f);
	BG_PanelButtonsRender_Text(button);
}

static qboolean CG_HudEditor_EditKeyDown(panel_button_t *button, int key)
{
	// don't modify default HUD
	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	return BG_PanelButton_EditClick(button, key);
}

/**
 * @brief CG_HudEditorPanel_EditUp
 * @param button
 * @param key
 * @return
 */
static qboolean CG_HudEditorPanel_EditKeyUp(panel_button_t *button, int key)
{
	return qtrue;
}

/**
* @brief CG_HudEditor_SetupEditPosition
* @param button
* @param label
* @param totalWidth
*/
static void CG_HudEditor_SetupEditPosition(panel_button_t *button, float totalWidth)
{
	// there's seemingly redundant repetition in this function, but we need explicit
	// calculation for every single editfield because client might be using
	// proportional custom font, so totalWidth doesn't necessarily match between X and W for example

	if (button == &hudEditorHudName || button == &hudEditorX || button == &hudEditorW)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.15f);
	}
	else if (button == &hudEditorY || button == &hudEditorH)
	{
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.15f);
	}
	else if (button == &hudEditorColorRed || button == &hudEditorColorGreen
	         || button == &hudEditorColorBlue || button == &hudEditorColorAlpha
	         || button == &hudEditorFontScale
	         || button == &hudEditorCircleDensity || button == &hudEditorCircleStartAngle
	         || button == &hudEditorCircleEndAngle || button == &hudEditorCircleThickness
	         || button == &hudEditorFeedTime || button == &hudEditorFeedStayTime
	         || button == &hudEditorFeedFadeTime)
	{
		// centered
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.20f);
	}
}

/**
* @brief CG_HudEditor_RenderEditName
* @param button
*/
static void CG_HudEditor_RenderEditName(panel_button_t *button)
{
	float      textWidth, textHeight, totalWidth;
	const char *label = hudData.active->isEditable ? "Name: " : "CLONE IT TO DO MODIFICATION";

	textWidth  = CG_Text_Width_Ext(label, button->font->scalex, 0, button->font->font);
	textHeight = CG_Text_Height_Ext(label, button->font->scaley, 0, button->font->font);
	totalWidth = textWidth + button->rect.w;

	if (hudData.active->isEditable)
	{
		CG_HudEditor_SetupEditPosition(button, totalWidth);
	}
	else
	{
		button->rect.x = HUDEditorCenterX - (textWidth * 0.5f);
	}

	CG_Text_Paint_Ext(button->rect.x, button->rect.y + (button->rect.h * 0.5f) + (textHeight * 0.5f),
	                  button->font->scalex, button->font->scaley, hudData.active->isEditable ? colorWhite : colorRed, label, 0, 0,
	                  hudData.active->isEditable ? button->font->style : ITEM_TEXTSTYLE_BLINK, button->font->font);

	if (hudData.active->isEditable)
	{
		button->rect.x += textWidth;
		CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, colorBlack);

		button->rect.x += 2; // for spacing
		button->rect.y -= button->rect.h * 0.5f - (textHeight * 0.5f);
		BG_PanelButton_RenderEdit(button);
		button->rect.x -= 2;
		button->rect.y += button->rect.h * 0.5f - (textHeight * 0.5f);
	}
}

/**
* @brief CG_HudEditorName_Finish
* @param button
*/
static void CG_HudEditorName_Finish(panel_button_t *button)
{
	char buffer[MAX_EDITFIELD] = { 0 };

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	// check name validity (no doublon, no default hud name, not empty)
	if (buffer[0] == '\0' || CG_GetHudByName(buffer))
	{
		// back to default name
		trap_Cvar_Set(button->text, hudData.active->name);
	}
	else
	{
		// ensure to update parent as well
		CG_UpdateParentHUD(hudData.active->name, buffer, hudData.active->hudnumber);

		Q_strncpyz(hudData.active->name, buffer, sizeof(hudData.active->name));
	}

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditor_RenderEdit
* @param button
*/
static void CG_HudEditor_RenderEdit(panel_button_t *button)
{
	char  label[32] = { 0 };
	char  *p        = NULL;
	char  *tmp      = "";
	float textWidth, textHeight, totalWidth;

	Q_strncpyz(label, button->text, 32);

	p = strtok(label, "_");
	while (p)
	{
		tmp = p;
		p   = strtok(NULL, "_");
	}

	// whitespace after ":" for spacing
	Com_sprintf(label, sizeof(label), "%s: \n", tmp);

	textWidth  = CG_Text_Width_Ext(label, button->font->scalex, 0, button->font->font);
	textHeight = CG_Text_Height_Ext(label, button->font->scaley, 0, button->font->font);
	totalWidth = textWidth + button->rect.w;

	CG_HudEditor_SetupEditPosition(button, totalWidth);

	CG_Text_Paint_Ext(button->rect.x - textWidth, button->rect.y + (button->rect.h * 0.5f) + (textHeight / 2),
	                  button->font->scalex, button->font->scaley, colorWhite, label, 0, 0,
	                  button->font->style, button->font->font);

	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, colorBlack);

	button->rect.x += 2; // for spacing
	button->rect.y -= button->rect.h * 0.5f - (textHeight * 0.5f);
	BG_PanelButton_RenderEdit(button);
	button->rect.x -= 2;
	button->rect.y += button->rect.h * 0.5f - (textHeight * 0.5f);
}

/**
* @brief CG_HudEditorX_Finish
* @param button
*/
static void CG_HudEditorX_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.x = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorY_Finish
* @param button
*/
static void CG_HudEditorY_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.y = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorWidth_Finish
* @param button
*/
static void CG_HudEditorWidth_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.w = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorHeight_Finish
* @param button
*/
static void CG_HudEditorHeight_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.h = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorEdit_Finish
* @param button
*/
static void CG_HudEditorEdit_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];
	float          *optionValue;

	// get the correct option to modify depending on slider
	if (button == &hudEditorFontScale)
	{
		optionValue = &comp->scale;
	}
	else if (button == &hudEditorCircleDensity)
	{
		optionValue = &comp->circleDensityPoint;
	}
	else if (button == &hudEditorCircleStartAngle)
	{
		optionValue = &comp->circleStartAngle;
	}
	else if (button == &hudEditorCircleEndAngle)
	{
		optionValue = &comp->circleEndAngle;
	}
	else if (button == &hudEditorCircleThickness)
	{
		optionValue = &comp->circleThickness;
	}
	else if (button == &hudEditorFeedTime)
	{
		optionValue = &comp->feedTime;
	}
	else if (button == &hudEditorFeedStayTime)
	{
		optionValue = &comp->feedStayTime;
	}
	else if (button == &hudEditorFeedFadeTime)
	{
		optionValue = &comp->feedFadeTime;
	}
	else
	{
		return;
	}

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	*optionValue = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorVisible_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorVisible_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	comp->visible = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorStyle_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorStyle_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	comp->style    ^= button->data[3];
	button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorBarStyle_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorBarStyle_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	comp->barStyle ^= button->data[3];
	button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorShowBackground_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorShowBackground_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	comp->showBackGround = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorShowBorder_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorShowBorder_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	comp->showBorder = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorAutoAdjust_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorAutoAdjust_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)hudData.active + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!hudData.active->isEditable)
	{
		return qfalse;
	}

	comp->autoAdjust = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditor_SetupCheckBoxPosition
* @param button
* @param label
* @param totalWidth
*/
static void CG_HudEditor_SetupCheckBoxPosition(panel_button_t *button, float totalWidth)
{
	if (button == &hudEditorVisible)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;
	}
	else if (button == &hudEditorAutoAdjust)
	{
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;
	}
	else if (button == &hudEditorShowBackground)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;
	}
	else if (button == &hudEditorShowBorder)
	{
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;
	}
	else if (button == &hudEditorStyle || button == &hudEditorBarStyle)
	{
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;
	}
}

/**
* @brief CG_HudEditor_RenderCheckbox
* @param button
*/
static void CG_HudEditor_RenderCheckbox(panel_button_t *button)
{
	char  labelText[32];
	float textWidth;
	float textHeight;
	float totalWidth;
	float textOffsetY;
	float scalex = button->font->scalex;

	Com_sprintf(labelText, sizeof(labelText), "%s ", button->text);

	textWidth = CG_Text_Width_Ext(labelText, scalex, 0, button->font->font);

	if (textWidth >= (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f)
	{
		scalex    = ((HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f) / CG_Text_Width_Ext(labelText, 1, 0, button->font->font) - 0.02f;
		textWidth = CG_Text_Width_Ext(labelText, scalex, 0, button->font->font);
	}

	textHeight  = CG_Text_Height_Ext(labelText, button->font->scaley, 0, button->font->font);
	totalWidth  = textWidth;
	textOffsetY = (CHECKBOX_SIZE - textHeight) * 0.5f;

	CG_HudEditor_SetupCheckBoxPosition(button, totalWidth);

	CG_Text_Paint_Ext(button->rect.x - totalWidth, button->rect.y + textHeight + textOffsetY, scalex, button->font->scaley,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, CHECKBOX_SIZE, CHECKBOX_SIZE, 2, colorBlack);

	if (button->data[2])
	{
		CG_DrawPic(button->rect.x + 2, button->rect.y + 2, CHECKBOX_SIZE - 3, CHECKBOX_SIZE - 3, cgs.media.readyShader);
	}
}

static panel_button_t styleCheckBox[MAXSTYLES];
static panel_button_t *styleCheckBoxPanel[MAXSTYLES + 1];

/**
* @brief CG_HudEditor_UpdateCheckboxStyle
* @param[in] style
*/
static void CG_HudEditor_UpdateCheckboxStyle(int style)
{
	int                        i;
	char                       *styleName;
	rectDef_t                  rect = hudEditorStyle.rect;
	const hudComponentFields_t *hudCompField;

	Com_Memset(styleCheckBox, 0, sizeof(styleCheckBox));
	Com_Memset(styleCheckBoxPanel, 0, sizeof(styleCheckBoxPanel));

	if (!lastFocusComponent)
	{
		return;
	}

	hudCompField = &hudComponentFields[lastFocusComponent->data[0]];

	for (i = 0, styleName = hudCompField->styles[i] ; styleName; styleName = hudCompField->styles[++i])
	{
		// off number, shift X
		rect.x = ((i & 1) ? HUDEditorCenterX : HUDEditorX) + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;

		styleCheckBox[i].text      = styleName;
		styleCheckBox[i].rect      = rect;
		styleCheckBox[i].onKeyDown = hudEditorStyle.onKeyDown;
		styleCheckBox[i].onKeyUp   = hudEditorStyle.onKeyUp;
		styleCheckBox[i].onDraw    = hudEditorStyle.onDraw;
		styleCheckBox[i].font      = hudEditorStyle.font;
		styleCheckBox[i].data[1]   = lastFocusComponent->data[0];
		styleCheckBox[i].data[2]   = style & BIT(i);
		styleCheckBox[i].data[3]   = BIT(i);

		styleCheckBoxPanel[i] = &styleCheckBox[i];

		// was odd number, shift to next line
		if (i & 1)
		{
			rect.y += rect.h + HUDEDITOR_CONTROLS_SPACER_XY;
		}
	}
}

static panel_button_t barStyleCheckBox[BAR_MAX];
static panel_button_t *barStyleCheckBoxPanel[BAR_MAX + 1];

/**
* @brief CG_HudEditor_UpdateCheckboxBarStyle
* @param[in] style
*/
static void CG_HudEditor_UpdateCheckboxBarStyle(int style)
{
	int       i;
	rectDef_t rect = hudEditorBarStyle.rect;

	Com_Memset(barStyleCheckBox, 0, sizeof(barStyleCheckBox));
	Com_Memset(barStyleCheckBoxPanel, 0, sizeof(barStyleCheckBoxPanel));

	if (!lastFocusComponent || hudComponentFields[lastFocusComponent->data[0]].type != HUD_COMP_TYPE_BAR)
	{
		return;
	}

	for (i = 0; barFlagsString[i]; ++i)
	{
		// off number, shift X
		rect.x = ((i & 1) ? HUDEditorCenterX : HUDEditorX) + (HUDEditorWidth * 0.5f) - CHECKBOX_SIZE * 1.5f;

		barStyleCheckBox[i].text      = barFlagsString[i];
		barStyleCheckBox[i].rect      = rect;
		barStyleCheckBox[i].onKeyDown = hudEditorBarStyle.onKeyDown;
		barStyleCheckBox[i].onKeyUp   = hudEditorBarStyle.onKeyUp;
		barStyleCheckBox[i].onDraw    = hudEditorBarStyle.onDraw;
		barStyleCheckBox[i].font      = hudEditorBarStyle.font;
		barStyleCheckBox[i].data[1]   = lastFocusComponent->data[0];
		barStyleCheckBox[i].data[2]   = style & BIT(i);
		barStyleCheckBox[i].data[3]   = BIT(i);

		barStyleCheckBoxPanel[i] = &barStyleCheckBox[i];

		// was odd number, shift to next line
		if (i & 1)
		{
			rect.y += rect.h + HUDEDITOR_CONTROLS_SPACER_XY;
		}
	}
}

/**
* @brief CG_HudEditor_HudRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_HudRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "HUD: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.3f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.3f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (BUTTON_HEIGHT - textHeight) * 0.5f;

	button->rect.x = HUDEditorCenterX - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.3f, 0.3f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;
	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, va("%s", hudData.active->name),
	                   button == BG_PanelButtons_GetFocusButton(), button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; i < hudData.count; i++)
		{
			hudStucture_t *hud = hudData.list[i];

			if (hud->hudnumber == hudData.active->hudnumber)
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, va("%s", hud->name), button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

/**
* @brief CG_HudEditor_ParentRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_ParentRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "Parent: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.3f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.3f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (BUTTON_HEIGHT - textHeight) * 0.5f;

	button->rect.x = HUDEditorCenterX - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.3f, 0.3f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;
	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, hudData.active->parentNumber >= 0 ? hudData.list[hudData.active->parentNumber]->name : "No Parent",
	                   button == BG_PanelButtons_GetFocusButton(), button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		if (hudData.active->parentNumber != -1)
		{
			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, "No Parent", button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		for (i = 0; i < hudData.count; i++)
		{
			hudStucture_t *hud = hudData.list[i];

			if (hud->hudnumber == hudData.active->hudnumber)
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, hud->name, button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

static const char *styleTextString[] =
{
	"NORMAL",
	"BLINK",
	"PULSE",
	"SHADOWED",
	"OUTLINED",
	"OUTLINESHADOWED",
	"SHADOWEDMORE",
	NULL
};

/**
* @brief CG_HudEditor_StyleTextRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_StyleTextRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "Style: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.24f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.24f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (CHECKBOX_SIZE - textHeight) * 0.5f;

	button->rect.x = HUDEditorX + (HUDEditorWidth * 0.5f) - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.24f, 0.24f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;

	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, styleTextString[button->data[2]], button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; styleTextString[i] != NULL; i++)
		{
			if (!Q_stricmp(styleTextString[button->data[2]], styleTextString[i]))
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, styleTextString[i], button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

static const char *alignTextString[] =
{
	"LEFT",
	"CENTER",
	"RIGHT",
	"CENTER2",
	NULL
};

/**
* @brief CG_HudEditor_AlignTextRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_AlignTextRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "Align: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.24f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.24f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (BUTTON_HEIGHT - textHeight) * 0.5f;

	button->rect.x = HUDEditorX + (HUDEditorWidth * 0.5f) - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.24f, 0.24f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;

	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, alignTextString[button->data[2]], button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; alignTextString[i] != NULL; i++)
		{
			if (!Q_stricmp(alignTextString[button->data[2]], alignTextString[i]))
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, alignTextString[i], button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

static panel_button_t hudEditorHudDropdown;
static panel_button_t hudEditorCompDropdown;

/**
* @brief CG_HudEditor_Dropdown_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_Dropdown_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		// don't modify default HUD but allow selecting comp and hud
		if (hudData.active->isEditable || button == &hudEditorHudDropdown || button == &hudEditorCompDropdown)
		{
			BG_PanelButtons_SetFocusButton(button);
			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_HudDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_HudDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; i < hudData.count; i++)
			{
				hudStucture_t *hud = hudData.list[i];

				if (hud->hudnumber == hudData.active->hudnumber)
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					trap_Cvar_Set(cgs.clientinfo[cg.clientNum].shoutcaster ? "cg_shoutcasterHud" : "cg_altHud", hud->name);
					//cg_altHud.integer = hud->hudnumber;
					CG_SetHud();

					if (lastFocusComponent)
					{
						CG_HudEditorUpdateFields(lastFocusComponent);
					}

					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_HudEditor_ReplaceDefaultParent
 * @param[in] newParentNumber
 */
static void CG_HudEditor_ReplaceDefaultParent(int newParentNumber)
{
	int           i;
	hudStucture_t *parenthud    = CG_GetHudByNumber(hudData.active->parentNumber);
	hudStucture_t *newParenthud = CG_GetHudByNumber(newParentNumber);

	for (i = 0; hudComponentFields[i].name; i++)
	{
		int            locationChanged = qfalse;
		hudComponent_t *comp           = (hudComponent_t *) ((char *) hudData.active + hudComponentFields[i].offset);
		hudComponent_t *parentComp     = (hudComponent_t *) ((char *) parenthud + hudComponentFields[i].offset);
		hudComponent_t *newParentComp  = (hudComponent_t *) ((char *) newParenthud + hudComponentFields[i].offset);

		if (hudComponentFields[i].isAlias)
		{
			continue;
		}

		if (comp->location.x == parentComp->location.x)
		{
			comp->location.x = newParentComp->location.x;
			locationChanged  = qtrue;
		}

		if (comp->location.y == parentComp->location.y)
		{
			comp->location.y = newParentComp->location.y;
			locationChanged  = qtrue;
		}

		if (comp->location.h == parentComp->location.h)
		{
			comp->location.h = newParentComp->location.h;
			locationChanged  = qtrue;
		}

		if (comp->location.w == parentComp->location.w)
		{
			comp->location.w = newParentComp->location.w;
			locationChanged  = qtrue;
		}

		if (comp->visible == parentComp->visible)
		{
			comp->visible = newParentComp->visible;
		}

		if (comp->style == parentComp->style)
		{
			comp->style = newParentComp->style;
		}

		if (comp->scale == parentComp->scale)
		{
			comp->scale = newParentComp->scale;
		}

		if (Vector4Compare(comp->colorMain, parentComp->colorMain))
		{
			Vector4Copy(newParentComp->colorMain, comp->colorMain);
		}

		if (Vector4Compare(comp->colorSecondary, parentComp->colorSecondary))
		{
			Vector4Copy(newParentComp->colorSecondary, comp->colorSecondary);
		}

		if (comp->showBackGround == parentComp->showBackGround)
		{
			comp->showBackGround = newParentComp->showBackGround;
		}

		if (Vector4Compare(comp->colorBackground, parentComp->colorBackground))
		{
			Vector4Copy(newParentComp->colorBackground, comp->colorBackground);
		}

		if (comp->showBorder == parentComp->showBorder)
		{
			comp->showBorder = newParentComp->showBorder;
		}

		if (Vector4Compare(comp->colorBorder, parentComp->colorBorder))
		{
			Vector4Copy(newParentComp->colorBorder, comp->colorBorder);
		}

		if (comp->styleText == parentComp->styleText)
		{
			comp->styleText = newParentComp->styleText;
		}

		if (comp->alignText == parentComp->alignText)
		{
			comp->alignText = newParentComp->alignText;
		}

		if (comp->autoAdjust == parentComp->autoAdjust)
		{
			comp->autoAdjust = newParentComp->autoAdjust;
		}

		if (locationChanged)
		{
			comp->anchorPoint        = newParentComp->anchorPoint;
			comp->parentAnchor.point = newParentComp->parentAnchor.point;
		}
        
        if (comp->barStyle == parentComp->barStyle)
        {
            comp->barStyle = newParentComp->barStyle;
        }

		if (comp->circleDensityPoint == parentComp->circleDensityPoint)
		{
			comp->circleDensityPoint = newParentComp->circleDensityPoint;
		}

		if (comp->circleStartAngle == parentComp->circleStartAngle)
		{
			comp->circleStartAngle = newParentComp->circleStartAngle;
		}

		if (comp->circleEndAngle == parentComp->circleEndAngle)
		{
			comp->circleEndAngle = newParentComp->circleEndAngle;
		}

		if (comp->circleThickness == parentComp->circleThickness)
		{
			comp->circleThickness = newParentComp->circleThickness;
		}
	}
}

/**
* @brief CG_HudEditor_ParentDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_ParentDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			// Handle no parent case first
			if (hudData.active->parentNumber != -1)
			{
				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					hudData.active->parentNumber = -1;
					Com_Memset(hudData.active->parent, 0, sizeof(hudData.active->parent));

					if (lastFocusComponent)
					{
						CG_HudEditorUpdateFields(lastFocusComponent);
					}

					BG_PanelButtons_SetFocusButton(NULL);

					return qtrue;
				}
			}

			for (i = 0; i < hudData.count; i++)
			{
				hudStucture_t *hud = hudData.list[i];

				// ignore self hud
				if (hud->hudnumber == hudData.active->hudnumber)
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					if (hudData.active->parentNumber != -1)
					{
						CG_HudEditor_ReplaceDefaultParent(hud->hudnumber);
					}
					hudData.active->parentNumber = hud->hudnumber;
					Q_strncpyz(hudData.active->parent, hud->name, sizeof(hudData.active->parent));

					if (lastFocusComponent)
					{
						CG_HudEditorUpdateFields(lastFocusComponent);
					}

					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_StyleTextDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_StyleTextDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; styleTextString[i] != NULL; i++)
			{
				if (!Q_stricmp(styleTextString[button->data[2]], styleTextString[i]))
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

					comp->styleText = button->data[2] = i;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_AlignTextDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_AlignTextDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; alignTextString[i] != NULL; i++)
			{
				if (!Q_stricmp(alignTextString[button->data[2]], alignTextString[i]))
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);

					comp->alignText = button->data[2] = i;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/*
static char *colorSelectionElement[] =
{
	"Text",
	"BckGrnd",
	"Border",
};
 */

static qboolean CG_HudEditoTabSelection_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		lastButtonTabSelected = button;

		if (lastFocusComponent)
		{
			CG_HudEditorUpdateFields(lastFocusComponent);
		}
		return qtrue;
	}

	return qfalse;
}

static qboolean CG_HudEditoColorSelection_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		lastButtonColorSelected = button;

		if (button == &hudEditorColorSelectionMain)
		{
			elementColorSelection = HUD_COLOR_SELECTION_MAIN;
		}
		else if (button == &hudEditorColorSelectionSecondary)
		{
			elementColorSelection = HUD_COLOR_SELECTION_SECONDARY;
		}
		else if (button == &hudEditorColorSelectionBackground)
		{
			elementColorSelection = HUD_COLOR_SELECTION_BACKGROUND;
		}
		else if (button == &hudEditorColorSelectionBorder)
		{
			elementColorSelection = HUD_COLOR_SELECTION_BORDER;
		}

		if (lastFocusComponent)
		{
			CG_HudEditorUpdateFields(lastFocusComponent);
		}
		return qtrue;
	}

	return qfalse;
}

static qboolean CG_HudEditorButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		button->data[4] = cg.time;

		CG_HudSave(-1, -1);

		return qtrue;
	}

	return qfalse;
}

static qboolean CG_HudEditorButton_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		button->data[4] = 0;

		return qtrue;
	}

	return qfalse;
}

static void CG_ResetComponent()
{
	if (lastFocusComponent)
	{
		hudComponent_t *comp;
		hudComponent_t *defaultComp;

		comp        = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[lastFocusComponent->data[0]].offset);
		defaultComp = (hudComponent_t *)((byte *) CG_GetHudByNumber(0) + hudComponentFields[lastFocusComponent->data[0]].offset);

		Com_Memcpy(comp, defaultComp, sizeof(hudComponent_t));

		CG_HudEditorUpdateFields(lastFocusComponent);
	}
}

/**
 * @brief CG_HudEditorRender_Button_Ext
 * @param[in] r
 * @param[in] text
 * @param[in] font
 */
void CG_HudEditorRender_Button_Ext(rectDef_t *r, const char *text, panel_button_text_t *font)
{
	vec4_t clrBdr = { 0.1f, 0.1f, 0.1f, 0.5f };
	vec4_t clrBck = { 0.3f, 0.3f, 0.3f, 0.4f };

	vec4_t clrBck_hi = { 0.5f, 0.5f, 0.5f, 0.4f };
	vec4_t clrTxt_hi = { 0.9f, 0.9f, 0.9f, 1.f };

	qboolean hilight = BG_CursorInRect(r);

	CG_FillRect(r->x, r->y, r->w, r->h, hilight ? clrBck_hi : clrBck);
	CG_DrawRect_FixedBorder(r->x, r->y, r->w, r->h, 1, clrBdr);

	if (text)
	{
		float w;

		w = CG_Text_Width_Ext(text, font->scalex, 0, font->font);

		CG_Text_Paint_Ext(r->x + ((r->w + 2) - w) * 0.5f, r->y + r->h / 1.5, font->scalex, font->scaley, hilight ? clrTxt_hi : font->colour, text, font->align, 0, font->style, font->font);
	}
}

/**
 * @brief CG_HudEditor_SetupButtonPosition
 * @param[in] button
 * @param[in] buttonW
 */
static float CG_HudEditor_SetupButtonPosition(panel_button_t *button, float buttonW)
{
	// left aligned (3 buttons)
	if (button == &hudEditorSave || button == &hudEditorResetComp)
	{
		button->rect.x = HUDEditorCenterX - (buttonW * 0.5f) - buttonW - HUDEDITOR_CONTROLS_SPACER_XY;
	}
	// centered (3 buttons)
	else if (button == &hudEditorClone)
	{
		button->rect.x = HUDEditorCenterX - (buttonW * 0.5f);
	}
	// right aligned(3 buttons)
	else if (button == &hudEditorDelete)
	{
		button->rect.x = HUDEditorCenterX + (buttonW * 0.5f) + HUDEDITOR_CONTROLS_SPACER_XY;
	}
	// left align (2 buttons)
	else if (button == &hudEditorColorSelectionMain || button == &hudEditorColorSelectionBackground
	         || button == &hudEditorColorButton || button == &hudEditorFontButton)
	{
		button->rect.x = HUDEditorX + HUDEDITOR_CONTROLS_SPACER_XY * 2;
	}
	// righ align (2 buttons)
	else if (button == &hudEditorColorSelectionSecondary || button == &hudEditorColorSelectionBorder
	         || button == &hudEditorStyleButton || button == &hudEditorBarButton || button == &hudEditorFeedButton)
	{
		button->rect.x = HUDEditorCenterX + HUDEDITOR_CONTROLS_SPACER_XY * 2;
	}

	return 0;
}

static void CG_HudEditorRender_HelpButton(panel_button_t *button)
{
	vec4_t clrBdr = { 0.1f, 0.1f, 0.1f, 0.5f };
	vec4_t clrBck = { 0.45f, 0.34f, 0.53f, 0.4f };

	vec4_t clrBck_hi = { 0.45f, 0.34f, 0.53f, 0.6f };
	vec4_t clrTxt_hi = { 0.9f, 0.9f, 0.9f, 1.f };

	rectDef_t *r;
	qboolean  hilight;

	// if default HUD, don't draw some component
	if (!hudData.active->isEditable)
	{
		if (button == &hudEditorSave || button == &hudEditorDelete || button == &hudEditorResetComp)
		{
			return;
		}
	}

	r       = &button->rect;
	hilight = BG_CursorInRect(r);

	CG_FillRect(r->x, r->y, r->w, r->h, hilight ? clrBck_hi : clrBck);
	CG_DrawRect_FixedBorder(r->x, r->y, r->w, r->h, 1, clrBdr);

	if (button->text)
	{
		float w;

		w = CG_Text_Width_Ext(button->text, button->font->scalex, 0, button->font->font);

		CG_Text_Paint_Ext(r->x + ((r->w + 2) - w) * 0.5f, r->y + r->h / 1.5, button->font->scalex, button->font->scaley, hilight ? clrTxt_hi : button->font->colour, button->text, button->font->align, 0, button->font->style, button->font->font);
	}

	trap_R_SetColor(NULL);
}

#define TIMER_KEYDOWN 500.f

/**
 * @brief Ensure we may display the options button depending of the comp type
 * @param[in] button
 * @return
 */
static qboolean CG_HUDEditorCheckTabDisplayValidity(panel_button_t *button)
{
	const hudComponentTypes_t type = hudComponentFields[lastFocusComponent->data[0]].type;

	// don't display bar customization button if comp isn't a bar
	if (button == &hudEditorBarButton && type != HUD_COMP_TYPE_BAR)
	{
		return qfalse;
	}

	// don't display font customization button if comp is a bar
	if (button == &hudEditorFontButton && type == HUD_COMP_TYPE_BAR)
	{
		return qfalse;
	}

	// don't display feed customization if comp isn't a feed comp
	if (button == &hudEditorFeedButton && type != HUD_COMP_TYPE_FEED)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_PanelButtonsRender_Button
 * @param[in] CG_HudEditorRender_Button
 */
static void CG_HudEditorRender_Button(panel_button_t *button)
{
	float               buttonW = Ccg_WideX(BUTTON_WIDTH);
	panel_button_text_t *font;

	// if default HUD or no comp selected, don't draw some buttons
	if (!hudData.active->isEditable)
	{
		if (button == &hudEditorSave || button == &hudEditorDelete || button == &hudEditorResetComp)
		{
			return;
		}
	}

	if (lastFocusComponent)
	{
		if (!CG_HUDEditorCheckTabDisplayValidity(button))
		{
			return;
		}
	}
	else if (button == &hudEditorBarButton || button == &hudEditorColorButton ||
	         button == &hudEditorFontButton || button == &hudEditorStyleButton ||
	         button == &hudEditorFeedButton)
	{
		// don't display all customization button as no comp is selected
		return;
	}

	CG_HudEditor_SetupButtonPosition(button, buttonW);

	if (button->data[4])
	{
		float curValue = (cg.time - button->data[4]) / TIMER_KEYDOWN;

		if (button->data[3])
		{
			vec4_t backG = { 1, 1, 1, 0.3f };
			CG_FilledBar(button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorRed, colorGreen, backG, backG, curValue, 0.f, BAR_LERP_COLOR, -1);
		}
		else
		{
			// HACK for save button, so it render dynamicly on pressing it
			CG_FillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, (vec4_t) { 0, 1.f, 0, curValue });
		}

		if (curValue > 1.f)
		{
			switch (button->data[3])
			{
			case 0:
				//CG_HudSave(-1, -1);
				break;
			case 1:
				CG_HudSave(hudData.active->hudnumber, -1);
				break;
			case 2:
				CG_HudSave(-1, hudData.active->hudnumber);
				break;
			case 3:
				CG_ResetComponent();
				break;
			default:
				break;
			}

			button->data[4] = 0;
		}
	}

	// highlight last selected button
	if (button == lastButtonTabSelected || button == lastButtonColorSelected)
	{
		font = &hudEditorButtonSelectedFont;
	}
	else
	{
		font = button->font;
	}
	CG_HudEditorRender_Button_Ext(&button->rect, button->text, font);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_HudEditorPanel_KeyUp
 * @param button
 * @param key
 * @return
 */
static qboolean CG_HudEditorPanel_KeyUp(panel_button_t *button, int key)
{
	BG_PanelButtons_SetFocusButton(NULL);
	return qtrue;
}

/**
* @brief CG_HudEditorUpdateFields
* @param[in] button
*/
static void CG_HudEditorUpdateFields(panel_button_t *button)
{
	hudComponent_t *comp;
	char           buffer[256];
	vec4_t(*compColor) = NULL;

	comp = (hudComponent_t *)((char *)hudData.active + hudComponentFields[button->data[0]].offset);

	// FIXME: add support for anchor setting in the hud editor!
	// we are just nuking the parent anchor here since the hud editor cannot set the anchors yet
	comp->parentAnchor.parent = NULL;

	// update the internal fields of the component
	CG_CalculateComponentInternals(hudData.active, comp);

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.x);
	trap_Cvar_Set("hudeditor_X", buffer);
	hudEditorX.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.y);
	trap_Cvar_Set("hudeditor_Y", buffer);
	hudEditorY.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.w);
	trap_Cvar_Set("hudeditor_W", buffer);
	hudEditorW.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.h);
	trap_Cvar_Set("hudeditor_H", buffer);
	hudEditorH.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->scale);
	trap_Cvar_Set("hudeditor_S", buffer);
	hudEditorFontScale.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->scale);
	trap_Cvar_Set("hudeditor_Scale", buffer);
	hudEditorFontScaleSlider.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleDensityPoint);
	trap_Cvar_Set("hudeditor_Density", buffer);
	hudEditorCircleDensity.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleDensityPoint);
	trap_Cvar_Set("hudeditor_CircleSliderDensity", buffer);
	hudEditorCircleSliderD.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleStartAngle);
	trap_Cvar_Set("hudeditor_Start Angle", buffer);
	hudEditorCircleStartAngle.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleStartAngle);
	trap_Cvar_Set("hudeditor_CircleSliderStart", buffer);
	hudEditorCircleSliderStarAngle.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleEndAngle);
	trap_Cvar_Set("hudeditor_End Angle", buffer);
	hudEditorCircleEndAngle.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleEndAngle);
	trap_Cvar_Set("hudeditor_CircleSliderEnd", buffer);
	hudEditorCircleSliderEndAngle.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleThickness);
	trap_Cvar_Set("hudeditor_Thickness", buffer);
	hudEditorCircleThickness.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->circleThickness);
	trap_Cvar_Set("hudeditor_CircleSliderThickness", buffer);
	hudEditorCircleSliderThickness.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.f", comp->feedTime);
	trap_Cvar_Set("hudeditor_Time", buffer);
	hudEditorFeedTime.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.f", comp->feedTime);
	trap_Cvar_Set("hudeditor_FeedTimeSlider", buffer);
	hudEditorFeedTimeSlider.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.f", comp->feedStayTime);
	trap_Cvar_Set("hudeditor_Stay Time", buffer);
	hudEditorFeedStayTime.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.f", comp->feedStayTime);
	trap_Cvar_Set("hudeditor_FeedStayTimeSlider", buffer);
	hudEditorFeedStayTimeSlider.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.f", comp->feedFadeTime);
	trap_Cvar_Set("hudeditor_Fade Time", buffer);
	hudEditorFeedFadeTime.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.f", comp->feedFadeTime);
	trap_Cvar_Set("hudeditor_FeedFadeTimeSlider", buffer);
	hudEditorFeedFadeTimeSlider.data[1] = button->data[0];

	switch (elementColorSelection)
	{
	case HUD_COLOR_SELECTION_MAIN:       compColor = &comp->colorMain;       break;
	case HUD_COLOR_SELECTION_SECONDARY:  compColor = &comp->colorSecondary;  break;
	case HUD_COLOR_SELECTION_BACKGROUND: compColor = &comp->colorBackground; break;
	case HUD_COLOR_SELECTION_BORDER:     compColor = &comp->colorBorder;     break;
	default: break;
	}

	if (compColor)
	{
		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[0] * 255.0f);
		trap_Cvar_Set("hudeditor_Red", buffer);
		hudEditorColorRed.data[1] = button->data[0];

		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[1] * 255.0f);
		trap_Cvar_Set("hudeditor_Green", buffer);
		hudEditorColorGreen.data[1] = button->data[0];

		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[2] * 255.0f);
		trap_Cvar_Set("hudeditor_Blue", buffer);
		hudEditorColorBlue.data[1] = button->data[0];

		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[3] * 255.0f);
		trap_Cvar_Set("hudeditor_Alpha", buffer);
		hudEditorColorAlpha.data[1] = button->data[0];

		hudEditorColorSliderR.data[1] = button->data[0];
		hudEditorColorSliderG.data[1] = button->data[0];
		hudEditorColorSliderB.data[1] = button->data[0];
		hudEditorColorSliderA.data[1] = button->data[0];
	}

	hudEditorVisible.data[1] = button->data[0];
	hudEditorVisible.data[2] = comp->visible;

	CG_HudEditor_UpdateCheckboxStyle(comp->style);
	CG_HudEditor_UpdateCheckboxBarStyle(comp->barStyle);

	hudEditorShowBackground.data[1] = button->data[0];
	hudEditorShowBackground.data[2] = comp->showBackGround;

	hudEditorShowBorder.data[1] = button->data[0];
	hudEditorShowBorder.data[2] = comp->showBorder;

	hudEditorAutoAdjust.data[1] = button->data[0];
	hudEditorAutoAdjust.data[2] = comp->autoAdjust;

	hudEditorFontStyle.data[1] = button->data[0];
	hudEditorFontStyle.data[2] = comp->styleText;

	hudEditorFontAlign.data[1] = button->data[0];
	hudEditorFontAlign.data[2] = comp->alignText;
}

/**
* @brief CG_HudEditor_Render draw borders for hud elements
* @param[in] button
*/
static void CG_HudEditor_Render(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[0]].offset);
	vec4_t         *color;

	button->rect = comp->location;

	if (button == lastFocusComponent)
	{
		color = &colorYellow;
	}
	else if (showLayout)
	{
		if (showLayout == HUD_SHOW_LAYOUT_VISIBLE_ONLY && !comp->visible)
		{
			return;
		}

		color = comp->visible ? &colorMdGreen : &colorMdRed;
	}
	else if (BG_CursorInRect(&button->rect) && !lastFocusComponentMoved)
	{
		if (!comp->visible)
		{
			return;
		}

		color = &colorMdGreen;
	}
	else
	{
		return;
	}

	CG_DrawRect_FixedBorder(button->rect.x - 1, button->rect.y - 1, button->rect.w + 2, button->rect.h + 2, 2, *color);
}

/**
* @brief CG_HudEditor_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[0]].offset);

		if (lastFocusComponent && BG_CursorInRect(&lastFocusComponent->rect))
		{
			CG_HudEditorUpdateFields(lastFocusComponent);
			lastFocusComponent->data[7] = 0;

			return qtrue;
		}
		else if (comp->visible || showLayout == HUD_SHOW_LAYOUT_ALL)
		{
			CG_HudEditorUpdateFields(button);
			BG_PanelButtons_SetFocusButton(button);
			button->data[7] = 0;

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[0]].offset);

		if (lastFocusComponent && lastFocusComponentMoved)
		{
			lastFocusComponentMoved     = qfalse;
			lastFocusComponent->data[7] = 1;

			return qtrue;
		}
		else if (comp->visible || showLayout == HUD_SHOW_LAYOUT_ALL)
		{
			lastFocusComponent = button;

			// ensure the newly selected comp use a valide tab option display
			if (!CG_HUDEditorCheckTabDisplayValidity(lastButtonTabSelected))
			{
				lastButtonTabSelected = &hudEditorColorButton;
			}

			CG_HudEditorUpdateFields(lastFocusComponent);
			BG_PanelButtons_SetFocusButton(NULL);
			button->data[7] = 1;

			return qtrue;
		}
	}

	return qfalse;
}

static panel_button_t *hudComponentsPanel[HUD_COMPONENTS_NUM + 1];
static panel_button_t hudComponents[HUD_COMPONENTS_NUM];

/**
* @brief CG_HudEditorColor_Finish colors
* @param button
*/
static void CG_HudEditorColor_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	switch (elementColorSelection)
	{
	case HUD_COLOR_SELECTION_MAIN:       comp->colorMain[button->data[3]]       = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
	case HUD_COLOR_SELECTION_SECONDARY:  comp->colorSecondary[button->data[3]]  = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
	case HUD_COLOR_SELECTION_BACKGROUND: comp->colorBackground[button->data[3]] = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
	case HUD_COLOR_SELECTION_BORDER:     comp->colorBorder[button->data[3]]     = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
	default: break;
	}

	if (lastFocusComponent)
	{
		CG_HudEditorUpdateFields(lastFocusComponent);
	}

	BG_PanelButtons_SetFocusButton(NULL);
}

static qboolean CG_HudEditorColor_KeyDown(panel_button_t *button, int key)
{
	// don't modify default HUD
	if (hudData.active->isEditable && key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);

		return qtrue;
	}

	return qfalse;
}

static void CG_HudEditorColor_Render(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	vec4_t         backG = { 1, 1, 1, 0.3f };
	vec4_t         *color;
	float          offset;

	// update color continuously
	if (lastFocusComponent && BG_PanelButtons_GetFocusButton() == button)
	{
		offset = Com_Clamp(0, 1.0f, (cgs.cursorX - button->rect.x) / button->rect.w);

		switch (elementColorSelection)
		{
		case HUD_COLOR_SELECTION_MAIN:       comp->colorMain[button->data[3]]       = offset; break;
		case HUD_COLOR_SELECTION_SECONDARY:  comp->colorSecondary[button->data[3]]  = offset; break;
		case HUD_COLOR_SELECTION_BACKGROUND: comp->colorBackground[button->data[3]] = offset; break;
		case HUD_COLOR_SELECTION_BORDER:     comp->colorBorder[button->data[3]]     = offset; break;
		default: return;
		}

		CG_HudEditorUpdateFields(lastFocusComponent);
	}
	else
	{
		switch (elementColorSelection)
		{
		case HUD_COLOR_SELECTION_MAIN:       offset = comp->colorMain[button->data[3]]      ; break;
		case HUD_COLOR_SELECTION_SECONDARY:  offset = comp->colorSecondary[button->data[3]] ; break;
		case HUD_COLOR_SELECTION_BACKGROUND: offset = comp->colorBackground[button->data[3]]; break;
		case HUD_COLOR_SELECTION_BORDER:     offset = comp->colorBorder[button->data[3]]    ; break;
		default: return;
		}
	}

	switch (button->data[3])
	{
	case 0: color = &colorRed; break;
	case 1: color = &colorGreen; break;
	case 2: color = &colorBlue; break;
	case 3: color = &colorWhite; break;
	default: return;
	}

	button->rect.x = HUDEditorCenterX - HUDEditorWidth * 0.5f + HUDEDITOR_CONTROLS_SPACER_XY * 2;

	CG_FilledBar(button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorBlack, *color, backG, backG, offset, 0.f, BAR_BORDER | BAR_LERP_COLOR, -1);
}

static void CG_HudEditor_Slider_Render(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[1]].offset);
	vec4_t         backG = { 1, 1, 1, 0.3f };
	vec4_t         sliderColor;
	float          offset;
	float          *optionValue;

	// get the correct option to modify depending on slider
	if (button == &hudEditorFontScaleSlider)
	{
		optionValue = &comp->scale;
	}
	else if (button == &hudEditorCircleSliderD)
	{
		optionValue = &comp->circleDensityPoint;
	}
	else if (button == &hudEditorCircleSliderStarAngle)
	{
		optionValue = &comp->circleStartAngle;
	}
	else if (button == &hudEditorCircleSliderEndAngle)
	{
		optionValue = &comp->circleEndAngle;
	}
	else if (button == &hudEditorCircleSliderThickness)
	{
		optionValue = &comp->circleThickness;
	}
	else if (button == &hudEditorFeedTimeSlider)
	{
		optionValue = &comp->feedTime;
	}
	else if (button == &hudEditorFeedStayTimeSlider)
	{
		optionValue = &comp->feedStayTime;
	}
	else if (button == &hudEditorFeedFadeTimeSlider)
	{
		optionValue = &comp->feedFadeTime;
	}
	else
	{
		return;
	}

	Vector4Copy(button->font->colour, sliderColor);

	// update value continuously
	if (lastFocusComponent && BG_PanelButtons_GetFocusButton() == button)
	{
		offset       = Com_Clamp(button->data[3], button->data[4], (cgs.cursorX - button->rect.x) / button->rect.w);
		*optionValue = offset * button->data[5];
		CG_HudEditorUpdateFields(lastFocusComponent);
	}
	else
	{
		offset = *optionValue / button->data[5];
	}

	button->rect.x = HUDEditorCenterX - HUDEditorWidth * 0.5f + HUDEDITOR_CONTROLS_SPACER_XY * 2;

	CG_FilledBar(button->rect.x, button->rect.y, button->rect.w, button->rect.h, sliderColor, sliderColor, backG, backG, offset, 0.f, BAR_BORDER, -1);
}

static int QDECL CG_SortComponentByName(const void *a, const void *b)
{
	const panel_button_t *ca = *(const panel_button_t **)a;
	const panel_button_t *cb = *(const panel_button_t **)b;
	char                 cleanNameA[32];
	char                 cleanNameB[32];

	Q_strncpyz(cleanNameA, hudComponentFields[ca->data[0]].name, sizeof(cleanNameA));
	Q_strncpyz(cleanNameB, hudComponentFields[cb->data[0]].name, sizeof(cleanNameB));
	Q_CleanStr(cleanNameA);
	Q_CleanStr(cleanNameB);

	return Q_stricmpn(cleanNameA, cleanNameB, 32);
}

/**
 * @brief CG_HudEditorAdjustWidthSpread
 * @param[in] panel
 */
void ID_INLINE CG_HudEditorAdjustWidthSpread(panel_button_t *panel[])
{
	int i;

	for (i = 0; panel[i]; i++)
	{
		// FIXME: temporary, remove if statement once all elements are repositioned
		if (!panel[i]->rect.x)
		{
			panel[i]->rect.x = HUDEditorX;
		}
		panel[i]->rect.w = Ccg_WideX(panel[i]->rect.w);
	}
}

/**
* @brief CG_HudEditorSetup
*/
void CG_HudEditorSetup(void)
{
	int i, j;

	// setup some useful coordinates for the side panel
	HUDEditorX       = SCREEN_WIDTH_SAFE;
	HUDEditorWidth   = (HUDEditorX * HUD_EDITOR_SIZE_COEFF) - HUDEditorX;
	HUDEditorCenterX = HUDEditorX + (HUDEditorWidth * 0.5f);

	for (i = 0, j = 0; hudComponentFields[i].name; i++, j++)
	{
		hudComponent_t *comp;

		if (hudComponentFields[i].isAlias)
		{
			j--;
			continue;
		}

		comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[i].offset);

		hudComponents[j].text      = hudComponentFields[i].name;
		hudComponents[j].rect      = comp->location;
		hudComponents[j].onKeyDown = CG_HudEditor_KeyDown;
		hudComponents[j].onKeyUp   = CG_HudEditor_KeyUp;
		hudComponents[j].onDraw    = CG_HudEditor_Render;
		hudComponents[j].data[0]   = i; // link button to hud component

		hudComponentsPanel[j] = &hudComponents[j];
	}

	if (!wsAdjusted)
	{
		// Set up the drawing of HUD editor controls to the right side panel
		CG_HudEditorAdjustWidthSpread(hudEditorGenericTab);
		CG_HudEditorAdjustWidthSpread(hudEditor);
		CG_HudEditorAdjustWidthSpread(hudEditorFontTab);
		CG_HudEditorAdjustWidthSpread(hudEditorColorTab);
		CG_HudEditorAdjustWidthSpread(hudEditorBarTab);
		CG_HudEditorAdjustWidthSpread(hudEditorFeedTab);
		wsAdjusted = qtrue;
	}

	qsort(hudComponentsPanel, HUD_COMPONENTS_NUM, sizeof(panel_button_t *), CG_SortComponentByName);

	// last element needs to be NULL
	hudComponentsPanel[HUD_COMPONENTS_NUM] = NULL;

	// clear last selected button
	lastFocusComponent = NULL;

	// set button default selection
	lastButtonTabSelected   = &hudEditorColorButton;
	lastButtonColorSelected = &hudEditorColorSelectionMain;

	// clear style box
	Com_Memset(styleCheckBox, 0, sizeof(styleCheckBox));
	Com_Memset(styleCheckBoxPanel, 0, sizeof(styleCheckBoxPanel));

	// clear bar style box
	Com_Memset(barStyleCheckBox, 0, sizeof(barStyleCheckBox));
	Com_Memset(barStyleCheckBoxPanel, 0, sizeof(barStyleCheckBoxPanel));

	elementColorSelection = 0;
}

#define COMPONENT_BUTTON_HEIGHT 12
#define COMPONENT_BUTTON_SPACE_X 2
#define COMPONENT_BUTTON_SPACE_Y 2
#define COMPONENT_BUTTON_COLUMN_NUM 7

/**
 * @brief CG_DrawHudEditor_ComponentLists
 */
static void CG_DrawHudEditor_ComponentLists(panel_button_t *button)
{
	float x = button->rect.x;
	float y = button->rect.y;
	float w = (SCREEN_WIDTH_SAFE - COMPONENT_BUTTON_SPACE_X * COMPONENT_BUTTON_COLUMN_NUM)
	          / COMPONENT_BUTTON_COLUMN_NUM;
	int            offsetX, offsetY;
	panel_button_t **buttons = hudComponentsPanel;
	panel_button_t *parsedButton;
	hudComponent_t *comp;

	for ( ; *buttons; buttons++)
	{
		float scalex = Ccg_WideX(button->font->scalex);
		parsedButton = (*buttons);

		comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[parsedButton->data[0]].offset);

		CG_FillRect(x, y, w, COMPONENT_BUTTON_HEIGHT, lastFocusComponent == parsedButton ? (vec4_t) { 1, 1, 0, 0.4f } : (vec4_t) { 0.3f, 0.3f, 0.3f, 0.4f });

		if (BG_CursorInRect(&(rectDef_t) { x, y, w, COMPONENT_BUTTON_HEIGHT }))
		{
			CG_FillRect(x, y, w, COMPONENT_BUTTON_HEIGHT, (vec4_t) { 1, 1, 1, 0.4f });

			if (parsedButton != lastFocusComponent)
			{
				CG_DrawRect_FixedBorder(parsedButton->rect.x - 1, parsedButton->rect.y - 1, parsedButton->rect.w + 2, parsedButton->rect.h + 2, 2, comp->visible ? colorMdGreen : colorMdRed);
			}
		}

		offsetX = CG_Text_Width_Ext(parsedButton->text, scalex, 0, button->font->font);
		offsetY = CG_Text_Height_Ext(parsedButton->text, button->font->scaley, 0, button->font->font);

		if (offsetX >= w)
		{
			scalex  = w / CG_Text_Width_Ext(parsedButton->text, 1, 0, button->font->font) - 0.02f;
			offsetX = CG_Text_Width_Ext(parsedButton->text, scalex, 0, button->font->font);
		}

		CG_Text_Paint_Ext(x + (w - offsetX) * 0.5f, y + ((COMPONENT_BUTTON_HEIGHT + offsetY) * 0.5f), scalex, button->font->scaley,
		                  comp->visible ? colorMdGreen : colorMdRed, parsedButton->text, 0, 0, button->font->style, button->font->font);

		y += COMPONENT_BUTTON_HEIGHT + COMPONENT_BUTTON_SPACE_Y;

		if (y + COMPONENT_BUTTON_HEIGHT >= button->rect.y + button->rect.h)
		{
			y  = button->rect.y;
			x += w + COMPONENT_BUTTON_SPACE_X;
		}
	}
}

/**
* @brief CG_HudEditor_ComponentLists_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_ComponentLists_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_ComponentLists_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_ComponentLists_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t      rect;
			panel_button_t **buttons = hudComponentsPanel;
			panel_button_t *parsedButton;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			rect.w = (SCREEN_WIDTH_SAFE - COMPONENT_BUTTON_SPACE_X * COMPONENT_BUTTON_COLUMN_NUM)
			         / COMPONENT_BUTTON_COLUMN_NUM;
			rect.h = COMPONENT_BUTTON_HEIGHT;

			for ( ; *buttons; buttons++)
			{
				parsedButton = (*buttons);

				if (BG_CursorInRect(&rect))
				{
					SOUND_SELECT;
					lastFocusComponent          = parsedButton;
					lastFocusComponentMoved     = qfalse;
					lastFocusComponent->data[7] = 1;
					CG_HudEditorUpdateFields(parsedButton);

					// ensure the newly selected comp use a valide tab option display
					if (!CG_HUDEditorCheckTabDisplayValidity(lastButtonTabSelected))
					{
						lastButtonTabSelected = &hudEditorColorButton;
					}

					break;
				}

				rect.y += rect.h + COMPONENT_BUTTON_SPACE_Y;

				if (rect.y + COMPONENT_BUTTON_HEIGHT >= button->rect.y + button->rect.h)
				{
					rect.y  = button->rect.y;
					rect.x += rect.w + COMPONENT_BUTTON_SPACE_X;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_DrawHudEditor_ToolTip
 * @param[in] name
 */
static void CG_DrawHudEditor_ToolTip(panel_button_t *button)
{
	int offsetX = CG_Text_Width_Ext(button->text, 0.20f, 0, &cgs.media.limboFont1);

	if (cgDC.cursorx + 10 + offsetX >= 640)
	{
		CG_Text_Paint_Ext(cgDC.cursorx - 10 - offsetX, cgDC.cursory, 0.20f, 0.22f, colorGreen, button->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else
	{
		CG_Text_Paint_Ext(cgDC.cursorx + 10, cgDC.cursory, 0.20f, 0.22f, colorGreen, button->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/**
 * @var helpStatus
 * @details
 */
static int helpStatus = SHOW_OFF;

/**
 * @brief CG_HudEditor_ToggleShowLayout
 */
static void CG_HudEditor_ToggleShowLayout(void)
{
	switch (showLayout)
	{
	case HUD_SHOW_LAYOUT_OFF:          showLayout = HUD_SHOW_LAYOUT_VISIBLE_ONLY; break;
	case HUD_SHOW_LAYOUT_VISIBLE_ONLY: showLayout = HUD_SHOW_LAYOUT_ALL; break;
	case HUD_SHOW_LAYOUT_ALL:          showLayout = HUD_SHOW_LAYOUT_OFF; break;
	default: break;
	}
}

/**
 * @brief CG_HudEditor_ToggleHelp
 */
static void CG_HudEditor_ToggleHelp(void)
{
	if (helpStatus != SHOW_ON)
	{
		CG_ShowHelp_On(&helpStatus);
	}
	else if (helpStatus == SHOW_ON)
	{
		CG_ShowHelp_Off(&helpStatus);
	}
}

/**
 * @brief CG_HudEditor_ToggleNoiseGenerator
 */
static void CG_HudEditor_ToggleNoiseGenerator(void)
{
	cg.generatingNoiseHud = !cg.generatingNoiseHud;
	CG_HudEditor_Cleanup();
}

/**
 * @brief CG_HudEditor_ToggleFullScreen
 */
static void CG_HudEditor_ToggleFullScreen(void)
{
	cg.fullScreenHudEditor = !cg.fullScreenHudEditor;
}

/**
 * @brief CG_HudEditor_HelpDraw
 */
static void CG_HudEditor_HelpDraw(void)
{
	if (helpStatus != SHOW_OFF)
	{
		static const helpType_t help[] =
		{
			{ "K_DOWN",              "move down by 1px"                       },
			{ "K_LEFT",              "move left by 1px"                       },
			{ "K_UP",                "move down by 1px"                       },
			{ "K_RIGHT",             "move right by 1px"                      },
			{ NULL,                  NULL                                     },
			{ "K_MWHEELDOWN",        "enlarge by 1px"                         },
			{ "K_MWHEELUP",          "shrink by 1px"                          },
			{ NULL,                  NULL                                     },
			{ "K_RCTRL / K_LCTRL",   "hold to move by 0.1px"                  },
			{ "K_RSHIFT / K_LSHIFT", "hold to move by 5px"                    },
			{ NULL,                  NULL                                     },
#ifdef __APPLE__
			{ "K_COMMAND",           "hold to key resize / mwheel scale"      },
#else
			{ "K_RALT / K_LALT",     "hold to key resize / mwheel scale"      },
#endif
			{ NULL,                  NULL                                     },
			{ "K_INS",               "move to center"                         },
			{ "K_PGUP",              "move from bottom -> middle -> top"      },
			{ "K_PGDN",              "move from top -> middle -> bottom"      },
			{ "K_HOME",              "move from left -> middle -> right"      },
			{ "K_END",               "move from right -> middle -> left"      },
			{ NULL,                  NULL                                     },
			{ "s",                   "square a comp bound using longest side" },
			{ "r",                   "rectangle a comp bound"                 },
			{ "m",                   "mirror the width and height comp"       },
			{ NULL,                  NULL                                     },
			{ "l",                   "show layout visible -> all -> off"      },
			{ "h",                   "help on/off"                            },
			{ "n",                   "noise generator on/off"                 },
			{ "f",                   "full screen on/off"                     },
			{ "a",                   "force grid alignment on/off"            },
			{ "t",                   "toggle showing only active component"   },
			{ "v",                   "toggle visibility of active component"  },
			{ "SHIFT + CTRL + v",    "set all components visible"             },
			{ NULL,                  NULL                                     },
			{ "o",                   "show micro grid on/off"                 },
			{ "c",                   "show grid OCD lvl 1/2/3"                },
			{ "d",                   "scale grid .25/.125/.1"                 },
		};

		vec4_t bgColor;

		VectorCopy(colorDkGrey, bgColor);
		bgColor[3] = .90f;

		CG_DrawHelpWindow(Ccg_WideX(SCREEN_WIDTH) * 0.2f, SCREEN_HEIGHT * 0.15f, &helpStatus, "HUD EDITOR CONTROLS", help, sizeof(help) / sizeof(helpType_t),
		                  bgColor, colorBlack, colorDkGrey, colorBlack,
		                  &hudEditorHeaderFont, &hudEditorTextFont);
	}
}

static qboolean CG_HudEditorHelpButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		CG_HudEditor_ToggleHelp();

		return qtrue;
	}

	return qfalse;
}

static void CG_HudEditor_IncreaseSize(hudComponent_t *comp, float offset, qboolean changeSize)
{
	if (!changeSize)   // increase component
	{
		comp->location.x -= offset * .5f;
		comp->location.y -= offset * .5f;
		comp->location.w += offset;
		comp->location.h += offset;
	}
	else     // increase text size
	{
		comp->scale += offset;
	}
}

static void CG_HudEditor_DecreaseSize(hudComponent_t *comp, float offset, qboolean changeSize)
{
	if (!changeSize)   // decrease component
	{
		comp->location.x += offset * .5f;
		comp->location.y += offset * .5f;
		comp->location.w -= offset;
		comp->location.h -= offset;
	}
	else     // decrease text size
	{
		comp->scale -= offset;
	}
}

static void CG_HudEditor_ToggleFilterActiveComponent()
{
	if (lastFocusComponent != NULL)
	{
		hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[lastFocusComponent->data[0]].offset);
		showOnlyHudComponent = (showOnlyHudComponent == NULL) ? comp : NULL;
	}
}

static void CG_HudEditor_ToggleVisibility(void)
{
	hudComponent_t *comp;
	// holding CTRL & SHIFT makes all component visible
	if ((trap_Key_IsDown(K_RCTRL) || trap_Key_IsDown(K_LCTRL))
	    && (trap_Key_IsDown(K_RSHIFT) || trap_Key_IsDown(K_LSHIFT)))
	{
		int i;
		for (i = 0; i < HUD_COMPONENTS_NUM; i++)
		{
			comp          = hudData.active->components[i];
			comp->visible = qtrue;
		}
	}
	// otherwise toggle visibility of focused component
	else
	{
		if (lastFocusComponent != NULL)
		{
			comp          = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[lastFocusComponent->data[0]].offset);
			comp->visible = comp->visible ? qfalse : qtrue;
		}
	}

}

/**
 * @brief CG_HudEditor_ToggleForceGridAlignment
 */
static void CG_HudEditor_ToggleForceGridAlignment(void)
{
	forceGridAlignment = !forceGridAlignment;
}

/**
 * @brief CG_HudEditor_ToggleGrid
 */
static void CG_HudEditor_ToggleGrid(void)
{
	++showGrid;

	if (showGrid == HUD_SHOW_GRID_MAX)
	{
		showGrid = HUD_SHOW_GRID_OFF;
	}
}

/**
 * @brief CG_HudEditor_ToggleFullScreen
 */
static void CG_HudEditor_ToggleMicroGrid(void)
{
	showMicroGrid = !showMicroGrid;
}

/**
 * @brief CG_HudEditor_ToggleGridScale
 */
static void CG_HudEditor_ToggleGridScale(void)
{
	++gridScale;

	if (gridScale == HUD_GRID_SCALE_MAX)
	{
		gridScale = HUD_GRID_SCALE_1;
	}
}

/**
 * @brief CG_HudEditor_GetGridScale
 * @return
 */
static ID_INLINE float CG_HudEditor_GetGridScale(void)
{
	switch (gridScale)
	{
	case HUD_GRID_SCALE_1: return 0.25f;
	case HUD_GRID_SCALE_2: return 0.125f;
	case HUD_GRID_SCALE_3: return 0.10f;
	default: return 0.10f;
	}
}

/**
 * @brief CG_HUDEditor_GridDrawSection2
 * @param[in] step
 * @param[in] size
 */
static void CG_HUDEditor_GridDrawSection2(float step, float size)
{
	float i, j;

	for (i = SCREEN_WIDTH_SAFE * step, j = SCREEN_HEIGHT_SAFE * step;
	     i < SCREEN_WIDTH_SAFE && j < SCREEN_HEIGHT_SAFE;
	     i += SCREEN_WIDTH_SAFE * step, j += SCREEN_HEIGHT_SAFE * step)
	{
		CG_DrawRect_FixedBorder(i, j, SCREEN_WIDTH_SAFE - i * 2, SCREEN_HEIGHT_SAFE - j * 2, size, (vec4_t) { 1, 1, 1, .5f });
	}
}

/**
 * @brief CG_HUDEditor_GridDrawSection
 * @param[in] step
 * @param[in] size
 */
static void CG_HUDEditor_GridDrawSection(float step, float size)
{
	float i = SCREEN_WIDTH_SAFE * step;
	float j = SCREEN_HEIGHT_SAFE * step;

	while (i < SCREEN_WIDTH_SAFE && j < SCREEN_HEIGHT_SAFE)
	{
		CG_FillRect(i, 0, size, SCREEN_HEIGHT_SAFE, (vec4_t) { 1, 1, 1, .5f });
		CG_FillRect(0, j, SCREEN_WIDTH_SAFE, size, (vec4_t) { 1, 1, 1, .5f });

		i += SCREEN_WIDTH_SAFE * step;
		j += SCREEN_HEIGHT_SAFE * step;
	}
}

/**
 * @brief CG_HudEditor_GridDraw
 */
static void CG_HudEditor_GridDraw(void)
{
	float step = CG_HudEditor_GetGridScale();

	if (showMicroGrid)
	{
		CG_HUDEditor_GridDrawSection(step / (1 / step), 0.1f);
	}

	switch (showGrid)
	{
	case HUD_SHOW_GRID_OCD_LEVEL3: CG_HUDEditor_GridDrawSection2(step, 1.25f);
	// fall through
	case HUD_SHOW_GRID_OCD_LEVEL2: CG_HUDEditor_GridDrawSection(step * 0.5f, 0.25f);
	// fall through
	case HUD_SHOW_GRID_OCD_LEVEL1: CG_HUDEditor_GridDrawSection(step, 0.5f);
	// fall through
	case HUD_SHOW_GRID_OFF:
	default: return;
	}
}

static panel_button_t ** CG_HudEditor_GetSelectedTab()
{
	if (lastButtonTabSelected == &hudEditorFontButton)
	{
		return hudEditorFontTab;
	}
	else if (lastButtonTabSelected == &hudEditorStyleButton)
	{
		return styleCheckBoxPanel;
	}
	else if (lastButtonTabSelected == &hudEditorBarButton)
	{
		return hudEditorBarTab;
	}
	else if (lastButtonTabSelected == &hudEditorFeedButton)
	{
		return hudEditorFeedTab;
	}
	else // default
	{
		return hudEditorColorTab;
	}
}

/**
* @brief CG_DrawHudEditor
*/
void CG_DrawHudEditor(void)
{
	static int altHud = -1;
	qboolean   skip;

	panel_button_t **buttons = hudComponentsPanel;
	panel_button_t *button;
	hudComponent_t *comp;

	if (altHud != hudData.active->hudnumber)
	{
		trap_Cvar_Set(hudEditorHudName.text, hudData.active->name);
		altHud = hudData.active->hudnumber;
	}

	CG_HudEditor_GridDraw();
	BG_PanelButtonsRender(hudComponentsPanel);

	// don't display customization option if no comp selected
	if (lastFocusComponent)
	{
		BG_PanelButtonsRender(hudEditorGenericTab);
		BG_PanelButtonsRender(CG_HudEditor_GetSelectedTab());

		// FIXME: find a more elegant way to draw it
		if (lastButtonTabSelected == &hudEditorBarButton)
		{
			BG_PanelButtonsRender(barStyleCheckBoxPanel);
		}
	}

	BG_PanelButtonsRender(hudEditor);
	CG_HudEditor_HelpDraw();

	trap_R_SetColor(NULL);
	CG_DrawCursor(cgDC.cursorx, cgDC.cursory);

	// start parsing hud components from the last focused button
	skip = qtrue;
	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[0]].offset);

		if (skip)
		{
			if (button != lastFocusComponent)
			{
				continue;
			}

			skip = qfalse;
		}

		if ((comp->visible || showLayout == HUD_SHOW_LAYOUT_ALL) && BG_CursorInRect(&button->rect))
		{
			CG_DrawHudEditor_ToolTip(button);
			return;
		}
	}

	// start from beginning
	buttons = hudComponentsPanel;

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[0]].offset);

		// early return
		if (lastFocusComponent && lastFocusComponent == button)
		{
			break;
		}

		if ((comp->visible || showLayout == HUD_SHOW_LAYOUT_ALL) && BG_CursorInRect(&button->rect))
		{
			CG_DrawHudEditor_ToolTip(button);
			break;
		}
	}
}

void CG_HudEditorReset()
{
	if (cg.generatingNoiseHud)
	{
		CG_HudEditor_Cleanup();
		cg.generatingNoiseHud = qfalse;
	}

	cg.editingHud          = qfalse;
	cg.fullScreenHudEditor = qfalse;
}

/**
* @brief CG_HudEditor_KeyHandling
* @param[in] key
* @param[in] down
*/
void CG_HudEditor_KeyHandling(int key, qboolean down)
{
	// close hud editor menu if any 'edithud' key binding is pressed again
	if (down)
	{
		int b1, b2;
		cgDC.getKeysForBinding("edithud", &b1, &b2);
		if ((b1 != -1 && b1 == key) || (b2 != -1 && b2 == key))
		{
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
			return;
		}
	}

	if (BG_PanelButtonsKeyEvent(key, down, hudEditor))
	{
		return;
	}

	// left mouse up
	if (key == K_MOUSE1 && !down)
	{
		// was moving a comp and up click outside a comp
		if (lastFocusComponent && lastFocusComponentMoved)
		{
			lastFocusComponentMoved     = qfalse;
			lastFocusComponent->data[7] = 1;

			return;
		}
	}

	if (key == K_MOUSE2)
	{
		lastFocusComponent    = NULL;
		lastButtonTabSelected = &hudEditorColorButton;
		return;
	}

	if (down)
	{
		switch (key)
		{
		case 'l': CG_HudEditor_ToggleShowLayout();         return;
		case 'h': CG_HudEditor_ToggleHelp();               return;
		case 'n': CG_HudEditor_ToggleNoiseGenerator();     return;
		case 'f': CG_HudEditor_ToggleFullScreen();         return;
		case 'o': CG_HudEditor_ToggleMicroGrid();          return;
		case 'c': CG_HudEditor_ToggleGrid();               return;
		case 'd': CG_HudEditor_ToggleGridScale();          return;
		case 'a': CG_HudEditor_ToggleForceGridAlignment(); return;
		case 't': CG_HudEditor_ToggleFilterActiveComponent(); return;
		case 'v': CG_HudEditor_ToggleVisibility(); return;
		default: break;
		}
	}

	// start parsing hud components from the last focused button
	if (lastFocusComponent)
	{
		panel_button_t **buttons = hudComponentsPanel;
		panel_button_t *button;

		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (button == lastFocusComponent)
			{
				if (BG_PanelButtonsKeyEvent(key, down, ++buttons))
				{
					return;
				}
				break;
			}
		}
	}

	if (BG_PanelButtonsKeyEvent(key, down, hudComponentsPanel))
	{
		return;
	}

	if (lastButtonTabSelected)
	{
		if (BG_PanelButtonsKeyEvent(key, down, hudEditorGenericTab))
		{
			return;
		}

		if (BG_PanelButtonsKeyEvent(key, down, CG_HudEditor_GetSelectedTab()))
		{
			return;
		}

		// FIXME: find a more elegant way to draw it
		if (lastButtonTabSelected == &hudEditorBarButton)
		{
			if (BG_PanelButtonsKeyEvent(key, down, barStyleCheckBoxPanel))
			{
				return;
			}
		}
	}

	// don't modify default HUD
	if (hudData.active->isEditable && lastFocusComponent && down)
	{
		hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[lastFocusComponent->data[0]].offset);
		qboolean       changeSize;
		qboolean       gridAlign = forceGridAlignment && (showGrid || showMicroGrid);
		float          offset;
		float          *pValue = NULL;

		// change the component shape to be a square by using the longest side
		if (key == 's')
		{
			comp->location.w = comp->location.h = MAX(comp->location.w, comp->location.h);
			return;
		}
		else if (key == 'r')    // change the component shape to be a rectable by using the 20% of the longest side
		{
			if (comp->location.w > comp->location.h)
			{
				comp->location.h = comp->location.w * 0.20f;
			}
			else
			{
				comp->location.w = comp->location.h * 0.20f;
			}

			return;
		}
		else if (key == 'm')    // mirror the width and the heigh
		{
			float tmp = comp->location.w;
			comp->location.w = comp->location.h;
			comp->location.h = tmp;
			return;
		}

#ifdef __APPLE__
		changeSize = trap_Key_IsDown(K_COMMAND);
#else
		changeSize = (trap_Key_IsDown(K_RALT) || trap_Key_IsDown(K_LALT));
#endif
		if (gridAlign)
		{
			float multiple = CG_HudEditor_GetGridScale();
			float side;

			if (key == K_LEFTARROW || key == K_RIGHTARROW)
			{
				// use 4:3 aspect ratio in case the size is changed
				side = changeSize ? SCREEN_HEIGHT_SAFE : SCREEN_WIDTH_SAFE;
			}
			else
			{
				side = SCREEN_HEIGHT_SAFE;
			}

			offset = side * (multiple / (1 / multiple));
		}
		else if (trap_Key_IsDown(K_RCTRL) || trap_Key_IsDown(K_LCTRL))
		{
			offset = 0.1f;
		}
		else if (trap_Key_IsDown(K_RSHIFT) || trap_Key_IsDown(K_LSHIFT))
		{
			offset = 5;
		}
		else
		{
			offset = 1;
		}

		if (trap_Key_IsDown(K_LEFTARROW))
		{
			pValue   = (changeSize ? &comp->location.w : &comp->location.x);
			*pValue -= offset;
		}

		if (trap_Key_IsDown(K_RIGHTARROW))
		{
			pValue   = (changeSize ? &comp->location.w : &comp->location.x);
			*pValue += offset;
		}

		if (trap_Key_IsDown(K_UPARROW))
		{
			pValue   = (changeSize ? &comp->location.h : &comp->location.y);
			*pValue -= offset;
		}

		if (trap_Key_IsDown(K_DOWNARROW))
		{
			pValue   = (changeSize ? &comp->location.h : &comp->location.y);
			*pValue += offset;
		}

		if (pValue && gridAlign)
		{
			*pValue = Q_ClosestMultipleFloat(*pValue, offset, 3);
		}

		switch (key)
		{
		case K_LEFTARROW:
		case K_RIGHTARROW:
		case K_UPARROW:
		case K_DOWNARROW: break;    // do nothing so it reach update function
		case K_PGUP:       comp->location.y = ((comp->location.y <= (SCREEN_HEIGHT - comp->location.h) * .5f) ?
			                                   0 : (SCREEN_HEIGHT - comp->location.h) * .5f); break;
		case K_PGDN:       comp->location.y = ((comp->location.y < (SCREEN_HEIGHT - comp->location.h) * .5f) ?
			                                   (SCREEN_HEIGHT - comp->location.h) * .5f : SCREEN_HEIGHT - comp->location.h); break;
		case K_HOME:       comp->location.x = (((int)comp->location.x <= (int)((Ccg_WideX(SCREEN_WIDTH) - comp->location.w) * .5f)) ?
			                                   0 : (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) * .5f); break;
		case K_END:        comp->location.x = ((comp->location.x < (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) * .5f) ?
			                                   (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) * .5f: Ccg_WideX(SCREEN_WIDTH) - comp->location.w); break;
		case K_INS:        comp->location.x = (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) * .5f; comp->location.y = (SCREEN_HEIGHT - comp->location.h) * .5f; break;
		case K_MWHEELDOWN: CG_HudEditor_DecreaseSize(comp, offset, changeSize); break;
		case K_MWHEELUP: CG_HudEditor_IncreaseSize(comp, offset, changeSize); break;
		default: return;
		}

		CG_HudEditorUpdateFields(lastFocusComponent);

		return;
	}
}

/**
* @brief CG_HudEditorMouseMove_Handling
* @param[in] x
* @param[in] y
*/
void CG_HudEditorMouseMove_Handling(int x, int y)
{
	panel_button_t *button = lastFocusComponent;
	static float   offsetX = 0;
	static float   offsetY = 0;

	if (!cg.editingHud)
	{
		return;
	}

	if (button && !button->data[7])
	{
		// we try to move a comp
		lastFocusComponentMoved = qtrue;

		// don't modify default HUD
		if (hudData.active->isEditable)
		{
			hudComponent_t *comp = (hudComponent_t *)((byte *)hudData.active + hudComponentFields[button->data[0]].offset);

			if (!offsetX && !offsetY)
			{
				offsetX = (x - comp->location.x);
				offsetY = (y - comp->location.y);
			}

			comp->location.x = x - offsetX;
			comp->location.y = y - offsetY;

			if (forceGridAlignment && (showGrid || showMicroGrid))
			{
				float multiple = CG_HudEditor_GetGridScale();

				comp->location.x = Q_ClosestMultipleFloat(comp->location.x, SCREEN_WIDTH_SAFE * (multiple / (1 / multiple)), 3);
				comp->location.y = Q_ClosestMultipleFloat(comp->location.y, SCREEN_HEIGHT_SAFE * (multiple / (1 / multiple)), 3);
			}

			CG_HudEditorUpdateFields(button);
			return;
		}
	}

	// reset offset
	offsetX = 0;
	offsetY = 0;
}
