package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public final class ChooseGameLibFunc extends GameLauncherFunc
{
    private String m_key;
    private final int m_code;
    private String m_path;
    private Runnable m_addCallback;
    private Runnable m_editCallback;

    public ChooseGameLibFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        m_key = data.getString("key");
        m_path = data.getString("path");

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.load_external_game_library)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;

        run();
    }

    public void SetAddCallback(Runnable runnable)
    {
        m_addCallback  = runnable;
    }

    public void SetEditCallback(Runnable runnable)
    {
        m_editCallback  = runnable;
    }

    public void run()
    {
        final SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        final String libPath = ContextUtility.NativeLibDir(m_gameLauncher) + "/";
        final String[] Libs = m_gameLauncher.GetGameManager().GetGameLibs(Q3EUtils.q3ei.game, false);
        final String PreferenceKey = m_key;
        final List<CharSequence> items = new ArrayList<>();
        final List<String> values = new ArrayList<>();
        String lib = preference.getString(PreferenceKey, "");
        int selected = -1;
        int i = 0;

        for(; i < Libs.length; i++)
        {
            String n = "lib" + Libs[i] + ".so";
            items.add(n);
            String p = libPath + n;
            values.add(p);
            if(p.equals(lib))
                selected = i;
        }

        try
        {
            String path = m_path;
            File file = new File(path);
            if(file.isDirectory())
            {
                File[] files = file.listFiles();
                for (File f : files)
                {
                    if(!f.isFile() || !f.canRead())
                        continue;
                    String n = f.getName();
                    if(!n.endsWith(".so") && !n.startsWith("lib"))
                        continue;
                    items.add("<external>/" + n);
                    String p = f.getCanonicalPath();
                    values.add(p);
                    if(p.equals(lib))
                        selected = i;
                    i++;
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        boolean hasExternal = items.size() > Libs.length;
        StringBuilder sb = new StringBuilder();
        if(Q3EGlobals.IS_64)
            sb.append("armv8");
        else
            sb.append("armv7-a");
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(Q3EUtils.q3ei.game_name + " " + Q3ELang.tr(m_gameLauncher, R.string.game_library) + "(" + sb.toString() + ")");
        builder.setSingleChoiceItems(items.toArray(new CharSequence[0]), selected, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int p)
            {
                String lib = values.get(p);
                SetResult(lib);
                Callback();
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.unset, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                SetResult("");
                Callback();
                dialog.dismiss();
            }
        });
        builder.setPositiveButton(R.string.add, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                if(null != m_addCallback)
                    m_addCallback.run();
            }
        });
        if(hasExternal)
        {
            builder.setNeutralButton(R.string.edit, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int p)
                {
                    if(null != m_editCallback)
                        m_editCallback.run();
                }
            });
        }
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
