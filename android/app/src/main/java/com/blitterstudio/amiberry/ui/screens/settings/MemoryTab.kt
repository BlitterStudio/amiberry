package com.blitterstudio.amiberry.ui.screens.settings

import android.content.Context
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MemoryTab(viewModel: SettingsViewModel) {
	val context = LocalContext.current
	val settings = viewModel.settings
	val noneLabel = stringResource(R.string.settings_option_none)
	val chipOptions = listOf(1, 2, 4, 8, 16).map { value ->
		value to formatMemory(context, value * 512, noneLabel)
	}
	val slowOptions = listOf(0, 2, 4, 6, 7).map { value ->
		value to if (value == 0) noneLabel else formatMemory(context, value * 256, noneLabel)
	}
	val z2FastOptions = listOf(0, 1, 2, 4, 8).map { value ->
		value to if (value == 0) noneLabel else context.getString(R.string.settings_memory_value_mb, value)
	}
	val z3FastOptions = listOf(0, 16, 32, 64, 128, 256, 512).map { value ->
		value to if (value == 0) noneLabel else context.getString(R.string.settings_memory_value_mb, value)
	}

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_memory_config_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(12.dp))

				// Chip RAM
				MemoryDropdown(
					label = stringResource(R.string.settings_memory_chip_ram),
					options = chipOptions,
					selectedValue = settings.chipRam,
					onValueChange = { viewModel.updateSettings { s -> s.copy(chipRam = it) } }
				)

				Spacer(modifier = Modifier.height(8.dp))

				// Slow RAM
				MemoryDropdown(
					label = stringResource(R.string.settings_memory_slow_ram),
					options = slowOptions,
					selectedValue = settings.slowRam,
					onValueChange = { viewModel.updateSettings { s -> s.copy(slowRam = it) } }
				)

				Spacer(modifier = Modifier.height(8.dp))

				// Fast RAM (Z2)
				MemoryDropdown(
					label = stringResource(R.string.settings_memory_fast_ram),
					options = z2FastOptions,
					selectedValue = settings.fastRam,
					onValueChange = { viewModel.updateSettings { s -> s.copy(fastRam = it) } }
				)

				// Z3 RAM (only for 32-bit addressing)
				if (!settings.address24Bit) {
					Spacer(modifier = Modifier.height(8.dp))

					MemoryDropdown(
						label = stringResource(R.string.settings_memory_z3_fast_ram),
						options = z3FastOptions,
						selectedValue = settings.z3Ram,
						onValueChange = { viewModel.updateSettings { s -> s.copy(z3Ram = it) } }
					)
				}
			}
		}

		// Summary card
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_memory_summary_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				val chipKB = settings.chipRam * 512
				val slowKB = settings.slowRam * 256
				val fastMB = settings.fastRam
				val z3MB = settings.z3Ram

				Text(
					stringResource(R.string.settings_memory_summary_chip, formatMemory(context, chipKB, noneLabel)),
					style = MaterialTheme.typography.bodyMedium
				)
				if (slowKB > 0) {
					Text(
						stringResource(R.string.settings_memory_summary_slow, formatMemory(context, slowKB, noneLabel)),
						style = MaterialTheme.typography.bodyMedium
					)
				}
				if (fastMB > 0) {
					Text(
						stringResource(R.string.settings_memory_summary_z2_fast, fastMB),
						style = MaterialTheme.typography.bodyMedium
					)
				}
				if (z3MB > 0) {
					Text(
						stringResource(R.string.settings_memory_summary_z3_fast, z3MB),
						style = MaterialTheme.typography.bodyMedium
					)
				}

				val totalMB = (chipKB + slowKB) / 1024.0 + fastMB + z3MB
				Spacer(modifier = Modifier.height(4.dp))
				Text(
					stringResource(R.string.settings_memory_summary_total, totalMB),
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
		?: options.firstOrNull()?.second ?: stringResource(R.string.settings_placeholder_unknown)

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
				.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
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

private fun formatMemory(context: Context, kb: Int, noneLabel: String): String {
	if (kb <= 0) return noneLabel
	return if (kb >= 1024) {
		context.getString(R.string.settings_memory_value_mb, kb / 1024)
	} else {
		context.getString(R.string.settings_memory_value_kb, kb)
	}
}
