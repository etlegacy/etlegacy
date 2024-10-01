package com.n0n3m4.q3e.onscreen;

import android.graphics.Rect;
import android.view.View;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.gl.KGLBitmapTexture;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Joystick extends Paintable implements TouchListener
{
    private static final int CONST_INVALID_DIRECTION = -1;
    private static final int CONST_HELPER_BORDER_WIDTH = 8;

    public View view;
    public int cx;
    public int cy;

    private final FloatBuffer verts_p; // ring vertex buffer
    private final FloatBuffer vertsd_p; // dot vertex buffer
    private final FloatBuffer tex_p; // rect texture coord buffer
    private final ByteBuffer inds_p; // rect index buffer
    private int tex_ind; // ring texture
    private int texd_ind; // dot texture

    private final float[] posx = new float[8];
    private final float[] posy = new float[8];
    public int size;
    private final int dot_size;
    private final float internalsize;

    private int dot_pos = CONST_INVALID_DIRECTION;
    private int dotx, doty;
    private boolean dotjoyenabled = false;

    private final int[] codes = { Q3EKeyCodes.KeyCodesGeneric.J_UP, Q3EKeyCodes.KeyCodesGeneric.J_RIGHT, Q3EKeyCodes.KeyCodesGeneric.J_DOWN, Q3EKeyCodes.KeyCodesGeneric.J_LEFT };
    private final int[] Menu_Codes = { Q3EKeyCodes.KeyCodesGeneric.K_UPARROW, Q3EKeyCodes.KeyCodesGeneric.K_RIGHTARROW, Q3EKeyCodes.KeyCodesGeneric.K_DOWNARROW, Q3EKeyCodes.KeyCodesGeneric.K_LEFTARROW };
    private final boolean[] keys = {false, false, false, false};
    private final boolean[] enarr = new boolean[4];

    private int m_joystickReleaseRange_2 = 0;
    private final int m_size_2;
    private int m_posX;
    private int m_posY;
    private int m_fullZoneRadius = 0;
    private final Rect m_range = new Rect();
    private boolean m_pressed = false;
    private final boolean m_editMode;
    private final boolean m_unfixed;
    private FloatBuffer m_outerVertexBuffer; // outer ring vertex buffer
    private FloatBuffer m_innerVertexBuffer; // inner ring vertex buffer
    private FloatBuffer m_borderVertexBuffer; // outer border vertex buffer
    private int m_outerTexture; // outer ring texture
    private int m_innerTexture; // inner ring texture
    private int m_borderTexture; // outer border texture
    private int m_deadZoneRadius = 0;
    private int m_joystickDeadZone_2 = 0;
    private boolean m_updateTexture = false;
    private int m_visibleMode = Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ALWAYS;

    public Joystick(View vw, GL10 gl, int r, float a, int x, int y, float fullZonePercent, float deadZonePercent, boolean unfixed, boolean editMode, int visibleMode, String texid)
    {
        int fullZoneRadius = fullZonePercent >= 1.0f ? (int)((float)r * fullZonePercent) : 0;
        int deadZoneRadius = deadZonePercent > 0.0f ? (int)((float)r * Math.max(0.0f, Math.min(deadZonePercent, 1.0f))) : 0;

        Q3EKeyCodes.ConvertRealKeyCodes(codes);
        Q3EKeyCodes.ConvertRealKeyCodes(Menu_Codes);

        view = vw;
        size = r * 2;
        alpha = a;
        this.m_posX = x;
        this.m_posY = y;
        this.m_editMode = editMode;
        this.m_visibleMode = visibleMode;
        if(unfixed && fullZoneRadius < r) // if unfixed, min range is circle radius
            fullZoneRadius = r;
        if(deadZoneRadius >= r)
            deadZoneRadius = 0;

        this.m_range.set(
                m_posX - fullZoneRadius,
                m_posY - fullZoneRadius,
                m_posX + fullZoneRadius,
                m_posY + fullZoneRadius
        );
        this.m_unfixed = unfixed;

        if (fullZoneRadius >= r)
        {
            this.m_fullZoneRadius = fullZoneRadius;
            m_joystickReleaseRange_2 = m_fullZoneRadius * m_fullZoneRadius * 4;
        }
        if (deadZoneRadius > 0)
        {
            this.m_deadZoneRadius = deadZoneRadius;
            m_joystickDeadZone_2 = m_deadZoneRadius * m_deadZoneRadius * 4;
        }
        m_size_2 = size * size;

        // if(m_editMode || !m_unfixed)
        {
            cx = m_posX;
            cy = m_posY;
        }
        dot_size = this.m_unfixed ? size / 2 : size / 3;

        float[] verts_dot = MakeVertexArray(dot_size);
        float[] verts_back = MakeVertexArray(size);
        final float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
        final byte[] indices = {0, 1, 2, 0, 2, 3};

        verts_p = ByteBuffer.allocateDirect(4 * verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        verts_p.put(verts_back);
        verts_p.position(0);

        vertsd_p = ByteBuffer.allocateDirect(4 * verts_dot.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        vertsd_p.put(verts_dot);
        vertsd_p.position(0);

        inds_p = ByteBuffer.allocateDirect(indices.length);
        inds_p.put(indices);
        inds_p.position(0);

        tex_p = ByteBuffer.allocateDirect(4 * texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        tex_p.put(texcoords);
        tex_p.position(0);

        internalsize = (size / 2.0f - CalcRingWidth()) - ((float) size / 3.0f) / 2.0f;
        for (int i = 0; i < 8; i++)
        {
            posx[i] = (float) (internalsize * Math.sin(i * Math.PI / 4));
            posy[i] = -(float) (internalsize * Math.cos(i * Math.PI / 4));
        }

        if(m_editMode)
        {
            float[] verts;
            if(m_fullZoneRadius > 0)
            {
                if(m_unfixed)
                {
                    verts = MakeVertexArray(m_fullZoneRadius * 2.0f);
                    m_borderVertexBuffer = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
                    m_borderVertexBuffer.put(verts);
                    m_borderVertexBuffer.position(0);
                }
                else
                {
                    verts = MakeVertexArray(m_fullZoneRadius * 2.0f);
                    m_outerVertexBuffer = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
                    m_outerVertexBuffer.put(verts);
                    m_outerVertexBuffer.position(0);
                }
            }

            if(m_deadZoneRadius > 0)
            {
                verts = MakeVertexArray(m_deadZoneRadius * 2.0f);
                m_innerVertexBuffer = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
                m_innerVertexBuffer.put(verts);
                m_innerVertexBuffer.position(0);
            }
        }

        tex_androidid = texid;
    }

    public void Paint(GL11 gl)
    {
        //main paint
        super.Paint(gl);

        if(!m_editMode)
        {
            if((m_visibleMode != Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_HIDDEN && m_visibleMode != Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ONLY_PRESSED) // always
                    || (m_visibleMode == Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ONLY_PRESSED && m_pressed) // only pressed
            )
            {
                if(!m_unfixed)
                {
                    Q3EGL.DrawVerts_GL1(gl, tex_ind, 6, tex_p, verts_p, inds_p, cx, cy, red, green, blue, alpha);

                    // int dp = dot_pos;//Multithreading.
                    if (dotjoyenabled)
                        Q3EGL.DrawVerts_GL1(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, cx + dotx, cy + doty, red, green, blue, alpha);
                    else if (dot_pos != CONST_INVALID_DIRECTION)
                        Q3EGL.DrawVerts_GL1(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, cx + posx[dot_pos], cy + posy[dot_pos], red, green, blue, alpha);
                }
                else
                {
                    if(m_pressed)
                    {
                        // GL.DrawVerts(gl, tex_ind, 6, tex_p, verts_p, inds_p, cx, cy, red, green, blue, alpha);
                        if (dotjoyenabled)
                            Q3EGL.DrawVerts_GL1(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, cx + dotx, cy + doty, red, green, blue, alpha);
                        else if (dot_pos != CONST_INVALID_DIRECTION)
                            Q3EGL.DrawVerts_GL1(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, cx + posx[dot_pos], cy + posy[dot_pos], red, green, blue, alpha);
                    }
                    else
                        Q3EGL.DrawVerts_GL1(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, m_posX, m_posY, red, green, blue, alpha);
                }
            }
        }
        else
        {
            if(!m_unfixed)
                Q3EGL.DrawVerts_GL1(gl, tex_ind, 6, tex_p, verts_p, inds_p, m_posX, m_posY, red, green, blue, alpha);
            else
                Q3EGL.DrawVerts_GL1(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, m_posX, m_posY, red, green, blue, alpha);

            if(null != m_outerVertexBuffer)
                Q3EGL.DrawVerts_GL1(gl, m_outerTexture, 6, tex_p, m_outerVertexBuffer, inds_p, m_posX, m_posY, /*red, green, blue, */0, 1, 0, alpha);
            else if(null != m_borderVertexBuffer)
                Q3EGL.DrawVerts_GL1(gl, m_borderTexture, 6, tex_p, m_borderVertexBuffer, inds_p, m_posX, m_posY, /*red, green, blue, */0, 1, 0, alpha);
            if(null != m_innerVertexBuffer)
                Q3EGL.DrawVerts_GL1(gl, m_innerTexture, 6, tex_p, m_innerVertexBuffer, inds_p, m_posX, m_posY, /*red, green, blue, */1, 0, 0, alpha);
        }
    }

    @Override
    public void loadtex(GL10 gl)
    {
        String[] m_textures = null;
        if(null != tex_androidid && !tex_androidid.isEmpty())
            m_textures = tex_androidid.split(";");

        final int[] color = {255, 255, 255, 255};
        if(null != m_textures && m_textures.length > 0)
            tex_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(view.getContext(), m_textures[0]));
        if(tex_ind == 0)
            tex_ind = KGLBitmapTexture.GenCircleRingTexture(gl, size, CalcRingWidth(), color);

        if(null != m_textures && m_textures.length > 1)
            texd_ind = Q3EGL.loadGLTexture(gl, Q3EUtils.ResourceToBitmap(view.getContext(), m_textures[1]));
        if(texd_ind == 0)
            texd_ind = KGLBitmapTexture.GenCircleTexture(gl, dot_size, color);

        if(m_editMode)
        {
            if(m_fullZoneRadius > 0)
            {
                if(m_unfixed)
                    m_borderTexture = KGLBitmapTexture.GenRectBorderTexture(gl, m_fullZoneRadius * 2, -1,CONST_HELPER_BORDER_WIDTH, color);
                else
                    m_outerTexture = KGLBitmapTexture.GenCircleRingTexture(gl, m_fullZoneRadius * 2, CONST_HELPER_BORDER_WIDTH, color);
            }
            if(m_deadZoneRadius > 0)
                m_innerTexture = KGLBitmapTexture.GenCircleRingTexture(gl, m_deadZoneRadius * 2, CONST_HELPER_BORDER_WIDTH, color);
        }
    }

    public void setenabled(int ind, boolean b)
    {
        if ((keys[ind] != b))
        {
            keys[ind] = b;
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(b, (Q3EUtils.q3ei.callbackObj.notinmenu ? codes : Menu_Codes)[ind], 0);
        }
    }

    public void setenabledarr(boolean[] arr)
    {
        for (int i = 0; i <= 3; i++)
            setenabled(i, arr[i]);
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act, int id)
    {
        return m_unfixed ? HandleEvent_unfixed(x, y, act) : HandleEvent_fixed(x, y, act);
    }

    @Override
    public boolean isInside(int x, int y)
    {
        if(!m_unfixed || m_editMode)
        {
            int deltax = cx - x;
            int deltay = cy - y;
            return 4 * (deltax * deltax + deltay * deltay) <= m_size_2;
        }
        else
        {
            return m_range.contains(x, y);
        }
    }

    private boolean HandleEvent_fixed(int x, int y, int act)
    {
        final int deltax = x - cx;
        final int deltay = y - cy;
        boolean res = !NotInFullZone(deltax, deltay);
        if(act == ACT_PRESS)
            m_pressed = true;
        if (res && act != ACT_RELEASE)
        {
            if (Q3EUtils.q3ei.callbackObj.notinmenu)
            {
                if (Q3EUtils.q3ei.joystick_smooth)
                {
                    dotjoyenabled = true;
                    dotx = deltax;
                    doty = deltay;
                    float analogx = (dotx) / internalsize;
                    float analogy = (-doty) / internalsize;
//                    if (Math.abs(analogx) > 1.0f) analogx = analogx / Math.abs(analogx);
//                    if (Math.abs(analogy) > 1.0f) analogy = analogy / Math.abs(analogy);
                    if (analogx < -1.0f || analogx > 1.0f) analogx = Math.signum(analogx);
                    if (analogy < -1.0f || analogy > 1.0f) analogy = Math.signum(analogy);

                    double dist = Math.sqrt(dotx * dotx + doty * doty);
                    if (dist > internalsize)
                    {
                        dotx = (int) (dotx * internalsize / dist);
                        doty = (int) (doty * internalsize / dist);
                    }
                    if(NotInDeadZone(deltax, deltay))
                    {
                        //m_pressed = true;
                        Q3EUtils.q3ei.callbackObj.sendAnalog(true, analogx, analogy);
                    }
                    else
                    {
                        //m_pressed = false;
                        Q3EUtils.q3ei.callbackObj.sendAnalog(false, 0, 0);
                    }
                }
                else
                {
                    final int angle = NormalizeDegree(
                            (int) (112.5 - 180 * (Math.atan2(-deltay, deltax) / Math.PI))
                    );
                    dot_pos = (int) (angle / 45);
                    if(NotInDeadZone(deltax, deltay))
                    {
                        //m_pressed = true;
                        enarr[0] = (dot_pos % 7 < 2);
                        enarr[1] = (dot_pos > 0) && (dot_pos < 4);
                        enarr[2] = (dot_pos > 2) && (dot_pos < 6);
                        enarr[3] = (dot_pos > 4);
                    }
                    else
                    {
                        //m_pressed = false;
                        enarr[0] = enarr[1] = enarr[2] = enarr[3] = false;
                    }
                }
            }
            else
            {
                //IN MENU
                final int angle = NormalizeDegree(
                        (int) (135 - 180 * (Math.atan2(-deltay, deltax) / Math.PI))
                );
                dot_pos = (int) (angle / 90);
                enarr[0] = enarr[1] = enarr[2] = enarr[3] = false;
                if(NotInDeadZone(deltax, deltay))
                {
                    //m_pressed = true;
                    enarr[dot_pos] = true;
                }
                dot_pos *= 2;
            }
        }
        else
        {
            m_pressed = false;
            if (Q3EUtils.q3ei.joystick_smooth)
            {
                dotjoyenabled = false;
                Q3EUtils.q3ei.callbackObj.sendAnalog(false, 0, 0);
            }
            dot_pos = CONST_INVALID_DIRECTION;
                enarr[0] = false;
                enarr[1] = false;
                enarr[2] = false;
                enarr[3] = false;
        }
        setenabledarr(enarr);
        return res;
    }

    public boolean HandleEvent_unfixed(int x, int y, int act)
    {
        int deltax;
        int deltay;
        //Q3EControlView controlView = (Q3EControlView) (this.view);
        boolean res = true;
        switch (act)
        {
            case ACT_PRESS:
                if(!m_editMode)
                {
                    cx = x;
                    cy = y;
                }
                m_pressed = true;
                break;

            case ACT_MOTION:
                deltax = x - cx;
                deltay = y - cy;
                /*
                if (NotInFullZone(deltax, deltay))
                {
                    m_pressed = false;
                    res = false;

                    if (controlView.analog)
                    {
                        dotjoyenabled = false;
                        controlView.sendAnalog(false, 0, 0);
                    }
                    dot_pos = CONST_INVALID_DIRECTION;
                    enarr[0] = false;
                    enarr[1] = false;
                    enarr[2] = false;
                    enarr[3] = false;
                }
                else // if(res)
                */
                {
                    if (Q3EUtils.q3ei.callbackObj.notinmenu)
                    {
                        if (Q3EUtils.q3ei.joystick_smooth)
                        {
                            dotjoyenabled = true;
                            dotx = deltax;
                            doty = deltay;
                            float analogx = (dotx) / internalsize;
                            float analogy = (-doty) / internalsize;
                            if (analogx < -1.0f || analogx > 1.0f) analogx = Math.signum(analogx);
                            if (analogy < -1.0f || analogy > 1.0f) analogy = Math.signum(analogy);
                            //Log.e("TAG", "HandleEvent_unfixed_MMMMMMMM: " + cx + "  " + cy + "   | "  +  + x + "  " + y + "   | "  + deltax + "  " + deltay + "   | " + analogx + "  " + analogy);
/*
                            double dist = Math.sqrt(dotx * dotx + doty * doty);
                            if (dist > internalsize)
                            {
                                dotx = (int) (dotx * internalsize / dist);
                                doty = (int) (doty * internalsize / dist);
                            }
                            */
                            if(NotInDeadZone(deltax, deltay))
                                Q3EUtils.q3ei.callbackObj.sendAnalog(true, analogx, analogy);
                            else
                                Q3EUtils.q3ei.callbackObj.sendAnalog(false, 0, 0);
                        }
                        else
                        {
                            final int angle = NormalizeDegree(
                                    (int) (112.5 - 180 * (Math.atan2(-deltay, deltax) / Math.PI))
                            );
                            dot_pos = (int) (angle / 45);
                            if(NotInDeadZone(deltax, deltay))
                            {
                                enarr[0] = (dot_pos % 7 < 2);
                                enarr[1] = (dot_pos > 0) && (dot_pos < 4);
                                enarr[2] = (dot_pos > 2) && (dot_pos < 6);
                                enarr[3] = (dot_pos > 4);
                            }
                            else
                            {
                                enarr[0] = enarr[1] = enarr[2] = enarr[3] = false;
                            }
                        }
                    }
                    else
                    {
                        //IN MENU
                        final int angle = NormalizeDegree(
                                (int) (135 - 180 * (Math.atan2(-deltay, deltax) / Math.PI))
                        );
                        dot_pos = (int) (angle / 90);
                        enarr[0] = enarr[1] = enarr[2] = enarr[3] = false;
                        if(NotInDeadZone(deltax, deltay))
                        {
                enarr[dot_pos] = true;
                        }
                dot_pos *= 2;
            }
                }
                break;

            case ACT_RELEASE:
                m_pressed = false;

                if (Q3EUtils.q3ei.joystick_smooth)
                {
                    dotjoyenabled = false;
                    Q3EUtils.q3ei.callbackObj.sendAnalog(false, 0, 0);
                }
                dot_pos = CONST_INVALID_DIRECTION;
            enarr[0] = false;
            enarr[1] = false;
            enarr[2] = false;
            enarr[3] = false;
                break;
        }
        setenabledarr(enarr);
        return res;
    }

    private float[] MakeVertexArray(float size)
    {
        float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
        for (int i = 0; i < verts.length; i += 2)
        {
            verts[i] = verts[i] * size;
            verts[i + 1] = verts[i + 1] * size;
        }
        return verts;
    }

    public static Joystick Move(Joystick tmp, GL10 gl, float fullZonePercent, float deadZonePercent)
    {
        Joystick newj = new Joystick(tmp.view, gl, tmp.size / 2, tmp.alpha, tmp.cx, tmp.cy, fullZonePercent, deadZonePercent, tmp.m_unfixed, tmp.m_editMode, tmp.m_visibleMode, tmp.tex_androidid);
        newj.tex_ind = tmp.tex_ind;
        newj.texd_ind = tmp.texd_ind;
        newj.m_outerTexture = tmp.m_outerTexture;
        newj.m_innerTexture = tmp.m_innerTexture;
        newj.m_borderTexture = tmp.m_borderTexture;
        return newj;
    }

    public void Translate(int dx, int dy)
    {
        m_posX += dx;
        m_posY += dy;
        cx += dx;
        cy += dy;
    }

    public void SetPosition(int x, int y)
    {
        m_posX = x;
        m_posY = y;
        cx = x;
        cy = y;
    }

    private boolean NotInDeadZone(int dx, int dy)
    {
        return m_joystickDeadZone_2 <= 0 || (4 * (dx * dx + dy * dy) >= m_joystickDeadZone_2);
    }

    private boolean NotInFullZone(int dx, int dy)
    {
        return m_joystickReleaseRange_2 > 0 && (4 * (dx * dx + dy * dy) > m_joystickReleaseRange_2);
    }

    private float CalcRingWidth()
    {
        return size / 24.0f;
    }

    private int NormalizeDegree(int angle)
    {
        return angle < 0 ? angle + 360 : (angle >= 360 ? angle - 360 : angle);
    }

    public void SetupFullZoneRadiusInEditMode(float fullZonePercent)
    {
        if(!m_editMode)
            return;
        int r = size / 2;
        int fullZoneRadius = fullZonePercent >= 1.0f ? (int)((float)r * fullZonePercent) : 0;
        if(fullZoneRadius < r)
        {
            if(m_unfixed)
                fullZoneRadius = r;
            else
                fullZoneRadius = 0;
        }

        if(fullZoneRadius == m_fullZoneRadius)
            return;
        this.m_fullZoneRadius = fullZoneRadius;

        if(m_fullZoneRadius >= r)
        {
            float[] verts;
            if(m_unfixed)
            {
                verts = MakeVertexArray(m_fullZoneRadius * 2.0f);
                m_borderVertexBuffer = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
                m_borderVertexBuffer.put(verts);
                m_borderVertexBuffer.position(0);
            }
            else
            {
                verts = MakeVertexArray(m_fullZoneRadius * 2.0f);
                m_outerVertexBuffer = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
                m_outerVertexBuffer.put(verts);
                m_outerVertexBuffer.position(0);
            }
            m_joystickReleaseRange_2 = m_fullZoneRadius * m_fullZoneRadius * 4;
        }
        else
            m_joystickReleaseRange_2 = 0;
        m_updateTexture = true;
    }

    public void SetupDeadZoneRadiusInEditMode(float deadZonePercent)
    {
        if(!m_editMode)
            return;

        int r = size / 2;
        int deadZoneRadius = deadZonePercent > 0.0f ? (int)((float)r * Math.max(0.0f, Math.min(deadZonePercent, 1.0f))) : 0;
        if(deadZoneRadius >= r)
            deadZoneRadius = 0;
        if(deadZoneRadius == m_deadZoneRadius)
            return;
        this.m_deadZoneRadius = deadZoneRadius;

        if(m_deadZoneRadius > 0)
        {
            float[] verts = MakeVertexArray(m_deadZoneRadius * 2.0f);
            m_innerVertexBuffer = ByteBuffer.allocateDirect(4 * verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
            m_innerVertexBuffer.put(verts);
            m_innerVertexBuffer.position(0);
            m_joystickDeadZone_2 = m_deadZoneRadius * m_deadZoneRadius * 4;
        }
        else
            m_joystickDeadZone_2 = 0;
        m_updateTexture = true;
    }

    public void UpdateTexture(GL11 gl)
    {
        if(!m_updateTexture)
            return;

        Q3EGL.glDeleteTexture(gl, m_borderTexture);
        Q3EGL.glDeleteTexture(gl, m_outerTexture);
        Q3EGL.glDeleteTexture(gl, m_innerTexture);
        final int[] color = {255, 255, 255, 255};
        if(m_fullZoneRadius > 0)
        {
            if(m_unfixed)
            {
                m_borderTexture = KGLBitmapTexture.GenRectBorderTexture(gl, m_fullZoneRadius * 2, -1,CONST_HELPER_BORDER_WIDTH, color);
            }
            else
            {
                m_outerTexture = KGLBitmapTexture.GenCircleRingTexture(gl, m_fullZoneRadius * 2, CONST_HELPER_BORDER_WIDTH, color);
            }
        }
        if(m_deadZoneRadius > 0)
        {
            m_innerTexture = KGLBitmapTexture.GenCircleRingTexture(gl, m_deadZoneRadius * 2, CONST_HELPER_BORDER_WIDTH, color);
        }
        m_updateTexture = false;
    }

    @Override
    public boolean SupportMultiTouch()
    {
        return false;
    }
}
