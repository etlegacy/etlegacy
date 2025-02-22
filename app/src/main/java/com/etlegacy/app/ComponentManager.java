package com.etlegacy.app;

public class ComponentManager {

	// Define the ComponentData class
	public static class ComponentData {
		int width;
		int height;
		int gravity;
		int[] margins;
		int resourceId;

		public ComponentData(int width, int height, int gravity, int[] margins, int resourceId) {
			this.width = width;
			this.height = height;
			this.gravity = gravity;
			this.margins = margins;
			this.resourceId = resourceId;
		}
	}
}
