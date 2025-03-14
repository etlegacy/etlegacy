package com.etlegacy.app;

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

	public static boolean hasBluetoothMouseOrKeyboardConnected() {
		int[] deviceIds = InputDevice.getDeviceIds();
		for (int id : deviceIds) {
			InputDevice device = InputDevice.getDevice(id);
			if (device != null) {
				int sources = device.getSources();

				// Check for mouse or keyboard sources
				boolean isMouse = (sources & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE;
				boolean isKeyboard = (sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD;

				if ((isMouse || isKeyboard) &&
					device.getId() > 0 &&          // Exclude virtual devices
					!device.isVirtual() &&         // Exclude virtual keyboards
					(sources & InputDevice.SOURCE_TOUCHSCREEN) != InputDevice.SOURCE_TOUCHSCREEN) { // Exclude touchscreens

					String deviceName = device.getName().toLowerCase();
					// Stricter Bluetooth detection
					if ((deviceName.contains("bluetooth") || deviceName.contains("bt")) &&
						// Exclude common Android internal input devices
						!deviceName.contains("touch") &&
						!deviceName.contains("virtual") &&
						!deviceName.contains("input") && // Generic Android input
						!deviceName.contains("synaptics") && // Common touchpad
						!deviceName.contains("usb") &&   // Exclude USB devices
						!deviceName.contains("hid")) {   // Generic HID interface
						return true;
					}
				}
			}
		}
		return false;
	}
}
