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
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.os.Build;
import android.os.Environment;
import android.os.Process;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import com.n0n3m4.q3e.device.Q3EMouseDevice;
import com.n0n3m4.q3e.device.Q3EOuya;
import com.n0n3m4.q3e.karin.KFDManager;

import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashMap;
import java.util.List;

public class Q3EUtils
{
    private static final String TAG = "Q3EUtils";
    public static Q3EInterface q3ei = new Q3EInterface(); //k: new
    public static boolean isOuya = false;
    public static int UI_FULLSCREEN_HIDE_NAV_OPTIONS = 0;
    public static int UI_FULLSCREEN_OPTIONS = 0;

    static
    {
        Q3EUtils.isOuya = Q3EOuya.IsValid();
        UI_FULLSCREEN_HIDE_NAV_OPTIONS = GetFullScreenFlags(true);
        UI_FULLSCREEN_OPTIONS = GetFullScreenFlags(false);
    }

    public static boolean isAppInstalled(Activity ctx, String nm)
    {
        try
        {
            ctx.getPackageManager().getPackageInfo(nm, PackageManager.GET_ACTIVITIES);
            return true;
        } catch (Exception e)
        {
            return false;
        }
    }

    public static Bitmap ResourceToBitmap(Context cnt, String assetname)
    {
        String type = PreferenceManager.getDefaultSharedPreferences(cnt).getString(Q3EPreference.CONTROLS_THEME, "");
        if(null == type)
            type = "";
        return LoadControlBitmap(cnt, assetname, type);
    }

