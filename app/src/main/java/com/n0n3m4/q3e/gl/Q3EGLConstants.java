package com.n0n3m4.q3e.gl;

import android.os.Build;

public final class Q3EGLConstants
{
    public static final int OPENGLES10 = 0x00010000;
    public static final int OPENGLES11 = 0x00010001;
    public static final int OPENGLES20 = 0x00020000; // Android 2.2 API 8
    public static final int OPENGLES30 = 0x00030000; // Android 4.3 API 18
    public static final int OPENGLES31 = 0x00030001; // Android 5.0 API 21
    public static final int OPENGLES32 = 0x00030002; // Android 7.0 API 24


    public static int GetPreferOpenGLESVersion()
    {
        return IsSupportOpenGLES3() ? OPENGLES30 : OPENGLES20;
    }

    public static boolean IsSupportOpenGLES3()
    {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2); // Android 4.3 Jelly bean 18
    }

    public static boolean IsSupportOpenGLES31()
    {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP); // Android 5.0 Lolipop 21
    }

    public static boolean IsSupportOpenGLES32()
    {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N); // Android 7.0 Nougat 24
    }

    private Q3EGLConstants() {}
}
