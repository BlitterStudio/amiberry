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
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.data.model.installedMemory
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.components.settings.schemaLabel
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.schemaValueText
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
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
		SchemaGroupSection("memory", viewModel, title = stringResource(R.string.settings_memory_config_title)) {
			MemorySizeRow(settings, "chipmem_size", settings.chipRam) { size ->
				viewModel.updateSettings { s -> s.copy(chipRam = size) }
			}
			MemorySizeRow(settings, "bogomem_size", settings.slowRam) { size ->
				viewModel.updateSettings { s -> s.copy(slowRam = size) }
			}
			SchemaChoiceRow(
				id = "fastmem_size",
				value = settings.fastRam.toString(),
				// The board's MB and KB keys are one size; the memory
				// transitions make setting either agree.
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.withOption("fastmem_size", size.toString()) } } }
			)
			SchemaChoiceRow(
				id = "z3mem_size",
				value = settings.z3Ram.toString(),
				enabled = SettingsControlAvailability.isAvailable(settings, "z3mem_size"),
				onSelect = { value -> value.toIntOrNull()?.let { size -> viewModel.updateSettings { s -> s.copy(z3Ram = size) } } }
			)
		}

		// Summary: each installed memory by the layer's label and size, then the total.
		val schema = settingsSchema()
		if (schema != null) SettingsSection(title = stringResource(R.string.settings_memory_summary_title)) {
			val installed = installedMemory(settings, schema)
			installed.forEach { memory ->
				Text(
					stringResource(R.string.settings_label_value, schemaLabel(memory.id), schemaValueText(memory.id, memory.value)),
					style = MaterialTheme.typography.bodyMedium
				)
			}
			Text(
				stringResource(R.string.settings_memory_summary_total, formatMemory(installed.sumOf { it.kb })),
				style = MaterialTheme.typography.titleSmall,
				color = MaterialTheme.colorScheme.primary
			)
		}

		SchemaGroupSection("rom", viewModel)
	}
}

/**
 * A memory size offering the sizes the machine allows (the layer's
 * availability table: the chipset extra's boards and the JIT rule some
 * out). While some are left out the option's constraint says why.
 */
@Composable
private fun MemorySizeRow(settings: EmulatorSettings, id: String, stored: Int, onSelect: (Int) -> Unit) {
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
