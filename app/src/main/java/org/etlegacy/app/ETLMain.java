package org.etlegacy.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import com.google.common.io.Files;
import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.FileAsyncHttpResponseHandler;

import java.io.File;
import java.io.IOException;

import cz.msebera.android.httpclient.Header;

public class ETLMain extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        File etl_pak = new File(getExternalFilesDir(null), "pak2.pk3");
        final Intent intent = new Intent(ETLMain.this, ETLActivity.class);

        if (etl_pak.exists()) {
            startActivity(intent);
            finish();
        }
        else {
            AsyncHttpClient client = new AsyncHttpClient();
            client.get("http://mirror.etlegacy.com/etmain/pak2.pk3", new FileAsyncHttpResponseHandler(this) {

                @Override
                public void onSuccess(int statusCode, Header[] headers, File file) {
                }

                @Override
                public void onFinish() {
                    super.onFinish();
                    if (file.getAbsoluteFile().exists()) {
                        try {
                            Files.move(file.getAbsoluteFile(), new File(getExternalFilesDir(null), "pak2.pk3"));
                            finish();
                            startActivity(intent);
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
