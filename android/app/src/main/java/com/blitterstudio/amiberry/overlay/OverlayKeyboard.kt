package com.blitterstudio.amiberry.overlay

import androidx.compose.foundation.background
import androidx.compose.foundation.focusable
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import kotlin.math.min

/**
 * The on-screen keyboard options of the running configuration, read from
 * the settings layer as the menu reads its options.
 */
data class KeyboardOptions(
	val keypad: Boolean = false,
	/** "US", "UK", "DE" or "FR", as amiberry.vkbd_language holds. */
	val layout: String = "US",
	/** 0..1; amiberry.vkbd_transparency's 0 means the default. */
	val opacity: Float = 0.85f,
	/**
	 * Whether Caps Lock can latch: only the MCU keyboard models' LED
	 * follows the guest's own Caps state (the CIA LED), so only they can
	 * show it back. The UAE model's status follows the host keyboard's
	 * Caps, which never reflects this keyboard's keys.
	 */
	val capsLatches: Boolean = false
) {
	companion object {
		const val LANGUAGE = "amiberry.vkbd_language"
		const val NUMPAD = "amiberry.vkbd_numpad"
		const val TRANSPARENCY = "amiberry.vkbd_transparency"
		const val KEYBOARD_TYPE = "keyboard_type"
		val IDS = listOf(LANGUAGE, NUMPAD, TRANSPARENCY, KEYBOARD_TYPE)

		/** The keyboard models whose controller emulates the Caps LED the guest writes. */
		private val CAPS_LED_KEYBOARD_TYPES = setOf(
			"a500_6570-036", "a600_6570-036", "a1000_6500-1", "a1000_6570-036",
			"a1200_6805", "a2000_8039", "ax000_6570-036"
		)

		/** The options of the [SettingState]s [OverlayNative.readSettings] returned; unknown values fall back. */
		fun from(settings: Map<String, SettingState>): KeyboardOptions {
			val layout = settings[LANGUAGE]?.value?.takeIf { it.isNotBlank() } ?: "US"
			val keypad = settings[NUMPAD]?.value == "true"
			val percent = settings[TRANSPARENCY]?.value?.toFloatOrNull()
			val opacity = if (percent != null && percent > 0f) min(percent, 100f) / 100f else 0.85f
			val capsLatches = settings[KEYBOARD_TYPE]?.value in CAPS_LED_KEYBOARD_TYPES
			return KeyboardOptions(keypad = keypad, layout = layout, opacity = opacity, capsLatches = capsLatches)
		}
	}
}

/** A cell of a block: an Amiga key, the keyboard's own Hide key, or a gap (null). */
private data class Cell(val key: AmigaKey?, val hide: Boolean = false)

/**
 * The native on-screen Amiga keyboard (apple/Shared/UI/AmigaKeyboardView.swift
 * is the model this follows): the main block with the cursor keys and the
 * Hide key, and the numeric keypad when amiberry.vkbd_numpad asks for it.
 * Several keys can be held at once; every key works with touch and with the
 * D-pad (its view holds the Android focus while it shows, as the menu does).
 */
