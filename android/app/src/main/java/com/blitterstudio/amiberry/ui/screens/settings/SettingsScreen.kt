package com.blitterstudio.amiberry.ui.screens.settings

import android.content.Intent
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.ScrollableTabRow
import androidx.compose.material3.Tab
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.AmiberryActivity
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

@Composable
fun SettingsScreen(viewModel: SettingsViewModel = viewModel()) {
	val context = LocalContext.current
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }

	val tabs = listOf("CPU & Chipset", "Memory", "Display", "Sound", "Input", "Storage")

	Column(modifier = Modifier.fillMaxSize()) {
		ScrollableTabRow(
			selectedTabIndex = selectedTab,
			modifier = Modifier.fillMaxWidth().focusGroup()
		) {
			tabs.forEachIndexed { index, title ->
				Tab(
					selected = selectedTab == index,
					onClick = { selectedTab = index },
					text = { Text(title) }
				)
			}
		}

		// Tab content
		Column(
			modifier = Modifier
				.weight(1f)
				.fillMaxWidth()
		) {
			when (selectedTab) {
				0 -> CpuChipsetTab(viewModel)
				1 -> MemoryTab(viewModel)
				2 -> DisplayTab(viewModel)
				3 -> SoundTab(viewModel)
				4 -> InputTab(viewModel)
				5 -> StorageTab(viewModel)
			}
		}

		// Bottom action buttons
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(16.dp)
				.focusGroup()
		) {
			Button(
				onClick = {
					val args = viewModel.generateLaunchArgs()
					val intent = Intent(context, AmiberryActivity::class.java)
					intent.putExtra("SDL_ARGS", args)
					context.startActivity(intent)
				},
				modifier = Modifier.weight(1f)
			) {
				Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(18.dp))
				Spacer(modifier = Modifier.width(6.dp))
				Text("Start")
			}
			Spacer(modifier = Modifier.width(8.dp))
			OutlinedButton(
				onClick = {
					val configFile = ConfigGenerator.writeConfig(
						context, viewModel.settings, ".temp_native_ui.uae"
					)
					EmulatorLauncher.launchAdvancedGui(context, configFile.absolutePath)
				},
				modifier = Modifier.weight(1f)
			) {
				Icon(Icons.Default.Settings, contentDescription = null, modifier = Modifier.size(18.dp))
				Spacer(modifier = Modifier.width(6.dp))
				Text("Advanced")
			}
		}
	}
}
