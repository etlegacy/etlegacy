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

import com.erz.joysticklibrary.JoyStick;
import com.erz.joysticklibrary.JoyStick.JoyStickListener;

import org.libsdl.app.SDLActivity;

import java.io.IOException;
import java.io.InputStream;

public class ETLActivity extends SDLActivity implements JoyStickListener
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

        JoyStick joyStick = new JoyStick(getApplicationContext());

        joyStick.setListener(this);
        joyStick.setPadBackground(getBitmapFromAsset("btn_joystick.png"));
        joyStick.setButtonColor(Color.RED);
        joyStick.setButtonRadiusScale(50);

        RelativeLayout.LayoutParams joystick_layout = new RelativeLayout.LayoutParams(
                400,
                400);

        joystick_layout.addRule(RelativeLayout.ALIGN_LEFT);
        joystick_layout.addRule(RelativeLayout.CENTER_VERTICAL);
        joystick_layout.leftMargin = 90;

        mLayout.addView(joyStick, joystick_layout);

    }

    @Override
    public void onMove(JoyStick joyStick, double angle, double power, int direction) {

        switch(direction)
        {
            case JoyStick.DIRECTION_CENTER:
                SDLActivity.onNativeKeyUp(51);
                SDLActivity.onNativeKeyUp(32);
                SDLActivity.onNativeKeyUp(47);
                SDLActivity.onNativeKeyUp(29);
                break;
            case JoyStick.DIRECTION_UP:
                SDLActivity.onNativeKeyUp(32);
                SDLActivity.onNativeKeyUp(47);
                SDLActivity.onNativeKeyUp(29);
                SDLActivity.onNativeKeyDown(51);
                break;
            case JoyStick.DIRECTION_UP_RIGHT:
                SDLActivity.onNativeKeyUp(47);
                SDLActivity.onNativeKeyUp(29);
                SDLActivity.onNativeKeyDown(51);
                SDLActivity.onNativeKeyDown(32);
                break;
            case JoyStick.DIRECTION_RIGHT:
                SDLActivity.onNativeKeyUp(51);
                SDLActivity.onNativeKeyUp(47);
                SDLActivity.onNativeKeyUp(29);
                SDLActivity.onNativeKeyDown(32);
                break;
            case JoyStick.DIRECTION_RIGHT_DOWN:
                SDLActivity.onNativeKeyUp(51);
                SDLActivity.onNativeKeyUp(29);
                SDLActivity.onNativeKeyDown(32);
                SDLActivity.onNativeKeyDown(47);
                break;
            case JoyStick.DIRECTION_DOWN:
                SDLActivity.onNativeKeyUp(51);
                SDLActivity.onNativeKeyUp(32);
                SDLActivity.onNativeKeyUp(29);
                SDLActivity.onNativeKeyDown(47);
                break;
            case JoyStick.DIRECTION_DOWN_LEFT:
                SDLActivity.onNativeKeyUp(51);
                SDLActivity.onNativeKeyUp(32);
                SDLActivity.onNativeKeyDown(47);
                SDLActivity.onNativeKeyDown(29);
                break;
            case JoyStick.DIRECTION_LEFT:
                SDLActivity.onNativeKeyUp(51);
                SDLActivity.onNativeKeyUp(32);
                SDLActivity.onNativeKeyUp(47);
                SDLActivity.onNativeKeyDown(29);
                break;
            case JoyStick.DIRECTION_LEFT_UP:
                SDLActivity.onNativeKeyUp(32);
                SDLActivity.onNativeKeyUp(47);
                SDLActivity.onNativeKeyDown(29);
                SDLActivity.onNativeKeyDown(51);
                break;
        }
    }

    @Override
    public void onTap() {

    }

    @Override
    public void onDoubleTap() {

    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL2",
                "etl"
        };
    }

}
