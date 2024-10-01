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

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import com.n0n3m4.q3e.onscreen.Q3EGUI;

import java.nio.ByteBuffer;
import java.util.LinkedList;

public class Q3ECallbackObj
{
    private static final String TAG = "Q3ECallbackObj";
    public Q3EAudioTrack mAudioTrack;
    public Q3EControlView vw;
    public int state = Q3EGlobals.STATE_NONE;
    private final Object m_audioLock = new Object();
    public static boolean reqThreadrunning = false;

    private Q3EGUI gui;
    private Q3EEventEngine eventEngine = new Q3EEventEngineJava();

    private final LinkedList<Runnable> m_eventQueue = new LinkedList<>();
    public boolean notinmenu = true; // inGaming
    public boolean inLoading = true;
    public boolean inConsole = false;

    public void setState(int newstate)
    {
        state = newstate;
        inConsole = (newstate & Q3EGlobals.STATE_CONSOLE) == Q3EGlobals.STATE_CONSOLE;
        notinmenu = ((newstate & Q3EGlobals.STATE_GAME) == Q3EGlobals.STATE_GAME) && !inConsole;
        inLoading = (newstate & Q3EGlobals.STATE_LOADING) == Q3EGlobals.STATE_LOADING;
        vw.setState(newstate);
    }

    public void pause()
    {
        if (null != mAudioTrack)
            mAudioTrack.Pause();
        reqThreadrunning = false;
    }

    public void resume()
    {
        if (null != mAudioTrack)
            mAudioTrack.Resume();
        reqThreadrunning = true;
    }

    // AudioTrack
    public void init(int size)
    {
		/*                        _.---"'"""""'`--.._
                             _,.-'                   `-._
                         _,."                            -.
                     .-""   ___...---------.._             `.
                     `---'""                  `-.            `.
                                                 `.            \
                                                   `.           \
                                                     \           \
                                                      .           \
                                                      |            .
                                                      |            |
                                _________             |            |
                          _,.-'"         `"'-.._      :            |
                      _,-'                      `-._.'             |
                   _.'            OUYA               `.             '
        _.-.    _,+......__                           `.          .
      .'    `-"'           `"-.,-""--._                 \        /
     /    ,'                  |    __  \                 \      /
    `   ..                       +"  )  \                 \    /
     `.'  \          ,-"`-..    |       |                  \  /
      / " |        .'       \   '.    _.'                   .'
     |,.."--"""--..|    "    |    `""`.                     |
   ,"               `-._     |        |                     |
 .'                     `-._+         |                     |
/                           `.                        /     |
|    `     '                  |                      /      |
`-.....--.__                  |              |      /       |
   `./ "| / `-.........--.-   '              |    ,'        '
     /| ||        `.'  ,'   .'               |_,-+         /
    / ' '.`.        _,'   ,'     `.          |   '   _,.. /
   /   `.  `"'"'""'"   _,^--------"`.        |    `.'_  _/
  /... _.`:.________,.'              `._,.-..|        "'
 `.__.'                                 `._  /
                                           "' */

        if (mAudioTrack != null) return;
        mAudioTrack = Instance(size);
        reqThreadrunning = true;
    }

    public static Q3EAudioTrack Instance(int size)
    {
        return Q3EAudioTrack.Instance(size);
    }

    //k NEW:
// Now call directly by native of libidtech4*.so, and don't need call requestAudioData in Java.
// if offset >= 0 and length > 0, only write.
// if offset >= 0 and length < 0, length = -length, then write and flush.
// If length == 0 and offset < 0, only flush.
    public int writeAudio(ByteBuffer audioData, int offset, int len)
    {
        if (null == mAudioTrack || !reqThreadrunning)
            return 0;
        return mAudioTrack.writeAudio(audioData, offset, len);
    }

    public int writeAudio_array(byte[] audioData, int offset, int len)
    {
        if (null == mAudioTrack || !reqThreadrunning)
            return 0;
        return mAudioTrack.writeAudio(audioData, offset, len);
    }

