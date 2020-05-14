package org.etlegacy.app;

import android.content.Intent;
import android.os.Bundle;

import com.google.common.io.Files;
import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.FileAsyncHttpResponseHandler;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.IOException;

import cz.msebera.android.httpclient.Header;

public class ETLMain extends SDLActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        AsyncHttpClient client = new AsyncHttpClient();
        client.get("http://mirror.etlegacy.com/etmain/pak2.pk3", new FileAsyncHttpResponseHandler(this) {

            @Override
            public void onSuccess(int statusCode, Header[] headers, File file) {
            }

            @Override
            public void onFinish() {
                super.onFinish();
                try {
                    Files.move(file.getAbsoluteFile(), new File(getExternalFilesDir(null), "pak2.pk3"));
                } catch (IOException e) {
                    e.printStackTrace();
                }
                startActivity(new Intent(getApplicationContext(), ETLActivity.class));
                finish();
            }

            @Override
            public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {

            }
        });
    }
}
