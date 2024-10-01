package com.n0n3m4.q3e.karin;

import android.content.Context;
import android.util.Log;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class KFileDescriptor
{
    private static final String TAG = "KFileDescriptor";
    private Context m_context;
    private String m_path;
    private List<String> m_searchPaths;

    KFileDescriptor(Context context, List<String> searchPaths, String path)
    {
        m_context = context;
        m_path = path;
        m_searchPaths = searchPaths;
    }

    public InputStream OpenRead()
    {
        InputStream is = null;
        if(m_path.startsWith("/"))
        {
            is = OpenRead_filesystem(m_path);
            if(null != is)
                Log.i(TAG, "Open file in filesystem: " + m_path);
        }
        else
        {
            for (String searchPath : m_searchPaths)
            {
                if("/android_asset".equals(searchPath))
                {
                    is = OpenRead_androidasset(m_path);
                    if(null != is)
                    {
                        Log.i(TAG, "Open file in android_asset: " + m_path);
                        break;
                    }
                }
                else if(searchPath.endsWith(Q3EGlobals.IDTECH4AMM_PAK_SUFFIX))
                {
                    if(searchPath.startsWith("/android_asset/"))
                        is = OpenRead_androidasset_zip(searchPath.substring("/android_asset/".length()), m_path);
                    else
                        is = OpenRead_filesystem_zip(searchPath, m_path);
                    if(null != is)
                    {
                        Log.i(TAG, "Open file in zip " + searchPath + ": " + m_path);
                        break;
                    }
                }
                else
                {
                    is = OpenRead_filesystem(searchPath + "/" + m_path);
                    if(null != is)
                    {
                        Log.i(TAG, "Open file in " + searchPath + ": " + m_path);
                        break;
                    }
                }
            }
        }
        return is;
    }

    public List<String> ListDir()
    {
        List<String> list = new ArrayList<>();
        if(m_path.startsWith("/"))
        {
            list.addAll(ListDir_filesystem(m_path));
        }
        else
        {
            for (String searchPath : m_searchPaths)
            {
                if("/android_asset".equals(searchPath))
                {
                    List<String> strings = ListDir_androidasset(m_path);
                    for (String file : strings)
                    {
                        if(!list.contains(file))
                            list.add(file);
                    }
                }
                else if(searchPath.endsWith(Q3EGlobals.IDTECH4AMM_PAK_SUFFIX))
                {
                    List<String> strings;
                    if(searchPath.startsWith("/android_asset/"))
                        strings = ListDir_androidasset_zip(searchPath.substring("/android_asset/".length()), m_path);
                    else
                        strings = ListDir_filesystem_zip(searchPath, m_path);
                    for (String file : strings)
                    {
                        if(!list.contains(file))
                            list.add(file);
                    }
                }
                else
                {
                    List<String> strings = ListDir_filesystem(searchPath + "/" + m_path);
                    for (String file : strings)
                    {
                        if(!list.contains(file))
                            list.add(file);
                    }
                }
            }
        }
        return list;
    }

    private List<String> ListDir_filesystem(String path)
    {
        List<String> list = new ArrayList<>();
        File dir = new File(path);
        if(dir.isDirectory())
        {
            File[] files = dir.listFiles(new KFDManager.FolderFileFilter());
            if(null != files)
            {
                for (File file : files)
                    list.add(file.getName());
            }
        }
        return list;
    }

    private List<String> ListDir_androidasset(String path)
    {
        List<String> list = new ArrayList<>();
        try
        {
            String[] files = m_context.getAssets().list(path);
            if(null != files)
            {
                list.addAll(Arrays.asList(files));
            }
        }
        catch (Exception ignored) {}
        return list;
    }

    private List<String> ListDir_filesystem_zip(String fspath, String path)
    {
        return ListDir_inputstream_zip(OpenRead_filesystem(fspath), path);
    }

    private List<String> ListDir_androidasset_zip(String fspath, String path)
    {
        return ListDir_inputstream_zip(OpenRead_androidasset(fspath), path);
    }

    private List<String> ListDir_inputstream_zip(InputStream is, String path)
    {
        List<String> list = new ArrayList<>();
        if(null == is)
            return null;
        ZipInputStream zipinputstream = null;
        try
        {
            zipinputstream = new ZipInputStream(is);
            ZipEntry zipentry;
            if(!path.endsWith("/"))
                path += "/";
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                if(!zipentry.isDirectory())
                    continue;
                if(zipentry.getName().equals(path))
                    continue;
                if(zipentry.getName().startsWith(path))
                {
                    String tail = zipentry.getName().substring(path.length());
                    if(tail.endsWith("/"))
                    {
                        tail = tail.substring(0, tail.length() - 1);
                        if(!tail.contains("/"))
                            list.add(tail);
                    }
                }
            }
        }
        catch (Exception ignored) {}
        finally
        {
            Q3EUtils.Close(zipinputstream);
            Q3EUtils.Close(is);
        }
        return list;
    }

    private InputStream OpenRead_filesystem(String path)
    {
        InputStream is = null;
        File file = new File(path);
        if(file.isFile() && file.canRead())
        {
            try
            {
                is = new FileInputStream(file);
            }
            catch (Exception ignored) {}
        }
        return is;
    }

    private InputStream OpenRead_androidasset(String path)
    {
        InputStream is = null;
        try
        {
            is = m_context.getAssets().open(path);
        }
        catch (Exception ignored) {}
        return is;
    }

    private InputStream OpenRead_filesystem_zip(String fspath, String path)
    {
        return OpenRead_inputstream_zip(OpenRead_filesystem(fspath), path);
    }

    private InputStream OpenRead_androidasset_zip(String fspath, String path)
    {
        return OpenRead_inputstream_zip(OpenRead_androidasset(fspath), path);
    }

    private InputStream OpenRead_inputstream_zip(InputStream is, String path)
    {
        if(null == is)
            return null;
        InputStream res = null;
        ZipInputStream zipinputstream = null;
        ByteArrayOutputStream os = null;
        try
        {
            byte[] bytes = null;
            zipinputstream = new ZipInputStream(is);
            ZipEntry zipentry;
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                if(zipentry.getName().equals(path))
                {
                    os = new ByteArrayOutputStream();
                    Q3EUtils.Copy(os, zipinputstream, 4096);
                    os.flush();
                    bytes = os.toByteArray();
                    os.close();
                    os = null;
                    zipinputstream.closeEntry();
                    break;
                }
            }
            if(null != bytes)
                res = new ByteArrayInputStream(bytes);
        }
        catch (Exception ignored) {}
        finally
        {
            Q3EUtils.Close(os);
            Q3EUtils.Close(zipinputstream);
            Q3EUtils.Close(is);
        }
        return res;
    }
}
