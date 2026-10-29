package com.blitterstudio.amiberry.overlay

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.ExitToApp
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.Crop
import androidx.compose.material.icons.filled.Gamepad
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Keyboard
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.PowerSettingsNew
import androidx.compose.material.icons.filled.RestartAlt
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.SwapVert
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.input.InputMode
import androidx.compose.ui.platform.LocalInputModeManager
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import java.text.DateFormat
import java.util.Date

/**
 * The in-emulation menu: a panel over the paused emulation's last frame.
 * Every page works with touch and with the D-pad alone; each opens with
 * focus on its first item, or on the item a D-pad user left it from.
 */
@Composable
fun OverlayScreen(viewModel: OverlayViewModel) {
	Box(
		modifier = Modifier
			.fillMaxSize()
			.background(Color.Black.copy(alpha = 0.55f))
			// Touches outside the panel stay with the menu, not the emulation below.
			.pointerInput(Unit) { awaitEachGesture { awaitFirstDown().consume() } }
			.windowInsetsPadding(WindowInsets.safeDrawing)
			.padding(16.dp),
		contentAlignment = Alignment.Center
	) {
		Surface(
			shape = RoundedCornerShape(24.dp),
			tonalElevation = 6.dp,
			shadowElevation = 8.dp,
			modifier = Modifier
				.widthIn(max = 560.dp)
				.fillMaxWidth()
		) {
			Box {
				Column(modifier = Modifier.padding(vertical = 12.dp)) {
					Header(viewModel)
					val focus = remember(viewModel.menu) { FocusTargets() }
					PageContent(viewModel, focus)
					// Each page change and each opening (the view composes while hidden).
					// Opening leaves touch mode (OverlayHost.show); a page a touch
					// user tapped open keeps no focus, so it shows no focus ring.
					val inputMode = LocalInputModeManager.current
					LaunchedEffect(viewModel.menu, viewModel.openings) {
						// Once the page has laid out, so its items' requesters are attached.
						withFrameNanos { }
						if (inputMode.inputMode != InputMode.Touch) focus.request(viewModel.menu.focus)
					}
				}
				// Over the page's foot, so the page never moves under a finger or the focus.
				viewModel.notice?.let { notice -> Notice(notice, Modifier.align(Alignment.BottomCenter)) }
			}
		}
	}
}

@Composable
private fun Notice(notice: OverlayNotice, modifier: Modifier) {
	Surface(
		shape = RoundedCornerShape(12.dp),
		color = if (notice.error) MaterialTheme.colorScheme.errorContainer else MaterialTheme.colorScheme.inverseSurface,
		contentColor = if (notice.error) MaterialTheme.colorScheme.onErrorContainer else MaterialTheme.colorScheme.inverseOnSurface,
		shadowElevation = 4.dp,
		modifier = modifier
			.padding(16.dp)
			.semantics { liveRegion = LiveRegionMode.Polite }
	) {
		Text(
			text = if (notice.slot != null) stringResource(notice.text, notice.slot) else stringResource(notice.text),
			style = MaterialTheme.typography.bodyMedium,
			modifier = Modifier.padding(horizontal = 16.dp, vertical = 10.dp)
		)
	}
}

/** The focus requesters of a page's items, by key; the first registered is the page's default. */
private class FocusTargets {
	private val requesters = LinkedHashMap<String, FocusRequester>()

	fun requester(key: String): FocusRequester = requesters.getOrPut(key) { FocusRequester() }

	/** Focuses [key], or the first item when it is not on the page (any more). */
	fun request(key: String?) {
		val wanted = key?.let { requesters[it] }
		if (wanted != null && runCatching { wanted.requestFocus() }.isSuccess) return
		requesters.values.firstOrNull()?.let { runCatching { it.requestFocus() } }
	}
}

