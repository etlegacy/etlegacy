package com.n0n3m4.q3e.onscreen;

public class FingerUi extends Finger
{
    public int lastx;
    public int lasty;
    public boolean movd;

    public FingerUi(TouchListener tgt, int myid)
    {
        super(tgt, myid);
    }
}
