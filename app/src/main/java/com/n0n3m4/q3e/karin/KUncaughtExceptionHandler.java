package com.n0n3m4.q3e.karin;

import android.annotation.SuppressLint;
import android.content.Context;

import java.io.File;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import android.content.SharedPreferences;
import android.os.Build;
import android.preference.PreferenceManager;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;

/**
 * Unexpected exception handler
 */
public class KUncaughtExceptionHandler implements Thread.UncaughtExceptionHandler
{
    public static final String CONST_PREFERENCE_APP_CRASH_INFO = "_APP_CRASH_INFO";
    public static final String CONST_PREFERENCE_EXCEPTION_DEBUG = "_EXCEPTION_DEBUG";

    private Context m_context;
    private Thread.UncaughtExceptionHandler m_originHandler;
    @SuppressLint("StaticFieldLeak")
    private static KUncaughtExceptionHandler _instance = null;
    
    private KUncaughtExceptionHandler() {}

    @Override
    public void uncaughtException(Thread t, Throwable e) {
/*
        if(null == m_context)
        {
            DefaultHandleException(t, e);
            return;
        }
*/
        try
        {
            String str = ExceptionStr(m_context, t, e);
            WriteToPreference(str);
            WriteToStorage(str);
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
        finally {
            DefaultHandleException(t, e);
        }
    }

    private void DefaultHandleException(Thread t, Throwable e)
    {
        try
        {
            if(null != m_originHandler)
                m_originHandler.uncaughtException(t, e);
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
    }
    
    public static String ExceptionStr(Context context, Thread t, Throwable e)
    {
            StringBuilder sb = new StringBuilder();
            StackTraceElement[] arr = e.getStackTrace();

            sb.append("********** DUMP **********\n");
            sb.append("----- Time: " + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date())).append('\n');
            sb.append('\n');

            sb.append("----- Thread: " + t).append('\n');
            sb.append("\tID: " + t.getId()).append('\n');
            sb.append("\tName: " + t.getName()).append('\n');
            sb.append('\n');

            sb.append("----- Throwable: " + e).append('\n');
            sb.append("\tInfo: " + e.getMessage()).append('\n');
            sb.append("\tStack: ").append('\n');
            for(StackTraceElement ste : arr)
            {
                sb.append("\t\t" + ste.toString()).append('\n');
            }
            sb.append('\n');

            sb.append("----- Memory:").append('\n');
            KMemoryInfo outInfo = new KMemoryInfo();
            if(null != context)
                outInfo.Get(context);

            sb.append("\tSystem: ").append('\n');
            sb.append("\t\tAvail: " + outInfo.avail_memory + " bytes").append('\n');
            sb.append("\t\tTotal: " + outInfo.total_memory + " bytes").append('\n');

            sb.append("\tApplication: ").append('\n');
            sb.append("\t\tNative heap: " + outInfo.native_memory + " bytes").append('\n');
            sb.append("\t\tDalvik heap: " + outInfo.java_memory + " bytes").append('\n');
            sb.append("\t\tGraphics: " + outInfo.graphics_memory + " bytes").append('\n');
            sb.append("\t\tStack: " + outInfo.stack_memory + " bytes").append('\n');
            sb.append('\n');

            sb.append("********** END **********\n");
            sb.append("Application exit.\n");
            return sb.toString();
    }
    
    public static void DumpException(Context context, Thread t, Throwable e) {
        try
        {
            String str = ExceptionStr(context, t, e);
            SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(context).edit();
            editor.putString(CONST_PREFERENCE_EXCEPTION_DEBUG, str);
            editor.commit();
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
    }

    private String GetDumpPath()
    {
        if(null == m_context)
            return null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            return m_context.getDataDir().getAbsolutePath();
        else
            return m_context.getCacheDir().getAbsolutePath();
    }

    private String GetExceptionDumpFile()
    {
        String dir = GetDumpPath();
        if(null == dir)
            return null;
        return dir + "/" + CONST_PREFERENCE_APP_CRASH_INFO;
    }

    public static KUncaughtExceptionHandler Instance()
    {
        return _instance;
    }

    public static String GetDumpExceptionContent()
    {
        if(null == _instance)
            return "";
        try
        {
            String str;

            str = Q3EUtils.file_get_contents(_instance.GetExceptionDumpFile());
            // str = PreferenceManager.getDefaultSharedPreferences(_instance.m_context).getString(KUncaughtExceptionHandler.CONST_PREFERENCE_APP_CRASH_INFO, null);
            return null != str ? str : "";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return "";
        }
    }

    public static boolean ClearDumpExceptionContent()
    {
        if(null == _instance)
            return false;
        try
        {
            return Q3EUtils.rm(_instance.GetExceptionDumpFile());
            //return PreferenceManager.getDefaultSharedPreferences(_instance.m_context).edit().remove(KUncaughtExceptionHandler.CONST_PREFERENCE_APP_CRASH_INFO).commit();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    private boolean WriteToPreference(String str)
    {
        if(null == m_context)
            return false;
        try
        {
            Q3EUtils.file_put_contents(GetExceptionDumpFile(), str);
/*            SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(m_context).edit();
            editor.putString(CONST_PREFERENCE_APP_CRASH_INFO, str);
            editor.commit();*/
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    private boolean WriteToStorage(String str)
    {
        FileWriter out = null;
        try
        {
            String fileName = String.format("%s_%s.crash.log", Q3EGlobals.CONST_APP_NAME, new SimpleDateFormat("yyyy-MM-dd HH-mm-ss-SSS").format(new Date()));
            String logPath = Q3EUtils.q3ei.app_storage_path + "/logs";
            File dir = new File(logPath);
            if(!dir.exists())
                dir.mkdirs();
            out = new FileWriter(logPath + "/" + fileName);
            out.append(str);
            out.flush();
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        finally
        {
            try
            {
                if(null != out)
                    out.close();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }
        return false;
    }

    private void Register(Context context)
    {
        if(null != m_context)
            return;
        if(null == context)
            return;
        m_context = context;
        m_originHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(_instance);
    }

    private void Unregister()
    {
        if(null == m_context)
            return;
        Thread.setDefaultUncaughtExceptionHandler(m_originHandler);
        m_context = null;
        m_originHandler = null;
    }

    public static void HandleUnexpectedException(Context context)
    {
        try
        {
            if(null == _instance)
                _instance = new KUncaughtExceptionHandler();
            else
                _instance.Unregister();
            _instance.Register(context);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
