package com.karin.idTech4Amm.sys;

import com.etlegacy.app.R;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

// game mod manager
public final class GameManager
{
    private final Map<String, List<GameProp>> GameProps = new LinkedHashMap<>();

    public final static String[] Games = {
            /*Q3EGlobals.GAME_DOOM3,
            Q3EGlobals.GAME_QUAKE4,
            Q3EGlobals.GAME_PREY,
            Q3EGlobals.GAME_RTCW,
            Q3EGlobals.GAME_QUAKE3,
            Q3EGlobals.GAME_QUAKE2,
            Q3EGlobals.GAME_QUAKE1,
            Q3EGlobals.GAME_DOOM3BFG,
            Q3EGlobals.GAME_TDM,
            Q3EGlobals.GAME_GZDOOM,*/
            Q3EGlobals.GAME_ETW,
    };

    public GameManager()
    {
        InitGameProps();
    }

    public static class GameProp
    {
        public final int     index;
        public final String  game;
        public final String  fs_game;
        public final String  fs_game_base;
        public final boolean is_user;
        public final String  lib;

        public GameProp(int index, String game, String fs_game, String fs_game_base, boolean is_user, String lib)
        {
            this.index = index;
            this.game = game;
            this.fs_game = fs_game;
            this.fs_game_base = fs_game_base;
            this.is_user = is_user;
            this.lib = lib;
        }

        public boolean IsGame(String game)
        {
            if (null == game)
                game = "";
            if(game.equals(this.game))
                return true;
            if(index == 0 && game.isEmpty())
                return true;
            return false;
        }

        public boolean IsValid()
        {
            return index >= 0 && !game.isEmpty();
        }
    }

    private void InitGameProps()
    {
        List<GameProp> props;
        GameProp prop;

        for(String game : Games)
            GameProps.put(game, new ArrayList<>());
        Game[] values = Game.values();

        for (Game value : values)
        {
            props = GameProps.get(value.type);
            prop = new GameProp(props.size(), value.game, value.fs_game, value.fs_game_base, false, value.lib);
            props.add(prop);
        }
    }

    public GameProp ChangeGameMod(String game, boolean userMod)
    {
        if (null == game)
            game = "";

        List<GameProp> list;
        /*if (Q3EUtils.q3ei.isQ4)
        {
            list = GameProps.get(Q3EGlobals.GAME_QUAKE4);
        }
        else if (Q3EUtils.q3ei.isPrey)
        {
            list = GameProps.get(Q3EGlobals.GAME_PREY);
        }
        else if (Q3EUtils.q3ei.isQ1)
        {
            list = GameProps.get(Q3EGlobals.GAME_QUAKE1);
        }
        else if (Q3EUtils.q3ei.isQ2)
        {
            list = GameProps.get(Q3EGlobals.GAME_QUAKE2);
        }
        else if (Q3EUtils.q3ei.isQ3)
        {
            list = GameProps.get(Q3EGlobals.GAME_QUAKE3);
        }
        else if (Q3EUtils.q3ei.isRTCW)
        {
            list = GameProps.get(Q3EGlobals.GAME_RTCW);
        }
        else if (Q3EUtils.q3ei.isTDM)
        {
            list = GameProps.get(Q3EGlobals.GAME_TDM);
        }
        else if (Q3EUtils.q3ei.isD3BFG)
        {
            list = GameProps.get(Q3EGlobals.GAME_DOOM3BFG);
        }
        else if (Q3EUtils.q3ei.isDOOM)
        {
            list = GameProps.get(Q3EGlobals.GAME_GZDOOM);
        }
        else */if (Q3EUtils.q3ei.isETW)
        {
            list = GameProps.get(Q3EGlobals.GAME_ETW);
        }
        else
        {
            list = GameProps.get(Q3EGlobals.GAME_DOOM3);
        }

        GameProp res = null;
        for (GameProp prop : list)
        {
            if(prop.IsGame(game))
            {
                res = prop;
                break;
            }
        }
        if(null == res)
            res = new GameProp(0, "", game, "", userMod, "");
        return res;
    }

    public String GetGameOfMod(String game)
    {
        for (String key : GameProps.keySet())
        {
            List<GameProp> props = GameProps.get(key);
            for (GameProp prop : props)
            {
                if(prop.game.equals(game))
                    return key;
            }
        }
        return null;
    }

