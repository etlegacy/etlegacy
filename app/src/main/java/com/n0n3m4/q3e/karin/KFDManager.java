package com.n0n3m4.q3e.karin;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.io.FileFilter;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public final class KFDManager
{
    private static final String TAG = "KFDManager";
    @SuppressLint("StaticFieldLeak")
    private static KFDManager _instance = null;
    private Context m_context;
    private final List<String> m_searchPaths = new ArrayList<>();

    private KFDManager(Context context)
    {
        SetContext(context);
    }

    public String[] GetSearchPaths()
    {
        return m_searchPaths.toArray(new String[0]);
    }

    public String[] GetSearchPathFolders()
    {
        List<String> list = new ArrayList<>();
        for (String sp : m_searchPaths)
        {
            if(!sp.endsWith(Q3EGlobals.IDTECH4AMM_PAK_SUFFIX))
                list.add(sp);
        }
        return list.toArray(new String[0]);
    }

    public static KFDManager Instance(Context context)
    {
        if(null == _instance)
            _instance = new KFDManager(context);
        else if(context != _instance.m_context)
            _instance.SetContext(context);
        return _instance;
    }

    private void SetContext(Context context)
    {
        m_context = context;
        InitSearchPaths();
    }

    // pref_datapath > /sdcard/Android/data > /data/user > /sdcard/diii4a > apk::/android_asset
    private void InitSearchPaths()
    {
        final String DefPath = Environment.getExternalStorageDirectory() + "/diii4a";
        m_searchPaths.clear();
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        final String[] Paths = {
                preferences.getString(Q3EPreference.pref_datapath, DefPath),
                Q3EUtils.GetAppStoragePath(m_context, null),
                Q3EUtils.GetAppInternalPath(m_context, null),
                //DefPath,
                "/android_asset"
        };

        for (String path : Paths)
        {
            if(!m_searchPaths.contains(path))
            {
                m_searchPaths.add(path);
                List<String> strings;
                if("/android_asset".equals(path))
                    strings = ListPakPath_androidasset("");
                else
                    strings = ListPakPath_filesystem(path);
                m_searchPaths.addAll(strings);
            }
        }

        Log.d(TAG, "Current search paths:");
        for (int i = 0; i < m_searchPaths.size(); i++)
            Log.d(TAG, (i + 1) + ": " + m_searchPaths.get(i));
    }

    private List<String> ListPakPath_filesystem(String path)
    {
        List<String> list = new ArrayList<>();
        File dir = new File(path);
        File[] files = dir.listFiles(new SuffixFileFilter(Q3EGlobals.IDTECH4AMM_PAK_SUFFIX));
        if(null != files)
        {
            for (File file : files)
            {
                list.add(file.getAbsolutePath());
            }
        }
        return list;
    }

    private List<String> ListPakPath_androidasset(String path)
    {
        List<String> list = new ArrayList<>();
        try
        {
            String[] files = m_context.getAssets().list(path);
            if(null != files)
            {
                for (String file : files)
                {
                    if(file.toLowerCase().endsWith(Q3EGlobals.IDTECH4AMM_PAK_SUFFIX))
                        list.add("/android_asset/" + file);
                }
            }
        }
        catch (Exception ignored) {}
        return list;
    }

    public KFileDescriptor File(String path)
    {
        return new KFileDescriptor(m_context, m_searchPaths, path);
    }

    public InputStream OpenRead(String path)
    {
        return File(path).OpenRead();
    }

    public List<String> ListDir(String path)
    {
        return File(path).ListDir();
    }


    public static class FolderFileFilter implements FileFilter
    {
        @Override
        public boolean accept(File pathname)
        {
            return pathname.isDirectory();
        }
    }

    public static class SuffixFileFilter implements FileFilter
    {
        private final String m_suffix;

        public SuffixFileFilter(String suffix)
        {
            this.m_suffix = suffix;
        }

        @Override
        public boolean accept(File pathname)
        {
            return pathname.isFile() && pathname.getName().toLowerCase().endsWith(m_suffix.toLowerCase());
        }
    }
}
