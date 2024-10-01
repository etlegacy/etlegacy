package com.n0n3m4.DIII4A.launcher;

import android.content.Context;
import android.os.Bundle;
import android.os.Parcelable;
import android.widget.Toast;

import com.n0n3m4.DIII4A.GameLauncher;

public abstract class GameLauncherFunc implements Runnable
{
    public static final String CONST_RESULT_KEY = "result";

    protected final GameLauncher m_gameLauncher;
    protected Bundle m_data;
    protected Runnable m_callback;

    public GameLauncherFunc(GameLauncher gameLauncher)
    {
        this.m_gameLauncher = gameLauncher;
    }

    public GameLauncherFunc(GameLauncher gameLauncher, Runnable runnable)
    {
        this(gameLauncher);
        m_callback = runnable;
    }

    public void Reset()
    {
    }

    public void SetCallback(Runnable runnable)
    {
        m_callback = runnable;
    }

    protected void Callback()
    {
        if(null != m_callback)
            m_callback.run();
    }

    protected void Callback(String result)
    {
        SetResult(result);
        Callback();
    }

    protected void Callback(boolean result)
    {
        SetResult(result);
        Callback();
    }

    protected void Callback(Parcelable result)
    {
        SetResult(result);
        Callback();
    }

    public void Start(Bundle data)
    {
        m_data = data;
    }

    protected void Toast_long(String str)
    {
        Toast.makeText(m_gameLauncher, str, Toast.LENGTH_LONG).show();
    }

    protected void Toast_short(String str)
    {
        Toast.makeText(m_gameLauncher, str, Toast.LENGTH_SHORT).show();
    }

    protected void Toast_long(int resId)
    {
        Toast.makeText(m_gameLauncher, resId, Toast.LENGTH_LONG).show();
    }

    protected void Toast_short(int resId)
    {
        Toast.makeText(m_gameLauncher, resId, Toast.LENGTH_SHORT).show();
    }

    public Bundle GetData()
    {
        return m_data;
    }

    protected void SetResult(Parcelable object)
    {
        if(null != m_data)
            m_data.putParcelable(CONST_RESULT_KEY, object);
    }

    protected void SetResult(String object)
    {
        if(null != m_data)
            m_data.putString(CONST_RESULT_KEY, object);
    }

    protected void SetResult(boolean object)
    {
        if(null != m_data)
            m_data.putBoolean(CONST_RESULT_KEY, object);
    }

    public <T> T GetResult()
    {
        return GetResultFromBundle(m_data);
    }

    public static <T> T GetResultFromBundle(Bundle data)
    {
        return null != data ? (T)data.get(CONST_RESULT_KEY) : null;
    }


    public Context getContext()
    {
        return m_gameLauncher;
    }
}
