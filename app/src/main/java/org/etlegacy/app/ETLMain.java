package org.etlegacy.app;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

import com.google.common.io.Files;
import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.FileAsyncHttpResponseHandler;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import cz.msebera.android.httpclient.Header;

public class ETLMain extends Activity {

    /**
     * Get an splash screen image from Asset Dir of an App
     * @param strName name of the splash screen image
     * @return image
     */
    private Drawable getSplashScreenFromAsset(String strName) {
        AssetManager assetManager = getAssets();
        InputStream istr = null;
        try {
            istr = assetManager.open(strName);
        } catch (IOException e) {
            e.printStackTrace();
        }

        // It somehow looks ugly and expensive, find a better solution
        Bitmap bitmap = BitmapFactory.decodeStream(istr);
        Drawable etl_drawable = new BitmapDrawable(getResources(), bitmap);
        return etl_drawable;
    }

    /**
     * Copy InputStream to a File.
     * @param in inputstream to be saved
     * @param file location of file to save inputstream in
     */
    private void copyInputStreamToFile(InputStream in, File file) {
        OutputStream out = null;

        try {
            out = new FileOutputStream(new File(getExternalFilesDir("etlegacy/legacy"), file.getName()));
            byte[] buf = new byte[1024];
            int len;
            while((len=in.read(buf))>0){
                out.write(buf,0,len);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        finally {
            // Ensure that the InputStreams are closed even if there's an exception.
            try {
                if ( out != null ) {
                    out.close();
                }

                // If you want to close the "in" InputStream yourself then remove this
                // from here but ensure that you close it yourself eventually.
                in.close();
            }
            catch ( IOException e ) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Convert pixel metrics to dp
     * @param px value of px to be converted
     * @return dp
     */
    public static int pxToDp(int px) {
        return (int) (px / Resources.getSystem().getDisplayMetrics().density);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        ImageView imageView = new ImageView(this);
        imageView.setBackground(getSplashScreenFromAsset("etl_splashscreen.png"));

        LinearLayout etl_Layout = new LinearLayout(this);

        RelativeLayout.LayoutParams etl_Params = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        etl_Params.topMargin = pxToDp(Resources.getSystem().getDisplayMetrics().heightPixels / 2);

        etl_Layout.addView(imageView, etl_Params);
        setContentView(etl_Layout);

        final File etl_etlegacy = new File(getExternalFilesDir(null), "etlegacy/legacy/legacy_v2.76.pk3");

        if (!etl_etlegacy.exists()) {
            AssetManager assManager = getApplicationContext().getAssets();
            InputStream is = null;
            try {
                is = assManager.open("legacy_v2.76.pk3");
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            copyInputStreamToFile(is, etl_etlegacy);
        }

        final File etl_pak0 = new File(getExternalFilesDir(null), "/etlegacy/etmain/pak0.pk3");
        final Intent intent = new Intent(ETLMain.this, ETLActivity.class);

        if (etl_pak0.exists()) {
            startActivity(intent);
            finish();
        } else {
            final ProgressDialog asyncDialog = new ProgressDialog(this);
            asyncDialog.setTitle("Downloading Game Data ..");
            asyncDialog.setIndeterminate(false);
            asyncDialog.setProgress(0);
            asyncDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            asyncDialog.setCancelable(false);
            asyncDialog.setCanceledOnTouchOutside(false);
            asyncDialog.show();
            final AsyncHttpClient client = new AsyncHttpClient();
            client.get("http://mirror.etlegacy.com/etmain/pak0.pk3", new FileAsyncHttpResponseHandler(this) {

                @Override
                public void onSuccess(int statusCode, Header[] headers, File file) {
                }

                @Override
                public void onProgress(long bytesWritten, long totalSize) {
                    super.onProgress(bytesWritten, totalSize);
                    final int etl_bytesWritten = (int) (bytesWritten / Math.pow(1024, 2));
                    int etl_totalSize = (int) (totalSize / Math.pow(1024, 2));
                    asyncDialog.setMax(etl_totalSize);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            asyncDialog.setProgress(etl_bytesWritten);
                        }
                    });
                }

                @Override
                public void onFinish() {
                    super.onFinish();
                    if (file.getAbsoluteFile().exists()) {
                        try {
                            Files.move(file.getAbsoluteFile(), new File(getExternalFilesDir("etlegacy/etmain"), etl_pak0.getName()));
                            client.cancelAllRequests(true);
                            asyncDialog.dismiss();
                            startActivity(intent);
                            finish();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }

                @Override
                public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {
                    asyncDialog.dismiss();
                }
            });
        }
    }
}
