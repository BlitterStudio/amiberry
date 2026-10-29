package com.blitterstudio.amiberry.overlay

import android.app.Application
import android.graphics.BitmapFactory
import androidx.annotation.StringRes
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.InGameControls
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.SettingsSchema
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.io.File

/** A state slot as the States page shows it. */
data class SlotState(
	val paths: SlotPaths,
	/** When the state was saved (epoch ms); null for an empty slot. */
	val savedAt: Long?,
	val thumbnail: ImageBitmap?
)

/** A short message under the menu's title: what an action did, or why it failed. */
data class OverlayNotice(@StringRes val text: Int, val slot: Int? = null, val error: Boolean = false)

/**
 * The in-emulation menu's state and actions. The menu shows while the
 * native loop (android_overlay_run) holds the emulation paused; everything
 * it changes goes through [OverlayNative]: actions are posted from the UI
 * thread, settings and state names are read and written on background
 * threads, which wait for the emulator thread.
 */
class OverlayViewModel(application: Application) : AndroidViewModel(application) {
	var menu by mutableStateOf(OverlayMenu.HOME)
		private set
	/** How often the menu has opened: each opening focuses its first item again. */
	var openings by mutableIntStateOf(0)
		private set
	var status by mutableStateOf(OverlayStatus.EMPTY)
		private set
	var swapper by mutableStateOf<List<String>>(emptyList())
		private set
	var settings by mutableStateOf<Map<String, SettingState>>(emptyMap())
		private set
	/** The layer's shader choices (built-ins and the shaders folder); null until read. */
	var shaderChoices by mutableStateOf<List<SettingsSchema.Choice>?>(null)
		private set
	/** The devices each port may take now, by port option; read as the Controls page opens. */
	var portChoices by mutableStateOf<Map<String, List<SettingsSchema.Choice>>>(emptyMap())
		private set
	var slots by mutableStateOf<List<SlotState>>(emptyList())
		private set
	/** A save or load is running; the States page waits for it. */
	var busy by mutableStateOf(false)
		private set
	var notice by mutableStateOf<OverlayNotice?>(null)
		private set

	private val files = FileRepository.getInstance(application)
	val floppies: StateFlow<List<AmigaFile>> = files.floppies
	val cdImages: StateFlow<List<AmigaFile>> = files.cdImages
	val scanning: StateFlow<Boolean> = files.isScanning

	/** Hides the overlay's view; set by [OverlayHost]. */
	internal var hideView: () -> Unit = {}

	private var polling: Job? = null
	private var noticeTimeout: Job? = null
	private var sources: StateNameSources? = null
	/** Setting changes commit through this, in order (kotlinx's Mutex is fair). */
	private val settingWrites = Mutex()
	/** Changes made and not yet committed; main thread only. */
	private var pendingWrites = 0

	/** The overlay became visible: a fresh menu over the current machine. */
	fun shown() {
		menu = OverlayMenu.HOME
		openings++
		notice = null
		busy = false
		polling?.cancel()
		// The media a session's states are named after may have changed since.
		sources = null
		polling = viewModelScope.launch {
			while (isActive) {
				refreshStatus()
				delay(STATUS_INTERVAL_MS)
			}
		}
		viewModelScope.launch { readSettings() }
		viewModelScope.launch { loadSlots() }
	}

	/** The overlay was hidden. */
	fun hidden() {
		polling?.cancel()
		polling = null
	}

	fun open(page: OverlayPage, from: String) {
		notice = null
		menu = menu.open(page, from)
		when (page) {
			is OverlayPage.Picker -> viewModelScope.launch {
				files.rescanCategory(if (page.target == MediaTarget.Cd) FileCategory.CD_IMAGES else FileCategory.FLOPPIES)
			}
			OverlayPage.Settings -> if (shaderChoices == null) viewModelScope.launch { readShaderChoices() }
			// Controllers may have come or gone since the last opening.
			OverlayPage.Controls -> viewModelScope.launch { readPortChoices() }
			else -> Unit
		}
	}

	/** Back: the page under this one, or out of the menu on Home. */
	fun back() {
		notice = null
		menu = menu.back() ?: run {
			resume()
			return
		}
	}

	fun resume() = close(OverlayNative.CLOSE_RESUME)

	/** Leaves the menu with the emulation paused (until the menu is opened and left again). */
	fun pause() {
		OverlayNative.pause()
		close(OverlayNative.CLOSE_RESUME)
	}

