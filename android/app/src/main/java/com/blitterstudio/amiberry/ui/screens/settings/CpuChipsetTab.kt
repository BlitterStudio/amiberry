package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MenuAnchorType
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
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun CpuChipsetTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Model preset
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Model Preset", style = MaterialTheme.typography.titleMedium)
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
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth(),
						supportingText = { Text("Applies default CPU, chipset, and memory settings") }
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
				Text("CPU", style = MaterialTheme.typography.titleMedium)
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
						label = { Text("CPU Model") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = cpuExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
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
					label = "Fastest Possible",
					checked = settings.cpuSpeed == "max",
					onCheckedChange = {
						viewModel.updateSettings { s ->
							s.copy(cpuSpeed = if (it) "max" else "real")
						}
					}
				)

				SwitchRow(
					label = "More Compatible",
					checked = settings.cpuCompatible,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(cpuCompatible = it) } }
				)

				SwitchRow(
					label = "24-bit Addressing",
					checked = settings.address24Bit,
					enabled = settings.cpuModel <= 68010,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(address24Bit = it) } }
				)

				// FPU
				if (settings.cpuModel >= 68020) {
					Spacer(modifier = Modifier.height(8.dp))
					var fpuExpanded by remember { mutableStateOf(false) }
					val fpuOptions = buildList {
						add(0 to "None")
						add(68881 to "68881")
						add(68882 to "68882")
						if (settings.cpuModel >= 68040) {
							add(settings.cpuModel to "CPU Internal")
						}
					}
					ExposedDropdownMenuBox(
						expanded = fpuExpanded,
						onExpandedChange = { fpuExpanded = it }
					) {
						OutlinedTextField(
							value = fpuOptions.firstOrNull { it.first == settings.fpuModel }?.second ?: "None",
							onValueChange = {},
							readOnly = true,
							label = { Text("FPU") },
							trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = fpuExpanded) },
							modifier = Modifier
								.menuAnchor(MenuAnchorType.PrimaryNotEditable)
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
					Text("JIT Cache (KB)", style = MaterialTheme.typography.bodyMedium)
					val jitValues = listOf(0, 1024, 2048, 4096, 8192, 16384)
					val currentIndex = jitValues.indexOf(settings.jitCacheSize).coerceAtLeast(0)
					Slider(
						value = currentIndex.toFloat(),
						onValueChange = { idx ->
							val size = jitValues[idx.toInt().coerceIn(0, jitValues.lastIndex)]
							viewModel.updateSettings { s -> s.copy(jitCacheSize = size) }
						},
						valueRange = 0f..(jitValues.lastIndex).toFloat(),
						steps = jitValues.size - 2
					)
					Text(
						text = if (settings.jitCacheSize == 0) "Disabled" else "${settings.jitCacheSize} KB",
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}
		}

		// Chipset
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Chipset", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				var chipsetExpanded by remember { mutableStateOf(false) }
				ExposedDropdownMenuBox(
					expanded = chipsetExpanded,
					onExpandedChange = { chipsetExpanded = it }
				) {
					val displayName = EmulatorSettings.chipsetOptions.firstOrNull { it.first == settings.chipset }?.second ?: settings.chipset
					OutlinedTextField(
						value = displayName,
						onValueChange = {},
						readOnly = true,
						label = { Text("Chipset") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = chipsetExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = chipsetExpanded,
						onDismissRequest = { chipsetExpanded = false }
					) {
						EmulatorSettings.chipsetOptions.forEach { (value, label) ->
							DropdownMenuItem(
								text = { Text(label) },
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
					label = "Immediate Blitter",
					checked = settings.immediateBlits,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(immediateBlits = it) } }
				)

				SwitchRow(
					label = "Cycle-Exact",
					checked = settings.cycleExact,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(cycleExact = it) } }
				)

				SwitchRow(
					label = "NTSC",
					checked = settings.ntsc,
					onCheckedChange = { viewModel.updateSettings { s -> s.copy(ntsc = it) } }
				)

				// Collision level
				var collisionExpanded by remember { mutableStateOf(false) }
				val collisionOptions = listOf(
					"none" to "None",
					"sprites" to "Sprites Only",
					"playfields" to "Sprites + Playfields",
					"full" to "Full"
				)
				ExposedDropdownMenuBox(
					expanded = collisionExpanded,
					onExpandedChange = { collisionExpanded = it }
				) {
					val displayName = collisionOptions.firstOrNull { it.first == settings.collisionLevel }?.second ?: settings.collisionLevel
					OutlinedTextField(
						value = displayName,
						onValueChange = {},
						readOnly = true,
						label = { Text("Collision Level") },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = collisionExpanded) },
						modifier = Modifier
							.menuAnchor(MenuAnchorType.PrimaryNotEditable)
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
			onCheckedChange = onCheckedChange,
			enabled = enabled
		)
	}
}