@Composable
private fun Header(viewModel: OverlayViewModel) {
	val page = viewModel.menu.page
	val title = when (page) {
		OverlayPage.Home -> stringResource(R.string.overlay_title)
		OverlayPage.Media -> stringResource(R.string.overlay_drives)
		is OverlayPage.Picker -> when (val target = page.target) {
			is MediaTarget.Floppy -> stringResource(R.string.overlay_picker_floppy_title, target.drive)
			MediaTarget.Cd -> stringResource(R.string.overlay_picker_cd_title)
		}
		OverlayPage.States -> stringResource(R.string.overlay_states)
		OverlayPage.Controls -> stringResource(R.string.overlay_controls)
		OverlayPage.Settings -> stringResource(R.string.overlay_settings)
		is OverlayPage.Confirm -> when (val action = page.action) {
			ConfirmAction.Reset -> stringResource(R.string.overlay_confirm_reset_title)
			ConfirmAction.HardReset -> stringResource(R.string.overlay_confirm_hard_reset_title)
			ConfirmAction.Quit -> stringResource(R.string.overlay_confirm_quit_title)
			is ConfirmAction.Overwrite -> stringResource(R.string.overlay_confirm_overwrite_title, action.slot.paths.slot)
		}
	}
	Row(
		verticalAlignment = Alignment.CenterVertically,
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 12.dp)
			.heightIn(min = 48.dp)
	) {
		if (viewModel.menu.depth > 1) {
			// Touch only: Back and the gamepad's B do the same for D-pad users.
			IconButton(onClick = viewModel::back) {
				Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = stringResource(R.string.overlay_back))
			}
		} else {
			Spacer(modifier = Modifier.width(12.dp))
		}
		Text(
			text = title,
			style = MaterialTheme.typography.titleLarge,
			maxLines = 1,
			overflow = TextOverflow.Ellipsis,
			modifier = Modifier.weight(1f)
		)
	}
	HorizontalDivider(modifier = Modifier.padding(top = 8.dp))
}

@Composable
private fun PageContent(viewModel: OverlayViewModel, focus: FocusTargets) {
	when (val page = viewModel.menu.page) {
		OverlayPage.Home -> ScrollingPage { HomePage(viewModel, focus) }
		OverlayPage.Media -> ScrollingPage { MediaPage(viewModel, focus) }
		is OverlayPage.Picker -> PickerPage(viewModel, page.target, focus)
		OverlayPage.States -> ScrollingPage { StatesPage(viewModel, focus) }
		OverlayPage.Controls -> ScrollingPage { ControlsPage(viewModel, focus) }
		OverlayPage.Settings -> ScrollingPage { SettingsPage(viewModel, focus) }
		is OverlayPage.Confirm -> ScrollingPage { ConfirmPage(viewModel, page.action, focus) }
	}
}

/** A page short enough to compose whole, so every item's focus requester is attached. */
@Composable
private fun ScrollingPage(content: @Composable ColumnScope.() -> Unit) {
	Column(
		modifier = Modifier
			.verticalScroll(rememberScrollState())
			.padding(horizontal = 16.dp, vertical = 8.dp),
		verticalArrangement = Arrangement.spacedBy(4.dp),
		content = content
	)
}

/** A full-width menu entry: one D-pad stop, a large touch target. */
@Composable
private fun MenuItem(
	icon: ImageVector,
	title: String,
	subtitle: String?,
	focusKey: String,
	focus: FocusTargets,
	onClick: () -> Unit,
	enabled: Boolean = true,
	emphasis: Boolean = false
) {
	val color = when {
		!enabled -> MaterialTheme.colorScheme.onSurface.copy(alpha = 0.38f)
		emphasis -> MaterialTheme.colorScheme.primary
		else -> MaterialTheme.colorScheme.onSurface
	}
	Row(
		verticalAlignment = Alignment.CenterVertically,
		modifier = Modifier
			.fillMaxWidth()
			.heightIn(min = 56.dp)
			.focusRequester(focus.requester(focusKey))
			// Before clickable: onFocusChanged reports the focus target after it in the chain.
			.dpadFocusIndicator()
			.clickable(enabled = enabled, onClick = onClick)
			.padding(horizontal = 12.dp, vertical = 8.dp)
	) {
		Icon(icon, contentDescription = null, tint = color)
		Spacer(modifier = Modifier.width(16.dp))
		Column(modifier = Modifier.weight(1f)) {
			Text(title, style = MaterialTheme.typography.titleMedium, color = color)
			if (!subtitle.isNullOrBlank()) {
				Text(
					subtitle,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant,
					maxLines = 2,
					overflow = TextOverflow.Ellipsis
				)
			}
		}
	}
}

