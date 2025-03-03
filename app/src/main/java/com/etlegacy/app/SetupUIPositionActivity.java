package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
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
	private boolean hasMoved;

	private FrameLayout overlayContainer;
	private View scaleOverlay;
	private LinearLayout controlsLayout;
	private ComponentManager.ComponentData currentComponentData;
	private String currentTag;

	private SeekBar widthSeekBar;
	private SeekBar heightSeekBar;

	private static final float MIN_SCALE = 0.5f;
	private static final float MAX_SCALE = 2.0f;
	private static final int NORMAL_BACKGROUND = R.drawable.border_drawable;
	private static final float MOVEMENT_THRESHOLD = 10.0f;
	private static final int OVERLAY_OFFSET = 40;

	private void hideSystemUI() {
		getWindow().getDecorView().setSystemUiVisibility(
			View.SYSTEM_UI_FLAG_IMMERSIVE
				| View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
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

		overlayContainer = findViewById(R.id.rootLayout);
		setupScaleOverlay();

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
			overlayContainer.addView(view);
		}
	}

	@SuppressLint("SetTextI18n")
	private void setupScaleOverlay() {
		scaleOverlay = new View(this);
		scaleOverlay.setBackgroundColor(Color.argb(0, 0, 0, 0));
		scaleOverlay.setVisibility(View.GONE);
		scaleOverlay.setClickable(true); // Make overlay intercept clicks

		FrameLayout.LayoutParams overlayParams = new FrameLayout.LayoutParams(
			FrameLayout.LayoutParams.MATCH_PARENT,
			FrameLayout.LayoutParams.MATCH_PARENT
		);
		scaleOverlay.setLayoutParams(overlayParams);

		controlsLayout = new LinearLayout(this);
		controlsLayout.setOrientation(LinearLayout.VERTICAL);
		controlsLayout.setGravity(Gravity.CENTER);
		controlsLayout.setBackgroundColor(Color.WHITE);
		controlsLayout.setPadding(20, 20, 20, 20);
		controlsLayout.setVisibility(View.GONE);
		controlsLayout.setElevation(10f); // Raise above other views

		widthSeekBar = new SeekBar(this);
		widthSeekBar.setMax(100);
		widthSeekBar.setId(View.generateViewId());

		heightSeekBar = new SeekBar(this);
		heightSeekBar.setMax(100);
		heightSeekBar.setId(View.generateViewId());

		Button confirmButton = new Button(this);
		confirmButton.setText("Confirm");

		Button cancelButton = new Button(this);
		cancelButton.setText("Cancel");

		controlsLayout.addView(createLabeledSeekBar("Width", widthSeekBar));
		controlsLayout.addView(createLabeledSeekBar("Height", heightSeekBar));
		controlsLayout.addView(confirmButton);
		controlsLayout.addView(cancelButton);

		FrameLayout.LayoutParams controlsParams = new FrameLayout.LayoutParams(
			FrameLayout.LayoutParams.WRAP_CONTENT,
			FrameLayout.LayoutParams.WRAP_CONTENT
		);

		// Add overlay and controls last to ensure they're on top
		overlayContainer.addView(scaleOverlay);
		overlayContainer.addView(controlsLayout, controlsParams);

		widthSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				if (fromUser && currentComponentData != null) {
					float scale = MIN_SCALE + (progress / 100f) * (MAX_SCALE - MIN_SCALE);
					updatePreviewWidth(scale);
				}
			}
			@Override public void onStartTrackingTouch(SeekBar seekBar) {}
			@Override public void onStopTrackingTouch(SeekBar seekBar) {}
		});

		heightSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				if (fromUser && currentComponentData != null) {
					float scale = MIN_SCALE + (progress / 100f) * (MAX_SCALE - MIN_SCALE);
					updatePreviewHeight(scale);
				}
			}
			@Override public void onStartTrackingTouch(SeekBar seekBar) {}
			@Override public void onStopTrackingTouch(SeekBar seekBar) {}
		});

		confirmButton.setOnClickListener(v -> {
			saveScaledDimensions();
			hideScaleOverlay();
			selectedView = null;
		});

		cancelButton.setOnClickListener(v -> {
			if (currentTag != null && currentComponentData != null) {
				View view = overlayContainer.findViewWithTag(currentTag);
				if (view != null) {
					FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
					params.width = currentComponentData.width;
					params.height = currentComponentData.height;
					view.setLayoutParams(params);
				}
			}
			hideScaleOverlay();
			selectedView = null;
		});
	}

	private LinearLayout createLabeledSeekBar(String label, SeekBar seekBar) {
		LinearLayout layout = new LinearLayout(this);
		layout.setOrientation(LinearLayout.VERTICAL);

		TextView labelView = new TextView(this);
		labelView.setText(label);
		layout.addView(labelView);
		layout.addView(seekBar);

		return layout;
	}

	private void showScaleOverlay(View view) {
		currentTag = (String) view.getTag();
		currentComponentData = componentMap.get(currentTag);

		if (currentComponentData != null) {
			scaleOverlay.setVisibility(View.VISIBLE);
			controlsLayout.setVisibility(View.VISIBLE);

			View currentView = overlayContainer.findViewWithTag(currentTag);
			FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) currentView.getLayoutParams();

			float currentWidthScale = (float) params.width / currentComponentData.width;
			float currentHeightScale = (float) params.height / currentComponentData.height;

			int widthProgress = (int) (((currentWidthScale - MIN_SCALE) / (MAX_SCALE - MIN_SCALE)) * 100);
			int heightProgress = (int) (((currentHeightScale - MIN_SCALE) / (MAX_SCALE - MIN_SCALE)) * 100);

			widthSeekBar.setProgress(Math.max(0, Math.min(100, widthProgress)));
			heightSeekBar.setProgress(Math.max(0, Math.min(100, heightProgress)));

			FrameLayout.LayoutParams overlayParams = (FrameLayout.LayoutParams) controlsLayout.getLayoutParams();
			int[] location = new int[2];
			currentView.getLocationOnScreen(location);
			int viewX = location[0];
			int viewY = location[1];
			int viewHeight = currentView.getHeight();

			overlayParams.leftMargin = viewX;
			overlayParams.topMargin = viewY + viewHeight + OVERLAY_OFFSET;

			int screenWidth = getResources().getDisplayMetrics().widthPixels;
			int screenHeight = getResources().getDisplayMetrics().heightPixels;

			controlsLayout.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);
			int overlayWidth = controlsLayout.getMeasuredWidth();
			int overlayHeight = controlsLayout.getMeasuredHeight();

			if (overlayParams.leftMargin + overlayWidth > screenWidth) {
				overlayParams.leftMargin = screenWidth - overlayWidth;
			}
			if (overlayParams.topMargin + overlayHeight > screenHeight) {
				overlayParams.topMargin = viewY - overlayHeight - OVERLAY_OFFSET;
				if (overlayParams.topMargin < 0) {
					overlayParams.topMargin = 0;
				}
			}
			if (overlayParams.leftMargin < 0) {
				overlayParams.leftMargin = 0;
			}

			controlsLayout.setLayoutParams(overlayParams);
			// Bring overlay to front
			scaleOverlay.bringToFront();
			controlsLayout.bringToFront();
		}
	}

	private void hideScaleOverlay() {
		scaleOverlay.setVisibility(View.GONE);
		controlsLayout.setVisibility(View.GONE);
		currentComponentData = null;
		currentTag = null;
	}

	private void updatePreviewWidth(float scale) {
		View view = overlayContainer.findViewWithTag(currentTag);
		if (view != null && currentComponentData != null) {
			FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
			params.width = (int) (currentComponentData.width * scale);
			view.setLayoutParams(params);
			showScaleOverlay(view);
		}
	}

	private void updatePreviewHeight(float scale) {
		View view = overlayContainer.findViewWithTag(currentTag);
		if (view != null && currentComponentData != null) {
			FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
			params.height = (int) (currentComponentData.height * scale);
			view.setLayoutParams(params);
			showScaleOverlay(view);
		}
	}

	private void saveScaledDimensions() {
		View view = overlayContainer.findViewWithTag(currentTag);
		if (view != null && currentComponentData != null) {
			FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
			currentComponentData.width = params.width;
			currentComponentData.height = params.height;
			componentMap.put(currentTag, currentComponentData);
			SaveComponentData();
		}
	}

	private View createMovableView(String tag) {
		View view = new View(this);
		view.setBackgroundResource(NORMAL_BACKGROUND);
		view.setTag(tag);
		view.setOnTouchListener(new View.OnTouchListener() {
			@SuppressLint("ClickableViewAccessibility")
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				if (scaleOverlay.getVisibility() == View.VISIBLE) {
					return true;
				}

				switch (event.getAction()) {
					case MotionEvent.ACTION_DOWN:
						selectedView = v;
						initialTouchX = event.getRawX();
						initialTouchY = event.getRawY();
						viewInitialX = v.getX();
						viewInitialY = v.getY();
						hasMoved = false;
						return true;

					case MotionEvent.ACTION_MOVE:
						if (selectedView != null) {
							float deltaX = event.getRawX() - initialTouchX;
							float deltaY = event.getRawY() - initialTouchY;

							if (Math.abs(deltaX) > MOVEMENT_THRESHOLD || Math.abs(deltaY) > MOVEMENT_THRESHOLD) {
								hasMoved = true;
							}

							if (hasMoved) {
								selectedView.setX(viewInitialX + deltaX);
								selectedView.setY(viewInitialY + deltaY);
							}
						}
						return true;

					case MotionEvent.ACTION_UP:
						if (selectedView != null) {
							if (hasMoved) {
								SaveComponentPosition(selectedView);
								selectedView = null;
							} else {
								showScaleOverlay(selectedView);
							}
						}
						return true;

					default:
						return false;
				}
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
		margins[2] = 0;
		margins[3] = 0;

		ComponentManager.ComponentData updatedData = componentMap.get(tag);
		assert updatedData != null;
		updatedData.margins[0] = margins[0];
		updatedData.margins[1] = margins[1];

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
		if (componentMap == null) {
			componentMap = new HashMap<>();
		}
		Log.v("ManualPositionActivity", "LoadComponentData: " + componentMap);
	}
}
