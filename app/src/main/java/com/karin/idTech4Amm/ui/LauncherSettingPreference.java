package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.os.Handler;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.karin.idTech4Amm.sys.Theme;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EPreference;

import java.util.Set;

import android.view.View;
import android.widget.Toast;

/**
 * Launcher preference fragment
 */
public class LauncherSettingPreference extends PreferenceFragment implements Preference.OnPreferenceChangeListener
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.launcher_settings_preference);

        findPreference(PreferenceKey.LAUNCHER_ORIENTATION).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.MAP_BACK).setOnPreferenceChangeListener(this);
        findPreference(PreferenceKey.HIDE_AD_BAR).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_function_key_toolbar_y).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.LANG).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.GAME_STANDALONE_DIRECTORY).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.THEME).setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
    {
        String key = preference.getKey();
        Context context = ContextUtility.GetContext(this);
        if("standalone_game_data_directories".equals(key))
        {
            StringBuilder sb = new StringBuilder();
            final String endl = TextHelper.GetDialogMessageEndl();
            for (String game : GameManager.Games)
            {
                String gameName = Q3ELang.tr(context, GameManager.GetGameNameTs(game));
                String path = Q3EInterface.GetGameStandaloneDirectory(game);
                String pathText = TextHelper.GenLinkText("file://" + path, path);
                sb.append(gameName).append(": ").append(pathText).append(endl);
            }
            ContextUtility.OpenMessageDialog(context, Q3ELang.tr(context, R.string.game_datas_standalone_directories), TextHelper.GetDialogMessage(sb.toString()));
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        String key = preference.getKey();
        Context context = ContextUtility.GetContext(this);
        switch (key)
        {
            case PreferenceKey.LAUNCHER_ORIENTATION:
                int o = (boolean) newValue ? 0 : 1;
                ContextUtility.SetScreenOrientation(getActivity(), o);
                return true;
            case Q3EPreference.MAP_BACK:
            {
                Set<String> values = (Set<String>) newValue;
                int r = 0;
                for (String str : values)
                {
                    r |= Integer.parseInt(str);
                }
                preference.getSharedPreferences().edit().putInt(Q3EPreference.pref_harm_mapBack, r).commit();
                return true;
            }
            case PreferenceKey.HIDE_AD_BAR:
                boolean b = (boolean) newValue;
                View view = getActivity().findViewById(R.id.main_ad_layout);
                if (null != view)
                    view.setVisibility(b ? View.GONE : View.VISIBLE);
                return true;
            case Q3EPreference.pref_harm_function_key_toolbar_y:
            {
                int i;
                try
                {
                    i = Integer.parseInt((String) newValue);
                    if (i >= 0)
                        return true;
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
                return false;
            }
            case Q3EPreference.LANG:
            {
                Toast.makeText(context, R.string.be_available_on_reboot_the_next_time, Toast.LENGTH_SHORT).show();
                return true;
            }
            case Q3EPreference.GAME_STANDALONE_DIRECTORY:
            {
                Toast.makeText(context, R.string.app_is_rebooting, Toast.LENGTH_SHORT).show();
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        ContextUtility.RestartApp(LauncherSettingPreference.this.getActivity());
                    }
                }, 1000);
                return true;
            }
            case Q3EPreference.THEME:
            {
                Toast.makeText(context, R.string.app_is_rebooting, Toast.LENGTH_SHORT).show();
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        Theme.Reset(null);
                        ContextUtility.RestartApp(LauncherSettingPreference.this.getActivity());
                    }
                }, 1000);
                return true;
            }
            default:
                return false;
        }
    }
}
