package com.blitterstudio.amiberry.data.model

data class SettingsChange(
	val label: String,
	val before: String,
	val after: String
)

object SettingsChangeSummary {
	fun diff(before: EmulatorSettings, after: EmulatorSettings): List<SettingsChange> = buildList {
		addIfChanged("Model", before.baseModel.displayName, after.baseModel.displayName)
		addIfChanged("CPU", before.cpuModel.toString(), after.cpuModel.toString())
		addIfChanged("Addressing", addressing(before.address24Bit), addressing(after.address24Bit))
		addIfChanged("CPU speed", before.cpuSpeed, after.cpuSpeed)
		addIfChanged("Compatible CPU", yesNo(before.cpuCompatible), yesNo(after.cpuCompatible))
		addIfChanged("JIT cache", cache(before.jitCacheSize), cache(after.jitCacheSize))
		addIfChanged("Chipset", chipset(before.chipset), chipset(after.chipset))
		addIfChanged("Cycle exact", yesNo(before.cycleExact), yesNo(after.cycleExact))
		addIfChanged("Chip RAM", chipRam(before.chipRam), chipRam(after.chipRam))
		addIfChanged("Slow RAM", slowRam(before.slowRam), slowRam(after.slowRam))
		addIfChanged("Fast RAM", mb(before.fastRam), mb(after.fastRam))
		addIfChanged("Z3 RAM", mb(before.z3Ram), mb(after.z3Ram))
		addIfChanged("ROM", fileName(before.romFile), fileName(after.romFile))
		addIfChanged("DF0", fileName(before.floppy0), fileName(after.floppy0))
		addIfChanged("DF1", fileName(before.floppy1), fileName(after.floppy1))
		addIfChanged("CD", fileName(before.cdImage), fileName(after.cdImage))
		addIfChanged("Scaling", scaling(before.scalingMethod), scaling(after.scalingMethod))
		addIfChanged("Resolution autoswitch", autoresolution(before.gfxAutoresolution), autoresolution(after.gfxAutoresolution))
		addIfChanged("Joy port 0", before.joyport0, after.joyport0)
		addIfChanged("Joy port 1", before.joyport1, after.joyport1)
		addIfChanged("On-screen joystick", yesNo(before.onScreenJoystick), yesNo(after.onScreenJoystick))
		addIfChanged("On-screen keyboard", yesNo(before.onScreenKeyboard), yesNo(after.onScreenKeyboard))
		addIfChanged("On-screen keyboard numpad", yesNo(before.onScreenKeyboardNumpad), yesNo(after.onScreenKeyboardNumpad))
	}

	private fun MutableList<SettingsChange>.addIfChanged(label: String, before: String, after: String) {
		if (before != after) {
			add(SettingsChange(label, before, after))
		}
	}

	private fun addressing(address24Bit: Boolean): String =
		if (address24Bit) "24-bit" else "32-bit"

	private fun yesNo(value: Boolean): String =
		if (value) "Yes" else "No"

	private fun cache(value: Int): String =
		if (value <= 0) "Off" else "$value KB"

	private fun chipset(value: String): String =
		EmulatorSettings.chipsetOptions.firstOrNull { it.first == value }?.second ?: value.uppercase()

	private fun chipRam(value: Int): String =
		EmulatorSettings.chipRamOptions.firstOrNull { it.first == value }?.second ?: "${value * 512} KB"

	private fun slowRam(value: Int): String =
		EmulatorSettings.slowRamOptions.firstOrNull { it.first == value }?.second ?: "${value * 256} KB"

	private fun mb(value: Int): String =
		if (value <= 0) "None" else "$value MB"

	private fun fileName(path: String): String =
		path.substringAfterLast('/').substringAfterLast('\\').ifBlank { "None" }

	private fun scaling(value: Int): String = when (value) {
		-1 -> "Auto"
		0 -> "Nearest"
		1 -> "Linear"
		2 -> "Integer"
		else -> value.toString()
	}

	private fun autoresolution(value: Int): String = when (value) {
		0 -> "Disabled"
		1 -> "Always on"
		else -> "$value"
	}
}
