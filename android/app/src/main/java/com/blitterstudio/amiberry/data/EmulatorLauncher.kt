package com.blitterstudio.amiberry.data

import android.content.Context
import android.content.Intent
import com.blitterstudio.amiberry.AmiberryActivity
import com.blitterstudio.amiberry.data.model.AmigaModel

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
		val args = mutableListOf("--model", model.cmdArg)

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
		val args = mutableListOf("--config", configPath)
		if (skipGui) args.add("-G")

		launchSdlActivity(context, args.toTypedArray())
	}

	/**
	 * Launch a WHDLoad game via the --autoload mechanism.
	 * The emulator auto-configures based on its XML database match.
	 */
	fun launchWhdload(context: Context, lhaPath: String) {
		val args = arrayOf("--autoload", lhaPath, "-G")
		launchSdlActivity(context, args)
	}

	/**
	 * Open the full ImGui GUI, optionally loading a config file for editing.
	 */
	fun launchAdvancedGui(context: Context, configPath: String? = null) {
		val args = mutableListOf<String>()
		if (configPath != null) {
			args.addAll(listOf("--config", configPath))
		}
		// No -G: ImGui GUI will open for the user

		launchSdlActivity(context, args.toTypedArray())
	}

	private fun launchSdlActivity(context: Context, args: Array<String>) {
		val intent = Intent(context, AmiberryActivity::class.java)
		intent.putExtra("SDL_ARGS", args)
		context.startActivity(intent)
	}
}
