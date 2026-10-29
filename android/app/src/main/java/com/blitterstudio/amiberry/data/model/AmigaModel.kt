package com.blitterstudio.amiberry.data.model

/**
 * An Amiga model the Quick Start screen offers: one of the shared settings
 * layer's machine presets (the ImGui Quickstart's), read from the bundled
 * settings schema. Its defaults are the first configuration's at the
 * default compatibility level; the other configurations are used when only
 * their ROMs are installed (see ModelRomAvailability). New presets in the
 * emulator show up here once the schema is regenerated.
 */
data class AmigaModel(
	/** The layer's stable preset ID ("a500"); what recent launches store. */
	val cmdArg: String,
	val displayName: String,
	/** The preset's configurations, in the emulator's order; never empty. */
	val configs: List<SettingsSchema.PresetConfig>,
	/** The highest compatibility level (0 = the model has one level). */
	val compatibilityMax: Int = 0
) {
	/** The first configuration's label, for example "1.3 ROM, OCS, 512 KB Chip + 512 KB Slow RAM". */
	val description: String
		get() = configs.first().label

	/**
	 * The level a model starts at: approximate (1) where the model has it,
	 * as --apply-preset and the native front ends pick.
	 */
	val defaultLevel: Int
		get() = minOf(1, compatibilityMax)

	/** The configuration index clamped to the model's configurations. */
	fun configIndex(config: Int): Int = config.coerceIn(0, configs.size - 1)

	/** The level clamped to the model's levels. */
	fun levelIndex(level: Int): Int = level.coerceIn(0, compatibilityMax)

	/** The options configuration [config] sets at [level], .uae spelling (see SettingsSchema.PresetConfig). */
	fun levelValues(config: Int, level: Int): Map<String, String> {
		val levels = configs[configIndex(config)].levels
		return levels.getOrNull(levelIndex(level)) ?: levels.lastOrNull().orEmpty()
	}

	/** The first configuration's value of [id] at the default level; the option's default where it sets none. */
	private fun value(id: String): String? =
		levelValues(0, defaultLevel)[id] ?: SettingsSchema.current?.option(id)?.defaultOnAndroid

	val hasFloppy: Boolean
		get() = value("floppy0type")?.let { it != "-1" } ?: true

	val hasCd: Boolean
		get() = value("cd32cd") == "true" || value("cdtvcd") == "true"

	val chipset: String
		get() = value("chipset").orEmpty()

	val defaultCpu: String
		get() = value("cpu_model").orEmpty()

	override fun toString(): String = cmdArg

	companion object {
		/** Every preset the bundled schema has, in the emulator's order. */
		val entries: List<AmigaModel>
			get() = SettingsSchema.current?.models.orEmpty()

		/**
		 * The model with this preset ID, or with an ID the launcher stored
		 * before it used presets ("A500", "A500P"). A2000 has no preset; it
		 * reads as the A500 it was configured like.
		 */
		fun find(id: String?): AmigaModel? {
			if (id.isNullOrBlank()) return null
			val key = LEGACY_IDS[id] ?: id.lowercase()
			return entries.firstOrNull { it.cmdArg == key }
		}

		private val LEGACY_IDS = mapOf(
			"A500" to "a500",
			"A500P" to "a500plus",
			"A600" to "a600",
			"A1000" to "a1000",
			"A2000" to "a500",
			"A1200" to "a1200",
			"A3000" to "a3000",
			"A4000" to "a4000",
			"CD32" to "cd32",
			"CDTV" to "cdtv"
		)

		private fun require(id: String): AmigaModel =
			find(id) ?: error("The bundled settings schema has no '$id' preset")

		// The presets the launcher refers to by name.
		val A500: AmigaModel get() = require("a500")
		val A500_PLUS: AmigaModel get() = require("a500plus")
		val A600: AmigaModel get() = require("a600")
		val A1000: AmigaModel get() = require("a1000")
		val A1200: AmigaModel get() = require("a1200")
		val A3000: AmigaModel get() = require("a3000")
		val A4000: AmigaModel get() = require("a4000")
		val CD32: AmigaModel get() = require("cd32")
		val CDTV: AmigaModel get() = require("cdtv")
	}
}