	fun reset(hard: Boolean) {
		OverlayNative.reset(hard)
		close(OverlayNative.CLOSE_RESUME)
	}

	fun quit() {
		OverlayNative.quit()
		close(OverlayNative.CLOSE_RESUME)
	}

	fun advancedSettings() = close(OverlayNative.CLOSE_ADVANCED)

	fun showKeyboard() = close(OverlayNative.CLOSE_KEYBOARD)

	fun insert(target: MediaTarget, path: String) {
		when (target) {
			is MediaTarget.Floppy -> OverlayNative.insertFloppy(target.drive, path)
			MediaTarget.Cd -> OverlayNative.insertCd(path)
		}
		menu = menu.back() ?: OverlayMenu.HOME
		viewModelScope.launch { refreshSoon() }
	}

	fun eject(target: MediaTarget) {
		when (target) {
			is MediaTarget.Floppy -> OverlayNative.ejectFloppy(target.drive)
			MediaTarget.Cd -> OverlayNative.ejectCd()
		}
		viewModelScope.launch { refreshSoon() }
	}

	fun insertSwapperEntry(index: Int, drive: Int) {
		OverlayNative.insertSwapperEntry(index, drive)
		viewModelScope.launch { refreshSoon() }
	}

	/** Changes one live option (see [changeAll]). */
	fun change(id: String, value: String) = changeAll(mapOf(id to value))

	/** Port 1's device into port 2 and port 2's into port 1, in one commit. */
	fun swapPorts() {
		OverlaySettings.swappedPorts(settings)?.let(::changeAll)
	}

	/**
	 * Changes live options, committed together; the running machine picks
	 * them up between frames. Changes commit one at a time in the order made
	 * (a held D-pad key on a slider makes many), and the menu reads the
	 * values back only after the last, so the slider never snaps back to an
	 * older one. The on-screen control switches are also kept for the next
	 * launch (InGameControls); the rest apply to this session.
	 */
	private fun changeAll(values: Map<String, String>) {
		values.forEach { (id, value) -> settings[id]?.let { settings = settings + (id to it.copy(value = value)) } }
		pendingWrites++
		viewModelScope.launch {
			val ids = values.keys.toTypedArray()
			val texts = values.values.toTypedArray()
			// Main.immediate: each launch queues on the fair mutex in call order.
			val applied = settingWrites.withLock {
				withContext(Dispatchers.IO) { OverlayNative.changeSettings(ids, texts) }
			}
			pendingWrites--
			if (applied < 0) {
				showNotice(OverlayNotice(R.string.overlay_setting_failed, error = true))
			} else {
				withContext(Dispatchers.IO) {
					values.filterKeys { it in OverlaySettings.CONTROLS }.forEach { (id, value) ->
						InGameControls.record(getApplication(), id, value == "true")
					}
				}
			}
			if (pendingWrites == 0) {
				readSettings()
				if (values.keys.any { it in OverlaySettings.PORTS || it == OverlaySettings.ON_SCREEN_JOYSTICK }) readPortChoices()
			}
		}
	}

	/** Save on a slot: straight away when it is empty, after a confirmation when it holds a state. */
	fun requestSave(slot: SlotState) {
		if (busy) return
		if (slot.savedAt == null) saveSlot(slot)
		else open(OverlayPage.Confirm(ConfirmAction.Overwrite(slot)), OverlayKeys.save(slot.paths.slot))
	}

	/** The confirmation page's action, back on the page that asked for it. */
	fun confirm(action: ConfirmAction) {
		when (action) {
			ConfirmAction.Reset -> reset(hard = false)
			ConfirmAction.HardReset -> reset(hard = true)
			ConfirmAction.Quit -> quit()
			is ConfirmAction.Overwrite -> {
				menu = menu.back() ?: OverlayMenu.HOME
				saveSlot(action.slot)
			}
		}
	}

	private fun saveSlot(slot: SlotState) {
		if (busy) return
		busy = true
		notice = null
		viewModelScope.launch {
			val saved = finished(OverlayNative.saveState(slot.paths.state))
			// The thumbnail is the frame the state was saved at, as the built-in GUI takes it.
			if (saved) finished(OverlayNative.screenshot(slot.paths.thumbnail))
			showNotice(
				if (saved) {
					OverlayNotice(R.string.overlay_state_saved, slot.paths.slot)
				} else {
					OverlayNotice(R.string.overlay_state_save_failed, slot.paths.slot, error = true)
				}
			)
			loadSlots()
			busy = false
		}
	}

