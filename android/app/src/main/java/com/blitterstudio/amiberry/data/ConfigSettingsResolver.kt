package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.SettingsSchema
import java.io.File

/** Applies native global defaults when Kotlin adopts a parsed machine configuration. */
object ConfigSettingsResolver {
	/**
	 * The settings without a configuration: the A500 preset's machine at its
	 * default compatibility level (so it reads as that level, not as edited),
	 * with the host defaults amiberry.conf gives a fresh configuration (the
	 * schema's sets_option mapping — target_default_options' copies: the
	 * global shader, sound, display and port defaults). The launcher writes
	 * those options itself, so without this a new machine would override
	 * them with the built-ins.
	 */
	fun defaults(globalSettingsFile: File?): EmulatorSettings {
		var settings = EmulatorSettings.fromModel(AmigaModel.A500)
			.copy(shader = ShaderCatalog.resolveGlobalShader(globalSettingsFile))
		val schema = SettingsSchema.current ?: return settings
		for ((option, value) in HostSettingsStore.machineOptionDefaults(globalSettingsFile, schema)) {
			settings = settings.withOption(option, value, schema)
		}
		return settings
	}

	fun parse(
		configFile: File,
		globalSettingsFile: File?
	): ConfigParser.ParsedConfig {
		val parsed = ConfigParser.parse(configFile)
		if ("amiberry.shader" in parsed.explicitKeys) return parsed

		return parsed.copy(
			settings = parsed.settings.copy(
				shader = ShaderCatalog.resolveGlobalShader(globalSettingsFile)
			)
		)
	}
}
