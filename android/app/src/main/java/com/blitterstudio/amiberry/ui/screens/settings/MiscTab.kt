package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/** The options the Apple app keeps on its Ports & Misc page that mean something on Android, one section per group. */
@Composable
fun MiscTab(viewModel: SettingsViewModel) {
	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		SettingsGroups.byTab.getValue(SettingsTab.Misc).forEach { placement ->
			SchemaGroupSection(placement.group, viewModel)
		}
	}
}
