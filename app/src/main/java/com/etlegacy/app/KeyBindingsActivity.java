package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.lang.reflect.Type;
import java.util.HashMap;

public class KeyBindingsActivity extends AppCompatActivity {
	private HashMap<String, ComponentManager.ComponentData> defaultcomponentMap;
	private KeyBindingsManager keyBindingsManager;

	private static final String PREFS_NAME = "ComponentPrefs";
	private static final String COMPONENT_MAP_KEY = "componentMap";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		hideSystemUI();

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		}

		keyBindingsManager = new KeyBindingsManager(getApplicationContext());
		loadComponentData();
		setupUI();
	}

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

	private void loadComponentData() {
		SharedPreferences sharedPreferences = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
		Gson gson = new Gson();
		String json = sharedPreferences.getString(COMPONENT_MAP_KEY, null);
		Log.v("KeyBindingsActivity", "Loaded JSON: " + json);
		Type type = new TypeToken<HashMap<String, ComponentManager.ComponentData>>() {}.getType();
		defaultcomponentMap = gson.fromJson(json, type);
		assert defaultcomponentMap != null;
	}

	@SuppressLint("SetTextI18n")
	private void setupUI() {
		// Root layout to center everything
		LinearLayout rootLayout = new LinearLayout(this);
		rootLayout.setLayoutParams(new LinearLayout.LayoutParams(
			LinearLayout.LayoutParams.MATCH_PARENT,
			LinearLayout.LayoutParams.MATCH_PARENT
		));
		rootLayout.setGravity(Gravity.CENTER);
		rootLayout.setOrientation(LinearLayout.VERTICAL);

		// ScrollView to prevent clipping on small screens
		ScrollView scrollView = new ScrollView(this);
		scrollView.setLayoutParams(new LinearLayout.LayoutParams(
			LinearLayout.LayoutParams.MATCH_PARENT,
			LinearLayout.LayoutParams.MATCH_PARENT
		));

		// FrameLayout to center the table horizontally inside ScrollView
		FrameLayout frameLayout = new FrameLayout(this);
		frameLayout.setLayoutParams(new FrameLayout.LayoutParams(
			FrameLayout.LayoutParams.MATCH_PARENT,
			FrameLayout.LayoutParams.WRAP_CONTENT
		));
		frameLayout.setForegroundGravity(Gravity.CENTER); // Center contents

		// Container to limit table width and center it
		LinearLayout tableContainer = new LinearLayout(this);
		FrameLayout.LayoutParams tableContainerParams = new FrameLayout.LayoutParams(
			600, // Fixed width
			FrameLayout.LayoutParams.WRAP_CONTENT,
			Gravity.CENTER // Center inside FrameLayout
		);
		tableContainer.setLayoutParams(tableContainerParams);
		tableContainer.setGravity(Gravity.CENTER);
		tableContainer.setOrientation(LinearLayout.VERTICAL);

		TableLayout tableLayout = new TableLayout(this);
		tableLayout.setLayoutParams(new TableLayout.LayoutParams(
			TableLayout.LayoutParams.MATCH_PARENT,
			TableLayout.LayoutParams.WRAP_CONTENT
		));

		String[] keys = {"etl_console", "esc_btn", "reloadBtn", "jumpBtn", "activateBtn", "altBtn", "crouchBtn"};

		for (int i = 0; i < keys.length; i++) {
			String key = keys[i];
			ComponentManager.ComponentData data = defaultcomponentMap.get(key);
			if (data == null) continue;

			TableRow row = new TableRow(this);
			TableRow.LayoutParams rowParams = new TableRow.LayoutParams(
				TableRow.LayoutParams.MATCH_PARENT,
				TableRow.LayoutParams.WRAP_CONTENT
			);
			row.setLayoutParams(rowParams);
			row.setGravity(Gravity.CENTER_VERTICAL);

			row.setBackground(ContextCompat.getDrawable(this, R.drawable.popup_background));
			row.setPadding(16, 16, 16, 16);

			ImageView icon = new ImageView(this);
			icon.setImageResource(data.resourceId);
			TableRow.LayoutParams iconParams = new TableRow.LayoutParams(130, 130);
			iconParams.setMargins(16, 0, 16, 0);
			icon.setLayoutParams(iconParams);

			TextView textView = new TextView(this);
			textView.setText(key + "\n" + KeyEvent.keyCodeToString(KeyBindingsManager.getKeyBinding(key)));
			textView.setTextSize(18);
			textView.setPadding(16, 0, 16, 0);

			TableRow.LayoutParams textParams = new TableRow.LayoutParams(0, TableRow.LayoutParams.WRAP_CONTENT, 1);
			textView.setLayoutParams(textParams);

			row.addView(icon);
			row.addView(textView);
			row.setOnClickListener(v -> changeKeyBinding(key));

			tableLayout.addView(row);

			// Add separator, but skip adding it after the last row
			if (i < keys.length - 1) {
				View separator = new View(this);
				separator.setLayoutParams(new TableRow.LayoutParams(
					TableRow.LayoutParams.MATCH_PARENT, 2
				));
				separator.setBackgroundColor(Color.BLACK);
				tableLayout.addView(separator);
			}
		}

		tableContainer.addView(tableLayout);
		frameLayout.addView(tableContainer);
		scrollView.addView(frameLayout);
		rootLayout.addView(scrollView);
		setContentView(rootLayout);
	}

	private void changeKeyBinding(String key) {
		// Show a dialog to select a new keybinding
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle("Change Key Binding for " + key);

		String[] keyOptions = {
			"KEYCODE_0",
			"KEYCODE_1",
			"KEYCODE_2",
			"KEYCODE_3",
			"KEYCODE_4",
			"KEYCODE_5",
			"KEYCODE_6",
			"KEYCODE_7",
			"KEYCODE_8",
			"KEYCODE_9",
			"KEYCODE_A",
			"KEYCODE_B",
			"KEYCODE_C",
			"KEYCODE_D",
			"KEYCODE_E",
			"KEYCODE_F",
			"KEYCODE_G",
			"KEYCODE_H",
			"KEYCODE_I",
			"KEYCODE_J",
			"KEYCODE_K",
			"KEYCODE_L",
			"KEYCODE_M",
			"KEYCODE_N",
			"KEYCODE_O",
			"KEYCODE_P",
			"KEYCODE_Q",
			"KEYCODE_R",
			"KEYCODE_S",
			"KEYCODE_T",
			"KEYCODE_U",
			"KEYCODE_V",
			"KEYCODE_W",
			"KEYCODE_X",
			"KEYCODE_Y",
			"KEYCODE_Z",
			"KEYCODE_ALT_LEFT",
			"KEYCODE_ALT_RIGHT",
			"KEYCODE_APOSTROPHE",
			"KEYCODE_AT",
			"KEYCODE_BACK",
			"KEYCODE_BACKSLASH",
			"KEYCODE_CALL",
			"KEYCODE_CAMERA",
			"KEYCODE_CLEAR",
			"KEYCODE_COMMA",
			"KEYCODE_CTRL_LEFT",
			"KEYCODE_CTRL_RIGHT",
			"KEYCODE_DEL",
			"KEYCODE_DPAD_CENTER",
			"KEYCODE_DPAD_DOWN",
			"KEYCODE_DPAD_LEFT",
			"KEYCODE_DPAD_RIGHT",
			"KEYCODE_DPAD_UP",
			"KEYCODE_ENDCALL",
			"KEYCODE_ENTER",
			"KEYCODE_ENVELOPE",
			"KEYCODE_EQUALS",
			"KEYCODE_ESCAPE",
			"KEYCODE_EXPLORER",
			"KEYCODE_F1",
			"KEYCODE_F10",
			"KEYCODE_F11",
			"KEYCODE_F12",
			"KEYCODE_F2",
			"KEYCODE_F3",
			"KEYCODE_F4",
			"KEYCODE_F5",
			"KEYCODE_F6",
			"KEYCODE_F7",
			"KEYCODE_F8",
			"KEYCODE_F9",
			"KEYCODE_FOCUS",
			"KEYCODE_FORWARD_DEL",
			"KEYCODE_GRAVE",
			"KEYCODE_HEADSETHOOK",
			"KEYCODE_HOME",
			"KEYCODE_LEFT_BRACKET",
			"KEYCODE_MEDIA_FAST_FORWARD",
			"KEYCODE_MEDIA_NEXT",
			"KEYCODE_MEDIA_PAUSE",
			"KEYCODE_MEDIA_PLAY",
			"KEYCODE_MEDIA_PLAY_PAUSE",
			"KEYCODE_MEDIA_PREVIOUS",
			"KEYCODE_MEDIA_REWIND",
			"KEYCODE_MEDIA_STOP",
			"KEYCODE_MENU",
			"KEYCODE_MINUS",
			"KEYCODE_MUTE",
			"KEYCODE_NOTIFICATION",
			"KEYCODE_NUM",
			"KEYCODE_NUMPAD_0",
			"KEYCODE_NUMPAD_1",
			"KEYCODE_NUMPAD_2",
			"KEYCODE_NUMPAD_3",
			"KEYCODE_NUMPAD_4",
			"KEYCODE_NUMPAD_5",
			"KEYCODE_NUMPAD_6",
			"KEYCODE_NUMPAD_7",
			"KEYCODE_NUMPAD_8",
			"KEYCODE_NUMPAD_9",
			"KEYCODE_NUMPAD_ADD",
			"KEYCODE_NUMPAD_COMMA",
			"KEYCODE_NUMPAD_DIVIDE",
			"KEYCODE_NUMPAD_DOT",
			"KEYCODE_NUMPAD_ENTER",
			"KEYCODE_NUMPAD_EQUALS",
			"KEYCODE_NUMPAD_LEFT_PAREN",
			"KEYCODE_NUMPAD_MULTIPLY",
			"KEYCODE_NUMPAD_RIGHT_PAREN",
			"KEYCODE_NUMPAD_SUBTRACT",
			"KEYCODE_PAGE_DOWN",
			"KEYCODE_PAGE_UP",
			"KEYCODE_PERIOD",
			"KEYCODE_PICTSYMBOLS",
			"KEYCODE_PLUS",
			"KEYCODE_POUND",
			"KEYCODE_POWER",
			"KEYCODE_RIGHT_BRACKET",
			"KEYCODE_SEARCH",
			"KEYCODE_SEMICOLON",
			"KEYCODE_SHIFT_LEFT",
			"KEYCODE_SHIFT_RIGHT",
			"KEYCODE_SLASH",
			"KEYCODE_SOFT_LEFT",
			"KEYCODE_SOFT_RIGHT",
			"KEYCODE_SPACE",
			"KEYCODE_STAR",
			"KEYCODE_SYM",
			"KEYCODE_TAB",
			"KEYCODE_VOLUME_DOWN",
			"KEYCODE_VOLUME_UP",
			"KEYCODE_WINDOW",
			"KEYCODE_ZOOM_IN",
			"KEYCODE_ZOOM_OUT"
		};
		builder.setItems(keyOptions, (dialog, which) -> {
			String selectedKeyName = keyOptions[which]; // Get key name (e.g., "KEYCODE_A")
			int newKeyCode = KeyEvent.keyCodeFromString(selectedKeyName); // Convert to KeyEvent code

			if (newKeyCode != KeyEvent.KEYCODE_UNKNOWN) { // Ensure it's a valid key
				keyBindingsManager.setKeyBinding(key, newKeyCode);
				setupUI(); // Refresh UI
			}
		});

		builder.setNegativeButton("Cancel", (dialog, which) -> dialog.dismiss());
		builder.show();
	}
}
