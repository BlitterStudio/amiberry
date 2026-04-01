package com.blitterstudio.amiberry.data

import android.content.Context
import android.content.Intent
import com.blitterstudio.amiberry.AmiberryActivity
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import java.io.File

object EmulatorLauncher {

	/**
	 * Launch emulation with a Quick Start model and optional media.
	 * Uses --model + -0/-s for floppy/CD, plus -G to skip ImGui GUI.
	 */
	fun launchQuickStart(
		context: Context,
		model: AmigaModel,
		floppyPath: String? = null,
		cdPath: String? = null
	) {
		val args = mutableListOf("--rescan-roms", "--model", model.cmdArg)

		if (floppyPath != null && model.hasFloppy) {
			args.addAll(listOf("-0", floppyPath))
		}
		if (cdPath != null && model.hasCd) {
			args.addAll(listOf("-s", "cdimage0=$cdPath"))
		}

		args.add("-G") // Skip ImGui GUI, start emulation directly

		launchSdlActivity(context, args.toTypedArray())
	}

	/**
	 * Launch emulation from a saved .uae config file.
	 */
	fun launchWithConfig(context: Context, configPath: String, skipGui: Boolean = true) {
		val args = mutableListOf("--rescan-roms", "--config", configPath)
		if (skipGui) args.add("-G")

		launchSdlActivity(context, args.toTypedArray())
	}

	/**
	 * Launch a WHDLoad game via the --autoload mechanism.
	 * The emulator auto-configures based on its XML database match.
	 */
	fun launchWhdload(context: Context, lhaPath: String) {
		val args = arrayOf("--rescan-roms", "--autoload", lhaPath, "-G")
		launchSdlActivity(context, args)
	}

	/**
	 * Open the full ImGui GUI, optionally loading a config file for editing.
	 * Does NOT write a session marker or mark as emulator launch, since this
	 * is a GUI-only session — the user expects to return cleanly to the
	 * Kotlin UI without seeing a crash dialog.
	 */
	fun launchAdvancedGui(context: Context, configPath: String? = null) {
		val args = mutableListOf("--rescan-roms")
		if (configPath != null) {
			args.addAll(listOf("--config", configPath))
		}
		// Force GUI even if the config file contains use_gui=no
		args.addAll(listOf("-s", "use_gui=yes"))
		// No -G: ImGui GUI will open for the user

		launchSdlActivity(context, args.toTypedArray(), trackSession = false)
	}

	/**
	 * Launch emulation with pre-built args (used by SettingsScreen).
	 */
	fun launchWithArgs(context: Context, args: Array<String>) {
		launchSdlActivity(context, args)
	}

	private const val SESSION_MARKER = ".emulator_session"

	/**
	 * @param trackSession If true, writes a crash-detection marker and marks the
	 *                     launch for review-prompt tracking. Set to false for
	 *                     GUI-only sessions (Advanced/ImGui) where process exit
	 *                     is expected and should not trigger the crash dialog.
	 */
	private fun launchSdlActivity(context: Context, args: Array<String>, trackSession: Boolean = true) {
		if (trackSession) {
			writeSessionMarker(context)
			(context as? com.blitterstudio.amiberry.ui.MainActivity)?.markEmulatorLaunched()
		}

		// Replace --rescan-roms with a no-op if ROM files haven't changed since last scan
		val finalArgs = if (needsRomRescan(context)) {
			args
		} else {
			args.filter { it != "--rescan-roms" }.toTypedArray()
		}

		val intent = Intent(context, AmiberryActivity::class.java)
		intent.putExtra("SDL_ARGS", finalArgs)
		context.startActivity(intent)
	}

	/**
	 * Check if ROM files have changed since the last launch.
	 * Checks all directories that FileManager.scanForCategory(ROMS) scans:
	 * roms/, root app dir, and whdboot/game-data/Kickstarts/.
	 */
	private fun needsRomRescan(context: Context): Boolean {
		val prefs = AppPreferences.getInstance(context)
		val base = FileManager.getAppStoragePath(context)
		val dirs = listOf(
			FileManager.getCategoryDir(context, FileCategory.ROMS),
			File(base),
			File(base, "whdboot/game-data/Kickstarts")
		)
		val currentFingerprint = dirs.joinToString("|") { computeDirFingerprint(it) }

		if (currentFingerprint == prefs.lastRomFingerprint) return false

		prefs.lastRomFingerprint = currentFingerprint
		return true
	}

	private fun computeDirFingerprint(dir: File): String {
		if (!dir.exists()) return "0:0:0"
		val romFiles = dir.listFiles { f ->
			f.isFile && f.extension.lowercase() in FileCategory.ROMS.extensions
		} ?: return "0:0:0"
		val count = romFiles.size
		val totalSize = romFiles.sumOf { it.length() }
		val latestModified = romFiles.maxOfOrNull { it.lastModified() } ?: 0L
		return "$count:$totalSize:$latestModified"
	}

	private fun writeSessionMarker(context: Context) {
		try {
			val marker = File(context.getExternalFilesDir(null), SESSION_MARKER)
			marker.writeText(System.currentTimeMillis().toString())
		} catch (_: Exception) {
			// Non-critical — crash detection is best-effort
		}
	}

	/**
	 * Delete the session marker. Called by AmiberryActivity on normal exit.
	 */
	fun clearSessionMarker(context: Context) {
		try {
			File(context.getExternalFilesDir(null), SESSION_MARKER).delete()
		} catch (_: Exception) {
			// Ignore
		}
	}

	/**
	 * Check if a previous emulator session crashed (marker file still present).
	 * Returns true if a crash was detected, and cleans up the marker.
	 *
	 * Includes a staleness check: if the marker is older than [STALE_MARKER_THRESHOLD_MS],
	 * it is silently discarded. This prevents false-positive crash dialogs when the system
	 * OOM-kills the :sdl process or the user force-stops the app from Android settings,
	 * since neither path triggers onDestroy(isFinishing=true).
	 */
	fun checkAndClearCrashMarker(context: Context): Boolean {
		val marker = File(context.getExternalFilesDir(null), SESSION_MARKER)
		if (!marker.exists()) return false

		val isCrash = try {
			val startTime = marker.readText().trim().toLongOrNull()
			if (startTime == null) {
				// Corrupt marker — treat as crash to be safe
				true
			} else {
				val elapsed = System.currentTimeMillis() - startTime
				elapsed in 1 until STALE_MARKER_THRESHOLD_MS
			}
		} catch (_: Exception) {
			// Unreadable marker (I/O error) — treat as crash to be safe
			true
		}

		marker.delete()
		return isCrash
	}

	/**
	 * Markers older than this are considered stale (force-stop / OOM) rather than crashes.
	 * 12 hours covers virtually all real emulator sessions.
	 */
	private const val STALE_MARKER_THRESHOLD_MS = 12 * 60 * 60 * 1000L
}
