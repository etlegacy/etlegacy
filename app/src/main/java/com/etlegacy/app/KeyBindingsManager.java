package com.etlegacy.app;

import android.annotation.SuppressLint;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import org.libsdl.app.SDLActivity;

import java.util.HashMap;
import java.util.Map;

public class KeyBindingsManager {
	private final Map<String, Integer> keyBindings = new HashMap<>();

	public KeyBindingsManager() {
		// Default key bindings
		keyBindings.put("console", KeyEvent.KEYCODE_GRAVE);
		keyBindings.put("escape", KeyEvent.KEYCODE_ESCAPE);
		keyBindings.put("reload", KeyEvent.KEYCODE_R);
		keyBindings.put("jump", KeyEvent.KEYCODE_SPACE);
		keyBindings.put("activate", KeyEvent.KEYCODE_F);
		keyBindings.put("alt", KeyEvent.KEYCODE_B);
		keyBindings.put("crouch", KeyEvent.KEYCODE_C);
	}

	public void setKeyBinding(String action, int keyCode) {
		keyBindings.put(action, keyCode);
	}

	public int getKeyBinding(String action) {
		return keyBindings.getOrDefault(action, -1);
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
