package com.blitterstudio.amiberry.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.lifecycle.lifecycleScope
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.AssetExtraction
import com.blitterstudio.amiberry.data.AssetExtractionFailureActions
import com.blitterstudio.amiberry.data.CrashRecoveryActions
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.EmulatorResumeRecovery
import com.blitterstudio.amiberry.data.ImportFeedback
import com.blitterstudio.amiberry.data.IntentImportExecutor
import com.blitterstudio.amiberry.data.LaunchDiagnostics
import com.blitterstudio.amiberry.data.model.StoragePaths
import com.blitterstudio.amiberry.ui.theme.AmiberryTheme
import com.google.android.play.core.review.ReviewManagerFactory
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.IOException

class MainActivity : ComponentActivity() {

	var isReady by mutableStateOf(false)
		private set
	var emulatorCrashDetected by mutableStateOf(false)
		private set
	var assetExtractionFailed by mutableStateOf(false)
		private set
	var assetExtractionFailureSummary by mutableStateOf<AssetExtraction.CopySummary?>(null)
		private set
	var assetExtractionInProgress by mutableStateOf(false)
		private set
	private val importLaunchGuard = LaunchInFlightGuard()
	val importLaunchInProgress: Boolean
		get() = importLaunchGuard.isLaunching

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

		// Storage permission is NOT requested automatically on startup.
		// The StoragePermissionBanner in the UI provides a user-initiated path
		// to grant MANAGE_EXTERNAL_STORAGE when they choose. This avoids
		// blocking the first-launch experience and strengthens the Play Store
		// case that the permission is user-initiated.
		startAssetExtraction()

