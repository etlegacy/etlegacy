package com.karin.idTech4Amm.lib;

import android.net.Uri;
import android.os.Environment;
import android.util.Log;

import com.karin.idTech4Amm.misc.Function;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.FileReader;
import java.io.Closeable;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * Local file IO utility
 */
public final class FileUtility
{
    public static final int DEFAULT_BUFFER_SIZE = 8192;
    
    public static boolean CloseStream(Closeable stream)
    {
        if(null == stream)
            return true;
        try
        {
            stream.close();
            return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static byte[] ReadStream(InputStream in, int...bufferSizeArg)
    {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        long size = Q3EUtils.Copy(os, in, bufferSizeArg);
        byte[] res = null;
        if(size >= 0)
            res = os.toByteArray();
        CloseStream(os);
        return res;
    }

    public static String GetFileExtension(String fileName)
    {
        if(null == fileName)
            return null;
        int index = fileName.lastIndexOf(".");
        if(index <= 0 || index == fileName.length() - 1)
            return "";
        return fileName.substring(index + 1);
    }

    public static String GetFileBaseName(String fileName)
    {
        if(null == fileName)
            return null;
        int index = fileName.lastIndexOf(".");
        if(index <= 0 || index == fileName.length() - 1)
            return fileName;
        return fileName.substring(0, index);
    }

    public static long du(String path)
    {
        return du(new File(path));
    }

    public static long du(File file)
    {
        return du(file, null);
    }

    public static long du(String path, Function filter)
    {
        return du(new File(path), filter);
    }

    public static long du(File file, Function filter)
    {
        if(null != filter && !(boolean)filter.Invoke(file))
            return -2L;
        if(!file.exists())
            return -1L;
        if(file.isDirectory())
        {
            long sum = 0;
            File[] files = file.listFiles();
            if(null == files)
                return 0;
            for (File f : files)
            {
                long l = du(f, filter);
                if(l > 0)
                    sum += l;
            }
            return sum;
        }
        else
        {
            return file.length();
        }
    }

    public static String FormatSize(long size)
    {
        String[] Unit = {"Bytes", "K", "M", "G", "T"};
        //const Unit = ["byte", "K", "M", "G", "T"];
        double s;
        int i;
        for(s = size, i = 0; s >= 1024.0 && i < Unit.length - 1; s /= 1024.0, i++);
        s = Math.round(s * 100.0) / 100.0;
        return s + Unit[i];
    }

    public static String RelativePath(String a, String b)
    {
        return RelativePath(new File(a), new File(b));
    }

    public static String RelativePath(File a, File b)
    {
        if(a == b)
            return "";
        String ap = AbsolutePath(a);
        String bp = AbsolutePath(b);
        if(ap.equals(bp))
            return "";
        if(ap.startsWith(bp))
            return ap.substring(bp.length());
        else if(bp.startsWith(ap))
            return bp.substring(ap.length());
        else
            return null;
    }

    public static String AbsolutePath(File a)
    {
        try
        {
            return a.getCanonicalPath();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return a.getAbsolutePath();
        }
    }

    public static boolean mv(String src, String target)
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O)
        {
            try
            {
                Path srcPath = Paths.get(src);
                Path targetPath = Paths.get(target);
                if(Files.isDirectory(srcPath))
                {
                    Files.walk(srcPath).forEach((x) -> {
                        File file = x.toFile();
                        String relativePath = file.getAbsolutePath().substring(src.length());
                        Path targetP = Paths.get(target + relativePath);
                        try
                        {
                            if(Files.isDirectory(x))
                            {
                                Files.createDirectories(targetP);
                            }
                            else
                            {
                                Files.move(x, targetP/*, StandardCopyOption.REPLACE_EXISTING*/);
                            }
                        }
                        catch (IOException e)
                        {
                            throw new RuntimeException(e);
                        }
                    });
                }
                else
                    Files.move(srcPath, targetPath/*, StandardCopyOption.REPLACE_EXISTING*/);
                return true;
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
        else
        {
            File srcFile = new File(src);
            try
            {
                return srcFile.renameTo(new File(target));
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
    }

    public static boolean IsSDCardPath(String path)
    {
        if (path.startsWith("/sdcard"))
            return true;
        else
        {
            final String emuPath = Environment.getExternalStorageDirectory().getAbsolutePath();
            if (path.startsWith(emuPath))
                return true;
        }
        return false;
    }

    public static String GetSDCardRelativePath(String path)
    {
        if (path.startsWith("/sdcard"))
            path = path.substring("/sdcard".length());
        else
        {
            final String emuPath = Environment.getExternalStorageDirectory().getAbsolutePath();
            if (path.startsWith(emuPath))
                path = path.substring(emuPath.length());
        }
        return path;
    }

    public static Uri PathGrantUri(String dir)
    {
        dir = GetSDCardRelativePath(dir);
        String d;
        if(dir.startsWith("/Android/data"))
            d = "Android/data";
        else if(dir.startsWith("/Android/obb"))
            d = "Android/obb";
        else
            d = "";
        d = d.replaceAll("/", "%2F");
        if(dir.startsWith("/"))
            dir = dir.substring(1);
        String str = "content://com.android.externalstorage.documents"
                + "/tree/primary%3A" // :
                + d
                + "/document/primary%3A"
                + dir.replaceAll("/", "%2F")
                ;
        return Uri.parse(str);
    }

    // content://com.android.externalstorage.documents/tree/primary%3AAndroid%2Fdata/document/primary%3AAndroid%2Fdata%2Fcom.android.fileexplorer
    // content://com.android.externalstorage.documents/tree/primary%3AAndroid%2Fdata%2Fcom.android.fileexplorer
    public static Uri PathUri(String dir)
    {
        dir = GetSDCardRelativePath(dir);
        if(dir.startsWith("/"))
            dir = dir.substring(1);
        String str = "content://com.android.externalstorage.documents"
                + "/tree/primary%3A" // :
                + dir.replaceAll("/", "%2F")
                ;
        return Uri.parse(str);
    }

    public static String ParentPath(String path)
    {
        if(path.endsWith("/"))
            path = path.substring(0, path.length() - 1);
        int i = path.lastIndexOf('/');
        path = i > 0 ? path.substring(0, i) : "/";
        return path;
    }

    public static String GetPathFromUri(Uri uri)
    {
        String path = uri.toString();
        path = path.replaceAll("content://com\\.android\\.externalstorage\\.documents/tree/primary%3A", "/storage/emulated/0/");
        path = path.replaceAll("%2F", "/");
        //Log.e("TAG", "GetPathFromUri: " + path);
        return path;
    }

    public static String GetRelativePath(String target, String parent)
    {
        if(!parent.endsWith("/"))
            parent += "/";
        int i = target.indexOf(parent);
        if(i != 0)
            return null;
        return target.substring(parent.length());
    }

    public static String[] SplitPathParts(String path)
    {
        String[] split = path.split("/");
        List<String> list = new ArrayList<>();
        for (String str : split)
        {
            if(KStr.NotEmpty(str))
                list.add(str);
        }
        return list.toArray(new String[0]);
    }

    public static String[] GetRelativePathParts(String target, String parent)
    {
        String path = GetRelativePath(target, parent);
        return SplitPathParts(path);
    }
    
    private FileUtility() {}
}