@Composable
private fun HomePage(viewModel: OverlayViewModel, focus: FocusTargets) {
	MenuItem(Icons.Filled.PlayArrow, stringResource(R.string.overlay_resume), stringResource(R.string.overlay_resume_help),
		OverlayKeys.RESUME, focus, viewModel::resume, emphasis = true)
	MenuItem(Icons.Filled.Pause, stringResource(R.string.overlay_pause), stringResource(R.string.overlay_pause_help),
		OverlayKeys.PAUSE, focus, viewModel::pause)
	MenuItem(Icons.Filled.Save, stringResource(R.string.overlay_drives), drivesSummary(viewModel.status),
		OverlayKeys.MEDIA, focus, { viewModel.open(OverlayPage.Media, OverlayKeys.MEDIA) })
	MenuItem(Icons.Filled.History, stringResource(R.string.overlay_states), stringResource(R.string.overlay_states_help),
		OverlayKeys.STATES, focus, { viewModel.open(OverlayPage.States, OverlayKeys.STATES) })
	MenuItem(Icons.Filled.SportsEsports, stringResource(R.string.overlay_controls), stringResource(R.string.overlay_controls_help),
		OverlayKeys.CONTROLS, focus, { viewModel.open(OverlayPage.Controls, OverlayKeys.CONTROLS) })
	MenuItem(Icons.Filled.Tune, stringResource(R.string.overlay_settings), stringResource(R.string.overlay_settings_help),
		OverlayKeys.SETTINGS, focus, { viewModel.open(OverlayPage.Settings, OverlayKeys.SETTINGS) })
	HorizontalDivider(modifier = Modifier.padding(vertical = 4.dp))
	MenuItem(Icons.Filled.RestartAlt, stringResource(R.string.overlay_reset), stringResource(R.string.overlay_reset_help),
		OverlayKeys.RESET, focus, { viewModel.open(OverlayPage.Confirm(ConfirmAction.Reset), OverlayKeys.RESET) })
	MenuItem(Icons.Filled.PowerSettingsNew, stringResource(R.string.overlay_hard_reset), stringResource(R.string.overlay_hard_reset_help),
		OverlayKeys.HARD_RESET, focus, { viewModel.open(OverlayPage.Confirm(ConfirmAction.HardReset), OverlayKeys.HARD_RESET) })
	MenuItem(Icons.Filled.Settings, stringResource(R.string.overlay_advanced), stringResource(R.string.overlay_advanced_help),
		OverlayKeys.ADVANCED, focus, viewModel::advancedSettings)
	MenuItem(Icons.AutoMirrored.Filled.ExitToApp, stringResource(R.string.overlay_quit), stringResource(R.string.overlay_quit_help),
		OverlayKeys.QUIT, focus, { viewModel.open(OverlayPage.Confirm(ConfirmAction.Quit), OverlayKeys.QUIT) })
}

/** "DF0: Game.adf · DF1: Empty": what the drives hold, on the Home entry. */
@Composable
private fun drivesSummary(status: OverlayStatus): String {
	val empty = stringResource(R.string.overlay_drive_empty)
	val parts = status.enabledDrives.map { drive ->
		stringResource(R.string.overlay_drive_name, drive.drive) + " " + fileLabel(drive.path, empty)
	}.toMutableList()
	if (status.hasCd) parts += stringResource(R.string.overlay_cd_name) + " " + fileLabel(status.cdPath, empty)
	return parts.joinToString(" · ").ifEmpty { stringResource(R.string.overlay_drives_help) }
}

private fun fileLabel(path: String, empty: String): String = path.substringAfterLast('/').ifEmpty { empty }

