package com.blitterstudio.amiberry.ui.screens.appsettings

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.blitterstudio.amiberry.data.ContentFolders
import com.blitterstudio.amiberry.data.HostSettingsStore
import com.blitterstudio.amiberry.data.ShaderCatalog
import com.blitterstudio.amiberry.data.model.SettingsSchema
import java.io.File
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * App Settings' state: the host settings' values as amiberry.conf holds
 * them (over the Android defaults the schema carries), and the catalogs
 * the shader and bezel defaults pick from. A change is written to the file
 * at once, as the Apple apps' settings are; the emulator reads them at its
 * next start, so a write while it runs is refused nowhere but seen later —
 * the file is read again before every write, and the screen reloads on
 * entry and on resume, so what the emulator's own Advanced settings window
 * saved is never overwritten from a stale copy.
 */
class AppSettingsViewModel(application: Application) : AndroidViewModel(application) {
	private val app = application

	val schema: SettingsSchema?
		get() = SettingsSchema.current ?: SettingsSchema.bundled(app)

	private val store: HostSettingsStore? by lazy {
		schema?.let {
			HostSettingsStore(
				HostSettingsStore.settingsFile(app.filesDir, app.getExternalFilesDir(null)),
				it
			)
		}
	}

	/** Every host setting's value: the file's, else the Android default the schema carries. */
	var values by mutableStateOf<Map<String, String>>(emptyMap())
		private set

	/** What went wrong with the last change, as a message to show until the next one. */
	var notice by mutableStateOf<Int?>(null)
		private set

	/**
	 * The collapsed sections the user opened (AppSettingsSection.key):
	 * remembered while the settings are open, so a disclosure opened for one
	 * change stays open for the next.
	 */
	var expandedSections by mutableStateOf(emptySet<String>())
		private set

	/** Opens or closes a collapsed section; only the user's own opens are kept. */
	fun setSectionExpanded(key: String, expanded: Boolean) {
		expandedSections = if (expanded) expandedSections + key else expandedSections - key
	}

	/** The message was shown; the next change states its own. */
	fun clearNotice() {
		notice = null
	}

	/** The shader catalog (the built-ins and the shaders folder scan) for the shader defaults. */
	var shaderCatalogEntries by mutableStateOf(ShaderCatalog.BUILT_INS)
		private set
	/** The bezel files of the bezel folder, for the custom bezel default. */
	var bezelEntries by mutableStateOf<List<String>>(emptyList())
		private set

	/** One folder setting's current state, as the Folders section shows it. */
	var folderStates by mutableStateOf<Map<String, FolderState>>(emptyMap())
		private set

	/** Reads amiberry.conf and the catalogs again (the screen calls it on entry and on resume). */
	fun reload() {
		val store = store
		if (store == null) {
			values = schema?.hostDefaults.orEmpty()
			folderStates = emptyMap()
			return
		}
		viewModelScope.launch {
			val read = withContext(Dispatchers.IO) {
				val settingsFile = HostSettingsStore.settingsFile(app.filesDir, app.getExternalFilesDir(null))
				val external = app.getExternalFilesDir(null)
				val fileValues = store.read()
				// Both catalogs resolve their folder from amiberry.conf the
				// way the emulator does (ShaderCatalog's shared resolver).
				val shaders = ShaderCatalog.scan(ShaderCatalog.resolveScanRoots(settingsFile, external))
				val bezels = ShaderCatalog.scanBezels(ShaderCatalog.resolveBezelScanRoots(settingsFile, external))
				val folders = folderIds.associateWith { id ->
					val effective = ContentFolders.effective(id, settingsFile, external)
					// A file setting names a file; every other path a folder.
					val isFile = schema?.hostSetting(id)?.type == SettingsSchema.TYPE_FILE
					FolderState(
						id = id,
						stored = fileValues[id],
						effective = effective,
						exists = effective?.let { if (isFile) safeIsFile(it) else safeIsDir(it) } == true
					)
				}
				ReadResult(fileValues, shaders, bezels, folders)
			}
			values = schema?.hostDefaults.orEmpty() + read.fileValues
			shaderCatalogEntries = read.shaders
			bezelEntries = read.bezels
			folderStates = read.folders
		}
	}

	/** The folder settings the schema carries, in the layer's order. */
	private val folderIds: List<String>
		get() = schema?.hostSettings
			?.filter {
				(it.type == SettingsSchema.TYPE_PATH || it.type == SettingsSchema.TYPE_FILE) &&
					!it.hiddenOnAndroid
			}
			?.map { it.id }
			.orEmpty()

	/** Sets one host setting and saves the file; false (with a notice) when it was refused. */
	fun set(id: String, value: String) {
		val store = store
		if (store == null) {
			notice = com.blitterstudio.amiberry.R.string.appsettings_unavailable
			return
		}
		viewModelScope.launch {
			val written = withContext(Dispatchers.IO) {
				// Picking a bezel shows it instead of the built-in one, and
				// None shows none, as the layer's setter does for the panels.
				val extra = when {
					id == "custom_bezel" && value != "none" ->
						mapOf("use_custom_bezel" to "true", "use_bezel" to "false")
					id == "custom_bezel" ->
						mapOf("use_custom_bezel" to "false")
					else -> emptyMap()
				}
				store.write(mapOf(id to value) + extra)
			}
			writeFinished(id, written)
		}
	}

	/**
	 * Returns a folder setting to its default: its line leaves the file,
	 * as the emulator's own writer leaves a setting at its default.
	 */
	fun resetFolder(id: String) {
		val store = store
		if (store == null) {
			notice = com.blitterstudio.amiberry.R.string.appsettings_unavailable
			return
		}
		viewModelScope.launch {
			val cleared = withContext(Dispatchers.IO) { store.clear(listOf(id)) }
			writeFinished(id, cleared)
		}
	}

	private fun writeFinished(id: String, written: Boolean) {
		if (written) {
			// A folder change moves what the repositories list: the Library
			// and the pickers rescan at once, without a restart.
			if (id in folderIds) ContentFolders.notifyChanged()
			notice = null
			reload()
		} else {
			notice = com.blitterstudio.amiberry.R.string.appsettings_write_failed
		}
	}

	private data class ReadResult(
		val fileValues: Map<String, String>,
		val shaders: List<String>,
		val bezels: List<String>,
		val folders: Map<String, FolderState>
	)

	private fun safeIsDir(dir: File): Boolean = try {
		dir.isDirectory
	} catch (_: Exception) {
		false
	}

	private fun safeIsFile(file: File): Boolean = try {
		file.isFile
	} catch (_: Exception) {
		false
	}

}
