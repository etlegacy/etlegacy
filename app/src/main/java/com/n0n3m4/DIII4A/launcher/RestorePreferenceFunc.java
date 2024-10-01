package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.widget.Toast;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.PreferenceBackup;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

import java.io.InputStream;

public final class RestorePreferenceFunc extends GameLauncherFunc
{
    private final int m_code;

    public RestorePreferenceFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        Uri uri = data.getParcelable("uri");
        if(null != uri)
        {
            RestorePreferences(uri);
            return;
        }

        super.Start(data);
        Reset();
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.read_preferences_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        Intent intent = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        else
            intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.setType("*/*"); // application/xml
        intent.addCategory(Intent.CATEGORY_OPENABLE);

        m_gameLauncher.startActivityForResult(intent, m_code);
    }

    private void RestorePreferences(Uri uri)
    {
        InputStream is = null;
        try
        {
            is = m_gameLauncher.getContentResolver().openInputStream(uri);
            PreferenceBackup backup = new PreferenceBackup(m_gameLauncher);
            if(backup.Restore(is))
            {
                Toast_long(R.string.restore_preferences_file_success_app_will_reboot);
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        ContextUtility.RestartApp(m_gameLauncher);
                    }
                }, 1000);
            }
            else
            {
                String[] args = {""};
                backup.GetError(args);
                Toast_long(Q3ELang.tr(m_gameLauncher, R.string.restore_preferences_file_fail) + args[0]);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Toast_long(R.string.restore_preferences_file_error);
        }
        finally
        {
            FileUtility.CloseStream(is);
        }
    }
}