@Composable
private fun MediaPage(viewModel: OverlayViewModel, focus: FocusTargets) {
	val status = viewModel.status
	val empty = stringResource(R.string.overlay_drive_empty)
	status.enabledDrives.forEach { drive ->
		DriveRow(
			name = stringResource(R.string.overlay_drive_name, drive.drive),
			path = drive.path,
			empty = empty,
			target = MediaTarget.Floppy(drive.drive),
			viewModel = viewModel,
			focus = focus
		)
	}
	if (status.hasCd) {
		DriveRow(stringResource(R.string.overlay_cd_name), status.cdPath, empty, MediaTarget.Cd, viewModel, focus)
	}
	val swapper = viewModel.swapper.withIndex().filter { it.value.isNotEmpty() }
	if (swapper.isNotEmpty()) {
		HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
		Text(stringResource(R.string.overlay_swapper), style = MaterialTheme.typography.titleSmall)
		swapper.forEach { (index, path) ->
			Text(fileLabel(path, empty), style = MaterialTheme.typography.bodyMedium, maxLines = 1, overflow = TextOverflow.Ellipsis)
			Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
				status.enabledDrives.forEach { drive ->
					OutlinedButton(
						onClick = { viewModel.insertSwapperEntry(index, drive.drive) },
						modifier = Modifier
							.weight(1f)
							.focusRequester(focus.requester("swapper_${index}_${drive.drive}"))
							.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.overlay_swapper_insert, drive.drive))
					}
				}
			}
		}
	}
}

@Composable
private fun DriveRow(
	name: String,
	path: String,
	empty: String,
	target: MediaTarget,
	viewModel: OverlayViewModel,
	focus: FocusTargets
) {
	Column(modifier = Modifier.padding(vertical = 4.dp)) {
		Row(verticalAlignment = Alignment.CenterVertically) {
			Text(name, style = MaterialTheme.typography.titleMedium, modifier = Modifier.width(56.dp))
			Text(
				fileLabel(path, empty),
				style = MaterialTheme.typography.bodyMedium,
				color = if (path.isEmpty()) MaterialTheme.colorScheme.onSurfaceVariant else MaterialTheme.colorScheme.onSurface,
				maxLines = 1,
				overflow = TextOverflow.Ellipsis
			)
		}
		Spacer(modifier = Modifier.height(4.dp))
		// Full-width buttons, so the D-pad moves straight between drives.
		Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
			val key = OverlayKeys.insert(target)
			Button(
				onClick = { viewModel.open(OverlayPage.Picker(target), key) },
				modifier = Modifier
					.weight(1f)
					.focusRequester(focus.requester(key))
					.dpadFocusIndicator()
			) {
				Text(
					stringResource(
						when {
							path.isEmpty() -> R.string.overlay_insert
							target == MediaTarget.Cd -> R.string.overlay_change_cd
							else -> R.string.overlay_change_disk
						}
					)
				)
			}
			// Never disabled: ejecting would take the focus from under a D-pad
			// user's feet; an empty drive's Eject does nothing.
			OutlinedButton(
				onClick = { viewModel.eject(target) },
				modifier = Modifier
					.weight(1f)
					.dpadFocusIndicator()
			) {
				Text(stringResource(R.string.overlay_eject))
			}
		}
	}
}

