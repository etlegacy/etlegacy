package com.n0n3m4.q3e;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

public final class Q3EPreference
{
    public static final String pref_datapath      = "q3e_datapath";
    public static final String pref_hideonscr     = "q3e_hideonscr";
    public static final String pref_mapvol        = "q3e_mapvol";
    public static final String pref_analog        = "q3e_analog";
    public static final String pref_detectmouse   = "q3e_detectmouse";
    public static final String pref_eventdev      = "q3e_eventdev";
    public static final String pref_mousepos      = "q3e_mousepos";
    public static final String pref_scrres        = "q3e_scrres";
    public static final String pref_resx          = "q3e_resx";
    public static final String pref_resy          = "q3e_resy";
    public static final String pref_32bit         = "q3e_32bit";
    public static final String pref_msaa          = "q3e_msaa";
    public static final String pref_2fingerlmb    = "q3e_2fingerlmb";
    public static final String pref_nolight       = "q3e_nolight";
    public static final String pref_useetc1       = "q3e_useetc1";
    public static final String pref_usedxt        = "q3e_usedxt";
    public static final String pref_useetc1cache  = "q3e_useetc1cache";
    public static final String pref_controlprefix = "q3e_controls_";

    public static final String pref_harm_image_useetc2                = "q3e_image_useetc2"; //k
    public static final String pref_harm_16bit                        = "q3e_harm_16bit"; //k
    public static final String pref_harm_r_harmclearvertexbuffer      = "q3e_r_harmclearvertexbuffer"; //k
    public static final String pref_harm_r_specularExponent           = "q3e_harm_r_specularExponent"; //k
    public static final String pref_harm_r_specularExponentBlinnPhong = "q3e_harm_r_specularExponentBlinnPhong"; //k
    public static final String pref_harm_r_specularExponentPBR        = "q3e_harm_r_specularExponentPBR"; //k
    public static final String pref_harm_r_lightingModel              = "q3e_harm_r_lightingModel"; //k
    public static final String pref_harm_r_ambientLightingBrightness        = "q3e_harm_r_ambientLightingBrightness"; //k
    public static final String pref_harm_mapBack                      = "q3e_harm_map_back"; //k
    public static final String pref_harm_game                         = "q3e_harm_game"; //k

