package com.blitterstudio.amiberry.data.model

/**
 * All emulator settings that the native Android UI can configure.
 * Maps to .uae config file keys used by cfgfile.cpp.
 */
data class EmulatorSettings(
	// Model base
	val baseModel: AmigaModel = AmigaModel.A500,

	// CPU
	val cpuModel: Int = 68000,
	val cpuCompatible: Boolean = true,
	val address24Bit: Boolean = true,
	val cpuSpeed: String = "real",  // "max" or "real"
	val fpuModel: Int = 0,  // 0=none, 68881, 68882
	val jitCacheSize: Int = 0,  // 0=disabled, else KB

	// Chipset
	val chipset: String = "ocs",  // ocs, ecs_agnus, ecs_denise, ecs, aga
	val immediateBlits: Boolean = false,
	val collisionLevel: String = "playfields",  // none, sprites, playfields, full
	val cycleExact: Boolean = false,
	val ntsc: Boolean = false,

	// Memory (in config file units)
	val chipRam: Int = 1,  // half-megabytes: 1=512KB, 2=1MB, 4=2MB, 8=4MB, 16=8MB
	val slowRam: Int = 2,  // 256KB units: 0=none, 2=512KB, 4=1MB
	val fastRam: Int = 0,  // megabytes
	val z3Ram: Int = 0,  // megabytes

	// ROM
	val romFile: String = "",
	val romExtFile: String = "",

	// Floppy drives
	val floppy0: String = "",
	val floppy0Type: Int = 0,  // 0=3.5"DD, 1=3.5"HD, -1=disabled
	val floppy1: String = "",
	val floppy1Type: Int = -1,
	val floppy2: String = "",
	val floppy2Type: Int = -1,
	val floppy3: String = "",
	val floppy3Type: Int = -1,

	// CD
	val cdImage: String = "",

	// Sound
	val soundOutput: String = "exact",  // none, interrupts, normal, exact
	val soundFreq: Int = 44100,
	val soundChannels: String = "stereo",  // mono, stereo, mixed

	// Display
	val gfxWidth: Int = 720,
	val gfxHeight: Int = 568,
	val correctAspect: Boolean = true,
	val autoCrop: Boolean = false,

	// Input
	val joyport0: String = "mouse",
	val joyport1: String = "joy0",
	val onScreenJoystick: Boolean = true,
	val onScreenKeyboard: Boolean = true
) {
	companion object {
		/** Create settings from an AmigaModel with sensible defaults. */
		fun fromModel(model: AmigaModel): EmulatorSettings {
			return when (model) {
				AmigaModel.A500 -> EmulatorSettings(
					baseModel = model, cpuModel = 68000, chipset = "ocs",
					chipRam = 1, slowRam = 2, fastRam = 0
				)
				AmigaModel.A500_PLUS -> EmulatorSettings(
					baseModel = model, cpuModel = 68000, chipset = "ecs",
					chipRam = 2, slowRam = 0, fastRam = 0
				)
				AmigaModel.A600 -> EmulatorSettings(
					baseModel = model, cpuModel = 68000, chipset = "ecs",
					chipRam = 4, slowRam = 0, fastRam = 0
				)
				AmigaModel.A1000 -> EmulatorSettings(
					baseModel = model, cpuModel = 68000, chipset = "ocs",
					chipRam = 1, slowRam = 0, fastRam = 0
				)
				AmigaModel.A2000 -> EmulatorSettings(
					baseModel = model, cpuModel = 68000, chipset = "ocs",
					chipRam = 1, slowRam = 2, fastRam = 0
				)
				AmigaModel.A1200 -> EmulatorSettings(
					baseModel = model, cpuModel = 68020, chipset = "aga",
					chipRam = 4, slowRam = 0, fastRam = 0, address24Bit = false
				)
				AmigaModel.A3000 -> EmulatorSettings(
					baseModel = model, cpuModel = 68030, chipset = "ecs",
					chipRam = 4, slowRam = 0, fastRam = 8, address24Bit = false
				)
				AmigaModel.A4000 -> EmulatorSettings(
					baseModel = model, cpuModel = 68040, chipset = "aga",
					chipRam = 4, slowRam = 0, fastRam = 8, address24Bit = false,
					fpuModel = 68040
				)
				AmigaModel.CD32 -> EmulatorSettings(
					baseModel = model, cpuModel = 68020, chipset = "aga",
					chipRam = 4, slowRam = 0, fastRam = 0, address24Bit = false
				)
				AmigaModel.CDTV -> EmulatorSettings(
					baseModel = model, cpuModel = 68000, chipset = "ocs",
					chipRam = 2, slowRam = 0, fastRam = 0
				)
			}
		}

		val chipRamOptions = listOf(
			1 to "512 KB",
			2 to "1 MB",
			4 to "2 MB",
			8 to "4 MB",
			16 to "8 MB"
		)

		val slowRamOptions = listOf(
			0 to "None",
			2 to "512 KB",
			4 to "1 MB",
			6 to "1.5 MB",
			7 to "1.8 MB"
		)

		val fastRamOptions = listOf(
			0 to "None",
			1 to "1 MB",
			2 to "2 MB",
			4 to "4 MB",
			8 to "8 MB"
		)

		val z3RamOptions = listOf(
			0 to "None",
			16 to "16 MB",
			32 to "32 MB",
			64 to "64 MB",
			128 to "128 MB",
			256 to "256 MB",
			512 to "512 MB"
		)

		val cpuModels = listOf(68000, 68010, 68020, 68030, 68040, 68060)

		val chipsetOptions = listOf(
			"ocs" to "OCS",
			"ecs_agnus" to "ECS Agnus",
			"ecs_denise" to "ECS Denise",
			"ecs" to "Full ECS",
			"aga" to "AGA"
		)

		val soundOutputOptions = listOf(
			"none" to "Disabled",
			"interrupts" to "Emulated (no output)",
			"normal" to "Normal",
			"exact" to "Exact"
		)
	}
}
