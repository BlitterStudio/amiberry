package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Eject
import androidx.compose.material.icons.filled.FolderOpen
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.rememberImportPicker
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SupportingText
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.schemaLabel
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.hasTouchScreen
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlin.math.roundToInt

/**
 * One option's row as its page shows it: the pages' own rows live here once,
 * and every page and All Settings render them through [SettingsOptionRow], so
 * the rows cannot drift between the page an option belongs to and the list of
 * every option. The generic schema row ([SchemaGroupRows]' branch per type)
 * is only for options no page customizes; [SettingsOptionRows.customIds]
 * names the rest.
 */
object SettingsOptionRows {

	/**
	 * The options whose row a page defines: a device list the schema cannot
	 * carry (the joyports), a visibility or availability rule of its own (the
	 * touch controls, the mouse map, the FPU list, the keypad), a coupled
	 * pair (integer scaling), a launcher list (the shader, the drive sounds)
	 * or a labelled choice list (the sound buffer's latency).
	 */
	val customIds: Set<String> = setOf(
		"joyport0", "joyport1", "joyport2", "joyport3", "joyport1mousemap",
		"input.joymouse_speed_analog", "amiberry.onscreen_joystick", "amiberry.vkbd_touch_button",
		"amiberry.vkbd_numpad", "amiberry.shader", "amiberry.shader_rtg", "amiberry.custom_bezel",
		"amiberry.scaling_method", "cart_file", "genlock_image", "genlock_video", "rtc_file", "gfxcard_type", "gfxcard_size",
		"zz9000_net", "fpu_model", "chipmem_size", "bogomem_size", "sound_max_buff",
		// The accelerator's board and memory: the schema's catalogs carry
		// the list and the limit, not the option's choices. The boot ROM
		// stays in the Accelerator section alone: no row of its own shows a
		// path in All Settings.
		"cpuboard_type", "cpuboardmem1_size"
	) + (0..3).map { "floppy${it}sound" }

	/**
	 * Whether the option's row shows at all: the pages hide these until they
	 * mean something (the launcher's constraints reset the touch controls on
	 * a device without a touchscreen; the analog mouse map needs a port-1
	 * controller with the system mouse kept on port 0).
	 */
	fun shows(id: String, settings: EmulatorSettings, hasTouchScreen: Boolean): Boolean = when (id) {
		"amiberry.onscreen_joystick", "amiberry.vkbd_touch_button" -> hasTouchScreen
		"joyport1mousemap", "input.joymouse_speed_analog" ->
			portSupportsMouseMap(1, settings.joyport1, settings.joyport0)
		else -> true
	}
}

/** RTG boards fitting the addressing and chip RAM limits of type_fits(). */
internal fun rtgBoardChoices(schema: SettingsSchema, settings: EmulatorSettings): List<SettingsSchema.Choice> =
	schema.rtgBoards.filter { board ->
		(board.bus != 3 || !settings.address24Bit) &&
			(board.bus != 2 || EmulatorSettings.chipRamKb(settings.chipRam) <= 2048)
	}.map { SettingsSchema.Choice(it.id, it.label) }

/**
 * The row for option [id] as the page that owns it shows it: the page's own
 * row where [SettingsOptionRows.customIds] names one, else the generic row of
 * the option's type. Rows [SettingsOptionRows.shows] hides render nothing.
 */
