package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;

public class MouseControl implements TouchListener
{
    private Q3EControlView view;
    private boolean alreadydown;
    private int lx;
    private int ly;
    private boolean isleftbutton;

    public MouseControl(Q3EControlView vw, boolean islmb)
    {
        view = vw;
        alreadydown = false;
        isleftbutton = islmb;
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        if (act == 1)
        {
            if (isleftbutton)
                Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MOUSE1, 0);//Can be sent twice, unsafe.
            alreadydown = true;
        }
        else
        {
            Q3EUtils.q3ei.callbackObj.sendMotionEvent(x - lx, y - ly);
        }
        lx = x;
        ly = y;

        if (act == -1)
        {
            if (isleftbutton)
                Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MOUSE1, 0);//Can be sent twice, unsafe.
            alreadydown = false;
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return !alreadydown;
    }
}
