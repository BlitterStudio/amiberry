package com.blitterstudio.amiberry.ui.screens.library

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.input.InputMode
import androidx.compose.ui.platform.LocalInputModeManager
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.ModelRomAvailability
import com.blitterstudio.amiberry.data.model.RomIdentity
import com.blitterstudio.amiberry.ui.components.ImportPicker
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadFocusIndicator

/**
 * The Library's System page: which models the installed Kickstart ROMs can run (a model
 * without its ROM still runs on AROS where AROS can stand in for it), the way to import
 * ROMs, and the ROM files themselves, each to delete except the built-in AROS.
 */
@Composable
internal fun LibraryKickstartRomsPage(
	modifier: Modifier,
	roms: List<AmigaFile>,
	query: String,
	importInProgress: Boolean,
	importPicker: ImportPicker,
	onDelete: (AmigaFile) -> Unit
) {
	val inputModeManager = LocalInputModeManager.current
	// Focus moves only for D-pad/keyboard users; a touch user's taps place no focus.
	fun requestDpadFocus(requester: FocusRequester) {
		if (inputModeManager.inputMode == InputMode.Keyboard) requester.requestFocus()
	}

	var confirmDelete by remember { mutableStateOf<AmigaFile?>(null) }

	// The models whose Kickstart is installed; a model whose ROM is missing still shows,
	// with what it boots meanwhile (or the ROMs it needs, where AROS cannot stand in).
	val modelStates = remember(roms) { romModelStates(roms) }
	val shownModels = filterRomModels(modelStates, query)

	// Deleting a ROM takes its focused row away, which dropped D-pad focus: once the
	// deleted file has left the list, focus returns to it (the models stay above).
	val listFocus = remember { FocusRequester() }
	var refocusAfterDeleting by remember { mutableStateOf<String?>(null) }
	LaunchedEffect(roms, refocusAfterDeleting) {
		val deletedPath = refocusAfterDeleting ?: return@LaunchedEffect
		if (roms.any { it.path == deletedPath }) return@LaunchedEffect
		refocusAfterDeleting = null
		requestDpadFocus(listFocus)
	}

	LazyColumn(
		modifier = modifier
			.focusRequester(listFocus)
			.contentEntryFocus()
			.focusRestorer(),
		contentPadding = PaddingValues(start = 16.dp, top = 8.dp, end = 16.dp, bottom = 16.dp),
		verticalArrangement = Arrangement.spacedBy(4.dp)
	) {
		item(key = "models-header") {
			Text(
				stringResource(R.string.library_roms_models_title),
				style = MaterialTheme.typography.titleSmall,
				modifier = Modifier.padding(top = 8.dp, bottom = 4.dp)
			)
		}
		if (shownModels.isEmpty()) {
			item(key = "models-empty") {
				Text(
					stringResource(R.string.file_manager_no_search_results),
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSurfaceVariant,
					modifier = Modifier.padding(vertical = 16.dp)
				)
			}
		} else {
			items(shownModels, key = { it.model.cmdArg }) { state ->
				RomModelRow(state)
			}
		}
		item(key = "models-footer") {
			Text(
				stringResource(R.string.library_roms_models_footer),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant,
				modifier = Modifier.padding(top = 4.dp, bottom = 12.dp)
			)
		}

		item(key = "import") {
			Button(
				onClick = { if (!importInProgress) importPicker.pickFiles() },
				enabled = !importInProgress,
				modifier = Modifier
					.fillMaxWidth()
					.padding(vertical = 4.dp)
					.dpadFocusIndicator()
			) {
				Icon(Icons.Default.Add, contentDescription = null, modifier = Modifier.size(18.dp))
				Spacer(modifier = Modifier.width(8.dp))
				Text(stringResource(R.string.action_import_roms))
			}
			Text(
				stringResource(R.string.library_roms_import_footer),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant,
				modifier = Modifier.padding(bottom = 8.dp)
			)
		}

		item(key = "files-header") {
			Text(
				stringResource(R.string.library_roms_files_title),
				style = MaterialTheme.typography.titleSmall,
				modifier = Modifier.padding(top = 8.dp, bottom = 4.dp)
			)
		}
		items(roms, key = { it.path }) { file ->
			RomFileRow(file = file, onConfirmDelete = { confirmDelete = file })
		}
	}

	confirmDelete?.let { file ->
		AlertDialog(
			onDismissRequest = { confirmDelete = null },
			title = { Text(stringResource(R.string.file_manager_delete_title)) },
			text = { Text(stringResource(R.string.file_manager_delete_message, file.name)) },
			confirmButton = {
				TextButton(
					onClick = {
						confirmDelete = null
						refocusAfterDeleting = file.path
						onDelete(file)
					},
					enabled = !importInProgress,
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_delete), color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				// Cancel takes focus, so an OK pressed at once deletes nothing.
				val cancelFocus = remember { FocusRequester() }
				LaunchedEffect(Unit) { cancelFocus.requestFocus() }
				TextButton(
					onClick = { confirmDelete = null },
					modifier = Modifier.focusRequester(cancelFocus).dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		)
	}
}

/** A model and whether its Kickstart is installed. */
internal data class RomModelState(
	val model: AmigaModel,
	val ready: Boolean,
	/** AROS can stand in for the model's ROM, so its machines run without a Kickstart. */
	val bootsAros: Boolean,
	/** The emulator's names for the ROMs the model is built with. */
	val neededRoms: List<String>
)

internal fun romModelStates(roms: List<AmigaFile>): List<RomModelState> =
	AmigaModel.entries.map { model ->
		RomModelState(
			model = model,
			ready = ModelRomAvailability.hasKickstart(model, roms),
			bootsAros = ModelRomAvailability.canBootAros(model),
			neededRoms = MachineSummaries.romsNeeded(model)
		)
	}

/** The models whose name matches every word of [query]. */
internal fun filterRomModels(states: List<RomModelState>, query: String): List<RomModelState> {
	val words = query.trim().lowercase().split(Regex("\\s+")).filter { it.isNotEmpty() }
	if (words.isEmpty()) return states
	return states.filter { state -> words.all { it in state.model.displayName.lowercase() } }
}

/** One model's readiness: Ready, or what it boots (needs) without its ROM. */
@Composable
private fun RomModelRow(state: RomModelState) {
	Row(
		modifier = Modifier
			.fillMaxWidth()
			.padding(horizontal = 4.dp, vertical = 10.dp),
		verticalAlignment = Alignment.CenterVertically
	) {
		Text(state.model.displayName, style = MaterialTheme.typography.bodyLarge, modifier = Modifier.weight(1f))
		if (state.ready) {
			Row(verticalAlignment = Alignment.CenterVertically) {
				Icon(
					Icons.Default.CheckCircle,
					contentDescription = null,
					tint = MaterialTheme.colorScheme.primary,
					modifier = Modifier.size(18.dp)
				)
				Spacer(modifier = Modifier.width(4.dp))
				Text(
					stringResource(R.string.library_rom_model_ready),
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.primary
				)
			}
		} else if (state.bootsAros) {
			Text(
				stringResource(R.string.library_rom_model_boots_aros),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		} else {
			Text(
				stringResource(R.string.library_rom_model_needs_rom, state.neededRoms.joinToString(", ")),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
		}
	}
}

/** A ROM file: what it is (a Kickstart by the emulator's name, or the built-in AROS). */
@Composable
private fun RomFileRow(file: AmigaFile, onConfirmDelete: () -> Unit) {
	val romIdentity = remember(file) { ModelRomAvailability.romIdentity(file) }
	val content: @Composable () -> Unit = {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(start = 12.dp, top = 10.dp, bottom = 10.dp, end = 8.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			Icon(
				Icons.Default.Memory,
				contentDescription = null,
				modifier = Modifier.size(24.dp),
				tint = MaterialTheme.colorScheme.primary
			)
			Spacer(modifier = Modifier.width(12.dp))
			Column(modifier = Modifier.weight(1f)) {
				Text(
					text = when (romIdentity) {
						is RomIdentity.Kickstart -> romIdentity.name
						RomIdentity.BuiltInAros -> stringResource(R.string.file_manager_aros_row)
						RomIdentity.Unknown -> file.name
					},
					style = MaterialTheme.typography.bodyMedium
				)
				Text(
					text = when (romIdentity) {
						is RomIdentity.Kickstart ->
							stringResource(R.string.file_manager_rom_file_detail, file.name, file.sizeDisplay)
						RomIdentity.BuiltInAros -> stringResource(R.string.file_manager_aros_row_detail)
						RomIdentity.Unknown -> file.sizeDisplay
					},
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
			if (romIdentity != RomIdentity.BuiltInAros) {
				Icon(
					Icons.AutoMirrored.Filled.KeyboardArrowRight,
					contentDescription = null,
					tint = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}
	Card(modifier = Modifier.fillMaxWidth()) {
		if (romIdentity == RomIdentity.BuiltInAros) {
			content()
		} else {
			// TalkBack announces the tap as "Show actions"; the only action is Delete,
			// which asks before it goes ahead.
			Box(
				modifier = Modifier
					.fillMaxWidth()
					.dpadFocusIndicator()
					.clickable(onClickLabel = stringResource(R.string.action_delete), onClick = onConfirmDelete)
			) {
				content()
			}
		}
	}
}
