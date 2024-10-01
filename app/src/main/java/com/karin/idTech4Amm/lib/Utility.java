package com.karin.idTech4Amm.lib;

/**
 * Common utility
 */
public final class Utility
{
    private Utility() {}
    
    public static boolean MASK(int a, int b)
    {
        return (a & b) != 0;
    }
    
    public static int BIT(int a, int b)
    {
        return a << b;
    }

    public static int ArrayIndexOf(int[] arr, int target)
    {
        for(int i = 0; i < arr.length; i++)
        {
            if(arr[i] == target)
                return i;
        }
        return 0;
    }

    public static int ArrayIndexOf(Object[] arr, Object target)
    {
        for(int i = 0; i < arr.length; i++)
        {
            if(target.equals(arr[i]))
                return i;
        }
        return -1;
    }

    public static boolean ArrayContains(Object[] arr, Object target)
    {
        return ArrayIndexOf(arr, target) >= 0;
    }

    public static int Step(int a, int step)
    {
        return (int)Math.round((float)a / (float)step) * step;
    }
}
