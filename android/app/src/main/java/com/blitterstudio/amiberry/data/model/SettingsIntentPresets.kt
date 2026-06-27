package com.blitterstudio.amiberry.data.model

enum class SettingsIntentPreset {
	Compatibility,
	Balanced,
	Performance,
	PixelPerfect,
	TouchControls
}

object SettingsIntentPresets {
	fun apply(settings: EmulatorSettings, preset: SettingsIntentPreset): EmulatorSettings {
		return when (preset) {
			SettingsIntentPreset.Compatibility -> settings.copy(
				cpuSpeed = "real",
				cpuCompatible = true,
				cycleExact = true,
				immediateBlits = false,
				jitCacheSize = 0,
				jitFpu = false
			)
			SettingsIntentPreset.Balanced -> settings.copy(
				cpuSpeed = "real",
				cpuCompatible = true,
				cycleExact = false,
				immediateBlits = false,
				jitCacheSize = 0,
				jitFpu = false,
				correctAspect = true
			)
			SettingsIntentPreset.Performance -> {
				val jitAllowed = settings.cpuModel >= 68020 && !settings.address24Bit
				settings.copy(
					cpuSpeed = "max",
					cpuCompatible = if (settings.cpuModel >= 68040) false else settings.cpuCompatible,
					cycleExact = false,
					immediateBlits = true,
					jitCacheSize = if (jitAllowed) 16384 else 0,
					jitFpu = jitAllowed
				)
			}
			SettingsIntentPreset.PixelPerfect -> settings.copy(
				correctAspect = true,
				autoCrop = false,
				scalingMethod = 2,
				gfxAutoresolution = 1
			)
			SettingsIntentPreset.TouchControls -> settings.copy(
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboard = true
			)
		}
	}
}
