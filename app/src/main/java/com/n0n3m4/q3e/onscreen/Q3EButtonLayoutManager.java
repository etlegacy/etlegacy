package com.n0n3m4.q3e.onscreen;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

public class Q3EButtonLayoutManager
{
    private final Activity m_context;
    private float m_scale;
    private boolean m_friendly;
    private int m_opacity;
    private boolean m_landscape;

    public Q3EButtonLayoutManager(Activity context)
    {
        this.m_context = context;
    }

    public Q3EButtonLayoutManager Scale(float scale)
    {
        if (scale <= 0.0f)
            scale = 1.0f;
        this.m_scale = scale;
        return this;
    }

    public Q3EButtonLayoutManager Friendly(boolean friendly)
    {
        this.m_friendly = friendly;
        return this;
    }

    public Q3EButtonLayoutManager Opacity(int opacity)
    {
        this.m_opacity = opacity;
        return this;
    }

    public Q3EButtonLayoutManager Landscape(boolean landscape)
    {
        this.m_landscape = landscape;
        return this;
    }

    private int Dip2px_s(int i)
    {
        final boolean NeedScale = m_scale > 0.0f && m_scale != 1.0f;
        int r = Q3EUtils.dip2px(m_context, i);
        if(NeedScale)
            r = Math.round((float)r * m_scale);
        return r;
    }

    private int Dip2px(int i)
    {
        return Q3EUtils.dip2px(m_context, i);
    }

    private int S(int i)
    {
        return Math.round((float)i * m_scale);
    }