    public void OnDestroy()
    {
        synchronized (m_audioLock)
        {
            if (null != mAudioTrack)
                mAudioTrack.Shutdown();
            mAudioTrack = null;
            reqThreadrunning = false;
        }
    }

    /**
     * thread event
     * @param num < 0: all; > 0: max; = 0: clear
     * @return event num
     */
    public int PullEvent(int num)
    {
        synchronized (m_eventQueue)
        {
            if (num < 0)
            {
                int i = m_eventQueue.size();
                while (!m_eventQueue.isEmpty())
                    m_eventQueue.removeFirst().run();
                return i; // 0
            }
            else if (num > 0)
            {
                int i = 0;
                while (!m_eventQueue.isEmpty())
                {
                    m_eventQueue.removeFirst().run();
                    i++;
                    if(i >= num)
                        break;
                }
                return i; // m_eventQueue.size();
            }
            else
            {
                m_eventQueue.clear();
                return 0;
            }
        }
    }

    public void PushEvent(Runnable r)
    {
        synchronized (m_eventQueue)
        {
            m_eventQueue.add(r);
        }
    }

    // mouse grab/ungrab
    public void GrabMouse(boolean grab)
    {
        try
        {
            if (grab)
                vw.GrabMouse();
            else
                vw.UnGrabMouse();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void CopyToClipboard(String text)
    {
        Runnable runnable = new Runnable()
        {
            @Override
            public void run()
            {
                Q3EUtils.CopyToClipboard(Q3EMain.gameHelper.GetContext(), text);
            }
        };
        //Q3EMain.mGLSurfaceView.post(runnable);
        runnable.run();
    }

    public String GetClipboardText()
    {
        return Q3EUtils.GetClipboardText(Q3EMain.gameHelper.GetContext());
    }

    public void sendAnalog(final boolean down, final float x, final float y)
    {
        eventEngine.SendAnalogEvent(down, x, y);
    }

    public void sendKeyEvent(final boolean down, final int keycode, final int charcode)
    {
        eventEngine.SendKeyEvent(down, keycode, charcode);
    }

    public void sendMotionEvent(final float deltax, final float deltay)
    {
        eventEngine.SendMotionEvent(deltax, deltay);
    }

    public void InitGUIInterface(Activity context)
    {
        gui = new Q3EGUI(context);
        int eventQueue = Q3EPreference.GetIntFromString(context, Q3EPreference.EVENT_QUEUE, 0);
        if(eventQueue == 1)
        {
            Log.i(TAG, "Using native event queue");
            eventEngine = new Q3EEventEngineNative();
        }
        else
        {
            Log.i(TAG, "Using java event queue");
            eventEngine = new Q3EEventEngineJava();
        }
    }

    public void ShowToast(String text)
    {
        //if(null != gui)
            gui.Toast(text);
    }

    public void CloseVKB()
    {
        vw.post(new Runnable() {
            @Override
            public void run() {
                Q3EUtils.CloseVKB(vw);
            }
        });
    }

    public void OpenVKB()
    {
        vw.post(new Runnable() {
            @Override
            public void run() {
                Q3EUtils.OpenVKB(vw);
            }
        });
    }

    public void ToggleToolbar(boolean on)
    {
        vw.ToggleToolbar(on);
    }


    public void OpenURL(String url)
    {
        vw.post(new Runnable()
        {
            @Override
            public void run()
            {
                Uri uri = Uri.parse(url);
                Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                vw.getContext().startActivity(intent);
            }
        });
    }

    public int OpenDialog(String title, String message, String[] buttons)
    {
        //if(null != gui)
            return gui.MessageDialog(title, message, buttons);
        //else return Q3EGUI.DIALOG_ERROR;
    }

    public void Finish()
    {
        if(null == Q3E.activity || Q3E.activity.isFinishing())
            return;
        Q3E.activity.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                Q3E.Finish();
            }
        });
    }

    public void SetupSmoothJoystick(boolean enable)
    {
        Q3EUtils.q3ei.joystick_smooth = enable;
    }
}

