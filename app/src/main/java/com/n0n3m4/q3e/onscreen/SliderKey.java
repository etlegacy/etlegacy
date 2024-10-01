package com.n0n3m4.q3e.onscreen;

public class SliderKey implements OnScreenKey
{
    public int left_key;
    public int middle_key;
    public int right_key;
    public int style;


    @Override
    public int[] ToArray()
    {
        return new int[] {
                left_key, middle_key, right_key, style
        };
    }
}
