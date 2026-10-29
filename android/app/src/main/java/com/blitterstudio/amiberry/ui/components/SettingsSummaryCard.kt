package com.blitterstudio.amiberry.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.takeOrElse
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.ui.screens.QuickStartEffectiveSummary

/**
 * The machine a launch runs as label/value rows: Quick Start's current settings and a
 * saved configuration in the Configurations detail pane. A Kickstart file that is gone
 * shows as missing, in the error colour.
 */
@Composable
fun SettingsSummaryCard(
	summary: QuickStartEffectiveSummary,
	title: String,
	subtitle: String? = null,
	modifier: Modifier = Modifier
) {
	SummaryCard(title = title, subtitle = subtitle, modifier = modifier) {
		SummaryRow(stringResource(R.string.quick_start_summary_model), summary.model)
		SummaryRow(stringResource(R.string.quick_start_summary_cpu), summary.cpu)
		if (summary.romMissing) {
			SummaryRow(
				stringResource(R.string.quick_start_summary_rom),
				stringResource(
					if (summary.romFallsBackToAros) R.string.summary_rom_missing_aros else R.string.summary_rom_missing,
					summary.romFileName
				),
				valueColor = MaterialTheme.colorScheme.error
			)
		} else {
			summary.rom?.let { rom ->
				SummaryRow(
					stringResource(R.string.quick_start_summary_rom),
					when (rom) {
						is RomIdentity.Kickstart -> rom.name
						RomIdentity.BuiltInAros -> stringResource(R.string.rom_aros_builtin)
						RomIdentity.Unknown -> summary.romFileName
					}
				)
			}
		}
		SummaryRow(stringResource(R.string.quick_start_summary_chipset), summary.chipset)
		SummaryRow(stringResource(R.string.quick_start_summary_memory), summary.memory)
		SummaryRow(
			stringResource(R.string.quick_start_summary_media),
			summary.media.ifBlank { stringResource(R.string.summary_no_media) }
		)
		SummaryRow(stringResource(R.string.quick_start_summary_controls), summary.controls)
	}
}

/** The summary card's frame: [title], an optional [subtitle] and label/value [rows] ([SummaryRow]). */
@Composable
fun SummaryCard(
	title: String,
	subtitle: String? = null,
	modifier: Modifier = Modifier,
	rows: @Composable () -> Unit
) {
	Card(
		modifier = modifier.fillMaxWidth(),
		colors = CardDefaults.cardColors(
			containerColor = MaterialTheme.colorScheme.primaryContainer
		)
	) {
		Column(
			modifier = Modifier.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(8.dp)
		) {
			Text(title, style = MaterialTheme.typography.titleLarge)
			if (subtitle != null) {
				Text(
					text = subtitle,
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
			rows()
		}
	}
}

@Composable
fun SummaryRow(label: String, value: String, valueColor: Color = Color.Unspecified) {
	Row(
		modifier = Modifier.fillMaxWidth(),
		horizontalArrangement = Arrangement.spacedBy(12.dp)
	) {
		Text(
			label,
			modifier = Modifier.weight(0.35f),
			style = MaterialTheme.typography.bodyMedium,
			color = MaterialTheme.colorScheme.onPrimaryContainer
		)
		Text(
			value,
			modifier = Modifier.weight(0.65f),
			style = MaterialTheme.typography.bodyMedium,
			color = valueColor.takeOrElse { MaterialTheme.colorScheme.onPrimaryContainer }
		)
	}
}
