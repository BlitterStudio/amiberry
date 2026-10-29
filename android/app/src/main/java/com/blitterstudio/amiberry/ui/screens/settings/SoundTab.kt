package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
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

@Composable
fun SoundTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val soundOn = SettingsControlAvailability.isSoundEmulationOn(settings)
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

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		SettingsSection(title = stringResource(R.string.settings_sound_title), footer = needsSound) {
			SchemaChoiceRow(
				id = "sound_output",
				value = settings.soundOutput,
				values = SettingsControlAvailability.soundOutputChoiceValues(settings),
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(soundOutput = value) } }
			)
			SchemaChoiceRow(
				id = "sound_channels",
				value = settings.soundChannels,
				enabled = soundOn,
				showReason = false,
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(soundChannels = value) } }
			)
			SchemaChoiceRow(
				id = "sound_frequency",
				value = settings.soundFreq.toString(),
				enabled = soundOn,
				showReason = false,
				onSelect = { value -> value.toIntOrNull()?.let { freq -> viewModel.updateSettings { s -> s.copy(soundFreq = freq) } } }
			)
			SchemaChoiceRow(
				id = "sound_max_buff",
				value = settings.soundBuffer.toString(),
				choices = bufferChoices.takeIf { it.isNotEmpty() },
				enabled = soundOn,
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
				enabled = soundOn,
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumePaula = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_cd",
				stored = settings.soundVolumeCd,
				enabled = soundOn,
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumeCd = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_ahi",
				stored = settings.soundVolumeAhi,
				enabled = soundOn,
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumeAhi = v) } }
			)
			SchemaLevelRow(
				id = "sound_volume_midi",
				stored = settings.soundVolumeMidi,
				enabled = soundOn,
				showReason = false,
				onStoredChange = { v -> viewModel.updateSettings { s -> s.copy(soundVolumeMidi = v) } }
			)
		}

		// Drive sounds for the drives that are enabled (drive types are set on the Storage tab).
		val drives = driveSounds.filter { it.type(settings) != -1 }
		if (drives.isNotEmpty()) {
			SettingsSection(title = stringResource(R.string.settings_sound_drive_sounds_title), footer = needsSound) {
				drives.forEach { drive ->
					val volumesAvailable = SettingsControlAvailability.isFloppySoundVolumeAvailable(settings, drive.index)
					SchemaChoiceRow(
						id = "floppy${drive.index}sound",
						value = drive.sound(settings).toString(),
						values = OFFERED_DRIVE_SOUNDS,
						enabled = soundOn,
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
						enabled = volumesAvailable,
						showReason = soundOn,
						onStoredChange = { v -> viewModel.updateSettings { s -> drive.withVolumeEmpty(s, v) } }
					)
				}
			}
		}
	}
}
