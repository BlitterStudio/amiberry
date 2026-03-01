package com.blitterstudio.amiberry.ui.screens

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.focusGroup
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
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SecondaryScrollableTabRow
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Tab
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.ui.viewmodel.FileManagerViewModel
import java.util.Locale
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FileManagerScreen(viewModel: FileManagerViewModel = viewModel()) {
	val context = LocalContext.current
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	var selectedTab by rememberSaveable { mutableIntStateOf(0) }

	val tabs = listOf(
		TabInfo(FileCategory.ROMS, stringResource(R.string.file_manager_tab_roms), Icons.Default.Memory),
		TabInfo(FileCategory.WHDLOAD_GAMES, stringResource(R.string.file_manager_tab_whdload), Icons.Default.SportsEsports),
		TabInfo(FileCategory.FLOPPIES, stringResource(R.string.file_manager_tab_floppies), Icons.Default.SaveAlt),
		TabInfo(FileCategory.HARD_DRIVES, stringResource(R.string.file_manager_tab_hard_drives), Icons.Default.Storage),
		TabInfo(FileCategory.CD_IMAGES, stringResource(R.string.file_manager_tab_cds), Icons.Default.Album)
	)
	val pathCopiedMessage = stringResource(R.string.msg_path_copied)
	val clipboardLabelPath = stringResource(R.string.clipboard_label_path)

	val currentCategory = tabs[selectedTab].category
	val files by when (currentCategory) {
		FileCategory.ROMS -> viewModel.roms
		FileCategory.FLOPPIES -> viewModel.floppies
		FileCategory.HARD_DRIVES -> viewModel.hardDrives
		FileCategory.CD_IMAGES -> viewModel.cdImages
		FileCategory.WHDLOAD_GAMES -> viewModel.whdloadGames
	}.collectAsState()

	val importResult by viewModel.importResult.collectAsState()
	val filePickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let { viewModel.importFile(it, currentCategory) }
	}

	LaunchedEffect(importResult) {
		importResult?.let { msg ->
			snackbarHostState.showSnackbar(msg)
			viewModel.clearImportResult()
		}
	}

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(title = { Text(stringResource(R.string.file_manager_title)) })
		},
		floatingActionButton = {
			ExtendedFloatingActionButton(
				onClick = { filePickerLauncher.launch(arrayOf("*/*")) },
				icon = { Icon(Icons.Default.Add, contentDescription = null) },
				text = { Text(stringResource(R.string.action_import)) }
			)
		}
	) { innerPadding ->
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
		) {
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
							Text(
								stringResource(R.string.file_manager_app_storage),
								style = MaterialTheme.typography.labelMedium
							)
						}
						Text(
							text = viewModel.getStoragePath(),
							style = MaterialTheme.typography.bodySmall,
							fontFamily = FontFamily.Monospace,
							color = MaterialTheme.colorScheme.onSurfaceVariant,
							maxLines = 1
						)
					}

					IconButton(
						onClick = {
							val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
							clipboard.setPrimaryClip(ClipData.newPlainText(clipboardLabelPath, viewModel.getStoragePath()))
							scope.launch { snackbarHostState.showSnackbar(pathCopiedMessage) }
						}
					) {
						Icon(
							Icons.Default.ContentCopy,
							contentDescription = stringResource(R.string.file_manager_copy_path),
							modifier = Modifier.size(18.dp)
						)
					}
				}
			}

			SecondaryScrollableTabRow(
				selectedTabIndex = selectedTab,
				modifier = Modifier
					.fillMaxWidth()
					.focusGroup()
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

			if (files.isEmpty()) {
				Box(
					modifier = Modifier
						.fillMaxSize()
						.padding(32.dp),
					contentAlignment = Alignment.Center
				) {
					Column(horizontalAlignment = Alignment.CenterHorizontally) {
						Text(
							text = stringResource(
								R.string.file_manager_no_files_found,
								tabs[selectedTab].title.lowercase(Locale.getDefault())
							),
							style = MaterialTheme.typography.bodyLarge
						)
						Spacer(modifier = Modifier.height(8.dp))
						Text(
							text = context.getString(
								R.string.file_manager_empty_help,
								viewModel.getStoragePath(),
								currentCategory.dirName
							),
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
				Text(text = file.name, style = MaterialTheme.typography.bodyMedium)
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
