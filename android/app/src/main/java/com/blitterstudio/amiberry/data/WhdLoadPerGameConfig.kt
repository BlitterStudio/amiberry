package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import java.io.File

/**
 * A WHDLoad game's own configuration: `<game>.uae` in the configuration
 * directory. WHDBooter loads it at start instead of choosing the hardware
 * itself. The launcher reads only the keys it owns (the shader and the
 * controls): the ones the file sets are the game's, the rest come from the
 * launcher's current choice.
 */
object WhdLoadPerGameConfig {

	/** Where WHDBooter looks for the game's configuration; null without a configuration directory. */
	internal fun file(globalSettingsFile: File?, externalFilesDir: File?, lhaFile: File): File? =
		ShaderCatalog.resolveConfigurationRoot(globalSettingsFile, externalFilesDir)?.let {
			File(it, "${lhaFile.nameWithoutExtension}.uae")
		}

	/** The game's configuration file; null when it has none. */
	fun existingFile(context: Context, lhaPath: String): File? {
		val externalFilesDir = context.getExternalFilesDir(null)
		return file(
			globalSettingsFile = ShaderCatalog.findSettingsFile(context.filesDir, externalFilesDir),
			externalFilesDir = externalFilesDir,
			lhaFile = File(lhaPath)
		)?.takeIf { it.isFile }
	}

	/** What a WHDLoad launch passes besides the game. */
	data class Launch(
		/** The launcher-owned values the game starts with: its shader and controls. */
		val settings: EmulatorSettings,
		/**
		 * The controls passed after `--autoload`, where they replace the ports
		 * WHDBooter assigns; only the groups the game's configuration does not
		 * set. Null when it sets them all.
		 */
		val controls: LaunchRequest.AndroidControlOverrides?
	)

	/** The launch for a game whose configuration is [perGameConfig] (null or missing: none), from [current]. */
	fun launch(perGameConfig: File?, current: EmulatorSettings): Launch {
		val parsed = perGameConfig?.takeIf { it.isFile }?.let(ConfigParser::parse)
			?: return Launch(current, LaunchRequest.AndroidControlOverrides.fromSettings(current))
		val shader = if ("amiberry.shader" in parsed.explicitKeys) parsed.settings.shader else current.shader
		return Launch(
			settings = AndroidControlSettings.withFallback(parsed.settings, parsed.explicitKeys, current)
				.copy(shader = shader),
			controls = LaunchRequest.AndroidControlOverrides.missingFrom(parsed, current)
		)
	}
}
