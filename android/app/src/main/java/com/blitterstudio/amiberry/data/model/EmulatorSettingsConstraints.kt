package com.blitterstudio.amiberry.data.model

object EmulatorSettingsConstraints {

	fun apply(settings: EmulatorSettings, hasTouchScreen: Boolean): EmulatorSettings {
		var result = settings

		if (result.chipset == "aga" && result.cpuModel < 68020) {
			result = result.copy(cpuModel = 68020)
		}

		// 68000/68010 must use 24-bit addressing
		if (result.cpuModel <= 68010) {
			result = result.copy(address24Bit = true, z3Ram = 0, jitCacheSize = 0)
		}

		// Cycle-exact forces real speed and disables JIT
		if (result.cycleExact) {
			result = result.copy(cpuSpeed = "real", jitCacheSize = 0, immediateBlits = false)
		}

		// Z3 RAM requires 32-bit addressing
		if (result.address24Bit && result.z3Ram > 0) {
			result = result.copy(z3Ram = 0)
		}

		if (result.gfxWidth <= 0 || result.gfxHeight <= 0) {
			result = result.copy(gfxWidth = DEFAULT_GFX_WIDTH, gfxHeight = DEFAULT_GFX_HEIGHT)
		}

		result = result.copy(
			soundOutput = normalizeSoundOutput(result.soundOutput),
			soundFreq = if (result.soundFreq > 0) result.soundFreq else DEFAULT_SOUND_FREQUENCY,
			soundChannels = normalizeSoundChannels(result.soundChannels)
		)

		val floppy0 = normalizeFloppyDrive(result.floppy0, result.floppy0Type)
		val floppy1 = normalizeFloppyDrive(result.floppy1, result.floppy1Type)
		val floppy2 = normalizeFloppyDrive(result.floppy2, result.floppy2Type)
		val floppy3 = normalizeFloppyDrive(result.floppy3, result.floppy3Type)
		result = result.copy(
			floppy0 = floppy0.path,
			floppy0Type = floppy0.driveType,
			floppy1 = floppy1.path,
			floppy1Type = floppy1.driveType,
			floppy2 = floppy2.path,
			floppy2Type = floppy2.driveType,
			floppy3 = floppy3.path,
			floppy3Type = floppy3.driveType
		)

		// 24-bit addressing disables JIT
		if (result.address24Bit) {
			result = result.copy(jitCacheSize = 0, jitFpu = false)
		}

		// JIT requires 68020+
		if (result.jitCacheSize > 0) {
			if (result.cpuModel < 68020) {
				result = result.copy(jitCacheSize = 0, jitFpu = false)
			} else {
				result = result.copy(
					cpuSpeed = "max",
					jitFpu = true,
					cpuCompatible = if (result.cpuModel >= 68040) false else result.cpuCompatible
				)
			}
		} else {
			result = result.copy(jitFpu = false)
		}

		result = result.copy(fpuModel = normalizeFpuModel(result.cpuModel, result.fpuModel))

		if (result.jitCacheSize > 0 && result.cpuModel >= 68040) {
			result = result.copy(cpuCompatible = false)
		}

		val normalizedJoyport0 = normalizeJoyport0(result.joyport0)
		if (normalizedJoyport0 != result.joyport0) {
			result = result.copy(joyport0 = normalizedJoyport0)
		}

		if (!hasTouchScreen && (result.joyport1 == "onscreen_joy" || result.onScreenJoystick)) {
			result = result.copy(joyport1 = "joy1", onScreenJoystick = false)
		} else if (result.onScreenJoystick) {
			result = result.copy(joyport1 = "onscreen_joy")
		} else if (result.joyport1 == "onscreen_joy") {
			result = result.copy(joyport1 = "joy1")
		}

		val normalizedJoyport1 = normalizeJoyport1(result.joyport1)
		if (normalizedJoyport1 != result.joyport1) {
			result = result.copy(joyport1 = normalizedJoyport1, onScreenJoystick = false)
		}

		return result
	}

	private fun normalizeFpuModel(cpuModel: Int, fpuModel: Int): Int = when {
		cpuModel <= 68010 -> 0
		cpuModel < 68040 -> when (fpuModel) {
			0 -> 0
			68882 -> 68882
			else -> 68881
		}
		else -> if (fpuModel == 0) 0 else cpuModel
	}

	private fun normalizeSoundOutput(soundOutput: String): String = when (soundOutput) {
		"none", "interrupts", "normal", "exact" -> soundOutput
		"good" -> "normal"
		"best" -> "exact"
		else -> DEFAULT_SOUND_OUTPUT
	}

	private fun normalizeSoundChannels(soundChannels: String): String =
		if (soundChannels in NATIVE_SOUND_CHANNELS) soundChannels else DEFAULT_SOUND_CHANNELS

	private fun normalizeFloppyDrive(path: String, driveType: Int): NormalizedFloppyDrive = when (driveType) {
		FLOPPY_TYPE_DISABLED -> NormalizedFloppyDrive(path = "", driveType = FLOPPY_TYPE_DISABLED)
		FLOPPY_TYPE_DD, FLOPPY_TYPE_HD -> NormalizedFloppyDrive(path = path, driveType = driveType)
		else -> if (path.isBlank()) {
			NormalizedFloppyDrive(path = "", driveType = FLOPPY_TYPE_DISABLED)
		} else {
			NormalizedFloppyDrive(path = path, driveType = FLOPPY_TYPE_DD)
		}
	}

	private fun normalizeJoyport0(joyport0: String): String =
		if (joyport0 in ANDROID_PORT_INPUT_DEVICES) joyport0 else DEFAULT_JOYPORT0

	private fun normalizeJoyport1(joyport1: String): String =
		if (joyport1 in ANDROID_PORT_INPUT_DEVICES || joyport1 == "onscreen_joy") joyport1 else DEFAULT_JOYPORT1

	private const val DEFAULT_GFX_WIDTH = 720
	private const val DEFAULT_GFX_HEIGHT = 568
	private const val DEFAULT_SOUND_OUTPUT = "exact"
	private const val DEFAULT_SOUND_FREQUENCY = 44100
	private const val DEFAULT_SOUND_CHANNELS = "stereo"
	private const val DEFAULT_JOYPORT0 = "mouse"
	private const val DEFAULT_JOYPORT1 = "joy1"
	private const val FLOPPY_TYPE_DISABLED = -1
	private const val FLOPPY_TYPE_DD = 0
	private const val FLOPPY_TYPE_HD = 1
	private val ANDROID_PORT_INPUT_DEVICES = setOf(
		"none",
		"mouse",
		"joy0",
		"joy1",
		"kbd1",
		"kbd2",
		"kbd3",
		"kbd9"
	)
	private val NATIVE_SOUND_CHANNELS = EmulatorSettings.soundChannelOptions.map { it.first }.toSet()

	private data class NormalizedFloppyDrive(
		val path: String,
		val driveType: Int
	)
}
