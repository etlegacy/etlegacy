package com.n0n3m4.q3e.karin;

import com.n0n3m4.q3e.Q3EUtils;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public final class KLogcat
{
    private KLogcatThread m_thread;
    private String m_command = "logcat";

    public void SetCommand(String str)
    {
        m_command = str;
    }

    public void Start(KLogcatCallback callback)
    {
        Stop();
        m_thread = new KLogcatThread();
        m_thread.SetCallback(callback);
        m_thread.Start(m_command);
    }

    public void Stop()
    {
        if(null != m_thread)
        {
            m_thread.Stop();
            m_thread = null;
        }
    }

    public boolean IsRunning()
    {
        return null != m_thread && m_thread.m_running;
    }

    public interface KLogcatCallback
    {
        public void Output(String str);
    }

    private static class KLogcatThread extends Thread
    {
        private boolean m_running = false;
        private KLogcatCallback m_callback;
        private String m_command = "logcat";

        @Override
        public void start()
        {
            if(m_running)
                return;
            m_running = true;
            super.start();
        }

        public void Start(String cmd)
        {
            if(m_running)
                Stop();
            if(null != cmd && !cmd.isEmpty())
                m_command = cmd;
            this.start();
        }

        public void Stop()
        {
            m_running = false;
            interrupt();
        }

        public void SetCallback(KLogcatCallback callback)
        {
            m_callback = callback;
        }

        @Override
        public void run()
        {
            int sleepCount = 0;
            Process logcatProc = null;
            BufferedReader mReader = null;
            try
            {
                final String[] cmd = {"/bin/sh", "-c", m_command};
                logcatProc = Runtime.getRuntime().exec(cmd);
                mReader = new BufferedReader(new InputStreamReader(logcatProc.getInputStream(), StandardCharsets.UTF_8), 1024);
                String line;
                while (m_running)
                {
                    line = mReader.readLine();
                    if(null == line || line.isEmpty())
                    {
                        sleep(1000);
                        sleepCount++;
                        continue;
                    }
                    if(null != m_callback)
                        m_callback.Output(line);
                }
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
            finally
            {
                m_running = false;
                if (null != logcatProc)
                {
                    if (logcatProc.isAlive())
                        logcatProc.destroy();
                }
                Q3EUtils.Close(mReader);
            }
        }
    }
}
