package com.etlegacy.app;

import android.app.Activity;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

public class DownloadClient {
	private static final String DOWNLOAD_TAG = "Download";
	final Map<String, Future<?>> downloadList = Collections.synchronizedMap(new HashMap<>());
	final ExecutorService executor;
	final Activity context;

	public DownloadClient(Activity context) {
		this.context = context;
		this.executor = Executors.newFixedThreadPool(1);
	}

	public void downloadPackFile(final String httpUrl, Consumer<File> download, Runnable failure, Consumer<DownloadProgress> progress) {
		Future<?> runnable = executor.submit(() -> {
			try {
				URL url = new URL(httpUrl);
				Log.d(DOWNLOAD_TAG, "Starting download: " + httpUrl);
				HttpURLConnection connection = (HttpURLConnection) url.openConnection();

				try {
					connection.setConnectTimeout((int) TimeUnit.SECONDS.toMillis(10));
					connection.setReadTimeout((int) TimeUnit.SECONDS.toMillis(10));
					connection.setRequestMethod("GET");
					connection.setDoInput(true);

					Path tempFile = Files.createTempFile("temp_", "_download");
					try (InputStream stream = connection.getInputStream()) {
						int len = connection.getContentLength();
						copyStream(stream, Files.newOutputStream(tempFile), len, progress);
					}

					if (download != null) {
						download.accept(tempFile.toFile());
					}
					Log.i(DOWNLOAD_TAG, "Download finished for: " + httpUrl);
				} finally {
					connection.disconnect();
				}
			} catch (Exception ex) {
				Log.e(DOWNLOAD_TAG, "Download failure for: " + httpUrl, ex);
				if (failure != null) {
					failure.run();
				}
			}
			downloadList.remove(httpUrl);
		});

		downloadList.put(httpUrl, runnable);
	}

	public void downloadPackFile(final String httpUrl, Path target, Runnable ready, Runnable failure, Consumer<DownloadProgress> progress) {
		downloadPackFile(httpUrl, (file) -> {
			if (!file.exists()) {
				Log.e(DOWNLOAD_TAG, "Downloaded file does not exist");
				if (failure != null) {
					failure.run();
				}
				return;
			}
			try {
				Files.move(file.toPath(), target);
			} catch (Exception ex) {
				Log.e(DOWNLOAD_TAG, "Move failure", ex);
				if (failure != null) {
					failure.run();
					return;
				}
			}
			ready.run();
		}, failure, progress);
	}

	public void downloadPackFile(final String httpUrl, Path target, Runnable failure, Consumer<DownloadProgress> progress) {
		downloadPackFile(httpUrl, (file) -> {
			if (!file.exists()) {
				Log.e(DOWNLOAD_TAG, "Downloaded file does not exist");
				if (failure != null) {
					failure.run();
				}
				return;
			}
			try {
				Files.move(file.toPath(), target, StandardCopyOption.REPLACE_EXISTING);
			} catch (Exception ex) {
				Log.e(DOWNLOAD_TAG, "Move failure", ex);
				if (failure != null) {
					failure.run();
				}
			}
		}, failure, progress);
	}

	public void whenReady(Runnable ready) {
		this.executor.submit(() -> {
			while (!this.downloadList.isEmpty()) {
				try {
					//noinspection BusyWait
					Thread.sleep(100);
				} catch (InterruptedException e) {
					Log.e(DOWNLOAD_TAG, "Failed to sleep", e);
					cancelAll();
				}
			}
			ready.run();
		});
	}

	public void cancelAll() {
		this.downloadList.forEach((requests, future) -> future.cancel(true));
		this.downloadList.clear();
	}

	private void copyStream(InputStream in, OutputStream out, int total, Consumer<DownloadProgress> progress) throws IOException {
		byte[] buffer = new byte[8192];
		int len, current = 0;
		while ((len = in.read(buffer)) != -1) {
			current += len;
			out.write(buffer, 0, len);
			if (progress != null) {
				progress.accept(new DownloadProgress(total, current));
			}
			out.flush();
		}
	}

	public static class DownloadProgress {
		public long total;
		public long written;

		public DownloadProgress(long total, long written) {
			this.total = total;
			this.written = written;
		}
	}
}
