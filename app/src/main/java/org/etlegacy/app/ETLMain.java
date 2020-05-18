package org.etlegacy.app;

import android.app.Activity;
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
import android.widget.TextView;

import com.google.common.io.Files;
import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.FileAsyncHttpResponseHandler;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

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

        TextView textView = new TextView(this);
        textView.append("Downloading Game Data ...");
        textView.setTextSize(25);

        LinearLayout etl_Layout = new LinearLayout(this);

        RelativeLayout.LayoutParams etl_Params = new RelativeLayout.LayoutParams(
                600, 300);

        etl_Params.leftMargin = pxToDp(Resources.getSystem().getDisplayMetrics().widthPixels / 2);
        etl_Params.topMargin = pxToDp(Resources.getSystem().getDisplayMetrics().heightPixels / 2);

        LinearLayout.LayoutParams etl_Params_download = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT, 1.0f);

        etl_Params_download.setMargins(-600, 600, -700,-50);

        etl_Layout.addView(imageView, etl_Params);
        etl_Layout.addView(textView, etl_Params_download);
        setContentView(etl_Layout);

        File etl_pak = new File(getExternalFilesDir(null), "/etlegacy/etmain/pak0.pk3");
        final Intent intent = new Intent(ETLMain.this, ETLActivity.class);

        if (etl_pak.exists()) {
            startActivity(intent);
            finish();
        }
        else {
            final AsyncHttpClient client = new AsyncHttpClient();
            client.get("http://mirror.etlegacy.com/etmain/pak0.pk3", new FileAsyncHttpResponseHandler(this) {

                @Override
                public void onSuccess(int statusCode, Header[] headers, File file) {

                }

                @Override
                public void onFinish() {
                    super.onFinish();
                    if (file.getAbsoluteFile().exists()) {
                        try {
                            Files.move(file.getAbsoluteFile(), new File(getExternalFilesDir(null), "/etlegacy/etmain/pak0.pk3"));
                            client.cancelAllRequests(true);
                            startActivity(intent);
                            finish();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }

                @Override
                public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {

                }
            });
        }

    }
}
