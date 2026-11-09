package com.blitterstudio.amiberry.ui.screens.settings

import android.os.Build
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.LauncherChoices
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SupportingText
import com.blitterstudio.amiberry.ui.components.settings.dpadMenuItemInitialFocus
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.settingText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.dpadArrowsMoveFocus
import com.blitterstudio.amiberry.ui.dpadFieldFocusIndicator
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlin.math.roundToInt

/** One floppy drive's sound fields, read and written by drive number. */
private class DriveSound(
	val index: Int,
	val type: (EmulatorSettings) -> Int,
	val sound: (EmulatorSettings) -> Int,
	val volumeDisk: (EmulatorSettings) -> Int,
	val volumeEmpty: (EmulatorSettings) -> Int,
	val withSound: EmulatorSettings.(Int) -> EmulatorSettings,
	val withVolumeDisk: EmulatorSettings.(Int) -> EmulatorSettings,
	val withVolumeEmpty: EmulatorSettings.(Int) -> EmulatorSettings
)

private val driveSounds = listOf(
	DriveSound(0, { it.floppy0Type }, { it.floppy0Sound }, { it.floppy0SoundVolumeDisk }, { it.floppy0SoundVolumeEmpty },
		{ copy(floppy0Sound = it) }, { copy(floppy0SoundVolumeDisk = it) }, { copy(floppy0SoundVolumeEmpty = it) }),
	DriveSound(1, { it.floppy1Type }, { it.floppy1Sound }, { it.floppy1SoundVolumeDisk }, { it.floppy1SoundVolumeEmpty },
		{ copy(floppy1Sound = it) }, { copy(floppy1SoundVolumeDisk = it) }, { copy(floppy1SoundVolumeEmpty = it) }),
	DriveSound(2, { it.floppy2Type }, { it.floppy2Sound }, { it.floppy2SoundVolumeDisk }, { it.floppy2SoundVolumeEmpty },
		{ copy(floppy2Sound = it) }, { copy(floppy2SoundVolumeDisk = it) }, { copy(floppy2SoundVolumeEmpty = it) }),
	DriveSound(3, { it.floppy3Type }, { it.floppy3Sound }, { it.floppy3SoundVolumeDisk }, { it.floppy3SoundVolumeEmpty },
		{ copy(floppy3Sound = it) }, { copy(floppy3SoundVolumeDisk = it) }, { copy(floppy3SoundVolumeEmpty = it) })
)

/** The drive sounds offered: off and the built-in A500 drive (external samples are kept when stored, not offered). */
private val OFFERED_DRIVE_SOUNDS = listOf("0", "1")

