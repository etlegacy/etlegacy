package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.Arrays;

public class MouseControl implements TouchListener
{
    private Q3EControlView view;
    private int lx;
    private int ly;
    private boolean isleftbutton;
    private int alreadydown_main;
    private int alreadydown_second;

    public MouseControl(Q3EControlView vw, boolean islmb)
    {
        view = vw;
        alreadydown_main = -1;
        alreadydown_second = -1;
        isleftbutton = islmb;
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act, int id)
    {
        if (act == 1)
        {
            if(alreadydown_main >= 0 && (alreadydown_second >= 0 || !isleftbutton))
                return true;

            if(alreadydown_main < 0)
            {
                alreadydown_main = id;
                lx = x;
                ly = y;
            }
            else
            {
                alreadydown_second = id;
                Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, Q3EKeyCodes.KeyCodes.K_MOUSE1, 0);//Can be sent twice, unsafe.
            }
        }
        else if (act == -1)
        {
            if(alreadydown_main == id)
            {
                alreadydown_main = -1;
                lx = 0;
                ly = 0;
            }
            else if(alreadydown_second == id)
            {
                alreadydown_second = -1;
                Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, Q3EKeyCodes.KeyCodes.K_MOUSE1, 0);//Can be sent twice, unsafe.
            }
        }
        else
        {
            if(alreadydown_main == id)
            {
                Q3EUtils.q3ei.callbackObj.sendMotionEvent(x - lx, y - ly);
                lx = x;
                ly = y;
            }
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return true;
    }

    @Override
    public boolean SupportMultiTouch()
    {
        return isleftbutton;
    }
}
