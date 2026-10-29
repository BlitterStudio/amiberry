package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.ui.components.settings.SchemaChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SchemaSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@Composable
fun ChipsetTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		SettingsSection(title = stringResource(R.string.settings_chipset_section_title)) {
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
	}
}
