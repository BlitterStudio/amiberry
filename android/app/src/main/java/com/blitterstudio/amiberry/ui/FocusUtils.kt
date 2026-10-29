package com.blitterstudio.amiberry.ui

import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.content.pm.PackageManager
import android.os.Build
import android.os.Environment
import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.border
import androidx.compose.foundation.focusable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.Stable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.focus.FocusDirection
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onKeyEvent
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.platform.SoftwareKeyboardController
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
 * Returns true when the app has full external storage access.
 * On Android 10 and below, this is always true (legacy storage).
 * On Android 11+, requires MANAGE_EXTERNAL_STORAGE permission.
 *
 * Re-checks on every lifecycle ON_RESUME so the banner disappears
 * immediately when the user returns from granting permission in Settings.
 */
@Composable
fun hasFullStorageAccess(): Boolean {
	if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) return true

	// Force re-evaluation on every resume by bumping a counter
	var resumeCount by remember { mutableIntStateOf(0) }
	val lifecycleOwner = LocalLifecycleOwner.current
	DisposableEffect(lifecycleOwner) {
		val observer = LifecycleEventObserver { _, event ->
			if (event == Lifecycle.Event.ON_RESUME) {
				resumeCount++
			}
		}
		lifecycleOwner.lifecycle.addObserver(observer)
		onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
	}

	// resumeCount is read here, making this recompose on every resume
	@Suppress("UNUSED_EXPRESSION")
	resumeCount
	return Environment.isExternalStorageManager()
}

/**
 * Walk the ContextWrapper chain to find the hosting Activity.
 * Safer than a direct cast of LocalContext.current, which can fail
 * if the context is wrapped (e.g., by theming or configuration).
 */
fun Context.findActivity(): Activity {
	var ctx = this
	while (ctx is ContextWrapper) {
		if (ctx is Activity) return ctx
		ctx = ctx.baseContext
	}
	throw IllegalStateException("No Activity found in context chain")
}

/**
 * Non-composable version for use in ViewModels or non-Compose code.
 */
fun hasFullStorageAccess(context: Context): Boolean {
	return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
		Environment.isExternalStorageManager()
	} else {
		true
	}
}

/**
 * Non-composable version for use in ViewModels or non-Compose code.
 */
fun isTvDevice(context: Context): Boolean {
	return context.packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)
}

/**
 * Non-composable version for use in ViewModels or non-Compose code.
 */
fun hasTouchScreen(context: Context): Boolean {
	return context.packageManager.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN)
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

/**
 * Text fields consume the D-pad arrows as text-cursor movement, trapping TV focus.
 * Up/Down move focus to the neighbouring control instead; Left/Right still move the
 * cursor of an editable single-line field.
 */
fun Modifier.dpadVerticalMovesFocus(): Modifier = arrowsMoveFocus(horizontal = false)

/**
 * Read-only dropdown anchors (ExposedDropdownMenuBox text fields) have no cursor to
 * move, yet consume all four D-pad arrows: every arrow moves focus instead, so Left/Right
 * reach an Eject button beside the field or the navigation rail.
 */
fun Modifier.dpadArrowsMoveFocus(): Modifier = arrowsMoveFocus(horizontal = true)

private fun Modifier.arrowsMoveFocus(horizontal: Boolean): Modifier = composed {
	val focusManager = LocalFocusManager.current
	this.onPreviewKeyEvent { event ->
		if (event.type != KeyEventType.KeyDown) return@onPreviewKeyEvent false
		when (event.key) {
			Key.DirectionDown -> focusManager.moveFocus(FocusDirection.Down)
			Key.DirectionUp -> focusManager.moveFocus(FocusDirection.Up)
			Key.DirectionLeft -> horizontal && focusManager.moveFocus(FocusDirection.Left)
			Key.DirectionRight -> horizontal && focusManager.moveFocus(FocusDirection.Right)
			else -> false
		}
	}
}

/**
 * A Slider takes all four arrow keys, so D-pad Up/Down changed the value and focus
 * could never leave it. Up/Down move to the neighbouring control instead (and never
 * change the value, even with nothing in that direction); Left/Right still adjust it,
 * and are consumed at either end so focus does not jump sideways off the slider.
 * Pass it in the Slider's modifier so it wraps the slider's own key handling.
 */
fun Modifier.dpadSliderKeys(): Modifier = composed {
	val focusManager = LocalFocusManager.current
	this.onPreviewKeyEvent { event ->
		when (event.key) {
			Key.DirectionDown -> {
				if (event.type == KeyEventType.KeyDown) focusManager.moveFocus(FocusDirection.Down)
				true
			}
			Key.DirectionUp -> {
				if (event.type == KeyEventType.KeyDown) focusManager.moveFocus(FocusDirection.Up)
				true
			}
			else -> false
		}
	}.onKeyEvent { event ->
		// Reached only when the slider itself did not use the key (at its end).
		event.key == Key.DirectionLeft || event.key == Key.DirectionRight
	}
}

/**
 * A TV IME opens as soon as an editable text field gains focus and then takes the
 * D-pad, so moving through a form got stuck in the keyboard. On a TV the field stays
 * read-only ([readOnly]) until OK/Enter is pressed on it, which opens the IME, and
 * becomes read-only again when focus leaves it or [stopTyping] is called (IME Done).
 * Apply [modifier] to the text field. Elsewhere nothing changes. (TV, not "no
 * touchscreen": TV emulators report one, and mouse users elsewhere press no OK key.)
 */
@Stable
class DpadTypingGate internal constructor(
	private val gated: Boolean,
	private val keyboardController: SoftwareKeyboardController?
) {
	private var typing by mutableStateOf(!gated)

	val readOnly: Boolean get() = !typing

	fun stopTyping() {
		if (gated) typing = false
	}

	val modifier: Modifier = if (!gated) Modifier else Modifier
		.onPreviewKeyEvent { event ->
			if (event.type == KeyEventType.KeyDown &&
				(event.key == Key.DirectionCenter || event.key == Key.Enter || event.key == Key.NumPadEnter)
			) {
				// Becoming editable starts the input session, which shows the IME; once
				// editable (IME closed with Back), OK shows it again.
				if (typing) keyboardController?.show() else typing = true
			}
			false
		}
		.onFocusChanged { if (!it.isFocused) typing = false }
}

@Composable
fun rememberDpadTypingGate(): DpadTypingGate {
	val tv = isTvDevice()
	val keyboardController = LocalSoftwareKeyboardController.current
	return remember(tv, keyboardController) { DpadTypingGate(tv, keyboardController) }
}