    // DOOM 3
    public static final String pref_params                     = "q3e_params";
    public static final String pref_harm_fs_game               = "q3e_harm_fs_game"; //k
    public static final String pref_harm_game_lib              = "q3e_harm_game_lib"; //k
    public static final String pref_harm_user_mod              = "q3e_harm_user_mod"; //k
    public static final String pref_harm_command_record        = "q3e_harm_command_record"; //k
    // Quake 4
    public static final String pref_harm_q4_fs_game            = "q3e_harm_q4_fs_game"; //k
    public static final String pref_harm_q4_game_lib           = "q3e_harm_q4_game_lib"; //k
    public static final String pref_harm_q4_user_mod           = "q3e_harm_q4_user_mod"; //k
    public static final String pref_params_quake4              = "q3e_params_quake4"; //k
    public static final String pref_harm_q4_command_record     = "q3e_harm_q4_command_record"; //k
    // Prey
    public static final String pref_harm_prey_user_mod         = "q3e_harm_prey_user_mod"; //k
    public static final String pref_params_prey                = "q3e_params_prey"; //k
    public static final String pref_harm_prey_fs_game          = "q3e_harm_prey_fs_game"; //k
    public static final String pref_harm_prey_game_lib         = "q3e_harm_prey_game_lib"; //k
    public static final String pref_harm_prey_command_record   = "q3e_harm_prey_command_record"; //k
    // Quake 2
    public static final String pref_harm_q2_fs_game            = "q3e_harm_q2_fs_game"; //k
    public static final String pref_harm_q2_game_lib           = "q3e_harm_q2_game_lib"; //k
    public static final String pref_harm_q2_user_mod           = "q3e_harm_q2_user_mod"; //k
    public static final String pref_params_q2                  = "q3e_params_q2"; //k
    public static final String pref_harm_q2_command_record     = "q3e_harm_q2_command_record"; //k
    // Quake 3
    public static final String pref_harm_q3_fs_game            = "q3e_harm_q3_fs_game"; //k
    public static final String pref_harm_q3_game_lib           = "q3e_harm_q3_game_lib"; //k
    public static final String pref_harm_q3_user_mod           = "q3e_harm_q3_user_mod"; //k
    public static final String pref_params_q3                  = "q3e_params_q3"; //k
    public static final String pref_harm_q3_command_record     = "q3e_harm_q3_command_record";
    // RTCW
    public static final String pref_harm_rtcw_fs_game          = "q3e_harm_rtcw_fs_game"; //k
    public static final String pref_harm_rtcw_game_lib         = "q3e_harm_rtcw_game_lib"; //k
    public static final String pref_harm_rtcw_user_mod         = "q3e_harm_rtcw_user_mod"; //k
    public static final String pref_params_rtcw                = "q3e_params_rtcw"; //k
    public static final String pref_harm_rtcw_command_record   = "q3e_harm_rtcw_command_record";
    // Quake 1
    public static final String pref_harm_q1_fs_game            = "q3e_harm_q1_fs_game"; //k
    public static final String pref_harm_q1_game_lib           = "q3e_harm_q1_game_lib"; //k
    public static final String pref_harm_q1_user_mod           = "q3e_harm_q1_user_mod"; //k
    public static final String pref_params_q1                  = "q3e_params_q1"; //k
    public static final String pref_harm_q1_command_record     = "q3e_harm_q1_command_record";
    // The Dark Mod
    public static final String pref_harm_tdm_fs_game           = "q3e_harm_tdm_fs_game"; //k
    public static final String pref_harm_tdm_game_lib          = "q3e_harm_tdm_game_lib"; //k
    public static final String pref_harm_tdm_user_mod          = "q3e_harm_tdm_user_mod"; //k
    public static final String pref_params_tdm                 = "q3e_params_tdm"; //k
    public static final String pref_harm_tdm_command_record    = "q3e_harm_tdm_command_record";
    // DOOM 3 BFG
    public static final String pref_harm_d3bfg_fs_game         = "q3e_harm_d3bfg_fs_game"; //k
    public static final String pref_harm_d3bfg_game_lib        = "q3e_harm_d3bfg_game_lib"; //k
    public static final String pref_harm_d3bfg_user_mod        = "q3e_harm_d3bfg_user_mod"; //k
    public static final String pref_params_d3bfg               = "q3e_params_d3bfg"; //k
    public static final String pref_harm_d3bfg_command_record  = "q3e_harm_d3bfg_command_record";
    // GZDOOM
    public static final String pref_harm_gzdoom_fs_game        = "q3e_harm_gzdoom_fs_game"; //k
    public static final String pref_harm_gzdoom_game_lib       = "q3e_harm_gzdoom_game_lib"; //k
    public static final String pref_harm_gzdoom_user_mod       = "q3e_harm_gzdoom_user_mod"; //k
    public static final String pref_params_gzdoom              = "q3e_params_gzdoom"; //k
    public static final String pref_harm_gzdoom_command_record = "q3e_harm_gzdoom_command_record";
    // ETW
    public static final String pref_harm_etw_fs_game          = "q3e_harm_etw_fs_game"; //k
    public static final String pref_harm_etw_game_lib         = "q3e_harm_etw_game_lib"; //k
    public static final String pref_harm_etw_user_mod         = "q3e_harm_etw_user_mod"; //k
    public static final String pref_params_etw                = "q3e_params_etw"; //k
    public static final String pref_harm_etw_command_record   = "q3e_harm_etw_command_record";

