package com.n0n3m4.q3e.onscreen;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;
import android.view.View;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGL;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Arrays;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Slider extends Paintable implements TouchListener
{
    private static final int SPLIT_NONE = 0;
    private static final int SPLIT_DOWN_RIGHT = 1;
    private static final int SPLIT_LEFT_RIGHT = 2;

    public View view;
    public int cx;
    public int cy;
    public int width;
    public int height;

    private FloatBuffer verts_p;
    private final FloatBuffer tex_p;
    private final ByteBuffer inds_p;
    private int tex_ind;
    private final int lkey, ckey, rkey;
    private int startx, starty;
    private final int SLIDE_DIST;
    private final int style;
    private int m_lastKey;
    private int[] tex_inds;
    private int m_split = SPLIT_NONE;

    public Slider(View vw, GL10 gl, int center_x, int center_y, int w, int h, String texid,
                  int leftkey, int centerkey, int rightkey, int stl, float a)
    {
        view = vw;
        cx = center_x;
        cy = center_y;
        style = stl;
        alpha = a;
        width = w;
        height = h;
        SLIDE_DIST = w / 3;
        lkey = leftkey;
        ckey = centerkey;
        rkey = rightkey;

        float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
        float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
        byte[] indices = {0, 1, 2, 0, 2, 3};
        for (int i = 0; i < verts.length; i += 2)
        {
            verts[i] = verts[i] * width;
            verts[i + 1] = verts[i + 1] * height;
        }

        verts_p = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        verts_p.put(verts);
        verts_p.position(0);

        inds_p = ByteBuffer.allocateDirect(indices.length);
        inds_p.put(indices);
        inds_p.position(0);

        tex_p = ByteBuffer.allocateDirect(4 * texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        tex_p.put(texcoords);
        tex_p.position(0);

        tex_androidid = texid;
    }

    @Override
    public void loadtex(GL10 gl)
    {
        Context context = view.getContext();
        String[] split = tex_androidid.split(";");
        m_split = SPLIT_NONE;
        if(split.length >= 4)
        {
            Bitmap[] bs = new Bitmap[3];
            boolean suc = true;
            for(int i = 1; i <= 3; i++)
            {
                Bitmap bitmap = Q3EUtils.ResourceToBitmap(context, split[i]);
                if(null == bitmap)
                {
                    suc = false;
                    break;
                }
                bs[i - 1] = bitmap;
            }
            if(suc)
            {
                tex_inds = new int[3];
                for(int i = 0; i < 3; i++)
                    tex_inds[i] = Q3EGL.loadGLTexture(gl, bs[i]);

                float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};

                if(style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT || style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK)
                {
                    for (int i = 0; i < verts.length; i += 2)
                    {
                        verts[i] = verts[i] * width / 2;
                        verts[i + 1] = verts[i + 1] * height / 2;
                    }
                    m_split = SPLIT_DOWN_RIGHT;
                }
                else
                {
                    for (int i = 0; i < verts.length; i += 2)
                    {
                        verts[i] = verts[i] * width / 3;
						verts[i + 1] = verts[i + 1] * height;
//                        verts[i + 1] = Math.min(verts[i] * width / 3, verts[i + 1] * height) + cy;
//                        verts[i] = verts[i] * width / 2 + cx;
//                        verts[i + 1] = verts[i + 1] * height + cy;
                    }
                    m_split = SPLIT_LEFT_RIGHT;
                }

                if(null != verts_p)
                    verts_p.clear();
                verts_p = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
                verts_p.put(verts);
                verts_p.position(0);
            }
            else
            {
                for(int i = 0; i < 3; i++)
                {
                    if(null != bs[i])
                        bs[i].recycle();
                }
                tex_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(context, split[0]));
            }
        }
        else
        {
            tex_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(context, split[0]));
        }
    }

    @Override
    public void Paint(GL11 gl)
    {
        super.Paint(gl);
        switch (m_split)
        {
            case SPLIT_DOWN_RIGHT: {
                int x = width / 4;
                int y = height / 4;
                Q3EGL.DrawVerts_GL1(gl, tex_inds[0], 6, tex_p, verts_p, inds_p, cx-x, cy+y, red, green, blue, alpha);
                Q3EGL.DrawVerts_GL1(gl, tex_inds[1], 6, tex_p, verts_p, inds_p, cx-x, cy-y, red, green, blue, alpha);
                Q3EGL.DrawVerts_GL1(gl, tex_inds[2], 6, tex_p, verts_p, inds_p, cx+x, cy-y, red, green, blue, alpha);
            }
                break;
            case SPLIT_LEFT_RIGHT: {
                int x = width / 3;
                Q3EGL.DrawVerts_GL1(gl, tex_inds[0], 6, tex_p, verts_p, inds_p, cx-x, cy, red, green, blue, alpha);
                Q3EGL.DrawVerts_GL1(gl, tex_inds[1], 6, tex_p, verts_p, inds_p, cx, cy, red, green, blue, alpha);
                Q3EGL.DrawVerts_GL1(gl, tex_inds[2], 6, tex_p, verts_p, inds_p, cx+x, cy, red, green, blue, alpha);
//                int x = width / 2;
//                Q3EGL.DrawVerts(gl, tex_inds[0], 6, tex_p, verts_p, inds_p, -x, 0, red, green, blue, alpha);
//                Q3EGL.DrawVerts(gl, tex_inds[1], 6, tex_p, verts_p, inds_p, 0, 0, red, green, blue, alpha);
//                Q3EGL.DrawVerts(gl, tex_inds[2], 6, tex_p, verts_p, inds_p, x, 0, red, green, blue, alpha);
            }
                break;
            default:
                Q3EGL.DrawVerts_GL1(gl, tex_ind, 6, tex_p, verts_p, inds_p, cx, cy, red, green, blue, alpha);
        }
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act, int id)
    {
        if (act == 1)
        {
            startx = x;
            starty = y;
            if(style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK || style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK)
            {
                int key = KeyInPosition(x, y);
                if(key > 0)
                {
                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, key, 0);
                    m_lastKey = key;
                }
            }
        }
        else if (act == -1)
        {
            switch (style)
            {
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT: {
                    if (x - startx < -SLIDE_DIST)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, lkey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, lkey, 0);
                    }
                    else if (x - startx > SLIDE_DIST)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, rkey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, rkey, 0);
                    }
                    else
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, ckey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, ckey, 0);
                    }
                }
                break;
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT: {
                    if ((y - starty > SLIDE_DIST) || (x - startx > SLIDE_DIST))
                    {
                        double ang = Math.abs(Math.atan2(y - starty, x - startx));
                        if (ang > Math.PI / 4 && ang < Math.PI * 3 / 4)
                        {
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, lkey, 0);
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, lkey, 0);
                        }
                        else
                        { //k
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, rkey, 0);
                            Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, rkey, 0);
                        } //k
                    }
                    else
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, ckey, 0);
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, ckey, 0);
                    }
                }
                break;
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK:
                case Q3EGlobals.ONSCRREN_SLIDER_STYLE_DOWN_RIGHT_SPLIT_CLICK:
                default: {
                    if(m_lastKey > 0)
                    {
                        Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, m_lastKey, 0);
                        m_lastKey = 0;
                    }
                }
                break;
            }
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        if (style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT || style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK)
            return ((2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height));
        else
            return ((2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height)) && (!((y > cy) && (x > cx)));
    }

    private int KeyInPosition(int x, int y)
    {
        if (style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT || style == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK)
        {
            int deltaX = x - cx;
            int slide_dist_2 = SLIDE_DIST / 2;
            if (deltaX < -slide_dist_2)
                return lkey;
            else if (deltaX > slide_dist_2)
                return rkey;
            else
                return ckey;
        }
        else
        {
            int deltaX = x - cx;
            int deltaY = y - cy;
            if (deltaX > 0 && deltaY < 0)
                return rkey;
            else if (deltaX < 0 && deltaY > 0)
                return lkey;
            else if (deltaX <= 0 && deltaY <= 0)
                return ckey;
            else
                return 0;
        }
    }

    public static Slider Move(Slider tmp, GL10 gl)
    {
        Slider news = new Slider(tmp.view, gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid, tmp.lkey, tmp.ckey, tmp.rkey, tmp.style, tmp.alpha);
        news.tex_ind = tmp.tex_ind;
        news.verts_p.clear();
        news.verts_p = tmp.verts_p.duplicate();
        if(null != tmp.tex_inds)
            news.tex_inds = Arrays.copyOf(tmp.tex_inds, 3);
        news.m_split = tmp.m_split;
        return news;
    }

    public void Translate(int dx, int dy)
    {
        cx += dx;
        cy += dy;
    }

    public void SetPosition(int x, int y)
    {
        cx = x;
        cy = y;
    }

    @Override
    public boolean SupportMultiTouch()
    {
        return false;
    }
}
