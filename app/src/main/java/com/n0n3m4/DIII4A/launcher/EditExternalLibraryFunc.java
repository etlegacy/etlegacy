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

public final class EditExternalLibraryFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_path;

    public EditExternalLibraryFunc(GameLauncher gameLauncher, Runnable runnable, int code)
    {
        super(gameLauncher, runnable);
        m_code = code;
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();
        m_path = data.getString("path");
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.access_external_game_library)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;

        run();
    }

    public void run()
    {
        final List<CharSequence> items = new ArrayList<>();
        final List<String> values = new ArrayList<>();

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
                items.add(n);
                String p = f.getAbsolutePath();
                values.add(p);
            }
        }

        String[] result = new String[items.size()];
        StringBuilder sb = new StringBuilder();
        if(Q3EGlobals.IS_64)
            sb.append("armv8");
        else
            sb.append("armv7-a");
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.edit_external_game_library);
        builder.setMultiChoiceItems(items.toArray(new CharSequence[0]), new boolean[items.size()], new DialogInterface.OnMultiChoiceClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which, boolean isChecked)
            {
                result[which] = isChecked ? values.get(which) : null;
            }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                SetResult(false);
                Callback();
                dialog.dismiss();
            }
        });
        builder.setNeutralButton(R.string.remove, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                SetResult(Remove(result) > 0);
                Callback();
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private int Remove(String[] paths)
    {
        int res = 0;
        for (String path : paths)
        {
            if(null == path || path.isEmpty())
                continue;
            File file = new File(path);
            if(file.isFile())
                file.delete();
            res++;
        }
        return res;
    }
}
