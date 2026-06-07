package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.EmulatorSettings

object QuickStartMediaState {

	fun pruneMissingMedia(
		settings: EmulatorSettings,
		floppies: List<AmigaFile>,
		cds: List<AmigaFile>,
		isScanning: Boolean,
		fileExists: (String) -> Boolean = { false }
	): EmulatorSettings {
		if (isScanning) return settings

		val floppyPaths = floppies.mapTo(mutableSetOf()) { it.path }
		val cdPaths = cds.mapTo(mutableSetOf()) { it.path }
		var updated = settings

		if (shouldClear(settings.floppy0, floppyPaths, fileExists)) {
			updated = updated.copy(floppy0 = "")
		}
		if (shouldClear(settings.floppy1, floppyPaths, fileExists)) {
			updated = updated.copy(floppy1 = "", floppy1Type = -1)
		}
		if (shouldClear(settings.cdImage, cdPaths, fileExists)) {
			updated = updated.copy(cdImage = "")
		}

		return if (updated == settings) settings else updated
	}

	private fun shouldClear(
		selectedPath: String,
		availablePaths: Set<String>,
		fileExists: (String) -> Boolean
	): Boolean =
		selectedPath.isNotBlank() &&
			availablePaths.isNotEmpty() &&
			selectedPath !in availablePaths &&
			!fileExists(selectedPath)
}
