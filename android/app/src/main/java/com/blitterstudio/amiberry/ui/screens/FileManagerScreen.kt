package com.blitterstudio.amiberry.ui.screens

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
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
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.FolderOpen
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material3.Card
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.ScrollableTabRow
import androidx.compose.material3.Tab
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.viewmodel.FileManagerViewModel

@Composable
fun FileManagerScreen(viewModel: FileManagerViewModel = viewModel()) {
	val context = LocalContext.current
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }

	val tabs = listOf(
		TabInfo(FileCategory.ROMS, "ROMs", Icons.Default.Memory),
		TabInfo(FileCategory.WHDLOAD_GAMES, "WHDLoad", Icons.Default.SportsEsports),
		TabInfo(FileCategory.FLOPPIES, "Floppies", Icons.Default.SaveAlt),
		TabInfo(FileCategory.HARD_DRIVES, "Hard Drives", Icons.Default.Storage),
		TabInfo(FileCategory.CD_IMAGES, "CDs", Icons.Default.Album)
	)

	val currentCategory = tabs[selectedTab].category
	val files by when (currentCategory) {
		FileCategory.ROMS -> viewModel.roms
		FileCategory.FLOPPIES -> viewModel.floppies
		FileCategory.HARD_DRIVES -> viewModel.hardDrives
		FileCategory.CD_IMAGES -> viewModel.cdImages
		FileCategory.WHDLOAD_GAMES -> viewModel.whdloadGames
	}.collectAsState()

	val importResult by viewModel.importResult.collectAsState()

	// SAF file picker launcher
	val filePickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let { viewModel.importFile(it, currentCategory) }
	}

	// Show toast on import result
	LaunchedEffect(importResult) {
		importResult?.let { msg ->
			Toast.makeText(context, msg, Toast.LENGTH_SHORT).show()
			viewModel.clearImportResult()
		}
	}

	Box(modifier = Modifier.fillMaxSize()) {
		Column {
			// Storage path info
			OutlinedCard(
				modifier = Modifier
					.fillMaxWidth()
					.padding(horizontal = 16.dp, vertical = 8.dp)
			) {
				Row(
					modifier = Modifier
						.fillMaxWidth()
						.padding(horizontal = 12.dp, vertical = 8.dp),
					verticalAlignment = Alignment.CenterVertically,
					horizontalArrangement = Arrangement.SpaceBetween
				) {
					Column(modifier = Modifier.weight(1f)) {
						Row(verticalAlignment = Alignment.CenterVertically) {
							Icon(
								Icons.Default.FolderOpen,
								contentDescription = null,
								modifier = Modifier.size(16.dp)
							)
							Spacer(modifier = Modifier.width(6.dp))
							Text("App Storage", style = MaterialTheme.typography.labelMedium)
						}
						Text(
							text = viewModel.getStoragePath(),
							style = MaterialTheme.typography.bodySmall,
							fontFamily = FontFamily.Monospace,
							color = MaterialTheme.colorScheme.onSurfaceVariant,
							maxLines = 1
						)
					}
					IconButton(onClick = {
						val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
						clipboard.setPrimaryClip(ClipData.newPlainText("path", viewModel.getStoragePath()))
						Toast.makeText(context, "Path copied", Toast.LENGTH_SHORT).show()
					}) {
						Icon(Icons.Default.ContentCopy, contentDescription = "Copy path", modifier = Modifier.size(18.dp))
					}
				}
			}

			// Tab row
			ScrollableTabRow(
				selectedTabIndex = selectedTab,
				modifier = Modifier.fillMaxWidth()
			) {
				tabs.forEachIndexed { index, tab ->
					Tab(
						selected = selectedTab == index,
						onClick = { selectedTab = index },
						text = { Text(tab.title) },
						icon = { Icon(tab.icon, contentDescription = tab.title, modifier = Modifier.size(18.dp)) }
					)
				}
			}

			// File list
			if (files.isEmpty()) {
				Box(
					modifier = Modifier
						.fillMaxSize()
						.padding(32.dp),
					contentAlignment = Alignment.Center
				) {
					Column(horizontalAlignment = Alignment.CenterHorizontally) {
						Text(
							text = "No ${tabs[selectedTab].title.lowercase()} found",
							style = MaterialTheme.typography.bodyLarge
						)
						Spacer(modifier = Modifier.height(8.dp))
						Text(
							text = "Tap + to import files, or copy them to:\n${viewModel.getStoragePath()}/${currentCategory.dirName}/",
							style = MaterialTheme.typography.bodySmall,
							color = MaterialTheme.colorScheme.onSurfaceVariant
						)
					}
				}
			} else {
				LazyColumn(
					contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
					verticalArrangement = Arrangement.spacedBy(6.dp)
				) {
					items(files, key = { it.path }) { file ->
						FileItem(file = file)
					}
				}
			}
		}

		// FAB for importing
		ExtendedFloatingActionButton(
			onClick = {
				filePickerLauncher.launch(arrayOf("*/*"))
			},
			modifier = Modifier
				.align(Alignment.BottomEnd)
				.padding(16.dp),
			icon = { Icon(Icons.Default.Add, contentDescription = null) },
			text = { Text("Import") }
		)
	}
}

@Composable
private fun FileItem(file: AmigaFile) {
	Card(modifier = Modifier.fillMaxWidth()) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(12.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			val icon = when (file.category) {
				FileCategory.ROMS -> Icons.Default.Memory
				FileCategory.FLOPPIES -> Icons.Default.SaveAlt
				FileCategory.HARD_DRIVES -> Icons.Default.Storage
				FileCategory.CD_IMAGES -> Icons.Default.Album
				FileCategory.WHDLOAD_GAMES -> Icons.Default.SportsEsports
			}
			Icon(
				icon,
				contentDescription = null,
				modifier = Modifier.size(28.dp),
				tint = MaterialTheme.colorScheme.primary
			)
			Spacer(modifier = Modifier.width(12.dp))
			Column(modifier = Modifier.weight(1f)) {
				Text(
					text = file.name,
					style = MaterialTheme.typography.bodyMedium
				)
				Text(
					text = file.sizeDisplay,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}
}

private data class TabInfo(
	val category: FileCategory,
	val title: String,
	val icon: ImageVector
)