@Composable
fun SettingsOptionRow(id: String, viewModel: SettingsViewModel, sectionTitle: String? = null) {
	val schema = settingsSchema() ?: return
	val option = schema.option(id) ?: return
	val settings = viewModel.settings
	val state = remember(settings, schema) { settings.schemaState(schema) }
	val touch = hasTouchScreen()
	if (!SettingsOptionRows.shows(id, settings, touch)) return
	fun available(candidate: String) = SettingsControlAvailability.isAvailable(state, candidate, schema)

	when (id) {
		// The joyports: the schema has no device list (the layer enumerates
		// the host's devices at run time), so the launcher offers its own.
		"joyport0" -> SchemaChoiceRow(
			id = id,
			value = settings.joyport0,
			choices = inputDeviceOptionIds(touch, portIndex = 0).map { token ->
				SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
			},
			testTag = "input-port0-dropdown",
			optionTestTag = { "input-port0-option-$it" },
			onSelect = { value -> viewModel.updateSettings { s -> s.copy(joyport0 = value) } }
		)
		"joyport1" -> SchemaChoiceRow(
			id = id,
			value = settings.joyport1,
			choices = inputDeviceOptionIds(touch, portIndex = 1).map { token ->
				SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
			},
			testTag = "input-port1-dropdown",
			optionTestTag = { "input-port1-option-$it" },
			onSelect = { value -> viewModel.updateSettings { s -> InputSettingsActions.selectPort1Device(s, value) } }
		)
		"joyport2", "joyport3" -> ParallelJoyportRow(id, viewModel, touch)
		"joyport1mousemap" -> SchemaChoiceRow(
			id = id,
			value = if (settings.joyport1MouseMap) "1" else "0",
			onSelect = { value ->
				viewModel.updateSettings { s -> InputSettingsActions.setPortMouseMap(s, 1, value == "1") }
			}
		)
		"input.joymouse_speed_analog" -> SchemaLevelRow(
			id = id,
			stored = settings.analogMouseSpeed,
			enabled = settings.joyport1MouseMap,
			testTag = "input-analog-mouse-speed-slider",
			onStoredChange = { speed -> viewModel.updateSettings { s -> s.copy(analogMouseSpeed = speed) } }
		)
		// The on-screen controls: the touch ones mean nothing (and the
		// constraints reset them) without a touchscreen; the keypad needs a
		// way to open the keyboard at all.
		"amiberry.onscreen_joystick" -> SchemaSwitchRow(
			id = id,
			checked = settings.onScreenJoystick,
			onCheckedChange = { isEnabled ->
				viewModel.updateSettings { s -> InputSettingsActions.setOnScreenJoystick(s, isEnabled) }
			}
		)
		"amiberry.vkbd_touch_button" -> SchemaSwitchRow(
			id = id,
			checked = settings.onScreenKeyboardButton,
			enabled = settings.onScreenJoystick,
			reason = stringResource(R.string.settings_input_needs_on_screen_joystick),
			onCheckedChange = {
				viewModel.updateSettings { s -> s.copy(onScreenKeyboardButton = it) }
			}
		)
		"amiberry.vkbd_numpad" -> SchemaSwitchRow(
			id = id,
			checked = settings.onScreenKeyboardNumpad,
			enabled = settings.onScreenKeyboardAvailable,
			onCheckedChange = {
				viewModel.updateSettings { s -> s.copy(onScreenKeyboardNumpad = it) }
			}
		)
		// Files outside the launcher's scanned libraries need the device picker,
		// which retains the selected storage path in the .uae file.
		"cart_file", "genlock_image", "genlock_video", "rtc_file" -> DevicePathRow(id, viewModel)
		// The shader and bezel lists come from their configured folders, not the schema.
		"amiberry.shader", "amiberry.shader_rtg" -> ShaderDropdownRow(viewModel, id)
		"amiberry.custom_bezel" -> BezelDropdownRow(viewModel)
		// The RTG board list is dynamic in the settings layer, so the schema
		// exports its catalog separately from gfxcard_type's descriptor.
		"gfxcard_type" -> {
			val boards = rtgBoardChoices(schema, settings)
			SchemaChoiceRow(
				id = id,
				value = settings.optionValue(id) ?: option.default,
				choices = boards,
				enabled = available(id),
				onSelect = { value -> viewModel.updateSettings { s -> s.withOption(id, value) } }
			)
		}
		"gfxcard_size" -> {
			val board = schema.rtgBoards.firstOrNull { it.id == (settings.optionValue("gfxcard_type") ?: "ZorroIII") }
			val choices = option.choices.filter { choice ->
				val size = choice.value.toIntOrNull() ?: return@filter false
				size == 0 || board == null || size in board.minMb..board.maxMb
			}
			SchemaChoiceRow(
				id = id,
				value = settings.optionValue(id) ?: option.default,
				choices = choices,
				onSelect = { value -> viewModel.updateSettings { s -> s.withOption(id, value) } }
			)
		}
		// Android has no selectable host ethernet backend. A retained
		// desktop backend remains visible until the user disconnects it.
		"zz9000_net" -> {
			val value = settings.optionValue(id) ?: "none"
			val choices = listOf(SettingsSchema.Choice("none", stringResource(R.string.settings_option_none))) +
				if (value == "none") emptyList() else listOf(SettingsSchema.Choice(value, value))
			SchemaChoiceRow(
				id = id,
				value = value,
				choices = choices,
				onSelect = { selected -> viewModel.updateSettings { s -> s.withOption(id, selected) } }
			)
		}
		"amiberry.scaling_method" -> IntegerScalingRow(viewModel)
		// A single offered FPU is no choice (an internal one the CPU carries).
		"fpu_model" -> {
			val values = SettingsControlAvailability.choiceValues(state, id, schema)
			SchemaChoiceRow(
				id = id,
				value = settings.fpuModel.toString(),
				values = values,
				enabled = values == null || values.size > 1,
				onSelect = { value ->
					value.toIntOrNull()?.let { fpu -> viewModel.updateSettings { s -> s.copy(fpuModel = fpu) } }
				}
			)
		}
		// Memory sizes: the layer's availability table rules sizes out; the
		// option's constraint (in the help) says why while some are missing.
		"chipmem_size", "bogomem_size" -> MemorySizeRow(settings, id, if (id == "chipmem_size") settings.chipRam else settings.slowRam) { size ->
			viewModel.updateSettings { s -> if (id == "chipmem_size") s.copy(chipRam = size) else s.copy(slowRam = size) }
		}
		// The buffer sizes, each with the latency it gives at the current
		// frequency (one emulator chunk is sound_max_buff / 4 frames).
		"sound_max_buff" -> {
			val bufferFreq = if (settings.soundFreq > 0) settings.soundFreq else EmulatorSettingsConstraints.DEFAULT_SOUND_FREQUENCY
			val bufferChoices = option.choices.map { choice ->
				val size = choice.value.toIntOrNull() ?: 0
				if (size <= 0) {
					choice
				} else {
					SettingsSchema.Choice(
						choice.value,
						stringResource(R.string.settings_sound_buffer_option, choice.label, (size / 4.0 * 1000.0 / bufferFreq).roundToInt())
					)
				}
			}
			SchemaChoiceRow(
				id = id,
				value = settings.soundBuffer.toString(),
				choices = bufferChoices.takeIf { it.isNotEmpty() },
				enabled = available(id),
				showReason = false,
				onSelect = { value ->
					value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(soundBuffer = size) } }
				}
			)
		}
		// The accelerator board's rows (AcceleratorBoardSection's).
		"cpuboard_type" -> AcceleratorTypeRow(viewModel)
		"cpuboardmem1_size" -> settingsSchema()?.acceleratorBoard(
			settings.optionValue("cpuboard_type") ?: "none"
		)?.let { board -> AcceleratorMemoryRow(viewModel, board) }
		"cpuboard_rom_file" -> AcceleratorRomRow(viewModel)
		// The drive sounds offered: off and the built-in A500 drive (external
		// samples are kept when stored, not offered).
		else -> if (id in OFFERED_DRIVE_SOUNDS) {
			SchemaChoiceRow(
				id = id,
				value = driveSoundValue(id, settings).toString(),
				values = listOf("0", "1"),
				enabled = available(id),
				showReason = false,
				onSelect = { value ->
					value.toIntOrNull()?.let { sound ->
						viewModel.updateSettings { s -> withDriveSound(s, id, sound) }
					}
				}
			)
		} else {
			// The generic row of the option's type, named in its section's words.
			SchemaGroupRows(
				options = listOf(option),
				settings = settings,
				state = state,
				schema = schema,
				showReasons = false,
				sectionTitle = sectionTitle
			) { changed, value ->
				viewModel.updateSettings { s -> s.withOption(changed, value) }
			}
		}
	}
}

