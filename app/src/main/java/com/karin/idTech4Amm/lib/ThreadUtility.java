package com.karin.idTech4Amm.lib;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

public final class ThreadUtility
{
    public static Object Post(String name, Runnable runnable/* run on new thread */, Runnable andThen/* run on origin thread */)
    {
        HandlerThread handlerThread = new HandlerThread(name);
        handlerThread.start(); // start new thread
        final Handler newThreadHandler = new Handler(handlerThread.getLooper()); // new thread
        final Looper originThreadLooper = Looper.myLooper(); // current thread
        newThreadHandler.post(new Runnable()
        {
            @Override
            public void run()
            {
                runnable.run(); // runnable run on new thread
                Handler originThreadHandler = new Handler(originThreadLooper);
                originThreadHandler.post(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        handlerThread.quit(); // quit new thread
                        if(null != andThen)
                            andThen.run(); // andThen run on origin thread
                    }
                });
            }
        });
        return handlerThread;
    }

    public static Object Post(Runnable runnable, Runnable andThen)
    {
        return Post("ThreadUtility", runnable, andThen);
    }

    public static Object Post(String name, Runnable runnable)
    {
        return Post("ThreadUtility", runnable, null);
    }

    public static Object Post(Runnable runnable)
    {
        return Post(runnable, null);
    }

    public static void Quit(Object handler)
    {
        if(null == handler)
            return;
        HandlerThread handlerThread = (HandlerThread)handler;
        if(handlerThread.isAlive())
            handlerThread.quit();
        handlerThread.interrupt();
    }

    private ThreadUtility() {}
}
