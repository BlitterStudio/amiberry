package com.blitterstudio.amiberry.ui.screens.appsettings

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.blitterstudio.amiberry.data.HostSettingsStore
import com.blitterstudio.amiberry.data.ShaderCatalog
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.data.model.StoragePaths
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

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

	/** The message was shown; the next change states its own. */
	fun clearNotice() {
		notice = null
	}

	/** The shader catalog (the built-ins and the shaders folder scan) for the shader defaults. */
	var shaderCatalogEntries by mutableStateOf(ShaderCatalog.BUILT_INS)
		private set

	/** The bezel files of the bezels folder, for the custom bezel default. */
	var bezelEntries by mutableStateOf<List<String>>(emptyList())
		private set

	/** Reads amiberry.conf and the catalogs again (the screen calls it on entry and on resume). */
	fun reload() {
		val store = store
		if (store == null) {
			values = schema?.hostDefaults.orEmpty()
			return
		}
		viewModelScope.launch {
			val read = withContext(Dispatchers.IO) {
				val fileValues = store.read()
				val shaders = ShaderCatalog.scan(
					ShaderCatalog.resolveScanRoots(
						HostSettingsStore.settingsFile(app.filesDir, app.getExternalFilesDir(null)),
						app.getExternalFilesDir(null)
					)
				)
				val bezels = scanBezels()
				Triple(fileValues, shaders, bezels)
			}
			values = schema?.hostDefaults.orEmpty() + read.first
			shaderCatalogEntries = read.second
			bezelEntries = read.third
		}
	}

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
			if (written) {
				notice = null
				reload()
			} else {
				notice = com.blitterstudio.amiberry.R.string.appsettings_write_failed
			}
		}
	}

	/** The bezel images of the canonical bezels folder, as the bezel default names them. */
	private fun scanBezels(): List<String> {
		val external = app.getExternalFilesDir(null) ?: return emptyList()
		val folder = File(File(external, StoragePaths.VISUALS), StoragePaths.BEZELS)
		val files = folder.listFiles { file -> file.isFile } ?: return emptyList()
		return files.map { it.name }.filter { it.lowercase().endsWith(".png") || it.lowercase().endsWith(".jpg") }.sorted()
	}
}
