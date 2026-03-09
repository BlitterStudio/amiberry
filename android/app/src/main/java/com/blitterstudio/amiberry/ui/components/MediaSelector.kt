package com.blitterstudio.amiberry.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Eject
import androidx.compose.material.icons.filled.Upload
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaFile

/**
 * Reusable media selector component for floppy disks, CDs, WHDLoad games, etc.
 *
 * Displays an outlined card with:
 * - Title row with icon and import button
 * - Dropdown menu for selecting from available files
 * - Eject button when a file is selected
 * - Empty state text when no files are available
 *
 * @param title The title label (e.g., "Floppy DF0:", "CD Image")
 * @param icon The leading icon for the title
 * @param items List of available files to choose from
 * @param selectedItem Currently selected file (null if none)
 * @param selectedPath Currently selected path string (for display when item not in list)
 * @param emptyText Text shown when no items are available
 * @param placeholder Text shown in dropdown when nothing is selected
 * @param helpText Optional description text shown between the title and the dropdown
 * @param onItemSelected Called when user selects an item from the dropdown
 * @param onEject Called when user ejects the current selection
 * @param onImport Called when user taps the import button
 * @param displayName Optional transform for item display names (default: AmigaFile.name)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MediaSelector(
	title: String,
	icon: ImageVector,
	items: List<AmigaFile>,
	selectedItem: AmigaFile?,
	selectedPath: String,
	emptyText: String,
	placeholder: String = stringResource(R.string.placeholder_none),
	helpText: String? = null,
	onItemSelected: (AmigaFile) -> Unit,
	onEject: () -> Unit,
	onImport: () -> Unit,
	displayName: (AmigaFile) -> String = { it.name },
	modifier: Modifier = Modifier
) {
	OutlinedCard(modifier = modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Row(
				modifier = Modifier.fillMaxWidth(),
				verticalAlignment = Alignment.CenterVertically,
				horizontalArrangement = Arrangement.SpaceBetween
			) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(icon, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(title, style = MaterialTheme.typography.titleMedium)
				}
				TextButton(onClick = onImport) {
					Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
					Spacer(modifier = Modifier.width(4.dp))
					Text(stringResource(R.string.action_import))
				}
			}
			if (helpText != null) {
				Text(
					text = helpText,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
			Spacer(modifier = Modifier.height(8.dp))
			if (items.isEmpty() && selectedPath.isBlank()) {
				Text(
					text = emptyText,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			} else {
				var expanded by remember { mutableStateOf(false) }

				Row(
					modifier = Modifier.fillMaxWidth(),
					verticalAlignment = Alignment.CenterVertically
				) {
					ExposedDropdownMenuBox(
						expanded = expanded,
						onExpandedChange = { expanded = it },
						modifier = Modifier.weight(1f)
					) {
						OutlinedTextField(
							value = selectedItem?.let { displayName(it) }
								?: selectedPath.takeIf { it.isNotBlank() }?.substringAfterLast('/')
								?: placeholder,
							onValueChange = {},
							readOnly = true,
							trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
							modifier = Modifier
								.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
								.fillMaxWidth()
						)
						ExposedDropdownMenu(
							expanded = expanded,
							onDismissRequest = { expanded = false }
						) {
							items.forEach { file ->
								DropdownMenuItem(
									text = {
										Column {
											Text(displayName(file))
											Text(
												file.sizeDisplay,
												style = MaterialTheme.typography.bodySmall,
												color = MaterialTheme.colorScheme.onSurfaceVariant
											)
										}
									},
									onClick = {
										onItemSelected(file)
										expanded = false
									}
								)
							}
						}
					}

					if (selectedPath.isNotBlank()) {
						IconButton(onClick = onEject) {
							Icon(Icons.Default.Eject, contentDescription = stringResource(R.string.action_eject))
						}
					}
				}
			}
		}
	}
}
