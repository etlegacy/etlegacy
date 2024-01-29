package org.etlegacy.app.web;

import android.util.Log;

import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.FileAsyncHttpResponseHandler;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import cz.msebera.android.httpclient.Header;

public class ETLDownload {
	private static final String TAG = "ETLDownload";

	private static final ETLDownload me;

	private boolean initDone = false;
	final AsyncHttpClient client = new AsyncHttpClient();

	static {
		me = new ETLDownload();
	}

	private ETLDownload() {
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

	/**
	 * @noinspection unused
	 */
	public void beginDownload(Request request) {
		if (!(request instanceof FileDownload)) {
			throw new RuntimeException("Not yet implemented");
		}

		final FileDownload fd = (FileDownload) request;
		final Path tempFile;
		try {
			tempFile = Files.createTempFile("temp_", "_download");
		} catch (IOException e) {
			Log.e(TAG, "Could not create a temporary file for download", e);
			return;
		}

		client.get(fd.url, new FileAsyncHttpResponseHandler(tempFile.toFile()) {

			@Override
			public void onSuccess(int statusCode, Header[] headers, File file) {
				Log.d(TAG, "Download complete for: " + fd.url);
				try {
					Files.move(tempFile, Paths.get(fd.downloadPath));
					request.setDone(true);
					request.setSuccessful(true);
				} catch (IOException e) {
					Log.e(TAG, "File move failed", e);
				}
				requestComplete(statusCode, null, fd.nativeIdentifier);
			}

			@Override
			public void onProgress(long bytesWritten, long totalSize) {
				super.onProgress(bytesWritten, totalSize);
				requestProgress(bytesWritten, totalSize, fd.nativeIdentifier);
			}

			@Override
			public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {
				Log.d(TAG, "Download failure for: " + fd.url);
				if (Files.exists(tempFile)) {
					try {
						Files.delete(tempFile);
					} catch (IOException e) {
						Log.e(TAG, "Could not remove temporary file", e);
					}
				}
				request.setDone(true);
				request.setSuccessful(false);
				requestComplete(statusCode, null, fd.nativeIdentifier);
			}
		});
	}

	/**
	 * @noinspection unused
	 */
	public void createWebRequest(Request request) {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	/**
	 * @noinspection unused
	 */
	public void abortAll() {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	public void shutdown() {
		// FIXME: implement
		throw new RuntimeException("Not yet implemented");
	}

	private native void init();

	private native int requestProgress(long current, long total, long id);

	private native void requestComplete(int httpCode, byte[] data, long id);

	public static ETLDownload instance() {
		return me;
	}
}
