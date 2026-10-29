package com.blitterstudio.amiberry.data.model

/** One memory the machine has: the option, its stored value and its size. */
data class InstalledMemory(val id: String, val value: String, val kb: Int)

/**
 * The memory a configuration of [settings] gives the machine, one entry
 * per memory option in use, in the layer's order: chip RAM always, then
 * every board with a size. Chip and slow RAM are stored in 512 KB and
 * 256 KB units (chipRamKb), board sizes in MB, and boards below 1 MB in
 * KB under the *_size_k key. The launcher writes a board's KB key after
 * its MB key and the later key sets the board, so a set KB key is the
 * board's size.
 */
fun installedMemory(settings: EmulatorSettings, schema: SettingsSchema): List<InstalledMemory> =
	schema.options.values.filter { it.group == "memory" }.mapNotNull { option ->
		val id = option.id
		val value = settings.optionValue(id) ?: return@mapNotNull null
		val number = value.toIntOrNull() ?: return@mapNotNull null
		val kb = when {
			id == "chipmem_size" -> EmulatorSettings.chipRamKb(number)
			id == "bogomem_size" -> number * 256
			id.endsWith("_size_k") -> number
			id.endsWith("_size") && settings.optionValue("${id}_k") != null -> 0
			id.endsWith("_size") -> number * 1024
			else -> 0
		}
		InstalledMemory(id, value, kb).takeIf { kb > 0 || id == "chipmem_size" }
	}
