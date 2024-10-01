package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;

import com.karin.idTech4Amm.lib.KCVar;
import com.karin.idTech4Amm.lib.KCVarSystem;

import java.util.List;

public class CVarSettingWidget extends ScrollView
{
    private LinearLayout m_mainLayout;

    public CVarSettingWidget(Context context)
    {
        super(context);
        Setup();
    }

    private void Setup()
    {
        m_mainLayout = new LinearLayout(getContext());
        m_mainLayout.setOrientation(LinearLayout.VERTICAL);
        ScrollView.LayoutParams params = new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        setLayoutParams(params);
        params = new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        params.setMargins(10, 10, 10, 10);
        addView(m_mainLayout, params);
    }

    public void SetGame(String game)
    {
        //Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "SetGame: " + game);
        SetCVarGroupList(KCVarSystem.Match(game));
    }

    public void SetCVarGroupList(List<KCVar.Group> groups)
    {
        Context context = getContext();
        for (KCVar.Group group : groups)
        {
            CVarSettingSection settingSection = new CVarSettingSection(context);
            if(settingSection.Load(group) == 0)
                continue;
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
            m_mainLayout.addView(settingSection, params);
        }
    }

    public void RestoreCommand(String cmd)
    {
        for(int i = 0; i < m_mainLayout.getChildCount(); i++)
        {
            View child = m_mainLayout.getChildAt(i);
            if(!(child instanceof CVarSettingSection))
                continue;
            ((CVarSettingSection)child).RestoreCommand(cmd);
        }
    }

    public String DumpCommand(String cmd)
    {
        String newCmd = cmd;
        for(int i = 0; i < m_mainLayout.getChildCount(); i++)
        {
            View child = m_mainLayout.getChildAt(i);
            if(!(child instanceof CVarSettingSection))
                continue;
            newCmd = ((CVarSettingSection)child).DumpCommand(newCmd);
        }
        return newCmd;
    }

    public String RemoveCommand(String cmd)
    {
        String newCmd = cmd;
        for(int i = 0; i < m_mainLayout.getChildCount(); i++)
        {
            View child = m_mainLayout.getChildAt(i);
            if(!(child instanceof CVarSettingSection))
                continue;
            newCmd = ((CVarSettingSection)child).RemoveCommand(newCmd);
        }
        return newCmd;
    }

    public String ResetCommand(String cmd)
    {
        String newCmd = cmd;
        for(int i = 0; i < m_mainLayout.getChildCount(); i++)
        {
            View child = m_mainLayout.getChildAt(i);
            if(!(child instanceof CVarSettingSection))
                continue;
            newCmd = ((CVarSettingSection)child).ResetCommand(newCmd);
        }
        return newCmd;
    }
}
