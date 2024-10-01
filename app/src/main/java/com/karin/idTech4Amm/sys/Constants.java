package com.karin.idTech4Amm.sys;

import android.os.Build;

import com.karin.idTech4Amm.lib.DateTimeUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.stericson.RootTools.BuildConfig;

import java.util.Arrays;
import java.util.Date;

/**
 * Constants define
 */
public final class Constants
{
    public static final int    CONST_UPDATE_RELEASE = 57;
    public static final String CONST_RELEASE = "2024-10-01";
    public static final String CONST_EMAIL = "mail@etlegacy.com";
    public static final String CONST_DEV = "ET: Legacy Dev Team";
    public static final String CONST_CODE = "etlegacy";
    public static final String CONST_APP_NAME = "ET: Legacy"; // "DIII4A++";
    public static final String CONST_NAME = "Wolfesntein Enemy Territory Legacy for Android";
	public static final String CONST_MAIN_PAGE = "https://github.com/etlegacy/etlegacy/";
    public static final String CONST_TIEBA = "https://tieba.baidu.com/p/6825594793";
	public static final String CONST_DEVELOPER = "https://github.com/etlegacy";
    public static final String CONST_DEVELOPER_XDA = "https://forum.xda-developers.com/member.php?u=10584229";
    public static final String CONST_PACKAGE = "com.etlegacy.app";
    public static final String CONST_FDROID = "https://f-droid.org/packages/com.etlegacy.app/";
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/CHECK_FOR_UPDATE.json";
    public static final String CONST_LICENSE_URL = "https://raw.githubusercontent.com/etlegacy/etlegacy/refs/heads/master/COPYING.txt";
	public static String[] CONST_CHANGES()
    {
        return new String[] {
            "Add `Wolfenstein: Enemy Territory` support, game standalone directory named `etw`, game data directory named `etmain` and `legacy`. More view in `" + TextHelper.GenLinkText("https://www.etlegacy.com", "ET: Legacy") + "`.",
        };
	};

    public static long GetBuildTimestamp()
    {
        return System.currentTimeMillis();
    }

    public static int GetBuildSDKVersion()
    {
        return Build.VERSION.SDK_INT;
    }

    public static boolean IsDebug()
    {
        return BuildConfig.DEBUG;
    }

    public static String GetBuildTime(String format)
    {
        return DateTimeUtility.Format(GetBuildTimestamp(), format);
    }

	private Constants() {}
}
