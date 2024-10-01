package com.n0n3m4.q3e.onscreen;

import android.view.MotionEvent;

public class Finger
{
    public TouchListener target;
    public int id;

    public Finger(TouchListener tgt, int myid)
    {
        id = myid;
        target = tgt;
    }

    public boolean onTouchEvent(MotionEvent event)
    {
        int act = 0;
        if (event.getPointerId(event.getActionIndex()) == id)
        {
            final int actionMasked = event.getActionMasked();
            if ((actionMasked == MotionEvent.ACTION_DOWN) || (actionMasked == MotionEvent.ACTION_POINTER_DOWN))
                act = 1;
            else if ((actionMasked == MotionEvent.ACTION_UP) || (actionMasked == MotionEvent.ACTION_POINTER_UP) || (actionMasked == MotionEvent.ACTION_CANCEL))
                act = -1;
        }
        final int pointerIndex = event.findPointerIndex(id);
        if(pointerIndex != -1)
            return target.onTouchEvent((int) event.getX(pointerIndex), (int) event.getY(pointerIndex), act, id);
        else
            return false;
    }
}
