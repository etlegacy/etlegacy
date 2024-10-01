package com.karin.idTech4Amm.ui.cvar;

import com.karin.idTech4Amm.lib.KCVar;

public interface CVarSettingInterface
{
    public void SetCVar(KCVar cvar);
    public void RestoreCommand(String cmd);
    public String DumpCommand(String cmd);
    public String RemoveCommand(String cmd);
    public String ResetCommand(String cmd);
    public void SetEnabled(boolean enabled);
}
