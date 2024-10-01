package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.karin.idTech4Amm.lib.KCVar;
import com.n0n3m4.q3e.karin.KidTech4Command;

public class CVarSetting_radiogroup extends LinearLayout implements CVarSettingInterface
{
    private KCVar m_cvar;
    private TextView m_label;
    private RadioGroup m_radioGroup;

    public CVarSetting_radiogroup(Context context)
    {
        super(context);
        Setup();
    }

    private void Setup()
    {
        Context context = getContext();
        LayoutParams params;

        setOrientation(VERTICAL);
        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        m_label = new TextView(context);
        addView(m_label, params);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        m_radioGroup = new RadioGroup(context);
        addView(m_radioGroup, params);
    }

    @Override
    public void SetCVar(KCVar cvar)
    {
        Context context = m_radioGroup.getContext();
        this.m_cvar = cvar;
        m_label.setText(cvar.description);
        if(null == cvar.values)
            return;
        for (KCVar.Value value : cvar.values)
        {
            RadioButton radio = new RadioButton(context);
            radio.setText(value.value + ": " + value.desc);
            radio.setTag(value.value);
            LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
            m_radioGroup.addView(radio, params);
            if(cvar.defaultValue.equals(value.value))
                radio.setChecked(true);
        }
    }

    @Override
    public void RestoreCommand(String cmd)
    {
        String[] defaultValue = {m_cvar.defaultValue};
        String value = KidTech4Command.GetProp(cmd, m_cvar.name, defaultValue);
        for(int i = 0; i < m_radioGroup.getChildCount(); i++)
        {
            View child = m_radioGroup.getChildAt(i);
            if(!(child instanceof RadioButton))
                continue;
            RadioButton radio = (RadioButton)child;
            radio.setChecked(radio.getTag().equals(value));
        }
    }

    @Override
    public String DumpCommand(String cmd)
    {
        RadioButton current = null;
        for(int i = 0; i < m_radioGroup.getChildCount(); i++)
        {
            View child = m_radioGroup.getChildAt(i);
            if(!(child instanceof RadioButton))
                continue;
            RadioButton radio = (RadioButton)child;
            if(radio.isChecked())
            {
                current = radio;
                break;
            }
        }
        if(null != current)
            return KidTech4Command.SetProp(cmd, m_cvar.name, current.getTag());
        else
            return KidTech4Command.SetProp(cmd, m_cvar.name, m_cvar.defaultValue);
    }

    @Override
    public String RemoveCommand(String cmd)
    {
        return KidTech4Command.RemoveProp(cmd, m_cvar.name);
    }

    @Override
    public String ResetCommand(String cmd)
    {
        return KidTech4Command.SetProp(cmd, m_cvar.name, m_cvar.defaultValue);
    }

    @Override
    public void SetEnabled(boolean enabled)
    {
        m_radioGroup.setEnabled(enabled);

        for(int i = 0; i < m_radioGroup.getChildCount(); i++)
        {
            View child = m_radioGroup.getChildAt(i);
            child.setEnabled(enabled);
        }
    }
}
