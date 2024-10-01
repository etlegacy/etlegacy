package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.text.InputType;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.karin.idTech4Amm.lib.KCVar;
import com.n0n3m4.q3e.karin.KidTech4Command;

public class CVarSetting_edittext extends LinearLayout implements CVarSettingInterface
{
    private KCVar m_cvar;
    private TextView m_label;
    private EditText m_editText;

    public CVarSetting_edittext(Context context)
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
        m_editText = new EditText(context);
        addView(m_editText, params);
    }

    @Override
    public void SetCVar(KCVar cvar)
    {
        this.m_cvar = cvar;
        m_editText.setHint("Default is " + cvar.defaultValue);
        m_editText.setText(cvar.defaultValue);
        m_editText.setEms(10);
        m_editText.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI);
        m_label.setText(cvar.description);
    }

    @Override
    public void RestoreCommand(String cmd)
    {
        String[] defaultValue = {m_cvar.defaultValue};
        String value = KidTech4Command.GetProp(cmd, m_cvar.name, defaultValue);
        m_editText.setText(value);
    }

    public void SetIsInteger(boolean onlyPositive)
    {
        int flag = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
        if(!onlyPositive)
            flag |= InputType.TYPE_NUMBER_FLAG_SIGNED;
        m_editText.setInputType(flag);
    }

    public void SetIsFloat(boolean onlyPositive)
    {
        int flag = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_NUMBER_FLAG_DECIMAL;
        if(!onlyPositive)
            flag |= InputType.TYPE_NUMBER_FLAG_SIGNED;
        m_editText.setInputType(flag);
    }

    @Override
    public String DumpCommand(String cmd)
    {
        String s = m_editText.getText().toString();
        if(s.isEmpty())
            s = m_cvar.defaultValue;
        return KidTech4Command.SetProp(cmd, m_cvar.name, s);
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
        m_editText.setEnabled(enabled);
    }
}
