package org.etlegacy.app;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageButton;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;

import com.erz.joysticklibrary.JoyStick;
import com.erz.joysticklibrary.JoyStick.JoyStickListener;

import org.libsdl.app.SDLActivity;

import java.io.IOException;
import java.io.InputStream;


public class ETLActivity extends SDLActivity implements JoyStickListener {

    static volatile boolean UiMenu = false;
    ImageButton btn;

    /**
     * Get an uiMenu boolean variable
     * @return UiMenu
     */
    public static boolean getUiMenu() {
        return UiMenu;
    }

    /**
     * RunUI Function
     */
    public void runUI() {

        final RelativeLayout etl_linearLayout =  new RelativeLayout(this);
        mLayout.addView(etl_linearLayout);

        // This needs some refactoring
        final ImageButton buttonPopUpMenu = new ImageButton(getApplicationContext());
        buttonPopUpMenu.setImageBitmap(getBitmapFromAsset("btn_menu.png"));
        buttonPopUpMenu.setBackgroundResource(0);

        final ImageButton btn2 = new ImageButton(getApplicationContext());
        btn2.setId(2);
        btn2.setImageBitmap(getBitmapFromAsset("btn_sht.png"));
        btn2.setBackgroundResource(0);

        final ImageButton btn_reload = new ImageButton(getApplicationContext());
        btn_reload.setImageBitmap(getBitmapFromAsset("btn_reload.png"));
        btn_reload.setBackgroundResource(0);

        final ImageButton btn_jump = new ImageButton(getApplicationContext());
        btn_jump.setImageBitmap(getBitmapFromAsset("btn_jump.png"));
        btn_jump.setBackgroundResource(0);

        final ImageButton btn_activate = new ImageButton(getApplicationContext());
        btn_activate.setImageBitmap(getBitmapFromAsset("btn_activate.png"));
        btn_activate.setBackgroundResource(0);

        final ImageButton btn_alternative = new ImageButton(getApplicationContext());
        btn_alternative.setImageBitmap(getBitmapFromAsset("btn_altfire.png"));
        btn_alternative.setBackgroundResource(0);

        final ImageButton btn_crouch = new ImageButton(getApplicationContext());
        btn_crouch.setImageBitmap(getBitmapFromAsset("btn_crouch.png"));
        btn_crouch.setBackgroundResource(0);

        final JoyStick joyStick_left = new JoyStick(getApplicationContext());

        final Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            public void run() {

                if (getUiMenu() == true) {
                    buttonPopUpMenu.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            PopupMenu etl_PopMenu = new PopupMenu(getApplicationContext(), buttonPopUpMenu);
                            etl_PopMenu.getMenu().add(0, 0, 0, "~");
                            etl_PopMenu.getMenu().add(1, 1, 1, "F1");
                            etl_PopMenu.getMenu().add(2, 2, 2, "F2");
                            etl_PopMenu.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem item) {

                                    switch (item.getItemId()) {
                                        case 0:
                                            SDLActivity.onNativeKeyDown(68);
                                            SDLActivity.onNativeKeyUp(68);
                                        case 1:
                                            SDLActivity.onNativeKeyDown(131);
                                            SDLActivity.onNativeKeyUp(131);
                                            break;
                                        case 2:
                                            SDLActivity.onNativeKeyDown(132);
                                            SDLActivity.onNativeKeyUp(132);
                                            break;
                                    }
                                    return false;
                                }
                            });
                            etl_PopMenu.show();
                        }
                    });

                    if (buttonPopUpMenu.getParent() == null)
                        mLayout.addView(buttonPopUpMenu);

                    btn2.setOnTouchListener(new View.OnTouchListener() {
                        @Override
                        public boolean onTouch(View v, MotionEvent event) {
                            final int touchDevId = event.getDeviceId();
                            final int pointerCount = event.getPointerCount();
                            int action = event.getActionMasked();
                            int pointerFingerId;
                            int i = -1;
                            float x,y,p;
                            float mWidth = Resources.getSystem().getDisplayMetrics().widthPixels;
                            float mHeight = Resources.getSystem().getDisplayMetrics().heightPixels;
                            switch (action) {
                                case MotionEvent.ACTION_DOWN:
                                    i = 0;
                                    pointerFingerId = event.getPointerId(i);
                                    x = event.getX(i) / mWidth;
                                    y = event.getY(i) / mHeight;
                                    p = event.getPressure(i);

                                    SDLActivity.onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
                                    SDLActivity.onNativeKeyDown(42);
                                    break;
                                case MotionEvent.ACTION_MOVE:
                                    for (i = 0; i < pointerCount; i++) {
                                        pointerFingerId = event.getPointerId(i);
                                        x = event.getX(i) / mWidth;
                                        y = event.getY(i) / mHeight;
                                        p = event.getPressure(i);
                                        SDLActivity.onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
                                    }
                                    break;
                                case MotionEvent.ACTION_UP:
                                case MotionEvent.ACTION_CANCEL:
                                    for (i = 0; i < pointerCount; i++) {
                                        pointerFingerId = event.getPointerId(i);
                                        x = event.getX(i) / mWidth;
                                        y = event.getY(i) / mHeight;
                                        p = event.getPressure(i);
                                        SDLActivity.onNativeTouch(touchDevId, pointerFingerId, MotionEvent.ACTION_UP, x, y, p);
                                    }
                                    SDLActivity.onNativeKeyUp(42);
                                    break;
                            }
                            return false;
                        }
                    });

                    RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
                            pxToDp(450),
                            pxToDp(350));

                    lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                    lp.addRule(RelativeLayout.CENTER_VERTICAL);
                    lp.rightMargin = pxToDp(300);

                    if (btn2.getParent() == null)
                        etl_linearLayout.addView(btn2, lp);

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

                    lp_reload.addRule(RelativeLayout.BELOW, btn.getId());
                    lp_reload.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                    lp_reload.topMargin = pxToDp(220);
                    lp_reload.rightMargin = pxToDp(90);

                    if (btn_reload.getParent() == null)
                        etl_linearLayout.addView(btn_reload, lp_reload);

                    btn_jump.setOnTouchListener(new View.OnTouchListener() {
                        @Override
                        public boolean onTouch(View v, MotionEvent event) {
                            switch (event.getAction()) {
                                case MotionEvent.ACTION_DOWN:
                                    SDLActivity.onNativeKeyDown(62);
                                    break;
                                case MotionEvent.ACTION_UP:
                                case MotionEvent.ACTION_CANCEL:
                                    SDLActivity.onNativeKeyUp(62);
                                    break;
                            }
                            return false;
                        }
                    });

                    RelativeLayout.LayoutParams lp_jump = new RelativeLayout.LayoutParams(
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);

                    lp_jump.addRule(RelativeLayout.CENTER_HORIZONTAL);
                    lp_jump.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
                    lp_jump.bottomMargin = pxToDp(-20);

                    if (btn_jump.getParent() == null)
                    etl_linearLayout.addView(btn_jump, lp_jump);

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
                    lp_activate.bottomMargin = pxToDp(-20);
                    lp_activate.leftMargin = pxToDp(400);

                    if (btn_activate.getParent() == null)
                        etl_linearLayout.addView(btn_activate, lp_activate);

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
                    lp_alternative.bottomMargin = pxToDp(-20);
                    lp_alternative.rightMargin = pxToDp(400);

                    if (btn_alternative.getParent() == null)
                        etl_linearLayout.addView(btn_alternative, lp_alternative);

                    btn_crouch.setOnTouchListener(new View.OnTouchListener() {
                        @Override
                        public boolean onTouch(View v, MotionEvent event) {
                            switch (event.getAction()) {
                                case MotionEvent.ACTION_DOWN:
                                    SDLActivity.onNativeKeyDown(31);
                                    break;
                                case MotionEvent.ACTION_UP:
                                case MotionEvent.ACTION_CANCEL:
                                    SDLActivity.onNativeKeyUp(31);
                                    break;
                            }
                            return false;
                        }
                    });

                    RelativeLayout.LayoutParams lp_crouch = new RelativeLayout.LayoutParams(
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);

                    lp_crouch.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                    lp_crouch.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
                    lp_crouch.bottomMargin = pxToDp(-20);

                    if ( btn_crouch.getParent() == null)
                        etl_linearLayout.addView(btn_crouch, lp_crouch);

                    joyStick_left.setListener(ETLActivity.this);
                    joyStick_left.setPadColor(Color.TRANSPARENT);
                    joyStick_left.setButtonColor(Color.TRANSPARENT);
                    joyStick_left.setButtonRadiusScale(50);

                    RelativeLayout.LayoutParams joystick_layout = new RelativeLayout.LayoutParams(
                            400,
                            350);

                    joystick_layout.addRule(RelativeLayout.ALIGN_LEFT);
                    joystick_layout.addRule(RelativeLayout.CENTER_VERTICAL);
                    joystick_layout.leftMargin = pxToDp(10);

                    if (joyStick_left.getParent() == null)
                        mLayout.addView(joyStick_left, joystick_layout);

                    handler.postDelayed(this, 2000);
                    }
                else {
                    mLayout.removeView(buttonPopUpMenu);
                    mLayout.removeView(joyStick_left);
                    etl_linearLayout.removeAllViews();
                    handler.postDelayed(this, 500);
                }
            }
        });
    }

    /**
     * Convert pixel metrics to dp
     * @param px value of px to be converted
     * @return dp
     */
    public static int pxToDp(int px) {
        return (int) (px / Resources.getSystem().getDisplayMetrics().density);
    }

    /**
     * Get an image stored from Asset Dir of an App
     * @param strName name of the image to get
     * @return "resized" image
     */
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
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);

        mLayout.requestFocus();

        btn = new ImageButton(getApplicationContext());
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

        keyboard_layout.leftMargin = pxToDp(390);
        keyboard_layout.topMargin = pxToDp(-20);
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
        lp2.leftMargin = pxToDp(-15);
        lp2.topMargin = -pxToDp(20);

        mLayout.addView(esc_btn, lp2);

        runUI();

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
