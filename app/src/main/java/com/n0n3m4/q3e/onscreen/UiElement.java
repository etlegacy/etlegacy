package com.n0n3m4.q3e.onscreen;

public class UiElement
{
    int cx;
    int cy;
    int size;
    int alpha;

    public UiElement(int incx, int incy, int insize, int inalpha)
    {
        cx = incx;
        cy = incy;
        size = insize;
        alpha = inalpha;
    }

    public UiElement(String str)
    {
        LoadFromString(str);
    }

    public void LoadFromString(String str)
    {
        String[] spl = str.split(" ");
        cx = Integer.parseInt(spl[0]);
        cy = Integer.parseInt(spl[1]);
        size = Integer.parseInt(spl[2]);
        alpha = Integer.parseInt(spl[3]);
    }

    public String SaveToString()
    {
        return cx + " " + cy + " " + size + " " + alpha;
    }
}
