package com.n0n3m4.q3e;

import com.n0n3m4.q3e.karin.KOnceRunnable;

public interface Q3EEventEngine
{
	public void SendKeyEvent(final boolean down, final int keycode, final int charcode);
	public void SendMotionEvent(final float deltax, final float deltay);
	public void SendAnalogEvent(final boolean down, final float x, final float y);
}

class Q3EEventEngineNative implements Q3EEventEngine
{
	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
		Q3EJNI.PushKeyEvent(down ? 1 : 0, keycode, charcode);
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
		Q3EJNI.PushMotionEvent(deltax, deltay);
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
		Q3EJNI.PushAnalogEvent(down ? 1 : 0, x, y);
	}
}

class Q3EEventEngineJava implements Q3EEventEngine
{
	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
/*		Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendKeyEvent(down ? 1 : 0, keycode, charcode);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new KeyEventRunnable(down, keycode, charcode));
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendMotionEvent(deltax, deltay);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new MotionEventRunnable(deltax, deltay));
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
/*		Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendAnalog(down ? 1 : 0, x, y);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new AnalogEventRunnable(down, x, y));
	}

	private static class KeyEventRunnable extends KOnceRunnable
	{
		public final boolean down;
		public final int     keycode;
		public final int     charcode;

		public KeyEventRunnable(boolean down, int keycode, int charcode)
		{
			this.down = down;
			this.keycode = keycode;
			this.charcode = charcode;
		}

		@Override
		protected void Run()
		{
			Q3EJNI.sendKeyEvent(down ? 1 : 0, keycode, charcode);
		}
	}

	private static class MotionEventRunnable extends KOnceRunnable
	{
		public final float deltax;
		public final float deltay;

		public MotionEventRunnable(float deltax, float deltay)
		{
			this.deltax = deltax;
			this.deltay = deltay;
		}

		@Override
		protected void Run()
		{
			Q3EJNI.sendMotionEvent(deltax, deltay);
		}
	}

	private static class AnalogEventRunnable extends KOnceRunnable
	{
		public final boolean down;
		public final float   x;
		public final float   y;

		public AnalogEventRunnable(boolean down, float x, float y)
		{
			this.down = down;
			this.x = x;
			this.y = y;
		}

		@Override
		protected void Run()
		{
			Q3EJNI.sendAnalog(down ? 1 : 0, x, y);
		}
	}
}
