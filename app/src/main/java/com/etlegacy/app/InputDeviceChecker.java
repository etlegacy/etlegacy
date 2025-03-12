package com.etlegacy.app;

import android.util.Log;
import android.view.InputDevice;

public class InputDeviceChecker {

	public static boolean hasUSBMouseOrKeyboardConnected() {
		int[] deviceIds = InputDevice.getDeviceIds();
		for (int id : deviceIds) {
			InputDevice device = InputDevice.getDevice(id);
			if (device != null) {
				int sources = device.getSources();

				// Check if it's a physical device (not virtual) and has mouse/keyboard sources
				boolean isMouse = (sources & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE;
				boolean isKeyboard = (sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD;

				// Exclude virtual devices and touchscreens
				if ((isMouse || isKeyboard) &&
					device.getId() > 0 &&  // Virtual devices often have ID 0
					!device.isVirtual() && // Exclude virtual keyboards
					(sources & InputDevice.SOURCE_TOUCHSCREEN) != InputDevice.SOURCE_TOUCHSCREEN) {

					// Additional check for USB connection (not foolproof but helps)
					String deviceName = device.getName().toLowerCase();
					if (deviceName.contains("usb") ||
						deviceName.contains("mouse") ||
						deviceName.contains("keyboard")) {
						return true;
					}
				}
			}
		}
		return false;
	}
}
