package com.blitterstudio.amiberry.ui.screens.appsettings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.ChoiceItem
import com.blitterstudio.amiberry.ui.components.settings.RowFooter
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaNumberRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SettingChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate

/** The default port devices the launcher offers: a device per port, or the fallback ("Automatic"). */
private val PORT_DEVICE_TOKENS = listOf("mouse", "joy0", "joy1", "joy2", "joy3", "onscreen_joy", "kbd1", "kbd2", "kbd3", "kbd9")

/** The parallel ports' default devices: no touch devices, no fallback. */
private val PARALLEL_PORT_DEVICE_TOKENS = listOf("mouse", "joy0", "joy1", "joy2", "joy3", "kbd1", "kbd2", "kbd3", "kbd9")

/**
 * One host setting over the value amiberry.conf holds, as the schema rows
 * render the machine options: a switch, a dropdown of the layer's choice
 * labels (or the launcher's own list for the settings that name the host's
 * devices, shaders and bezels), a slider or a field for numbers, and a
 * field for a key, a name or a folder. Changes apply at once ([onSet]
 * writes the file).
 */
@Composable
internal fun HostSettingRow(
	option: SettingsSchema.Option,
	value: String,
	onSet: (String) -> Unit,
	shaderCatalog: List<String>,
	bezels: List<String>,
	modifier: Modifier = Modifier,
	folder: FolderState? = null,
	onResetFolder: (() -> Unit)? = null,
	onChooseFolder: (() -> Unit)? = null
) {
	when {
		(option.type == SettingsSchema.TYPE_PATH || option.type == SettingsSchema.TYPE_FILE) &&
			folder != null ->
			FolderSettingRow(
				option = option,
				folder = folder,
				onChoose = onChooseFolder ?: {},
				onReset = onResetFolder,
				modifier = modifier
			)
		option.type == SettingsSchema.TYPE_BOOL ->
			SchemaSwitchRow(
				id = option.id,
				checked = value == "true",
				onCheckedChange = { onSet(if (it) "true" else "false") },
				modifier = modifier,
				option = option
			)
		option.type == SettingsSchema.TYPE_CHOICE ->
			SchemaChoiceRow(
				id = option.id,
				value = value,
				onSelect = onSet,
				modifier = modifier,
				choices = option.choices.ifEmpty { hostChoices(option, shaderCatalog, bezels) },
				option = option
			)
		option.type == SettingsSchema.TYPE_REAL || option.level ->
			SchemaLevelRow(
				id = option.id,
				stored = value.toDoubleOrNull()?.toInt() ?: 0,
				onStoredChange = { onSet(it.toString()) },
				modifier = modifier,
				option = option
			)
		option.type == SettingsSchema.TYPE_INT ->
			SchemaNumberRow(
				id = option.id,
				stored = value.toIntOrNull() ?: 0,
				onStoredChange = { onSet(it.toString()) },
				modifier = modifier,
				option = option
			)
		else -> when (option.id) {
			// The on-screen keyboard toggle: the buttons the machine option
			// offers, plus the key a configuration could name.
			"default_vkbd_toggle" -> {
				val choices = LauncherChoices.oskToggleButtons.map { (button, label) ->
					ChoiceItem(button.orEmpty(), stringResource(label))
				}
				SettingChoiceRow(
					label = option.label,
					valueText = toggleText(value),
					choices = choices,
					onSelect = onSet,
					modifier = modifier,
					help = option.help
				)
			}
			else -> HostTextRow(option = option, value = value, onSet = onSet, modifier = modifier)
		}
	}
}

/** The value text of the keyboard toggle default: a button by name, anything else as it is. */
@Composable
private fun toggleText(value: String): String = when (value) {
	"" -> stringResource(R.string.settings_osk_toggle_none)
	else -> settingText(LauncherChoices.oskToggle(value))
}

/**
 * The choice list of a setting the schema carries empty because it names
 * the host's own things: the port devices, the audio device, the shaders
 * and the bezels. A stored value the list lacks stays shown as it is (the
 * schema row's own rule), so a shader no longer in its folder reads by its
 * name.
 */
