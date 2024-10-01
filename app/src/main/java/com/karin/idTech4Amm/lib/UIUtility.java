package com.karin.idTech4Amm.lib;

import android.text.InputType;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import java.util.ArrayList;
import java.util.List;

public final class UIUtility
{
    public static void EditText__SetReadOnly(EditText editText, boolean readonly, int inputTypeIfEditable)
    {
        // if(false) editText.setEnabled(!readonly); else
        {
            int inputType;
            if(readonly)
                inputType = InputType.TYPE_NULL;
            else
                inputType = inputTypeIfEditable;
            editText.setInputType(inputType);
            editText.setFocusable(!readonly);
            editText.setFocusableInTouchMode(!readonly);
        }

        editText.setSingleLine(readonly);
    }

    public static <T> List<T> GetChildren(ViewGroup view, Class<T> clazz)
    {
        List<T> res = new ArrayList<>();
        for (int i = 0; i < view.getChildCount(); i++)
        {
            View child = view.getChildAt(i);
            if(clazz.isAssignableFrom(child.getClass()))
            {
                res.add((T)child);
            }
            else if(child instanceof ViewGroup)
            {
                List<T> l = GetChildren((ViewGroup)child, clazz);
                res.addAll(l);
            }
        }
        return res;
    }

    private UIUtility() {}
}
