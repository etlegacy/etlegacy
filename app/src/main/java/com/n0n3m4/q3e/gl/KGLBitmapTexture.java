package com.n0n3m4.q3e.gl;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;

import javax.microedition.khronos.opengles.GL10;
/*
import java.awt.*;
import java.awt.font.TextAttribute;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;
import java.util.Map;
*/

public final class KGLBitmapTexture
{
    private KGLBitmapTexture() {}

    public static int GenCircleRingTexture(GL10 gl, int width, float ringWidth, int[] rgba)
    {
        if(width <= 0)
            return 0;

        final float radius = (float)width / 2.0f;
        final float internalsize = radius - ringWidth;

        try
        {
            Bitmap bmp = Bitmap.createBitmap(width, width, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(bmp);
            Paint p = new Paint();
            p.setAntiAlias(true);
            p.setARGB(rgba[0], rgba[1], rgba[2], rgba[3]);
            c.drawARGB(0, 0, 0, 0);
            c.drawCircle(radius, radius, radius, p);
            p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
            c.drawCircle(radius, radius, internalsize, p);

            return Q3EGL.loadGLTexture(gl, bmp);
        }
        catch (OutOfMemoryError e)
        {
            e.printStackTrace();
            return 0;
        }
    }

    public static int GenCircleTexture(GL10 gl, int width, int[] rgba)
    {
        if(width <= 0)
            return 0;

        final float radius = (float)width / 2.0f;

        try
        {
            Bitmap bmp = Bitmap.createBitmap(width, width, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(bmp);
            Paint p = new Paint();
            p.setAntiAlias(true);
            p.setARGB(rgba[0], rgba[1], rgba[2], rgba[3]);
            c.drawARGB(0, 0, 0, 0);
            c.drawCircle(radius, radius, radius, p);

            return Q3EGL.loadGLTexture(gl, bmp);
        }
        catch (OutOfMemoryError e)
        {
            e.printStackTrace();
            return 0;
        }
    }

    public static int GenRectBorderTexture(GL10 gl, int width, int height, float borderWidth, int[] rgba)
    {
        if(width <= 0)
            return 0;

        if(height <= 0)
            height = width;
        Rect rect = new Rect(0, 0, width, height);

        try
        {
            Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(bmp);
            Paint p = new Paint();
            p.setAntiAlias(true);
            p.setARGB(rgba[0], rgba[1], rgba[2], rgba[3]);
            c.drawARGB(0, 0, 0, 0);
            c.drawRect(rect, p);
            p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
            RectF rectf = new RectF(borderWidth, borderWidth, width - borderWidth, height - borderWidth);
            c.drawRect(rectf, p);

            return Q3EGL.loadGLTexture(gl, bmp);
        }
        catch (OutOfMemoryError e)
        {
            e.printStackTrace();
            return 0;
        }
    }

/*
    public static Image CreateTextButton(String text)
    {
        final int WIDTH = 256;
        final int CIRCLE_WIDTH = 12;
        final int FONT_SIZE = 200;
        final int CENTER = WIDTH / 2 - 1;
        final String FONT_FAMILY = "monospace";

        Image image = new BufferedImage(WIDTH, WIDTH, BufferedImage.TYPE_INT_ARGB);
        Graphics g = image.getGraphics();

        Graphics2D g2d = (Graphics2D)g;
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g2d.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        g2d.setColor(Color.WHITE);
        Stroke stroke = new BasicStroke(CIRCLE_WIDTH);
        g2d.setStroke(stroke);
        g.drawArc(CIRCLE_WIDTH / 2, CIRCLE_WIDTH / 2, WIDTH - CIRCLE_WIDTH - 1, WIDTH - CIRCLE_WIDTH - 1, 0, 360);

        Map<TextAttribute, Object> attributes = (Map<TextAttribute, Object>) g.getFont().getAttributes();
        attributes.put(TextAttribute.SIZE, FONT_SIZE);
        attributes.put(TextAttribute.FAMILY, FONT_FAMILY);
        //attributes.put(TextAttribute.WEIGHT, TextAttribute.WEIGHT_BOLD);
        Font font = Font.getFont(attributes);
        g2d.setFont(font);

        Rectangle rect = new Rectangle();
        FontMetrics fontMetrics = g.getFontMetrics();
        Rectangle2D bounds = fontMetrics.getStringBounds(text, g);

        Rectangle b = bounds.getBounds();
        rect.x = -(int)b.getCenterX();
        rect.y = -(int)b.getCenterY();
        rect.width = b.width;
        rect.height = b.height;

        g.drawString(text, CENTER + rect.x, CENTER + rect.y);

        return image;
    }
 */
}
