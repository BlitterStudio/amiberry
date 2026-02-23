package com.blitterstudio.amiberry.ui.screens

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.data.ConfigInfo
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.ui.viewmodel.ConfigurationsViewModel
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@Composable
fun ConfigurationsScreen(viewModel: ConfigurationsViewModel = viewModel()) {
	val configs by viewModel.configs.collectAsState()

	Column(modifier = Modifier.fillMaxSize()) {
		Text(
			text = "Saved Configurations",
			style = MaterialTheme.typography.headlineMedium,
			modifier = Modifier.padding(16.dp)
		)

		if (configs.isEmpty()) {
			Box(
				modifier = Modifier
					.fillMaxSize()
					.padding(32.dp),
				contentAlignment = Alignment.Center
			) {
				Column(horizontalAlignment = Alignment.CenterHorizontally) {
					Text("No configurations saved yet", style = MaterialTheme.typography.bodyLarge)
					Spacer(modifier = Modifier.height(8.dp))
					Text(
						"Configure settings and launch emulation to generate config files, " +
							"or use the Advanced (ImGui) interface to create them.",
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}
		} else {
			LazyColumn(
				contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
				verticalArrangement = Arrangement.spacedBy(8.dp)
			) {
				items(configs, key = { it.path }) { config ->
					ConfigItem(
						config = config,
						onDelete = { viewModel.deleteConfig(config.path) },
						onDuplicate = {
							viewModel.duplicateConfig(config.path, "${config.name}_copy")
						}
					)
				}
			}
		}
	}
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun ConfigItem(
	config: ConfigInfo,
	onDelete: () -> Unit,
	onDuplicate: () -> Unit
) {
	val context = LocalContext.current
	var showMenu by remember { mutableStateOf(false) }
	var showDeleteDialog by remember { mutableStateOf(false) }

	Card(
		modifier = Modifier
			.fillMaxWidth()
			.combinedClickable(
				onClick = {
					EmulatorLauncher.launchWithConfig(context, config.path, skipGui = true)
				},
				onLongClick = { showMenu = true }
			)
	) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(12.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			Column(modifier = Modifier.weight(1f)) {
				Text(
					text = config.name,
					style = MaterialTheme.typography.titleMedium
				)
				if (config.description.isNotEmpty()) {
					Text(
						text = config.description,
						style = MaterialTheme.typography.bodySmall,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
				Text(
					text = formatDate(config.lastModified),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}

			// Quick action buttons
			IconButton(onClick = {
				EmulatorLauncher.launchWithConfig(context, config.path, skipGui = true)
			}) {
				Icon(
					Icons.Default.PlayArrow,
					contentDescription = "Launch",
					modifier = Modifier.size(24.dp)
				)
			}
			IconButton(onClick = {
				EmulatorLauncher.launchAdvancedGui(context, config.path)
			}) {
				Icon(
					Icons.Default.Settings,
					contentDescription = "Edit in ImGui",
					modifier = Modifier.size(24.dp)
				)
			}

			// More options button (D-pad accessible alternative to long-press)
			Box {
				IconButton(onClick = { showMenu = true }) {
					Icon(
						Icons.Default.MoreVert,
						contentDescription = "More options",
						modifier = Modifier.size(24.dp)
					)
				}
				DropdownMenu(
					expanded = showMenu,
					onDismissRequest = { showMenu = false }
				) {
					DropdownMenuItem(
						text = { Text("Duplicate") },
						onClick = {
							onDuplicate()
							showMenu = false
						},
						leadingIcon = { Icon(Icons.Default.ContentCopy, contentDescription = null) }
					)
					DropdownMenuItem(
						text = { Text("Delete") },
						onClick = {
							showDeleteDialog = true
							showMenu = false
						},
						leadingIcon = {
							Icon(
								Icons.Default.Delete,
								contentDescription = null,
								tint = MaterialTheme.colorScheme.error
							)
						}
					)
				}
			}
		}
	}

	// Delete confirmation dialog
	if (showDeleteDialog) {
		AlertDialog(
			onDismissRequest = { showDeleteDialog = false },
			title = { Text("Delete Configuration") },
			text = { Text("Delete \"${config.name}\"? This cannot be undone.") },
			confirmButton = {
				TextButton(onClick = {
					onDelete()
					showDeleteDialog = false
				}) {
					Text("Delete", color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				TextButton(onClick = { showDeleteDialog = false }) {
					Text("Cancel")
				}
			}
		)
	}
}

private fun formatDate(timestamp: Long): String {
	if (timestamp == 0L) return ""
	val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())
	return sdf.format(Date(timestamp))
}
