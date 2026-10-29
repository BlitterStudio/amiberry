package com.blitterstudio.amiberry.data.model

/**
 * A change the launcher made to a requested configuration. [optionId] names
 * the schema option whose constraint text (SettingsSchema.constraint) says
 * why, for the emulator's own rules; the launcher's rules, which the
 * emulator has no text for, have none and carry their own message.
 */
enum class SettingsAdjustmentNotice(val optionId: String?) {
	AgaRequires68020(null),
	AddressingChanged("cpu_24bit_addressing"),
	JitTurnedOff("cachesize"),
	Z3MemoryRemoved("z3mem_size"),
	SoundOutputRaised("sound_output"),
	IntegerScalingSynced(null),
	OnScreenControlsRequireTouch(null),
	OnScreenJoystickMovedToPort1(null),
	InvalidInputDeviceReset(null)
}

object SettingsAdjustmentNotices {
	fun fromAdjustment(
		requested: EmulatorSettings,
		applied: EmulatorSettings
	): List<SettingsAdjustmentNotice> = buildList {
		if (requested == applied) return@buildList

		if (requested.chipset == "aga" && requested.cpuModel < 68020 && applied.cpuModel >= 68020) {
			add(SettingsAdjustmentNotice.AgaRequires68020)
		}
		if (requested.address24Bit != applied.address24Bit) {
			add(SettingsAdjustmentNotice.AddressingChanged)
		}
		if (requested.jitCacheSize > 0 && applied.jitCacheSize == 0) {
			add(SettingsAdjustmentNotice.JitTurnedOff)
		}
		if (requested.z3Ram > 0 && applied.z3Ram == 0) {
			add(SettingsAdjustmentNotice.Z3MemoryRemoved)
		}
		if (requested.soundOutput == "none" && applied.soundOutput != "none") {
			add(SettingsAdjustmentNotice.SoundOutputRaised)
		}
		if (integerScalingWasSynced(requested, applied)) {
			add(SettingsAdjustmentNotice.IntegerScalingSynced)
		}
		if (requested.onScreenJoystick && !applied.onScreenJoystick) {
			add(SettingsAdjustmentNotice.OnScreenControlsRequireTouch)
		}
		if (requested.joyport0 == "onscreen_joy" && applied.joyport1 == "onscreen_joy") {
			add(SettingsAdjustmentNotice.OnScreenJoystickMovedToPort1)
		}
		if ((requested.joyport0 != applied.joyport0 || requested.joyport1 != applied.joyport1) &&
			requested.joyport0 != "onscreen_joy" &&
			applied.joyport1 != "onscreen_joy" &&
			!requested.onScreenJoystick
		) {
			add(SettingsAdjustmentNotice.InvalidInputDeviceReset)
		}
	}

	private fun integerScalingWasSynced(
		requested: EmulatorSettings,
		applied: EmulatorSettings
	): Boolean {
		return (requested.scalingMethod == 2 && requested.gfxAutoresolution != 1 && applied.gfxAutoresolution == 1) ||
			(requested.gfxAutoresolution == 1 && requested.scalingMethod != 2 && applied.scalingMethod == 2)
	}
}
