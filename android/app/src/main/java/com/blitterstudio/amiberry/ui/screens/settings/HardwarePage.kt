package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/**
 * The Hardware page: the designed sections of the registry
 * (SettingsGroups.byPage), as the Apple app's Hardware page is designed
 * (audit §3a) — CPU, Chipset, Memory, ROMs and Expansions, the on-board
 * interfaces and the expert rows in collapsed Advanced sections — with the
 * launcher's own rows leading the sections that have them. The model picker
 * is on the Overview.
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
		// same rows All Settings shows, in the registry's order.
		SettingsGroups.byPage.getValue(SettingsPage.Hardware).forEach { placement ->
			SchemaGroupSection(placement, viewModel, leading = leadingRows(placement.group, viewModel))
		}

		// The accelerator board (its own collapsed section: the board list
		// and the jumper codec come from the schema's catalogs, not the
		// options) and the expansion boards, as the Apple Hardware page
		// ends with its boards.
		AcceleratorBoardSection(viewModel)
		ExpansionBoardsSection(viewModel)
	}
}

/**
 * The page's own rows a designed section leads with (Apple's §3a layout):
 * the CPU, Chipset and Memory choices people make, and the Kickstart picker
 * the Overview shows under ROMs (its row, re-shown here as the Apple
 * Hardware page shows it). Sections without own rows take their options
 * from the layer's group alone.
 */
private fun leadingRows(group: String?, viewModel: SettingsViewModel): (@Composable ColumnScope.() -> Unit)? = when (group) {
	"cpu" -> {
		{
			SettingsOptionRow("cpu_model", viewModel)
			SettingsOptionRow("cpu_compatible", viewModel)
			SettingsOptionRow("cpu_24bit_addressing", viewModel)
			SettingsOptionRow("cpu_speed", viewModel)
			SettingsOptionRow("fpu_model", viewModel)
		}
	}
	"chipset" -> {
		{
			SettingsOptionRow("chipset", viewModel)
			SettingsOptionRow("chipset_compatible", viewModel)
			SettingsOptionRow("ntsc", viewModel)
			SettingsOptionRow("cycle_exact", viewModel)
			SettingsOptionRow("immediate_blits", viewModel)
			SettingsOptionRow("collision_level", viewModel)
		}
	}
	"memory" -> {
		{
			SettingsOptionRow("chipmem_size", viewModel)
			SettingsOptionRow("bogomem_size", viewModel)
			SettingsOptionRow("fastmem_size", viewModel)
			SettingsOptionRow("z3mem_size", viewModel)
		}
	}
	"rom" -> {
		{ KickstartRomRow(viewModel) }
	}
	else -> null
}
