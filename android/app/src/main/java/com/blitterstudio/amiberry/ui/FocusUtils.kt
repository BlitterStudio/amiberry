package com.blitterstudio.amiberry.ui

import android.content.Context
import android.content.pm.PackageManager
import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.border
import androidx.compose.foundation.focusable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

/**
 * Returns true when running on an Android TV (leanback) device.
 * Cached per-composition so the PackageManager query only runs once.
 */
@Composable
fun isTvDevice(): Boolean {
	val context = LocalContext.current
	return remember {
		context.packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)
	}
}

/**
 * Returns true when the device has a touchscreen.
 * On Android TV, this is typically false.
 */
@Composable
fun hasTouchScreen(): Boolean {
	val context = LocalContext.current
	return remember {
		context.packageManager.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN)
	}
}

/**
 * Non-composable version for use in ViewModels or non-Compose code.
 */
fun isTvDevice(context: Context): Boolean {
	return context.packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)
}

/**
 * Modifier that adds D-pad focus support with a visible focus ring.
 * When an element receives focus (via D-pad/keyboard navigation), a colored
 * border appears around it. The border disappears when focus is lost.
 *
 * Use this on Cards, Rows, or other custom composables that aren't
 * standard Material interactive elements (which already have focus indication).
 */
fun Modifier.dpadFocusable(): Modifier = composed {
	var isFocused by remember { mutableStateOf(false) }
	val borderColor by animateColorAsState(
		targetValue = if (isFocused) MaterialTheme.colorScheme.primary else Color.Transparent,
		label = "focusBorder"
	)

	this
		.onFocusChanged { isFocused = it.isFocused }
		.border(
			width = 2.dp,
			color = borderColor,
			shape = RoundedCornerShape(12.dp)
		)
		.focusable()
}

/**
 * Lighter variant that only adds focus tracking + border styling
 * without making the element focusable (for elements that are already
 * focusable via clickable() or other means).
 */
fun Modifier.dpadFocusIndicator(): Modifier = composed {
	var isFocused by remember { mutableStateOf(false) }
	val borderColor by animateColorAsState(
		targetValue = if (isFocused) MaterialTheme.colorScheme.primary else Color.Transparent,
		label = "focusBorder"
	)

	this
		.onFocusChanged { isFocused = it.isFocused }
		.border(
			width = 2.dp,
			color = borderColor,
			shape = RoundedCornerShape(12.dp)
		)
}
