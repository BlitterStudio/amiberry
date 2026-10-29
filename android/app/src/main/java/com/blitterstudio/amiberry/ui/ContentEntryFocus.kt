package com.blitterstudio.amiberry.ui

import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester

internal val LocalContentEntryFocus = staticCompositionLocalOf { FocusRequester() }
fun Modifier.contentEntryFocus(): Modifier = composed { focusRequester(LocalContentEntryFocus.current) }
