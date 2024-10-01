package com.karin.idTech4Amm.network;

import android.content.Context;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.q3e.Q3ELang;

import org.json.JSONObject;

public final class CheckUpdate
{
    public static final int CONST_INVALID_RELEASE = -1;

    private final Context m_context;
    public String error = "";
    public int release = CONST_INVALID_RELEASE;
    public String update;
    public String version;
    public String apk_url;
    public String changes;

    public CheckUpdate(Context context)
    {
        m_context = context;
    }

    public void CheckForUpdate_github(Runnable runnable)
    {
        Reset();
        String[] args = { "" };
        NetworkAccessManager.AsyncGet(Constants.CONST_CHECK_FOR_UPDATE_URL, new Runnable()
        {
            @Override
            public void run()
            {
                String text = args[0];
                if(null != text)
                {
                    if(!text.isEmpty())
                    {
                        try
                        {
                            JSONObject json = new JSONObject(text);
                            release = json.getInt("release");
                            update = json.getString("update");
                            version = json.getString("version");
                            apk_url = json.getString("apk_url");
                            changes = json.getString("changes");
                        }
                        catch (Exception e)
                        {
                            e.printStackTrace();
                        }
                    }
                    error = Q3ELang.tr(m_context, R.string.empty_response_data);
                }
                else
                    error = Q3ELang.tr(m_context, R.string.network_error);
                runnable.run();
            }
        }, args);
    }

    public void CheckForUpdate(Runnable runnable)
    {
        CheckForUpdate_github(runnable);
    }

    private void Reset()
    {
        error = "";
        release = CONST_INVALID_RELEASE;
        update = null;
        version = null;
        apk_url = null;
        changes = null;
    }
}
