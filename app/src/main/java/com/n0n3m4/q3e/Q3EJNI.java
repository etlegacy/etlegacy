/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import android.view.Surface;

public class Q3EJNI {	
	public static native void setCallbackObject(Object obj);

    public static native boolean init(
            String LibPath, // engine's library file path
            String nativeLibPath, // apk's dynamic library directory path
            int width, // surface width
            int height, // surface height
            String GameDir, // game data directory(external)
			String gameSubDir, // game data sub directory(external)
            String Args, // doom3 command line arguments
            Surface view, // render surface
            int format, // OpenGL color buffer format: 0x8888, 0x4444, 0x5551, 0x565
            int msaa, // MSAA: 0, 4, 16
            int glVersion, // OpenGLES verison: 0x00020000, 0x00030000
            boolean redirect_output_to_file, // save runtime log to file
            boolean no_handle_signals, // not handle signals
            boolean multithread, // enable multithread
            boolean usingMouse, // using mouse
			int refreshRate, // refresh rate,
			String appHome, // app home path
			boolean smoothJoystick, // is smooth joystick
            boolean continueNoGLContext
    );
	public static native void drawFrame();
	public static native void sendKeyEvent(int state,int key,int character);
	public static native void sendAnalog(int enable,float x,float y);
	public static native void sendMotionEvent(float x, float y);
	public static native void vidRestart();
    public static native void shutdown();
    public static native boolean Is64();
    public static native void OnPause();
    public static native void OnResume();
    public static native void SetSurface(Surface view);
	public static native void PushKeyEvent(int state, int key, int character);
	public static native void PushAnalogEvent(int enable, float x, float y);
	public static native void PushMotionEvent(float x, float y);
	public static native void PreInit(int eventQueueType);

	static {
		System.loadLibrary("q3eloader");
	}
}

