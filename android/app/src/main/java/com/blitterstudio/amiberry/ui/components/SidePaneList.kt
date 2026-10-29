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
import androidx.compose.runtime.remember
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
 * tab row. Returning with D-pad Left lands on the entry last focused here, or on the
 * selected one, rather than wherever the 2D search would hit.
 */
@Composable
fun SidePaneList(
	entries: List<SidePaneEntry>,
	selectedIndex: Int,
	onSelect: (Int) -> Unit,
	modifier: Modifier = Modifier
) {
	val requesters = remember(entries.size) { List(entries.size) { FocusRequester() } }
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
					.dpadFocusIndicator()
			)
		}
	}
}

private val SIDE_PANE_WIDTH = 220.dp