    public static final String pref_harm_view_motion_control_gyro     = "q3e_harm_mouse_move_control_gyro"; //k
    public static final String pref_harm_view_motion_gyro_x_axis_sens = "q3e_harm_view_motion_gyro_x_axis_sens"; //k
    public static final String pref_harm_view_motion_gyro_y_axis_sens = "q3e_harm_view_motion_gyro_y_axis_sens"; //k
    public static final String pref_harm_auto_quick_load              = "q3e_harm_auto_quick_load"; //k
    public static final String pref_harm_multithreading               = "q3e_harm_multithreading"; //k
    public static final String pref_harm_s_driver                     = "q3e_harm_s_driver"; //k
    public static final String pref_harm_function_key_toolbar         = "harm_function_key_toolbar"; //k
    public static final String pref_harm_function_key_toolbar_y       = "harm_function_key_toolbar_y"; //k
    public static final String pref_harm_joystick_release_range       = "harm_joystick_release_range"; //k
    public static final String pref_harm_joystick_unfixed             = "harm_joystick_unfixed"; //k
    public static final String pref_harm_joystick_visible             = "harm_joystick_visible"; //k
    public static final String pref_harm_joystick_inner_dead_zone     = "harm_joystick_inner_dead_zone"; //k
    public static final String pref_harm_using_mouse                  = "harm_using_mouse"; //k
    public static final String pref_harm_find_dll                     = "harm_find_dll"; //k
    public static final String pref_harm_r_maxFps                     = "q3e_harm_r_maxFps"; //k
    public static final String pref_harm_skip_intro                   = "q3e_harm_skip_intro"; //k
    public static final String pref_harm_scale_by_screen_area         = "q3e_harm_scale_by_screen_area";
    public static final String pref_harm_r_useShadowMapping           = "q3e_harm_r_useShadowMapping"; //k
    public static final String pref_harm_r_shadowMapAlpha             = "q3e_harm_r_shadowMapAlpha"; //k
    public static final String pref_harm_opengl                       = "q3e_harm_opengl"; //k
    public static final String pref_harm_s_useOpenAL                  = "q3e_harm_s_useOpenAL"; //k
    public static final String pref_harm_s_useEAXReverb               = "q3e_harm_s_useEAXReverb"; //k
    public static final String pref_harm_r_stencilShadowTranslucent   = "q3e_harm_r_stencilShadowTranslucent"; //k
    public static final String pref_harm_r_stencilShadowAlpha         = "q3e_harm_r_stencilShadowAlpha"; //k
    public static final String pref_harm_r_stencilShadowSoft          = "q3e_harm_r_stencilShadowSoft"; //k
    public static final String pref_harm_r_stencilShadowCombine       = "q3e_harm_r_stencilShadowCombine"; //k
    public static final String pref_harm_r_autoAspectRatio            = "q3e_harm_r_autoAspectRatio"; //k
    public static final String pref_scrres_scheme                     = "harm_q3e_scrres";
    public static final String pref_scrres_scale                      = "harm_q3e_scrres_scale";
    public static final String pref_harm_r_shadowMapPerforatedShadow  = "q3e_harm_r_shadowMapPerforatedShadow"; //k

    public static final String RUN_BACKGROUND                = "harm_run_background";
    public static final String RENDER_MEM_STATUS             = "harm_render_mem_status";
    public static final String HIDE_NAVIGATION_BAR           = "harm_hide_nav";
    public static final String VOLUME_UP_KEY                 = "harm_volume_up_key";
    public static final String VOLUME_DOWN_KEY               = "harm_volume_down_key";
    public static final String WEAPON_PANEL_KEYS             = "harm_weapon_panel_keys";
    public static final String CONTROLS_CONFIG_POSITION_UNIT = "harm_controls_config_position_unit";
    public static final String NO_HANDLE_SIGNALS             = "harm_no_handle_signals";
    public static final String REDIRECT_OUTPUT_TO_FILE       = "harm_redirect_output_to_file";
    public static final String COVER_EDGES                   = "harm_cover_edges";
    public static final String CONTROLS_THEME                = "harm_controls_theme";
    public static final String LANG                          = "harm_lang";
    public static final String MAP_BACK                      = "harm_map_back";
    public static final String ONSCREEN_BUTTON               = "harm_onscreen_key"; // old = "harm_onscreen_button"
    public static final String GAME_STANDALONE_DIRECTORY     = "harm_game_standalone_directory";
    public static final String LOAD_LOCAL_ENGINE_LIB         = "harm_load_local_engine_lib";
    public static final String THEME                         = "harm_theme";
    public static final String EVENT_QUEUE                   = "harm_event_queue"; // 0 - java 1 - native
    public static final String AUTOSAVE_BUTTON_SETTINGS      = "harm_autosave_button_settings";