    public static int dip2px(Context ctx, int dip)
    {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, ctx.getResources().getDisplayMetrics());
    }

    public static int nextpowerof2(int x)
    {
        int candidate = 1;
        while (candidate < x)
            candidate *= 2;
        return candidate;
    }

    public static void togglevkbd(View vw)
    {
        InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        if (q3ei.function_key_toolbar)
        {
            boolean changed = imm.hideSoftInputFromWindow(vw.getWindowToken(), 0);
            if (changed) // im from open to close
                ToggleToolbar(false);
            else // im is closed
            {
                //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                ToggleToolbar(true);
            }
        } else
        {
            imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
        }
    }

    public static void ToggleToolbar(boolean on)
    {
        q3ei.callbackObj.ToggleToolbar(on);
    }

    public static void OpenVKB(View vw)
    {
        if (null != vw)
        {
            InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
            imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
            if (Q3EUtils.q3ei.function_key_toolbar)
                Q3EUtils.ToggleToolbar(true);
        }
    }

    public static void CloseVKB(View vw)
    {
        if (null != vw)
        {
            InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(vw.getWindowToken(), 0);
        }
    }

    public static int[] GetNormalScreenSize(Activity activity)
    {
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1)
        {
            display.getSize(size);
            return new int[]{size.x, size.y};
        } else
        {
            return new int[]{display.getWidth(), display.getHeight()};
        }
    }

    public static int[] GetFullScreenSize(Activity activity)
    {
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
        {
            display.getRealSize(size);
            return new int[]{size.x, size.y};
        } else
        {
            return new int[]{display.getWidth(), display.getHeight()};
        }
    }

    public static int GetEdgeHeight(Activity activity, boolean landscape)
    {
        int safeInsetTop = 0;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
        {
            WindowInsets rootWindowInsets = activity.getWindow().getDecorView().getRootWindowInsets();
            if (null != rootWindowInsets)
            {
                DisplayCutout displayCutout = rootWindowInsets.getDisplayCutout();
                if (null != displayCutout)
                {
                    safeInsetTop = landscape ? displayCutout.getSafeInsetLeft() : displayCutout.getSafeInsetTop();
                }
            }
        }
        return safeInsetTop;
    }

    public static int GetEndEdgeHeight(Activity activity, boolean landscape)
    {
        int safeInsetBottom = 0;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
        {
            WindowInsets rootWindowInsets = activity.getWindow().getDecorView().getRootWindowInsets();
            if (null != rootWindowInsets)
            {
                DisplayCutout displayCutout = rootWindowInsets.getDisplayCutout();
                if (null != displayCutout)
                {
                    safeInsetBottom = landscape ? displayCutout.getSafeInsetRight() : displayCutout.getSafeInsetBottom();
                }
            }
        }
        return safeInsetBottom;
    }

    public static int GetStatusBarHeight(Activity activity)
    {
        int result = 0;

        Resources resources = activity.getResources();
        int resourceId = resources.getIdentifier("status_bar_height","dimen", "android");

        if (resourceId > 0)
            result = resources.getDimensionPixelSize(resourceId);

        return result;
    }

    public static int GetNavigationBarHeight(Activity activity, boolean landscape)
    {
        int[] fullSize = GetFullScreenSize(activity);
        int[] size = GetNormalScreenSize(activity);
        return fullSize[1] - size[1] - GetEdgeHeight(activity, landscape) - GetEndEdgeHeight(activity, landscape);
    }

    public static String GetGameLibDir(Context context)
    {
        try
        {
            ApplicationInfo ainfo = context.getApplicationContext().getPackageManager().getApplicationInfo
                    (
                            context.getApplicationContext().getPackageName(),
                            PackageManager.GET_SHARED_LIBRARY_FILES
                    );
            return ainfo.nativeLibraryDir; //k for arm64-v8a apk install
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return context.getCacheDir().getAbsolutePath().replace("cache", "lib");		//k old, can work with armv5 and armv7-a
        }
    }

    public static <T extends Comparable> T Clamp(T target, T min, T max)
    {
        return target.compareTo(min) < 0 ? min : (target.compareTo(max) > 0 ? max : target);
    }

    public static float Clampf(float target, float min, float max)
    {
        return Math.max(min, Math.min(target, max));
    }

    public static float Rad2Deg(double rad)
    {
        double deg = rad / Math.PI * 180.0;
        return FormatAngle((float) deg);
    }

    public static float FormatAngle(float deg)
    {
        while (deg > 360)
            deg -= 360;
        while (deg < 0)
            deg += 360.0;
        return deg;
    }

    public static InputStream OpenResource(Context cnt, String assetname)
    {
        InputStream is;
        is = KFDManager.Instance(cnt).OpenRead(assetname);
        return is;
    }

    public static InputStream OpenResource_assets(Context cnt, String assetname)
    {
        InputStream is = null;
        try
        {
            is = cnt.getAssets().open(assetname);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return is;
    }

    public static String GetAppStoragePath(Context context)
    {
        String path = Q3EUtils.GetAppStoragePath(context, null);
        File dir = new File(path);
        return dir.getParent();
    }

    public static String GetAppStoragePath(Context context, String filename)
    {
        String path;
        File externalFilesDir = context.getExternalFilesDir(null);
        if(null != externalFilesDir)
            path = externalFilesDir.getAbsolutePath();
        else
            path = Environment.getExternalStorageDirectory() + "/Android/data/" + Q3EGlobals.CONST_PACKAGE_NAME + "/files";
        if(null != filename && !filename.isEmpty())
            path += filename;
        return path;
    }

    public static String GetAppInternalPath(Context context, String filename)
    {
        String path;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            path = context.getDataDir().getAbsolutePath();
        else
            path = context.getCacheDir().getAbsolutePath();
        if(null != filename && !filename.isEmpty())
            path += filename;
        return path;
    }

    public static void Close(Closeable closeable)
    {
        try
        {
            if(null != closeable)
                closeable.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public static LinkedHashMap<String, String> GetControlsThemes(Context context)
    {
        LinkedHashMap<String, String> list = new LinkedHashMap<>();
        list.put("/android_asset", "Default");
        list.put("", "External");
        List<String> controls_theme = KFDManager.Instance(context).ListDir("controls_theme");
        for (String file : controls_theme)
        {
            list.put("controls_theme/" + file, file);
        }
        return list;
    }

    public static Bitmap LoadControlBitmap(Context context, String path, String type)
    {
        InputStream is = null;
        Bitmap texture = null;
        switch (type)
        {
            case "/android_asset":
                is = Q3EUtils.OpenResource_assets(context, path);
                break;
            case "":
                is = Q3EUtils.OpenResource(context, path);
                break;
            default:
                if(type.startsWith("/"))
                {
                    type = type.substring(1);
                    is = Q3EUtils.OpenResource_assets(context, type + "/" + path);
                }
                else
                {
                    if((is = Q3EUtils.OpenResource(context, type + "/" + path)) == null)
                        is = Q3EUtils.OpenResource_assets(context, path);
                }
                break;
        }

        try
        {
            texture = BitmapFactory.decodeStream(is);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        finally
        {
            Q3EUtils.Close(is);
        }
        return texture;
    }

    public static int SupportMouse()
    {
        // return Q3EGlobals.MOUSE_EVENT;
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O || !Q3EMouseDevice.DeviceIsRoot() ? Q3EGlobals.MOUSE_EVENT : Q3EGlobals.MOUSE_DEVICE;
    }

    public static String Join(String d, String...strs)
    {
        if(null == strs)
            return null;
        if(strs.length == 0)
            return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < strs.length; i++) {
            sb.append(strs[i]);
            if(i < strs.length - 1)
                sb.append(d);
        }
        return sb.toString();
    }

    public static float parseFloat_s(String str, float...def)
    {
        float defVal = null != def && def.length > 0 ? def[0] : 0.0f;
        if(null == str)
            return defVal;
        str = str.trim();
        if(str.isEmpty())
            return defVal;
        try
        {
            return Float.parseFloat(str);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static int parseInt_s(String str, int...def)
    {
        int defVal = null != def && def.length > 0 ? def[0] : 0;
        if(null == str)
            return defVal;
        str = str.trim();
        if(str.isEmpty())
            return defVal;
        try
        {
            return Integer.parseInt(str);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static long Copy(OutputStream out, InputStream in, int...bufferSizeArg) throws RuntimeException
    {
        if(null == out)
            return -1;
        if(null == in)
            return -1;

        int bufferSize = bufferSizeArg.length > 0 ? bufferSizeArg[0] : 0;
        if (bufferSize <= 0)
            bufferSize = 8192;

        byte[] buffer = new byte[bufferSize];

        long size = 0L;

        int readSize;
        try
        {
            while((readSize = in.read(buffer)) != -1)
            {
                out.write(buffer, 0, readSize);
                size += readSize;
                out.flush();
            }
        }
        catch (IOException e)
        {
            throw new RuntimeException(e);
        }

        return size;
    }

    public static String Read(InputStream in) throws RuntimeException
    {
        if(null == in)
            return "";

        ByteArrayOutputStream os = new ByteArrayOutputStream();
        try
        {
            Copy(os, in);
            byte[] bytes = os.toByteArray();
            return new String(bytes, StandardCharsets.UTF_8);
        }
        catch (Exception e)
        {
            throw new RuntimeException(e);
        }
        finally
        {
            Q3EUtils.Close(os);
        }
    }

    public static long cp(String src, String dst)
    {
        FileInputStream is = null;
        FileOutputStream os = null;
        File srcFile = new File(src);
        if(!srcFile.isFile())
            return -2;
        if(!srcFile.canRead())
            return -3;
        File dstFile = new File(dst);
        File dstDir = dstFile.getParentFile();
        if(null != dstDir && !dstDir.isDirectory())
        {
            if(!dstDir.mkdirs())
                return -4;
        }
        try
        {
            is = new FileInputStream(srcFile);
            os = new FileOutputStream(dst);
            return Q3EUtils.Copy(os, is);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return -1;
        }
        finally
        {
            Q3EUtils.Close(is);
            Q3EUtils.Close(os);
        }
    }

    /*
    @SuppressLint("InlinedApi")
    private final int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN;
    @SuppressLint("InlinedApi")
    private final int m_uiOptions_def = View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
     */
    public static int GetFullScreenFlags(boolean hideNav)
    {
        int m_uiOptions = 0;

        if(hideNav)
        {
            m_uiOptions |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            {
                m_uiOptions |= View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
        {
            m_uiOptions |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            m_uiOptions |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
            m_uiOptions |= View.SYSTEM_UI_FLAG_FULLSCREEN;
        }
        return m_uiOptions;
    }

    public static void CopyToClipboard(Context context, String text)
    {
        ClipboardManager clipboard = (ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("idTech4A++", text);
        clipboard.setPrimaryClip(clip);
    }

    public static String GetClipboardText(Context context)
    {
        ClipboardManager clipboard = (ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE);

        String pasteData = null;
        if (clipboard.hasPrimaryClip())
        {
            ClipData primaryClip = clipboard.getPrimaryClip();
            if(null != primaryClip && primaryClip.getItemCount() > 0)
            {
                ClipData.Item item = primaryClip.getItemAt(0);
                pasteData = item.getText().toString();
            }
        }
        return pasteData;
    }

    public static int[] CalcSizeByScaleScreenArea(int width, int height, BigDecimal scale)
    {
        double p = Math.sqrt(scale.doubleValue());
        BigDecimal bp = BigDecimal.valueOf(p);
        BigDecimal bw = new BigDecimal(width);
        BigDecimal bh = new BigDecimal(height);
        BigDecimal ww = bw.multiply(bp);
        int w = ww.intValue();
        int h = ww.multiply(bh).divide(bw, 2, RoundingMode.HALF_UP).intValue();
        return new int[]{w, h};
    }

    public static int[] CalcSizeByScaleWidthHeight(int width, int height, BigDecimal scale)
    {
        int w = new BigDecimal(width).multiply(scale).intValue();
        int h = new BigDecimal(height).multiply(scale).intValue();
        return new int[]{w, h};
    }

    public static boolean file_put_contents(String path, String content)
    {
        if(null == path)
            return false;
        return file_put_contents(new File(path), content);
    }

    public static boolean file_put_contents(File file, String content)
    {
        if(null == file)
            return false;

        FileWriter writer = null;
        try
        {
            writer = new FileWriter(file);
            writer.append(content);
            writer.flush();
            return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Close(writer);
        }
    }

    public static String file_get_contents(String path)
    {
        if(null == path)
            return null;
        return file_get_contents(new File(path));
    }

    public static String file_get_contents(File file)
    {
        if(null == file || !file.isFile() || !file.canRead())
            return null;

        FileReader reader = null;
        try
        {
            reader = new FileReader(file);
            int BUF_SIZE = 1024;
            char[] chars = new char[BUF_SIZE];
            int len;
            StringBuilder sb = new StringBuilder();
            while ((len = reader.read(chars)) > 0)
                sb.append(chars, 0, len);
            return sb.toString();
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return null;
        }
        finally
        {
            Close(reader);
        }
    }

    public static boolean rm(String path)
    {
        if(null == path)
            return false;
        return rm(new File(path));
    }

    public static boolean rm(File file)
    {
        if(null == file || !file.isFile())
            return false;

        try
        {
            return file.delete();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static boolean mkdir(String path, boolean p)
    {
        File file = new File(path);
        if(file.exists())
        {
            return file.isDirectory();
        }
        try
        {
            if(p)
                return file.mkdirs();
            else
                return file.mkdir();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    private static boolean _dumpPID = false;
    public static void DumpPID(Context context)
    {
        if(_dumpPID || !BuildConfig.DEBUG )
            return;
        try
        {
            String text = "" + Process.myPid();
            final String[] Paths;
            Paths = new String[]{
                    PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.pref_datapath, ""),
                    android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N ? context.getDataDir().getAbsolutePath() : context.getCacheDir().getAbsolutePath()
            };
            for (String dir : Paths)
            {
                if(null == dir || dir.isEmpty())
                    continue;
                String path = dir + "/.idtech4amm.pid";
                file_put_contents(path, text);
                Log.i(TAG, "DumpPID " + text + " to " + path);
                _dumpPID = true;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    public static float GetRefreshRate(Context context)
    {
        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        return display.getRefreshRate();
    }

    public static void RunLauncher(Activity activity)
    {
        Intent intent = activity.getPackageManager().getLaunchIntentForPackage(activity.getApplicationContext().getPackageName());
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        activity.startActivity(intent);
    }

    public static int[] GetSurfaceViewSize(Context context, int screenWidth, int screenHeight)
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        int scheme = mPrefs.getInt(Q3EPreference.pref_scrres_scheme, 0);
        int scale = mPrefs.getInt(Q3EPreference.pref_scrres_scale, 100);
        BigDecimal scalef = new BigDecimal(scale).divide(new BigDecimal("100"), 2, RoundingMode.UP);
        if(scalef.compareTo(BigDecimal.ZERO) <= 0)
            scalef = BigDecimal.ONE;
        int width, height;
        switch (scheme)
        {
            case 1: {
                int[] size = CalcSizeByScaleWidthHeight(screenWidth, screenHeight, scalef);
                width = size[0];
                height = size[1];
            }
                break;
            case 2: {
                int[] size = CalcSizeByScaleScreenArea(screenWidth, screenHeight, scalef);
                width = size[0];
                height = size[1];
            }
                break;
            case 3: {
                try
                {
                    String str = mPrefs.getString(Q3EPreference.pref_resx, "0");
                    if(null == str)
                        str = "0";
                    width = Integer.parseInt(str);
                }
                catch (Exception e)
                {
                    width = 0;
                }
                try
                {
                    String str = mPrefs.getString(Q3EPreference.pref_resy, "0");
                    if(null == str)
                        str = "0";
                    height = Integer.parseInt(str);
                }
                catch (Exception e)
                {
                    height = 0;
                }
                if (width <= 0 && height <= 0)
                {
                    width = screenWidth;
                    height = screenHeight;
                }
                if (width <= 0)
                {
                    width = (int)((float)height * (float)screenWidth / (float)screenHeight);
                }
                else if (height <= 0)
                {
                    height = (int)((float)width * (float)screenHeight / (float)screenWidth);
                }
            }
                break;
            case 0:
            default:
                width = screenWidth;
                height = screenHeight;
                break;
        }
        return new int[]{width, height};
    }

    public static String GetAppInternalSearchPath(Context context, String path)
    {
        if(null == path)
            path = "";
        return Q3EUtils.GetAppStoragePath(context, "/diii4a" + path);
    }
}
