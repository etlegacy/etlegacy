package com.n0n3m4.q3e;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public final class Q3EGlobals
{
    public static final String CONST_PACKAGE_NAME = "com.karin.idTech4Amm";
    public static final String CONST_APP_NAME     = "idTech4A++"; // "DIII4A++";

    // log tag
    public static final String CONST_Q3E_LOG_TAG = "Q3E";

    // on-screen buttons index
    public static final int UI_JOYSTICK     = 0;
    public static final int UI_SHOOT        = 1;
    public static final int UI_JUMP         = 2;
    public static final int UI_CROUCH       = 3;
    public static final int UI_RELOADBAR    = 4;
    public static final int UI_PDA          = 5;
    public static final int UI_FLASHLIGHT   = 6;
    public static final int UI_SAVE         = 7;
    public static final int UI_1            = 8;
    public static final int UI_2            = 9;
    public static final int UI_3            = 10;
    public static final int UI_KBD          = 11;
    public static final int UI_CONSOLE      = 12;
    public static final int UI_RUN          = 13;
    public static final int UI_ZOOM         = 14;
    public static final int UI_INTERACT     = 15;
    public static final int UI_WEAPON_PANEL = 16;
    public static final int UI_SCORE        = 17;
    public static final int UI_0            = 18;
    public static final int UI_4            = 19;
    public static final int UI_5            = 20;
    public static final int UI_6            = 21;
    public static final int UI_7            = 22;
    public static final int UI_8            = 23;
    public static final int UI_9            = 24;
    public static final int UI_SIZE         = UI_9 + 1;
    /*public static final int UI_A            = 25;
    public static final int UI_B            = 26;
    public static final int UI_C            = 27;
    public static final int UI_D            = 28;
    public static final int UI_E            = 29;
    public static final int UI_F            = 30;
    public static final int UI_G            = 31;
    public static final int UI_H            = 32;
    public static final int UI_I            = 33;
    public static final int UI_J            = 34;
    public static final int UI_K            = 35;
    public static final int UI_L            = 36;
    public static final int UI_M            = 37;
    public static final int UI_N            = 38;
    public static final int UI_O            = 39;
    public static final int UI_P            = 40;
    public static final int UI_Q            = 41;
    public static final int UI_R            = 42;
    public static final int UI_S            = 43;
    public static final int UI_T            = 44;
    public static final int UI_U            = 45;
    public static final int UI_V            = 46;
    public static final int UI_W            = 47;
    public static final int UI_X            = 48;
    public static final int UI_Y            = 49;
    public static final int UI_Z            = 50;*/

    // on-screen item type
    public static final int TYPE_BUTTON   = 0;
    public static final int TYPE_SLIDER   = 1;
    public static final int TYPE_JOYSTICK = 2;
    public static final int TYPE_DISC     = 3;
    public static final int TYPE_MOUSE    = -1;

    // mouse
    public static final int MOUSE_EVENT  = 1;
    public static final int MOUSE_DEVICE = 2;

    // default size
    public static final int SCREEN_WIDTH  = 640;
    public static final int SCREEN_HEIGHT = 480;

    // on-screen button type
    public static final int ONSCREEN_BUTTON_TYPE_FULL         = 0;
    public static final int ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM = 1;
    public static final int ONSCREEN_BUTTON_TYPE_CENTER       = 2;
    public static final int ONSCREEN_BUTTON_TYPE_LEFT_TOP     = 3;

    // on-screen button can hold
    public static final int ONSCRREN_BUTTON_NOT_HOLD = 0;
    public static final int ONSCRREN_BUTTON_CAN_HOLD = 1;

    // on-screen slider type
    public static final int ONSCRREN_SLIDER_STYLE_LEFT_RIGHT             = 0;
    public static final int ONSCRREN_SLIDER_STYLE_DOWN_RIGHT             = 1;
    public static final int ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK = 2;
    public static final int ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK = 3;

    // on-screen joystick visible
    public static final int ONSCRREN_JOYSTICK_VISIBLE_ALWAYS       = 0;
    public static final int ONSCRREN_JOYSTICK_VISIBLE_HIDDEN       = 1;
    public static final int ONSCRREN_JOYSTICK_VISIBLE_ONLY_PRESSED = 2;

    // game state
    public static final int STATE_NONE    = 0;
    public static final int STATE_ACT     = 1; // RTCW4A-specific, keep
    public static final int STATE_GAME    = 1 << 1; // map spawned
    public static final int STATE_KICK    = 1 << 2; // RTCW4A-specific, keep
    public static final int STATE_LOADING = 1 << 3; // current GUI is guiLoading
    public static final int STATE_CONSOLE = 1 << 4; // fullscreen or not
    public static final int STATE_MENU    = 1 << 5; // any menu excludes guiLoading
    public static final int STATE_DEMO    = 1 << 6; // demo

    // game view control
    public static final int VIEW_MOTION_CONTROL_TOUCH     = 1;
    public static final int VIEW_MOTION_CONTROL_GYROSCOPE = 1 << 1;
    public static final int VIEW_MOTION_CONTROL_ALL       = VIEW_MOTION_CONTROL_TOUCH | VIEW_MOTION_CONTROL_GYROSCOPE;

    // game engine library
    public static final String LIB_ENGINE_ID        = "libidtech4.so"; // DOOM3
    public static final String LIB_ENGINE_RAVEN     = "libidtech4_raven.so"; // Quake 4
    public static final String LIB_ENGINE_HUMANHEAD = "libidtech4_humanhead.so"; // Prey 2006
    public static final String LIB_ENGINE2_ID       = "libidtech2.so"; // Quake 2
    public static final String LIB_ENGINE3_ID       = "libidtech3.so"; // Quake 3
    public static final String LIB_ENGINE3_RTCW     = "libidtech3_rtcw.so"; // RTCW
    public static final String LIB_ENGINE4_TDM      = "libthedarkmod.so"; // TDM
    public static final String LIB_ENGINE1_QUAKE    = "libidtech_quake.so"; // Quake 1
    public static final String LIB_ENGINE4_D3BFG    = "libRBDoom3BFG.so"; // Doom3-BFG
    public static final String LIB_ENGINE1_DOOM     = "libgzdoom.so"; // GZDOOM
    public static final String LIB_ENGINE3_ETW      = "libidtech3_etw.so"; // ETW

    // game config file
    public static final String CONFIG_FILE_DOOM3    = "DoomConfig.cfg"; // DOOM3
    public static final String CONFIG_FILE_QUAKE4   = "Quake4Config.cfg"; // Quake 4
    public static final String CONFIG_FILE_PREY     = "preyconfig.cfg"; // Prey 2006
    public static final String CONFIG_FILE_QUAKE2   = "config.cfg"; // Quake 2
    public static final String CONFIG_FILE_QUAKE3   = "q3config.cfg"; // Quake 3
    public static final String CONFIG_FILE_RTCW     = "wolfconfig.cfg"; // RTCW
    public static final String CONFIG_FILE_TDM      = "Darkmod.cfg"; // TDM
    public static final String CONFIG_FILE_QUAKE1   = "config.cfg"; // Quake 1
    public static final String CONFIG_FILE_DOOM3BFG = "D3BFGConfig.cfg"; // DOOM3-BFG
    public static final String CONFIG_FILE_GZDOOM   = "gzdoom.ini"; // GZDOOM
    public static final String CONFIG_FILE_ETW      = "etconfig.cfg"; // ETW

    // game type token
    public static final String GAME_DOOM3    = "doom3";
    public static final String GAME_QUAKE4   = "quake4";
    public static final String GAME_PREY     = "prey2006";
    public static final String GAME_QUAKE2   = "quake2";
    public static final String GAME_QUAKE3   = "quake3";
    public static final String GAME_RTCW     = "rtcw";
    public static final String GAME_TDM      = "tdm";
    public static final String GAME_QUAKE1   = "quake1";
    public static final String GAME_DOOM3BFG = "doom3bfg";
    public static final String GAME_GZDOOM   = "gzdoom";
    public static final String GAME_ETW      = "etw";

    // game name
    public static final String GAME_NAME_DOOM3    = "DOOM 3";
    public static final String GAME_NAME_QUAKE4   = "Quake 4";
    public static final String GAME_NAME_PREY     = "Prey(2006)";
    public static final String GAME_NAME_QUAKE2   = "Quake 2";
    public static final String GAME_NAME_QUAKE3   = "Quake 3";
    public static final String GAME_NAME_RTCW     = "RTCW"; // "Return to Castle Wolfenstein";
    public static final String GAME_NAME_TDM      = "Dark mod"; // The Dark Mod
    public static final String GAME_NAME_QUAKE1   = "Quake 1";
    public static final String GAME_NAME_DOOM3BFG = "DOOM 3 BFG";
    public static final String GAME_NAME_GZDOOM   = "GZDOOM";
    public static final String GAME_NAME_ETW      = "ETW"; // "Wolfenstein: Enemy Territory";

    // game base folder
    public static final String GAME_BASE_DOOM3      = "base";
    public static final String GAME_BASE_D3XP       = "d3xp";
    public static final String GAME_BASE_QUAKE4     = "q4base";
    public static final String GAME_BASE_PREY       = "preybase"; // Other platform is `base`
    public static final String GAME_BASE_QUAKE2     = "baseq2";
    public static final String GAME_BASE_QUAKE3     = "baseq3";
    public static final String GAME_BASE_RTCW       = "main";
    public static final String GAME_BASE_TDM        = ""; // the dark mod is standalone
    public static final String GAME_BASE_QUAKE1     = "darkplaces/id1"; // "darkplaces";
    public static final String GAME_BASE_QUAKE1_DIR = "id1";
    public static final String GAME_BASE_DOOM3BFG   = "base"; // RBDoom3BFG always in doom3bfg folder
    public static final String GAME_BASE_GZDOOM     = ""; // GZDOOM is standalone
    public static final String GAME_BASE_ETW        = "etmain";

    // game sub directory
    public static final String GAME_SUBDIR_DOOM3   = "doom3";
    public static final String GAME_SUBDIR_QUAKE4  = "quake4";
    public static final String GAME_SUBDIR_PREY    = "prey";
    public static final String GAME_SUBDIR_QUAKE2  = "quake2";
    public static final String GAME_SUBDIR_QUAKE3  = "quake3";
    public static final String GAME_SUBDIR_RTCW    = "rtcw";
    public static final String GAME_SUBDIR_TDM     = "darkmod";
    public static final String GAME_SUBDIR_QUAKE1  = "quake1";
    public static final String GAME_SUBDIR_DOOMBFG = "doom3bfg";
    public static final String GAME_SUBDIR_GZDOOM  = "gzdoom";
    public static final String GAME_SUBDIR_ETW     = "etw";

/*
    public static final String[] QUAKE4_MAPS = {
            "airdefense1",
            "airdefense2",
            "hangar1",
            "hangar2",
            "mcc_landing",
            "mcc_1",
            "convoy1",
            "building_b",
            "convoy2",
            "convoy2b",
            "hub1",
            "hub2",
            "medlabs",
            "walker",
            "dispersal",
            "recomp",
            "putra",
            "waste",
            "mcc_2",
            "storage1 first",
            "storage2",
            "storage1 second",
            "tram1",
            "tram1b",
            "process1 first",
            "process2",
            "process1 second",
            "network1",
            "network2",
            "core1",
            "core2",
    };

    public static final String[] QUAKE4_LEVELS = {
            "AIR DEFENSE BUNKER", // Act I
            "AIR DEFENSE TRENCHES",
            "HANGAR PERIMETER",
            "INTERIOR HANGAR",
            "MCC LANDING SITE",
            "OPERATION: ADVANTAGE", // Act II
            "CANYON",
            "PERIMETER DEFENSE STATION",
            "AQUEDUCTS",
            "AQUEDUCTS ANNEX",
            "NEXUS HUB TUNNELS",
            "NEXUS HUB",
            "STROGG MEDICAL FACILITIES", // Act III
            "CONSTRUCTION ZONE",
            "DISPERSAL FACILITY",
            "RECOMPOSITION CENTER",
            "PUTRIFICATION CENTER",
            "WASTE PROCESSING FACILITY",
            "OPERATION: LAST HOPE", // Act IV
            "DATA STORAGE TERMINAL",
            "DATA STORAGE SECURITY",
            "DATA STORAGE TERMINAL",
            "TRAM HUB STATION",
            "TRAM RAIL",
            "DATA PROCESSING TERMINAL",
            "DATA PROCESSING SECURITY",
            "DATA PROCESSING TERMINAL",
            "DATA NETWORKING TERMINAL",
            "DATA NETWORKING SECURITY",
            "NEXUS CORE", // Act V
            "THE NEXUS",
    };*/

    public static final String[] CONTROLS_NAMES = {
            "Joystick",
            "Shoot",
            "Jump",
            "Crouch",
            "Reload",
            "PDA",
            "Flashlight",
            "Pause",
            "Extra 1",
            "Extra 2",
            "Extra 3",
            "Keyboard",
            "Console",
            "Run",
            "Zoom",
            "Interact",
            "Weapon",
            "Score",
            "Extra 0",
            "Extra 4",
            "Extra 5",
            "Extra 6",
            "Extra 7",
            "Extra 8",
            "Extra 9",
    };

    // OpenGL Surface color format
    public static final int GLFORMAT_RGB565      = 0x0565;
    public static final int GLFORMAT_RGBA4444    = 0x4444;
    public static final int GLFORMAT_RGBA5551    = 0x5551;
    public static final int GLFORMAT_RGBA8888    = 0x8888;
    public static final int GLFORMAT_RGBA1010102 = 0xaaa2;

    // back key function mask
    public static final int ENUM_BACK_NONE   = 0;
    public static final int ENUM_BACK_ESCAPE = 1;
    public static final int ENUM_BACK_EXIT   = 2;
    public static final int ENUM_BACK_ALL    = 0xFF;

    public static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_INTERVAL = 1000;
    public static final int CONST_DOUBLE_PRESS_BACK_TO_EXIT_COUNT    = 3;

    public static final String GAME_EXECUABLE = "game.arm";

    // extra internal game file version
    public static final String TDM_GLSL_SHADER_VERSION        = "2.12.3";
    public static final String RBDOOM3BFG_HLSL_SHADER_VERSION = "1.4.1";
    public static final String GZDOOM_VERSION                 = "4.12.2.2";

    public static final String IDTECH4AMM_PAK_SUFFIX = ".zipak";


    public static  boolean IS_NEON      = false; // only armv7-a 32. arm64 always support, but using hard
    public static  boolean IS_64        = false;
    public static  boolean SYSTEM_64    = false;
    public static  String  ARCH         = "";
    private static boolean _is_detected = false;

    private static boolean GetCpuInfo()
    {
        if(_is_detected)
            return true;
        IS_64 = Q3EJNI.Is64();
        ARCH = IS_64 ? "aarch64" : "arm";
        BufferedReader br = null;
        try
        {
            br = new BufferedReader(new FileReader("/proc/cpuinfo"));
            String l;
            while((l = br.readLine()) != null)
            {
                if((l.contains("Features")) && (l.contains("neon")))
                {
                    IS_NEON = true;
                }
                if(l.contains("Processor") && (l.contains("AArch64")))
                {
                    SYSTEM_64 = true;
                    IS_NEON = true;
                }

            }
            _is_detected = true;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            _is_detected = false;
        }
        finally
        {
            try
            {
                if(br != null)
                    br.close();
            }
            catch(IOException ioe)
            {
                ioe.printStackTrace();
            }
        }
        return _is_detected;
    }

    static
    {
        GetCpuInfo();
    }

    private Q3EGlobals()
    {
    }
}
