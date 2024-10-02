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
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Surface;
import android.widget.Toast;

import com.etlegacy.app.R;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTech4Command;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class Q3EGameHelper
{
    private Context m_context;

    public Q3EGameHelper()
    {
    }

    public Q3EGameHelper(Context context)
    {
        this.m_context = context;
    }

    public void SetContext(Context context)
    {
        this.m_context = context;
    }

    public void ShowMessage(String s)
    {
        Toast.makeText(m_context, s, Toast.LENGTH_LONG).show();
    }

    public void ShowMessage(int resId)
    {
        Toast.makeText(m_context, resId, Toast.LENGTH_LONG).show();
    }

    public boolean checkGameFiles()
    {
        String dataDir = Q3EUtils.q3ei.GetGameDataDirectoryPath(null);
        if (!new File(dataDir).exists())
        {
            ShowMessage(Q3ELang.tr(m_context, R.string.game_files_weren_t_found_put_game_files_to) + dataDir);
            return false;
        }

        return true;
    }

    public void InitGlobalEnv()
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);

        if (!Q3EUtils.q3ei.IsInitGame()) // not from GameLauncher::startActivity
        {
            Q3EUtils.q3ei.standalone = preferences.getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);

            Q3EKeyCodes.InitD3Keycodes();

            Q3EUtils.q3ei.InitD3();

            Q3EUtils.q3ei.InitDefaultsTable();

            Q3EUtils.q3ei.default_path = Environment.getExternalStorageDirectory() + "/etlegacy";

            Q3EUtils.q3ei.SetupGame(preferences.getString(Q3EPreference.pref_harm_game, Q3EGlobals.GAME_DOOM3));

            Q3EUtils.q3ei.LoadTypeAndArgTablePreference(m_context);

            String extraCommand = "";
            if(preferences.getBoolean(Q3EPreference.pref_harm_skip_intro, false) && (Q3EUtils.q3ei.IsIdTech4() || Q3EUtils.q3ei.IsIdTech3()/* || Q3EUtils.q3ei.IsTDMTech()*/))
            {
                extraCommand += " +disconnect";
            }
            if(preferences.getBoolean(Q3EPreference.pref_harm_auto_quick_load, false) && (Q3EUtils.q3ei.IsIdTech4() || Q3EUtils.q3ei.isRTCW))
            {
                extraCommand += " +loadGame QuickSave";
            }
            Q3EUtils.q3ei.start_temporary_extra_command = extraCommand.trim();
        }
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Run " + Q3EUtils.q3ei.game_name);

        Q3EUtils.q3ei.joystick_release_range = preferences.getFloat(Q3EPreference.pref_harm_joystick_release_range, 0.0f);
        Q3EUtils.q3ei.joystick_inner_dead_zone = preferences.getFloat(Q3EPreference.pref_harm_joystick_inner_dead_zone, 0.0f);
        Q3EUtils.q3ei.SetupEngineLib(); //k setup engine library here again
        Q3EUtils.q3ei.view_motion_control_gyro = preferences.getBoolean(Q3EPreference.pref_harm_view_motion_control_gyro, false);
        Q3EUtils.q3ei.multithread = preferences.getBoolean(Q3EPreference.pref_harm_multithreading, false);
        Q3EUtils.q3ei.function_key_toolbar = preferences.getBoolean(Q3EPreference.pref_harm_function_key_toolbar, true);
        Q3EUtils.q3ei.joystick_unfixed = preferences.getBoolean(Q3EPreference.pref_harm_joystick_unfixed, false);
        Q3EUtils.q3ei.joystick_smooth = preferences.getBoolean(Q3EPreference.pref_analog, true);
        Q3EUtils.q3ei.VOLUME_UP_KEY_CODE = Q3EKeyCodes.GetRealKeyCode(preferences.getInt(Q3EPreference.VOLUME_UP_KEY, Q3EKeyCodes.KeyCodesGeneric.K_F3));
        Q3EUtils.q3ei.VOLUME_DOWN_KEY_CODE = Q3EKeyCodes.GetRealKeyCode(preferences.getInt(Q3EPreference.VOLUME_DOWN_KEY, Q3EKeyCodes.KeyCodesGeneric.K_F2));

        // DOOM 3: Hardscorps and Quake4: hardqore mod template disable smooth joystick
        if(Q3EUtils.q3ei.joystick_smooth)
        {
            String game = preferences.getString(Q3EUtils.q3ei.GetGameModPreferenceKey(), "");
            if(Q3EUtils.q3ei.isQ4)
            {
                if("hardqore".equals(game))
                    Q3EUtils.q3ei.joystick_smooth = false;
            }
            else if(Q3EUtils.q3ei.isD3)
            {
                if("hardscorps".equals(game))
                    Q3EUtils.q3ei.joystick_smooth = false;
            }
        }

        Q3EUtils.q3ei.SetAppStoragePath(m_context);

        Q3EUtils.q3ei.datadir = preferences.getString(Q3EPreference.pref_datapath, Q3EUtils.q3ei.default_path);
        if (null == Q3EUtils.q3ei.datadir)
            Q3EUtils.q3ei.datadir = Q3EUtils.q3ei.default_path;
        if ((Q3EUtils.q3ei.datadir.length() > 0) && (Q3EUtils.q3ei.datadir.charAt(0) != '/'))//lolwtfisuserdoing?
        {
            Q3EUtils.q3ei.datadir = "/" + Q3EUtils.q3ei.datadir;
            preferences.edit().putString(Q3EPreference.pref_datapath, Q3EUtils.q3ei.datadir).commit();
        }

        String cmd = preferences.getString(Q3EUtils.q3ei.GetGameCommandPreferenceKey(), Q3EUtils.q3ei.libname);
        if(null == cmd)
            cmd = Q3EGlobals.GAME_EXECUABLE;
        if(preferences.getBoolean(Q3EPreference.pref_harm_find_dll, false)
                && Q3EUtils.q3ei.IsIdTech4()
        )
        {
            KidTech4Command command = new KidTech4Command(cmd);
            String fs_game = command.Prop(Q3EUtils.q3ei.GetGameCommandParm());
            if(null == fs_game || fs_game.isEmpty())
            {
                switch (Q3EUtils.q3ei.game)
                {
                    case Q3EGlobals.GAME_PREY:
                        fs_game = "preybase";
                        break;
                    case Q3EGlobals.GAME_QUAKE4:
                        fs_game = "q4base";
                        break;
                    case Q3EGlobals.GAME_DOOM3:
                    default:
                        fs_game = "base";
                        break;
                }
            }
            String dll = FindDLL(fs_game);
            if(null != dll)
                command.SetProp("harm_fs_gameLibPath", dll);
            cmd = command.toString();
        }
        String binDir = Q3EUtils.q3ei.GetGameDataDirectoryPath(null);
        cmd = binDir + "/" + cmd + " " + Q3EUtils.q3ei.start_temporary_extra_command/* + " +set harm_fs_gameLibDir " + lib_dir*/;
        Q3EUtils.q3ei.cmd = cmd;

