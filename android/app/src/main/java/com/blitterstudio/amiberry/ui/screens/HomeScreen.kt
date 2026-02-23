package com.blitterstudio.amiberry.ui.screens

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.RocketLaunch
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Upload
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.AmiberryActivity
import com.blitterstudio.amiberry.data.EmulatorLauncher
import com.blitterstudio.amiberry.data.FileManager
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.FileCategory
import java.io.File

@Composable
fun HomeScreen(
	onNavigateToQuickStart: () -> Unit = {},
	onNavigateToFiles: () -> Unit = {},
	onNavigateToSettings: () -> Unit = {}
) {
	val context = LocalContext.current
	val storagePath = remember {
		context.getExternalFilesDir(null)?.absolutePath ?: "Unknown"
	}
	val romCount = remember {
		val romsDir = File(context.getExternalFilesDir(null), "roms")
		if (romsDir.exists()) {
			romsDir.listFiles { f ->
				f.extension.lowercase() in setOf("rom", "bin")
			}?.size ?: 0
		} else 0
	}

	val repository = remember { FileRepository.getInstance(context) }
	val whdloadGames by repository.whdloadGames.collectAsState()

	// SAF picker for importing WHDLoad games
	val whdloadPickerLauncher = rememberLauncherForActivityResult(
		contract = ActivityResultContracts.OpenDocument()
	) { uri ->
		uri?.let {
			val path = FileManager.importFile(context, it, FileCategory.WHDLOAD_GAMES)
			if (path != null) {
				repository.rescanCategory(FileCategory.WHDLOAD_GAMES)
				Toast.makeText(context, "Game imported", Toast.LENGTH_SHORT).show()
			}
		}
	}

	Column(
		modifier = Modifier
			.fillMaxSize()
			.verticalScroll(rememberScrollState())
			.padding(16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		// Header
		Text(
			text = "Amiberry",
			style = MaterialTheme.typography.headlineLarge
		)
		Text(
			text = "Amiga Emulator - v8.0.0",
			style = MaterialTheme.typography.bodyLarge,
			color = MaterialTheme.colorScheme.onSurfaceVariant
		)

		Spacer(modifier = Modifier.height(4.dp))

		// Quick actions
		Row(
			modifier = Modifier.fillMaxWidth(),
			horizontalArrangement = Arrangement.spacedBy(12.dp)
		) {
			FilledTonalButton(
				onClick = onNavigateToQuickStart,
				modifier = Modifier.weight(1f)
			) {
				Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(18.dp))
				Spacer(modifier = Modifier.width(6.dp))
				Text("Quick Start")
			}
			FilledTonalButton(
				onClick = {
					val intent = Intent(context, AmiberryActivity::class.java)
					intent.putExtra("SDL_ARGS", arrayOf<String>())
					context.startActivity(intent)
				},
				modifier = Modifier.weight(1f)
			) {
				Icon(Icons.Default.Settings, contentDescription = null, modifier = Modifier.size(18.dp))
				Spacer(modifier = Modifier.width(6.dp))
				Text("Advanced")
			}
		}

		// ROM status
		Card(
			modifier = Modifier.fillMaxWidth(),
			colors = CardDefaults.cardColors(
				containerColor = if (romCount > 0)
					MaterialTheme.colorScheme.primaryContainer
				else
					MaterialTheme.colorScheme.errorContainer
			)
		) {
			Row(
				modifier = Modifier
					.fillMaxWidth()
					.padding(16.dp),
				verticalAlignment = Alignment.CenterVertically
			) {
				Icon(
					Icons.Default.Memory,
					contentDescription = null,
					modifier = Modifier.size(32.dp)
				)
				Spacer(modifier = Modifier.width(12.dp))
				Column {
					Text(
						text = if (romCount > 0) "$romCount ROM(s) found" else "No Kickstart ROMs found",
						style = MaterialTheme.typography.titleMedium
					)
					Text(
						text = if (romCount > 0)
							"Ready to emulate"
						else
							"Import ROMs via the Files tab to get started",
						style = MaterialTheme.typography.bodySmall
					)
				}
			}
		}

		// WHDLoad Games - front and center
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(
					modifier = Modifier.fillMaxWidth(),
					verticalAlignment = Alignment.CenterVertically,
					horizontalArrangement = Arrangement.SpaceBetween
				) {
					Row(verticalAlignment = Alignment.CenterVertically) {
						Icon(
							Icons.Default.SportsEsports,
							contentDescription = null,
							modifier = Modifier.size(24.dp),
							tint = MaterialTheme.colorScheme.primary
						)
						Spacer(modifier = Modifier.width(8.dp))
						Text(
							text = "WHDLoad Games",
							style = MaterialTheme.typography.titleMedium
						)
					}
					TextButton(onClick = { whdloadPickerLauncher.launch(arrayOf("*/*")) }) {
						Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
						Spacer(modifier = Modifier.width(4.dp))
						Text("Import")
					}
				}

				Spacer(modifier = Modifier.height(4.dp))
				Text(
					text = "Import .lha game archives and launch them with auto-configuration",
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)

				if (whdloadGames.isEmpty()) {
					Spacer(modifier = Modifier.height(12.dp))
					Text(
						text = "No WHDLoad games found. Tap Import to add .lha game files.",
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				} else {
					Spacer(modifier = Modifier.height(8.dp))
					// Show up to 5 games with a play button each
					val displayGames = whdloadGames.take(5)
					displayGames.forEach { game ->
						Card(
							modifier = Modifier
								.fillMaxWidth()
								.padding(vertical = 2.dp)
								.clickable {
									EmulatorLauncher.launchWhdload(context, game.path)
								},
							colors = CardDefaults.cardColors(
								containerColor = MaterialTheme.colorScheme.surfaceVariant
							)
						) {
							Row(
								modifier = Modifier
									.fillMaxWidth()
									.padding(horizontal = 12.dp, vertical = 10.dp),
								verticalAlignment = Alignment.CenterVertically
							) {
								Icon(
									Icons.Default.RocketLaunch,
									contentDescription = "Launch",
									modifier = Modifier.size(20.dp),
									tint = MaterialTheme.colorScheme.primary
								)
								Spacer(modifier = Modifier.width(10.dp))
								Column(modifier = Modifier.weight(1f)) {
									Text(
										text = game.name.removeSuffix(".lha").removeSuffix(".lzx").removeSuffix(".lzh"),
										style = MaterialTheme.typography.bodyMedium,
										maxLines = 1,
										overflow = TextOverflow.Ellipsis
									)
									Text(
										text = game.sizeDisplay,
										style = MaterialTheme.typography.bodySmall,
										color = MaterialTheme.colorScheme.onSurfaceVariant
									)
								}
								Icon(
									Icons.Default.PlayArrow,
									contentDescription = "Play",
									modifier = Modifier.size(24.dp),
									tint = MaterialTheme.colorScheme.primary
								)
							}
						}
					}
					if (whdloadGames.size > 5) {
						Spacer(modifier = Modifier.height(4.dp))
						TextButton(
							onClick = onNavigateToFiles,
							modifier = Modifier.align(Alignment.End)
						) {
							Text("View all ${whdloadGames.size} games...")
						}
					}
				}
			}
		}

		// File storage info
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Row(
					modifier = Modifier.fillMaxWidth(),
					verticalAlignment = Alignment.CenterVertically,
					horizontalArrangement = Arrangement.SpaceBetween
				) {
					Row(verticalAlignment = Alignment.CenterVertically) {
						Icon(
							Icons.Default.Folder,
							contentDescription = null,
							modifier = Modifier.size(20.dp)
						)
						Spacer(modifier = Modifier.width(8.dp))
						Text(
							text = "App Storage",
							style = MaterialTheme.typography.titleMedium
						)
					}
					IconButton(onClick = {
						val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
						clipboard.setPrimaryClip(ClipData.newPlainText("Amiberry path", storagePath))
						Toast.makeText(context, "Path copied", Toast.LENGTH_SHORT).show()
					}) {
						Icon(Icons.Default.ContentCopy, contentDescription = "Copy path")
					}
				}
				Text(
					text = storagePath,
					style = MaterialTheme.typography.bodySmall,
					fontFamily = FontFamily.Monospace,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
				Spacer(modifier = Modifier.height(8.dp))
				Text(
					text = "You can copy ROMs and disk images here using a file manager, or use the Files tab to import them.",
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}

		// Getting started
		OutlinedCard(modifier = Modifier.fillMaxWidth()) {
			Column(modifier = Modifier.padding(16.dp)) {
				Text(
					text = "Getting Started",
					style = MaterialTheme.typography.titleMedium
				)
				Spacer(modifier = Modifier.height(8.dp))
				Text(
					text = "1. Import Kickstart ROMs via the Files tab\n" +
						"2. Import WHDLoad games (.lha) and tap to play, or:\n" +
						"3. Go to Quick Start, pick a model and a floppy disk\n" +
						"4. Tap Start Emulation\n\n" +
						"For detailed hardware configuration, use Settings or the Advanced (ImGui) interface.",
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}
}
