package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Objects;

public class SetupUIThemeActivity extends AppCompatActivity {

	private static final String PREFS_NAME = "ComponentPrefs";
	private static final String COMPONENT_MAP_KEY = "componentMap";
	private HashMap<String, ComponentManager.ComponentData> componentMap = new HashMap<>();

	private final int[][] icons = {
		/* esc_btn                 jumpBtn              etl_console              reloadBtn                shootBtn */
		{ R.drawable.ic_escape,    R.drawable.ic_jump,  R.drawable.ic_one_line,  R.drawable.ic_reload,    R.drawable.ic_shoot,
			/* crouchBtn             activateBtn          moveJoystick             btn                      gears */
			R.drawable.ic_crouch,    R.drawable.ic_use,   0,                       R.drawable.ic_keyboard,  R.drawable.gears,
			/* altBtn */
			R.drawable.ic_alt, R.drawable.keycap },

		/* esc_btn                          jumpBtn                          etl_console */
		{ R.drawable.deltatouch_btn_escape, R.drawable.deltatouch_btn_jump,  R.drawable.deltatouch_btn_notepad,
			/* reloadBtn                      shootBtn                         crouchBtn */
			R.drawable.deltatouch_btn_reload, R.drawable.deltatouch_btn_sht,   R.drawable.deltatouch_btn_crouch,
			/* activateBtn                    moveJoystick                     btn */
			R.drawable.deltatouch_btn_activate, 0,                             R.drawable.deltatouch_btn_keyboard,
			/* gears                           altBtn */
			R.drawable.gears,                   0, R.drawable.keycap },

		/* esc_btn                      jumpBtn                      etl_console */
		{ R.drawable.tech4a_btn_pause,  R.drawable.tech4a_btn_jump,  R.drawable.tech4a_btn_notepad,
			/* reloadBtn                  shootBtn                     crouchBtn */
			R.drawable.tech4a_btn_reload, R.drawable.tech4a_btn_sht,   R.drawable.tech4a_btn_crouch,
			/* activateBtn                moveJoystick                 btn */
			R.drawable.tech4a_btn_activate, 0,                         R.drawable.tech4a_btn_keyboard,
			/* gears                       altBtn */
			R.drawable.gears,              R.drawable.tech4a_btn_altfire, R.drawable.keycap }
	};
	private int currentIndex = 0;
	private GestureDetector gestureDetector;

	/**
	 * Hide System UI
	 */
	private void hideSystemUI() {
		getWindow().getDecorView().setSystemUiVisibility(
			View.SYSTEM_UI_FLAG_IMMERSIVE
				// Set the content to appear under the system bars so that the
				// content doesn't resize when the system bars hide and show.
				| View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
				// Hide the nav bar and status bar
				| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
				| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_FULLSCREEN);
	}

	@SuppressLint("ClickableViewAccessibility")
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		int i = 0; // Index tracker

		hideSystemUI();

		getWindow().setBackgroundDrawable(new ColorDrawable(android.graphics.Color.TRANSPARENT));
		setContentView(R.layout.activity_ui_position);

		gestureDetector = new GestureDetector(this, new SetupUIThemeActivity.SwipeGestureListener());

		View theme_view = new View(this);
		FrameLayout.LayoutParams theme_params = new FrameLayout.LayoutParams(
			ViewGroup.LayoutParams.MATCH_PARENT,
			ViewGroup.LayoutParams.MATCH_PARENT
		);
		theme_view.setLayoutParams(theme_params);
		theme_view.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));
		theme_view.setClickable(true);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		}

		LoadComponentData();

		FrameLayout rootLayout = findViewById(R.id.rootLayout);

		rootLayout.addView(theme_view);

		for (String key : componentMap.keySet()) {
			ComponentManager.ComponentData componentData = componentMap.get(key);

			ImageView imageview = new ImageView(this);
			assert componentData != null;
			FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
				componentData.width,
				componentData.height
			);
			params.gravity = componentData.gravity;
			params.setMargins(
				componentData.margins[0],
				componentData.margins[1],
				componentData.margins[2],
				componentData.margins[3]
			);

			// Ensure we don't go out of bounds
			if (i < icons[currentIndex].length) {
				int icon = icons[currentIndex][i];
				imageview.setImageResource(icon);
				Log.v("SetupUIThemeActivity", "Assigned icon: " + icon + " to key: " + key);
			}

			imageview.setLayoutParams(params);
			rootLayout.addView(imageview);
			i++;
		}
	}

	private void SaveComponentData() {
		SharedPreferences sharedPreferences = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = sharedPreferences.edit();
		Gson gson = new Gson();
		String json = gson.toJson(componentMap);
		editor.putString(COMPONENT_MAP_KEY, json);
		editor.apply();
		Intent intent = new Intent("REFRESH_ACTION");
		LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
	}

	private void LoadComponentData() {
		SharedPreferences sharedPreferences = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
		Gson gson = new Gson();
		String json = sharedPreferences.getString(COMPONENT_MAP_KEY, null);
		Type type = new TypeToken<HashMap<String, ComponentManager.ComponentData>>() {}.getType();
		componentMap = gson.fromJson(json, type);
		assert componentMap != null;
		Log.v("SetupUIThemeActivity", "LoadComponentData: " + componentMap);
	}

	class SwipeGestureListener extends GestureDetector.SimpleOnGestureListener {
		private static final int SWIPE_THRESHOLD = 100;
		private static final int SWIPE_VELOCITY_THRESHOLD = 100;

		@Override
		public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
			float diffX = e2.getX() - Objects.requireNonNull(e1).getX();
			if (Math.abs(diffX) > SWIPE_THRESHOLD && Math.abs(velocityX) > SWIPE_VELOCITY_THRESHOLD) {
				if (diffX > 0) {
					swipeRight();
				} else {
					swipeLeft();
				}
				return true;
			}
			return false;
		}
	}

	private void swipeLeft() {
		currentIndex = (currentIndex + 1) % icons.length;
		changeTheme();
	}

	private void swipeRight() {
		currentIndex = (currentIndex - 1 + icons.length) % icons.length;
		changeTheme();
	}

	private void changeTheme() {
		int i = 0;
		for (String key : componentMap.keySet()) {
			if (i < icons[currentIndex].length) {
				Objects.requireNonNull(componentMap.get(key)).resourceId = icons[currentIndex][i];
			}
			i++;
		}
		SaveComponentData();
	}
}
