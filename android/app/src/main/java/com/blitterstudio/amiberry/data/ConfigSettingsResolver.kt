package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.EmulatorSettings
import java.io.File

/** Applies native global defaults when Kotlin adopts a parsed machine configuration. */
object ConfigSettingsResolver {
	fun defaults(globalSettingsFile: File?): EmulatorSettings =
		EmulatorSettings(shader = ShaderCatalog.resolveGlobalShader(globalSettingsFile))

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
