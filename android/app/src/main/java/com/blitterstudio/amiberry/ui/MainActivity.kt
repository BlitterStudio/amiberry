package com.blitterstudio.amiberry.ui

import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.ui.theme.AmiberryTheme
import java.io.File
import java.io.IOException

class MainActivity : ComponentActivity() {

	private var isReady by mutableStateOf(false)

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		enableEdgeToEdge()

		setContent {
			AmiberryTheme {
				if (isReady) {
					AmiberryApp()
				} else {
					Box(
						modifier = Modifier.fillMaxSize(),
						contentAlignment = Alignment.Center
					) {
						Column(horizontalAlignment = Alignment.CenterHorizontally) {
							CircularProgressIndicator()
							Spacer(modifier = Modifier.height(16.dp))
							Text(
								text = "Preparing Amiberry...",
								style = MaterialTheme.typography.bodyLarge
							)
						}
					}
				}
			}
		}

		// Extract assets in background, skip if already done for this version
		Thread {
			extractAssetsIfNeeded()
			ensureDirectories()
			runOnUiThread { isReady = true }
		}.start()
	}

	private fun extractAssetsIfNeeded() {
		val externalDir = getExternalFilesDir(null) ?: return
		val versionFile = File(externalDir, ".extracted_version")
		val currentVersion = try {
			packageManager.getPackageInfo(packageName, 0).versionName
		} catch (e: PackageManager.NameNotFoundException) {
			"unknown"
		}

		if (versionFile.exists() && versionFile.readText().trim() == currentVersion) {
			Log.d(TAG, "Assets already extracted for version $currentVersion, skipping")
			return
		}

		Log.d(TAG, "Extracting assets for version $currentVersion...")
		copyAssets()
		versionFile.writeText(currentVersion ?: "unknown")
		Log.d(TAG, "Asset extraction complete")
	}

	private fun copyAssets() {
		val assetManager = assets
		val files = try {
			assetManager.list("") ?: return
		} catch (e: IOException) {
			Log.e(TAG, "Failed to get asset file list", e)
			return
		}

		val skipDirs = setOf("images", "sounds", "webkit", "kioskmode")
		for (filename in files) {
			if (filename in skipDirs || filename.startsWith("_")) continue
			copyAssetFileOrDir(filename, "")
		}
	}

	private fun copyAssetFileOrDir(filename: String, parentPath: String) {
		val assetManager = assets
		val fullAssetPath = if (parentPath.isNotEmpty()) "$parentPath/$filename" else filename

		val children = try {
			assetManager.list(fullAssetPath)
		} catch (e: IOException) {
			null
		}

		if (children != null && children.isNotEmpty()) {
			// Directory: create and recurse
			val targetDir = File(getExternalFilesDir(null), fullAssetPath)
			if (!targetDir.exists()) targetDir.mkdirs()
			for (child in children) {
				copyAssetFileOrDir(child, fullAssetPath)
			}
		} else {
			// File: copy it
			copyAssetFile(fullAssetPath)
		}
	}

	private fun copyAssetFile(assetPath: String) {
		try {
			val outFile = File(getExternalFilesDir(null), assetPath)
			outFile.parentFile?.let { parent ->
				if (!parent.exists()) parent.mkdirs()
			}

			assets.open(assetPath).use { input ->
				outFile.outputStream().use { output ->
					input.copyTo(output)
				}
			}
		} catch (e: IOException) {
			Log.e(TAG, "Failed to copy asset: $assetPath", e)
		}
	}

	/**
	 * Ensure user-facing subdirectories exist for file organization.
	 */
	private fun ensureDirectories() {
		val base = getExternalFilesDir(null) ?: return
		listOf("roms", "floppies", "hdf", "cd", "lha", "conf").forEach { dir ->
			File(base, dir).let { if (!it.exists()) it.mkdirs() }
		}
	}

	companion object {
		private const val TAG = "Amiberry-Main"
	}
}
