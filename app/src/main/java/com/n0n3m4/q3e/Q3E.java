package com.n0n3m4.q3e;

import android.view.Surface;
import android.util.Log;
import android.os.Build;

public final class Q3E
{
    public static Q3EMain activity;
    public static Surface surface;

    public static Q3EView        gameView;
    public static Q3EControlView controlView;

    public static void Finish()
    {
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Finish activity......");
        if(null != activity && !activity.isFinishing())
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
                activity.finishAffinity();
            else
                activity.finish();
        }
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Finish activity done");
        System.exit(0);
    }
}
