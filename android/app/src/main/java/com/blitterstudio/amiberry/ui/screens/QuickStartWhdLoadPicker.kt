package com.blitterstudio.amiberry.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowDropDown
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Upload
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.WhdLoadTitles
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate

/**
 * Quick Start's WHDLoad game: the chosen game by its readable title, a searchable list
 * to choose another (recently played first), Clear selection and Import. With
 * [onLaunch] (a TV), one OK on a game in the list starts it.
 */
@Composable
internal fun WhdLoadGameCard(
	games: List<AmigaFile>,
	selectedGame: AmigaFile?,
	/** Paths of the WHDLoad games launched lately, most recent first. */
	recentPaths: List<String>,
	importInProgress: Boolean,
	onImport: () -> Unit,
	onSelect: (AmigaFile) -> Unit,
	onClear: () -> Unit,
	onLaunch: ((AmigaFile) -> Unit)?,
	modifier: Modifier = Modifier
) {
	var pickerOpen by rememberSaveable { mutableStateOf(false) }
	val fieldFocus = remember { FocusRequester() }
	OutlinedCard(modifier = modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Row(
				modifier = Modifier.fillMaxWidth(),
				verticalAlignment = Alignment.CenterVertically,
				horizontalArrangement = Arrangement.SpaceBetween
			) {
				Row(verticalAlignment = Alignment.CenterVertically) {
					Icon(Icons.Default.SportsEsports, contentDescription = null, modifier = Modifier.size(20.dp))
					Spacer(modifier = Modifier.width(8.dp))
					Text(stringResource(R.string.quick_start_whdload_title), style = MaterialTheme.typography.titleMedium)
				}
				TextButton(
					onClick = onImport,
					enabled = !importInProgress,
					modifier = Modifier.dpadFocusIndicator()
				) {
					if (importInProgress) {
						CircularProgressIndicator(modifier = Modifier.size(16.dp), strokeWidth = 2.dp)
					} else {
						Icon(Icons.Default.Upload, contentDescription = null, modifier = Modifier.size(16.dp))
					}
					Spacer(modifier = Modifier.width(4.dp))
					Text(stringResource(if (importInProgress) R.string.action_importing else R.string.action_import))
				}
			}
			Text(
				text = stringResource(R.string.quick_start_whdload_help),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
			Spacer(modifier = Modifier.height(8.dp))
			if (games.isEmpty()) {
				Text(
					text = stringResource(R.string.quick_start_no_whdload_games),
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			} else {
				Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
					OutlinedButton(
						onClick = { pickerOpen = true },
						shape = MaterialTheme.shapes.extraSmall,
						modifier = Modifier
							.weight(1f)
							.focusRequester(fieldFocus)
							.dpadFocusIndicator()
					) {
						Text(
							selectedGame?.let { WhdLoadTitles.clean(it.name) }
								?: stringResource(R.string.quick_start_whdload_choose_game),
							modifier = Modifier.weight(1f),
							maxLines = 1,
							overflow = TextOverflow.Ellipsis,
							style = MaterialTheme.typography.bodyLarge
						)
						Icon(Icons.Default.ArrowDropDown, contentDescription = null)
					}
					if (selectedGame != null) {
						IconButton(
							onClick = {
								onClear()
								// The button leaves with the selection: keep D-pad focus on the field.
								fieldFocus.requestFocus()
							},
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_clear_selection))
						}
					}
				}
			}
		}
	}

	if (pickerOpen) {
		WhdLoadGamePickerDialog(
			games = games,
			selectedGame = selectedGame,
			recentPaths = recentPaths,
			launchesOnPick = onLaunch != null,
			onDismiss = { pickerOpen = false },
			onPick = { game ->
				pickerOpen = false
				onSelect(game)
				onLaunch?.invoke(game)
			}
		)
	}
}

