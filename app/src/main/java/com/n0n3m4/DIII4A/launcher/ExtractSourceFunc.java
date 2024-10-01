package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.PreferenceBackup;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class ExtractSourceFunc extends GameLauncherFunc
{
    private final int m_code;

    public ExtractSourceFunc(GameLauncher gameLauncher, int code)
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
            ExtractSource(uri);
            return;
        }

        super.Start(data);
        Reset();
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.extract_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    private String GetSourceCodeUrl()
    {
        return "https://github.com/glKarin/com.n0n3m4.diii4a/archive/refs/tags/v1.1.0harmattan" + Constants.CONST_UPDATE_RELEASE + ".zip";
    }

    private boolean SourceCodeExists()
    {
        try
        {
            String[] sources = m_gameLauncher.getAssets().list("source");
            for (String source : sources)
            {
                if(source.equals("DIII4A.source.tgz"))
                    return true;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return false;
    }

    public void run()
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT && SourceCodeExists())
        {
            Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            intent.setType("*/*"); // application/xml
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.putExtra(Intent.EXTRA_TITLE, "DIII4A.source.tgz");

            m_gameLauncher.startActivityForResult(intent, m_code);
        }
        else
        {
            ContextUtility.OpenUrlExternally(m_gameLauncher, GetSourceCodeUrl());
        }
    }

    private void ExtractSource(Uri uri)
    {
        OutputStream os = null;
        InputStream is = null;
        try
        {
            os = m_gameLauncher.getContentResolver().openOutputStream(uri);
            is = m_gameLauncher.getAssets().open("source/DIII4A.source.tgz");
            Q3EUtils.Copy(os, is);
            Toast_long(R.string.extract_file_success);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Toast_long(R.string.extract_file_fail);
        }
        finally
        {
            FileUtility.CloseStream(os);
            FileUtility.CloseStream(is);
        }
    }
}
