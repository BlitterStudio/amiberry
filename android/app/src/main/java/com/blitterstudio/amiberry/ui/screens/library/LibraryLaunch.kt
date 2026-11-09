package com.blitterstudio.amiberry.ui.screens.library

import android.content.Context
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AndroidLaunchConfig
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.ConfigParser
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

/**
 * The model a disk played straight from the Library boots: the first model with a drive,
 * one with its own Kickstart before one that would boot AROS. Null: none can start.
 */
internal fun <M> floppyLaunchModel(
	current: M,
	available: List<M>,
	hasFloppy: (M) -> Boolean,
	hasKickstart: (M) -> Boolean
): M? {
	val withDrive = available.filter(hasFloppy)
	if (current in withDrive) return current
	return withDrive.firstOrNull(hasKickstart) ?: withDrive.firstOrNull()
}

/**
 * The Library's launches. Quick play runs a file in the machine its kind needs (WHDBooter
 * deciding the hardware for a WHDLoad archive), from the settings of the last session —
 * not the editor's, which may hold another machine's unsaved edits. Playing in a saved
 * machine loads the machine and puts the file in DF0: or its CD drive.
 */
internal object LibraryLaunch {

	/** The configuration a Play In writes, beside the machines it plays. */
	internal const val PLAY_IN_CONFIG = ".library_play_in.uae"

	/** What playing a file ended in: started, or the message to show instead. */
	sealed interface PlayResult {
		data object Started : PlayResult
		data class Refused(val messageRes: Int, val argument: String? = null) : PlayResult
	}

	/** Quick play: the file's kind in its machine, started at once. */
	suspend fun play(context: Context, file: AmigaFile, roms: List<AmigaFile>): PlayResult =
		when (file.category) {
			FileCategory.FLOPPIES -> playFloppy(context, file, roms)
			FileCategory.CD_IMAGES -> playCd(context, file, roms)
			FileCategory.WHDLOAD_GAMES -> playWhdload(context, file)
			else -> PlayResult.Refused(R.string.library_play_unsupported, file.name)
		}

	/** A disk in an A500-style machine (one with its Kickstart before one on AROS). */
	private suspend fun playFloppy(context: Context, disk: AmigaFile, roms: List<AmigaFile>): PlayResult {
		val model = floppyLaunchModel(
			current = AmigaModel.A500,
			available = ModelRomAvailability.availableModels(roms),
			hasFloppy = { it.hasFloppy },
			hasKickstart = { ModelRomAvailability.hasKickstart(it, roms) }
		) ?: return PlayResult.Refused(R.string.quick_start_no_compatible_models_message)
		launchQuickPlay(context, model, roms, floppy0 = disk.path)
		return PlayResult.Started
	}

	/** A CD in a CD32 (a CDTV when only its Kickstart is installed). */
	private suspend fun playCd(context: Context, cd: AmigaFile, roms: List<AmigaFile>): PlayResult {
		val model = listOf(AmigaModel.CD32, AmigaModel.CDTV)
			.firstOrNull { ModelRomAvailability.isModelAvailable(it, roms) }
			?: return PlayResult.Refused(R.string.library_cd_needs_rom)
		launchQuickPlay(context, model, roms, cdImage = cd.path)
		return PlayResult.Started
	}

	/** A WHDLoad archive through the WHDBooter path: it decides the hardware at start. */
	suspend fun playWhdload(context: Context, game: AmigaFile): PlayResult =
		playWhdloadPath(context, game.path)

	private suspend fun playWhdloadPath(context: Context, path: String): PlayResult {
		withContext(Dispatchers.IO) {
			val prepared = AndroidLaunchConfig.prepareWhdLoad(
				context,
				path,
				AndroidLaunchConfig.loadLastSessionSettings(context)
			)
			EmulatorLauncher.launchWhdload(
				context = context,
				lhaPath = path,
				configPath = prepared.configFile.absolutePath,
				shaderOverride = prepared.shaderOverride,
				controlOverrides = prepared.controlOverrides
			)
		}
		return PlayResult.Started
	}

	/** A recent entry again, as it was launched. */
	suspend fun playRecent(context: Context, entry: JSONObject, roms: List<AmigaFile>): PlayResult {
		val path = entry.optString("path")
		return when (RecentLaunches.Type.of(entry)) {
			RecentLaunches.Type.Configuration -> {
				EmulatorLauncher.launchWithConfig(
					context,
					path,
					rememberedControls = AppPreferences.getInstance(context).rememberedAndroidControls()
				)
				PlayResult.Started
			}
			RecentLaunches.Type.WhdLoad -> playWhdloadPath(context, path)
			RecentLaunches.Type.Rp9 -> {
				withContext(Dispatchers.IO) {
					EmulatorLauncher.launchRp9(
						context,
						path,
						AndroidLaunchConfig.loadLastSessionSettings(context)
					)
				}
				PlayResult.Started
			}
			RecentLaunches.Type.QuickStart -> playRecentQuickStart(context, entry, roms)
			RecentLaunches.Type.Unknown -> PlayResult.Refused(R.string.recent_type_unknown)
		}
	}

