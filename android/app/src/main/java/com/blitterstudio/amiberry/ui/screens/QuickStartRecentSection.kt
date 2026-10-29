package com.blitterstudio.amiberry.ui.screens

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ClearAll
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.RemoveCircleOutline
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.RecentLaunches
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import org.json.JSONObject

/** How much an entry that can no longer be launched is dimmed. */
private const val UNAVAILABLE_ALPHA = 0.38f

/**
 * Quick Start's Recent mode: the last launches, most recent first. An entry whose files are
 * gone (or whose model has no Kickstart ROM) stays listed, dimmed with the reason, so the user
 * can remove it; every entry has Remove from Recent (long-press or its menu) and the list has
 * Clear Recent.
 */
@Composable
internal fun QuickStartRecentSection(
	items: List<RecentLaunches.Item>,
	launchInProgress: Boolean,
	onLaunch: (JSONObject) -> Unit,
	onRemove: (JSONObject) -> Unit,
	onClear: () -> Unit
) {
	var confirmClear by remember { mutableStateOf(false) }

	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Row(verticalAlignment = Alignment.CenterVertically) {
				Icon(Icons.Default.History, contentDescription = null, modifier = Modifier.size(20.dp))
				Spacer(modifier = Modifier.width(8.dp))
				Text(
					stringResource(R.string.quick_start_recent_title),
					style = MaterialTheme.typography.titleMedium,
					modifier = Modifier.weight(1f)
				)
				if (items.isNotEmpty()) {
					TextButton(
						onClick = { confirmClear = true },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(Icons.Default.ClearAll, contentDescription = null, modifier = Modifier.size(18.dp))
						Spacer(modifier = Modifier.width(4.dp))
						Text(stringResource(R.string.action_clear_recent))
					}
				}
			}
			Spacer(modifier = Modifier.height(8.dp))
			if (items.isEmpty()) {
				Text(
					stringResource(R.string.quick_start_recent_empty),
					style = MaterialTheme.typography.bodyMedium,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			} else {
				items.forEach { item ->
					RecentRow(
						item = item,
						launchInProgress = launchInProgress,
						onLaunch = { onLaunch(item.entry) },
						onRemove = { onRemove(item.entry) }
					)
				}
			}
		}
	}

	if (confirmClear) {
		AlertDialog(
			onDismissRequest = { confirmClear = false },
			title = { Text(stringResource(R.string.dialog_clear_recent_title)) },
			text = { Text(stringResource(R.string.dialog_clear_recent_message)) },
			confirmButton = {
				TextButton(
					onClick = {
						confirmClear = false
						onClear()
					},
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_clear), color = MaterialTheme.colorScheme.error)
				}
			},
			dismissButton = {
				TextButton(
					onClick = { confirmClear = false },
					modifier = Modifier.dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_cancel))
				}
			}
		)
	}
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun RecentRow(
	item: RecentLaunches.Item,
	launchInProgress: Boolean,
	onLaunch: () -> Unit,
	onRemove: () -> Unit
) {
	var showMenu by remember { mutableStateOf(false) }
	val details = item.details
	val launchable = item.status == RecentLaunches.Status.Available
	val statusText = when (item.status) {
		RecentLaunches.Status.Available -> null
		RecentLaunches.Status.MissingFiles -> stringResource(R.string.recent_status_missing)
		RecentLaunches.Status.ModelUnavailable -> stringResource(R.string.recent_status_model_unavailable)
	}

	Row(
		modifier = Modifier.fillMaxWidth(),
		verticalAlignment = Alignment.CenterVertically
	) {
		Column(
			modifier = Modifier
				.weight(1f)
				.clip(RoundedCornerShape(12.dp))
				.dpadFocusIndicator()
				.combinedClickable(
					// An entry that cannot be launched opens its actions instead, so Remove is one tap away.
					onClickLabel = stringResource(if (launchable) R.string.action_launch else R.string.action_show_actions),
					onClick = {
						when {
							!launchable -> showMenu = true
							!launchInProgress -> onLaunch()
						}
					},
					onLongClickLabel = stringResource(R.string.action_show_actions),
					onLongClick = { showMenu = true }
				)
				.padding(horizontal = 12.dp, vertical = 8.dp)
		) {
			val contentAlpha = if (launchable) 1f else UNAVAILABLE_ALPHA
			Text(
				details.title,
				style = MaterialTheme.typography.titleSmall,
				modifier = Modifier.alpha(contentAlpha)
			)
			Text(
				stringResource(R.string.recent_entry_summary, stringResource(details.type.labelRes), details.detail),
				style = MaterialTheme.typography.bodySmall,
				color = MaterialTheme.colorScheme.onSurfaceVariant,
				modifier = Modifier.alpha(contentAlpha)
			)
			if (statusText != null) {
				Text(
					statusText,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
		Box {
			IconButton(
				onClick = { showMenu = true },
				modifier = Modifier.dpadFocusIndicator()
			) {
				Icon(
					Icons.Default.MoreVert,
					contentDescription = stringResource(R.string.action_more_for_named, details.title)
				)
			}
			DropdownMenu(
				expanded = showMenu,
				onDismissRequest = { showMenu = false }
			) {
				DropdownMenuItem(
					text = { Text(stringResource(R.string.action_remove_from_recent)) },
					onClick = {
						showMenu = false
						onRemove()
					},
					leadingIcon = {
						Icon(
							Icons.Default.RemoveCircleOutline,
							contentDescription = null
						)
					},
					modifier = Modifier.dpadFocusIndicator()
				)
			}
		}
	}
}
