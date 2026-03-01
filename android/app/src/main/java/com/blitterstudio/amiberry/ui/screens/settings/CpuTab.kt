package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun CpuTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Model preset
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_cpu_model_preset), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				var modelExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = modelExpanded,
					onExpandedChange = { modelExpanded = it }
				) {
					OutlinedTextField(
						value = settings.baseModel.displayName,
						onValueChange = {},
						readOnly = true,
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = modelExpanded) },
						modifier = Modifier
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth(),
						supportingText = {
							Text(stringResource(R.string.settings_cpu_model_preset_help))
						}
					)
					ExposedDropdownMenu(
						expanded = modelExpanded,
						onDismissRequest = { modelExpanded = false }
					) {
						AmigaModel.entries.forEach { m ->
							DropdownMenuItem(
								text = { Text("${m.displayName} - ${m.description}") },
								onClick = {
									viewModel.applyModel(m)
									modelExpanded = false
								}
							)
						}
					}
				}
			}
		}

		// CPU
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_cpu_section_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				// CPU model
				var cpuExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = cpuExpanded,
					onExpandedChange = { cpuExpanded = it }
				) {
					OutlinedTextField(
						value = "${settings.cpuModel}",
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_cpu_model_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = cpuExpanded) },
						modifier = Modifier
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = cpuExpanded,
						onDismissRequest = { cpuExpanded = false }
					) {
						EmulatorSettings.cpuModels.forEach { cpu ->
							DropdownMenuItem(
								text = { Text("$cpu") },
								onClick = {
									viewModel.updateSettings { it.copy(cpuModel = cpu) }
									cpuExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				// CPU speed
				SwitchRow(
					label = stringResource(R.string.settings_cpu_fastest_possible),
					checked = settings.cpuSpeed == "max",
					enabled = !settings.cycleExact,
					onCheckedChange = {
						viewModel.updateSettings { s ->
							s.copy(cpuSpeed = if (it) "max" else "real")
						}
					}
				)

				SwitchRow(
					label = stringResource(R.string.settings_cpu_more_compatible),
					checked = settings.cpuCompatible,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(cpuCompatible = it) } }
				)

				SwitchRow(
					label = stringResource(R.string.settings_cpu_24bit_addressing),
					checked = settings.address24Bit,
					enabled = settings.cpuModel <= 68010,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(address24Bit = it) } }
				)

				// FPU
				if (settings.cpuModel >= 68020) {
					Spacer(modifier = Modifier.height(8.dp))
					var fpuExpanded by remember { mutableStateOf(false) }
					val fpuOptions = buildList {
						add(0 to stringResource(R.string.settings_option_none))
						add(68881 to "68881")
						add(68882 to "68882")
						if (settings.cpuModel >= 68040) {
							add(settings.cpuModel to stringResource(R.string.settings_cpu_fpu_internal))
						}
					}
					ExposedDropdownMenuBox(
						expanded = fpuExpanded,
						onExpandedChange = { fpuExpanded = it }
					) {
						OutlinedTextField(
							value = fpuOptions.firstOrNull { it.first == settings.fpuModel }?.second
								?: stringResource(R.string.settings_option_none),
							onValueChange = {},
							readOnly = true,
							label = { Text(stringResource(R.string.settings_cpu_fpu_label)) },
							trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = fpuExpanded) },
							modifier = Modifier
								.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
								.fillMaxWidth()
						)
						ExposedDropdownMenu(
							expanded = fpuExpanded,
							onDismissRequest = { fpuExpanded = false }
						) {
							fpuOptions.forEach { (value, label) ->
								DropdownMenuItem(
									text = { Text(label) },
									onClick = {
										viewModel.updateSettings { s -> s.copy(fpuModel = value) }
										fpuExpanded = false
									}
								)
							}
						}
					}
				}

				// JIT
				if (settings.cpuModel >= 68020) {
					Spacer(modifier = Modifier.height(8.dp))
					Text(stringResource(R.string.settings_cpu_jit_cache_label), style = MaterialTheme.typography.bodyMedium)
					val jitValues = listOf(0, 1024, 2048, 4096, 8192, 16384)
					val currentIndex = jitValues.indexOf(settings.jitCacheSize).coerceAtLeast(0)
					val isJitAllowed = !settings.cycleExact && !settings.address24Bit
					Slider(
						value = currentIndex.toFloat(),
						onValueChange = { idx ->
							val size = jitValues[idx.toInt().coerceIn(0, jitValues.lastIndex)]
							viewModel.updateSettings { s -> s.copy(jitCacheSize = size) }
						},
						valueRange = 0f..(jitValues.lastIndex).toFloat(),
						steps = jitValues.size - 2,
						enabled = isJitAllowed
					)
					Text(
						text = if (settings.jitCacheSize == 0) {
							stringResource(R.string.settings_common_disabled)
						} else {
							stringResource(R.string.settings_cpu_jit_cache_value, settings.jitCacheSize)
						},
						style = MaterialTheme.typography.bodySmall,
						color = if (isJitAllowed) MaterialTheme.colorScheme.onSurfaceVariant else MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.5f)
					)
				}
			}
		}

	}
}

@Composable
fun SwitchRow(
	label: String,
	checked: Boolean,
	onCheckedChange: (Boolean) -> Unit,
	enabled: Boolean = true
) {
	Row(
		modifier = Modifier
			.fillMaxWidth()
			.toggleable(
				value = checked,
				enabled = enabled,
				role = Role.Switch,
				onValueChange = onCheckedChange
			)
			.padding(vertical = 4.dp),
		verticalAlignment = Alignment.CenterVertically,
		horizontalArrangement = Arrangement.SpaceBetween
	) {
		Text(
			text = label,
			style = MaterialTheme.typography.bodyMedium,
			color = if (enabled) MaterialTheme.colorScheme.onSurface
			else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.38f)
		)
		Switch(
			checked = checked,
			onCheckedChange = null,
			enabled = enabled
		)
	}
}