@Composable
fun OverlayKeyboard(
	model: AmigaKeyboardModel,
	options: KeyboardOptions,
	onHide: () -> Unit,
	modifier: Modifier = Modifier
) {
	val blocks = remember(options.keypad) { blocks(options.keypad) }
	// The key the D-pad has pressed (Center, Enter, the gamepad's A): the
	// focus may have moved to another key by the time it comes up, so the
	// release goes to this one, never to whichever keycap sees the key up.
	val selected = remember { mutableStateOf<Cell?>(null) }
	Box(
		modifier = modifier
			.fillMaxWidth()
			.graphicsLayer { alpha = options.opacity }
			.background(MaterialTheme.colorScheme.surfaceContainerLow)
	) {
		BoxWithConstraints(
			modifier = Modifier
				.fillMaxWidth()
				.heightIn(max = MaxHeight)
		) {
			val rows = AmigaKeyboardLayout.MAIN.size
			val totalUnits = blocks.sumOf { units(it) }
			val unit = minOf(
				(maxWidth - Separation * (blocks.size - 1) - Gap * 2) / totalUnits.toFloat(),
				(maxHeight - Gap * (rows + 1)) / rows.toFloat()
			)
			Row(
				horizontalArrangement = Arrangement.spacedBy(Separation),
				modifier = Modifier.padding(Gap)
			) {
				blocks.forEach { block ->
					Column(verticalArrangement = Arrangement.spacedBy(Gap)) {
						// Every row has the fixed key height, the empty ones
						// between Hide and the cursor keys included, so the
						// cluster lines up with the main block's rows and
						// D-pad navigation pairs with the right neighbours.
						block.forEach { row ->
							Row(
								horizontalArrangement = Arrangement.spacedBy(Gap),
								modifier = Modifier.height(unit - Gap)
							) {
								row.forEach { cell ->
									if (cell == null) {
										Spacer(Modifier.width(unit - Gap).height(unit - Gap))
									} else {
										KeyCap(cell, unit, options.layout, model, selected, onHide)
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

/** The blocks side by side: main, cursor (with the Hide key above Up), the keypad when asked for. */
private fun blocks(keypad: Boolean): List<List<List<Cell?>>> = buildList {
	add(AmigaKeyboardLayout.MAIN.map { row -> row.map { Cell(it) } })
	add(
		AmigaKeyboardLayout.CURSOR.mapIndexed { index, row ->
			if (index == 0) listOf(Cell(HIDE_KEY.copy(width = 2.0), hide = true))
			else row.map { it?.let(::Cell) }
		}
	)
	if (keypad) add(AmigaKeyboardLayout.KEYPAD.map { row -> row.map { Cell(it) } })
}

/** The width of a block in key units (a gap counts one). */
private fun units(block: List<List<Cell?>>): Double =
	block.maxOf { row -> row.sumOf { it?.key?.width ?: 1.0 } }

/** The keycap that hides the keyboard: not an AK_* code, so nothing reaches the emulation. */
private val HIDE_KEY = AmigaKey(-1, "Hide")

@Composable
private fun KeyCap(
	cell: Cell,
	unit: Dp,
	layout: String,
	model: AmigaKeyboardModel,
	/** The key the D-pad holds down, shared by every cap (see [OverlayKeyboard]). */
	selected: MutableState<Cell?>,
	onHide: () -> Unit
) {
	val key = cell.key ?: return
	val down = !cell.hide && key.code in model.down
	val legend = remember(key, layout) { AmigaKeyboardLayout.legend(key, layout) }
	val press = { if (cell.hide) Unit else model.touchDown(key) }
	val lift = { if (cell.hide) onHide() else model.touchUp(key) }
	val cancel = { if (cell.hide) Unit else model.touchCancelled(key) }
	// The select key pressed here; releasing it releases what it pressed,
	// even if the focus (and so the key-up) has moved to another cap.
	val selectDown = {
		if (selected.value == null) {
			selected.value = cell
			press()
		}
	}
	val selectUp = {
		selected.value?.let { held ->
			selected.value = null
			if (held.hide) onHide() else model.touchUp(held.key!!)
		}
		Unit
	}
	val contentColor = if (down) MaterialTheme.colorScheme.onPrimary else MaterialTheme.colorScheme.onSurface
	Box(
		modifier = Modifier
			.width(unit * key.width.toFloat() - Gap)
			.height(unit - Gap)
			.clip(RoundedCornerShape(5.dp))
			.background(
				when {
					down -> MaterialTheme.colorScheme.primary
					cell.hide -> MaterialTheme.colorScheme.secondaryContainer
					else -> MaterialTheme.colorScheme.surfaceContainerHighest
				}
			)
			.dpadFocusIndicator()
			.onFocusChanged { state ->
				// The focus moved while the select key was held: what it
				// pressed comes up, or it would stay down in the Amiga.
				if (!state.isFocused && selected.value != null) selectUp()
			}
			.focusable()
			.semantics {
				role = Role.Button
				contentDescription = legend.label.ifEmpty { "Space" }
			}
			.onPreviewKeyEvent { event ->
				val select = event.key == Key.DirectionCenter || event.key == Key.Enter
				when {
					event.type == KeyEventType.KeyDown && select && event.nativeKeyEvent.repeatCount == 0 -> {
						selectDown(); true
					}
					event.type == KeyEventType.KeyUp && select -> {
						selectUp(); true
					}
					else -> false
				}
			}
			.pointerInput(key.code) {
				awaitEachGesture {
					awaitFirstDown()
					press()
					var lifted = false
					try {
						while (true) {
							val event = awaitPointerEvent()
							if (event.motionEvent?.actionMasked == android.view.MotionEvent.ACTION_CANCEL) break
							if (event.changes.none { it.pressed }) {
								lifted = true
								break
							}
						}
					} finally {
						if (lifted) lift() else cancel()
					}
				}
			},
		contentAlignment = Alignment.Center
	) {
		Column(horizontalAlignment = Alignment.CenterHorizontally) {
			legend.shifted?.let {
				Text(it, style = MaterialTheme.typography.labelSmall, color = contentColor)
			}
			Text(
				legend.label,
				fontSize = when (legend.label.length) {
					in 0..1 -> 15.sp
					in 2..4 -> 10.sp
					else -> 8.sp
				},
				fontWeight = FontWeight.Medium,
				textAlign = TextAlign.Center,
				softWrap = false,
				maxLines = 1,
				color = contentColor
			)
		}
	}
}

private val Gap = 4.dp
private val Separation = 10.dp
/** The tallest the keyboard gets, so the emulation stays in view (half a phone screen at most). */
private val MaxHeight = 280.dp
