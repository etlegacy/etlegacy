package com.etlegacy.app;


import android.content.SharedPreferences;

import android.content.Context;
import com.google.gson.Gson;

public class ComponentManager {
	private static final String PREFS_NAME = "ComponentPrefs";
	private static final String COMPONENT_MAP_KEY = "componentMap";
	private SharedPreferences sharedPreferences;
	private Gson gson;

	// Define the ComponentData class
	static class ComponentData {
		int width;
		int height;
		int gravity;
		int[] margins;

		public ComponentData(int width, int height, int gravity, int[] margins) {
			this.width = width;
			this.height = height;
			this.gravity = gravity;
			this.margins = margins;
		}
	}

	public ComponentManager(Context context) {
		sharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
		gson = new Gson();
	}

}