    public List<GameProp> GetGame(String game)
    {
        return GameProps.get(game);
    }

    public static int GetGameIcon()
    {
        /*if (Q3EUtils.q3ei.isPrey)
            return R.drawable.prey_icon;
        else if (Q3EUtils.q3ei.isQ4)
            return R.drawable.q4_icon;
        else if (Q3EUtils.q3ei.isQ1)
            return R.drawable.q1_icon;
        else if (Q3EUtils.q3ei.isQ2)
            return R.drawable.q2_icon;
        else if (Q3EUtils.q3ei.isQ3)
            return R.drawable.q3_icon;
        else if (Q3EUtils.q3ei.isRTCW)
            return R.drawable.rtcw_icon;
        else if (Q3EUtils.q3ei.isTDM)
            return R.drawable.tdm_icon;
        else if (Q3EUtils.q3ei.isD3BFG)
            return R.drawable.d3bfg_icon;
        else if (Q3EUtils.q3ei.isDOOM)
            return R.drawable.gzdoom_icon;
        else */
		if (Q3EUtils.q3ei.isETW)
			return R.mipmap.ic_launcher;
            /*return R.drawable.etw_icon;*/
        else
            return R.mipmap.ic_launcher;
    }

    public static int GetGameThemeColor()
    {
        /*if (Q3EUtils.q3ei.isPrey)
            return R.color.theme_prey_main_color;
        else if (Q3EUtils.q3ei.isQ4)
            return R.color.theme_quake4_main_color;
        else if (Q3EUtils.q3ei.isQ1)
            return R.color.theme_quake1_main_color;
        else if (Q3EUtils.q3ei.isQ2)
            return R.color.theme_quake2_main_color;
        else if (Q3EUtils.q3ei.isQ3)
            return R.color.theme_quake3_main_color;
        else if (Q3EUtils.q3ei.isRTCW)
            return R.color.theme_rtcw_main_color;
        else if (Q3EUtils.q3ei.isTDM)
            return R.color.theme_tdm_main_color;
        else if (Q3EUtils.q3ei.isD3BFG)
            return R.color.theme_d3bfg_main_color;
        else if (Q3EUtils.q3ei.isDOOM)
            return R.color.theme_gzdoom_main_color;
        else */
		if (Q3EUtils.q3ei.isETW)
            return R.color.theme_etw_main_color;
        else
            return R.color.theme_etw_main_color;
    }

    public static int GetGameNameTs(String name)
    {
        /*if(Q3EGlobals.GAME_PREY.equalsIgnoreCase(name))
            return R.string.prey_2006;
        else if(Q3EGlobals.GAME_QUAKE4.equalsIgnoreCase(name))
            return R.string.quake_4;
        else if(Q3EGlobals.GAME_QUAKE2.equalsIgnoreCase(name))
            return R.string.quake_2;
        else if(Q3EGlobals.GAME_QUAKE3.equalsIgnoreCase(name))
            return R.string.quake_3;
        else if(Q3EGlobals.GAME_RTCW.equalsIgnoreCase(name))
            return R.string.rtcw;
        else if(Q3EGlobals.GAME_TDM.equalsIgnoreCase(name))
            return R.string.tdm;
        else if(Q3EGlobals.GAME_QUAKE1.equalsIgnoreCase(name))
            return R.string.quake_1;
        else if(Q3EGlobals.GAME_DOOM3BFG.equalsIgnoreCase(name))
            return R.string.doom_3_bfg;
        else if(Q3EGlobals.GAME_GZDOOM.equalsIgnoreCase(name))
            return R.string.doom;
        else */
		if(Q3EGlobals.GAME_ETW.equalsIgnoreCase(name))
            return R.string.etw;
        else
            return R.string.etw;
    }

    public String[] GetGameLibs(String name, boolean makePlatform)
    {
        List<GameProp> gameProps = GameProps.get(name);
        List<String> list = new ArrayList<>();
        for (GameProp gameProp : gameProps)
        {
            if(!list.contains(gameProp.lib))
                list.add(gameProp.lib);
        }
        if(!makePlatform)
            return list.toArray(new String[0]);
        else
        {
            String[] res = new String[list.size()];
            for (int i = 0; i < list.size(); i++)
            {
                res[i] = "lib" + list.get(i) + ".so";
            }
            return res;
        }
    }
}