/**
 * The Display & Sound page: the display settings people reach for (the
 * launcher's own card), then the layer's remaining display and filter rows
 * and the sound sections.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DisplaySoundPage(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val lifecycleOwner = LocalLifecycleOwner.current

	LaunchedEffect(viewModel) {
		viewModel.refreshShaderCatalog()
	}
	DisposableEffect(lifecycleOwner, viewModel) {
		val observer = LifecycleEventObserver { _, event ->
			if (event == Lifecycle.Event.ON_RESUME) {
				viewModel.refreshShaderCatalog()
			}
		}
		lifecycleOwner.lifecycle.addObserver(observer)
		onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
	}

	val resolutionPresets = listOf(
		Triple(640, 256, stringResource(R.string.settings_display_resolution_pal_low)),
		Triple(640, 512, stringResource(R.string.settings_display_resolution_pal_high)),
		Triple(720, 568, stringResource(R.string.settings_display_resolution_default)),
		Triple(800, 600, stringResource(R.string.settings_display_resolution_800_600)),
		Triple(1024, 768, stringResource(R.string.settings_display_resolution_1024_768))
	)

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_display_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				// Resolution preset
				var resExpanded by remember { mutableStateOf(false) }
				val currentRes = "${settings.gfxWidth}x${settings.gfxHeight}"
				val currentLabel = resolutionPresets.firstOrNull {
					it.first == settings.gfxWidth && it.second == settings.gfxHeight
				}?.third ?: currentRes

				ExposedDropdownMenuBox(
					expanded = resExpanded,
					onExpandedChange = { resExpanded = it }
				) {
					OutlinedTextField(
						value = currentLabel,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_display_resolution_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = resExpanded) },
						modifier = Modifier
							.dpadFieldFocusIndicator()
							.dpadArrowsMoveFocus()
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = resExpanded,
						onDismissRequest = { resExpanded = false }
					) {
						resolutionPresets.forEach { (w, h, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								modifier = Modifier.dpadMenuItemInitialFocus(w == settings.gfxWidth && h == settings.gfxHeight),
								onClick = {
									viewModel.updateSettings { s ->
										s.copy(gfxWidth = w, gfxHeight = h)
									}
									resExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				SchemaSwitchRow(
					id = "amiberry.gfx_correct_aspect",
					checked = settings.correctAspect,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(correctAspect = it) }
					}
				)
				SchemaSwitchRow(
					id = "amiberry.gfx_auto_crop",
					checked = settings.autoCrop,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(autoCrop = it) }
					}
				)
				IntegerScalingRow(viewModel)

				Spacer(modifier = Modifier.height(8.dp))

				ShaderDropdownRow(viewModel)
			}
		}

		SchemaGroupSection("display", viewModel, title = stringResource(R.string.settings_display_more_title))
		SchemaGroupSection("filter", viewModel)

		SoundSections(viewModel)

		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_display_app_theme), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				val context = LocalContext.current
				val appPrefs = AppPreferences.getInstance(context)

				// Theme mode selector
				val themeMode by appPrefs.themeMode
				val themeModeOptions = listOf(
					"system" to stringResource(R.string.settings_display_theme_system),
					"light" to stringResource(R.string.settings_display_theme_light),
					"dark" to stringResource(R.string.settings_display_theme_dark)
				)
				var themeExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = themeExpanded,
					onExpandedChange = { themeExpanded = it }
				) {
					OutlinedTextField(
						value = themeModeOptions.firstOrNull { it.first == themeMode }?.second ?: themeMode,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_display_theme_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = themeExpanded) },
						modifier = Modifier
							.dpadFieldFocusIndicator()
							.dpadArrowsMoveFocus()
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = themeExpanded,
						onDismissRequest = { themeExpanded = false }
					) {
						themeModeOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								modifier = Modifier.dpadMenuItemInitialFocus(value == themeMode),
								onClick = {
									appPrefs.setThemeMode(value)
									themeExpanded = false
								}
							)
						}
					}
				}

				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
					Spacer(modifier = Modifier.height(8.dp))
					val useDynamicColor by appPrefs.useDynamicColor
					SettingSwitchRow(
						label = stringResource(R.string.settings_display_dynamic_color),
						checked = useDynamicColor,
						onCheckedChange = { appPrefs.setDynamicColor(it) }
					)
				}
			}
		}
	}
}

/** The sound sections: output, volumes and the drives' sounds (the drive types are on the Storage page). */
@Composable
private fun SoundSections(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	fun available(id: String) = SettingsControlAvailability.isAvailable(settings, id)
	val soundOn = available("sound_channels")
	// The reason every option that needs sound emulation shares, said once per section.
	val needsSound = if (soundOn) null else settingsSchema()?.constraint("sound_channels")

	// The layer's buffer sizes, each with the latency it gives at the current
	// frequency (one emulator chunk is sound_max_buff / 4 frames).
	val bufferFreq = if (settings.soundFreq > 0) settings.soundFreq else EmulatorSettingsConstraints.DEFAULT_SOUND_FREQUENCY
	val bufferChoices = schemaOption("sound_max_buff")?.choices.orEmpty().map { choice ->
		val size = choice.value.toIntOrNull() ?: 0
		if (size <= 0) choice
		else SettingsSchema.Choice(
			choice.value,
			stringResource(R.string.settings_sound_buffer_option, choice.label, (size / 4.0 * 1000.0 / bufferFreq).roundToInt())
		)
	}

	Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
		SchemaGroupSection("sound", viewModel, title = stringResource(R.string.settings_sound_title), footer = needsSound) {
			SchemaChoiceRow(
				id = "sound_output",
				value = settings.soundOutput,
				values = SettingsControlAvailability.choiceValues(settings, "sound_output"),
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(soundOutput = value) } }
			)
			SchemaChoiceRow(
				id = "sound_channels",
				value = settings.soundChannels,
				enabled = available("sound_channels"),
				showReason = false,
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(soundChannels = value) } }
			)
			SchemaChoiceRow(
				id = "sound_frequency",
				value = settings.soundFreq.toString(),
				enabled = available("sound_frequency"),
				showReason = false,
				onSelect = { value -> value.toIntOrNull()?.let { freq -> viewModel.updateSettings { s -> s.copy(soundFreq = freq) } } }
			)
			SchemaChoiceRow(
				id = "sound_max_buff",
				value = settings.soundBuffer.toString(),
				choices = bufferChoices.takeIf { it.isNotEmpty() },
				enabled = available("sound_max_buff"),
				showReason = false,
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(soundBuffer = size) } } }
			)
		}

		SettingsSection(title = stringResource(R.string.settings_sound_volume_title), footer = needsSound) {
			SchemaLevelRow(
				id = "sound_volume",
				stored = settings.soundVolume,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolume = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_paula",
				stored = settings.soundVolumePaula,
				enabled = available("sound_volume_paula"),
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumePaula = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_cd",
				stored = settings.soundVolumeCd,
				enabled = available("sound_volume_cd"),
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumeCd = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_ahi",
				stored = settings.soundVolumeAhi,
				enabled = available("sound_volume_ahi"),
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumeAhi = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_midi",
				stored = settings.soundVolumeMidi,
				enabled = available("sound_volume_midi"),
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumeMidi = v) } }
			)
		}

		// Drive sounds for the drives that are enabled (drive types are set on the Storage page).
		val drives = driveSounds.filter { it.type(settings) != -1 }
		if (drives.isNotEmpty()) {
			SettingsSection(title = stringResource(R.string.settings_sound_drive_sounds_title), footer = needsSound) {
				drives.forEach { drive ->
					val volumesAvailable = available("floppy${drive.index}soundvolume_disk")
					SchemaChoiceRow(
						id = "floppy${drive.index}sound",
						value = drive.sound(settings).toString(),
						values = OFFERED_DRIVE_SOUNDS,
						enabled = available("floppy${drive.index}sound"),
						showReason = false,
						onSelect = { value ->
							value.toIntOrNull()?.let { sound -> viewModel.updateSettings { s -> drive.withSound(s, sound) } }
						}
					)
					SchemaLevelRow(
						id = "floppy${drive.index}soundvolume_disk",
						stored = drive.volumeDisk(settings),
						enabled = volumesAvailable,
						showReason = false,
						onStoredChange = { v -> viewModel.updateSettings { s -> drive.withVolumeDisk(s, v) } }
					)
					SchemaLevelRow(
						id = "floppy${drive.index}soundvolume_empty",
						stored = drive.volumeEmpty(settings),
						enabled = available("floppy${drive.index}soundvolume_empty"),
						showReason = soundOn,
						onStoredChange = { v -> viewModel.updateSettings { s -> drive.withVolumeEmpty(s, v) } }
					)
				}
			}
		}
	}
}

