package com.karin.idTech4Amm.lib;

import android.Manifest;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.UriPermission;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.content.Intent;
import android.net.Uri;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.support.v4.provider.DocumentFile;
import android.util.Log;
import android.view.Display;
import android.view.Window;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.TextView;
import android.text.util.Linkify;
import android.text.method.LinkMovementMethod;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.misc.Function;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
// import android.widget.Magnifier.Builder;

/**
 * Android context/activity utility
 */
public final class ContextUtility
{
    public static final int CHECK_PERMISSION_RESULT_GRANTED = 0;
    public static final int CHECK_PERMISSION_RESULT_REQUEST = 1;
    public static final int CHECK_PERMISSION_RESULT_REJECT = 2;

    public static final int SCREEN_ORIENTATION_AUTOMATIC = 0;
    public static final int SCREEN_ORIENTATION_PORTRAIT = 1;
    public static final int SCREEN_ORIENTATION_LANDSCAPE = 2;

    public static AlertDialog.Builder CreateMessageDialogBuilder(Context context, CharSequence title, CharSequence message)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setPositiveButton(R.string.ok, new AlertDialog.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                }
            });

        return builder;
    }

    public static AlertDialog OpenMessageDialog(Context context, CharSequence title, CharSequence message)
    {
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(context, title, message);

        AlertDialog dialog = builder.create();
        dialog.show();

        TextView messageText = (TextView)(dialog.findViewById(android.R.id.message));
        if(messageText != null) // never
        {
            if(!TextHelper.USING_HTML)
                messageText.setAutoLinkMask(Linkify.ALL);
            messageText.setMovementMethod(LinkMovementMethod.getInstance());
        }

        return dialog;
    }

    public static void OpenAppSetting(Context context)
    {
        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        Uri uri = Uri.fromParts("package", context.getApplicationContext().getPackageName(), null);
        intent.setData(uri);
        context.startActivity(intent);
    }

    public static String GetAppVersion(Context context)
    {
        String version = "UNKNOWN";
        try
        {
            PackageManager manager = context.getPackageManager();
            PackageInfo info = manager.getPackageInfo(context.getApplicationContext().getPackageName(), 0);
            version = info.versionName;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return version;
    }

    public static boolean BuildIsDebug(Context context)
    {
        try
        {
            ApplicationInfo info = context.getApplicationInfo();
            return (info.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0 || Constants.IsDebug();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return Constants.IsDebug(); // default is release
        }
    }

    public static int CheckPermission(Activity context, String permission, int resultCode)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // Android M
        {
            boolean granted = context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
            if(granted)
                return CHECK_PERMISSION_RESULT_GRANTED;
            if (false && !context.shouldShowRequestPermissionRationale(permission)) // do not ask
            {
                ContextUtility.OpenAppSetting(context);
                return CHECK_PERMISSION_RESULT_REJECT; // goto app detail settings activity
            }
            context.requestPermissions(new String[] { permission }, resultCode);
            return CHECK_PERMISSION_RESULT_REQUEST;
        }
        else
            return CHECK_PERMISSION_RESULT_GRANTED; // other think has granted
    }

    public static void SetScreenOrientation(Activity context, int o)
    {
        switch(o)
        {
            case SCREEN_ORIENTATION_LANDSCAPE:
                    context.setRequestedOrientation(Build.VERSION.SDK_INT >= 9 ? ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                    break;
            case SCREEN_ORIENTATION_PORTRAIT:
                    context.setRequestedOrientation(Build.VERSION.SDK_INT >= 9 ? ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    break;
            case SCREEN_ORIENTATION_AUTOMATIC:
                default:
                    context.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER);
                    break;
        }
    }

    public static void Confirm(Context context, CharSequence title, CharSequence message, final Runnable yes, final Runnable no, Runnable other, String otherName)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setCancelable(true);
            builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int v)
                    {
                        if(yes != null)
                            yes.run();
                        dialog.dismiss();
                    }
                });
            builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int v)
                    {
                        if(no != null)
                            no.run();
                        dialog.dismiss();
                    }
                });
        if(null != other)
        {
            builder.setNeutralButton(null != otherName ? otherName : Q3ELang.tr(context, R.string.other), new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int v)
                {
                    other.run();
                    dialog.dismiss();
                }
            });
        }
        builder.create().show();
    }

    public static AlertDialog Input(Context context, CharSequence title, CharSequence message, String val, String[] args, final Runnable yes, final Runnable no, String otherName, Runnable other, Function editTextConfig)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title);
        EditText editText = new EditText(context);
        if(null != message)
            editText.setHint(message);
        if(null != val)
            editText.setText(val);
        if(null != editTextConfig)
            editTextConfig.Invoke(editText);
        builder.setView(editText);
        builder.setCancelable(true);
        builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int v)
            {
                if(null != args && args.length > 0)
                    args[0] = editText.getText().toString();
                if(yes != null)
                    yes.run();
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int v)
            {
                if(null != args && args.length > 0)
                    args[0] = editText.getText().toString();
                if(no != null)
                    no.run();
                dialog.dismiss();
            }
        });
        if(null != other)
        {
            builder.setNeutralButton(null != otherName ? otherName : Q3ELang.tr(context, R.string.other), new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int v)
                {
                    if(null != args && args.length > 0)
                        args[0] = editText.getText().toString();
                    other.run();
                    dialog.dismiss();
                }
            });
        }
        AlertDialog dialog = builder.create();
        dialog.show();
        return dialog;
    }

    public static String NativeLibDir(Context context)
    {
        try
        {
            ApplicationInfo ainfo = context.getApplicationContext().getPackageManager().getApplicationInfo
            (
                context.getApplicationContext().getPackageName(),
                PackageManager.GET_SHARED_LIBRARY_FILES
            );
            return ainfo.nativeLibraryDir;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return "";
        }
    }

    public static int CheckFilePermission(Activity context, int resultCode)
    {
        String permission = Manifest.permission.WRITE_EXTERNAL_STORAGE;
        // Android SDK > 28
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) // Android 11
        {
            if(Environment.isExternalStorageManager())
                return CHECK_PERMISSION_RESULT_GRANTED;
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + context.getApplicationContext().getPackageName()));
            context.startActivityForResult(intent, resultCode);
            return CHECK_PERMISSION_RESULT_REQUEST;
        }
        else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // Android M - Q
        {
            boolean granted = context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
            if(granted)
                return CHECK_PERMISSION_RESULT_GRANTED;
            if (false && !context.shouldShowRequestPermissionRationale(permission)) // do not ask
            {
                ContextUtility.OpenAppSetting(context);
                return CHECK_PERMISSION_RESULT_REJECT; // goto app detail settings activity
            }
            context.requestPermissions(new String[] { permission }, resultCode);
            return CHECK_PERMISSION_RESULT_REQUEST;
        }
        else
            return CHECK_PERMISSION_RESULT_GRANTED; // other think has granted
    }

    public static boolean ExtractAsset(Context context, String path, String outPath)
    {
        InputStream is = null;
        FileOutputStream os = null;
        try
        {
            is = context.getAssets().open(path);
            File out = new File(outPath);
            Q3EUtils.mkdir(out.getParent(), true);
            os = new FileOutputStream(out);

            long res = Q3EUtils.Copy(os, is);

            return res > 0;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally {
            FileUtility.CloseStream(os);
            FileUtility.CloseStream(is);
        }
    }

    public static boolean ExtractAssetToDirectory(Context context, String path, String outPath)
    {
        InputStream is = null;
        FileOutputStream os = null;
        File f = new File(path);
        String name = f.getName();

        try
        {
            Q3EUtils.mkdir(outPath, true);
            is = context.getAssets().open(path);
            File out = new File(outPath + File.separator + name);
            os = new FileOutputStream(out);

            long res = Q3EUtils.Copy(os, is);

            return res > 0;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally {
            FileUtility.CloseStream(os);
            FileUtility.CloseStream(is);
        }
    }

    public static void OpenUrlExternally(Context context, String url)
    {
        Uri uri = Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
        context.startActivity(intent);
    }

    public static void RestartApp(Activity activity)
    {
        if(null == activity)
            return;
        activity.finish();
        Intent intent = activity.getPackageManager().getLaunchIntentForPackage(activity.getApplicationContext().getPackageName());
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        activity.startActivity(intent);
    }

    public static boolean OpenApp(Activity activity, String packageName)
    {
        final Intent intent = activity.getPackageManager().getLaunchIntentForPackage(packageName);
        if(null == intent)
            return false;
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        activity.startActivity(intent);
        return true;
    }

    public static Context GetContext(Fragment fragment)
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M)
            return fragment.getContext();
        else
            return fragment.getActivity();
    }

    public static AlertDialog.Builder CreateMessageDialogBuilder(Context context, CharSequence title, CharSequence message, Runnable callback, Object[] args)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setPositiveButton(R.string.ok, new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        if(null != callback && null != args && args.length > 0)
        {
            args[0] = builder;
            callback.run();
        }

        return builder;
    }

    public static AlertDialog OpenMessageDialog(Context context, CharSequence title, CharSequence message, Runnable callback, Object[] args)
    {
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(context, title, message, callback, args);

        AlertDialog dialog = builder.create();
        dialog.show();

        TextView messageText = (TextView)(dialog.findViewById(android.R.id.message));
        if(messageText != null) // never
        {
            if(!TextHelper.USING_HTML)
                messageText.setAutoLinkMask(Linkify.ALL);
            messageText.setMovementMethod(LinkMovementMethod.getInstance());
        }

        return dialog;
    }

    public static ProgressDialog Progress(Context context, CharSequence title, CharSequence message, Runnable cancel)
    {
        ProgressDialog dialog = new ProgressDialog(context);
        dialog.setTitle(title);
        dialog.setMessage(message);
        dialog.setCancelable(false);
        dialog.setButton(DialogInterface.BUTTON_NEGATIVE, Q3ELang.tr(context, R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });
        dialog.setOnCancelListener(new DialogInterface.OnCancelListener()
        {
            @Override
            public void onCancel(DialogInterface dialog)
            {
                if(null != cancel)
                    cancel.run();
            }
        });
        return dialog;
    }

    public static float SetRefreshRate(Activity context, int modeId)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            Display display = null;
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
            {
                display = context.getDisplay();
            }
            else
            {
                display = context.getWindowManager().getDefaultDisplay();
            }
            Display.Mode[] supportedModes = display.getSupportedModes();
            Display.Mode mode = null;
            for (Display.Mode supportedMode : supportedModes)
            {
                if(supportedMode.getModeId() == modeId)
                {
                    mode = supportedMode;
                    break;
                }
            }
            if(null == mode)
            {
                return -1;
            }
            Window window = context.getWindow();
            WindowManager.LayoutParams a = window.getAttributes();
            a.preferredDisplayModeId = mode.getModeId();
            a.preferredRefreshRate = mode.getRefreshRate();
            window.setAttributes(a);
            return mode.getRefreshRate();
        }
        else
            return 0;
    }

    public static Map<Integer, Float> GetSupportRefreshRates(Activity context)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            Display display = null;
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
            {
                display = context.getDisplay();
            }
            else
            {
                display = context.getWindowManager().getDefaultDisplay();
            }
            Display.Mode[] supportedModes = display.getSupportedModes();
            Map<Integer, Float> map = new LinkedHashMap<>();
            for (Display.Mode supportedMode : supportedModes)
            {
                map.put(supportedMode.getModeId(), supportedMode.getRefreshRate());
            }
            return map;
        }
        else
            return null;
    }

    public static boolean IsInAppPrivateDirectory(Context context, String path)
    {
        final String appPath = Q3EUtils.GetAppStoragePath(context);
        if(path.startsWith("/sdcard"))
        {
            int i = appPath.indexOf("/Android/data");
            path = appPath.substring(0, i) + path.substring("/sdcard".length());
        }
        return path.startsWith(appPath);
    }

    public static boolean NeedGrantUriPermission(Context context, String path)
    {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q) // <= 10
            return false;
        if(!FileUtility.IsSDCardPath(path))
            return false;
        path = FileUtility.GetSDCardRelativePath(path);
        if(path.endsWith("/"))
            path = path.substring(0, path.length() - 1);

        if (!path.startsWith("/Android/data") && !path.startsWith("/Android/obb"))
            return false;
        if (path.equals("/Android/data") || path.equals("/Android/obb"))
            return Build.VERSION.SDK_INT <= Build.VERSION_CODES.R + 1; // <= 12

        if(path.startsWith("/Android/data"))
            path = path.substring("/Android/data".length());
        else if(path.startsWith("/Android/obb"))
            path = path.substring("/Android/obb".length());
        if(path.startsWith("/"))
            path = path.substring(1);
        if(path.endsWith("/"))
            path = path.substring(0, path.length() - 1);
        // Log.e("XXX", path + "|" + path + "="+!path.contains("/"));

        if(path.startsWith(context.getApplicationContext().getPackageName()))
            return false;

        return true; // !path.contains("/");
    }

    public static boolean NeedUsingDocumentFile(Context context, String path)
    {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q) // <= 10
            return false;
        if(!FileUtility.IsSDCardPath(path))
            return false;
        path = FileUtility.GetSDCardRelativePath(path);
        if (!path.startsWith("/Android/data") && !path.startsWith("/Android/obb"))
            return false;

        String npath = path;
        if(npath.endsWith("/"))
            npath = npath.substring(0, npath.length() - 1);
        if (npath.equals("/Android/data") || npath.equals("/Android/obb"))
            return false;

        String packageName = context.getApplicationContext().getPackageName();
        if(path.startsWith("/Android/data/" + packageName) || path.startsWith("/Android/obb/" + packageName))
            return false;
        return true;
    }

    public static boolean NeedListPackagesAsFiles(Context context, String path)
    {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.R + 1) // <= 12
            return false;
        if(!FileUtility.IsSDCardPath(path))
            return false;
        path = FileUtility.GetSDCardRelativePath(path);
        if(path.endsWith("/"))
            path = path.substring(0, path.length() - 1);
        return path.equals("/Android/data") || path.equals("/Android/obb");
    }

    public static boolean IsUriPermissionGrant(Context context, String path)
    {
        if(!NeedGrantUriPermission(context, path))
            return true;
        List<UriPermission> persistedUriPermissions = context.getContentResolver().getPersistedUriPermissions();
        for (UriPermission persistedUriPermission : persistedUriPermissions)
        {
            Uri uri = FileUtility.PathUri(path);
            //Log.e("TAG", "IsUriPermissionGrant: " +uri + "|" + persistedUriPermission.getUri()+"="+persistedUriPermission.getUri().equals(uri));
            if(uri.equals(persistedUriPermission.getUri()))
                return true;
        }
        return false;
    }

    public static boolean IsUriPermissionGrantPrefix(Context context, String path)
    {
        if(!NeedGrantUriPermission(context, path))
            return true;
        List<UriPermission> persistedUriPermissions = context.getContentResolver().getPersistedUriPermissions();
        for (UriPermission persistedUriPermission : persistedUriPermissions)
        {
            Uri uri = FileUtility.PathUri(path);
            //Log.e("TAG", "IsUriPermissionGrantPrefix: " +uri + "|" + persistedUriPermission.getUri()+"="+persistedUriPermission.getUri().equals(uri));
            if(uri.toString().startsWith(persistedUriPermission.getUri().toString()))
                return true;
        }
        return false;
    }

    public static Uri GetPermissionGrantedUri(Context context, String path)
    {
        if(!NeedGrantUriPermission(context, path))
            return null;
        List<UriPermission> persistedUriPermissions = context.getContentResolver().getPersistedUriPermissions();
        for (UriPermission persistedUriPermission : persistedUriPermissions)
        {
            Uri uri = FileUtility.PathUri(path);
            //Log.e("TAG", "IsUriPermissionGrantPrefix: " +uri + "|" + persistedUriPermission.getUri()+"="+persistedUriPermission.getUri().equals(uri));
            Uri grantedUri = persistedUriPermission.getUri();
            if(uri.toString().startsWith(grantedUri.toString()))
                return grantedUri;
        }
        return null;
    }

    public static boolean GrantUriPermission(Activity activity, String path, int resultCode)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            if(!NeedGrantUriPermission(activity, path))
                return false;
            Uri uri;

            uri = FileUtility.PathUri(path);
            //Log.e("TAG", "111: "+uri);
            DocumentFile documentFile = DirectoryDocument(activity, path);
            if(null != documentFile)
                uri = documentFile.getUri();
            else
                uri = FileUtility.PathGrantUri(path);
            //Log.e("TAG", "222: "+uri);
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, uri);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                    | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                    | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                    | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION
            );
            intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
            activity.startActivityForResult(intent, resultCode);
            return true;
        }
        return false;
    }

    public static void PersistableUriPermission(Activity activity, Uri uri)
    {
        //Log.e("TAG", "PersistableUriPermission: "+uri);
        activity.getContentResolver()
                .takePersistableUriPermission(uri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION
                        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                );
    }

    public static String[] ListPackages(Context context)
    {
        List<PackageInfo> installedPackages = context.getPackageManager().getInstalledPackages(0);
        String[] res = new String[installedPackages.size()];
        for (int i = 0; i < installedPackages.size(); i++)
        {
            res[i] = installedPackages.get(i).packageName;
        }
        return res;
    }

    public static DocumentFile DirectoryDocument(Context context, String path)
    {
        return DocumentFile.fromTreeUri(context, FileUtility.PathUri(path));
    }

    public static void OpenDocumentsUI(Context context)
    {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        ComponentName name = new ComponentName("com.google.android.documentsui", "com.android.documentsui.files.FilesActivity");
        intent.setComponent(name);
        context.startActivity(intent);
    }

    public static boolean InScopedStorage()
    {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R); // >= 11
    }

	private ContextUtility() {}
}
