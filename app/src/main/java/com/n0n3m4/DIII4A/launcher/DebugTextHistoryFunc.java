package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.misc.TextHelper;
import com.n0n3m4.DIII4A.GameLauncher;

import java.util.LinkedList;

public final class DebugTextHistoryFunc extends GameLauncherFunc
{
    private static LinkedList<String> m_debugTextHistory = null;
    private static boolean m_revTextHistory = true;

    public DebugTextHistoryFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle("Debug text history")
                .setMessage(MakeDebugTextHistoryText(m_revTextHistory))
                .setPositiveButton(R.string.ok, null)
                .setNegativeButton(R.string.clear, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int p)
                    {
                        if(null != m_debugTextHistory)
                            m_debugTextHistory.clear();
                        ((AlertDialog)dialog).setMessage("");
                    }
                })
                .setNeutralButton("Rev", null)
        ;
        AlertDialog dialog = builder.create();
        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(final DialogInterface dialog) {
                ((AlertDialog)dialog).getButton(DialogInterface.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        m_revTextHistory = !m_revTextHistory;
                        ((AlertDialog)dialog).setMessage(MakeDebugTextHistoryText(m_revTextHistory));
                    }
                });
            }
        });
        dialog.show();
    }

    private CharSequence MakeDebugTextHistoryText(Boolean rev)
    {
        if(null == m_debugTextHistory)
            return "<empty>";

        StringBuilder sb = new StringBuilder();
        final String endl = TextHelper.GetDialogMessageEndl();
        for(int i = 0; i < m_debugTextHistory.size(); i++)
        {
            sb.append("[");
            String str;
            if(rev)
            {
                sb.append(m_debugTextHistory.size() - i);
                str = m_debugTextHistory.get(m_debugTextHistory.size() - i - 1);
            }
            else
            {
                sb.append(i + 1);
                str = m_debugTextHistory.get(i);
            }
            sb.append("]: ").append(str).append(endl);
        }
        return TextHelper.GetDialogMessage(sb.toString());
    }

    public static void DebugText(Context context, Object format, Object...args)
    {
        String str;
        if(null == format)
            str = "NULL";
        else if(format instanceof String)
            str = String.format((String)format, args);
        else
            str = format.toString();
        Log.e("TAG xxxxx", str);
        Toast.makeText(context, str, Toast.LENGTH_SHORT).show();
        if(null == m_debugTextHistory)
            m_debugTextHistory = new LinkedList<>();
        m_debugTextHistory.add(str);
    }
}
