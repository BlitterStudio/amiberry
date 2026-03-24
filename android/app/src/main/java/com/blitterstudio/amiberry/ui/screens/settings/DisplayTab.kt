package com.blitterstudio.amiberry.ui.screens.settings

import android.os.Build
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
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DisplayTab(viewModel: SettingsViewModel) {
	val settings = viewModel.settings

	val resolutionPresets = listOf(
		Triple(640, 256, stringResource(R.string.settings_display_resolution_pal_low)),
		Triple(640, 512, stringResource(R.string.settings_display_resolution_pal_high)),
		Triple(720, 568, stringResource(R.string.settings_display_resolution_default)),
		Triple(800, 600, stringResource(R.string.settings_display_resolution_800_600)),
		Triple(1024, 768, stringResource(R.string.settings_display_resolution_1024_768))
	)

	Column(
		modifier = Modifier
			.fillMaxWidth()
			.verticalScroll(rememberScrollState())
			.padding(start = 16.dp, top = 16.dp, end = 16.dp, bottom = 120.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(stringResource(R.string.settings_display_title), style = MaterialTheme.typography.titleMedium)
				Spacer(modifier = Modifier.height(8.dp))

				// Resolution preset
				var resExpanded by remember { mutableStateOf(false) }
				val currentRes = "${settings.gfxWidth}x${settings.gfxHeight}"
				val currentLabel = resolutionPresets.firstOrNull {
					it.first == settings.gfxWidth && it.second == settings.gfxHeight
				}?.third ?: currentRes

				ExposedDropdownMenuBox(
					expanded = resExpanded,
					onExpandedChange = { resExpanded = it }
				) {
					OutlinedTextField(
						value = currentLabel,
						onValueChange = {},
						readOnly = true,
						label = { Text(stringResource(R.string.settings_display_resolution_label)) },
						trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = resExpanded) },
						modifier = Modifier
							.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
							.fillMaxWidth()
					)
					ExposedDropdownMenu(
						expanded = resExpanded,
						onDismissRequest = { resExpanded = false }
					) {
						resolutionPresets.forEach { (w, h, label) ->
							DropdownMenuItem(
								text = { Text(label) },
								onClick = {
									viewModel.updateSettings { s ->
										s.copy(gfxWidth = w, gfxHeight = h)
									}
									resExpanded = false
								}
							)
						}
					}
				}

				Spacer(modifier = Modifier.height(8.dp))

				SwitchRow(
					label = stringResource(R.string.settings_display_correct_aspect_ratio),
					checked = settings.correctAspect,
					onCheckedChange = {
						viewModel.updateSettings { s -> s.copy(correctAspect = it) }
					}
				)

			SwitchRow(
				label = stringResource(R.string.settings_display_auto_crop),
				checked = settings.autoCrop,
				onCheckedChange = {
					viewModel.updateSettings { s -> s.copy(autoCrop = it) }
				}
			)
			}
		}

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(stringResource(R.string.settings_display_app_theme), style = MaterialTheme.typography.titleMedium)
					Spacer(modifier = Modifier.height(8.dp))

					val context = LocalContext.current
					val appPrefs = AppPreferences.getInstance(context)
					val useDynamicColor by appPrefs.useDynamicColor

					SwitchRow(
						label = stringResource(R.string.settings_display_dynamic_color),
						checked = useDynamicColor,
						onCheckedChange = { appPrefs.setDynamicColor(it) }
					)
				}
			}
		}
	}
}
