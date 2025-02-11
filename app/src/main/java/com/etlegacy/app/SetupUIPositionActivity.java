package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.lang.reflect.Type;
import java.util.HashMap;


public class SetupUIPositionActivity extends AppCompatActivity {


	private static final String PREFS_NAME = "ComponentPrefs";
	private static final String COMPONENT_MAP_KEY = "componentMap";
	private HashMap<String, ComponentManager.ComponentData> componentMap = new HashMap<>();

	private View selectedView;
	private float initialTouchX, initialTouchY;
	private float viewInitialX, viewInitialY;

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

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		hideSystemUI();

		getWindow().setBackgroundDrawable(new ColorDrawable(android.graphics.Color.TRANSPARENT));
		setContentView(R.layout.activity_ui_position);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		}

		LoadComponentData();

		FrameLayout rootLayout = findViewById(R.id.rootLayout);

		for (String key : componentMap.keySet()) {
			ComponentManager.ComponentData componentData = componentMap.get(key);

			View view = createMovableView(key);
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

			view.setLayoutParams(params);
			rootLayout.addView(view);
		}
	}

	private View createMovableView(String tag) {
		View view = new View(this);
		view.setBackgroundResource(R.drawable.border_drawable);
		//view.setBackgroundColor(getResources().getColor(android.R.color.holo_red_dark, this.getTheme()));
		view.setTag(tag);
		view.setOnTouchListener(new View.OnTouchListener() {
			@SuppressLint("ClickableViewAccessibility")
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				switch (event.getAction()) {
					case MotionEvent.ACTION_DOWN:
						selectedView = v;
						initialTouchX = event.getRawX();
						initialTouchY = event.getRawY();
						viewInitialX = v.getX();
						viewInitialY = v.getY();
						break;

					case MotionEvent.ACTION_MOVE:
						if (selectedView != null) {
							float deltaX = event.getRawX() - initialTouchX;
							float deltaY = event.getRawY() - initialTouchY;

							selectedView.setX(viewInitialX + deltaX);
							selectedView.setY(viewInitialY + deltaY);
						}
						break;

					case MotionEvent.ACTION_UP:
						if (selectedView != null) {
							SaveComponentPosition(selectedView);
							selectedView = null;
						}
						break;
				}
				return true;
			}
		});

		return view;
	}

	private void SaveComponentPosition(View view) {
		String tag = (String) view.getTag();
		if (tag == null || !componentMap.containsKey(tag))
			return;

		int[] margins = new int[4];
		margins[0] = (int) view.getX();
		margins[1] = (int) view.getY();
		margins[2] = 0; // Not using right margin
		margins[3] = 0; // Not using bottom margin

		ComponentManager.ComponentData updatedData = componentMap.get(tag);
		assert updatedData != null;
		updatedData.margins[0]= margins[0];
		updatedData.margins[1]= margins[1];

		componentMap.put(tag, updatedData);

		SaveComponentData();

		Toast.makeText(this, "Position saved for " + tag, Toast.LENGTH_SHORT).show();
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
		Log.v("ManualPositionActivity", "LoadComponentData: " + componentMap);
	}

}
