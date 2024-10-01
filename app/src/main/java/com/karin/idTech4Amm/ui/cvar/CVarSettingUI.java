package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.view.View;

import com.karin.idTech4Amm.lib.Utility;
import com.karin.idTech4Amm.lib.KCVar;

public final class CVarSettingUI
{
    public static final int TYPE_NONE = 0;
    public static final int TYPE_INPUT = 1;
    public static final int TYPE_INPUT_NUMBER = 2;
    public static final int TYPE_INPUT_DECIMAL = 3;
    public static final int TYPE_SELECTION = 4;
    public static final int TYPE_CHECK = 5;
    public static final int TYPE_SEEK = 6;

    public static int GetSettingUIType(KCVar cvar)
    {
        if(cvar.category == KCVar.CATEGORY_COMMAND)
            return TYPE_NONE;
        final String[] IgnoreCVars = {
                "harm_r_clearVertexBuffer",
                "harm_r_lightingModel",
                "harm_r_specularExponent",
                "harm_r_specularExponentBlinnPhong",
                "harm_r_specularExponentPBR",
                "harm_fs_gameLibPath",
                "r_maxFps",
                "r_useShadowMapping",
                "harm_r_stencilShadowTranslucent",
                "harm_fs_gameLibDir",
                "harm_r_stencilShadowAlpha",
                "harm_r_stencilShadowSoft",
                "harm_r_stencilShadowCombine",
                "harm_r_autoAspectRatio",
                "harm_r_shadowMapAlpha",
                "r_forceShadowMapsOnAlphaTestedSurfaces",
        };
        if(Utility.ArrayContains(IgnoreCVars, cvar.name))
            return TYPE_NONE;
        if(null != cvar.values)
            return TYPE_SELECTION;
        else if(KCVar.TYPE_BOOL.equals(cvar.type))
            return TYPE_CHECK;
        else if(KCVar.TYPE_INTEGER.equals(cvar.type))
            return TYPE_INPUT_NUMBER;
        else if(KCVar.TYPE_FLOAT.equals(cvar.type))
            return TYPE_INPUT_DECIMAL;
        else
            return TYPE_INPUT;
    }

    public static View GenerateSettingUI(Context context, KCVar cvar)
    {
        int type = GetSettingUIType(cvar);
        View view;
        switch (type)
        {
            case TYPE_CHECK:
                view = new CVarSetting_checkbox(context);
                break;
            case TYPE_INPUT:
                view = new CVarSetting_edittext(context);
                break;
            case TYPE_INPUT_DECIMAL: {
                CVarSetting_edittext edittext = new CVarSetting_edittext(context);
                edittext.SetIsFloat(cvar.HasFlag(KCVar.FLAG_POSITIVE));
                view = edittext;
            }
                break;
            case TYPE_INPUT_NUMBER: {
                CVarSetting_edittext edittext = new CVarSetting_edittext(context);
                edittext.SetIsInteger(cvar.HasFlag(KCVar.FLAG_POSITIVE));
                view = edittext;
            }
                break;
            case TYPE_SELECTION:
                view = new CVarSetting_radiogroup(context);
                break;
            case TYPE_NONE:
            default:
                return null;
        }
        CVarSettingInterface obj = (CVarSettingInterface) view;
        obj.SetCVar(cvar);
        return view;
    }

    private CVarSettingUI() {}
}
