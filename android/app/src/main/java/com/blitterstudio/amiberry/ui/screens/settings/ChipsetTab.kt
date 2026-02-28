package com.blitterstudio.amiberry.ui.screens.settings

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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ChipsetTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val chipsetLabels = mapOf(
		"ocs" to stringResource(R.string.settings_chipset_ocs),
		"ecs_agnus" to stringResource(R.string.settings_chipset_ecs_agnus),
		"ecs_denise" to stringResource(R.string.settings_chipset_ecs_denise),
		"ecs" to stringResource(R.string.settings_chipset_full_ecs),
		"aga" to stringResource(R.string.settings_chipset_aga)
	)
	val collisionOptions = listOf(
		"none" to stringResource(R.string.settings_option_none),
		"sprites" to stringResource(R.string.settings_chipset_collision_sprites_only),
		"playfields" to stringResource(R.string.settings_chipset_collision_sprites_playfields),
		"full" to stringResource(R.string.settings_chipset_collision_full)
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
				Text(stringResource(R.string.settings_chipset_section_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				var chipsetExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = chipsetExpanded,
					onExpandedChange = { chipsetExpanded = it }
				) {
					val displayName = chipsetLabels[settings.chipset] ?: settings.chipset
					OutlinedTextField(
						value = displayName,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_chipset_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = chipsetExpanded) },
						modifier = Modifier
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = chipsetExpanded,
						onDismissRequest = { chipsetExpanded = false }
					) {
						EmulatorSettings.chipsetOptions.forEach { (value, fallbackLabel) ->
							DropdownMenuItem(
								text = { Text(chipsetLabels[value] ?: fallbackLabel) },
								onClick = {
									viewModel.updateSettings { s -> s.copy(chipset = value) }
									chipsetExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				SwitchRow(
					label = stringResource(R.string.settings_chipset_immediate_blitter),
					checked = settings.immediateBlits,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(immediateBlits = it) } }
				)

				SwitchRow(
					label = stringResource(R.string.settings_chipset_cycle_exact),
					checked = settings.cycleExact,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(cycleExact = it) } }
				)

				SwitchRow(
					label = stringResource(R.string.settings_chipset_ntsc),
					checked = settings.ntsc,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(ntsc = it) } }
				)

				// Collision level
				var collisionExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = collisionExpanded,
					onExpandedChange = { collisionExpanded = it }
				) {
					val displayName = collisionOptions.firstOrNull { it.first == settings.collisionLevel }?.second ?: settings.collisionLevel
					OutlinedTextField(
						value = displayName,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_chipset_collision_level_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = collisionExpanded) },
						modifier = Modifier
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = collisionExpanded,
						onDismissRequest = { collisionExpanded = false }
					) {
						collisionOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s -> s.copy(collisionLevel = value) }
									collisionExpanded = false
								}
							)
						}
					}
				}
			}
		}
	}
}
