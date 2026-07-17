package com.blitterstudio.amiberry.data

import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.content.Intent
import com.blitterstudio.amiberry.AmiberryActivity
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import org.json.JSONObject
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
		floppy1Path: String? = null,
		cdPath: String? = null,
		configPath: String? = null
	) {
		val request = LaunchRequest.QuickStart(
			model = model,
			configPath = configPath,
			floppy0 = floppyPath,
			floppy1 = floppy1Path,
			cd = cdPath
		)

		launchRequest(context, request)

		AppPreferences.getInstance(context).addRecentLaunch(JSONObject().apply {
			put("type", "quickstart")
			put("model", model.cmdArg)
			put("df0", floppyPath ?: "")
			put("df1", floppy1Path ?: "")
			put("cd", cdPath ?: "")
		})
	}

	/**
	 * Launch emulation from a saved .uae config file.
	 */
	fun launchWithConfig(
		context: Context,
		configPath: String,
		skipGui: Boolean = true,
		controlSettings: EmulatorSettings? = null
	) {
		val request = LaunchRequest.SavedConfig(
			configPath = configPath,
			skipGui = skipGui,
			controlOverrides = controlSettings?.let { LaunchRequest.AndroidControlOverrides.fromSettings(it) }
		)

		launchRequest(context, request)
		AppPreferences.getInstance(context).addRecentLaunch(JSONObject().apply {
			put("type", "config")
			put("path", configPath)
		})
	}

	/**
	 * Launch the current Settings screen snapshot.
	 * This is intentionally not added to Recents because the backing config is
	 * the transient .current_settings.uae file, not a user-saved configuration.
	 */
	fun launchSettingsConfig(context: Context, model: AmigaModel, configPath: String) {
		launchRequest(context, LaunchRequest.SettingsConfig(model = model, configPath = configPath))
	}

	/**
	 * Launch a WHDLoad game via the --autoload mechanism.
	 * The emulator auto-configures based on its XML database match.
	 */
	fun launchWhdload(context: Context, lhaPath: String, configPath: String? = null) {
		val request = LaunchRequest.WhdLoad(lhaPath = lhaPath, configPath = configPath)
		launchRequest(context, request)
		AppPreferences.getInstance(context).addRecentLaunch(JSONObject().apply {
			put("type", "whdload")
			put("path", lhaPath)
		})
	}

	fun launchRp9(context: Context, path: String) {
		launchRequest(context, LaunchRequest.Rp9(path))
		AppPreferences.getInstance(context).addRecentLaunch(JSONObject().apply {
			put("type", "rp9")
			put("path", path)
		})
	}

	/**
	 * Open the full ImGui GUI, optionally loading a config file for editing.
	 * Does NOT write a session marker or mark as emulator launch, since this
	 * is a GUI-only session — the user expects to return cleanly to the
	 * Kotlin UI without seeing a crash dialog.
	 */
	fun launchAdvancedGui(context: Context, configPath: String? = null) {
		// No -G: ImGui GUI will open for the user

		launchRequest(
			context = context,
			request = LaunchRequest.AdvancedGui(configPath = configPath)
		)
	}

	private const val SESSION_MARKER = ".emulator_session"
	private const val CLEAN_EXIT_MARKER = ".clean_exit"

	private fun launchRequest(context: Context, request: LaunchRequest) {
		launchSdlActivity(context, request, request.toArgs(), request.trackSession)
	}

	/**
	 * @param trackSession If true, writes a crash-detection marker and marks the
	 *                     launch for review-prompt tracking. Set to false for
	 *                     GUI-only sessions (Advanced/ImGui) where process exit
	 *                     is expected and should not trigger the crash dialog.
	 */
	private fun launchSdlActivity(
		context: Context,
		request: LaunchRequest,
		args: Array<String>,
		trackSession: Boolean = true
	) {
		if (trackSession) {
			writeSessionMarker(context)
		} else {
			clearUntrackedLaunchMarkers(context)
		}

		val finalArgs = prepareLaunchArgs(args)
		writeLaunchDiagnostics(context, request, finalArgs, trackSession)

		val intent = Intent(context, AmiberryActivity::class.java)
		intent.putExtra("SDL_ARGS", finalArgs)
		if (!context.isActivityBacked()) {
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
		}
		try {
			context.startActivity(intent)
		} catch (e: RuntimeException) {
			if (trackSession) {
				clearSessionMarker(context)
			}
			throw e
		}

		if (trackSession) {
			(context as? com.blitterstudio.amiberry.ui.MainActivity)?.markEmulatorLaunched()
		}
	}

	internal fun prepareLaunchArgs(args: Array<String>): Array<String> {
		if (args.contains(RESCAN_ROMS_ARG)) {
			return args.copyOf()
		}
		return arrayOf(RESCAN_ROMS_ARG, *args)
	}

	private fun writeLaunchDiagnostics(
		context: Context,
		request: LaunchRequest,
		finalArgs: Array<String>,
		trackSession: Boolean
	) {
		try {
			LaunchDiagnostics.write(
				context.filesDir,
				LaunchDiagnostics.fromRequest(
					request = request,
					args = finalArgs.toList(),
					trackSession = trackSession,
					startedAtEpochMs = System.currentTimeMillis()
				)
			)
		} catch (_: Exception) {
			// Non-critical — diagnostics are best-effort
		}
	}

	private fun writeSessionMarker(context: Context) {
		try {
			val baseDir = context.getExternalFilesDir(null) ?: return
			prepareTrackedSessionMarkers(baseDir)
		} catch (_: Exception) {
			// Non-critical — crash detection is best-effort
		}
	}

	internal fun prepareTrackedSessionMarkers(baseDir: File, nowMs: Long = System.currentTimeMillis()) {
		File(baseDir, CLEAN_EXIT_MARKER).delete()
		File(baseDir, SESSION_MARKER).writeText(nowMs.toString())
	}

	private fun clearUntrackedLaunchMarkers(context: Context) {
		try {
			val baseDir = context.getExternalFilesDir(null) ?: return
			prepareUntrackedLaunchMarkers(baseDir)
		} catch (_: Exception) {
			// Non-critical — marker cleanup is best-effort
		}
	}

	internal fun prepareUntrackedLaunchMarkers(baseDir: File) {
		File(baseDir, CLEAN_EXIT_MARKER).delete()
		File(baseDir, SESSION_MARKER).delete()
	}

	private fun Context.isActivityBacked(): Boolean = when (this) {
		is Activity -> true
		is ContextWrapper -> baseContext?.isActivityBacked() == true
		else -> false
	}

	internal fun sessionMarkerExists(baseDir: File): Boolean =
		File(baseDir, SESSION_MARKER).exists()

	fun hasSessionMarker(context: Context): Boolean =
		try {
			context.getExternalFilesDir(null)?.let { sessionMarkerExists(it) } == true
		} catch (_: Exception) {
			false
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
	 * Write a clean-exit marker so the main process knows this was a
	 * user-initiated quit (e.g., from the pause menu), not a crash.
	 * Called from the :sdl process before finish().
	 */
	fun writeCleanExitMarker(context: Context) {
		try {
			File(context.getExternalFilesDir(null), CLEAN_EXIT_MARKER).writeText("1")
		} catch (_: Exception) {
			// Best-effort
		}
	}

	/**
	 * Check and consume the clean-exit marker. Returns true if present
	 * (meaning the user quit intentionally).
	 */
	fun checkAndClearCleanExit(context: Context): Boolean {
		val marker = File(context.getExternalFilesDir(null), CLEAN_EXIT_MARKER)
		if (marker.exists()) {
			marker.delete()
			return true
		}
		return false
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
	private const val RESCAN_ROMS_ARG = "--rescan-roms"
}
