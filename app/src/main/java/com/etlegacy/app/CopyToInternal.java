package com.etlegacy.app;

import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Paths;

public class CopyToInternal extends AppCompatActivity {

	public CopyToInternal()
	{

	}

	public boolean copyIntoAPPDirectory(String filename) throws IOException {
		File appLibDir = getFilesDir();  // App-specific directory
		File libFile = new File(appLibDir, filename.substring(filename.lastIndexOf("/")+1));
		// Copy library to appLibDir
		Files.deleteIfExists(libFile.toPath());
		try (InputStream in = Files.newInputStream(Paths.get(filename));
			 OutputStream out = Files.newOutputStream(libFile.toPath())) {
			byte[] buffer = new byte[4096];
			int len;
			while ((len = in.read(buffer)) > 0) {
				out.write(buffer, 0, len);
			}
			return true;
		} catch (IOException e) {
			e.printStackTrace();
		}

		return false;
	}
}