@Composable
private fun hostChoices(
	option: SettingsSchema.Option,
	shaderCatalog: List<String>,
	bezels: List<String>
): List<SettingsSchema.Choice> = when (option.id) {
	"default_controller1", "default_controller2", "default_mouse1", "default_mouse2" ->
		listOf(SettingsSchema.Choice("", stringResource(R.string.appsettings_device_automatic))) +
			PORT_DEVICE_TOKENS.map { token ->
				SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
			}
	"default_controller3", "default_controller4" ->
		listOf(SettingsSchema.Choice("", stringResource(R.string.settings_option_none))) +
			PARALLEL_PORT_DEVICE_TOKENS.map { token ->
				SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
			}
	"default_soundcard" -> listOf(SettingsSchema.Choice("0", stringResource(R.string.appsettings_sound_system_default)))
	"shader", "shader_rtg" -> shaderCatalog.map { name ->
		SettingsSchema.Choice(name, settingText(LauncherChoices.shader(name)))
	}
	"custom_bezel" ->
		listOf(SettingsSchema.Choice("none", stringResource(R.string.settings_option_none))) +
			bezels.map { name -> SettingsSchema.Choice(name, name) }
	else -> emptyList()
}

/**
 * A typed host setting — a key combination, a file name, a folder — whose
 * text applies when it is submitted or the field left, not as each letter
 * is typed: a half-typed folder must not become the emulator's. Empty text
 * means none (a path falls back to the default, a key to unbound).
 */
@Composable
internal fun HostTextRow(
	option: SettingsSchema.Option,
	value: String,
	onSet: (String) -> Unit,
	modifier: Modifier = Modifier
) {
	var text by remember(option.id, value) { mutableStateOf(value) }
	var editing by remember { mutableStateOf(false) }
	val typingGate = rememberDpadTypingGate()
	fun commit() {
		if (text != value) onSet(text)
	}
	OutlinedTextField(
		value = text,
		onValueChange = { text = it },
		singleLine = true,
		readOnly = typingGate.readOnly,
		label = { Text(option.label) },
		placeholder = { Text(stringResource(R.string.settings_option_none)) },
		keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
		keyboardActions = KeyboardActions(onDone = {
			commit()
			editing = false
			typingGate.stopTyping()
		}),
		modifier = modifier
			.dpadVerticalMovesFocus()
			.then(typingGate.modifier)
			.onFocusChanged { state ->
				editing = state.isFocused
				if (!state.isFocused) commit()
			}
			.fillMaxWidth()
	)
	RowFooter(option.help, enabled = true, reason = null)
}

/**
 * One folder setting as the Folders section shows it: the path the file
 * holds ([stored], null at its default), the folder in effect (the stored
 * one, else the base content root's), and whether that folder is there.
 */
data class FolderState(
	val id: String,
	val stored: String?,
	val effective: java.io.File?,
	val exists: Boolean
) {
	/** The setting's line names a folder of its own. */
	val overridden: Boolean get() = stored != null
}

/**
 * One path host setting: the folder in effect and where it comes from, a
 * picker of the device's storage ([onChoose]), and — while the file names
 * one — a reset to the default. The RetroArch config picks a file; every
 * other setting a folder, as the Apple apps' settings do.
 */
@Composable
internal fun FolderSettingRow(
	option: SettingsSchema.Option,
	folder: FolderState,
	onChoose: () -> Unit,
	onReset: (() -> Unit)?,
	modifier: Modifier = Modifier
) {
	Column(modifier = modifier.fillMaxWidth()) {
		Text(option.label, style = MaterialTheme.typography.titleSmall)
		Spacer(modifier = Modifier.height(2.dp))
		val pathText = folder.effective?.path
		Text(
			text = when {
				folder.overridden -> pathText.orEmpty()
				pathText != null -> stringResource(R.string.appsettings_folder_default_path, pathText)
				else -> stringResource(R.string.appsettings_folder_default)
			},
			style = MaterialTheme.typography.bodySmall,
			color = MaterialTheme.colorScheme.onSurfaceVariant
		)
		// A file setting never warns: the log is written when logging
		// starts and the RetroArch config is optional, and a folder with
		// no default of its own (the sounds, the plugins) has none to miss.
		if (!folder.exists && folder.effective != null && option.type == SettingsSchema.TYPE_PATH) {
			Spacer(modifier = Modifier.height(2.dp))
			Text(
				stringResource(R.string.appsettings_folder_missing),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.error
			)
		}
		Spacer(modifier = Modifier.height(8.dp))
		Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
			OutlinedButton(
				onClick = onChoose,
				modifier = Modifier.weight(1f).dpadFocusIndicator()
			) {
				Text(stringResource(R.string.appsettings_folder_choose))
			}
			if (folder.overridden && onReset != null) {
				TextButton(
					onClick = onReset,
					modifier = Modifier.weight(1f).dpadFocusIndicator()
				) {
					Text(stringResource(R.string.appsettings_folder_reset))
				}
			}
		}
		RowFooter(option.help, enabled = true, reason = null)
	}
}
