package org.etlegacy.app;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.Bundle;
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
     * Shows ProgressBar
     * @return progressBar obj
     */
    private ProgressDialog DownloadBar() {
        ProgressDialog asyncDialog = new ProgressDialog(this);
        asyncDialog.setTitle("Downloading Game Data ..");
        asyncDialog.setIndeterminate(false);
        asyncDialog.setProgress(0);
        asyncDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        asyncDialog.setCancelable(false);
        asyncDialog.setCanceledOnTouchOutside(false);
        asyncDialog.show();

        return asyncDialog;
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

        String etl_pak = new String();
        AssetManager assManager = getApplicationContext().getAssets();

        try {
            for(String file: assManager.list("")) {
                if(file.endsWith(".pk3")) {
                    etl_pak += file;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        ImageView imageView = new ImageView(this);
        imageView.setBackgroundResource(R.drawable.ic_horizontal_white);

        LinearLayout etl_Layout = new LinearLayout(this);

        RelativeLayout.LayoutParams etl_Params = new RelativeLayout.LayoutParams(Resources.getSystem().getDisplayMetrics().widthPixels,
                Resources.getSystem().getDisplayMetrics().heightPixels);

        etl_Layout.addView(imageView, etl_Params);
        setContentView(etl_Layout);

        final File etl_etlegacy = new File(getExternalFilesDir(null), "etlegacy/legacy/".concat(etl_pak));

        if (!etl_etlegacy.exists()) {
            InputStream is = null;
            try {
                is = assManager.open(etl_pak);
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            copyInputStreamToFile(is, etl_etlegacy);
        }

        // FIXME:This is ugly
        final File etl_pak0 = new File(getExternalFilesDir(null), "/etlegacy/etmain/pak0.pk3");
        final File etl_pak1 = new File(getExternalFilesDir(null), "/etlegacy/etmain/pak1.pk3");
        final File etl_pak2 = new File(getExternalFilesDir(null), "/etlegacy/etmain/pak2.pk3");

        final Intent intent = new Intent(ETLMain.this, ETLActivity.class);

        if (etl_pak0.exists() && etl_pak1.exists() && etl_pak2.exists()) {
            ETLMain.this.startActivity(intent);
            ETLMain.this.finish();
        } else {
            final ProgressDialog etl_Dialog = DownloadBar();
            final AsyncHttpClient client = new AsyncHttpClient();

            // pak2
            client.get("https://mirror.etlegacy.com/etmain/pak2.pk3", new FileAsyncHttpResponseHandler(this) {
                @Override
                public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {

                }

                @Override
                public void onSuccess(int statusCode, Header[] headers, File file) {

                }

                @Override
                public void onFinish() {
                    super.onFinish();
                    if (file.getAbsoluteFile().exists()) {
                        try {
                            Files.move(file.getAbsoluteFile(), new File(getExternalFilesDir("etlegacy/etmain"), etl_pak2.getName()));
                            client.cancelAllRequests(true);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            });

            // pak1
            client.get("https://mirror.etlegacy.com/etmain/pak1.pk3", new FileAsyncHttpResponseHandler(this) {
                @Override
                public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {

                }

                @Override
                public void onSuccess(int statusCode, Header[] headers, File file) {

                }

                @Override
                public void onFinish() {
                    super.onFinish();
                    if (file.getAbsoluteFile().exists()) {
                        try {
                            Files.move(file.getAbsoluteFile(), new File(getExternalFilesDir("etlegacy/etmain"), etl_pak1.getName()));
                            client.cancelAllRequests(true);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            });

            // pak0
            client.get("https://mirror.etlegacy.com/etmain/pak0.pk3", new FileAsyncHttpResponseHandler(this) {

                @Override
                public void onSuccess(int statusCode, Header[] headers, File file) {
                }

                @Override
                public void onProgress(long bytesWritten, long totalSize) {
                    super.onProgress(bytesWritten, totalSize);
                    final int etl_bytesWritten = (int) (bytesWritten / Math.pow(1024, 2));
                    int etl_totalSize = (int) (totalSize / Math.pow(1024, 2));
                    etl_Dialog.setMax(etl_totalSize);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            etl_Dialog.setProgress(etl_bytesWritten);
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
                            etl_Dialog.dismiss();
                            ETLMain.this.startActivity(intent);
                            ETLMain.this.finish();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }

                @Override
                public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {
                    etl_Dialog.dismiss();
                }
            });
        }
    }
}
