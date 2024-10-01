package com.n0n3m4.q3e.onscreen;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;

import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.gl.KGLBitmapTexture;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Disc extends Paintable implements TouchListener
{
    private static class Part
    {
        public float start;
        public float end;
        public char key;
        public int keyCode;
        public int textureId = 0;
        public int borderTextureId = 0;
        public boolean pressed;
        public boolean disabled;
    }

    public View view;
    public int cx, cy;

    private final FloatBuffer m_fanVertexArray;
    private final FloatBuffer verts_p;
    private final FloatBuffer tex_p;
    private final ByteBuffer inds_p;
    private Part[] m_parts = null;
    public int size;
    private final int dot_size;
    private int tex_ind;
    private int m_circleWidth;
    private final char[] m_keys;
    private final int m_size_2;

    private int dotx, doty;
    private boolean dotjoyenabled = false;

    public Disc(View vw, GL10 gl, int center_x, int center_y, int r, float a, char[] keys, String texid)
    {
        view = vw;
        cx = center_x;
        cy = center_y;
        size = r * 2;
        dot_size = size * 3;
        alpha = a;
        m_size_2 = size * size;
        float[] verts_back = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
        float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
        byte[] indices = {0, 1, 2, 0, 2, 3};

        float[] tmp = new float[verts_back.length];
        for (int i = 0; i < verts_back.length; i += 2)
        {
            tmp[i] = verts_back[i] * size;
            tmp[i + 1] = verts_back[i + 1] * size;
        }

        verts_p = ByteBuffer.allocateDirect(4 * verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        verts_p.put(tmp);
        verts_p.position(0);

        inds_p = ByteBuffer.allocateDirect(indices.length);
        inds_p.put(indices);
        inds_p.position(0);

        tex_p = ByteBuffer.allocateDirect(4 * texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        tex_p.put(texcoords);
        tex_p.position(0);

        float[] tmp2 = new float[verts_back.length];
        for (int i = 0; i < verts_back.length; i += 2)
        {
            tmp2[i] = verts_back[i] * dot_size;
            tmp2[i + 1] = verts_back[i + 1] * dot_size;
        }

        m_fanVertexArray = ByteBuffer.allocateDirect(4 * verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        m_fanVertexArray.put(tmp2);
        m_fanVertexArray.position(0);

        m_keys = keys;

        tex_androidid = texid;
    }

    private Part GenPart(int index, char key, int total, GL10 gl)
    {
        Part res = new Part();
        res.key = key;
        res.keyCode = Q3EKeyCodes.GetRealKeyCode(key);
        double P = Math.PI * 2 / total;
        int centerR = size / 2;
        double start = P * index;
        double end = start + P;
        final double offset = -Math.PI / 2;
        final double mid = (end + start) / 2 + offset;
        int r = dot_size / 2;
        int sw = m_circleWidth / 2;
        final int fontSize = 10;

        res.start = Q3EUtils.Rad2Deg(start);
        res.end = Q3EUtils.Rad2Deg(end);

        if(dot_size > 0)
        {
            Bitmap bmp = Bitmap.createBitmap(dot_size, dot_size, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(bmp);
            Paint p = new Paint();
            p.setAntiAlias(true);
            p.setARGB(255, 255, 255, 255);
            c.drawARGB(0, 0, 0, 0);
            p.setStrokeWidth(sw);
            p.setTextSize(sw * fontSize * 3 / 2);
            p.setTextAlign(Paint.Align.CENTER);
            Paint.FontMetrics fontMetrics = p.getFontMetrics();
            float fontHeight = (fontMetrics.descent - fontMetrics.ascent) / 2 - fontMetrics.descent;

            double rad = start + offset;
            double x = Math.cos(rad);
            double y = Math.sin(rad);
            c.drawLine(r + (int) (x * centerR), r + (int) (y * centerR), r + (int) (x * r), r + (int) (y * r), p);
            rad = end + offset;
            x = Math.cos(rad);
            y = Math.sin(rad);
            c.drawLine(r + (int) (x * centerR), r + (int) (y * centerR), r + (int) (x * r), r + (int) (y * r), p);

            x = Math.cos(mid);
            y = Math.sin(mid);
            int mr = (r + centerR) / 2;
            c.drawText("" + Character.toUpperCase(key), r + (int) (x * mr), r + (int) (y * mr + (fontHeight)), p);

            res.textureId = Q3EGL.loadGLTexture(gl, bmp);

            sw = m_circleWidth;
            bmp = Bitmap.createBitmap(dot_size, dot_size, Bitmap.Config.ARGB_8888);
            c = new Canvas(bmp);
            p = new Paint();
            p.setAntiAlias(true);
            p.setARGB(255, 255, 255, 255);
            c.drawARGB(0, 0, 0, 0);
            p.setStrokeWidth(sw);
            p.setTextSize(sw * fontSize);
            p.setTextAlign(Paint.Align.CENTER);
            fontMetrics = p.getFontMetrics();
            fontHeight = (fontMetrics.descent - fontMetrics.ascent) / 2 - fontMetrics.descent;
            rad = start + offset;
            x = Math.cos(rad);
            y = Math.sin(rad);
            centerR -= m_circleWidth;
            c.drawLine(r + (int) (x * centerR), r + (int) (y * centerR), r + (int) (x * r), r + (int) (y * r), p);
            rad = end + offset;
            x = Math.cos(rad);
            y = Math.sin(rad);
            c.drawLine(r + (int) (x * centerR), r + (int) (y * centerR), r + (int) (x * r), r + (int) (y * r), p);

            x = Math.cos(mid);
            y = Math.sin(mid);
            mr = (r + centerR) / 2;
            c.drawText("" + Character.toUpperCase(key), r + (int) (x * mr), r + (int) (y * mr + (fontHeight)), p);

            p.setStyle(Paint.Style.STROKE);
            start = start / Math.PI * 180;
            end = end / Math.PI * 180;
            RectF rect = new RectF(dot_size / 2 - size / 2 + sw / 2, dot_size / 2 - size / 2 + sw / 2, dot_size / 2 + size / 2 - sw / 2, dot_size / 2 + size / 2 - sw / 2);
            c.drawArc(rect, (float) (start - 90), (float) (end - start), false, p);

            rect = new RectF(0 + sw / 2, 0 + sw / 2, dot_size - sw / 2, dot_size - sw / 2);
            c.drawArc(rect, (float) (start - 90), (float) (end - start), false, p);

            res.borderTextureId = Q3EGL.loadGLTexture(gl, bmp);
        }

        return res;
    }

    public void Paint(GL11 gl)
    {
        //main paint
        super.Paint(gl);
        Q3EGL.DrawVerts_GL1(gl, tex_ind, 6, tex_p, verts_p, inds_p, cx, cy, red, green, blue, alpha);
        if (null == m_parts || m_parts.length == 0)
            return;

        if (dotjoyenabled)
        {
            for (Part p : m_parts)
            {
                //DrawVerts(gl, p.textureId, 6, tex_p, m_fanVertexArray, inds_p, 0, 0, red,green,blue,p.pressed ? (float)Math.max(alpha, 0.9) : (float)(Math.min(alpha, 0.1)));
                if (p.pressed)
                    Q3EGL.DrawVerts_GL1(gl, p.borderTextureId, 6, tex_p, m_fanVertexArray, inds_p, cx, cy, red, green, blue, alpha + (1.0f - alpha) * 0.5f);
                else
                    Q3EGL.DrawVerts_GL1(gl, p.textureId, 6, tex_p, m_fanVertexArray, inds_p, cx, cy, red, green, blue, alpha - (alpha * 0.5f));
            }
        }
    }

    @Override
    public void loadtex(GL10 gl)
    {
        String[] m_textures = null;
        if (null != tex_androidid && !tex_androidid.isEmpty())
            m_textures = tex_androidid.split(";");

        int internalsize = size / 2 * 7 / 8;
        m_circleWidth = size / 2 - internalsize;
        final int[] color = {255, 255, 255, 255};
        if (null != m_textures && m_textures.length > 0)
            tex_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(view.getContext(), m_textures[0]));
        if (tex_ind == 0)
            tex_ind = KGLBitmapTexture.GenCircleRingTexture(gl, size, m_circleWidth, color);

        if (null != m_keys && m_keys.length > 0)
        {
            m_parts = new Part[m_keys.length];
            for (int i = 0; i < m_keys.length; i++)
            {
                m_parts[i] = GenPart(i, m_keys[i], m_keys.length, gl);
            }
        }
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act/* 1: Down, -1: Up */, int id)
    {
        if (null == m_parts || m_parts.length == 0)
            return true;
        dotjoyenabled = true;
        dotx = x - cx;
        doty = y - cy;
        boolean inside = 4 * (dotx * dotx + doty * doty) <= m_size_2;

        switch (act)
        {
            case ACT_PRESS:
                if (inside)
                    dotjoyenabled = true;
                break;
            case ACT_RELEASE:
                if (dotjoyenabled)
                {
                    if (!inside)
                    {
                        boolean has = false;
                        for (Part p : m_parts)
                        {
                            if (!has)
                            {
                                if (p.pressed)
                                {
                                    has = true;
                                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(true, p.keyCode, 0);
                                    Q3EUtils.q3ei.callbackObj.sendKeyEvent(false, p.keyCode, 0);
                                }
                            }
                            p.pressed = false;
                        }
                    }
                    else
                    {
                        for (Part p : m_parts)
                            p.pressed = false;
                    }
                }
                dotjoyenabled = false;
                break;
            case ACT_MOTION:
            default:
                if (dotjoyenabled)
                {
                    if (!inside)
                    {
                        float t = Q3EUtils.Rad2Deg(Math.atan2(doty, dotx) + Math.PI / 2);
                        boolean has = false;
                        for (Part p : m_parts)
                        {
                            boolean b = false;
                            if (!has)
                            {
                                if (t >= p.start && t < p.end)
                                {
                                    has = true;
                                    b = true;
                                }
                            }
                            p.pressed = b;
                        }
                    }
                    else
                    {
                        for (Part p : m_parts)
                            p.pressed = false;
                    }
                }
                break;
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return 4 * ((cx - x) * (cx - x) + (cy - y) * (cy - y)) <= m_size_2;
    }

    public static Disc Move(Disc tmp, GL10 gl)
    {
        Disc newd = new Disc(tmp.view, gl, tmp.cx, tmp.cy, tmp.size / 2, tmp.alpha, null, tmp.tex_androidid);
        newd.tex_ind = tmp.tex_ind;
        newd.m_parts = tmp.m_parts;
        return newd;
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