@Composable
private fun PickerPage(viewModel: OverlayViewModel, target: MediaTarget, focus: FocusTargets) {
	val files by (if (target == MediaTarget.Cd) viewModel.cdImages else viewModel.floppies).collectAsState()
	val scanning by viewModel.scanning.collectAsState()
	// The emulator process starts with no scan: the list fills after the page
	// opened, when its default focus found nothing to focus.
	val inputMode = LocalInputModeManager.current
	val hasFiles = files.isNotEmpty()
	LaunchedEffect(hasFiles) {
		if (!hasFiles || inputMode.inputMode == InputMode.Touch) return@LaunchedEffect
		withFrameNanos { }
		focus.request(null)
	}
	if (files.isEmpty()) {
		Text(
			text = stringResource(
				when {
					scanning -> R.string.overlay_picker_scanning
					target == MediaTarget.Cd -> R.string.overlay_picker_empty_cd
					else -> R.string.overlay_picker_empty_floppy
				}
			),
			style = MaterialTheme.typography.bodyMedium,
			modifier = Modifier.padding(24.dp)
		)
		return
	}
	LazyColumn(
		modifier = Modifier
			.fillMaxWidth()
			.heightIn(max = 480.dp)
			.padding(horizontal = 16.dp, vertical = 8.dp)
	) {
		itemsIndexed(files, key = { _, file -> file.path }) { index, file ->
			Row(
				verticalAlignment = Alignment.CenterVertically,
				modifier = Modifier
					.fillMaxWidth()
					.heightIn(min = 52.dp)
					// Only the first file: the page's default focus, always composed.
					.then(if (index == 0) Modifier.focusRequester(focus.requester("file_first")) else Modifier)
					.dpadFocusIndicator()
					.clickable { viewModel.insert(target, file.path) }
					.padding(horizontal = 12.dp, vertical = 8.dp)
			) {
				Icon(if (target == MediaTarget.Cd) Icons.Filled.Album else Icons.Filled.Save, contentDescription = null)
				Spacer(modifier = Modifier.width(16.dp))
				Text(file.name, style = MaterialTheme.typography.bodyLarge, maxLines = 2, overflow = TextOverflow.Ellipsis, modifier = Modifier.weight(1f))
				Spacer(modifier = Modifier.width(8.dp))
				Text(file.sizeDisplay, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
			}
		}
	}
}

@Composable
private fun StatesPage(viewModel: OverlayViewModel, focus: FocusTargets) {
	if (viewModel.slots.isEmpty()) {
		Text(stringResource(R.string.overlay_states_unavailable), modifier = Modifier.padding(8.dp))
		return
	}
	val format = remember { DateFormat.getDateTimeInstance(DateFormat.MEDIUM, DateFormat.SHORT) }
	// Saving or loading: the buttons wait for it.
	if (viewModel.busy) LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
	viewModel.slots.forEach { slot ->
		val number = slot.paths.slot
		Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(vertical = 6.dp)) {
			Box(
				contentAlignment = Alignment.Center,
				modifier = Modifier
					.size(width = 112.dp, height = 84.dp)
					.background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(8.dp))
			) {
				val thumbnail = slot.thumbnail
				if (thumbnail != null) {
					Image(
						bitmap = thumbnail,
						contentDescription = stringResource(R.string.overlay_slot_thumbnail, number),
						contentScale = ContentScale.Fit,
						modifier = Modifier.fillMaxSize()
					)
				} else {
					Icon(Icons.Filled.History, contentDescription = null, tint = MaterialTheme.colorScheme.onSurfaceVariant)
				}
			}
			Spacer(modifier = Modifier.width(12.dp))
			Column(modifier = Modifier.weight(1f)) {
				Text(stringResource(R.string.overlay_slot, number), style = MaterialTheme.typography.titleMedium)
				Text(
					slot.savedAt?.let { stringResource(R.string.overlay_slot_saved_at, format.format(Date(it))) }
						?: stringResource(R.string.overlay_slot_empty),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
				Spacer(modifier = Modifier.height(4.dp))
				Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
					Button(
						onClick = { viewModel.requestSave(slot) },
						// Busy ignores presses rather than disabling: a disabled
						// button drops the D-pad focus.
						modifier = Modifier
							.weight(1f)
							.focusRequester(focus.requester(OverlayKeys.save(number)))
							.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.overlay_save))
					}
					OutlinedButton(
						onClick = { viewModel.loadSlot(slot) },
						enabled = slot.savedAt != null,
						modifier = Modifier
							.weight(1f)
							.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.overlay_load))
					}
				}
			}
		}
	}
}

