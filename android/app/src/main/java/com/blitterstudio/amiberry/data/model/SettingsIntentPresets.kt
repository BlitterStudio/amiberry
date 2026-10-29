package com.blitterstudio.amiberry.data.model

/**
 * The launcher's presets for its own settings. How compatible or fast the
 * machine is comes from the model preset's compatibility levels instead
 * (EmulatorSettings.withCompatibilityLevel).
 */
enum class SettingsIntentPreset {
	PixelPerfect,
	TouchControls
}

object SettingsIntentPresets {
	fun apply(settings: EmulatorSettings, preset: SettingsIntentPreset): EmulatorSettings {
		return when (preset) {
			SettingsIntentPreset.PixelPerfect -> settings.copy(
				correctAspect = true,
				autoCrop = false,
				scalingMethod = 2,
				gfxAutoresolution = 1
			)
			SettingsIntentPreset.TouchControls -> settings.copy(
				joyport1 = "onscreen_joy",
				onScreenJoystick = true,
				onScreenKeyboardButton = true
			)
		}
	}

	fun matches(settings: EmulatorSettings, preset: SettingsIntentPreset): Boolean =
		apply(settings, preset) == settings
}
