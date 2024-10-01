package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.ui.cvar.CVarSettingWidget;
import com.n0n3m4.DIII4A.GameLauncher;

public final class CVarEditorFunc extends GameLauncherFunc
{
    private String m_game;
    private String m_baseCommand;
    private String m_command;

    public CVarEditorFunc(GameLauncher gameLauncher, Runnable callback)
    {
        super(gameLauncher, callback);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        m_game = data.getString("game");
        m_command = data.getString("command");
        m_baseCommand = data.getString("baseCommand");

        run();
    }

    public void run()
    {
        CVarSettingWidget widget = new CVarSettingWidget(m_gameLauncher);
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.cvar_editor)
                .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        SetCmdText(widget.DumpCommand(GetCmdText()));
                    }
                })
                .setNegativeButton(R.string.remove, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        SetCmdText(m_baseCommand);
                    }
                })
                .setNeutralButton(R.string.reset, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        SetCmdText(widget.ResetCommand(GetCmdText()));
                    }
                })
        ;
        widget.SetGame(m_game);
        widget.RestoreCommand(GetCmdText());
        builder.setView(widget);
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private String GetCmdText()
    {
        return m_command;
    }

    private void SetCmdText(String str)
    {
        SetResult(str);
        Callback();
    }
}
