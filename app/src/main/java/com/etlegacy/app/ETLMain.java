package com.etlegacy.app;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Objects;
import java.util.function.Consumer;

public class ETLMain extends Activity {

	private static final String PACK_TAG = "PK3";

	/**
	 * Shows ProgressBar
	 *
	 * @return progressBar obj
	 */
	private ProgressDialog DownloadBar() {
		ProgressDialog asyncDialog = new ProgressDialog(this);
		asyncDialog.setTitle("Downloading Game Data ..");
		asyncDialog.setIndeterminate(false);
		asyncDialog.setProgress(0);
		asyncDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
		asyncDialog.setCancelable(false);
		asyncDialog.setCanceledOnTouchOutside(false);
		asyncDialog.show();

		return asyncDialog;
	}

	/**
	 * Convert pixel metrics to dp
	 *
	 * @param px value of px to be converted
	 * @return dp
	 */
	public static int pxToDp(int px) {
		return (int) (px / Resources.getSystem().getDisplayMetrics().density);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
							 WindowManager.LayoutParams.FLAG_FULLSCREEN);
		getWindow().requestFeature(Window.FEATURE_NO_TITLE);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
			getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
		}

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

		ImageView imageView = new ImageView(this);
		imageView.setBackgroundResource(R.drawable.ic_horizontal_white);

		LinearLayout etlLayout = new LinearLayout(this);

		RelativeLayout.LayoutParams etlParams = new RelativeLayout.LayoutParams(Resources.getSystem().getDisplayMetrics().widthPixels,
																				Resources.getSystem().getDisplayMetrics().heightPixels);

		etlLayout.addView(imageView, etlParams);
		setContentView(etlLayout);

		Path etmain = null;
		Path legacy = null;

		if (!isExternalStorageWritable()) {
			Log.d(PACK_TAG, "External storage is not available");
			throw new RuntimeException("Fuck no ext. storage for writing");
		}

		try {
			etmain = Objects.requireNonNull(getExternalFilesDir("/etlegacy/etmain")).toPath();
			legacy = Objects.requireNonNull(getExternalFilesDir("/etlegacy/legacy")).toPath();
		} catch (Exception e) {
			Log.e("ASSETS", "Could not fetch the external files directory", e);
		}

		assert etmain != null;
		assert legacy != null;
		Log.d("ASSETS", "Acquired the external files dir: " + etmain.toAbsolutePath().toString());

		if (!Files.exists(etmain) && !Files.exists(legacy)) {
			try {
				Files.createDirectories(etmain);
				Files.createDirectories(legacy);
			} catch (IOException e) {
				Log.e(PACK_TAG, "Could not create directories", e);
				// FIXME: actually close the app gracefully
				throw new RuntimeException(e);
			}
		}

		try {
			extractIncludedPackages(legacy);
		} catch (IOException e) {
			Log.e(PACK_TAG, "Could not extract packages", e);
		}

		final Path pak0 = etmain.resolve("pak0.pk3");
		final Path pak1 = etmain.resolve("pak1.pk3");
		final Path pak2 = etmain.resolve("pak2.pk3");

		final Intent intent = new Intent(ETLMain.this, ETLActivity.class);
		intent.addCategory(Intent.CATEGORY_HOME);
		// intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
		intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		if (Files.exists(pak0) && Files.exists(pak1) && Files.exists(pak2)) {
			startActivity(intent);
			finish();
			return;
		}

		final ProgressDialog progressDialog = DownloadBar();
		final DownloadClient cc = new DownloadClient(this);

		Consumer<DownloadClient.DownloadProgress> progressConsumer = (progress) -> {
			final int bytesWritten = (int) (progress.written / Math.pow(1024, 2));
			int totalSize = (int) (progress.total / Math.pow(1024, 2));
			runOnUiThread(() -> {
				progressDialog.setMax(totalSize);
				progressDialog.setProgress(bytesWritten);
			});
		};

		final String mirrorUrl = "https://mirror.etlegacy.com/etmain/";

		if (!Files.exists(pak2)) {
			cc.downloadPackFile(mirrorUrl + pak2.getFileName(), pak2, progressDialog::dismiss, progressConsumer);
		}
		if (!Files.exists(pak1)) {
			cc.downloadPackFile(mirrorUrl + pak1.getFileName(), pak1, progressDialog::dismiss, progressConsumer);
		}
		if (!Files.exists(pak0)) {
			cc.downloadPackFile(mirrorUrl + pak0.getFileName(), pak0, progressDialog::dismiss, progressConsumer);
		}
		cc.whenReady(() -> runOnUiThread(() -> {
			progressDialog.dismiss();
			startActivity(intent);
			finish();
			cc.executor.shutdown();
		}));
	}

	private void extractIncludedPackages(Path etmain) throws IOException {
		AssetManager assManager = getApplicationContext().getAssets();
		String[] assets = Objects.requireNonNull(assManager.list(""));
		for (String file : assets) {
			if (file.endsWith(".pk3")) {

				Path pakFile = etmain.resolve(file);

				if (Files.exists(pakFile)) {
					Log.d(PACK_TAG, "pk3 already exists, skipping copy: " + file);
					continue;
				}

				try (InputStream is = assManager.open(file)) {
					Files.copy(is, pakFile);
				} catch (Exception e) {
					Log.e(PACK_TAG, "Something went wrong with the etl package copy", e);
				}
			}
		}
	}

	private boolean isExternalStorageWritable() {
		return Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
	}
}