	fun loadSlot(slot: SlotState) {
		if (busy) return
		busy = true
		notice = null
		viewModelScope.launch {
			// A load ends the native loop itself (the emulation restores the
			// state), which hides the menu; a failed one leaves it up.
			if (finished(OverlayNative.loadState(slot.paths.state))) {
				close(OverlayNative.CLOSE_RESUME)
			} else {
				showNotice(OverlayNotice(R.string.overlay_state_load_failed, slot.paths.slot, error = true))
			}
			busy = false
		}
	}

	private fun close(how: Int) {
		hideView()
		OverlayNative.close(how)
	}

	/** Shows [shown] over the page for a while; it covers nothing the page needs and moves nothing. */
	private fun showNotice(shown: OverlayNotice) {
		notice = shown
		noticeTimeout?.cancel()
		noticeTimeout = viewModelScope.launch {
			delay(NOTICE_MS)
			if (notice == shown) notice = null
		}
	}

	private fun refreshStatus() {
		OverlayStatus.parse(OverlayNative.status())?.let { status = it }
		swapper = OverlayNative.swapperEntries().toList()
	}

	/** Actions run at the native loop's next pass; show what they did once they have. */
	private suspend fun refreshSoon() {
		delay(ACTION_SETTLE_MS)
		refreshStatus()
	}

	private suspend fun readSettings() {
		val ids = OverlaySettings.ALL
		val read = withContext(Dispatchers.IO) {
			SettingState.parse(ids, OverlayNative.readSettings(ids.toTypedArray()))
		}
		if (read.isNotEmpty()) settings = read
	}

	private suspend fun readShaderChoices() {
		val pairs = withContext(Dispatchers.IO) { OverlayNative.settingChoices(OverlaySettings.SHADER) } ?: return
		shaderChoices = pairs.toList().chunked(2).filter { it.size == 2 }.map { SettingsSchema.Choice(it[0], it[1]) }
	}

	private suspend fun readPortChoices() {
		val read = withContext(Dispatchers.IO) {
			OverlaySettings.PORTS.associateWith { id ->
				OverlayNative.settingChoices(id)?.toList()?.chunked(2)?.filter { it.size == 2 }
					?.map { SettingsSchema.Choice(it[0], it[1]) }
			}
		}
		portChoices = read.filterValues { it != null }.mapValues { it.value.orEmpty() }
	}

	private suspend fun loadSlots() {
		val loaded = withContext(Dispatchers.IO) {
			val names = sources ?: StateNameSources.parse(OverlayNative.stateNameSources())
			names?.let { found ->
				found to StateSlots.slots.map { slot -> readSlot(StateSlots.paths(found, slot)) }
			}
		} ?: return
		sources = loaded.first
		slots = loaded.second
	}

	/** Polls a posted action until it has run; false when it was rejected, failed or never ran. */
	private suspend fun finished(id: Long): Boolean {
		if (id == 0L) return false
		var waited = 0L
		while (waited < ACTION_TIMEOUT_MS) {
			when (OverlayNative.poll(id)) {
				OverlayNative.RESULT_DONE -> return true
				OverlayNative.RESULT_PENDING -> Unit
				else -> return false
			}
			delay(ACTION_POLL_MS)
			waited += ACTION_POLL_MS
		}
		return false
	}

	override fun onCleared() {
		polling?.cancel()
	}

	private companion object {
		const val STATUS_INTERVAL_MS = 250L
		const val NOTICE_MS = 4_000L
		const val ACTION_SETTLE_MS = 100L
		const val ACTION_POLL_MS = 30L
		const val ACTION_TIMEOUT_MS = 15_000L
		/** Thumbnails are decoded at no more than this width. */
		const val THUMBNAIL_WIDTH = 360

		fun readSlot(paths: SlotPaths): SlotState {
			val state = File(paths.state)
			if (!state.isFile) return SlotState(paths, savedAt = null, thumbnail = null)
			return SlotState(paths, savedAt = state.lastModified(), thumbnail = decodeThumbnail(paths.thumbnail))
		}

		fun decodeThumbnail(path: String): ImageBitmap? {
			if (!File(path).isFile) return null
			val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
			BitmapFactory.decodeFile(path, bounds)
			var sample = 1
			while (bounds.outWidth / (sample * 2) >= THUMBNAIL_WIDTH) sample *= 2
			val options = BitmapFactory.Options().apply { inSampleSize = sample }
			return BitmapFactory.decodeFile(path, options)?.asImageBitmap()
		}
	}
}
