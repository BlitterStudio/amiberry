package com.blitterstudio.amiberry.ui

import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.lifecycle.lifecycleScope
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.blitterstudio.amiberry.ui.theme.AmiberryTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.IOException

class MainActivity : ComponentActivity() {

	private var isReady by mutableStateOf(false)

	override fun onCreate(savedInstanceState: Bundle?) {
		val splashScreen = installSplashScreen()
		super.onCreate(savedInstanceState)
		enableEdgeToEdge()

		// Keep the splash screen visible while assets are being extracted
		splashScreen.setKeepOnScreenCondition { !isReady }

		setContent {
			AmiberryTheme {
				AmiberryApp()
			}
		}

		// Request MANAGE_EXTERNAL_STORAGE on Android 11+ if not already granted
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && !Environment.isExternalStorageManager()) {
			try {
				val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
				intent.data = Uri.parse("package:$packageName")
				storagePermissionLauncher.launch(intent)
			} catch (e: Exception) {
				Log.w(TAG, "Could not open app-specific storage settings, trying generic", e)
				val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
				storagePermissionLauncher.launch(intent)
			}
		} else {
			startAssetExtraction()
		}
	}

	private val storagePermissionLauncher = registerForActivityResult(
		ActivityResultContracts.StartActivityForResult()
	) {
		// Whether user granted or denied, proceed with extraction
		// (app still works with scoped storage, just can't access arbitrary paths)
		startAssetExtraction()
	}

	private fun startAssetExtraction() {
		lifecycleScope.launch(Dispatchers.IO) {
			extractAssetsIfNeeded()
			ensureDirectories()
			withContext(Dispatchers.Main) {
				isReady = true
			}
		}
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

	/**
	 * Directories containing user-modifiable files that should not be overwritten
	 * on version upgrades (users may have customized controller mappings, WHDLoad configs, etc.)
	 */
	private val userModifiableDirs = setOf("controllers", "whdboot", "conf")

	private fun copyAssetFile(assetPath: String) {
		try {
			val outFile = File(getExternalFilesDir(null), assetPath)

			// Skip overwriting files in user-modifiable directories if they already exist
			if (outFile.exists()) {
				val topDir = assetPath.substringBefore('/')
				if (topDir in userModifiableDirs) {
					Log.d(TAG, "Preserving user-modified file: $assetPath")
					return
				}
			}

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
		listOf("roms", "floppies", "harddrives", "cdroms", "lha", "conf").forEach { dir ->
			File(base, dir).let { if (!it.exists()) it.mkdirs() }
		}
	}

	companion object {
		private const val TAG = "Amiberry-Main"
	}
}
