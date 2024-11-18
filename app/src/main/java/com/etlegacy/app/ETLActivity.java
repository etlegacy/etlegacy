package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageButton;
import android.widget.RelativeLayout;

import com.erz.joysticklibrary.JoyStick;
import com.erz.joysticklibrary.JoyStick.JoyStickListener;
import com.etlegacy.app.web.ETLDownload;

import org.libsdl.app.*;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Objects;

public class ETLActivity extends SDLActivity implements JoyStickListener {

	static volatile boolean UiMenu = false;
	ImageButton btn;

	private RelativeLayout etlLinearLayout;
	private ImageButton shootBtn;
	private ImageButton reloadBtn;
	private ImageButton jumpBtn;
	private ImageButton activateBtn;
	private ImageButton altBtn;
	private ImageButton crouchBtn;
	private JoyStick moveJoystick;
	private Handler handler;
	private Runnable uiRunner;

	/**
	 * Get an uiMenu boolean variable
	 *
	 * @return UiMenu
	 */
	public static boolean getUiMenu() {
		return UiMenu;
	}

	/**
	 * Hide System UI
	 */
	private void hideSystemUI() {
		View decorView = getWindow().getDecorView();
		decorView.setSystemUiVisibility(
			View.SYSTEM_UI_FLAG_IMMERSIVE
			// Set the content to appear under the system bars so that the
			// content doesn't resize when the system bars hide and show.
			| View.SYSTEM_UI_FLAG_LAYOUT_STABLE
			| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
			| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
			// Hide the nav bar and status bar
			| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
			| View.SYSTEM_UI_FLAG_FULLSCREEN);
	}

	/**
	 * Convert pixel metrics to dp
	 *
	 * @param px value of px to be converted
	 * @return dp
	 */
	public static int pxToDp(int px) {
		return (int) (px / Resources.getSystem().getDisplayMetrics().density);
	}

