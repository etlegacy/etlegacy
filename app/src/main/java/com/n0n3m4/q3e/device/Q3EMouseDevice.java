package com.n0n3m4.q3e.device;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import com.n0n3m4.q3e.Q3ECallbackObj;
import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.FileInputStream;
import java.io.FileOutputStream;

public class Q3EMouseDevice
{
    private final Q3EControlView view;

    private String mouse_name = null;
    private int readmouse_dx = 0;
    private int readmouse_dy = 0;
    private int readmouse_keycode = 0;
    private int readmouse_keystate = 0;
    private boolean qevnt_available = true;
    private int mouse_corner = 3;

    //MOUSE
    public static String detectedtmp;
    public static String detectedname;
    public static final String detecthnd = "Handlers=";
    public static final String detectmouse = "mouse";
    public static final String detectrel = "REL=";
    public static boolean detectfoundpreferred = false;

    public Q3EMouseDevice(Q3EControlView context)
    {
        view = context;
    }

    public void Init()
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(view.getContext());
        boolean detectMouse = mPrefs.getBoolean(Q3EPreference.pref_detectmouse, true);
        //k: first check su
        mouse_name = DeviceIsRoot() ? (detectMouse ? detectmouse() : mPrefs.getString(Q3EPreference.pref_eventdev, "/dev/input/event???")) : null;
        mouse_corner = mPrefs.getInt(Q3EPreference.pref_mousepos, 3);
    }

    public void Start()
    {
        if (mouse_name != null)
        {
            readmouse.setPriority(7);
            readmouse.start();
        }
    }

    public void Stop()
    {
        if (mouse_name != null)
        {
            readmouse.interrupt();
        }
    }

    public static boolean DeviceIsRoot()
    {
        try
        {
            // return com.stericson.RootTools.RootTools.isRootAvailable();
            Class<?> clazz = Class.forName("com.stericson.RootTools.RootTools");
            return (boolean)clazz.getDeclaredMethod("isRootAvailable").invoke(null);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    // comment these for pure-free
    public Thread readmouse = new Thread(new Runnable()
    {
        @Override
        public void run()
        {
            try
            {
                /*
                com.stericson.RootTools.CommandCapture command = new com.stericson.RootTools.CommandCapture(0, "chmod 777 " + mouse_name);//insecure =(
                com.stericson.RootTools.RootTools.getShell(true).add(command).waitForFinish();
                 */
                Object command = Class.forName("com.stericson.RootTools.CommandCapture").getDeclaredConstructor(int.class, String[].class).newInstance(0, new String[]{"chmod 777 " + mouse_name});//insecure =(
                Object shell = Class.forName("com.stericson.RootTools.RootTools").getDeclaredMethod("getShell", boolean.class).invoke(null, true);
                command = shell.getClass().getDeclaredMethod("add", Class.forName("com.stericson.RootTools.Command")).invoke(shell, command);
                command.getClass().getDeclaredMethod("waitForFinish").invoke(command);

                FileInputStream fis = new FileInputStream(mouse_name);//.getChannel();
                FileOutputStream fout = new FileOutputStream(mouse_name);//.getChannel();
                final int sizeofstruct = 8 + 2 + 2 + 4;
                byte[] arr = new byte[sizeofstruct];
                byte xcornr = (mouse_corner % 2 == 0) ? (byte) -127 : 127;
                byte ycornr = (mouse_corner < 2) ? (byte) -127 : 127;
                byte xargs = (xcornr < 0) ? (byte) -1 : 0;
                byte yargs = (ycornr < 0) ? (byte) -1 : 0;
                byte[] narr = {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, xcornr, xargs, xargs, xargs,
                        0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, ycornr, yargs, yargs, yargs,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0};
                Runnable qevnt_runnable = new Runnable()
                {
                    @Override
                    public void run()
                    {
                        Q3EJNI.sendMotionEvent(readmouse_dx, readmouse_dy);
                        readmouse_dx = 0;
                        readmouse_dy = 0;
                        qevnt_available = true;
                    }
                };
                Runnable qkeyevnt_runnable = new Runnable()
                {
                    @Override
                    public void run()
                    {
                        Q3EJNI.sendKeyEvent(readmouse_keystate, readmouse_keycode, 0);
                    }
                };
                while (fis.read(arr, 0, sizeofstruct) != -1)
                {
                    if (!Q3ECallbackObj.reqThreadrunning)
                    {
                        Thread.yield();
                        continue;
                    }

                    if ((arr[sizeofstruct - 4] == 127) || (arr[sizeofstruct - 4] == -127)) continue;

                    if (arr[sizeofstruct - 8] == 0)
                    {
                        if (qevnt_available)
                        {
                            qevnt_available = false;
                            Q3EUtils.q3ei.callbackObj.PushEvent(qevnt_runnable);
                        }
                        fout.write(narr);
                    }
                    if (arr[sizeofstruct - 8] == 1)
                    {
                        readmouse_keycode = 0;
                        if (arr[sizeofstruct - 6] == 16)
                            readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MOUSE1;
                        if (arr[sizeofstruct - 6] == 17)
                            readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MOUSE2;
                        if (arr[sizeofstruct - 6] == 18)
                            readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MOUSE3;
                        if (arr[sizeofstruct - 6] == 19)
                            readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MOUSE4;
                        if (arr[sizeofstruct - 6] == 20)
                            readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MOUSE5;

                        readmouse_keystate = arr[sizeofstruct - 4];
                        if (readmouse_keycode != 0)
                            Q3EUtils.q3ei.callbackObj.PushEvent(qkeyevnt_runnable);
                    }

                    if (arr[sizeofstruct - 8] == 2)
                    {
                        if ((arr[sizeofstruct - 6]) == 0) readmouse_dx += arr[sizeofstruct - 4];
                        if ((arr[sizeofstruct - 6]) == 1) readmouse_dy += arr[sizeofstruct - 4];
                        if ((arr[sizeofstruct - 6]) == 8)
                        {
                            if (arr[sizeofstruct - 4] == 1)
                                readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MWHEELUP;
                            else
                                readmouse_keycode = Q3EKeyCodes.KeyCodes.K_MWHEELDOWN;
                            readmouse_keystate = 1;
                            Q3EUtils.q3ei.callbackObj.PushEvent(qkeyevnt_runnable);
                            Thread.sleep(25);
                            readmouse_keystate = 0;
                            Q3EUtils.q3ei.callbackObj.PushEvent(qkeyevnt_runnable);
                        }
                    }
                }

            } catch (Throwable ignored) {}
        }
    });

    public static String detectmouse()
    {
        try
        {
            /*com.stericson.RootTools.Command command = new com.stericson.RootTools.Command(0, "cat /proc/bus/input/devices")
            {
                @Override
                public void output(int id, String line)
                {
                    if (line == null) return;
                    if (line.contains(detecthnd) && (line.contains(detectmouse) || !detectfoundpreferred))
                    {
                        detectedtmp = line.substring(line.indexOf(detecthnd) + detecthnd.length());
                        detectedtmp = detectedtmp.substring(detectedtmp.indexOf("event"));
                        if (detectedtmp.contains(" "))
                            detectedtmp = detectedtmp.substring(0, detectedtmp.indexOf(" "));
                        detectfoundpreferred = line.contains(detectmouse);
                    }
                    if (line.contains(detectrel))
                    {
                        detectedname = "/dev/input/" + detectedtmp;
                    }
                }
            };
            com.stericson.RootTools.RootTools.getShell(true).add(command).waitForFinish();*/
            Object command = Class.forName("com.stericson.RootTools.CommandCapture").getDeclaredConstructor(int.class, String[].class).newInstance(0, new String[]{"cat /proc/bus/input/devices"});//insecure =(
            Object shell = Class.forName("com.stericson.RootTools.RootTools").getDeclaredMethod("getShell", boolean.class).invoke(null, true);
            command = shell.getClass().getDeclaredMethod("add", Class.forName("com.stericson.RootTools.Command")).invoke(shell, command);
            command.getClass().getDeclaredMethod("waitForFinish").invoke(command);
            String line = command.toString();
            if (!line.isEmpty())
            {
                int index = line.indexOf("\n");
                if(index != -1)
                    line = line.substring(0, index);
                if (line.contains(detecthnd) && (line.contains(detectmouse) || !detectfoundpreferred))
                {
                    detectedtmp = line.substring(line.indexOf(detecthnd) + detecthnd.length());
                    detectedtmp = detectedtmp.substring(detectedtmp.indexOf("event"));
                    if (detectedtmp.contains(" "))
                        detectedtmp = detectedtmp.substring(0, detectedtmp.indexOf(" "));
                    detectfoundpreferred = line.contains(detectmouse);
                }
                if (line.contains(detectrel))
                {
                    detectedname = "/dev/input/" + detectedtmp;
                }
            }
            return detectedname;
        } catch (Throwable t)
        {
            t.printStackTrace();
            return null;
        }
    }
}
