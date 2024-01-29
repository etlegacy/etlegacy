package com.etlegacy.app;

import android.app.Activity;
import android.util.Log;

import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.FileAsyncHttpResponseHandler;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.function.Consumer;

import cz.msebera.android.httpclient.Header;

public class DownloadClient {
	private static final String DOWNLOAD_TAG = "Download";
	final AsyncHttpClient client = new AsyncHttpClient();
	final List<String> downloadList = Collections.synchronizedList(new ArrayList<>());
	final ExecutorService pool;
	final Activity context;

	public DownloadClient(Activity context) {
		this.context = context;
		this.pool = Executors.newFixedThreadPool(1);
	}

	public void downloadPackFile(final String httpUrl, Consumer<File> download, Runnable failure, Consumer<DownloadProgress> progress) {
		downloadList.add(httpUrl);
		client.get(httpUrl, new FileAsyncHttpResponseHandler(this.context) {

			@Override
			public void onSuccess(int statusCode, Header[] headers, File file) {
				Log.d(DOWNLOAD_TAG, "Download complete for: " + httpUrl);
			}

			@Override
			public void onProgress(long bytesWritten, long totalSize) {
				super.onProgress(bytesWritten, totalSize);
				if (progress != null) {
					progress.accept(new DownloadProgress(totalSize, bytesWritten));
				}
			}

			@Override
			public void onFinish() {
				super.onFinish();
				Log.d(DOWNLOAD_TAG, "Download finished for: " + httpUrl);
				if (download != null) {
					download.accept(file);
				}
				downloadList.remove(httpUrl);
			}

			@Override
			public void onFailure(int statusCode, Header[] headers, Throwable throwable, File file) {
				Log.d(DOWNLOAD_TAG, "Download failure for: " + httpUrl);
				if (failure != null) {
					failure.run();
				}
			}
		});
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
		this.pool.submit(() -> {
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
		client.cancelAllRequests(true);
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
