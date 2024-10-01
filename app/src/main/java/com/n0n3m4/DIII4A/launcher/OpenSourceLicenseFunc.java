package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.TextView;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.network.NetworkAccessManager;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.io.InputStream;

public final class OpenSourceLicenseFunc extends GameLauncherFunc
{
    private String m_text = "";
    private AlertDialog m_dialog;

    public OpenSourceLicenseFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
        m_text = GetLicenseInAPK();
        if(KStr.IsBlank(m_text))
            m_text = Q3ELang.tr(m_gameLauncher, R.string.get_open_source_license);

    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
        String[] args = { "" };
        NetworkAccessManager.AsyncGet(Constants.CONST_LICENSE_URL, new Runnable() {
            @Override
            public void run() {
                m_text = args[0];
                if(null == m_text)
                    m_text = Q3ELang.tr(m_gameLauncher, R.string.network_error);
                else if(m_text.isEmpty())
                    m_text = Q3ELang.tr(m_gameLauncher, R.string.empty_response_data);
                if(null != m_dialog)
                {
                    TextView message = m_dialog.findViewById(android.R.id.message);
                    if(null != message)
                        message.setText(m_text);
                }
            }
        }, args);
    }

    private String GetLicenseInAPK()
    {
        InputStream is = null;
        try
        {
            is = m_gameLauncher.getAssets().open("source/LICENSE");
            return Q3EUtils.Read(is);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return "";
        }
        finally
        {
            FileUtility.CloseStream(is);
        }
    }

    public void run()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.open_source_license);
        builder.setMessage(m_text);
        builder.setPositiveButton(R.string.ok, new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        m_dialog = builder.create();
        m_dialog.setOnDismissListener(new DialogInterface.OnDismissListener()
        {
            @Override
            public void onDismiss(DialogInterface dialog)
            {
                m_dialog = null;
            }
        });
        m_dialog.show();
    }
}
