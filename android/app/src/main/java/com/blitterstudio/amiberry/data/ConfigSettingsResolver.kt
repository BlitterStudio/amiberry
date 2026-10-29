package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import java.io.File

/** Applies native global defaults when Kotlin adopts a parsed machine configuration. */
object ConfigSettingsResolver {
	/**
	 * The settings without a configuration: the A500 preset's machine at its
	 * default compatibility level (so it reads as that level, not as edited),
	 * with the global shader.
	 */
	fun defaults(globalSettingsFile: File?): EmulatorSettings =
		EmulatorSettings.fromModel(AmigaModel.A500).copy(shader = ShaderCatalog.resolveGlobalShader(globalSettingsFile))

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
