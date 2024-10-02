package com.karin.idTech4Amm.sys;

import android.content.Context;

import com.etlegacy.app.R;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3ELang;

// game config
/*
 * config this can change launcher's game mod list.
 * If you want add a idTech4's mod and you have completed compiling c++ source code, you just edit this enum now.
 */
public enum Game
{
    /*// DOOM 3
    DOOM3_BASE(Q3EGlobals.GAME_DOOM3, "base", "", "game", "", false, R.string.doom_iii),
    DOOM3_D3XP(Q3EGlobals.GAME_DOOM3, "d3xp", "d3xp", "d3xp", "", true, R.string.doom3_resurrection_of_evil_d3xp),

    DOOM3_CDOOM(Q3EGlobals.GAME_DOOM3, "cdoom", "cdoom", "cdoom", "", true, R.string.classic_doom_cdoom),
    DOOM3_D3LE(Q3EGlobals.GAME_DOOM3, "d3le", "d3le", "d3le", "d3xp", true, R.string.doom3_bfg_the_lost_mission_d3le),
    DOOM3_RIVENSIN(Q3EGlobals.GAME_DOOM3, "rivensin", "rivensin", "rivensin", "", true, R.string.rivensin_rivensin),
    DOOM3_HARDCORPS(Q3EGlobals.GAME_DOOM3, "hardcorps", "hardcorps", "hardcorps", "", true, R.string.hardcorps_hardcorps),
    DOOM3_OVERTHINKED(Q3EGlobals.GAME_DOOM3, "overthinked", "overthinked", "overthinked", "", true, R.string.overthinked_doom_3),
    DOOM3_SABOT(Q3EGlobals.GAME_DOOM3, "sabot", "sabot",  "sabot","d3xp", true, R.string.stupid_angry_bot_a7x),
    DOOM3_HEXENEOC(Q3EGlobals.GAME_DOOM3, "hexeneoc", "hexeneoc", "hexeneoc", "", true, R.string.hexen_edge_of_chaos),
    DOOM3_FRAGGINGFREE(Q3EGlobals.GAME_DOOM3, "fraggingfree", "fraggingfree", "fraggingfree", "d3xp", true, R.string.fragging_free),
    DOOM3_LIBRECOOP(Q3EGlobals.GAME_DOOM3, "librecoop", "librecoop", "librecoop", "", true, R.string.librecoop),
    DOOM3_LIBRECOOPXP(Q3EGlobals.GAME_DOOM3, "librecoopxp", "librecoopxp", "librecoopxp", "d3xp", true, R.string.librecoop_roe),
    DOOM3_PERFECTED(Q3EGlobals.GAME_DOOM3, "perfected", "perfected", "perfected",  "", true, R.string.perfected_doom_3),
    DOOM3_PERFECTEDROE(Q3EGlobals.GAME_DOOM3, "perfected_roe", "perfected_roe", "perfected_roe",  "d3xp", true, R.string.perfected_doom_3_resurrection_of_evil),

    // Quake 4
    QUAKE4_BASE(Q3EGlobals.GAME_QUAKE4, "q4base", "", "q4game", "", false, R.string.quake_iv_q4base),
    QUAKE4_HARDQORE(Q3EGlobals.GAME_QUAKE4, "hardqore", "hardqore", "hardqore",  "", true, R.string.hardqore),

    // Prey(2006)
    PREY_BASE(Q3EGlobals.GAME_PREY, "preybase", "", "preygame", "", false, R.string.prey_preybase),

    // Quake 1
    QUAKE1_BASE(Q3EGlobals.GAME_QUAKE1, "darkplaces/id1", "", "idtech_quake", "", false, R.string.quake_1_base),

    // Quake 2
    QUAKE2_BASE(Q3EGlobals.GAME_QUAKE2, "baseq2", "", "q2game", "", false, R.string.quake_2_base),
    QUAKE2_CTF(Q3EGlobals.GAME_QUAKE2, "ctf", "ctf", "q2ctf", "", true, R.string.quake_2_ctf),
    QUAKE2_ROGUE(Q3EGlobals.GAME_QUAKE2, "rogue", "rogue", "q2rogue", "", true, R.string.quake_2_rogue),
    QUAKE2_XATRIX(Q3EGlobals.GAME_QUAKE2, "xatrix", "xatrix", "q2xatrix", "", true, R.string.quake_2_xatrix),
    QUAKE2_ZAERO(Q3EGlobals.GAME_QUAKE2, "zaero", "zaero", "q2zaero", "", true, R.string.quake_2_zaero),

    // Return to Castle Wolfenstein
    RTCW_BASE(Q3EGlobals.GAME_RTCW, "main", "", "rtcwgame", "", false, R.string.rtcw_base),

    // The dark mod
    TDM_BASE(Q3EGlobals.GAME_TDM, "", "", "thedarkmod", "", false, R.string.tdm_base),

    // Quake 3
    QUAKE3_BASE(Q3EGlobals.GAME_QUAKE3, "baseq3", "", "qagame", "", false, R.string.quake_3_base),
    QUAKE3_TEAMARENA(Q3EGlobals.GAME_QUAKE3, "missionpack", "missionpack", "qagame_mp", "", true, R.string.quake_3_teamarena),

    // Doom3 BFG
    D3BFG_BASE(Q3EGlobals.GAME_DOOM3BFG, "base", "", "RBDoom3BFG", "", false, R.string.d3bfg_base),

    // GZDOOM
    //GZDOOM_BASE(Q3EGlobals.GAME_GZDOOM, "", "", "", false, R.string.doom_base),
    GZDOOM_DOOM1(Q3EGlobals.GAME_GZDOOM, "DOOM.WAD", "DOOM.WAD", "gzdoom", "", true, R.string.doom1_base),
    GZDOOM_DOOM2(Q3EGlobals.GAME_GZDOOM, "DOOM2.WAD", "DOOM2.WAD", "gzdoom", "", true, R.string.doom2_base),
    GZDOOM_FREEDOOM1(Q3EGlobals.GAME_GZDOOM, "freedoom1.wad", "freedoom1.wad", "gzdoom", "", true, R.string.freedoom1_base),
    GZDOOM_FREEDOOM2(Q3EGlobals.GAME_GZDOOM, "freedoom2.wad", "freedoom2.wad", "gzdoom", "", true, R.string.freedoom2_base),*/

    // Wolfenstein: Enemy Territory
    ETW_BASE(Q3EGlobals.GAME_ETW, "legacy", "", "etwgame", "", false, R.string.etw_base),
    ;

    public final String  type; // game type: doom3/quake4/prey2006/......
    public final String  game; // game id: unique
    public final String  fs_game; // game data folder name
    public final String  lib; // game library file name, only for DOOM3/Quake4/Prey
    public final String  fs_game_base; // game mod data base folder name, e.g. d3xp
    public final boolean is_mod; // is a mod
    public final Object  name; // game name string resource's id or game name string

    Game(String type, String game, String fs_game, String lib, String fs_game_base, boolean is_mod, Object name)
    {
        this.type = type;
        this.game = game;
        this.fs_game = fs_game;
        this.lib = lib;
        this.fs_game_base = fs_game_base;
        this.is_mod = is_mod;
        this.name = name;
    }

    public String GetName(Context context)
    {
        if(name instanceof Integer)
            return Q3ELang.tr(context, (Integer)name);
        else if(name instanceof String)
            return (String)name;
        else
            return "";
    }
}
