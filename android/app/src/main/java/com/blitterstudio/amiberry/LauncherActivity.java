package com.blitterstudio.amiberry;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import org.libsdl.app.SDLActivity;

public class LauncherActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_launcher);

        Button btnStart = findViewById(R.id.btn_start);
        btnStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Disable button to prevent multiple clicks
                v.setEnabled(false);
                ((Button)v).setText("Extracting data...");

                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        copyAssets();
                        
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Intent intent = new Intent(LauncherActivity.this, SDLActivity.class);
                                startActivity(intent);
                                finish();
                            }
                        });
                    }
                }).start();
            }
        });
    }

    private void copyAssets() {
        android.content.res.AssetManager assetManager = getAssets();
        String[] files = null;
        try {
            files = assetManager.list("");
        } catch (java.io.IOException e) {
            android.util.Log.e("Launcher", "Failed to get asset file list.", e);
        }

        if (files != null) {
            for (String filename : files) {
                // Skip system files and hidden files
                if (filename.equals("images") || filename.equals("sounds") || filename.equals("webkit") || 
                    filename.equals("kioskmode") || filename.startsWith("_")) {
                        continue;
                }
                
                // Recursively copy assets
                copyAssetFileOrDir(assetManager, filename, "");
            }
        }
    }

    private void copyAssetFileOrDir(android.content.res.AssetManager assetManager, String filename, String targetSubDir) {
        String fullAssetName = targetSubDir.length() > 0 ? targetSubDir + "/" + filename : filename;
        
        // Initial check: is it a directory or file?
        // AssetManager doesn't give an easy way to distinguish.
        // Try to list it. If it returns children, it's a dir.
        String[] children = null;
        try {
            children = assetManager.list(fullAssetName);
        } catch (java.io.IOException e) {
            // It might be a file or just empty dir.
        }

        if (children != null && children.length > 0) {
            // It is a directory
            String newSubDir = fullAssetName; // logic simplified
            // Create directory in target
            java.io.File targetDir = new java.io.File(getExternalFilesDir(null), newSubDir);
            android.util.Log.d("Amiberry-Launcher", "Extracting dir to: " + targetDir.getAbsolutePath());
            if (!targetDir.exists()) {
                targetDir.mkdirs();
            }
            
            for (String child : children) {
                copyAssetFileOrDir(assetManager, child, newSubDir);
            }
        } else {
            // It is a file (or empty dir, but we treat as file first and fall back if exception)
            copyAssetFile(assetManager, fullAssetName);
        }
    }

    private void copyAssetFile(android.content.res.AssetManager assetManager, String filename) {
        java.io.InputStream in = null;
        java.io.OutputStream out = null;
        try {
            java.io.File outFile = new java.io.File(getExternalFilesDir(null), filename);
             android.util.Log.d("Amiberry-Launcher", "Extracting file to: " + outFile.getAbsolutePath());
            in = assetManager.open(filename);

            // Check if file exists and is newer? For now, just overwrite or skip if exists?
            // Safer to overwrite for development.
            // if (outFile.exists()) return; 

            // Ensure parent exists
            java.io.File parent = outFile.getParentFile();
            if (parent != null && !parent.exists()) {
                parent.mkdirs();
            }

            out = new java.io.FileOutputStream(outFile);
            copyFile(in, out);
        } catch(java.io.IOException e) {
            // If it failed to open, it might have been an empty directory that list() returned empty for?
            // Or just a genuine error.
            android.util.Log.e("Launcher", "Failed to copy asset file: " + filename, e);
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (java.io.IOException e) {
                    // NOOP
                }
            }
            if (out != null) {
                try {
                    out.close();
                } catch (java.io.IOException e) {
                    // NOOP
                }
            }
        }
    }

    private void copyFile(java.io.InputStream in, java.io.OutputStream out) throws java.io.IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }
}
