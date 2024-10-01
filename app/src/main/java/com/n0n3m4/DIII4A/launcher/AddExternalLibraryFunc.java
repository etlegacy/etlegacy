package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public final class AddExternalLibraryFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_path;

    public AddExternalLibraryFunc(GameLauncher gameLauncher, Runnable runnable, int code)
    {
        super(gameLauncher, runnable);
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
            Save(uri);
            return;
        }

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

    private void InputFileName(Uri uri)
    {
        String[] args = {""};
        ContextUtility.Input(m_gameLauncher, Q3ELang.tr(m_gameLauncher, R.string.input_external_library_name), Q3ELang.tr(m_gameLauncher, R.string.e_g) + " libxxxxx.so", null, args, new Runnable()
        {
            @Override
            public void run()
            {
                if(args[0].isEmpty())
                {
                    Toast_short(Q3ELang.tr(m_gameLauncher, R.string.must_input_new_library_name));
                    // InputFileName(uri);
                }
                else
                {
                    if(!args[0].startsWith("lib"))
                        args[0] = "lib" + args[0];
                    if(!args[0].endsWith(".so"))
                        args[0] = args[0] + ".so";
                    Write(uri, args[0]);
                }
            }
        }, null, null, null, null);
    }

    private void Save(Uri uri)
    {
        InputFileName(uri);
    }

    private void Write(Uri uri, String fileName)
    {
        FileOutputStream os = null;
        try
        {
            InputStream is = m_gameLauncher.getContentResolver().openInputStream(uri);
            String path = m_path;
            Q3EUtils.mkdir(path, true);
            File file = new File(path + File.separator + fileName);
            os = new FileOutputStream(file);
            Q3EUtils.Copy(os, is);
            Toast_long(R.string.add_game_library_file_success);
            Callback();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Toast_long(R.string.add_game_library_file_fail);
        }
        finally
        {
            FileUtility.CloseStream(os);
        }
    }
}
