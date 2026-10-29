package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.schemaLabel
import com.blitterstudio.amiberry.ui.components.settings.schemaValueText
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlin.math.roundToInt

internal sealed interface MemoryAmountDisplay {
	data object None : MemoryAmountDisplay
	data class Kilobytes(val kb: Int) : MemoryAmountDisplay
	data class WholeMegabytes(val mb: Int) : MemoryAmountDisplay
	data class FractionalMegabytes(val tenths: Int) : MemoryAmountDisplay
}

internal fun memoryAmountDisplayValue(kb: Int): MemoryAmountDisplay {
	if (kb <= 0) return MemoryAmountDisplay.None
	if (kb < 1024) return MemoryAmountDisplay.Kilobytes(kb)

	val tenths = ((kb / 1024.0) * 10).roundToInt()
	return if (tenths % 10 == 0) {
		MemoryAmountDisplay.WholeMegabytes(tenths / 10)
	} else {
		MemoryAmountDisplay.FractionalMegabytes(tenths)
	}
}

@Composable
fun MemoryTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		SettingsSection(title = stringResource(R.string.settings_memory_config_title)) {
			SchemaChoiceRow(
				id = "chipmem_size",
				value = settings.chipRam.toString(),
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(chipRam = size) } } }
			)
			SchemaChoiceRow(
				id = "bogomem_size",
				value = settings.slowRam.toString(),
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(slowRam = size) } } }
			)
			SchemaChoiceRow(
				id = "fastmem_size",
				value = settings.fastRam.toString(),
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(fastRam = size) } } }
			)
			SchemaChoiceRow(
				id = "z3mem_size",
				value = settings.z3Ram.toString(),
				enabled = SettingsControlAvailability.isZ3MemoryAvailable(settings),
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(z3Ram = size) } } }
			)
		}

		// Summary: each installed memory by the layer's label and size, then the total.
		SettingsSection(title = stringResource(R.string.settings_memory_summary_title)) {
			val chipKb = EmulatorSettings.chipRamKb(settings.chipRam)
			val slowKb = settings.slowRam * 256
			val summary = buildList {
				add("chipmem_size" to settings.chipRam)
				if (settings.slowRam > 0) add("bogomem_size" to settings.slowRam)
				if (settings.fastRam > 0) add("fastmem_size" to settings.fastRam)
				if (settings.z3Ram > 0) add("z3mem_size" to settings.z3Ram)
			}
			summary.forEach { (id, value) ->
				Text(
					stringResource(R.string.settings_label_value, schemaLabel(id), schemaValueText(id, value.toString())),
					style = MaterialTheme.typography.bodyMedium
				)
			}
			val totalKb = chipKb + slowKb + (settings.fastRam + settings.z3Ram) * 1024
			Text(
				stringResource(R.string.settings_memory_summary_total, formatMemory(totalKb)),
				style = MaterialTheme.typography.titleSmall,
				color = MaterialTheme.colorScheme.primary
			)
		}
	}
}

@Composable
private fun formatMemory(kb: Int): String {
	return when (val amount = memoryAmountDisplayValue(kb)) {
		MemoryAmountDisplay.None -> stringResource(R.string.settings_option_none)
		is MemoryAmountDisplay.Kilobytes -> stringResource(R.string.settings_memory_value_kb, amount.kb)
		is MemoryAmountDisplay.WholeMegabytes -> stringResource(R.string.settings_memory_value_mb, amount.mb)
		is MemoryAmountDisplay.FractionalMegabytes -> {
			stringResource(R.string.settings_memory_value_mb_decimal, amount.tenths / 10.0)
		}
	}
}
