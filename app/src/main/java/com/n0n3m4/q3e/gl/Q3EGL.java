package com.n0n3m4.q3e.gl;

import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.opengl.GLUtils;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EUtils;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public final class Q3EGL
{
    public static boolean usegles20 = true;

    public static int vertexShader;
    public static int fragmentShader;
    public static int gl20program;

    public static int gl20tx;
    public static int gl20sc;
    public static int gl20tr;
    public static int gl20cl;

    //GL20 Utils

    public static void initGL20()
    {
        final String vertexShaderCode =
                "attribute vec4 vPosition;" +
                        "attribute vec4 vTexCoord;" +
                        "uniform vec4 uScale;" +
                        "uniform vec4 uTranslate;" +
                        "varying vec2 varTexCoord;" +
                        "void main() {" +
                        "  gl_Position = uScale*(vPosition+uTranslate);" +//vec4(uScale.x*(vPosition.x+uTranslate.x),uScale.y*(vPosition.y+uTranslate.y),vPosition.z,vPosition.w);" +
                        "  varTexCoord = vec2(vTexCoord.x,vTexCoord.y);" +
                        "}";

        final String fragmentShaderCode =
                "precision mediump float;" +
                        "uniform sampler2D uTexture;" +
                        "uniform vec4 uColor;" +
                        "varying vec2 varTexCoord;" +
                        "void main() {" +
                        "  gl_FragColor = uColor*texture2D(uTexture, varTexCoord);" +
                        "}";

        vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode);
        fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode);
        gl20program = GLES20.glCreateProgram();
        GLES20.glAttachShader(gl20program, vertexShader);
        GLES20.glAttachShader(gl20program, fragmentShader);
        GLES20.glLinkProgram(gl20program);
        gl20tx = GLES20.glGetUniformLocation(gl20program, "uTexture");
        gl20sc = GLES20.glGetUniformLocation(gl20program, "uScale");
        gl20tr = GLES20.glGetUniformLocation(gl20program, "uTranslate");
        gl20cl = GLES20.glGetUniformLocation(gl20program, "uColor");
    }

    public static int loadShader(int type, String shaderCode)
    {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);
        return shader;
    }

    //End GL20

    public static void DrawVerts(GL11 gl, int texid, int cnt, FloatBuffer texcoord, FloatBuffer vertcoord, ByteBuffer inds, float trax, float tray, float r, float g, float b, float a)
    {
        if (usegles20)
        {
            GLES20.glUseProgram(gl20program);
            int mPositionHandle = GLES20.glGetAttribLocation(gl20program, "vPosition");
            int mTexHandle = GLES20.glGetAttribLocation(gl20program, "vTexCoord");

            GLES20.glEnableVertexAttribArray(mPositionHandle);
            GLES20.glVertexAttribPointer(mPositionHandle, 2, GLES20.GL_FLOAT, false, 0, vertcoord);
            GLES20.glEnableVertexAttribArray(mTexHandle);
            GLES20.glVertexAttribPointer(mTexHandle, 2, GLES20.GL_FLOAT, false, 0, texcoord);
            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texid);
            GLES20.glUniform1i(gl20tx, 0);
            GLES20.glUniform4f(gl20sc, 2.0f / Q3EControlView.orig_width, -2.0f / Q3EControlView.orig_height, 0.0f, 1.0f);
            GLES20.glUniform4f(gl20tr, -Q3EControlView.orig_width / 2.0f + trax, -Q3EControlView.orig_height / 2.0f + tray, 0.0f, 0.0f);
            GLES20.glUniform4f(gl20cl, r, g, b, a);
            GLES20.glDrawElements(GLES20.GL_TRIANGLES, cnt, GLES20.GL_UNSIGNED_BYTE, inds);
            /*if ((!Q3EUtils.q3ei.isQ1) && (!Q3EUtils.q3ei.isD3BFG))*/
            {
                GLES20.glDisableVertexAttribArray(mPositionHandle);
                GLES20.glDisableVertexAttribArray(mTexHandle);
            }
        }
        else
        {
            gl.glColor4f(r, g, b, a);
            gl.glBindTexture(GL10.GL_TEXTURE_2D, texid);
            gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, texcoord);
            gl.glVertexPointer(2, GL10.GL_FLOAT, 0, vertcoord);
            gl.glTranslatef(trax, tray, 0);
            gl.glDrawElements(GL10.GL_TRIANGLES, cnt, GL10.GL_UNSIGNED_BYTE, inds);
            gl.glTranslatef(-trax, -tray, 0);
        }
    }

    public static void DrawVerts_GL1(GL11 gl, int texid, int cnt, FloatBuffer texcoord, FloatBuffer vertcoord, ByteBuffer inds, float trax, float tray, float r, float g, float b, float a)
    {
        gl.glColor4f(r, g, b, a);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, texid);
        gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, texcoord);
        gl.glVertexPointer(2, GL10.GL_FLOAT, 0, vertcoord);
        gl.glTranslatef(trax, tray, 0);
        gl.glDrawElements(GL10.GL_TRIANGLES, cnt, GL10.GL_UNSIGNED_BYTE, inds);
        gl.glTranslatef(-trax, -tray, 0);
    }

    public static int loadGLTexture(GL10 gl, Bitmap bmp)
    {
        if(null == bmp)
            return 0;

        int[] t = new int[1];

        if (usegles20)
        {
            GLES20.glEnable(GLES20.GL_TEXTURE_2D);
            GLES20.glGenTextures(1, t, 0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, t[0]);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        }
        else
        {
            gl.glEnable(GL10.GL_TEXTURE_2D);
            gl.glGenTextures(1, t, 0);
            gl.glBindTexture(GL10.GL_TEXTURE_2D, t[0]);
            gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
            gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
            gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
            gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
        }

        Bitmap powerof2bmp = Bitmap.createScaledBitmap(bmp, Q3EUtils.nextpowerof2(bmp.getWidth()), Q3EUtils.nextpowerof2(bmp.getHeight()), true);

        GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, powerof2bmp, 0);
        bmp.recycle();
        return t[0];
    }

    public static int BitmapToGLTexture(GL10 gl, Bitmap bmp, int texture)
    {
        if(null == bmp)
            return 0;

        if(texture <= 0)
        {
            return loadGLTexture(gl, bmp);
        }
        else
        {
            boolean isTexture = true;
            if (usegles20)
                isTexture = GLES20.glIsTexture(texture);
            if(!isTexture)
                return loadGLTexture(gl, bmp);

            if (usegles20)
            {
                GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
            }
            else
            {
                gl.glBindTexture(GL10.GL_TEXTURE_2D, texture);
            }
            Bitmap powerof2bmp = Bitmap.createScaledBitmap(bmp, Q3EUtils.nextpowerof2(bmp.getWidth()), Q3EUtils.nextpowerof2(bmp.getHeight()), true);

            GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, powerof2bmp, 0);
            bmp.recycle();
            return texture;
        }
    }

    public static void glDeleteTexture(GL10 gl, int texture)
    {
        if(texture <= 0)
            return;
        final int[] ts = { texture };
        if (usegles20)
        {
            GLES20.glDeleteTextures(1, ts, 0);
        }
        else
        {
            gl.glDeleteTextures(1, ts, 0);
        }
    }
}
