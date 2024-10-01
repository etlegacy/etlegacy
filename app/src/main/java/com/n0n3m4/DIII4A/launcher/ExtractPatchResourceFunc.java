package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.Toast;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class ExtractPatchResourceFunc extends GameLauncherFunc
{
    private final String[] m_patchResources = {
            // "glslprogs.pk4",
            "q4base/sabot_a9.pk4",
            "rivensin/play_original_doom3_level.pk4",
    };
    private String m_path;
    private List<String> m_files;
    private final int m_code;

    public ExtractPatchResourceFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
        if(null != m_files)
            m_files.clear();
        else
            m_files = new ArrayList<>();
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        m_path = data.getString("path");
        // D3-format fonts don't need on longer
        final String[] Names = {
                // Q3ELang.tr(m_gameLauncher, R.string.opengles_shader),
                Q3ELang.tr(m_gameLauncher, R.string.bot_q3_bot_support_in_mp_game),
                Q3ELang.tr(m_gameLauncher, R.string.rivensin_play_original_doom3_level),
        };
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.game_patch_resource)
                .setItems(Names, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int p)
                    {
                        m_files.add(m_patchResources[p]);
                        ExtractPatchResource();
                    }
                })
                .setNegativeButton(R.string.cancel, null)
                .setPositiveButton(R.string.extract_all, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int p)
                    {
                        m_files.addAll(Arrays.asList(m_patchResources));
                        ExtractPatchResource();
                    }
                })
                .create()
                .show()
        ;
    }

    private int ExtractPatchResource()
    {
        if(null == m_files || m_files.isEmpty())
            return -1;

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return -1;

        run();
        return m_files.size();
    }

    public void run()
    {
        int r = 0;
        String gamePath = m_path;
        final String BasePath = gamePath + File.separator;
        for (String str : m_files)
        {
            File f = new File(str);
            String path = f.getParent();
            if(null == path)
                path = "";
            String name = f.getName();
            String newFileName = "z_" + FileUtility.GetFileBaseName(name) + "_idTech4Amm." + FileUtility.GetFileExtension(name);
            boolean ok = ContextUtility.ExtractAsset(m_gameLauncher, "pak/" + str, BasePath + path + File.separator + newFileName);
            if(ok)
                r++;
        }
        Toast_short(Q3ELang.tr(m_gameLauncher, R.string.extract_path_resource_) + r);
    }
}
