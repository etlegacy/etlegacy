package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.provider.DocumentFile;
import android.view.View;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.ui.FileBrowserDialog;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

import java.io.File;

public final class ChooseGameFolderFunc extends GameLauncherFunc
{
    private final int m_code;
    private final int m_uriCode;
    private String m_path;

    public ChooseGameFolderFunc(GameLauncher gameLauncher, int code, int uriCode, Runnable runnable)
    {
        super(gameLauncher, runnable);
        m_code = code;
        m_uriCode = uriCode;
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
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        FileBrowserDialog dialog = new FileBrowserDialog(m_gameLauncher);
        dialog.SetupUI(Q3ELang.tr(m_gameLauncher, R.string.choose_data_folder));
        FileBrowserDialog.FileBrowserCallback callback = new FileBrowserDialog.FileBrowserCallback()
        {
            @Override
            public void Check(String path)
            {
                ContextUtility.GrantUriPermission(m_gameLauncher, path, m_uriCode);
            }
        };

        dialog.SetCallback(callback);

        String gamePath = m_path;

        String defaultPath = Environment.getExternalStorageDirectory().getAbsolutePath(); //System.getProperty("user.home");
        if(null == gamePath || gamePath.isEmpty())
            gamePath = defaultPath;

        FileBrowser fileBrowser = dialog.GetFileBrowser();
        int checked = fileBrowser.GetFileType(gamePath);
        switch (checked)
        {
            case FileBrowser.FileModel.ID_FILE_TYPE_FILE:
                gamePath = FileUtility.ParentPath(gamePath);
                break;
            case FileBrowser.FileModel.ID_FILE_TYPE_NOT_EXISTS:
                gamePath = defaultPath;
                break;
        }

        dialog.SetPath(gamePath);
        dialog.setButton(AlertDialog.BUTTON_NEGATIVE, Q3ELang.tr(m_gameLauncher, R.string.cancel), new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        dialog.setButton(AlertDialog.BUTTON_POSITIVE, Q3ELang.tr(m_gameLauncher, R.string.choose_current_directory), new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String result = ((FileBrowserDialog) dialog).Path();
                SetResult(result);
                dialog.dismiss();
                Callback();
            }
        });

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
        {
            dialog.setButton(AlertDialog.BUTTON_NEUTRAL, Q3ELang.tr(m_gameLauncher, R.string._default), (AlertDialog.OnClickListener)null);
            dialog.create();
            dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    dialog.SetPath(m_gameLauncher.GetDefaultGameDirectory());
                }
            });
        }
        else
        {
            dialog.setButton(AlertDialog.BUTTON_NEUTRAL, Q3ELang.tr(m_gameLauncher, R.string.reset), new AlertDialog.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    SetResult(m_gameLauncher.GetDefaultGameDirectory());
                    dialog.dismiss();
                    Callback();
                }
            });
        }

        dialog.show();
    }
}
