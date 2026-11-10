package com.blitterstudio.amiberry.data

import android.content.Context
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.StoragePaths
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import java.io.File

/**
 * The folders the launcher's repositories use, each the one the matching
 * amiberry.conf path setting resolves to — the emulator's own resolution
 * ([GlobalSettingsPaths], the same resolver the shader and bezel catalogs
 * read), so the launcher and the emulator list, import and write to the
 * same folders. A folder the file does not name resolves to its default
 * under the base content root, which on Android is the app's own storage.
 */
object ContentFolders {

	/** The amiberry.conf path setting each media category is stored in. */
	fun categoryKey(category: FileCategory): String = when (category) {
		FileCategory.ROMS -> "rom_path"
		FileCategory.FLOPPIES -> "floppy_path"
		FileCategory.HARD_DRIVES -> "harddrive_path"
		FileCategory.CD_IMAGES -> "cdrom_path"
		FileCategory.WHDLOAD_GAMES -> "whdload_arch_path"
	}

	/**
	 * Rises whenever a write may have changed a folder: the Library and the
	 * pickers rescan, so a folder change reaches them without a restart.
	 */
	private val _changes = MutableStateFlow(0L)

	/** See [_changes]. */
	val changes: StateFlow<Long> = _changes.asStateFlow()

	/** The launcher wrote a path setting; the repositories follow. */
	fun notifyChanged() {
		_changes.value = _changes.value + 1
	}

	/** One read of the settings, reused while the file's stamp stands. */
	private var cached: CachedResolver? = null

	private class CachedResolver(
		val settingsFile: File?,
		val stamp: Long,
		val externalFilesDir: File?,
		val resolver: GlobalSettingsPaths.Resolver
	)

	private fun stampOf(file: File?): Long = try {
		file?.let { it.lastModified() xor (it.length() shl 21) } ?: 0L
	} catch (_: Exception) {
		-1L
	}

	private fun resolver(globalSettingsFile: File?, externalFilesDir: File?): GlobalSettingsPaths.Resolver {
		val stamp = stampOf(globalSettingsFile)
		val cachedResolver = cached
		if (cachedResolver != null &&
			cachedResolver.settingsFile == globalSettingsFile &&
			cachedResolver.stamp == stamp &&
			cachedResolver.externalFilesDir == externalFilesDir
		) {
			return cachedResolver.resolver
		}
		val resolver = GlobalSettingsPaths.Resolver.of(globalSettingsFile, externalFilesDir)
		cached = CachedResolver(globalSettingsFile, stamp, externalFilesDir, resolver)
		return resolver
	}

	/** The folder [key] resolves to, or its default under the base content root. */
	fun effective(key: String, globalSettingsFile: File?, externalFilesDir: File?): File? =
		resolver(globalSettingsFile, externalFilesDir).roots(key).configuredRoot
			?: default(key, globalSettingsFile, externalFilesDir)

	/**
	 * The base content root's own [key] folder: the default a reset returns
	 * to. The RetroArch config's default is the configuration folder's
	 * `retroarch.cfg` (init_amiberry_dirs); the floppy sounds and the
	 * plugins override have none of the launcher's to name.
	 */
	fun default(key: String, globalSettingsFile: File?, externalFilesDir: File?): File? {
		val suffix = GlobalSettingsPaths.managedPathSuffixes[key]
		if (suffix != null) {
			return externalFilesDir?.let { base -> if (suffix.isEmpty()) base else File(base, suffix) }
		}
		if (key == "retroarch_config") {
			return GlobalSettingsPaths.resolveConfigurationRoot(globalSettingsFile, externalFilesDir)
				?.let { File(it, "retroarch.cfg") }
		}
		return null
	}

	/** The base content root in effect (the emulator's working root for relative paths). */
	fun baseRoot(globalSettingsFile: File?, externalFilesDir: File?): File? =
		resolver(globalSettingsFile, externalFilesDir).baseRoot()

	/** The folder a media category's files are in, following its path setting. */
	fun categoryDir(context: Context, category: FileCategory): File =
		effective(
			categoryKey(category),
			ShaderCatalog.findSettingsFile(context.filesDir, context.getExternalFilesDir(null)),
			context.getExternalFilesDir(null)
		) ?: File(context.getExternalFilesDir(null) ?: context.filesDir, category.dirName)

	/** The user configurations directory, following `config_path`. */
	fun configurationsDir(context: Context): File =
		ShaderCatalog.resolveConfigurationRoot(
			ShaderCatalog.findSettingsFile(context.filesDir, context.getExternalFilesDir(null)),
			context.getExternalFilesDir(null)
		) ?: File(context.getExternalFilesDir(null) ?: context.filesDir, StoragePaths.CONFIGURATIONS)

	/** The WHDBooter kickstarts folder, inside the WHDBoot folder in effect. */
	fun kickstartsDir(globalSettingsFile: File?, externalFilesDir: File?): File? =
		effective("whdboot_path", globalSettingsFile, externalFilesDir)
			?.let { whdboot -> File(whdboot, "save-data/Kickstarts") }

	/**
	 * The folders the Library lists a category's files from: the category's
	 * folder, the base content root (users may put files there) and, for
	 * ROMs, the WHDBoot folder's kickstarts. Listing, deleting and the
	 * recursive import's exclusions all read this, so a file the Library
	 * shows is always one it may delete.
	 */
	fun libraryRoots(category: FileCategory, globalSettingsFile: File?, externalFilesDir: File?): List<File> =
		listOfNotNull(
			effective(categoryKey(category), globalSettingsFile, externalFilesDir)
				?: externalFilesDir?.let { File(it, category.dirName) },
			baseRoot(globalSettingsFile, externalFilesDir),
			if (category == FileCategory.ROMS) kickstartsDir(globalSettingsFile, externalFilesDir) else null
		)
}
