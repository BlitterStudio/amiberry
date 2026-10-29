package com.blitterstudio.amiberry.ui

import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester

/**
 * The control D-pad focus lands on when it enters a tab's content: Right from the
 * navigation rail, and the app's first focus on a TV. The app shell ([AmiberryApp]) owns
 * the requester; outside the shell (previews, tests) nothing reads it.
 */
internal val LocalContentEntryFocus = staticCompositionLocalOf { FocusRequester() }

/**
 * Marks the screen's primary control as where focus enters its content. Without one,
 * focus enters at the content's first control in reading order, which is often a top
 * bar action. Mark one control per screen.
 */
fun Modifier.contentEntryFocus(): Modifier = composed {
	focusRequester(LocalContentEntryFocus.current)
}
