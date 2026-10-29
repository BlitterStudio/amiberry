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
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.ui.components.settings.CompatibilityLevelRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaNumberRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.dpadArrowsMoveFocus
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/** The JIT cache sizes offered in the size field's menu (stored KB; 0 turns the JIT off). */
private val JIT_CACHE_SUGGESTIONS = listOf(0, 1024, 2048, 4096, 8192, 16384)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun CpuTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val fpuValues = SettingsControlAvailability.choiceValues(settings, "fpu_model")

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
							.dpadArrowsMoveFocus()
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
				CompatibilityLevelRow(
					settings = settings,
					onSelect = viewModel::applyCompatibilityLevel,
					modifier = Modifier.padding(top = 8.dp)
				)
			}
		}

		SchemaGroupSection("cpu", viewModel, title = stringResource(R.string.settings_cpu_section_title)) {
			SchemaChoiceRow(
				id = "cpu_model",
				value = settings.cpuModel.toString(),
				onSelect = { value ->
					viewModel.updateSettings { s -> s.withOption("cpu_model", value) }
				}
			)
			SchemaChoiceRow(
				id = "cpu_speed",
				value = settings.cpuSpeed,
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(cpuSpeed = value) } }
			)
			SchemaSwitchRow(
				id = "cpu_compatible",
				checked = settings.cpuCompatible,
				enabled = SettingsControlAvailability.isAvailable(settings, "cpu_compatible"),
				onCheckedChange = { viewModel.updateSettings { s -> s.withOption("cpu_compatible", it.toString()) } }
			)
			SchemaSwitchRow(
				id = "cpu_24bit_addressing",
				checked = settings.address24Bit,
				enabled = SettingsControlAvailability.isAvailable(settings, "cpu_24bit_addressing"),
				onCheckedChange = { viewModel.updateSettings { s -> s.copy(address24Bit = it) } }
			)
			SchemaChoiceRow(
				id = "fpu_model",
				value = settings.fpuModel.toString(),
				values = fpuValues,
				enabled = fpuValues == null || fpuValues.size > 1,
				onSelect = { value ->
					value.toIntOrNull()?.let { fpu -> viewModel.updateSettings { s -> s.copy(fpuModel = fpu) } }
				}
			)
		}

		SchemaGroupSection("mmu", viewModel)
		SchemaGroupSection("fpu", viewModel)
		SchemaGroupSection("cpu_speed", viewModel)

		SchemaGroupSection("jit", viewModel, title = stringResource(R.string.settings_cpu_jit_section_title)) {
			SchemaNumberRow(
				id = "cachesize",
				stored = settings.jitCacheSize,
				suggestions = JIT_CACHE_SUGGESTIONS,
				// The core takes 0 (off) or at least 128 KB (cachesize_accepts in settings_cpu.cpp).
				accepts = { it == 0 || it >= 128 },
				enabled = SettingsControlAvailability.isAvailable(settings, "cachesize"),
				onStoredChange = { size -> viewModel.updateSettings { s -> s.withOption("cachesize", size.toString()) } }
			)
			SchemaSwitchRow(
				id = "compfpu",
				checked = settings.jitFpu,
				enabled = SettingsControlAvailability.isAvailable(settings, "compfpu"),
				onCheckedChange = { viewModel.updateSettings { s -> s.copy(jitFpu = it) } }
			)
		}
	}
}
