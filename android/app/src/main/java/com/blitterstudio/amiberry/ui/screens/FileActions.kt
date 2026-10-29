package com.blitterstudio.amiberry.ui.screens

import android.content.Context
import com.blitterstudio.amiberry.data.AndroidLaunchConfig
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.ui.viewmodel.QuickStartViewModel
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** What a Files row offers, by category: disks go in a drive, games launch, all delete. */
enum class FileAction { InsertDf0AndStart, InsertDf1, LaunchWhdload, Delete }

internal fun fileActionsFor(category: FileCategory): List<FileAction> = when (category) {
	FileCategory.FLOPPIES -> listOf(FileAction.InsertDf0AndStart, FileAction.InsertDf1, FileAction.Delete)
	FileCategory.WHDLOAD_GAMES -> listOf(FileAction.LaunchWhdload, FileAction.Delete)
	FileCategory.ROMS,
	FileCategory.HARD_DRIVES,
	FileCategory.CD_IMAGES -> listOf(FileAction.Delete)
}

/**
 * The model a disk started from Files boots: Quick Start's model when it has a floppy
 * drive and can start, otherwise (as Quick Start's Disk mode picks) the first model with
 * a drive, one with its own Kickstart before one that would boot AROS. Null: none can start.
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
 * The Files tab's launches go through Quick Start's settings, so the disk or game is
 * also what Quick Start shows afterwards (Disk mode with DF0, WHDLoad mode with the game).
 */
internal object FileLaunch {

	/** Puts [disk] in DF1 of Quick Start's Disk mode. */
	fun insertDf1(disk: AmigaFile, settings: SettingsViewModel, quickStart: QuickStartViewModel) {
		settings.updateSettings { it.copy(floppy1 = disk.path, floppy1Type = 0) }
		quickStart.selectLaunchMode(QuickStartLaunchMode.DISK)
	}

	/** Puts [disk] in DF0 and starts Quick Start's Disk mode; false when no model can start. */
	suspend fun insertDf0AndStart(
		context: Context,
		disk: AmigaFile,
		roms: List<AmigaFile>,
		settings: SettingsViewModel,
		quickStart: QuickStartViewModel
	): Boolean {
		val model = floppyLaunchModel(
			current = settings.settings.baseModel,
			available = ModelRomAvailability.availableModels(roms),
			hasFloppy = { it.hasFloppy },
			hasKickstart = { ModelRomAvailability.hasKickstart(it, roms) }
		) ?: return false
		if (model != settings.settings.baseModel) settings.applyModel(model)
		settings.updateSettings { it.copy(floppy0 = disk.path) }
		quickStart.selectLaunchMode(QuickStartLaunchMode.DISK)
		val launchSettings = settings.settings
		val unknownLines = settings.currentUnknownLines
		val configFile = withContext(Dispatchers.IO) {
			settings.writeSettingsConfig(launchSettings, AndroidLaunchConfig.QUICKSTART_CONFIG, unknownLines)
		}
		EmulatorLauncher.launchQuickStart(
			context = context,
			model = launchSettings.baseModel,
			configPath = configFile.absolutePath,
			floppyPath = launchSettings.floppy0.takeIf { it.isNotBlank() },
			floppy1Path = launchSettings.floppy1.takeIf { it.isNotBlank() },
			cdPath = launchSettings.cdImage.takeIf { it.isNotBlank() }
		)
		return true
	}

	/** Launches [game] as Quick Start's WHDLoad mode does. */
	suspend fun launchWhdload(
		context: Context,
		game: AmigaFile,
		settings: SettingsViewModel,
		quickStart: QuickStartViewModel
	) {
		quickStart.selectWhdload(game)
		quickStart.selectLaunchMode(QuickStartLaunchMode.WHDLOAD)
		val controlSettings = settings.settings
		val prepared = withContext(Dispatchers.IO) {
			AndroidLaunchConfig.prepareWhdLoad(context, game.path, controlSettings)
		}
		EmulatorLauncher.launchWhdload(
			context = context,
			lhaPath = game.path,
			configPath = prepared.configFile.absolutePath,
			shaderOverride = prepared.shaderOverride,
			controlOverrides = prepared.controlOverrides
		)
	}
}
