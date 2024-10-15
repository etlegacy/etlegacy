package com.etlegacy.app;

import android.os.Build;
import android.os.Bundle;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import com.etlegacy.app.web.ETLDownload;

import org.libsdl.app.*;

public class ETLActivity extends SDLActivity {

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
		// shutdown the download thread executor
		ETLDownload.instance().shutdownExecutor();

		super.onDestroy();

		// FIXME: figure out what is actually keeping this thing alive.
		System.exit(0);
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);
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
	protected String[] getLibraries() {
		return new String[]{
			"etl"
		};
	}

}
