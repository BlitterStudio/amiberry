package com.blitterstudio.amiberry.ui.components

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationDrawerItem
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.onFocusChanged
import kotlinx.coroutines.delay
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.ui.dpadFocusIndicator

/** One choice of a [SidePaneList]. */
data class SidePaneEntry(
	val label: String,
	val icon: ImageVector? = null,
	/** Short text at the end of the row, such as a count. */
	val badge: String? = null,
	val testTag: String? = null
)

/**
 * The vertical list a wide window shows beside a screen's content in place of its
 * scrolling tab row (Settings' tabs, the Files categories). OK/tap selects, as on the
 * tab row; with [selectOnFocus] an entry D-pad focus rests on is selected too (the TV
 * pattern: the page follows the focus). Right goes to [contentFocus], the page beside
 * the list (give it focusRestorer(): it enters at the control last focused there, or
 * the page's first), rather than to whatever the 2D search hits at the entry's height.
 * Returning with D-pad Left lands on the entry last focused here, or on the selected
 * one.
 */
@Composable
fun SidePaneList(
	entries: List<SidePaneEntry>,
	selectedIndex: Int,
	onSelect: (Int) -> Unit,
	modifier: Modifier = Modifier,
	contentFocus: FocusRequester? = null,
	selectOnFocus: Boolean = false
) {
	val requesters = remember(entries.size) { List(entries.size) { FocusRequester() } }
	var focusedIndex by remember { mutableIntStateOf(-1) }
	val currentSelected by rememberUpdatedState(selectedIndex)
	val currentOnSelect by rememberUpdatedState(onSelect)
	if (selectOnFocus) {
		// Debounced: moving through several entries loads only the one focus stops on.
		LaunchedEffect(focusedIndex) {
			val index = focusedIndex
			if (index < 0 || index == currentSelected) return@LaunchedEffect
			delay(SELECT_ON_FOCUS_DELAY_MS)
			currentOnSelect(index)
		}
	}
	Column(
		modifier = modifier
			.width(SIDE_PANE_WIDTH)
			.fillMaxHeight()
			.focusRestorer(requesters.getOrElse(selectedIndex) { FocusRequester.Default })
			.verticalScroll(rememberScrollState())
			.padding(horizontal = 12.dp, vertical = 8.dp)
	) {
		entries.forEachIndexed { index, entry ->
			NavigationDrawerItem(
				label = { Text(entry.label) },
				selected = index == selectedIndex,
				onClick = { onSelect(index) },
				icon = entry.icon?.let { icon -> { Icon(icon, contentDescription = null) } },
				badge = entry.badge?.let { badge -> { Text(badge) } },
				modifier = Modifier
					.focusRequester(requesters[index])
					.then(entry.testTag?.let { Modifier.testTag(it) } ?: Modifier)
					.onFocusChanged {
						if (it.isFocused) focusedIndex = index else if (focusedIndex == index) focusedIndex = -1
					}
					.focusProperties { contentFocus?.let { right = it } }
					.dpadFocusIndicator()
			)
		}
	}
}

private const val SELECT_ON_FOCUS_DELAY_MS = 300L

private val SIDE_PANE_WIDTH = 220.dp
