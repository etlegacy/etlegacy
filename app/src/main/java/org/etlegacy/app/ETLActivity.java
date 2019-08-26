package org.etlegacy.app;

import android.app.AlertDialog;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.erz.joysticklibrary.JoyStick;
import com.erz.joysticklibrary.JoyStick.JoyStickListener;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class ETLActivity extends SDLActivity implements JoyStickListener
{
    private String dir_etl;

    private boolean checkGameFiles() {

        ContextWrapper c = new ContextWrapper(this);
        dir_etl = c.getFilesDir().getPath();
        //Toast.makeText(this, c.getFilesDir().getPath(), Toast.LENGTH_LONG).show();

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
        joystick_layout.leftMargin =     10;

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
