package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import org.libsdl.app.SDLActivity;

import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;

public class KeyBindingsManager {
	private static final Map<String, Integer> keyBindings = new HashMap<>();
	private static final String PREFS_NAME = "KeyBindingsPrefs";
	private static final String KEY_BINDINGS_MAP_KEY = "keyBindingsMap";

	private final SharedPreferences sharedPreferences;
	private final Gson gson;

	public KeyBindingsManager(Context context) {
		sharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
		gson = new Gson();

		loadKeyBindings();

		setDefaultKeyBindings();
	}

	private void setDefaultKeyBindings() {
		if (keyBindings.isEmpty()) {
			keyBindings.put("etl_console", KeyEvent.KEYCODE_GRAVE);
			keyBindings.put("esc_btn", KeyEvent.KEYCODE_ESCAPE);
			keyBindings.put("reloadBtn", KeyEvent.KEYCODE_R);
			keyBindings.put("jumpBtn", KeyEvent.KEYCODE_SPACE);
			keyBindings.put("activateBtn", KeyEvent.KEYCODE_F);
			keyBindings.put("altBtn", KeyEvent.KEYCODE_B);
			keyBindings.put("crouchBtn", KeyEvent.KEYCODE_C);
			saveKeyBindings();
		}
	}

	public void setKeyBinding(String action, int keyCode) {
		keyBindings.put(action, keyCode);
		saveKeyBindings();
	}

	public static int getKeyBinding(String action) {
		Integer keyCode = keyBindings.get(action);
		return (keyCode != null) ? keyCode : -1;
	}

	private void saveKeyBindings() {
		String json = gson.toJson(keyBindings);
		sharedPreferences.edit().putString(KEY_BINDINGS_MAP_KEY, json).apply();
	}

	private void loadKeyBindings() {
		String json = sharedPreferences.getString(KEY_BINDINGS_MAP_KEY, null);
		if (json != null) {
			Type type = new TypeToken<HashMap<String, Integer>>() {}.getType();
			Map<String, Integer> loadedBindings = gson.fromJson(json, type);
			if (loadedBindings != null) {
				keyBindings.putAll(loadedBindings);
			}
		}
	}

	public void bindClickListener(View view, String action) {
		view.setOnClickListener(v -> {
			int keyCode = getKeyBinding(action);
			if (keyCode != -1) {
				SDLActivity.onNativeKeyDown(keyCode);
				SDLActivity.onNativeKeyUp(keyCode);
			}
		});
	}

	@SuppressLint("ClickableViewAccessibility")
	public void bindTouchListener(View view, String action) {
		view.setOnTouchListener((v, event) -> {
			int keyCode = getKeyBinding(action);
			if (keyCode != -1) {
				switch (event.getAction()) {
					case MotionEvent.ACTION_DOWN:
						SDLActivity.onNativeKeyDown(keyCode);
						break;
					case MotionEvent.ACTION_UP:
					case MotionEvent.ACTION_CANCEL:
						SDLActivity.onNativeKeyUp(keyCode);
						break;
				}
			}
			return false;
		});
	}

}
