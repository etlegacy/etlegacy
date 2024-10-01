package com.n0n3m4.DIII4A.launcher;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.n0n3m4.DIII4A.GameLauncher;

import java.util.Map;

public final class DebugPreferenceFunc extends GameLauncherFunc
{
    public DebugPreferenceFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        StringBuilder sb = new StringBuilder();
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        Map<String, ?> map = preference.getAll();
        final String endl = TextHelper.GetDialogMessageEndl();
        int i = 0;
        for(Map.Entry<String, ?>  entry : map.entrySet())
        {
            sb.append(i++).append(". ").append(entry.getKey());
            sb.append(": ").append(entry.getValue());
            sb.append(endl);
        }
        ContextUtility.OpenMessageDialog(m_gameLauncher, "Shared preferences", TextHelper.GetDialogMessage(sb.toString()));
    }
}