	@Override
	public void finish() {
		// finishAffinity();
		finishAndRemoveTask();
	}

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		}
	}

	@Override
	protected void onDestroy() {
		// shutdown the looper and the download thread executor
		this.handler.removeCallbacks(uiRunner);
		ETLDownload.instance().shutdownExecutor();

		super.onDestroy();

		// FIXME: figure out what is actually keeping this thing alive.
		System.exit(0);
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);

		HIDDeviceManager.acquire(getContext());
		getMotionListener();
		clipboardGetText();

		setRelativeMouseEnabled(true);

		if (isAndroidTV() || isChromebook()) {
			Log.v("ETL", "AndroidTV / ChromeBook Detected, Display UI Disabled!");
			return;
		}

		runOnUiThread(this::setupUI);
	}

	private void setupUI() {
		final ImageButton etl_console = new ImageButton(getApplicationContext());
		etl_console.setImageResource(R.drawable.ic_one_line);
		etl_console.setBackgroundResource(0);
		etl_console.setOnClickListener(v -> {
			onNativeKeyDown(68);
			onNativeKeyUp(68);
		});

		mLayout.addView(etl_console);

		btn = new ImageButton(getApplicationContext());
		btn.setImageResource(R.drawable.ic_keyboard);
		btn.setBackgroundResource(0);
		btn.setId(1);
		btn.setOnClickListener(v -> {
			InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
			imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
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
		esc_btn.setOnClickListener(v -> {
			onNativeKeyDown(111);
			onNativeKeyUp(111);
		});

		RelativeLayout.LayoutParams lp2 = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		lp2.addRule(RelativeLayout.RIGHT_OF, btn.getId());
		lp2.leftMargin = pxToDp(-15);
		lp2.topMargin = -pxToDp(20);

		mLayout.addView(esc_btn, lp2);

		etlLinearLayout = new RelativeLayout(this);
		mLayout.addView(etlLinearLayout);

		// This needs some refactoring
		shootBtn = new ImageButton(getApplicationContext());
		shootBtn.setId(2);
		shootBtn.setImageResource(R.drawable.ic_shoot);
		shootBtn.setBackgroundResource(0);

		reloadBtn = new ImageButton(getApplicationContext());
		reloadBtn.setImageResource(R.drawable.ic_reload);
		reloadBtn.setBackgroundResource(0);

		jumpBtn = new ImageButton(getApplicationContext());
		jumpBtn.setImageResource(R.drawable.ic_jump);
		jumpBtn.setBackgroundResource(0);

		activateBtn = new ImageButton(getApplicationContext());
		activateBtn.setImageResource(R.drawable.ic_use);
		activateBtn.setBackgroundResource(0);

		altBtn = new ImageButton(getApplicationContext());
		altBtn.setImageResource(R.drawable.ic_alt);
		altBtn.setBackgroundResource(0);

		crouchBtn = new ImageButton(getApplicationContext());
		crouchBtn.setImageResource(R.drawable.ic_crouch);
		crouchBtn.setBackgroundResource(0);

		moveJoystick = new JoyStick(getApplicationContext());

		handler = new Handler(Looper.getMainLooper());
		uiRunner = () -> {
			Log.d("LOOPPER", "Running");
			int delay = runUI();
			if (delay > 0) {
				handler.postDelayed(uiRunner, delay);
			}
		};
		handler.post(uiRunner);
	}

	@SuppressLint("ClickableViewAccessibility")
	private int runUI() {
		if (!getUiMenu()) {
			mLayout.removeView(moveJoystick);
			etlLinearLayout.removeAllViews();
			return 500;
		}

		shootBtn.setOnTouchListener((v, event) -> {
			final int touchDevId = event.getDeviceId();
			final int pointerCount = event.getPointerCount();
			int action = event.getActionMasked();
			float x, y, p;
			float mWidth = Resources.getSystem().getDisplayMetrics().widthPixels;
			float mHeight = Resources.getSystem().getDisplayMetrics().heightPixels;
			switch (action) {
				case MotionEvent.ACTION_DOWN: {
					int pointerFingerId = event.getPointerId(0);
					x = event.getX(0) / mWidth;
					y = event.getY(0) / mHeight;
					p = event.getPressure(0);

					onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
					onNativeKeyDown(42);
				}
				break;
				case MotionEvent.ACTION_MOVE:
					for (int i = 0; i < pointerCount; i++) {
						int pointerFingerId = event.getPointerId(i);
						x = event.getX(i) / mWidth;
						y = event.getY(i) / mHeight;
						p = event.getPressure(i);
						onNativeTouch(touchDevId, pointerFingerId, action, x, y, p);
					}
					break;
				case MotionEvent.ACTION_UP:
				case MotionEvent.ACTION_CANCEL:
					for (int i = 0; i < pointerCount; i++) {
						int pointerFingerId = event.getPointerId(i);
						x = event.getX(i) / mWidth;
						y = event.getY(i) / mHeight;
						p = event.getPressure(i);
						onNativeTouch(touchDevId, pointerFingerId, MotionEvent.ACTION_UP, x, y, p);
					}
					onNativeKeyUp(42);
					break;
			}
			return false;
		});

		RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp.addRule(RelativeLayout.CENTER_VERTICAL);

		switch (Resources.getSystem().getDisplayMetrics().densityDpi) {
			case DisplayMetrics.DENSITY_LOW:
			case DisplayMetrics.DENSITY_140:
			case DisplayMetrics.DENSITY_180:
				lp.rightMargin = pxToDp(50 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.width = pxToDp(600 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.height = pxToDp(200 * (int) Resources.getSystem().getDisplayMetrics().density);
				break;
			case DisplayMetrics.DENSITY_200:
			case DisplayMetrics.DENSITY_220:
			case DisplayMetrics.DENSITY_HIGH:
			case DisplayMetrics.DENSITY_260:
			case DisplayMetrics.DENSITY_280:
			case DisplayMetrics.DENSITY_300:
			case DisplayMetrics.DENSITY_XHIGH:
				lp.rightMargin = pxToDp(100 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.width = pxToDp(550 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.height = pxToDp(220 * (int) Resources.getSystem().getDisplayMetrics().density);
				break;
			case DisplayMetrics.DENSITY_340:
			case DisplayMetrics.DENSITY_360:
			case DisplayMetrics.DENSITY_400:
			case DisplayMetrics.DENSITY_420:
			case DisplayMetrics.DENSITY_440:
			case DisplayMetrics.DENSITY_450:
			case DisplayMetrics.DENSITY_XXHIGH:
				lp.rightMargin = pxToDp(500 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.width = pxToDp(900 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.height = pxToDp(500 * (int) Resources.getSystem().getDisplayMetrics().density);
				break;
			case DisplayMetrics.DENSITY_560:
			case DisplayMetrics.DENSITY_600:
			case DisplayMetrics.DENSITY_XXXHIGH:
				lp.rightMargin = pxToDp(900 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.width = pxToDp(1300 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.height = pxToDp(900 * (int) Resources.getSystem().getDisplayMetrics().density);
				break;
			case DisplayMetrics.DENSITY_DEFAULT:
				lp.rightMargin = pxToDp(200 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.width = pxToDp(600 * (int) Resources.getSystem().getDisplayMetrics().density);
				lp.height = pxToDp(200 * (int) Resources.getSystem().getDisplayMetrics().density);
				break;
		}

		if (shootBtn.getParent() == null)
			etlLinearLayout.addView(shootBtn, lp);

		reloadBtn.setOnClickListener(v -> {
			onNativeKeyDown(46);
			onNativeKeyUp(46);
		});

		RelativeLayout.LayoutParams lp_reload = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		lp_reload.addRule(RelativeLayout.BELOW, btn.getId());
		lp_reload.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp_reload.topMargin = pxToDp(220);
		lp_reload.rightMargin = pxToDp(90);

		if (reloadBtn.getParent() == null)
			etlLinearLayout.addView(reloadBtn, lp_reload);

		jumpBtn.setOnTouchListener((v, event) -> {
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
		});

		RelativeLayout.LayoutParams jumpLayout = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		jumpLayout.addRule(RelativeLayout.CENTER_HORIZONTAL);
		jumpLayout.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
		jumpLayout.bottomMargin = pxToDp(-20);

		if (jumpBtn.getParent() == null)
			etlLinearLayout.addView(jumpBtn, jumpLayout);

		activateBtn.setOnClickListener(v -> {
			onNativeKeyDown(34);
			onNativeKeyUp(34);
		});

		RelativeLayout.LayoutParams lp_activate = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		lp_activate.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
		lp_activate.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
		lp_activate.bottomMargin = pxToDp(-20);
		lp_activate.leftMargin = pxToDp(400);

		if (activateBtn.getParent() == null)
			etlLinearLayout.addView(activateBtn, lp_activate);

		altBtn.setOnClickListener(v -> {
			onNativeKeyDown(30);
			onNativeKeyUp(30);
		});

		RelativeLayout.LayoutParams lp_alternative = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		lp_alternative.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp_alternative.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
		lp_alternative.bottomMargin = pxToDp(-20);
		lp_alternative.rightMargin = pxToDp(400);

		if (altBtn.getParent() == null)
			etlLinearLayout.addView(altBtn, lp_alternative);

		crouchBtn.setOnTouchListener((v, event) -> {
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
		});

		RelativeLayout.LayoutParams lp_crouch = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.WRAP_CONTENT,
			ViewGroup.LayoutParams.WRAP_CONTENT);

		lp_crouch.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp_crouch.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
		lp_crouch.bottomMargin = pxToDp(-20);

		if (crouchBtn.getParent() == null)
			etlLinearLayout.addView(crouchBtn, lp_crouch);

		moveJoystick.setListener(ETLActivity.this);
		moveJoystick.setPadColor(Color.TRANSPARENT);
		moveJoystick.setButtonColor(Color.TRANSPARENT);
		moveJoystick.setButtonRadiusScale(50);

		RelativeLayout.LayoutParams joystick_layout = new RelativeLayout.LayoutParams(
			400,
			350);

		joystick_layout.addRule(RelativeLayout.ALIGN_LEFT);
		joystick_layout.addRule(RelativeLayout.CENTER_VERTICAL);
		joystick_layout.leftMargin = pxToDp(10);

		if (moveJoystick.getParent() == null)
			mLayout.addView(moveJoystick, joystick_layout);

		return 2000;
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		if (hasFocus) {
			hideSystemUI();
		}
		super.onWindowFocusChanged(hasFocus);
	}

	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		// Check that the event came from a game controller
		if (((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
			 (event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) && event.getAction() == MotionEvent.ACTION_MOVE) {
			SDLControllerManager.handleJoystickMotionEvent(event);
			return true;
		}
		return super.onGenericMotionEvent(event);
	}

	@Override
	public void onMove(JoyStick joyStick, double angle, double power, int direction) {

		if (isAndroidTV() || isChromebook()) {
			Log.d("MOVE", "Ignoring onMove, since the device not a mobile device");
			return;
		}

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

	@Override
	public void onTap() {

	}

	@Override
	public void onDoubleTap() {

	}

	@Override
	protected String[] getArguments() {
		return new String[]{
			String.valueOf(Paths.get((Objects.requireNonNull(getExternalFilesDir(null)).toPath() + "/etlegacy"))),
		};
	}

	@Override
	protected String[] getLibraries() {
		return new String[]{
			"etl"
		};
	}

}
