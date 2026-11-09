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
import com.blitterstudio.amiberry.data.AndroidControlSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import kotlinx.coroutines.CancellationException
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

	/** The drive type DF0: takes when the machine was saved without one. */
	private const val FLOPPY_DRIVE_TYPE = 0

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
		return launchQuickPlay(context, model, roms, floppy0 = disk.path)
	}

	/** A CD in a CD32 (a CDTV when only its Kickstart is installed). */
	private suspend fun playCd(context: Context, cd: AmigaFile, roms: List<AmigaFile>): PlayResult {
		val model = listOf(AmigaModel.CD32, AmigaModel.CDTV)
			.firstOrNull { ModelRomAvailability.isModelAvailable(it, roms) }
			?: return PlayResult.Refused(R.string.library_cd_needs_rom)
		return launchQuickPlay(context, model, roms, cdImage = cd.path)
	}

	/** A WHDLoad archive through the WHDBooter path: it decides the hardware at start. */
	suspend fun playWhdload(context: Context, game: AmigaFile): PlayResult =
		playWhdloadPath(context, game.path)

	private suspend fun playWhdloadPath(context: Context, path: String): PlayResult {
		val prepared = withContext(Dispatchers.IO) {
			try {
				AndroidLaunchConfig.prepareWhdLoad(
					context,
					path,
					AndroidLaunchConfig.loadLastSessionSettings(context)
				)
			} catch (e: CancellationException) {
				throw e
			} catch (_: Exception) {
				null
			}
		} ?: return PlayResult.Refused(R.string.msg_failed_save_config)
		EmulatorLauncher.launchWhdload(
			context = context,
			lhaPath = path,
			configPath = prepared.configFile.absolutePath,
			shaderOverride = prepared.shaderOverride,
			controlOverrides = prepared.controlOverrides
		)
		return PlayResult.Started
	}

	/** A recent entry again, as it was launched. */
	suspend fun playRecent(
		context: Context,
		entry: JSONObject,
		roms: List<AmigaFile>,
		loadMachine: suspend (String) -> ConfigParser.ParsedConfig,
		writeConfig: (EmulatorSettings, List<String>) -> File
	): PlayResult {
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
			// The machine plays the media again, through the same path a Play In takes
			// (its hardware, hard drives and other disks with the file in its drive).
			RecentLaunches.Type.PlayIn -> {
				val request = recentPlayIn(entry)
					?: return PlayResult.Refused(R.string.recent_type_unknown)
				val machine = ConfigInfo(
					path = request.machinePath,
					name = File(request.machinePath).nameWithoutExtension,
					lastModified = 0,
					description = ""
				)
				if (!withContext(Dispatchers.IO) { File(request.machinePath).exists() }) {
					return PlayResult.Refused(R.string.library_play_in_machine_gone, machine.name)
				}
				playIn(context, request.media, machine, loadMachine, writeConfig)
			}
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
		return launchQuickPlay(context, model, roms, floppy0, floppy1, cdImage)
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
	): PlayResult {
		val configFile = withContext(Dispatchers.IO) {
			try {
				val settings = recentQuickStartSettingsSnapshot(
					model = model,
					roms = roms,
					currentSettings = AndroidLaunchConfig.loadLastSessionSettings(context),
					floppy0 = floppy0,
					floppy1 = floppy1,
					cdImage = cdImage
				)
				ConfigGenerator.writeConfig(context, settings, AndroidLaunchConfig.QUICKSTART_CONFIG)
			} catch (e: CancellationException) {
				throw e
			} catch (_: Exception) {
				null
			}
		} ?: return PlayResult.Refused(R.string.msg_failed_save_config)
		withContext(Dispatchers.IO) {
			EmulatorLauncher.launchQuickStart(
				context = context,
				model = model,
				configPath = configFile.absolutePath,
				floppyPath = floppy0.takeIf { it.isNotBlank() },
				floppy1Path = floppy1.takeIf { it.isNotBlank() },
				cdPath = cdImage.takeIf { it.isNotBlank() }
			)
		}
		return PlayResult.Started
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

	/** What a Play In launches, or why it is refused. */
	internal sealed interface PlayInOutcome {
		data class Launch(val settings: EmulatorSettings, val unknownLines: List<String>) : PlayInOutcome
		data class Refused(val messageRes: Int, val argument: String? = null) : PlayInOutcome
	}

	/**
	 * The machine [file] plays in: its settings, the file in DF0: (whose drive a machine
	 * saved without one gets back, [driveType]) or its CD drive, and the remembered
	 * Android controls ([fallbackControls]) filling the groups the machine leaves unset —
	 * or the refusal: a disk needs a model with a floppy drive, a CD the model's CD drive.
	 */
	internal fun playInLaunchSettings(
		parsed: ConfigParser.ParsedConfig,
		file: AmigaFile,
		fallbackControls: EmulatorSettings,
		driveType: Int = FLOPPY_DRIVE_TYPE
	): PlayInOutcome {
		val model = parsed.settings.baseModel
		val settings = when (file.category) {
			FileCategory.FLOPPIES -> {
				if (!model.hasFloppy) {
					return PlayInOutcome.Refused(R.string.library_play_in_no_floppy_drive, model.displayName)
				}
				parsed.settings.copy(
					floppy0 = file.path,
					floppy0Type = parsed.settings.floppy0Type.takeIf { it != -1 } ?: driveType
				)
			}
			FileCategory.CD_IMAGES -> {
				if (!model.hasCd) {
					return PlayInOutcome.Refused(R.string.library_play_in_no_cd_drive, model.displayName)
				}
				parsed.settings.copy(cdImage = file.path)
			}
			else -> return PlayInOutcome.Refused(R.string.library_play_unsupported, file.name)
		}
		return PlayInOutcome.Launch(
			AndroidControlSettings.withFallback(settings, parsed.explicitKeys, fallbackControls),
			parsed.unknownLines
		)
	}

	/**
	 * Plays [file] in [machine]: loads the machine, puts the file in DF0: or its CD drive
	 * and starts it. [loadMachine] reads the machine's file; [writeConfig] writes the
	 * launch configuration.
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
		} catch (e: CancellationException) {
			throw e
		} catch (_: Exception) {
			return PlayResult.Refused(R.string.msg_failed_load_config)
		}
		val launch = when (val outcome = playInLaunchSettings(
			parsed = parsed,
			file = file,
			fallbackControls = AppPreferences.getInstance(context).rememberedAndroidControls()
		)) {
			is PlayInOutcome.Launch -> outcome
			is PlayInOutcome.Refused ->
				return PlayResult.Refused(outcome.messageRes, outcome.argument)
		}
		val configFile = withContext(Dispatchers.IO) {
			try {
				writeConfig(launch.settings, launch.unknownLines)
			} catch (e: CancellationException) {
				throw e
			} catch (_: Exception) {
				null
			}
		} ?: return PlayResult.Refused(R.string.msg_failed_save_config)
		withContext(Dispatchers.IO) {
			EmulatorLauncher.launchWithConfig(context, configFile.absolutePath)
		}
		// The launch recorded the transient configuration; Recent keeps the machine it
		// played with the media in its drive, which replays stably however many follow.
		val preferences = AppPreferences.getInstance(context)
		preferences.addRecentLaunch(playInRecentEntry(machine, file))
		preferences.removeRecentLaunch(
			JSONObject().put("type", "config").put("path", configFile.absolutePath)
		)
		return PlayResult.Started
	}

	/** The Recent entry a Play In leaves: the machine it played, with the media in its drive. */
	internal fun playInRecentEntry(machine: ConfigInfo, file: AmigaFile): JSONObject =
		JSONObject().apply {
			put("type", "playin")
			put("path", machine.path)
			when (file.category) {
				FileCategory.FLOPPIES -> put("df0", file.path)
				FileCategory.CD_IMAGES -> put("cd", file.path)
				else -> Unit
			}
		}

	/** What a Play In Recent entry replays: the saved machine and the media it played. */
	internal data class PlayInRequest(val machinePath: String, val media: AmigaFile)

	/** The machine and media a Play In entry names; null when the entry holds neither. */
	internal fun recentPlayIn(entry: JSONObject): PlayInRequest? {
		val machinePath = entry.optString("path")
		if (machinePath.isBlank()) return null
		val mediaPath = entry.optString("df0").takeIf { it.isNotBlank() }
			?: entry.optString("cd").takeIf { it.isNotBlank() }
			?: return null
		val category = if (entry.has("cd")) FileCategory.CD_IMAGES else FileCategory.FLOPPIES
		return PlayInRequest(
			machinePath = machinePath,
			media = AmigaFile(
				path = mediaPath,
				name = mediaPath.substringAfterLast('/').substringAfterLast('\\'),
				extension = mediaPath.substringAfterLast('.').lowercase(),
				size = 0,
				lastModified = 0,
				category = category
			)
		)
	}
}
