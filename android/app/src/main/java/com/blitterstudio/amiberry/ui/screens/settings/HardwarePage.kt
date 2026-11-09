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
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/**
 * The Hardware page: a section per layer group, from the CPU to the genlock
 * (Apple's generated Hardware page), the launcher's own rows first in the
 * groups that have them. The model picker is on the Overview.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HardwarePage(viewModel: SettingsViewModel) {
	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Every option's row comes from the registry (SettingsOptionRow), the
		// same rows All Settings shows.
		SchemaGroupSection("cpu", viewModel, title = stringResource(R.string.settings_cpu_section_title)) {
			SettingsOptionRow("cpu_model", viewModel)
			SettingsOptionRow("cpu_speed", viewModel)
			SettingsOptionRow("cpu_compatible", viewModel)
			SettingsOptionRow("cpu_24bit_addressing", viewModel)
			SettingsOptionRow("fpu_model", viewModel)
		}

		SchemaGroupSection("mmu", viewModel)
		SchemaGroupSection("fpu", viewModel)
		SchemaGroupSection("cpu_speed", viewModel)

		SchemaGroupSection("jit", viewModel, title = stringResource(R.string.settings_cpu_jit_section_title)) {
			SettingsOptionRow("cachesize", viewModel)
			SettingsOptionRow("compfpu", viewModel)
		}

		SchemaGroupSection("chipset", viewModel, title = stringResource(R.string.settings_chipset_section_title)) {
			SettingsOptionRow("chipset", viewModel)
			SettingsOptionRow("cycle_exact", viewModel)
			SettingsOptionRow("immediate_blits", viewModel)
			SettingsOptionRow("ntsc", viewModel)
			SettingsOptionRow("collision_level", viewModel)
		}
		SchemaGroupSection("adv_chipset", viewModel)

		SchemaGroupSection("memory", viewModel, title = stringResource(R.string.settings_memory_config_title)) {
			SettingsOptionRow("chipmem_size", viewModel)
			SettingsOptionRow("bogomem_size", viewModel)
			SettingsOptionRow("fastmem_size", viewModel)
			SettingsOptionRow("z3mem_size", viewModel)
		}

		SchemaGroupSection("rom", viewModel)
		SchemaGroupSection("accelerator", viewModel)
		SchemaGroupSection("rtg", viewModel)
		SchemaGroupSection("expansions", viewModel)
		SchemaGroupSection("ppc", viewModel)
		SchemaGroupSection("genlock", viewModel)
	}
}