    public static float GetFloatFromString(Context context, String name, float defVal)
    {
        return GetFloatFromString(PreferenceManager.getDefaultSharedPreferences(context), name, defVal);
    }

    public static float GetFloatFromString(SharedPreferences preferences, String name, float defVal)
    {
        String str = preferences.getString(name, "");
        if(null == str || str.isEmpty())
            return defVal;
        try
        {
            return Float.parseFloat(str);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static int GetIntFromString(Context context, String name, int defVal)
    {
        return GetIntFromString(PreferenceManager.getDefaultSharedPreferences(context), name, defVal);
    }

    public static int GetIntFromString(SharedPreferences preferences, String name, int defVal)
    {
        String str = preferences.getString(name, "");
        if(null == str || str.isEmpty())
            return defVal;
        try
        {
            return Integer.parseInt(str);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static String GetStringFromInt(Context context, String name, int defVal)
    {
        return GetStringFromInt(PreferenceManager.getDefaultSharedPreferences(context), name, defVal);
    }

    public static String GetStringFromInt(SharedPreferences preferences, String name, int defVal)
    {
        return "" + preferences.getInt(name, defVal);
    }

    public static String GetStringFromFloat(Context context, String name, int defVal)
    {
        return GetStringFromFloat(PreferenceManager.getDefaultSharedPreferences(context), name, defVal);
    }

    public static String GetStringFromFloat(SharedPreferences preferences, String name, float defVal)
    {
        return "" + preferences.getFloat(name, defVal);
    }

    public static void SetStringFromInt(SharedPreferences preferences, String name, int val)
    {
        SetStringFromInt(preferences.edit(), name, val).commit();
    }

    public static void SetStringFromInt(Context context, String name, int val)
    {
        SetStringFromInt(PreferenceManager.getDefaultSharedPreferences(context), name, val);
    }

    public static SharedPreferences.Editor SetStringFromInt(SharedPreferences.Editor editor, String name, int val)
    {
        return editor.putString(name, "" + val);
    }

    public static void SetStringFromFloat(SharedPreferences preferences, String name, float val)
    {
        SetStringFromFloat(preferences.edit(), name, val).commit();
    }

    public static void SetStringFromFloat(Context context, String name, float val)
    {
        SetStringFromFloat(PreferenceManager.getDefaultSharedPreferences(context), name, val);
    }

    public static SharedPreferences.Editor SetStringFromFloat(SharedPreferences.Editor editor, String name, float val)
    {
        return editor.putString(name, "" + val);
    }

    public static void SetIntFromString(SharedPreferences preferences, String name, String val, int def)
    {
        SetIntFromString(preferences.edit(), name, val, def).commit();
    }

    public static void SetIntFromString(Context context, String name, String val, int def)
    {
        SetIntFromString(PreferenceManager.getDefaultSharedPreferences(context), name, val, def);
    }

    public static SharedPreferences.Editor SetIntFromString(SharedPreferences.Editor editor, String name, String val, int def)
    {
        return editor.putInt(name, Q3EUtils.parseInt_s(val, def));
    }

    public static void SetFloatFromString(SharedPreferences preferences, String name, String val, float def)
    {
        SetFloatFromString(preferences.edit(), name, val, def).commit();
    }

    public static void SetFloatFromString(Context context, String name, String val, float def)
    {
        SetFloatFromString(PreferenceManager.getDefaultSharedPreferences(context), name, val, def);
    }

    public static SharedPreferences.Editor SetFloatFromString(SharedPreferences.Editor editor, String name, String val, float def)
    {
        return editor.putFloat(name, Q3EUtils.parseFloat_s(val, def));
    }

    private Q3EPreference() {}
}
