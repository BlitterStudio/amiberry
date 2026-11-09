package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaNumberRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/** The JIT cache sizes offered in the size field's menu (stored KB; 0 turns the JIT off). */
private val JIT_CACHE_SUGGESTIONS = listOf(0, 1024, 2048, 4096, 8192, 16384)

/**
 * The Hardware page: a section per layer group, from the CPU to the genlock
 * (Apple's generated Hardware page), the launcher's own rows first in the
 * groups that have them. The model picker is on the Overview.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HardwarePage(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val fpuValues = SettingsControlAvailability.choiceValues(settings, "fpu_model")

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
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

		SchemaGroupSection("chipset", viewModel, title = stringResource(R.string.settings_chipset_section_title)) {
			SchemaChoiceRow(
				id = "chipset",
				value = settings.chipset,
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(chipset = value) } }
			)
			SchemaChoiceRow(
				id = "cycle_exact",
				value = settings.cycleExact,
				values = SettingsControlAvailability.choiceValues(settings, "cycle_exact"),
				onSelect = { value -> viewModel.updateSettings { s -> s.withOption("cycle_exact", value) } }
			)
			SchemaSwitchRow(
				id = "immediate_blits",
				checked = settings.immediateBlits,
				onCheckedChange = { viewModel.updateSettings { s -> s.copy(immediateBlits = it) } }
			)
			SchemaSwitchRow(
				id = "ntsc",
				checked = settings.ntsc,
				onCheckedChange = { viewModel.updateSettings { s -> s.copy(ntsc = it) } }
			)
			SchemaChoiceRow(
				id = "collision_level",
				value = settings.collisionLevel,
				onSelect = { value -> viewModel.updateSettings { s -> s.copy(collisionLevel = value) } }
			)
		}
		SchemaGroupSection("adv_chipset", viewModel)

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

		SchemaGroupSection("rom", viewModel)
		SchemaGroupSection("accelerator", viewModel)
		SchemaGroupSection("rtg", viewModel)
		SchemaGroupSection("expansions", viewModel)
		SchemaGroupSection("ppc", viewModel)
		SchemaGroupSection("genlock", viewModel)
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
