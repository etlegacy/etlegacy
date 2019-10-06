package org.etlegacy.app;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageButton;
import android.widget.RelativeLayout;

import com.erz.joysticklibrary.JoyStick;
import com.erz.joysticklibrary.JoyStick.JoyStickListener;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class ETLActivity extends SDLActivity implements JoyStickListener
{
    private String dir_etl;
    private static Context context;

    public static int pxToDp(int px) {
        return (int) (px / Resources.getSystem().getDisplayMetrics().density);
    }

    private boolean checkGameFiles() {

        File etmain_dir = new File(dir_etl.concat("etmain"));
        if(etmain_dir.exists() == false)
        {
            Log.e("SDL","Creating" + etmain_dir + "etmain");
            etmain_dir.mkdir();
        }

        File legacy_dir = new File(dir_etl.concat("legacy"));
        if(legacy_dir.exists() == false)
        {
            Log.e("SDL", "Creating" + legacy_dir + "legacy");
            legacy_dir.mkdir();
        }

        /* Draw Dialog Box */
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getApplicationContext());
        alertDialogBuilder.setTitle("ET Legacy");
        alertDialogBuilder.setMessage("Do you want to download files required by game to run ?")
                .setCancelable(false)
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // TODO:Implement DownloadManager here.
                        dialog.cancel();
                    }
                })
                .setNegativeButton("No", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // if clicked no finish activity and close app
                        ETLActivity.this.finish();
                    }
                });
        AlertDialog alertDialog = alertDialogBuilder.create();

        /* Check if pak0.pk3-pak2.pk3 are around */
        for(int i=0; i<3; i++)
        {
            String pak_filename = "pak" + i + ".pk3";
            File etl_pak_file = new File(dir_etl.concat("etmain").concat("/") + pak_filename);
            if(etl_pak_file.exists() == false)
            {
                alertDialog.show();
            }
        }
        return true;
    }

    private Bitmap getBitmapFromAsset(String strName) {
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

        ImageButton btn = new ImageButton(getApplicationContext());
        btn.setImageBitmap(getBitmapFromAsset("btn_keyboard.png"));
        btn.setBackgroundResource(0);
        btn.setId(1);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED,0);
            }
        });

        RelativeLayout.LayoutParams keyboard_layout = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        keyboard_layout.leftMargin = 210;
        keyboard_layout.topMargin = -20;
        mLayout.addView(btn, keyboard_layout);

        ImageButton esc_btn = new ImageButton(getApplicationContext());
        esc_btn.setImageBitmap(getBitmapFromAsset("btn_esc.png"));
        esc_btn.setBackgroundResource(0);
        esc_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyDown(111);
                SDLActivity.onNativeKeyUp(111);
            }
        });

        RelativeLayout.LayoutParams lp2 = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        lp2.addRule(RelativeLayout.RIGHT_OF, btn.getId());
        lp2.leftMargin = -15;
        lp2.topMargin = -20;

        mLayout.addView(esc_btn, lp2);

        ImageButton btn2 = new ImageButton(getApplicationContext());
        btn2.setId(2);
        btn2.setImageBitmap(getBitmapFromAsset("btn_sht.png"));
        btn2.setBackgroundResource(0);
        btn2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyDown(42);
                SDLActivity.onNativeKeyUp(42);
            }
        });

        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
                150,
                150);

        lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        lp.addRule(RelativeLayout.CENTER_VERTICAL);
        lp.rightMargin = 200;

        mLayout.addView(btn2, lp);


        ImageButton btn_reload = new ImageButton(getApplicationContext());
        btn_reload.setImageBitmap(getBitmapFromAsset("btn_reload.png"));
        btn_reload.setBackgroundResource(0);
        btn_reload.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyDown(46);
                SDLActivity.onNativeKeyUp(46);
            }
        });

        RelativeLayout.LayoutParams lp_reload = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        lp_reload.addRule(RelativeLayout.BELOW, btn2.getId());
        lp_reload.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        lp_reload.addRule(RelativeLayout.CENTER_VERTICAL);
        lp_reload.rightMargin = 90;

        mLayout.addView(btn_reload, lp_reload);


        ImageButton btn_jump = new ImageButton(getApplicationContext());
        btn_jump.setImageBitmap(getBitmapFromAsset("btn_jump.png"));
        btn_jump.setBackgroundResource(0);
        btn_jump.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyUp(62);
                SDLActivity.onNativeKeyDown(62);
            }
        });

        RelativeLayout.LayoutParams lp_jump = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        lp_jump.addRule(RelativeLayout.CENTER_HORIZONTAL);
        lp_jump.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        lp_jump.bottomMargin = -20;

        mLayout.addView(btn_jump, lp_jump);

        ImageButton btn_activate = new ImageButton(getApplicationContext());
        btn_activate.setImageBitmap(getBitmapFromAsset("btn_activate.png"));
        btn_activate.setBackgroundResource(0);
        btn_activate.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyDown(34);
                SDLActivity.onNativeKeyUp(34);
            }
        });

        RelativeLayout.LayoutParams lp_activate = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        lp_activate.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lp_activate.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        lp_activate.bottomMargin = -20;
        lp_activate.leftMargin = pxToDp(400);

        mLayout.addView(btn_activate, lp_activate);

        ImageButton btn_alternative = new ImageButton(getApplicationContext());
        btn_alternative.setImageBitmap(getBitmapFromAsset("btn_altfire.png"));
        btn_alternative.setBackgroundResource(0);
        btn_alternative.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SDLActivity.onNativeKeyDown(36);
                SDLActivity.onNativeKeyUp(36);
            }
        });

        RelativeLayout.LayoutParams lp_alternative = new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        lp_alternative.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        lp_alternative.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        lp_alternative.bottomMargin = -20;
        lp_alternative.rightMargin = pxToDp(400);

        mLayout.addView(btn_alternative, lp_alternative);


        JoyStick joyStick_left = new JoyStick(getApplicationContext());

        joyStick_left.setListener(this);
        joyStick_left.setPadColor(Color.TRANSPARENT);
        joyStick_left.setButtonColor(Color.TRANSPARENT);
        joyStick_left.setButtonRadiusScale(50);

        RelativeLayout.LayoutParams joystick_layout = new RelativeLayout.LayoutParams(
                400,
                400);

        joystick_layout.addRule(RelativeLayout.ALIGN_LEFT);
        joystick_layout.addRule(RelativeLayout.CENTER_VERTICAL);
        joystick_layout.leftMargin = 10;

        mLayout.addView(joyStick_left, joystick_layout);

    }

    @Override
    public void onMove(JoyStick joyStick, double angle, double power, int direction) {

        switch (direction) {
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
                "hidapi",
                "SDL2",
                "etl"
        };
    }

}
