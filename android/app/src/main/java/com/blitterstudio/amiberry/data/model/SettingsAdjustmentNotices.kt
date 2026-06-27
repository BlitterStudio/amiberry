package com.blitterstudio.amiberry.data.model

enum class SettingsAdjustmentNotice {
	AgaRequires68020,
	CpuRequires32BitAddressing,
	CycleExactDisabledJit,
	CycleExactDisabledImmediateBlits,
	Z3Requires32BitAddressing,
	IntegerScalingSynced,
	JitRequires68020,
	JitRequires32BitAddressing,
	OnScreenControlsRequireTouch,
	OnScreenJoystickMovedToPort1,
	InvalidInputDeviceReset
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
		if (requested.address24Bit && !applied.address24Bit && requested.cpuModel >= 68030) {
			add(SettingsAdjustmentNotice.CpuRequires32BitAddressing)
		}
		if (requested.cycleExact && requested.jitCacheSize > 0 && applied.jitCacheSize == 0) {
			add(SettingsAdjustmentNotice.CycleExactDisabledJit)
		}
		if (requested.cycleExact && requested.immediateBlits && !applied.immediateBlits) {
			add(SettingsAdjustmentNotice.CycleExactDisabledImmediateBlits)
		}
		if (requested.z3Ram > 0 && applied.z3Ram == 0 && requested.address24Bit) {
			add(SettingsAdjustmentNotice.Z3Requires32BitAddressing)
		}
		if (integerScalingWasSynced(requested, applied)) {
			add(SettingsAdjustmentNotice.IntegerScalingSynced)
		}
		if (requested.jitCacheSize > 0 && applied.jitCacheSize == 0 && !requested.cycleExact) {
			when {
				requested.cpuModel < 68020 -> add(SettingsAdjustmentNotice.JitRequires68020)
				requested.address24Bit || applied.address24Bit -> add(SettingsAdjustmentNotice.JitRequires32BitAddressing)
			}
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
