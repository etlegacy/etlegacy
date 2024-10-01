package com.n0n3m4.q3e.gl;

import android.opengl.GLSurfaceView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;

public class Q3EConfigChooser implements GLSurfaceView.EGLConfigChooser
{
    int r;
    int g;
    int b;
    int a;
    int msaa;
    boolean gl2;
    EGL10 eglcmp;
    EGLDisplay dspcmp;

    public Q3EConfigChooser(int inr, int ing, int inb, int ina, int inmsaa, boolean gles2)
    {
        r = inr;
        g = ing;
        b = inb;
        a = ina;
        msaa = inmsaa;
        gl2 = gles2;
    }

    /*EGLConfig[] configs=new EGLConfig[1000];
     int numconfigs=0;
     */
    class comprtr implements Comparator<EGLConfig>
    {
        @Override
        public int compare(EGLConfig lhs, EGLConfig rhs)
        {
            int[] tmp = new int[1];
            int lr, lg, lb, la, ld, ls;
            int rr, rg, rb, ra, rd, rs;
            int rat1, rat2;
            eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_RED_SIZE, tmp);
            lr = tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_GREEN_SIZE, tmp);
            lg = tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_BLUE_SIZE, tmp);
            lb = tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_ALPHA_SIZE, tmp);
            la = tmp[0];
            //eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_DEPTH_SIZE, tmp);ld=tmp[0];
            //eglcmp.eglGetConfigAttrib(dspcmp, lhs, EGL10.EGL_STENCIL_SIZE, tmp);ls=tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_RED_SIZE, tmp);
            rr = tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_GREEN_SIZE, tmp);
            rg = tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_BLUE_SIZE, tmp);
            rb = tmp[0];
            eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_ALPHA_SIZE, tmp);
            ra = tmp[0];
            //eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_DEPTH_SIZE, tmp);rd=tmp[0];
            //eglcmp.eglGetConfigAttrib(dspcmp, rhs, EGL10.EGL_STENCIL_SIZE, tmp);rs=tmp[0];
            rat1 = (Math.abs(lr - r) + Math.abs(lg - g) + Math.abs(lb - b));//*1000000-(ld*10000+la*100+ls);
            rat2 = (Math.abs(rr - r) + Math.abs(rg - g) + Math.abs(rb - b));//*1000000-(rd*10000+ra*100+rs);
            return Integer.valueOf(rat1).compareTo(Integer.valueOf(rat2));
        }
    }

    public int[] intListToArr(ArrayList<Integer> integers)
    {
        int[] ret = new int[integers.size()];
        Iterator<Integer> iterator = integers.iterator();
        for (int i = 0; i < ret.length; i++)
        {
            ret[i] = iterator.next().intValue();
        }
        return ret;
    }

    @Override
    public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display)
    {
        dspcmp = display;
        eglcmp = egl;

        int[] tmp = new int[1];
        ArrayList<Integer> alst = new ArrayList<Integer>(0);
        alst.add(EGL10.EGL_SAMPLE_BUFFERS);
        alst.add((msaa > 0) ? 1 : 0);
        alst.add(EGL10.EGL_SAMPLES);
        alst.add(msaa);

        //TODO tegra zbuf
        //alst.add(0x30E2);alst.add(0x30E3);
        alst.add(EGL10.EGL_RED_SIZE);
        alst.add(r);
        alst.add(EGL10.EGL_GREEN_SIZE);
        alst.add(g);
        alst.add(EGL10.EGL_BLUE_SIZE);
        alst.add(b);
        alst.add(EGL10.EGL_ALPHA_SIZE);
        alst.add(a);
        if (gl2)
        {
            alst.add(EGL10.EGL_RENDERABLE_TYPE);
            alst.add(4);
        }
        //k alst.add(EGL10.EGL_DEPTH_SIZE);alst.add(32);
        alst.add(EGL10.EGL_DEPTH_SIZE);
        alst.add(r == 8 ? 32 : 16);
        alst.add(EGL10.EGL_STENCIL_SIZE);
        alst.add(8);
        alst.add(EGL10.EGL_NONE);
        int[] pararr = intListToArr(alst);
        EGLConfig[] configs = new EGLConfig[1000];
        while (tmp[0] == 0)
        {
            egl.eglChooseConfig(display, pararr, configs, 1000, tmp);
            pararr[pararr.length - 4] -= 4;
            if (pararr[pararr.length - 4] < 0)
            {
                pararr[pararr.length - 4] = 32;
                pararr[pararr.length - 2] -= 4;
                if (pararr[pararr.length - 2] < 0)
                {
                    if (pararr[0] != 0x30E0)
                    {
                        pararr[0] = 0x30E0;
                        pararr[2] = 0x30E1;
                        pararr[pararr.length - 4] = 32;
                        pararr[pararr.length - 2] = 8;
                    } else
                    {
                        //LOLWUT?! Let's crash.
                        return null;
                    }
                }
            }
        }
        //numconfigs=tmp[0];
        Arrays.sort(configs, 0, tmp[0], new comprtr());
        return configs[0];
    }
}
