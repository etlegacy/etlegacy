package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3EGlobals;

public class ButtonKey implements OnScreenKey
{
    public int key;
    public boolean hold;
    public int style;

    @Override
    public int[] ToArray()
    {
        return new int[] {
                key, hold ? Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD : Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD, style, 0
        };
    }
}
