package com.etlegacy.app.keyboard;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Color;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

import com.etlegacy.app.R;

import org.libsdl.app.SDLActivity;

public class CustomKeyboard {
	private final Context context;
	private final Handler repeatHandler;
	private static final int REPEAT_DELAY = 100; // Repeat every 100ms while held

	public CustomKeyboard(Context context, Handler repeatHandler) {
		this.context = context;
		this.repeatHandler = repeatHandler;
	}

	public RelativeLayout createKeyboardLayout() {
		RelativeLayout layout = new RelativeLayout(context);
		DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
		int maxHeight = (int) (displayMetrics.heightPixels * 0.6);

		RelativeLayout.LayoutParams layoutParams = new RelativeLayout.LayoutParams(
			ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		layoutParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
		layoutParams.height = Math.min(maxHeight, ViewGroup.LayoutParams.WRAP_CONTENT);
		layout.setLayoutParams(layoutParams);

		String[][] keyRows = {
			{"l-ctrl", "l-alt", "space", "r-alt", "r-ctrl"},
			{"l-shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "r-shift"},
			{"capslock", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "enter"},
			{"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]"},
			{"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\\", "backspace"},
			{"esc", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "Pgn-Up", "Pgn-Dn"}
		};

		int previousRowId = View.NO_ID;
		for (int i = 0; i < keyRows.length; i++) {
			LinearLayout rowLayout = new LinearLayout(context);
			rowLayout.setOrientation(LinearLayout.HORIZONTAL);
			rowLayout.setId(View.generateViewId());

			RelativeLayout.LayoutParams rowParams = new RelativeLayout.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			if (i == 0) {
				rowParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
			} else {
				rowParams.addRule(RelativeLayout.ABOVE, previousRowId);
			}
			rowLayout.setLayoutParams(rowParams);

			for (String key : keyRows[i]) {
				Button keyButton = new Button(context);
				keyButton.setText(key);
				keyButton.setBackgroundResource(R.drawable.popup_background);
				keyButton.setTextColor(Color.WHITE);
				LinearLayout.LayoutParams buttonParams = new LinearLayout.LayoutParams(
					0, dpToPx(32), 1.0f);
				keyButton.setLayoutParams(buttonParams);
				keyButton.setPadding(0, 0, 0, 0);
				keyButton.setTextSize("backspace".equals(key) ? 10 : 12);
				setupKeyTouchListener(keyButton, key);
				rowLayout.addView(keyButton);
			}

			layout.addView(rowLayout);
			previousRowId = rowLayout.getId();
		}

		return layout;
	}

	private int dpToPx(int dp) {
		float density = context.getResources().getDisplayMetrics().density;
		return Math.round(dp * density);
	}

	@SuppressLint("ClickableViewAccessibility")
	private void setupKeyTouchListener(Button button, String key) {
		Runnable repeatRunnable = new Runnable() {
			@Override
			public void run() {
				handleKeyPress(key, true);
				repeatHandler.postDelayed(this, REPEAT_DELAY);
			}
		};

		button.setOnTouchListener((v, event) -> {
			switch (event.getAction()) {
				case MotionEvent.ACTION_DOWN:
					handleKeyPress(key, true);
					repeatHandler.postDelayed(repeatRunnable, REPEAT_DELAY);
					return true;
				case MotionEvent.ACTION_UP:
				case MotionEvent.ACTION_CANCEL:
					repeatHandler.removeCallbacks(repeatRunnable);
					handleKeyPress(key, false);
					return true;
			}
			return false;
		});
	}

	private void handleKeyPress(String key, boolean isDown) {
		int keyCode;
		switch (key.toLowerCase()) {
			case "`": keyCode = KeyEvent.KEYCODE_GRAVE; break;
			case "space": keyCode = KeyEvent.KEYCODE_SPACE; break;
			case "enter": keyCode = KeyEvent.KEYCODE_ENTER; break;
			case ",": keyCode = KeyEvent.KEYCODE_COMMA; break;
			case ".": keyCode = KeyEvent.KEYCODE_PERIOD; break;
			case "/": keyCode = KeyEvent.KEYCODE_SLASH; break;
			case ";": keyCode = KeyEvent.KEYCODE_SEMICOLON; break;
			case "'": keyCode = KeyEvent.KEYCODE_APOSTROPHE; break;
			case "\\": keyCode = KeyEvent.KEYCODE_BACKSLASH; break;
			case "[": keyCode = KeyEvent.KEYCODE_LEFT_BRACKET; break;
			case "]": keyCode = KeyEvent.KEYCODE_RIGHT_BRACKET; break;
			case "-": keyCode = KeyEvent.KEYCODE_MINUS; break;
			case "=": keyCode = KeyEvent.KEYCODE_EQUALS; break;
			case "l-shift": keyCode = KeyEvent.KEYCODE_SHIFT_LEFT; break;
			case "r-shift": keyCode = KeyEvent.KEYCODE_SHIFT_RIGHT; break;
			case "l-ctrl": keyCode = KeyEvent.KEYCODE_CTRL_LEFT; break;
			case "r-ctrl": keyCode = KeyEvent.KEYCODE_CTRL_RIGHT; break;
			case "l-alt": keyCode = KeyEvent.KEYCODE_ALT_LEFT; break;
			case "r-alt": keyCode = KeyEvent.KEYCODE_ALT_RIGHT; break;
			case "tab": keyCode = KeyEvent.KEYCODE_TAB; break;
			case "capslock": keyCode = KeyEvent.KEYCODE_CAPS_LOCK; break;
			case "backspace": keyCode = KeyEvent.KEYCODE_DEL; break;
			case "esc": keyCode = KeyEvent.KEYCODE_ESCAPE; break;
			case "pgn-up": keyCode = KeyEvent.KEYCODE_PAGE_UP; break;
			case "pgn-dn": keyCode = KeyEvent.KEYCODE_PAGE_DOWN; break;
			case "f1": keyCode = KeyEvent.KEYCODE_F1; break;
			case "f2": keyCode = KeyEvent.KEYCODE_F2; break;
			case "f3": keyCode = KeyEvent.KEYCODE_F3; break;
			case "f4": keyCode = KeyEvent.KEYCODE_F4; break;
			case "f5": keyCode = KeyEvent.KEYCODE_F5; break;
			case "f6": keyCode = KeyEvent.KEYCODE_F6; break;
			case "f7": keyCode = KeyEvent.KEYCODE_F7; break;
			case "f8": keyCode = KeyEvent.KEYCODE_F8; break;
			case "f9": keyCode = KeyEvent.KEYCODE_F9; break;
			case "f10": keyCode = KeyEvent.KEYCODE_F10; break;
			case "f11": keyCode = KeyEvent.KEYCODE_F11; break;
			case "f12": keyCode = KeyEvent.KEYCODE_F12; break;
			default:
				char c = key.charAt(0);
				keyCode = keyCharToKeyCode(c);
				if (keyCode == -1) return;
				break;
		}
		if (isDown) {
			SDLActivity.onNativeKeyDown(keyCode);
		} else {
			SDLActivity.onNativeKeyUp(keyCode);
		}
	}

	private int keyCharToKeyCode(char c) {
		switch (Character.toLowerCase(c)) {
			case 'a': return android.view.KeyEvent.KEYCODE_A;
			case 'b': return android.view.KeyEvent.KEYCODE_B;
			case 'c': return android.view.KeyEvent.KEYCODE_C;
			case 'd': return android.view.KeyEvent.KEYCODE_D;
			case 'e': return android.view.KeyEvent.KEYCODE_E;
			case 'f': return android.view.KeyEvent.KEYCODE_F;
			case 'g': return android.view.KeyEvent.KEYCODE_G;
			case 'h': return android.view.KeyEvent.KEYCODE_H;
			case 'i': return android.view.KeyEvent.KEYCODE_I;
			case 'j': return android.view.KeyEvent.KEYCODE_J;
			case 'k': return android.view.KeyEvent.KEYCODE_K;
			case 'l': return android.view.KeyEvent.KEYCODE_L;
			case 'm': return android.view.KeyEvent.KEYCODE_M;
			case 'n': return android.view.KeyEvent.KEYCODE_N;
			case 'o': return android.view.KeyEvent.KEYCODE_O;
			case 'p': return android.view.KeyEvent.KEYCODE_P;
			case 'q': return android.view.KeyEvent.KEYCODE_Q;
			case 'r': return android.view.KeyEvent.KEYCODE_R;
			case 's': return android.view.KeyEvent.KEYCODE_S;
			case 't': return android.view.KeyEvent.KEYCODE_T;
			case 'u': return android.view.KeyEvent.KEYCODE_U;
			case 'v': return android.view.KeyEvent.KEYCODE_V;
			case 'w': return android.view.KeyEvent.KEYCODE_W;
			case 'x': return android.view.KeyEvent.KEYCODE_X;
			case 'y': return android.view.KeyEvent.KEYCODE_Y;
			case 'z': return android.view.KeyEvent.KEYCODE_Z;
			case '0': return android.view.KeyEvent.KEYCODE_0;
			case '1': return android.view.KeyEvent.KEYCODE_1;
			case '2': return android.view.KeyEvent.KEYCODE_2;
			case '3': return android.view.KeyEvent.KEYCODE_3;
			case '4': return android.view.KeyEvent.KEYCODE_4;
			case '5': return android.view.KeyEvent.KEYCODE_5;
			case '6': return android.view.KeyEvent.KEYCODE_6;
			case '7': return android.view.KeyEvent.KEYCODE_7;
			case '8': return android.view.KeyEvent.KEYCODE_8;
			case '9': return android.view.KeyEvent.KEYCODE_9;
			default: return -1;
		}
	}
}
