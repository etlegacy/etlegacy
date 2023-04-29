package org.etlegacy.app;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageButton;
import android.widget.RelativeLayout;

import com.erz.joysticklibrary.JoyStick;
import com.erz.joysticklibrary.JoyStick.JoyStickListener;

import org.libsdl.app.*;

public class ETLActivity extends SDLActivity implements JoyStickListener {

    static volatile boolean UiMenu = false;
	int fixdpi;
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
        final ImageButton btn2 = new ImageButton(getApplicationContext());
        btn2.setId(2);
		btn2.setImageResource(R.drawable.ic_shoot);
        btn2.setBackgroundResource(0);

        final ImageButton btn_reload = new ImageButton(getApplicationContext());
        btn_reload.setImageResource(R.drawable.ic_reload);
        btn_reload.setBackgroundResource(0);

        final ImageButton btn_jump = new ImageButton(getApplicationContext());
        btn_jump.setImageResource(R.drawable.ic_jump);
        btn_jump.setBackgroundResource(0);

        final ImageButton btn_activate = new ImageButton(getApplicationContext());
        btn_activate.setImageResource(R.drawable.ic_use);
        btn_activate.setBackgroundResource(0);

        final ImageButton btn_alternative = new ImageButton(getApplicationContext());
        btn_alternative.setImageResource(R.drawable.ic_alt);
        btn_alternative.setBackgroundResource(0);

        final ImageButton btn_crouch = new ImageButton(getApplicationContext());
        btn_crouch.setImageResource(R.drawable.ic_crouch);
        btn_crouch.setBackgroundResource(0);

        final JoyStick joyStick_left = new JoyStick(getApplicationContext());

