package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.graphics.Color;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;

import com.karin.idTech4Amm.lib.KCVar;
import com.karin.idTech4Amm.sys.Theme;

public class CVarSettingField extends LinearLayout implements CompoundButton.OnCheckedChangeListener
{
    private static final boolean CONST_DEFAULT_ENABLED = false;
    private CVarSettingInterface m_field;
    private Switch m_checkBox; // CheckBox

    public CVarSettingField(Context context, String name, View view)
    {
        super(context);
        m_field = (CVarSettingInterface) view;
        Setup(name, view);
    }

    private void Setup(String name, View view)
    {
        Context context = getContext();
        LayoutParams params;

        setOrientation(VERTICAL);

        LinearLayout labelLayout = new LinearLayout(context);
        labelLayout.setOrientation(HORIZONTAL);
        m_checkBox = new Switch(context);
        params = new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        m_checkBox.setChecked(CONST_DEFAULT_ENABLED);
        m_field.SetEnabled(CONST_DEFAULT_ENABLED);
        m_checkBox.setOnCheckedChangeListener(this);
        labelLayout.addView(m_checkBox, params);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        TextView label = new TextView(context);
        label.setTextSize(16);
        label.setText(name);
        label.setTextColor(Theme.BlackColor(context));
        labelLayout.addView(label, params);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        addView(labelLayout, params);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        params.setMargins(24, params.topMargin, params.rightMargin, params.bottomMargin);
        addView(view, params);
    }

    public CVarSettingInterface CVarSettingUI()
    {
        return m_field;
    }

    public static CVarSettingField Create(Context context, KCVar cvar)
    {
        View view = CVarSettingUI.GenerateSettingUI(context, cvar);
        if(null == view)
            return null;
        return new CVarSettingField(context, cvar.name, view);
    }

    public boolean IsEnabled()
    {
        return m_checkBox.isChecked();
    }

    public void RestoreCommand(String cmd)
    {
        //if(IsEnabled())
            m_field.RestoreCommand(cmd);
    }

    public String DumpCommand(String cmd)
    {
        return(IsEnabled() ? m_field.DumpCommand(cmd) : cmd);
    }

    public String RemoveCommand(String cmd)
    {
        return(IsEnabled() ? m_field.RemoveCommand(cmd) : cmd);
    }

    public String ResetCommand(String cmd)
    {
        return(IsEnabled() ? m_field.ResetCommand(cmd) : cmd);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
        m_field.SetEnabled(isChecked);
    }
}
