package com.karin.idTech4Amm.sys;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.preference.PreferenceManager;

import com.n0n3m4.q3e.Q3EPreference;

/**
 * Theme define
 */
public final class Theme
{
    private static final int THEME_UNINITIALIZED = -2; // uninitialized
    private static final int THEME_UNSET         = -1; // unset
    private static final int THEME_DEFAULT       = 0; // system
    private static final int THEME_CLASSIC       = 1; // Android 2/3 style
    private static final int THEME_HOLO          = 2; // Android 4 style
    private static final int THEME_MATERIAL      = 3; // Android 5+ style

    private static int Theme = THEME_UNINITIALIZED;

    public static int GetTheme(Context context)
    {
        Init(context);
        switch (Theme)
        {
            case THEME_CLASSIC:
                return android.R.style.Theme_WithActionBar;
            case THEME_HOLO:
                return android.R.style.Theme_Holo_Light_DarkActionBar;
            case THEME_MATERIAL:
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
                    return android.R.style.Theme_Material_Light_DarkActionBar;
                else
                    return 0;
            default:
                return android.R.style.Theme_DeviceDefault_Light_DarkActionBar;
        }
    }

    public static void SetTheme(Activity activity, boolean restart)
    {
        int theme = GetTheme(activity);
        if(theme > 0)
            activity.setTheme(theme);
        if(restart)
            activity.recreate();
    }

    private static void Init(Context context)
    {
        if(Theme <= THEME_UNINITIALIZED)
        {
            int themeId;
            String theme = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.THEME, "system");
            switch (theme)
            {
                case "classic":
                    themeId = THEME_CLASSIC;
                    break;
                case "holo":
                    themeId = THEME_HOLO;
                    break;
                case "material":
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
                        themeId = THEME_MATERIAL;
                    else
                        themeId = THEME_UNSET;
                    break;
                default:
                    themeId = THEME_DEFAULT;
            }
            Theme = themeId;
        }
    }

    public static int BlackColor(Context context)
    {
        Init(context);
        return Theme == THEME_CLASSIC ? Color.WHITE : Color.BLACK;
    }

    public static int WhiteColor(Context context)
    {
        Init(context);
        return Theme == THEME_CLASSIC ? Color.BLACK : Color.WHITE;
    }

    public static void Reset(Context context)
    {
        Theme = THEME_UNINITIALIZED;
        if(null != context)
            Init(context);
    }

	private Theme() {}
}