		handleIncomingIntent(intent)
	}

	override fun onNewIntent(intent: Intent) {
		super.onNewIntent(intent)
		setIntent(intent)
		handleIncomingIntent(intent)
	}

	/**
	 * Handle ACTION_VIEW intents from file managers.
	 * Imports the file into app storage, then launches the emulator with it.
	 */
	private fun handleIncomingIntent(intent: Intent?) {
		if (intent?.action != Intent.ACTION_VIEW) return
		val uri = intent.data ?: return
		// Clear the action so it isn't re-processed on config change
		intent.action = null

		pendingFileUri = uri
	}

	/** URI from an incoming ACTION_VIEW intent, processed after asset extraction. */
	var pendingFileUri by mutableStateOf<Uri?>(null)
		private set

	fun clearPendingFileUri() {
		pendingFileUri = null
	}

	/**
	 * Import a file from a content/file URI and launch emulation with it.
	 */
	fun importAndLaunch(uri: Uri) {
		if (!importLaunchGuard.begin()) {
			Log.d(TAG, "Ignoring incoming file intent while another import launch is in progress")
			return
		}
		lifecycleScope.launch(Dispatchers.IO) {
			try {
				val preparedImport = try {
					IntentImportExecutor.importAndPrepare(this@MainActivity, uri)
				} catch (e: Exception) {
					Log.e(TAG, "Failed to handle incoming file intent", e)
					ImportFeedback.importFailed(uri.lastPathSegment ?: "file").let { feedback ->
						IntentImportExecutor.PreparedImport(feedback)
					}
				}
				withContext(Dispatchers.Main) {
					showImportFeedback(preparedImport.feedback)
					launchPreparedImport(preparedImport.launch)
				}
			} finally {
				withContext(NonCancellable + Dispatchers.Main) {
					importLaunchGuard.finish()
				}
			}
		}
	}

	private fun launchPreparedImport(launch: IntentImportExecutor.Launch?) {
		when (launch) {
			is IntentImportExecutor.Launch.SavedConfig -> EmulatorLauncher.launchWithConfig(
				this,
				launch.configPath,
				controlSettings = launch.controlSettings
			)
			is IntentImportExecutor.Launch.QuickStart -> EmulatorLauncher.launchQuickStart(
				context = this,
				model = launch.model,
				floppyPath = launch.floppyPath,
				floppy1Path = launch.floppy1Path,
				cdPath = launch.cdPath,
				configPath = launch.configPath
			)
			is IntentImportExecutor.Launch.WhdLoad -> EmulatorLauncher.launchWhdload(
				this,
				launch.lhaPath,
				launch.configPath
			)
			is IntentImportExecutor.Launch.Rp9 -> EmulatorLauncher.launchRp9(
				this,
				launch.path,
				launch.controlSettings
			)
			null -> Unit
		}
	}

	private fun showImportFeedback(message: ImportFeedback.Message) {
		Toast.makeText(this, getString(message.stringRes, message.argument), Toast.LENGTH_SHORT).show()
	}

	private fun startAssetExtraction() {
		if (assetExtractionInProgress) {
			Log.d(TAG, "Ignoring asset extraction start while extraction is already in progress")
			return
		}
		assetExtractionInProgress = true
		lifecycleScope.launch(Dispatchers.IO) {
			try {
				val copySummary = extractAssetsIfNeeded()
				ensureDirectories()
				withContext(Dispatchers.Main) {
					assetExtractionFailureSummary = copySummary.takeUnless { it.isSuccess }
					assetExtractionFailed = !copySummary.isSuccess
					isReady = true
					processResumeRecovery()
				}
			} finally {
				withContext(NonCancellable + Dispatchers.Main) {
					assetExtractionInProgress = false
				}
			}
		}
	}

	fun retryAssetExtraction() {
		assetExtractionFailed = false
		assetExtractionFailureSummary = null
		isReady = false
		startAssetExtraction()
	}

	/**
	 * @return successful summary if assets are ready (already extracted or freshly extracted),
	 *         failed summary if extraction failed.
	 */
	private fun extractAssetsIfNeeded(): AssetExtraction.CopySummary {
		val externalDir = getExternalFilesDir(null)
		if (externalDir == null) {
			Log.e(TAG, "External files directory is unavailable")
			return AssetExtraction.CopySummary.failed("<external-files-dir>")
		}
		val versionFile = File(externalDir, ".extracted_version")
		val currentVersion = try {
			packageManager.getPackageInfo(packageName, 0).versionName
		} catch (e: PackageManager.NameNotFoundException) {
			"unknown"
		}

		if (versionFile.exists() && versionFile.readText().trim() == currentVersion) {
			Log.d(TAG, "Assets already extracted for version $currentVersion, skipping")
			return AssetExtraction.CopySummary.Empty
		}

		Log.d(TAG, "Extracting assets for version $currentVersion...")
		return try {
			val copySummary = copyAssets()
			if (!copySummary.isSuccess) {
				Log.e(TAG, "Asset extraction failed for: ${copySummary.failedAssets.joinToString()}")
				return copySummary
			}
			versionFile.writeText(currentVersion ?: "unknown")
			Log.d(TAG, "Asset extraction complete: copied=${copySummary.copiedFiles}, preserved=${copySummary.preservedFiles}")
			copySummary
		} catch (e: Exception) {
			Log.e(TAG, "Asset extraction failed", e)
			AssetExtraction.CopySummary.failed("<asset-extraction>")
		}
	}

	private fun copyAssets(): AssetExtraction.CopySummary {
		val assetManager = assets
		val files = try {
			assetManager.list("") ?: return AssetExtraction.CopySummary.failed("<asset-root>")
		} catch (e: IOException) {
			Log.e(TAG, "Failed to get asset file list", e)
			return AssetExtraction.CopySummary.failed("<asset-root>")
		}

		var summary = AssetExtraction.CopySummary.Empty
		for (filename in files) {
			if (AssetExtraction.shouldSkipTopLevelAsset(filename)) continue
			summary += copyAssetFileOrDir(filename, "")
		}
		return summary
	}

	private fun copyAssetFileOrDir(filename: String, parentPath: String): AssetExtraction.CopySummary {
		val assetManager = assets
		val fullAssetPath = if (parentPath.isNotEmpty()) "$parentPath/$filename" else filename

		val children = try {
			assetManager.list(fullAssetPath)
		} catch (e: IOException) {
			Log.e(TAG, "Failed to list asset path: $fullAssetPath", e)
			null
		}

		if (children != null && children.isNotEmpty()) {
			// Directory: create and recurse
			val externalDir = getExternalFilesDir(null) ?: return AssetExtraction.CopySummary.failed(fullAssetPath)
			val targetDir = File(externalDir, AssetExtraction.storagePathForAsset(fullAssetPath))
			if (!targetDir.exists() && !targetDir.mkdirs()) {
				Log.e(TAG, "Failed to create asset target directory: ${targetDir.absolutePath}")
				return AssetExtraction.CopySummary.failed(fullAssetPath)
			}
			var summary = AssetExtraction.CopySummary.Empty
			for (child in children) {
				summary += copyAssetFileOrDir(child, fullAssetPath)
			}
			return summary
		} else {
			// File: copy it
			return copyAssetFile(fullAssetPath)
		}
	}

	/**
	 * Directories containing user-modifiable files that should not be overwritten
	 * on version upgrades (users may have customized controller mappings, WHDLoad configs, etc.)
	 */
	private fun copyAssetFile(assetPath: String): AssetExtraction.CopySummary {
		try {
			val externalDir = getExternalFilesDir(null) ?: return AssetExtraction.CopySummary.failed(assetPath)
			val storagePath = AssetExtraction.storagePathForAsset(assetPath)
			val outFile = File(externalDir, storagePath)

			// Skip overwriting files in user-modifiable directories if they already exist
			if (outFile.exists() && AssetExtraction.shouldPreserveExistingAsset(storagePath)) {
				Log.d(TAG, "Preserving user-modified file: $storagePath")
				return AssetExtraction.CopySummary.preserved()
			}

			outFile.parentFile?.let { parent ->
				if (!parent.exists() && !parent.mkdirs()) {
					Log.e(TAG, "Failed to create asset parent directory: ${parent.absolutePath}")
					return AssetExtraction.CopySummary.failed(assetPath)
				}
			}

			assets.open(assetPath).use { input ->
				outFile.outputStream().use { output ->
					input.copyTo(output)
				}
			}
			return AssetExtraction.CopySummary.copied()
		} catch (e: IOException) {
			Log.e(TAG, "Failed to copy asset: $assetPath", e)
			return AssetExtraction.CopySummary.failed(assetPath)
		}
	}

	/**
	 * Ensure user-facing subdirectories exist for file organization.
	 */
	private fun ensureDirectories() {
		val base = getExternalFilesDir(null) ?: return
		StoragePaths.userContentDirs.forEach { dir ->
			File(base, dir).let { if (!it.exists()) it.mkdirs() }
		}
	}

	/** Tracks whether the emulator was running so we can distinguish
	 *  "returning from emulation" vs normal activity resume. */
	private var emulatorWasLaunched = false
	private var isActivityResumed = false

	override fun onResume() {
		super.onResume()
		isActivityResumed = true
		processResumeRecovery()
	}

	override fun onPause() {
		isActivityResumed = false
		super.onPause()
	}

	private fun processResumeRecovery() {
		if (EmulatorResumeRecovery.shouldDefer(isReady = isReady, isResumed = isActivityResumed)) {
			return
		}
		// Check for user-initiated quit (pause menu → Quit to Launcher)
		// before checking for crashes. Clean exit takes priority.
		val hadTrackedSession = EmulatorLauncher.hasSessionMarker(this)
		val wasCleanExit = EmulatorLauncher.checkAndClearCleanExit(this)
		val wasCrash = !wasCleanExit && EmulatorLauncher.checkAndClearCrashMarker(this)
		val decision = EmulatorResumeRecovery.decide(
			wasCleanExit = wasCleanExit,
			wasCrash = wasCrash,
			emulatorWasLaunched = emulatorWasLaunched,
			hadTrackedSession = hadTrackedSession
		)

		if (decision.clearSessionMarker) {
			EmulatorLauncher.clearSessionMarker(this)
		}
		if (decision.showCrashDialog) {
			emulatorCrashDetected = true
		}
		if (decision.countSuccessfulSession) {
			if (AppPreferences.getInstance(this).incrementLaunchCountAndCheckReview()) {
				requestInAppReview()
			}
		}
		emulatorWasLaunched = decision.keepLaunchTracked
	}

	/** Called by EmulatorLauncher before starting the SDL activity. */
	fun markEmulatorLaunched() {
		emulatorWasLaunched = true
	}

	private fun requestInAppReview() {
		val manager = ReviewManagerFactory.create(this)
		manager.requestReviewFlow().addOnSuccessListener { reviewInfo ->
			manager.launchReviewFlow(this, reviewInfo)
			// No success/failure handling needed — Google controls the UI
		}
	}

	fun clearCrashFlag() {
		emulatorCrashDetected = false
	}

	fun crashDiagnosticsSummary(): String? =
		CrashRecoveryActions.summary(latestLaunchDiagnostics())

	fun copyCrashDiagnosticsToClipboard() {
		val packageInfo = runCatching {
			packageManager.getPackageInfo(packageName, 0)
		}.getOrNull()
		val payload = CrashRecoveryActions.copyPayload(
			entry = latestLaunchDiagnostics(),
			packageName = packageName,
			versionName = packageInfo?.versionName ?: "unknown",
			versionCode = packageInfo?.let { info ->
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
					info.longVersionCode
				} else {
					@Suppress("DEPRECATION")
					info.versionCode.toLong()
				}
			} ?: 0L,
			sdkInt = Build.VERSION.SDK_INT
		)
		val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
		clipboard.setPrimaryClip(ClipData.newPlainText("Amiberry crash diagnostics", payload))

		val message = CrashRecoveryActions.copyMessage()
		Toast.makeText(
			this,
			message.argument?.let { getString(message.stringRes, it) } ?: getString(message.stringRes),
			Toast.LENGTH_SHORT
		).show()
	}

	fun assetExtractionFailureDetails(): String? =
		AssetExtractionFailureActions.summary(assetExtractionFailureSummary)

	fun copyAssetExtractionFailureDetailsToClipboard() {
		val packageInfo = runCatching {
			packageManager.getPackageInfo(packageName, 0)
		}.getOrNull()
		val payload = AssetExtractionFailureActions.copyPayload(
			copySummary = assetExtractionFailureSummary,
			packageName = packageName,
			versionName = packageInfo?.versionName ?: "unknown",
			versionCode = packageInfo?.let { info ->
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
					info.longVersionCode
				} else {
					@Suppress("DEPRECATION")
					info.versionCode.toLong()
				}
			} ?: 0L,
			sdkInt = Build.VERSION.SDK_INT,
			externalDirPath = getExternalFilesDir(null)?.absolutePath
		)
		val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
		clipboard.setPrimaryClip(ClipData.newPlainText("Amiberry setup failure", payload))

		val message = AssetExtractionFailureActions.copyMessage()
		Toast.makeText(
			this,
			message.argument?.let { getString(message.stringRes, it) } ?: getString(message.stringRes),
			Toast.LENGTH_SHORT
		).show()
	}

	private fun latestLaunchDiagnostics(): LaunchDiagnostics.Entry? =
		runCatching {
			LaunchDiagnostics.read(File(filesDir, LaunchDiagnostics.LAST_LAUNCH_FILE))
		}.getOrNull()

	companion object {
		private const val TAG = "Amiberry-Main"
	}
}
