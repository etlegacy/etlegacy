package com.n0n3m4.q3e.onscreen;

public interface TouchListener
{
    public static final int ACT_PRESS = 1;
    public static final int ACT_MOTION = 0;
    public static final int ACT_RELEASE = -1;

    public abstract boolean onTouchEvent(int x, int y, int act, int id);

    public abstract boolean isInside(int x, int y);

    public abstract boolean SupportMultiTouch();
}
