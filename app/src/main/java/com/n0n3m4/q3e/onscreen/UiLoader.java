package com.n0n3m4.q3e.onscreen;

import android.content.SharedPreferences;
import android.graphics.Rect;
import android.preference.PreferenceManager;
import android.view.View;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

import javax.microedition.khronos.opengles.GL10;

public class UiLoader
{
    View ctx;
    public GL10 gl;
    int height;
    int width;
    //String filename;

    public static String[] defaults_table;

    public UiLoader(View cnt, GL10 gl10, int w, int h)//, String fname)
    {
        ctx = cnt;
        gl = gl10;
        width = w;
        height = h;
        //filename=fname;
        defaults_table = Q3EUtils.q3ei.defaults_table;

        //Set defaults table
    }

    public Object LoadElement(int id, boolean editMode)
    {
        SharedPreferences shp = PreferenceManager.getDefaultSharedPreferences(ctx.getContext());
        String tmp = shp.getString(Q3EPreference.pref_controlprefix + id, null);
        if (tmp == null) tmp = defaults_table[id];
        UiElement el = new UiElement(tmp);
        return LoadUiElement(id, el.cx, el.cy, el.size, el.alpha, editMode);
    }

    private Object LoadUiElement(int id, int cx, int cy, int size, int alpha, boolean editMode)
    {
        int key, key2, key3;
        switch (Q3EUtils.q3ei.type_table[id])
        {
            case Q3EGlobals.TYPE_BUTTON:
                int bh = size;
                if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL)
                    bh = size;
                else if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM)
                    bh = size;
                else if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_CENTER)
                    bh = size / 2;
                key = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4]);
                return new Button(ctx, gl, cx, cy, size, bh, Q3EUtils.q3ei.texture_table[id], key, Q3EUtils.q3ei.arg_table[id * 4 + 2], Q3EUtils.q3ei.arg_table[id * 4 + 1] == 1, (float) alpha / 100);
            case Q3EGlobals.TYPE_JOYSTICK: {
                int visibleMode = PreferenceManager.getDefaultSharedPreferences(ctx.getContext()).getInt(Q3EPreference.pref_harm_joystick_visible, Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ALWAYS);
                return new Joystick(ctx, gl, size, (float) alpha / 100, cx, cy, Q3EUtils.q3ei.joystick_release_range, Q3EUtils.q3ei.joystick_inner_dead_zone, Q3EUtils.q3ei.joystick_unfixed, editMode, visibleMode, Q3EUtils.q3ei.texture_table[id]);
            }
            case Q3EGlobals.TYPE_SLIDER:
                key = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4]);
                key2 = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4 + 1]);
                key3 = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4 + 2]);
                int sh = size;
                if (Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT || Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK)
                    sh = size / 2;
                /*else if (Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT)
                    sh = size;*/
                return new Slider(ctx, gl, cx, cy, size, sh, Q3EUtils.q3ei.texture_table[id], key, key2, key3, Q3EUtils.q3ei.arg_table[id * 4 + 3], (float) alpha / 100);
            case Q3EGlobals.TYPE_DISC:
            {
                String keysStr = PreferenceManager.getDefaultSharedPreferences(ctx.getContext()).getString(Q3EPreference.WEAPON_PANEL_KEYS, Q3EKeyCodes.K_WEAPONS_STR);
                char[] keys = null;
                if (null != keysStr && !keysStr.isEmpty())
                {
                    String[] arr = keysStr.split(",");
                    keys = new char[arr.length];
                    for (int i = 0; i < arr.length; i++)
                    {
                        keys[i] = arr[i].charAt(0);
                    }
                }
                return new Disc(ctx, gl, cx, cy, size, (float) alpha / 100, keys, Q3EUtils.q3ei.texture_table[id]);
            }
        }
        return null;
    }

    public boolean CheckVisible(int id)
    {
        SharedPreferences shp = PreferenceManager.getDefaultSharedPreferences(ctx.getContext());
        String tmp = shp.getString(Q3EPreference.pref_controlprefix + id, null);
        if (tmp == null) tmp = defaults_table[id];
        UiElement el = new UiElement(tmp);
        final Rect ScreenRect = new Rect(0, 0, width, height);
        Rect btnRect;
        switch (Q3EUtils.q3ei.type_table[id])
        {
            case Q3EGlobals.TYPE_BUTTON:
                int bh = el.size;
                if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL)
                    bh =  el.size;
                else if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM)
                    bh =  el.size;
                else if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_CENTER)
                    bh =  el.size / 2;
                btnRect = new Rect(-el.size / 2 + el.cx, -bh / 2 + el.cy, el.size / 2 + el.cx, bh / 2 + el.cy);
                return ScreenRect.intersect(btnRect);
            case Q3EGlobals.TYPE_JOYSTICK: {
                return true;
            }
            case Q3EGlobals.TYPE_SLIDER:
                int sh = el.size;
                if (Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT || Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK)
                    sh = el.size / 2;
                btnRect = new Rect(-el.size / 2 + el.cx, -sh / 2 + el.cy, el.size / 2 + el.cx, sh / 2 + el.cy);
                return ScreenRect.intersect(btnRect);
            case Q3EGlobals.TYPE_DISC:
            {
                int r = el.size * 2;
                btnRect = new Rect(-r / 2 + el.cx, -r / 2 + el.cy, r / 2 + el.cx, r / 2 + el.cy);
                return ScreenRect.intersect(btnRect);
            }
        }
        return false;
    }
}
