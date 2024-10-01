package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.os.Bundle;

import com.karin.idTech4Amm.ConfigEditorActivity;
import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;

public final class EditConfigFileFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_path;
    private String m_game;
    private String m_file;

    public EditConfigFileFunc(GameLauncher gameLauncher, int code)
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

        m_path = data.getString("path");
        m_game = data.getString("game");
        m_file = data.getString("file");

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        String basePath = m_path;
        if(!Q3EUtils.q3ei.IsStandaloneGame())
        {
            String innerDir = Q3EUtils.q3ei.GetGameModSubDirectory();
            if(KStr.NotEmpty(innerDir))
            {
                if(!m_game.startsWith(innerDir))
                    basePath = KStr.AppendPath(basePath, innerDir);
            }
            if(!Q3EUtils.q3ei.isETW)
				basePath = KStr.AppendPath(basePath, m_game);
        }
        basePath = KStr.AppendPath(basePath, m_file);
        File f = new File(basePath);
        if(!f.isFile() || !f.canWrite() || !f.canRead())
        {
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.file_can_not_access) + basePath);
            return;
        }

        Intent intent = new Intent(m_gameLauncher, ConfigEditorActivity.class);
        intent.putExtra("CONST_FILE_PATH", basePath);
        m_gameLauncher.startActivity(intent);
    }
}