@Composable
private fun ControlsPage(viewModel: OverlayViewModel, focus: FocusTargets) {
	val schema = settingsSchema()
	if (RemoteJoystick.isActive) {
		Note(stringResource(R.string.overlay_remote_joystick))
	}
	val ports = OverlaySettings.offered(schema, OverlaySettings.PORTS)
	ports.forEach { option ->
		val state = viewModel.settings[option.id] ?: return@forEach
		SchemaChoiceRow(
			id = option.id,
			value = state.value,
			onSelect = { viewModel.change(option.id, it) },
			enabled = state.available,
			// The layer lists the devices connected now; the schema cannot.
			choices = viewModel.portChoices[option.id],
			modifier = Modifier
				.padding(horizontal = 12.dp)
				.focusRequester(focus.requester(option.id))
		)
	}
	if (OverlaySettings.swappedPorts(viewModel.settings) != null) {
		MenuItem(
			icon = Icons.Filled.SwapVert,
			title = stringResource(R.string.overlay_swap_ports),
			subtitle = stringResource(R.string.overlay_swap_ports_help),
			focusKey = OverlayKeys.SWAP_PORTS,
			focus = focus,
			onClick = viewModel::swapPorts
		)
	}
	if (ports.isNotEmpty()) HorizontalDivider(modifier = Modifier.padding(vertical = 4.dp))
	OverlaySettings.offered(schema, OverlaySettings.CONTROLS).forEach { option ->
		val state = viewModel.settings[option.id] ?: return@forEach
		SwitchItem(
			icon = if (option.id == OverlaySettings.ON_SCREEN_KEYBOARD) Icons.Filled.Keyboard else Icons.Filled.Gamepad,
			title = option.label,
			subtitle = option.help,
			checked = state.value == "true",
			onCheckedChange = { viewModel.change(option.id, if (it) "true" else "false") },
			enabled = state.available,
			focusKey = option.id,
			focus = focus
		)
	}
	val keyboardOn = viewModel.settings[OverlaySettings.ON_SCREEN_KEYBOARD]?.value == "true"
	MenuItem(
		icon = Icons.Filled.Keyboard,
		title = stringResource(R.string.overlay_show_keyboard),
		subtitle = stringResource(if (keyboardOn) R.string.overlay_show_keyboard_help else R.string.overlay_show_keyboard_off),
		focusKey = "show_keyboard",
		focus = focus,
		onClick = viewModel::showKeyboard,
		enabled = keyboardOn
	)
	Note(stringResource(R.string.overlay_controls_kept))
}

/** An on/off menu entry, laid out as [MenuItem]: the whole row toggles (one D-pad stop). */
@Composable
private fun SwitchItem(
	icon: ImageVector,
	title: String,
	subtitle: String?,
	checked: Boolean,
	onCheckedChange: (Boolean) -> Unit,
	enabled: Boolean,
	focusKey: String,
	focus: FocusTargets
) {
	val color = if (enabled) MaterialTheme.colorScheme.onSurface else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.38f)
	Row(
		verticalAlignment = Alignment.CenterVertically,
		modifier = Modifier
			.fillMaxWidth()
			.heightIn(min = 56.dp)
			.focusRequester(focus.requester(focusKey))
			// Before toggleable: onFocusChanged reports the focus target after it in the chain.
			.dpadFocusIndicator()
			.toggleable(value = checked, enabled = enabled, role = Role.Switch, onValueChange = onCheckedChange)
			.padding(horizontal = 12.dp, vertical = 8.dp)
	) {
		Icon(icon, contentDescription = null, tint = color)
		Spacer(modifier = Modifier.width(16.dp))
		Column(modifier = Modifier.weight(1f)) {
			Text(title, style = MaterialTheme.typography.titleMedium, color = color)
			if (!subtitle.isNullOrBlank()) {
				Text(
					subtitle,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant,
					maxLines = 2,
					overflow = TextOverflow.Ellipsis
				)
			}
		}
		Spacer(modifier = Modifier.width(12.dp))
		Switch(checked = checked, onCheckedChange = null, enabled = enabled)
	}
}

/** A line of explanation on a page, inset as the entries' text. */
@Composable
private fun Note(text: String) {
	Text(
		text,
		style = MaterialTheme.typography.bodySmall,
		color = MaterialTheme.colorScheme.onSurfaceVariant,
		modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)
	)
}

