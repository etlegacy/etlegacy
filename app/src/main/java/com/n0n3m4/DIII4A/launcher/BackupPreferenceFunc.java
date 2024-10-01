package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Toast;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.PreferenceBackup;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

import java.io.OutputStream;

public final class BackupPreferenceFunc extends GameLauncherFunc
{
    private final int m_code;

    public BackupPreferenceFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
    }

    private String GenDefaultBackupFileName()
    {
        return m_gameLauncher.getApplicationContext().getPackageName() + "_preferences_backup.xml";
    }

    public void Start(Bundle data)
    {
        Uri uri = data.getParcelable("uri");
        if(null != uri)
        {
            BackupPreferences(uri);
            return;
        }

        super.Start(data);
        Reset();
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.save_preferences_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT)
        {
            Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            intent.setType("*/*"); // application/xml
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.putExtra(Intent.EXTRA_TITLE, GenDefaultBackupFileName());

            m_gameLauncher.startActivityForResult(intent, m_code);
        }
        else
        {
            Toast_long(m_gameLauncher.getString(R.string.not_supported));
        }
    }

    private void BackupPreferences(Uri uri)
    {
        OutputStream os = null;
        try
        {
            os = m_gameLauncher.getContentResolver().openOutputStream(uri);
            PreferenceBackup backup = new PreferenceBackup(m_gameLauncher);
            if(backup.Dump(os))
                Toast_long(R.string.backup_preferences_file_success);
            else
            {
                String[] args = {""};
                backup.GetError(args);
                Toast_long(Q3ELang.tr(m_gameLauncher, R.string.backup_preferences_file_fail) + args[0]);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Toast_long(R.string.backup_preferences_file_error);
        }
        finally
        {
            FileUtility.CloseStream(os);
        }
    }
}
