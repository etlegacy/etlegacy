package org.etlegacy.app;

import android.util.Log;

public class ETLDownload {
	private static final String TAG = "ETLDownload";

	private static final ETLDownload me;

	private boolean initDone = false;

	static {
		me = new ETLDownload();
	}

	private ETLDownload() {
		handleSystemInit();
	}

	public void handleSystemInit() {
		if (this.initDone) {
			Log.d(TAG, "Init called when the initial inis is already done. Please check..");
			return;
		}

		try {
			Log.d(TAG, "Calling the native download init code");
			init();
			this.initDone = true;
			Log.d(TAG, "Init done");
		} catch (Exception ex) {
			Log.e(TAG, "Could not call the native method", ex);
		}
	}

	public void beginDownload(String localName, String remoteUrl, long identifier) {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	public void createWebRequest(String url, String authToken, long identifier) {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	public void abortAll() {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	public void shutdown() {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	private native void init();

	public static ETLDownload instance() {
		return me;
	}
}
