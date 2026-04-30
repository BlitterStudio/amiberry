package com.blitterstudio.amiberry.data.model

object StoragePaths {
	const val CONFIGURATIONS = "Configurations"
	const val CONTROLLERS = "Controllers"
	const val WHDBOOT = "WHDBoot"
	const val WHDLOAD_ARCHIVES = "LHA"
	const val FLOPPIES = "Floppies"
	const val HARD_DRIVES = "HardDrives"
	const val CDROMS = "CDROMs"
	const val ROMS = "ROMs"
	const val SAVE_STATES = "SaveStates"
	const val INPUT_RECORDINGS = "InputRecordings"
	const val SCREENSHOTS = "Screenshots"
	const val NVRAM = "NVRAM"
	const val VIDEOS = "Videos"
	const val VISUALS = "Visuals"
	const val THEMES = "Themes"
	const val SHADERS = "Shaders"
	const val BEZELS = "Bezels"

	const val WHDBOOT_KICKSTARTS = "$WHDBOOT/save-data/Kickstarts"

	val userContentDirs = listOf(
		ROMS,
		FLOPPIES,
		HARD_DRIVES,
		CDROMS,
		WHDLOAD_ARCHIVES,
		CONFIGURATIONS
	)

	val userModifiableAssetDirs = setOf(
		CONTROLLERS,
		WHDBOOT,
		CONFIGURATIONS
	)
}
