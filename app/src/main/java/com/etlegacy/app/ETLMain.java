package com.etlegacy.app;

import android.app.ProgressDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.Manifest;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Objects;
import java.util.function.Consumer;

public class ETLMain extends AppCompatActivity {

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

	public boolean checkStoragePermissions(){
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.R){
			//Android is 11 (R) or above
			return Environment.isExternalStorageManager();
		}else {
			//Below android 11
			int write = ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);
			int read = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE);
			int bluetooth = ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH);
			int internet = ContextCompat.checkSelfPermission(this, Manifest.permission.INTERNET);

			return read == PackageManager.PERMISSION_GRANTED && write == PackageManager.PERMISSION_GRANTED && bluetooth == PackageManager.PERMISSION_GRANTED
				&& internet == PackageManager.PERMISSION_GRANTED;
		}
	}

	private void requestForStoragePermissions() {
		//Android is 11 (R) or above
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.R){
			try {
				Intent intent = new Intent();
				intent.setAction(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
				Uri uri = Uri.fromParts("package", this.getPackageName(), null);
				intent.setData(uri);
				storageActivityResultLauncher.launch(intent);
			}catch (Exception e){
				Intent intent = new Intent();
				intent.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
				storageActivityResultLauncher.launch(intent);
			}
		}else{
			//Below android 11
			ActivityCompat.requestPermissions(
				this,
				new String[]{
					Manifest.permission.WRITE_EXTERNAL_STORAGE,
					Manifest.permission.READ_EXTERNAL_STORAGE,
					Manifest.permission.BLUETOOTH,
					Manifest.permission.INTERNET
				},
				1
			);
		}

	}

	private final ActivityResultLauncher<Intent> storageActivityResultLauncher =
		registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
			new ActivityResultCallback<ActivityResult>(){

				@Override
				public void onActivityResult(ActivityResult o) {
					if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.R){
						//Android is 11 (R) or above
						if(Environment.isExternalStorageManager()){
							//Manage External Storage Permissions Granted
							Start();
							Log.d("PERMISSION", "onActivityResult: Manage External Storage Permissions Granted");
						}else{
							Log.d("PERMISSION", "onActivityResult: Manage External Storage Permissions Denied");
						}
					}else{
						//Below android 11
						Start();
					}
				}
			});

	@Override
	public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);

		if (requestCode == 1) {

			//Manifest.permission.WRITE_EXTERNAL_STORAGE,
			if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
				Log.v("PERMISSION", "WRITE_EXTERNAL_STORAGE Permission is granted");
			}else {
				Log.v("PERMISSION", "WRITE_EXTERNAL_STORAGE Permission is denied");
			}

			//Manifest.permission.READ_EXTERNAL_STORAGE,
			if (grantResults[1] == PackageManager.PERMISSION_GRANTED) {
				Log.v("PERMISSION", "READ_EXTERNAL_STORAGE Permission is granted");
			}else {
				Log.v("PERMISSION", "READ_EXTERNAL_STORAGE Permission is denied");
			}

			//Manifest.permission.BLUETOOTH,
			if (grantResults[2] == PackageManager.PERMISSION_GRANTED) {
				Log.v("PERMISSION", "BLUETOOTH Permission is granted");
			}else {
				Log.v("PERMISSION", "BLUETOOTH Permission is denied");
			}


			//Manifest.permission.INTERNET
			if (grantResults[3] == PackageManager.PERMISSION_GRANTED) {
				Log.v("PERMISSION", "INTERNET Permission is granted");
			}else {
				Log.v("PERMISSION", "INTERNET Permission is denied");
			}
			Start();
		}
	}

	public void Start() {
		Path etmain = null;
		Path legacy = null;

		if (!isExternalStorageWritable()) {
			Log.d(PACK_TAG, "External storage is not available");
			throw new RuntimeException("Fuck no ext. storage for writing");
		}

		try {
			etmain = Paths.get(Objects.requireNonNull(getExternalFilesDir(null)).toPath() + "/etlegacy/etmain");
			legacy = Paths.get(Objects.requireNonNull(getExternalFilesDir(null)).toPath() + "/etlegacy/legacy");
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

		if (!checkStoragePermissions()) {
			requestForStoragePermissions();
		}
		else {
			Start();
		}

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