	private suspend fun playRecentQuickStart(context: Context, entry: JSONObject, roms: List<AmigaFile>): PlayResult {
		val model = AmigaModel.find(entry.optString("model")) ?: AmigaModel.A500
		if (model !in ModelRomAvailability.availableModels(roms)) {
			return PlayResult.Refused(R.string.quick_start_no_compatible_models_message)
		}
		val floppy0 = entry.optString("df0")
		val floppy1 = entry.optString("df1")
		val cdImage = entry.optString("cd")
		launchQuickPlay(context, model, roms, floppy0, floppy1, cdImage)
		return PlayResult.Started
	}

	/**
	 * Writes the machine the launch plays (the model's preset with the last session's
	 * controls and the media in its drives) and starts it.
	 */
	private suspend fun launchQuickPlay(
		context: Context,
		model: AmigaModel,
		roms: List<AmigaFile>,
		floppy0: String = "",
		floppy1: String = "",
		cdImage: String = ""
	) {
		withContext(Dispatchers.IO) {
			val settings = recentQuickStartSettingsSnapshot(
				model = model,
				roms = roms,
				currentSettings = AndroidLaunchConfig.loadLastSessionSettings(context),
				floppy0 = floppy0,
				floppy1 = floppy1,
				cdImage = cdImage
			)
			val configFile = ConfigGenerator.writeConfig(context, settings, AndroidLaunchConfig.QUICKSTART_CONFIG)
			EmulatorLauncher.launchQuickStart(
				context = context,
				model = model,
				configPath = configFile.absolutePath,
				floppyPath = floppy0.takeIf { it.isNotBlank() },
				floppy1Path = floppy1.takeIf { it.isNotBlank() },
				cdPath = cdImage.takeIf { it.isNotBlank() }
			)
		}
	}

	/**
	 * A Recent launch's machine: the model the entry names, built with the ROMs it was
	 * launched with, the media it had in its drives and the current controls.
	 */
	internal fun recentQuickStartSettingsSnapshot(
		model: AmigaModel,
		roms: List<AmigaFile>,
		currentSettings: EmulatorSettings,
		floppy0: String,
		floppy1: String,
		cdImage: String
	): EmulatorSettings = AndroidLaunchConfig.buildQuickStartSettingsSnapshot(
		model = model,
		roms = roms,
		currentSettings = currentSettings,
		floppy0 = floppy0,
		floppy1 = floppy1,
		cdImage = cdImage
	)

	/**
	 * Plays [file] in [machine]: loads the machine, puts the file in DF0: (a disk) or its
	 * CD drive (a CD) and starts it. A CD is refused for a machine whose model has no CD
	 * drive, as setting the empty cdimage0 would add one the machine was never set up for.
	 * [loadMachine] reads the machine's file; [writeConfig] writes the launch configuration.
	 */
	suspend fun playIn(
		context: Context,
		file: AmigaFile,
		machine: ConfigInfo,
		loadMachine: suspend (String) -> ConfigParser.ParsedConfig,
		writeConfig: (EmulatorSettings, List<String>) -> File
	): PlayResult {
		val parsed = try {
			loadMachine(machine.path)
		} catch (_: Exception) {
			return PlayResult.Refused(R.string.msg_failed_load_config)
		}
		val settings = when (file.category) {
			FileCategory.FLOPPIES -> parsed.settings.copy(floppy0 = file.path)
			FileCategory.CD_IMAGES ->
				if (!parsed.settings.baseModel.hasCd) {
					return PlayResult.Refused(R.string.library_play_in_no_cd_drive, machine.name)
				} else {
					parsed.settings.copy(cdImage = file.path)
				}
			else -> return PlayResult.Refused(R.string.library_play_unsupported, file.name)
		}
		withContext(Dispatchers.IO) {
			val configFile = writeConfig(settings, parsed.unknownLines)
			EmulatorLauncher.launchWithConfig(
				context,
				configFile.absolutePath,
				rememberedControls = AppPreferences.getInstance(context).rememberedAndroidControls()
			)
		}
		return PlayResult.Started
	}
}