        final Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            public void run() {

                if (getUiMenu() == true) {
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

                                    onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
                                    onNativeKeyDown(42);
                                    break;
                                case MotionEvent.ACTION_MOVE:
                                    for (i = 0; i < pointerCount; i++) {
                                        pointerFingerId = event.getPointerId(i);
                                        x = event.getX(i) / mWidth;
                                        y = event.getY(i) / mHeight;
                                        p = event.getPressure(i);
                                        onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
                                    }
                                    break;
                                case MotionEvent.ACTION_UP:
                                case MotionEvent.ACTION_CANCEL:
                                    for (i = 0; i < pointerCount; i++) {
                                        pointerFingerId = event.getPointerId(i);
                                        x = event.getX(i) / mWidth;
                                        y = event.getY(i) / mHeight;
                                        p = event.getPressure(i);
                                        onNativeTouch(touchDevId, pointerFingerId, MotionEvent.ACTION_UP, x, y, p);
                                    }
                                    onNativeKeyUp(42);
                                    break;
                            }
                            return false;
                        }
                    });

                    RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
							ViewGroup.LayoutParams.WRAP_CONTENT,
							ViewGroup.LayoutParams.WRAP_CONTENT);

					Log.v("ETL", String.valueOf(fixdpi));
					lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                    lp.addRule(RelativeLayout.CENTER_VERTICAL);
                    lp.rightMargin = pxToDp(400 * fixdpi);

                    if (btn2.getParent() == null) {
						etl_linearLayout.addView(btn2, lp);
					}

                    btn_reload.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            onNativeKeyDown(46);
                            onNativeKeyUp(46);
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
                                    onNativeKeyDown(62);
                                    break;
                                case MotionEvent.ACTION_UP:
                                case MotionEvent.ACTION_CANCEL:
                                    onNativeKeyUp(62);
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
                            onNativeKeyDown(34);
                            onNativeKeyUp(34);
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
                            onNativeKeyDown(30);
                            onNativeKeyUp(30);
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
                                    onNativeKeyDown(31);
                                    break;
                                case MotionEvent.ACTION_UP:
                                case MotionEvent.ACTION_CANCEL:
                                    onNativeKeyUp(31);
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
		//Log.v("ETL", String.valueOf(Resources.getSystem().getDisplayMetrics().density));
		//Log.v("ETL", String.valueOf(Resources.getSystem().getDisplayMetrics().densityDpi));
        return (int) (px / Resources.getSystem().getDisplayMetrics().density);
    }

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		}

		// FIXME: WTF IT DOES NOT SEEMS TO WORK AS INTENDED
		switch ((int) Resources.getSystem().getDisplayMetrics().densityDpi) {
			case DisplayMetrics.DENSITY_LOW:
			case DisplayMetrics.DENSITY_MEDIUM:
				fixdpi = (int) 1.0;
				break;
			case DisplayMetrics.DENSITY_HIGH:
			case DisplayMetrics.DENSITY_XHIGH:
				fixdpi = (int) 2.0;
				break;
			case DisplayMetrics.DENSITY_XXHIGH:
				fixdpi = (int) 3.0;
				break;
			case DisplayMetrics.DENSITY_XXXHIGH:
				fixdpi = (int) 4.0;
				break;
			default:
				fixdpi = 0;
		}
	}

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);

        clipboardGetText();

        if(Build.VERSION.SDK_INT > Build.VERSION_CODES.M)
            setRelativeMouseEnabled(true);
        else
            setRelativeMouseEnabled(false);

        if (isAndroidTV() || isChromebook()) {
            Log.v("ETL", "AndroidTV / ChromeBook Detected, Display UI Disabled!");
        } else {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    final ImageButton etl_console = new ImageButton(getApplicationContext());
                    etl_console.setImageResource(R.drawable.ic_one_line);
                    etl_console.setBackgroundResource(0);
                    etl_console.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            onNativeKeyDown(68);
                            onNativeKeyUp(68);
                        }
                    });

                    mLayout.addView(etl_console);

                    btn = new ImageButton(getApplicationContext());
                    btn.setImageResource(R.drawable.ic_keyboard);
                    btn.setBackgroundResource(0);
                    btn.setId(1);
                    btn.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                            imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                        }
                    });

                    RelativeLayout.LayoutParams keyboard_layout = new RelativeLayout.LayoutParams(
                            ViewGroup.LayoutParams.WRAP_CONTENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT);

                    keyboard_layout.leftMargin = pxToDp(390);
                    keyboard_layout.topMargin = pxToDp(-20);
                    mLayout.addView(btn, keyboard_layout);

                    ImageButton esc_btn = new ImageButton(getApplicationContext());
                    esc_btn.setImageResource(R.drawable.ic_escape);
                    esc_btn.setBackgroundResource(0);
                    esc_btn.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            onNativeKeyDown(111);
                            onNativeKeyUp(111);
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
            });

        }

    }

    @Override
    public void onMove(JoyStick joyStick, double angle, double power, int direction) {

        if (isAndroidTV() || isChromebook()) {
        } else {
            switch (direction) {
                case JoyStick.DIRECTION_CENTER:
                    onNativeKeyUp(51);
                    onNativeKeyUp(32);
                    onNativeKeyUp(47);
                    onNativeKeyUp(29);
                    break;
                case JoyStick.DIRECTION_UP:
                    onNativeKeyUp(32);
                    onNativeKeyUp(47);
                    onNativeKeyUp(29);
                    onNativeKeyDown(51);
                    break;
                case JoyStick.DIRECTION_UP_RIGHT:
                    onNativeKeyUp(47);
                    onNativeKeyUp(29);
                    onNativeKeyDown(51);
                    onNativeKeyDown(32);
                    break;
                case JoyStick.DIRECTION_RIGHT:
                    onNativeKeyUp(51);
                    onNativeKeyUp(47);
                    onNativeKeyUp(29);
                    onNativeKeyDown(32);
                    break;
                case JoyStick.DIRECTION_RIGHT_DOWN:
                    onNativeKeyUp(51);
                    onNativeKeyUp(29);
                    onNativeKeyDown(32);
                    onNativeKeyDown(47);
                    break;
                case JoyStick.DIRECTION_DOWN:
                    onNativeKeyUp(51);
                    onNativeKeyUp(32);
                    onNativeKeyUp(29);
                    onNativeKeyDown(47);
                    break;
                case JoyStick.DIRECTION_DOWN_LEFT:
                    onNativeKeyUp(51);
                    onNativeKeyUp(32);
                    onNativeKeyDown(47);
                    onNativeKeyDown(29);
                    break;
                case JoyStick.DIRECTION_LEFT:
                    onNativeKeyUp(51);
                    onNativeKeyUp(32);
                    onNativeKeyUp(47);
                    onNativeKeyDown(29);
                    break;
                case JoyStick.DIRECTION_LEFT_UP:
                    onNativeKeyUp(32);
                    onNativeKeyUp(47);
                    onNativeKeyDown(29);
                    onNativeKeyDown(51);
                    break;
            }
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
