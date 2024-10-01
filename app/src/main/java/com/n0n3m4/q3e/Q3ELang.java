package com.n0n3m4.q3e;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Build;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;

import java.util.Locale;

public final class Q3ELang
{
    public static final String CONST_LANG_SYSTEM = "system";

    public static String tr(Context context, int id, Object...args)
    {
        return context.getResources().getString(id, args);
    }

    public static void Locale(Context context)
    {
        String lang = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.LANG, CONST_LANG_SYSTEM);
        Locale(context, lang);
    }

    public static void Locale(Context context, String language)
    {
        if(null == language || language.isEmpty() || CONST_LANG_SYSTEM.equalsIgnoreCase(language))
            return;

        Locale locale = new Locale(language);
        Locale.setDefault(locale);

        Locale loc;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP)
            loc = Locale.forLanguageTag(language);
        else
            loc = new Locale(language);

        Resources resources = context.getResources();
        Configuration configuration = resources.getConfiguration();
        DisplayMetrics metrics = resources.getDisplayMetrics();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
            configuration.setLocale(loc);
        else
            configuration.locale = loc;

        resources.updateConfiguration(configuration, metrics);
    }
}