/**
 * A path option selected from Android shared storage. It is deliberately not
 * an editable field: Android users cannot reasonably type a storage path.
 */
@Composable
private fun DevicePathRow(id: String, viewModel: SettingsViewModel) {
	val selectedPath = viewModel.settings.optionValue(id).orEmpty()
	val picker = rememberImportPicker(
		mimeTypes = arrayOf("*/*"),
		extensions = emptySet()
	) { uris ->
		val path = uris.firstOrNull()?.path ?: return@rememberImportPicker
		viewModel.updateSettings { it.withOption(id, path) }
	}
	Column(modifier = Modifier.fillMaxWidth()) {
		Row(verticalAlignment = Alignment.CenterVertically) {
			OutlinedTextField(
				value = selectedPath.takeIf { it.isNotBlank() }?.let(LauncherChoices::fileName)
					?: stringResource(R.string.placeholder_none),
				onValueChange = {},
				readOnly = true,
				label = { Text(schemaLabel(id)) },
				modifier = Modifier.weight(1f)
			)
			TextButton(onClick = { picker.browseDevice() }, modifier = Modifier.dpadFocusIndicator()) {
				Icon(Icons.Default.FolderOpen, contentDescription = null)
				Text(stringResource(R.string.settings_file_choose))
			}
			if (selectedPath.isNotBlank()) {
				IconButton(
					onClick = { viewModel.updateSettings { it.withOption(id, "") } },
					modifier = Modifier.dpadFocusIndicator()
				) {
					Icon(
						Icons.Default.Eject,
						contentDescription = stringResource(
							R.string.action_eject_named,
							LauncherChoices.fileName(selectedPath)
						)
					)
				}
			}
		}
		schemaOption(id)?.help?.takeIf { it.isNotBlank() }?.let { SupportingText(it) }
	}
}