    public String[] Make()
    {
        int safeInsetTop = Q3EUtils.GetEdgeHeight(m_context, m_landscape);
        int safeInsetBottom = Q3EUtils.GetEndEdgeHeight(m_context, m_landscape);
        int[] fullSize = Q3EUtils.GetFullScreenSize(m_context);
        int[] size = Q3EUtils.GetNormalScreenSize(m_context);
        if(m_landscape)
        {
            int tmp = fullSize[0];
            fullSize[0] = fullSize[1];
            fullSize[1] = tmp;

            tmp = size[0];
            size[0] = size[1];
            size[1] = tmp;
        }
        int navBarHeight = fullSize[1] - size[1] - safeInsetTop - safeInsetBottom;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        boolean hideNav = preferences.getBoolean(Q3EPreference.HIDE_NAVIGATION_BAR, true);
        boolean coverEdges = preferences.getBoolean(Q3EPreference.COVER_EDGES, true);
        int w, h;
        int start = 0;
        if (m_friendly)
        {
            w = fullSize[0];
            h = fullSize[1];
            h -= navBarHeight;
            if (coverEdges)
                start = safeInsetTop;
            else
                h -= (safeInsetTop + safeInsetBottom);
        }
        else
        {
            w = fullSize[0];
            h = fullSize[1];
            if (!hideNav)
                h -= navBarHeight;
            if (!coverEdges)
                h -= (safeInsetTop + safeInsetBottom);
        }
        final int Width = Math.max(w, h);
        final int Height = Math.min(w, h);

        final int LargeButton_Width = Dip2px_s(60); //k 64 // button width/height // 75
        final int MediumButton_Width = Dip2px_s(55); //k 60
        final int SmallButton_Width = Dip2px_s(50); //k 56
        // int rightoffset = 0; // LargeButton_Width * 3 / 4;
        final int Sliders_Width = Dip2px_s(120); // slider width
        final int Joystick_Radius = Dip2px_s(75); // half width
        final int Attack_Width = Dip2px_s(80); //k 100
        final int Panel_Radius = Dip2px_s(25); //k 24
        final int Crouch_Width = Dip2px_s(70); //k 80
        final int Horizontal_Space = Dip2px_s(5); //k 4
        final int Vertical_Space = Dip2px_s(5); //k 2
        final int Attack_right_Margin = Dip2px_s(40); //k 20
        final int Attack_Bottom_Margin = Dip2px_s(40); //k 20
        final int JoyStick_Left_Margin = Dip2px_s(30); //k 30
        final int JoyStick_Bottom_Margin = Dip2px_s(40); //k 30
        final int Alpha = m_opacity;

        Q3EButtonGeometry[] layouts = Q3EButtonGeometry.Alloc(Q3EGlobals.UI_SIZE);
        
        layouts[Q3EGlobals.UI_JOYSTICK].Set(start + Joystick_Radius + JoyStick_Left_Margin, Height - Joystick_Radius - JoyStick_Bottom_Margin, Joystick_Radius, Alpha);
        layouts[Q3EGlobals.UI_SHOOT].Set(Width - Attack_Width / 2 - Crouch_Width / 2 - Attack_right_Margin, Height - Attack_Width / 2 - Crouch_Width / 2 - Attack_Bottom_Margin, Attack_Width, Alpha);

        layouts[Q3EGlobals.UI_SAVE].Set(start + Sliders_Width / 2, Sliders_Width / 2, Sliders_Width, Alpha);
        layouts[Q3EGlobals.UI_RELOADBAR].Set(Width - Sliders_Width / 2, Sliders_Width / 4, Sliders_Width, Alpha);

        layouts[Q3EGlobals.UI_KBD].Set(start + Sliders_Width + SmallButton_Width / 2 + Horizontal_Space, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_CONSOLE].Set(start + Sliders_Width / 2 + SmallButton_Width / 2 + Horizontal_Space, Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_JUMP].Set(Width - LargeButton_Width / 2, layouts[Q3EGlobals.UI_SHOOT].y - LargeButton_Width / 2 - Attack_Width / 2 - Horizontal_Space, LargeButton_Width, Alpha);
        layouts[Q3EGlobals.UI_CROUCH].Set(Width - Crouch_Width / 2, Height - Crouch_Width / 2, Crouch_Width, Alpha);

        layouts[Q3EGlobals.UI_PDA].Set(start + Sliders_Width + SmallButton_Width / 2 + SmallButton_Width + Horizontal_Space * 2, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_SCORE].Set(start + Sliders_Width + SmallButton_Width / 2 + SmallButton_Width * 2 + Horizontal_Space * 3, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        int bottomLineRight = layouts[Q3EGlobals.UI_SHOOT].x - Attack_right_Margin + Horizontal_Space * 2;
        layouts[Q3EGlobals.UI_ZOOM].Set(bottomLineRight - MediumButton_Width - Horizontal_Space, Height - MediumButton_Width / 2, MediumButton_Width, Alpha);
        layouts[Q3EGlobals.UI_FLASHLIGHT].Set(bottomLineRight - MediumButton_Width * 2 - Horizontal_Space * 2, Height - MediumButton_Width / 2, MediumButton_Width, Alpha);
        layouts[Q3EGlobals.UI_RUN].Set(bottomLineRight - MediumButton_Width * 3 - Horizontal_Space * 3, Height - MediumButton_Width / 2, MediumButton_Width, Alpha);

        layouts[Q3EGlobals.UI_INTERACT].Set(Width - Sliders_Width - SmallButton_Width / 2 - Horizontal_Space, SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_1].Set(Width - SmallButton_Width / 2 - SmallButton_Width * 2 - Horizontal_Space * 2, Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_2].Set(Width - SmallButton_Width / 2 - SmallButton_Width - Horizontal_Space, Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);
        layouts[Q3EGlobals.UI_3].Set(Width - SmallButton_Width / 2, Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, SmallButton_Width, Alpha);

        layouts[Q3EGlobals.UI_WEAPON_PANEL].Set(Width - Math.max(Sliders_Width + MediumButton_Width / 2 + Horizontal_Space, SmallButton_Width * 3 + Horizontal_Space * 2) - Panel_Radius * 5 / 2 - Horizontal_Space * 4, Panel_Radius + Sliders_Width / 2 + SmallButton_Width / 2 + Vertical_Space, Panel_Radius, Alpha);

        for (int i = Q3EGlobals.UI_0; i < Q3EGlobals.UI_SIZE; i++)
            layouts[i].Set(SmallButton_Width / 2 + SmallButton_Width * (i - Q3EGlobals.UI_0), Height + SmallButton_Width / 2, SmallButton_Width, Alpha);

        return Q3EButtonGeometry.ToStrings(layouts);
    }

    public static String[] GetDefaultLayout(Activity context, boolean friendly, float scale, int opacity, boolean landscape)
    {
        Q3EButtonLayoutManager manager = new Q3EButtonLayoutManager(context);
        return manager.Scale(scale)
                .Friendly(friendly)
                .Opacity(opacity)
                .Landscape(landscape)
                .Make();
    }

    public static String[] GetDefaultLayout_DIII4A(Activity context, boolean friendly, float scale, int opacity, boolean landscape)
    {
        if (scale <= 0.0f)
            scale = 1.0f;

        int safeInsetTop = Q3EUtils.GetEdgeHeight(context, landscape);
        int safeInsetBottom = Q3EUtils.GetEndEdgeHeight(context, landscape);
        int[] fullSize = Q3EUtils.GetFullScreenSize(context);
        int[] size = Q3EUtils.GetNormalScreenSize(context);
        if(landscape)
        {
            int tmp = fullSize[0];
            fullSize[0] = fullSize[1];
            fullSize[1] = tmp;

            tmp = size[0];
            size[0] = size[1];
            size[1] = tmp;
        }
        int navBarHeight = fullSize[1] - size[1] - safeInsetTop - safeInsetBottom;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        boolean hideNav = preferences.getBoolean(Q3EPreference.HIDE_NAVIGATION_BAR, true);
        boolean coverEdges = preferences.getBoolean(Q3EPreference.COVER_EDGES, true);
        int w, h;
        int start = 0;
        if (friendly)
        {
            w = fullSize[0];
            h = fullSize[1];
            h -= navBarHeight;
            if (coverEdges)
                start = safeInsetTop;
            else
                h -= (safeInsetTop + safeInsetBottom);
        }
        else
        {
            w = fullSize[0];
            h = fullSize[1];
            if (!hideNav)
                h -= navBarHeight;
            if (!coverEdges)
                h -= (safeInsetTop + safeInsetBottom);
        }
        int width = Math.max(w, h);
        int height = Math.min(w, h);

        final boolean needScale = scale > 0.0f && scale != 1.0f;
        int r = Q3EUtils.dip2px(context, 75);
        if (needScale)
            r = Math.round((float) r * scale);
        int rightoffset = r * 3 / 4;
        int sliders_width = Q3EUtils.dip2px(context, 125);
        if (needScale)
            sliders_width = Math.round((float) sliders_width * scale);
        final int alpha = opacity;

        String[] defaults_table = new String[Q3EGlobals.UI_SIZE];
        defaults_table[Q3EGlobals.UI_JOYSTICK] = (start + r * 4 / 3) + " " + (height - r * 4 / 3) + " " + r + " " + alpha;
        defaults_table[Q3EGlobals.UI_SHOOT] = (width - r / 2 - rightoffset) + " " + (height - r / 2 - rightoffset) + " " + r * 3 / 2 + " " + alpha;
        defaults_table[Q3EGlobals.UI_JUMP] = (width - r / 2) + " " + (height - r - 2 * rightoffset) + " " + r + " " + alpha;
        defaults_table[Q3EGlobals.UI_CROUCH] = (width - r / 2) + " " + (height - r / 2) + " " + r + " " + alpha;
        defaults_table[Q3EGlobals.UI_RELOADBAR] = (width - sliders_width / 2 - rightoffset / 3) + " " + (sliders_width * 3 / 8) + " " + sliders_width + " " + alpha;
        defaults_table[Q3EGlobals.UI_PDA] = (width - r - 2 * rightoffset) + " " + (height - r / 2) + " " + r + " " + alpha;
        defaults_table[Q3EGlobals.UI_FLASHLIGHT] = (width - r / 2 - 4 * rightoffset) + " " + (height - r / 2) + " " + r + " " + alpha;
        defaults_table[Q3EGlobals.UI_SAVE] = (start + sliders_width / 2) + " " + sliders_width / 2 + " " + sliders_width + " " + alpha;

        for (int i = Q3EGlobals.UI_SCORE; i < Q3EGlobals.UI_SIZE; i++)
            defaults_table[i] = (r / 2 + r * (i - Q3EGlobals.UI_SAVE - 1)) + " " + (height + r / 2) + " " + r + " " + alpha;

        defaults_table[Q3EGlobals.UI_WEAPON_PANEL] = (width - sliders_width - r - rightoffset) + " " + (r) + " " + (r / 3) + " " + alpha;

        //k
        final int sr = r / 6 * 5;
        defaults_table[Q3EGlobals.UI_1] = String.format("%d %d %d %d", width - sr / 2 - sr * 2, (sliders_width * 5 / 8 + sr / 2), sr, alpha);
        defaults_table[Q3EGlobals.UI_2] = String.format("%d %d %d %d", width - sr / 2 - sr, (sliders_width * 5 / 8 + sr / 2), sr, alpha);
        defaults_table[Q3EGlobals.UI_3] = String.format("%d %d %d %d", width - sr / 2, (sliders_width * 5 / 8 + sr / 2), sr, alpha);
        defaults_table[Q3EGlobals.UI_KBD] = String.format("%d %d %d %d", start + sliders_width + sr / 2, sr / 2, sr, alpha);
        defaults_table[Q3EGlobals.UI_CONSOLE] = String.format("%d %d %d %d", start + sliders_width / 2 + sr / 2, sliders_width / 2 + sr / 2, sr, alpha);

        return defaults_table;
    }
}