@Composable
private fun WhdLoadGamePickerDialog(
	games: List<AmigaFile>,
	selectedGame: AmigaFile?,
	recentPaths: List<String>,
	launchesOnPick: Boolean,
	onDismiss: () -> Unit,
	onPick: (AmigaFile) -> Unit
) {
	var query by rememberSaveable { mutableStateOf("") }
	// Titles are cleaned once per game list, not on every keystroke or row.
	val entries = remember(games) { QuickStartUiState.whdloadGameEntries(games) }
	val shown = remember(entries, recentPaths, query) { QuickStartUiState.whdloadGameList(entries, recentPaths, query) }
	val recent = remember(recentPaths) { recentPaths.toSet() }
	val typingGate = rememberDpadTypingGate()
	val listState = rememberLazyListState()
	val initialRowFocus = remember { FocusRequester() }
	val initialIndex = shown.indexOfFirst { it.game.path == selectedGame?.path }.coerceAtLeast(0)
	// On a TV, focus starts on the chosen game (else the first), not in the search field.
	var initialFocusPending by remember { mutableStateOf(launchesOnPick) }

	Dialog(onDismissRequest = onDismiss, properties = DialogProperties(usePlatformDefaultWidth = false)) {
		Surface(
			shape = MaterialTheme.shapes.extraLarge,
			tonalElevation = 6.dp,
			modifier = Modifier
				.fillMaxWidth(0.92f)
				.fillMaxHeight(0.85f)
		) {
			Column(modifier = Modifier.padding(20.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
				if (initialFocusPending) LaunchedEffect(Unit) { listState.scrollToItem(initialIndex) }
				Text(stringResource(R.string.quick_start_whdload_picker_title), style = MaterialTheme.typography.titleLarge)
				Text(
					stringResource(
						if (launchesOnPick) R.string.quick_start_whdload_picker_hint_launch else R.string.quick_start_whdload_picker_hint
					),
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
				OutlinedTextField(
					value = query,
					onValueChange = { query = it },
					readOnly = typingGate.readOnly,
					singleLine = true,
					leadingIcon = { Icon(Icons.Default.Search, contentDescription = null) },
					placeholder = { Text(stringResource(R.string.quick_start_whdload_search)) },
					keyboardOptions = KeyboardOptions(imeAction = ImeAction.Search),
					keyboardActions = KeyboardActions(onSearch = { typingGate.stopTyping() }),
					modifier = Modifier
						.fillMaxWidth()
						.then(typingGate.modifier)
						.dpadVerticalMovesFocus()
				)
				if (shown.isEmpty()) {
					Text(
						stringResource(R.string.quick_start_whdload_no_match, query.trim()),
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant,
						modifier = Modifier.weight(1f)
					)
				} else {
					LazyColumn(state = listState, modifier = Modifier.weight(1f)) {
						itemsIndexed(shown, key = { _, entry -> entry.game.path }) { index, entry ->
							val game = entry.game
							val focusHere = initialFocusPending && index == initialIndex
							if (focusHere) {
								// Here the row's requester is attached; the effect runs once.
								LaunchedEffect(Unit) {
									initialRowFocus.requestFocus()
									initialFocusPending = false
								}
							}
							GameRow(
								game = game,
								title = entry.title,
								recentlyPlayed = game.path in recent,
								selected = game.path == selectedGame?.path,
								launchesOnPick = launchesOnPick,
								modifier = if (focusHere) Modifier.focusRequester(initialRowFocus) else Modifier,
								onPick = { onPick(game) }
							)
						}
					}
				}
				Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.End) {
					TextButton(onClick = onDismiss, modifier = Modifier.dpadFocusIndicator()) {
						Text(stringResource(R.string.action_cancel))
					}
				}
			}
		}
	}
}

@Composable
private fun GameRow(
	game: AmigaFile,
	title: String,
	recentlyPlayed: Boolean,
	selected: Boolean,
	launchesOnPick: Boolean,
	modifier: Modifier,
	onPick: () -> Unit
) {
	Row(
		modifier = modifier
			.fillMaxWidth()
			.clip(RoundedCornerShape(12.dp))
			.dpadFocusIndicator()
			.clickable(
				onClickLabel = stringResource(if (launchesOnPick) R.string.action_launch else R.string.action_select),
				onClick = onPick
			)
			.padding(horizontal = 12.dp, vertical = 10.dp),
		verticalAlignment = Alignment.CenterVertically
	) {
		Column(modifier = Modifier.weight(1f)) {
			Text(title, style = MaterialTheme.typography.titleSmall)
			Text(
				stringResource(R.string.quick_start_whdload_game_detail, game.name, game.sizeDisplay),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant,
				maxLines = 1,
				overflow = TextOverflow.Ellipsis
			)
		}
		if (recentlyPlayed) {
			Icon(
				Icons.Default.History,
				contentDescription = stringResource(R.string.quick_start_whdload_recently_played),
				modifier = Modifier.padding(start = 8.dp).size(18.dp),
				tint = MaterialTheme.colorScheme.onSurfaceVariant
			)
		}
		if (selected) {
			Icon(
				Icons.Default.Check,
				contentDescription = stringResource(R.string.quick_start_whdload_selected),
				modifier = Modifier.padding(start = 8.dp).size(18.dp),
				tint = MaterialTheme.colorScheme.primary
			)
		}
	}
}