/** Parallel-port joyports use the same Android device choices as the native ports. */
@Composable
private fun ParallelJoyportRow(id: String, viewModel: SettingsViewModel, touch: Boolean) {
	val settings = viewModel.settings
	val portIndex = id.removePrefix("joyport").toIntOrNull() ?: return
	SchemaChoiceRow(
		id = id,
		value = settings.optionValue(id) ?: "none",
		choices = inputDeviceOptionIds(touch, portIndex).map { token ->
			SettingsSchema.Choice(token, settingText(LauncherChoices.portDevice(token)))
		},
		onSelect = { value -> viewModel.updateSettings { it.withOption(id, value) } }
	)
}

/** The drive sounds offered, by option id. */
private val OFFERED_DRIVE_SOUNDS = (0..3).map { "floppy${it}sound" }.toSet()

private fun driveSoundIndex(id: String): Int = id.removePrefix("floppy").removeSuffix("sound").toIntOrNull() ?: 0

private fun driveSoundValue(id: String, settings: EmulatorSettings): Int = when (driveSoundIndex(id)) {
	0 -> settings.floppy0Sound
	1 -> settings.floppy1Sound
	2 -> settings.floppy2Sound
	else -> settings.floppy3Sound
}

private fun withDriveSound(settings: EmulatorSettings, id: String, sound: Int): EmulatorSettings = when (driveSoundIndex(id)) {
	0 -> settings.copy(floppy0Sound = sound)
	1 -> settings.copy(floppy1Sound = sound)
	2 -> settings.copy(floppy2Sound = sound)
	else -> settings.copy(floppy3Sound = sound)
}

/**
 * A memory size offering the sizes the machine allows (the layer's
 * availability table: the chipset extra's boards and the JIT rule some
 * out). While some are left out the option's constraint says why.
 */
@Composable
internal fun MemorySizeRow(settings: EmulatorSettings, id: String, stored: Int, onSelect: (Int) -> Unit) {
	val values = SettingsControlAvailability.choiceValues(settings, id)
	val option = schemaOption(id)
	val narrowed = values != null && option != null && values.size < option.choices.size
	SchemaChoiceRow(
		id = id,
		value = stored.toString(),
		values = values,
		help = if (narrowed) listOfNotNull(option?.help?.takeIf { it.isNotBlank() }, option?.constraint).joinToString("\n") else null,
		onSelect = { value -> value.toIntOrNull()?.let(onSelect) }
	)
}