/**
 * The native screen shader as a dropdown of the catalog's entries (the
 * built-ins and the shaders folder scan), the configured shader named and
 * marked when the catalog has lost it. Shared by the Display & Sound page
 * and All Settings, so both offer the shader the same way.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
internal fun ShaderDropdownRow(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	var shaderExpanded by remember { mutableStateOf(false) }
	val shaderUnavailable =
		viewModel.shaderCatalogStatus == SettingsViewModel.ShaderCatalogStatus.LOADED &&
			settings.shader !in viewModel.shaderCatalogEntries
	val unavailableLabel = stringResource(R.string.settings_display_shader_unavailable)
	val shaderName = settingText(LauncherChoices.shader(settings.shader))
	val shaderDisplayValue = if (shaderUnavailable) {
		stringResource(
			R.string.settings_display_shader_unavailable_value,
			shaderName,
			unavailableLabel
		)
	} else {
		shaderName
	}
	val shaderSupportingText = stringResource(
		if (shaderUnavailable) {
			R.string.settings_display_shader_unavailable_help
		} else {
			R.string.settings_display_shader_help
		}
	)

	ExposedDropdownMenuBox(
		expanded = shaderExpanded,
		onExpandedChange = { shaderExpanded = it }
	) {
		OutlinedTextField(
			value = shaderDisplayValue,
			onValueChange = {},
			readOnly = true,
			label = { Text(stringResource(R.string.settings_display_shader_label)) },
			isError = shaderUnavailable,
			trailingIcon = {
				ExposedDropdownMenuDefaults.TrailingIcon(expanded = shaderExpanded)
			},
			modifier = Modifier
				.dpadFieldFocusIndicator()
				.dpadArrowsMoveFocus()
				.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
				.fillMaxWidth()
		)
		ExposedDropdownMenu(
			expanded = shaderExpanded,
			onDismissRequest = { shaderExpanded = false }
		) {
			viewModel.shaderCatalogEntries.forEach { shader ->
				DropdownMenuItem(
					text = { Text(settingText(LauncherChoices.shader(shader))) },
					modifier = Modifier.dpadMenuItemInitialFocus(shader == settings.shader),
					onClick = {
						if (shader != settings.shader) {
							viewModel.updateSettings { s -> s.copy(shader = shader) }
						}
						shaderExpanded = false
					}
				)
			}
			if (shaderUnavailable) {
				DropdownMenuItem(
					text = { Text(shaderDisplayValue) },
					onClick = {},
					enabled = false
				)
			}
		}
	}
	// Under the field like every other row's help, not inside it (Material indents that).
	SupportingText(shaderSupportingText)
}

/**
 * Integer scaling as one switch: the scaling method and the resolution
 * autoswitch the launcher's constraints keep together — turning either
 * alone off is restored from the other, so no page may edit one half of
 * the pair by itself. Shared by the Display & Sound page and All Settings.
 */
@Composable
internal fun IntegerScalingRow(viewModel: SettingsViewModel) {
	SettingSwitchRow(
		label = stringResource(R.string.settings_display_integer_scaling),
		checked = viewModel.settings.scalingMethod == 2 && viewModel.settings.gfxAutoresolution == 1,
		onCheckedChange = { on ->
			viewModel.updateSettings { s ->
				if (on) s.copy(scalingMethod = 2, gfxAutoresolution = 1)
				else s.copy(scalingMethod = -1, gfxAutoresolution = 0)
			}
		},
		help = stringResource(R.string.settings_display_integer_scaling_desc)
	)
}
