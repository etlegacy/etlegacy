package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.karin.idTech4Amm.lib.KCVar;

public class CVarSettingSection extends LinearLayout
{
    private TextView m_label;
    private LinearLayout m_contents;

    public CVarSettingSection(Context context)
    {
        super(context);
        Setup();
    }

    private void Setup()
    {
        Context context = getContext();
        LinearLayout.LayoutParams params;
        setOrientation(VERTICAL);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        m_label = new TextView(context);
        m_label.setGravity(Gravity.CENTER_HORIZONTAL);
        m_label.setTextSize(20);
        addView(m_label, params);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        m_contents = new LinearLayout(context);
        m_contents.setOrientation(VERTICAL);
        addView(m_contents, params);
    }

    public int Load(KCVar.Group group)
    {
        Context context = getContext();
        m_label.setText(group.name);
        int res = 0;
        for (KCVar cvar : group.list)
        {
            View view = CVarSettingField.Create(context, cvar);
            if(null == view)
                continue;
            LinearLayout.LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
            m_contents.addView(view, params);
            res++;
        }
        return res;
    }

    public void RestoreCommand(String cmd)
    {
        for(int i = 0; i < m_contents.getChildCount(); i++)
        {
            View child = m_contents.getChildAt(i);
            if(!(child instanceof CVarSettingField))
                continue;
            ((CVarSettingField)child).RestoreCommand(cmd);
        }
    }

    public String DumpCommand(String cmd)
    {
        String newCmd = cmd;
        for(int i = 0; i < m_contents.getChildCount(); i++)
        {
            View child = m_contents.getChildAt(i);
            if(!(child instanceof CVarSettingField))
                continue;
            newCmd = ((CVarSettingField)child).DumpCommand(newCmd);
        }
        return newCmd;
    }

    public String RemoveCommand(String cmd)
    {
        String newCmd = cmd;
        for(int i = 0; i < m_contents.getChildCount(); i++)
        {
            View child = m_contents.getChildAt(i);
            if(!(child instanceof CVarSettingField))
                continue;
            newCmd = ((CVarSettingField)child).RemoveCommand(newCmd);
        }
        return newCmd;
    }

    public String ResetCommand(String cmd)
    {
        String newCmd = cmd;
        for(int i = 0; i < m_contents.getChildCount(); i++)
        {
            View child = m_contents.getChildAt(i);
            if(!(child instanceof CVarSettingField))
                continue;
            newCmd = ((CVarSettingField)child).ResetCommand(newCmd);
        }
        return newCmd;
    }
}
