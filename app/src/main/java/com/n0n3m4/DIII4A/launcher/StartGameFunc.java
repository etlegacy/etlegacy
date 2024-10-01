package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.os.Bundle;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EMain;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class StartGameFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_path;

    public StartGameFunc(GameLauncher gameLauncher, int code)
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

        //k check external storage permission
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if (res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.startgame)));
        if (res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        /*
		String dir=m_path;
		if ((new File(dir+"/base").exists())&&(!new File(dir+"/base/gl2progs").exists()))
		getgl2progs(dir+"/base/");
        */

        m_gameLauncher.finish();
        m_gameLauncher.startActivity(new Intent(m_gameLauncher, Q3EMain.class));
    }

    public void getgl2progs(String destname)
    {
        try
        {
            byte[] buf = new byte[4096];
            ZipInputStream zipinputstream = null;
            InputStream bis;
            ZipEntry zipentry;

            bis = m_gameLauncher.getAssets().open("source/gl2progs.zip");
            zipinputstream = new ZipInputStream(bis);

            (new File(destname)).mkdirs();
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                String tmpname = zipentry.getName();

                String entryName = destname + tmpname;
                entryName = entryName.replace('/', File.separatorChar);
                entryName = entryName.replace('\\', File.separatorChar);

                int n;
                FileOutputStream fileoutputstream;
                File newFile = new File(entryName);
                if (zipentry.isDirectory())
                {
                    if (!newFile.mkdirs())
                    {
                    }
                    continue;
                }
                fileoutputstream = new FileOutputStream(entryName);
                while ((n = zipinputstream.read(buf, 0, 4096)) > -1)
                {
                    fileoutputstream.write(buf, 0, n);
                }
                fileoutputstream.close();
                zipinputstream.closeEntry();
            }

            zipinputstream.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