/*        if(Q3EUtils.q3ei.isDOOM)
            Q3EUtils.q3ei.joystick_smooth = false;*/
    }

    private String FindDLL(String fs_game)
    {
        String DLLPath = Q3EUtils.q3ei.GetGameDataDirectoryPath(fs_game); // /sdcard/etlegacy/<fs_game>
        String Suffix = "game" + Q3EGlobals.ARCH + ".so"; // gameaarch64.so(64) / gamearm.so(32)
        String[] guess = {
                Suffix,
                "lib" + Suffix,
        };
        String res = null;
        String targetDir = m_context.getCacheDir() + File.separator; // /data/user/<package_name>/cache/
        for (String f : guess)
        {
            String p = KStr.AppendPath(DLLPath, f);
            File file = new File(p);
            if(!file.isFile() || !file.canRead())
                continue;
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Found user game library file: " + p);
            String cacheFile = targetDir + Suffix;
            long r = Q3EUtils.cp(p, cacheFile);
            if(r > 0)
            {
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Load user game library: " + cacheFile);
                res = cacheFile;
            }
            else
                Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "Upload user game library fail: " + cacheFile);
        }
        return res;
    }

    public String GetEngineLib()
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        String libname = Q3EUtils.q3ei.libname;
        // if(Q3EUtils.q3ei.isTDM) libname = "libDukeNukemForever.so"; // Test a new game using TDM
        String libPath = Q3EUtils.GetGameLibDir(m_context) + "/" + libname; // Q3EUtils.q3ei.libname;
        //if(Q3EUtils.q3ei.isTDM) Q3EUtils.q3ei.subdatadir = "dnf"; // Test a new game using TDM
        if(preferences.getBoolean(Q3EPreference.LOAD_LOCAL_ENGINE_LIB, false))
        {
            String localLibPath = Q3EUtils.q3ei.GetGameDataDirectoryPath(libname);
            // if(Q3EUtils.q3ei.isTDM) Q3EUtils.q3ei.subdatadir = Q3EGlobals.GAME_SUBDIR_TDM; // Test a new game using TDM
            File file = new File(localLibPath);
            if(!file.isFile() || !file.canRead())
            {
                Log.w(Q3EGlobals.CONST_Q3E_LOG_TAG, "Local engine library not file or unreadable: " + localLibPath);
            }
            else
            {
                String cacheFile = m_context.getCacheDir() + File.separator + /*Q3EUtils.q3ei.*/libname;
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Found local engine library file: " + localLibPath);
                long r = Q3EUtils.cp(localLibPath, cacheFile);
                if(r > 0)
                {
                    libPath = cacheFile;
                    Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Load local engine library: " + cacheFile);
                }
                else
                {
                    Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "Upload local engine library fail: " + cacheFile);
                }
            }
        }
        return libPath;
    }

    private boolean ExtractCopy(String systemFolderPath, boolean overwrite, String...assetPaths)
    {
        InputStream bis = null;
        FileOutputStream fileoutputstream = null;

        try
        {
            Q3EUtils.mkdir(systemFolderPath, true);

            for (String assetPath : assetPaths)
            {
                bis = m_context.getAssets().open(assetPath);

                String tmpname;
                int index = assetPath.lastIndexOf('/');
                if(index >= 0)
                    tmpname = assetPath.substring(index + 1);
                else
                    tmpname = assetPath;

                String entryName = systemFolderPath + "/" + tmpname;
                File file = new File(entryName);

                if(!overwrite && file.exists())
                    continue;

                fileoutputstream = new FileOutputStream(entryName);
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying " + assetPath + " to " + systemFolderPath);
                Q3EUtils.Copy(fileoutputstream, bis, 4096);
                fileoutputstream.close();
                fileoutputstream = null;
                bis.close();
                bis = null;
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
            Q3EUtils.Close(bis);
        }
    }

    private boolean ExtractCopy(String assetFolderPath, String systemFolderPath, boolean overwrite, String...assetPaths)
    {
        InputStream bis = null;
        FileOutputStream fileoutputstream = null;

        try
        {
            Q3EUtils.mkdir(systemFolderPath, true);

            for (String assetPath : assetPaths)
            {
                String sourcePath = assetFolderPath + "/" + assetPath;
                bis = m_context.getAssets().open(sourcePath);

                String entryName = systemFolderPath + "/" + assetPath;
                File file = new File(entryName);

                if(!overwrite && file.exists())
                    continue;

                Q3EUtils.mkdir(file.getParent(), true);

                fileoutputstream = new FileOutputStream(entryName);
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying " + sourcePath + " to " + entryName);
                Q3EUtils.Copy(fileoutputstream, bis, 4096);
                fileoutputstream.close();
                fileoutputstream = null;
                bis.close();
                bis = null;
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
            Q3EUtils.Close(bis);
        }
    }

    private boolean ExtractZip(String assetPath, String systemFolderPath, boolean overwrite)
    {
        InputStream bis = null;
        ZipInputStream zipinputstream = null;
        FileOutputStream fileoutputstream = null;

        try
        {
            bis = m_context.getAssets().open(assetPath);
            zipinputstream = new ZipInputStream(bis);

            ZipEntry zipentry;
            Q3EUtils.mkdir(systemFolderPath, true);
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                String tmpname = zipentry.getName();

                String entryName = systemFolderPath + "/" + tmpname;
                entryName = entryName.replace('/', File.separatorChar);
                entryName = entryName.replace('\\', File.separatorChar);
                File file = new File(entryName);

                if (zipentry.isDirectory())
                {
                    if(!file.exists())
                        Q3EUtils.mkdir(entryName, true);
                    continue;
                }
                if(!overwrite && file.exists())
                    continue;

                fileoutputstream = new FileOutputStream(entryName);
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting " + tmpname + " to " + systemFolderPath);
                Q3EUtils.Copy(fileoutputstream, zipinputstream, 4096);
                fileoutputstream.close();
                fileoutputstream = null;
                zipinputstream.closeEntry();
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
            Q3EUtils.Close(zipinputstream);
            Q3EUtils.Close(bis);
        }
    }

    private boolean CheckExtractResourceOverwrite(String systemVersionPath, String apkVersion, String name)
    {
        boolean overwrite = false;

        try
        {
            File versionFile = new File(systemVersionPath);
            if(!versionFile.isFile() || !versionFile.canRead())
            {
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file version not exists.");
                overwrite = true;
            }
            else
            {
                String version = Q3EUtils.file_get_contents(versionFile);
                if(null != version)
                    version = version.trim();
                if(!apkVersion.equalsIgnoreCase(version))
                {
                    Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, String.format(name + " file version is mismatch: %s != %s.", version, Q3EGlobals.RBDOOM3BFG_HLSL_SHADER_VERSION));
                    overwrite = true;
                }
            }
            if(overwrite)
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file will be overwrite.");
            else
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file will keep exists version.");
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Check " + name + "(" + systemVersionPath + ") version file error.");
        }
        return overwrite;
    }

    private void DumpExtractResourceVersion(String systemVersionPath, String apkVersion, String name)
    {
        Q3EUtils.file_put_contents(systemVersionPath, apkVersion);
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Write " + name + " file version is " + apkVersion);
    }

    public void ExtractTDMGLSLShaderSource()
    {
        final String destname = Q3EUtils.q3ei.datadir + "/darkmod";
        final String versionFile = destname + "/glslprogs/idtech4amm.version";
        final String version = Q3EGlobals.TDM_GLSL_SHADER_VERSION;
        final String name = "The Dark Mod GLSL shader source";
        final String zip = "pak/darkmod/glprogs.pk4";

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);

        if(ExtractZip(zip, destname, overwrite))
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(R.string.extract_the_dark_mod_glsl_shader_source_files_fail);
    }

    public void ExtractDOOM3BFGHLSLShaderSource()
    {
        final String destname = Q3EUtils.q3ei.datadir + "/doom3bfg/base";
        final String versionFile = destname + "/renderprogs/idtech4amm.version";
        final String version = Q3EGlobals.RBDOOM3BFG_HLSL_SHADER_VERSION;
        final String name = "RBDOOM 3 BFG HLSL shader source";
        final String zip = "pak/doom3bfg/renderprogs.pk4";

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);

        if(ExtractZip(zip, destname, overwrite))
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(R.string.extract_rbdoom3bfg_hlsl_shader_source_files_fail);
    }

    public void ExtractGZDOOMResource()
    {
        final String destname = Q3EUtils.q3ei.datadir + "/gzdoom";
        final String versionFile = destname + "/idtech4amm.version";
        final String version = Q3EGlobals.GZDOOM_VERSION;
        final String name = "GZDOOM game resource";
        final String assetFolder = "pak/gzdoom";
        final String[] files = {
                "brightmaps.pk3",
                "game_support.pk3",
                "game_widescreen_gfx.pk3",
                "gzdoom.pk3",
                "lights.pk3",
                "soundfonts/gzdoom.sf2",
                "fm_banks/GENMIDI.GS.wopl",
                "fm_banks/gs-by-papiezak-and-sneakernets.wopn",
        };

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);

        if(ExtractCopy(assetFolder, destname, overwrite, files))
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(R.string.extract_gzdoom_game_resource_files_fail);
    }

    public void ExtractGameResource()
    {
        if(Q3EUtils.q3ei.IsTDMTech()) // if game is TDM, extract glsl shader
            ExtractTDMGLSLShaderSource();
        else if(Q3EUtils.q3ei.IsIdTech4BFG()) // if game is D3BFG, extract hlsl shader
            ExtractDOOM3BFGHLSLShaderSource();
        else if(Q3EUtils.q3ei.isDOOM)
            ExtractGZDOOMResource();
    }

    private int GetMSAA()
    {
        int msaa = PreferenceManager.getDefaultSharedPreferences(m_context).getInt(Q3EPreference.pref_msaa, 0);
        switch (msaa)
        {
            case 0: msaa = 0;break;
            case 1: msaa = 4;break;
            case 2: msaa = 16;break;
        }
        return msaa;
    }

    public int GetPixelFormat()
    {
        if (PreferenceManager.getDefaultSharedPreferences(m_context).getBoolean(Q3EPreference.pref_32bit, false))
        {
            return PixelFormat.RGBA_8888;
        }
        else
        {
            //if (Q3EUtils.q3ei.isD3)
            //k setEGLConfigChooser(new Q3EConfigChooser(5, 6, 5, 0, msaa, Q3EUtils.usegles20));
            //k getHolder().setFormat(PixelFormat.RGB_565);

            int harm16Bit = PreferenceManager.getDefaultSharedPreferences(m_context).getInt(Q3EPreference.pref_harm_16bit, 0);
            switch (harm16Bit)
            {
                case 1: // RGBA4444
                    return PixelFormat.RGBA_4444;
                case 2: // RGBA5551
                    return PixelFormat.RGBA_5551;
                case 3: // RGBA10101002
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                        return PixelFormat.RGBA_1010102;
                    else
                        return PixelFormat.RGBA_8888;
                case 0: // RGB565
                default:
                    return PixelFormat.RGB_565;
            }
        }
    }

    public int GetGLFormat()
    {
        int pixelFormat = GetPixelFormat();
        int glFormat = 0x8888;
        switch (pixelFormat) {
            case PixelFormat.RGBA_4444:
                glFormat = Q3EGlobals.GLFORMAT_RGBA4444;
                break;
            case PixelFormat.RGBA_5551:
                glFormat = Q3EGlobals.GLFORMAT_RGBA5551;
                break;
            case PixelFormat.RGB_565:
                glFormat = Q3EGlobals.GLFORMAT_RGB565;
                break;
            case PixelFormat.RGBA_1010102:
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                    glFormat = Q3EGlobals.GLFORMAT_RGBA1010102;
                else
                    glFormat = Q3EGlobals.GLFORMAT_RGBA8888;
                break;
            case PixelFormat.RGBA_8888:
                glFormat = Q3EGlobals.GLFORMAT_RGBA8888;
                break;
        }
        return glFormat;
    }

    private int[] GetFrameSize(int w, int h)
    {
        int[] size = Q3EUtils.GetSurfaceViewSize(m_context, w, h);
        Log.i("Q3EView", "FrameSize: (" + size[0] + ", " + size[1] + ")");
        return size;
    }

    private int[] GetFrameSize_old(int w, int h)
    {
        int width;
        int height;
        SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(m_context);
        boolean scaleByScreenArea = mPrefs.getBoolean(Q3EPreference.pref_harm_scale_by_screen_area, false);

        switch (mPrefs.getInt(Q3EPreference.pref_scrres, 0))
        {
            case 0:
                width = w;
                height = h;
                break;
            case 1:
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, new BigDecimal("0.5"));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 2;
                    height = h / 2;
                }
                break;
            case 2:
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, new BigDecimal("2"));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w * 2;
                    height = h * 2;
                }
                break;
            case 3:
                width = 1920;
                height = 1080;
                break;
            case 4:
                //k width=Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resx, "640"));
                //k height=Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resy, "480"));
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
                    width = w;
                    height = h;
                }
                if (width <= 0)
                {
                    width = (int)((float)height * (float)w / (float)h);
                }
                else if (height <= 0)
                {
                    height = (int)((float)width * (float)h / (float)w);
                }
                break;

            //k
            case 5: // 720p
                width = 1280;
                height = 720;
                break;
            case 6: // 480p
                width = 720;
                height = 480;
                break;
            case 7: // 360p
                width = 640;
                height = 360;
                break;
            case 8: // 1/3
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, BigDecimal.ONE.divide(new BigDecimal("3"), 2, RoundingMode.HALF_UP));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 3;
                    height = h / 3;
                }
                break;
            case 9: // 1/4
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, new BigDecimal("0.25"));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 4;
                    height = h / 4;
                }
                break;
            //k
            default:
                width = w;
                height = h;
                break;
        }
        Log.i("Q3EView", "FrameSize: (" + width + ", " + height + ")");
        return new int[] { width, height };
    }

    public boolean Start(Surface surface, int surfaceWidth, int surfaceHeight)
    {
        // GL
        int msaa = GetMSAA();
        int glFormat = GetGLFormat();
        int[] size = GetFrameSize(surfaceWidth, surfaceHeight);
        int width = size[0];
        int height = size[1];

        // Args
        String cmd = Q3EUtils.q3ei.cmd;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        boolean redirectOutputToFile = preferences.getBoolean(Q3EPreference.REDIRECT_OUTPUT_TO_FILE, true);
        boolean noHandleSignals = preferences.getBoolean(Q3EPreference.NO_HANDLE_SIGNALS, false);
        int runBackground = Q3EUtils.parseInt_s(preferences.getString(Q3EPreference.RUN_BACKGROUND, "0"), 0);
        int glVersion = preferences.getInt(Q3EPreference.pref_harm_opengl, 0x00020000);
        boolean usingMouse = preferences.getBoolean(Q3EPreference.pref_harm_using_mouse, false) && Q3EUtils.SupportMouse() == Q3EGlobals.MOUSE_EVENT;

        String subdatadir = Q3EUtils.q3ei.subdatadir;
        // if(Q3EUtils.q3ei.isTDM) subdatadir = "dnf"; // Test a new game using TDM

        int refreshRate = (int)Q3EUtils.GetRefreshRate(m_context);
        String appHome = Q3EUtils.GetAppInternalSearchPath(m_context, null);
		appHome = KStr.AppendPath(appHome, subdatadir);

        int eventQueue = Q3EPreference.GetIntFromString(preferences, Q3EPreference.EVENT_QUEUE, 0);
        Q3EJNI.PreInit(eventQueue);

        boolean res = Q3EJNI.init(
                GetEngineLib(),
                Q3EUtils.GetGameLibDir(m_context),
                width,
                height,
                Q3EUtils.q3ei.datadir,
                subdatadir,
                cmd,
                surface,
                glFormat,
                msaa, glVersion,
                redirectOutputToFile,
                noHandleSignals,
                Q3EUtils.q3ei.multithread,
                usingMouse,
                refreshRate,
                appHome,
                Q3EUtils.q3ei.joystick_smooth,
                runBackground > 0
        );
        if(!res)
        {
            if(m_context instanceof Activity)
            {
                Activity activity = (Activity)m_context;
                activity.runOnUiThread(new Runnable() {
                    @Override
                    public void run()
                    {
                        activity.finish();
                        // Q3EUtils.RunLauncher(activity);
                    }
                });
            }
        }
        return res;
    }

    public Context GetContext()
    {
        return m_context;
    }
}
