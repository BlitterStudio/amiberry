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
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MenuAnchorType
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MemoryTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Memory Configuration", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(12.dp))

				// Chip RAM
				MemoryDropdown(
					label = "Chip RAM",
					options = EmulatorSettings.chipRamOptions,
					selectedValue = settings.chipRam,
					onValueChange = { viewModel.updateSettings { s -> s.copy(chipRam = it) } }
				)

				Spacer(modifier = Modifier.height(8.dp))

				// Slow RAM
				MemoryDropdown(
					label = "Slow RAM (Trapdoor)",
					options = EmulatorSettings.slowRamOptions,
					selectedValue = settings.slowRam,
					onValueChange = { viewModel.updateSettings { s -> s.copy(slowRam = it) } }
				)

				Spacer(modifier = Modifier.height(8.dp))

				// Fast RAM (Z2)
				MemoryDropdown(
					label = "Fast RAM (Zorro II)",
					options = EmulatorSettings.fastRamOptions,
					selectedValue = settings.fastRam,
					onValueChange = { viewModel.updateSettings { s -> s.copy(fastRam = it) } }
				)

				// Z3 RAM (only for 32-bit addressing)
				if (!settings.address24Bit) {
					Spacer(modifier = Modifier.height(8.dp))

					MemoryDropdown(
						label = "Z3 Fast RAM (Zorro III)",
						options = EmulatorSettings.z3RamOptions,
						selectedValue = settings.z3Ram,
						onValueChange = { viewModel.updateSettings { s -> s.copy(z3Ram = it) } }
					)
				}
			}
		}

		// Summary card
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text("Summary", style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				val chipKB = settings.chipRam * 512
				val slowKB = settings.slowRam * 256
				val fastMB = settings.fastRam
				val z3MB = settings.z3Ram

				Text("Chip: ${formatMemory(chipKB)}", style = MaterialTheme.typography.bodyMedium)
				if (slowKB > 0) Text("Slow: ${formatMemory(slowKB)}", style = MaterialTheme.typography.bodyMedium)
				if (fastMB > 0) Text("Z2 Fast: ${fastMB} MB", style = MaterialTheme.typography.bodyMedium)
				if (z3MB > 0) Text("Z3 Fast: ${z3MB} MB", style = MaterialTheme.typography.bodyMedium)

				val totalMB = (chipKB + slowKB) / 1024.0 + fastMB + z3MB
				Spacer(modifier = Modifier.height(4.dp))
				Text(
					"Total: ${String.format("%.1f", totalMB)} MB",
					style = MaterialTheme.typography.titleSmall,
					color = MaterialTheme.colorScheme.primary
				)
			}
		}
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun MemoryDropdown(
	label: String,
	options: List<Pair<Int, String>>,
	selectedValue: Int,
	onValueChange: (Int) -> Unit
) {
	var expanded by remember { mutableStateOf(false) }
	val displayText = options.firstOrNull { it.first == selectedValue }?.second
		?: options.firstOrNull()?.second ?: "?"

	ExposedDropdownMenuBox(
		expanded = expanded,
		onExpandedChange = { expanded = it }
	) {
		OutlinedTextField(
			value = displayText,
			onValueChange = {},
			readOnly = true,
			label = { Text(label) },
			trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
			modifier = Modifier
				.menuAnchor(MenuAnchorType.PrimaryNotEditable)
				.fillMaxWidth()
		)
		ExposedDropdownMenu(
			expanded = expanded,
			onDismissRequest = { expanded = false }
		) {
			options.forEach { (value, text) ->
				DropdownMenuItem(
					text = { Text(text) },
					onClick = {
						onValueChange(value)
						expanded = false
					}
				)
			}
		}
	}
}

private fun formatMemory(kb: Int): String {
	return if (kb >= 1024) "${kb / 1024} MB" else "$kb KB"
}
