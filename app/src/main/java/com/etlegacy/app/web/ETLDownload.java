package com.etlegacy.app.web;

import static java.nio.file.StandardCopyOption.REPLACE_EXISTING;
import static java.nio.file.StandardOpenOption.WRITE;

import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

public class ETLDownload {
	private static final String TAG = "ETLDownload";

	private static final ETLDownload me;

	private boolean initDone = false;
	final ExecutorService executor = Executors.newCachedThreadPool(Executors.defaultThreadFactory());
	final Map<Request, Future<?>> requests = Collections.synchronizedMap(new HashMap<>());

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

		final Future<?> future = executor.submit(() -> {
			ThrowableCallable<HttpURLConnection> handler = (conn -> {
				InputStream response = conn.getErrorStream();
				if (response == null) {
					response = conn.getInputStream();
					request.setSuccessful(true);
				} else {
					request.setSuccessful(false);
				}

				if (request.isSuccessful()) {
					int len = conn.getContentLength();
					try (OutputStream out = Files.newOutputStream(tempFile, WRITE)) {
						InputStream in = new BufferedInputStream(response);
						copyStream(in, out, len, request);
						out.flush();
					}
					Files.move(tempFile, Paths.get(((FileDownload) request).downloadPath), REPLACE_EXISTING);
				}

				int code = conn.getResponseCode();

				requestComplete(code, null, request.nativeIdentifier);
				request.setDone(true);
			});

			try {
				createHttpConnection(request, handler);
			} catch (Exception e) {
				Log.e(TAG, "Failure", e);
				request.setSuccessful(false);
				request.setDone(true);
				requestComplete(400, null, request.nativeIdentifier);
			}
			requests.remove(request);
		});
		this.requests.put(request, future);
	}

	/**
	 * @noinspection unused
	 */
	public void createWebRequest(final Request request) {
		final Future<?> future = executor.submit(() -> {
			ThrowableCallable<HttpURLConnection> handler = (conn -> {

				if (request instanceof UploadData) {
					UploadData dd = (UploadData) request;
					ByteArrayInputStream buf = new ByteArrayInputStream(dd.buffer);
					try (OutputStream out = new BufferedOutputStream(conn.getOutputStream())) {
						copyStream(buf, out, dd.buffer.length, request);
						out.flush();
					}
				}

				ByteArrayOutputStream outBuffer = new ByteArrayOutputStream();

				InputStream response = conn.getErrorStream();
				if (response == null) {
					response = conn.getInputStream();
					request.setSuccessful(true);
				} else {
					request.setSuccessful(false);
				}

				InputStream in = new BufferedInputStream(response);
				copyStream(in, outBuffer, conn.getContentLength(), request);

				int code = conn.getResponseCode();

				requestComplete(code, outBuffer.toByteArray(), request.nativeIdentifier);
				request.setDone(true);
			});

			try {
				createHttpConnection(request, handler);
			} catch (Exception e) {
				Log.e(TAG, "Failure", e);
				request.setSuccessful(false);
				request.setDone(true);
				requestComplete(400, null, request.nativeIdentifier);
			}
			requests.remove(request);
		});
		this.requests.put(request, future);
	}

	private void createHttpConnection(final Request request, ThrowableCallable<HttpURLConnection> handler) throws Exception {
		URL url = new URL(request.url);
		HttpURLConnection connection = (HttpURLConnection) url.openConnection();
		try {
			connection.setConnectTimeout((int) TimeUnit.SECONDS.toMillis(10));
			connection.setReadTimeout((int) TimeUnit.SECONDS.toMillis(10));

			if (request instanceof UploadData) {
				connection.setRequestMethod("POST");
				connection.setDoOutput(true);
				connection.setChunkedStreamingMode(0);
			} else {
				connection.setRequestMethod("GET");
			}
			connection.setDoInput(true);

			if (request.authToken != null) {
				connection.setRequestProperty("X-ETL-KEY", request.authToken);
			}

			if (!request.headers.isEmpty()) {
				request.headers.forEach(connection::setRequestProperty);
			}

			// Well we are not libcurl but just to have it print out similarly
			connection.setRequestProperty("user-agent", "ID_DOWNLOAD/2.0 libcurl");

			handler.apply(connection);
		} finally {
			connection.disconnect();
		}
	}

	private void copyStream(InputStream in, OutputStream out, int total, Request request) throws IOException {
		byte[] buffer = new byte[8192];
		int len, current = 0;
		while ((len = in.read(buffer)) != -1) {
			current += len;
			out.write(buffer, 0, len);
			requestProgress(current, total, request.nativeIdentifier);
			out.flush();
		}
	}

	/**
	 * @noinspection unused
	 */
	public void abortAll() {
		this.requests.forEach((requests, future) -> future.cancel(true));
		this.requests.clear();
	}

	public void shutdown() {
		abortAll();
	}

	public void shutdownExecutor() {
		this.executor.shutdown();
		try {
			if (!this.executor.awaitTermination(5, TimeUnit.SECONDS)) {
				Log.e(TAG, "Failed to terminate executor");
			}
		} catch (InterruptedException e) {
			Log.e(TAG, "Failed to wait for executor shutdown", e);
		}
	}

	private native void init();

	private native int requestProgress(long current, long total, long id);

	private native void requestComplete(int httpCode, byte[] data, long id);

	public static ETLDownload instance() {
		return me;
	}
}