@Composable
private fun SettingsPage(viewModel: OverlayViewModel, focus: FocusTargets) {
	val options = OverlaySettings.offered(settingsSchema(), OverlaySettings.SETTINGS)
		// A machine with no CD drive has no CD audio to set.
		.filter { it.id != OverlaySettings.CD_VOLUME || viewModel.status.hasCd }
	if (options.isEmpty() || viewModel.settings.isEmpty()) {
		Text(stringResource(R.string.overlay_settings_unavailable), modifier = Modifier.padding(8.dp))
		return
	}
	Note(stringResource(R.string.overlay_settings_session))
	options.forEach { option ->
		val state = viewModel.settings[option.id] ?: return@forEach
		val modifier = Modifier
			.padding(horizontal = 12.dp)
			.focusRequester(focus.requester(option.id))
		val change: (String) -> Unit = { viewModel.change(option.id, it) }
		when {
			option.type == SettingsSchema.TYPE_BOOL -> SwitchItem(
				icon = Icons.Filled.Crop,
				title = option.label,
				subtitle = option.help,
				checked = state.value == "true",
				onCheckedChange = { change(if (it) "true" else "false") },
				enabled = state.available,
				focusKey = option.id,
				focus = focus
			)
			option.type == SettingsSchema.TYPE_INT && option.min != null && option.max != null -> SchemaLevelRow(
				id = option.id,
				stored = state.value.toIntOrNull() ?: option.min.toInt(),
				onStoredChange = { change(it.toString()) },
				enabled = state.available,
				modifier = modifier
			)
			option.type == SettingsSchema.TYPE_CHOICE -> SchemaChoiceRow(
				id = option.id,
				value = state.value,
				onSelect = change,
				enabled = state.available,
				// The layer lists the shaders folder's presets; the schema cannot.
				choices = if (option.id == OverlaySettings.SHADER) viewModel.shaderChoices else null,
				modifier = modifier
			)
		}
	}
}

@Composable
private fun ConfirmPage(viewModel: OverlayViewModel, action: ConfirmAction, focus: FocusTargets) {
	val format = remember { DateFormat.getDateTimeInstance(DateFormat.MEDIUM, DateFormat.SHORT) }
	Text(
		when (action) {
			ConfirmAction.Reset -> stringResource(R.string.overlay_confirm_reset_text)
			ConfirmAction.HardReset -> stringResource(R.string.overlay_confirm_hard_reset_text)
			ConfirmAction.Quit -> stringResource(R.string.overlay_confirm_quit_text)
			is ConfirmAction.Overwrite -> stringResource(
				R.string.overlay_confirm_overwrite_text,
				action.slot.savedAt?.let { format.format(Date(it)) }.orEmpty()
			)
		},
		style = MaterialTheme.typography.bodyLarge,
		modifier = Modifier.padding(8.dp)
	)
	Spacer(modifier = Modifier.height(8.dp))
	Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
		// Cancel first: the default focus, so a stray OK changes nothing.
		OutlinedButton(
			onClick = viewModel::back,
			modifier = Modifier
				.weight(1f)
				.focusRequester(focus.requester("cancel"))
				.dpadFocusIndicator()
		) {
			Text(stringResource(R.string.overlay_cancel))
		}
		Button(
			onClick = { viewModel.confirm(action) },
			colors = ButtonDefaults.buttonColors(
				containerColor = MaterialTheme.colorScheme.error,
				contentColor = MaterialTheme.colorScheme.onError
			),
			modifier = Modifier
				.weight(1f)
				.dpadFocusIndicator()
		) {
			Text(
				stringResource(
					when (action) {
						ConfirmAction.Reset -> R.string.overlay_reset
						ConfirmAction.HardReset -> R.string.overlay_hard_reset
						ConfirmAction.Quit -> R.string.overlay_quit
						is ConfirmAction.Overwrite -> R.string.overlay_overwrite
					}
				)
			)
		}
	}
}
