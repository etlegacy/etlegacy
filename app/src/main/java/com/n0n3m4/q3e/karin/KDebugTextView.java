package com.n0n3m4.q3e.karin;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Debug;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.widget.TextView;

import java.util.Timer;
import java.util.TimerTask;

public class KDebugTextView extends android.support.v7.widget.AppCompatTextView {
    private MemDumpFunc m_memFunc = null;

    @SuppressLint("ResourceType")
    public KDebugTextView(Context context)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setTextColor(Color.WHITE);
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23
            setTextAppearance(android.R.attr.textAppearanceMedium);
        setPadding(10, 5, 10, 5);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            setAlpha(0.75f);
        }
        setTypeface(Typeface.MONOSPACE);
        m_memFunc = new
                MemDumpFunc_timer
                //MemDumpFunc_handler
                (this);
    }

    public void Start(int interval)
    {
        if(m_memFunc != null && interval > 0)
            m_memFunc.Start(interval);
    }

    public void Stop()
    {
        if(m_memFunc != null)
            m_memFunc.Stop();
    }

    private abstract class MemDumpFunc
    {
        private boolean  m_lock = false;
        private ActivityManager m_am = null;
        private final int[] m_processs = {Process.myPid()};
        private final ActivityManager.MemoryInfo m_outInfo = new ActivityManager.MemoryInfo();
        private final KMemoryInfo m_memoryInfo = new KMemoryInfo();
        private final TextView m_memoryUsageText;
        protected Runnable m_runnable = new Runnable() {
            @Override
            public void run()
            {
                if (IsLock())
                    return;
                Lock();
                final String text = GetMemText();
                HandleMemText(text);
            }
        };

        public MemDumpFunc(TextView view)
        {
            m_memoryUsageText = view;
        }

        public void Start(int interval)
        {
            Stop();
            m_am = (ActivityManager)getContext().getSystemService(Context.ACTIVITY_SERVICE);
            Unlock();
        }

        public void Stop()
        {
            Unlock();
        }

        private void Lock()
        {
            m_lock = true;
        }

        private void Unlock()
        {
            m_lock = false;
        }

        private boolean IsLock()
        {
            return m_lock;
        }

        private String GetMemText()
        {
            m_am.getMemoryInfo(m_outInfo);
            m_memoryInfo.Invalid();
            m_memoryInfo.Get(m_am, m_processs, m_outInfo);
            m_memoryInfo.Mb();

            long app_total_used = m_memoryInfo.native_memory + m_memoryInfo.java_memory;
            String total_used_str = m_memoryInfo.graphics_memory >= 0 ? "" + (app_total_used + m_memoryInfo.graphics_memory) : (app_total_used + "(Excluding graphics memory)");
            String graphics_mem_str = m_memoryInfo.graphics_memory >= 0 ? "" + m_memoryInfo.graphics_memory : "<unknown>";
            int percent = (int)Math.round(((double) m_memoryInfo.used_memory / (double)m_memoryInfo.total_memory) * 100);
            long availMem = m_memoryInfo.total_memory - m_memoryInfo.used_memory;

            String sb = "App:"
                    + "Dalvik(" + m_memoryInfo.java_memory + ")+"
                    + "Native(" + m_memoryInfo.native_memory + ")+"
                    + "Graphics(" + graphics_mem_str + ")"
                    + "≈" + total_used_str + "\n"
                    + "Sys:"
                    + "Used(" + m_memoryInfo.used_memory + ")/"
                    + "Total(" + m_memoryInfo.total_memory + ")"
                    + "≈" + percent + "%"
                    + "-=" + availMem
                    ;
            return sb;
        }

        private void HandleMemText(final String text)
        {
            m_memoryUsageText.post(new Runnable(){
                public void run()
                {
                    m_memoryUsageText.setText(text);
                    Unlock();
                }
            });
        }
    }

    private class MemDumpFunc_timer extends MemDumpFunc
    {
        private Timer m_timer = null;

        public MemDumpFunc_timer(TextView view)
        {
            super(view);
        }

        @Override
        public void Start(int interval)
        {
            super.Start(interval);
            TimerTask task = new TimerTask(){
                @Override
                public void run()
                {
                    m_runnable.run();
                }
            };

            m_timer = new Timer();
            m_timer.scheduleAtFixedRate(task, 0, interval);
        }

        @Override
        public void Stop()
        {
            super.Stop();
            if(m_timer != null)
            {
                m_timer.cancel();
                m_timer.purge();
                m_timer = null;
            }
        }
    }

    private class MemDumpFunc_handler extends MemDumpFunc
    {
        private HandlerThread m_thread = null;
        private Handler m_handler = null;
        private Runnable m_handlerCallback = null;

        public MemDumpFunc_handler(TextView view)
        {
            super(view);
        }

        @Override
        public void Start(final int interval)
        {
            super.Start(interval);
            m_thread = new HandlerThread("MemDumpFunc_thread");
            m_thread.start();
            m_handler = new Handler(m_thread.getLooper());
            m_handlerCallback = new Runnable(){
                public void run()
                {
                    m_runnable.run();
                    m_handler.postDelayed(m_handlerCallback, interval);
                }
            };
            m_handler.post(m_handlerCallback);
        }

        @Override
        public void Stop()
        {
            super.Stop();
            if(m_handler != null)
            {
                if(m_handlerCallback != null)
                {
                    m_handler.removeCallbacks(m_handlerCallback);
                    m_handlerCallback = null;
                }
                m_handler = null;
            }
            if(m_thread != null)
            {
                m_thread.quit();
                m_thread = null;
            }
        }
    }
}
