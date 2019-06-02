package org.etlegacy.app;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.RelativeLayout;

import org.libsdl.app.SDLActivity;

import java.io.IOException;
import java.io.InputStream;

public class ETLActivity extends SDLActivity
{
    private Bitmap getBitmapFromAsset(String strName)
    {
        AssetManager assetManager = getAssets();
        InputStream istr = null;
        try {
            istr = assetManager.open(strName);
        } catch (IOException e) {
            e.printStackTrace();
        }
        Bitmap bitmap = BitmapFactory.decodeStream(istr);
        Bitmap resized = Bitmap.createScaledBitmap(bitmap, 80, 80, true);
        return resized;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Button btn = new Button(getApplicationContext());
        btn.setText("Keyboard");
        btn.setId(1);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED,0);
            }
        });

        mLayout.addView(btn);

        ImageButton btn2 = new ImageButton(getApplicationContext());
        btn2.setImageBitmap(getBitmapFromAsset("btn_keyboard.png"));
        btn2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyDown(68);
            }
        });

        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);

        lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, btn.getId());

        mLayout.addView(btn2, lp);

    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL2",
                "etl"
        };
    }
}
