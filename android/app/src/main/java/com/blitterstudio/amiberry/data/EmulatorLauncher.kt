package com.blitterstudio.amiberry.data

import android.content.Context
import android.content.Intent
import com.blitterstudio.amiberry.AmiberryActivity
import com.blitterstudio.amiberry.data.model.AmigaModel
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
	 */
	fun launchAdvancedGui(context: Context, configPath: String? = null) {
		val args = mutableListOf("--rescan-roms")
		if (configPath != null) {
			args.addAll(listOf("--config", configPath))
		}
		// Force GUI even if the config file contains use_gui=no
		args.addAll(listOf("-s", "use_gui=yes"))
		// No -G: ImGui GUI will open for the user

		launchSdlActivity(context, args.toTypedArray())
	}

	/**
	 * Launch emulation with pre-built args (used by SettingsScreen).
	 */
	fun launchWithArgs(context: Context, args: Array<String>) {
		launchSdlActivity(context, args)
	}

	private const val SESSION_MARKER = ".emulator_session"

	private fun launchSdlActivity(context: Context, args: Array<String>) {
		writeSessionMarker(context)
		(context as? com.blitterstudio.amiberry.ui.MainActivity)?.markEmulatorLaunched()
		val intent = Intent(context, AmiberryActivity::class.java)
		intent.putExtra("SDL_ARGS", args)
		context.startActivity(intent)
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
	 */
	fun checkAndClearCrashMarker(context: Context): Boolean {
		val marker = File(context.getExternalFilesDir(null), SESSION_MARKER)
		if (marker.exists()) {
			marker.delete()
			return true
		}
		return false
	}
}
