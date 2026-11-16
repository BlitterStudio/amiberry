package com.blitterstudio.amiberry.ui.components.settings

import android.view.KeyEvent as AndroidKeyEvent
import androidx.compose.foundation.focusable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
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
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.res.stringResource
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.dpadFocusIndicator

/** The .uae spellings get_hotkey_from_config() accepts for Android key events. */
internal object AndroidHotkeyFormat {
	private val names = mapOf(
		AndroidKeyEvent.KEYCODE_SPACE to "Space",
		AndroidKeyEvent.KEYCODE_ENTER to "Enter",
		AndroidKeyEvent.KEYCODE_DEL to "Backspace",
		AndroidKeyEvent.KEYCODE_FORWARD_DEL to "Delete",
		AndroidKeyEvent.KEYCODE_ESCAPE to "Escape",
		AndroidKeyEvent.KEYCODE_TAB to "Tab",
		AndroidKeyEvent.KEYCODE_DPAD_LEFT to "LeftArrow",
		AndroidKeyEvent.KEYCODE_DPAD_RIGHT to "RightArrow",
		AndroidKeyEvent.KEYCODE_DPAD_UP to "UpArrow",
		AndroidKeyEvent.KEYCODE_DPAD_DOWN to "DownArrow",
		AndroidKeyEvent.KEYCODE_INSERT to "Insert",
		AndroidKeyEvent.KEYCODE_MOVE_HOME to "Home",
		AndroidKeyEvent.KEYCODE_MOVE_END to "End",
		AndroidKeyEvent.KEYCODE_PAGE_UP to "PageUp",
		AndroidKeyEvent.KEYCODE_PAGE_DOWN to "PageDown",
		AndroidKeyEvent.KEYCODE_BREAK to "Pause",
		AndroidKeyEvent.KEYCODE_SYSRQ to "PrintScreen",
		AndroidKeyEvent.KEYCODE_SCROLL_LOCK to "ScrollLock",
		AndroidKeyEvent.KEYCODE_NUM_LOCK to "NumLock",
		AndroidKeyEvent.KEYCODE_MINUS to "Minus",
		AndroidKeyEvent.KEYCODE_EQUALS to "Equal",
		AndroidKeyEvent.KEYCODE_LEFT_BRACKET to "LeftBracket",
		AndroidKeyEvent.KEYCODE_RIGHT_BRACKET to "RightBracket",
		AndroidKeyEvent.KEYCODE_BACKSLASH to "Backslash",
		AndroidKeyEvent.KEYCODE_SEMICOLON to "Semicolon",
		AndroidKeyEvent.KEYCODE_APOSTROPHE to "Apostrophe",
		AndroidKeyEvent.KEYCODE_COMMA to "Comma",
		AndroidKeyEvent.KEYCODE_PERIOD to "Period",
		AndroidKeyEvent.KEYCODE_SLASH to "Slash",
		AndroidKeyEvent.KEYCODE_GRAVE to "GraveAccent"
	)

	private val modifiers = listOf(
		AndroidKeyEvent.META_CTRL_LEFT_ON to "LCtrl",
		AndroidKeyEvent.META_CTRL_RIGHT_ON to "RCtrl",
		AndroidKeyEvent.META_SHIFT_LEFT_ON to "LShift",
		AndroidKeyEvent.META_SHIFT_RIGHT_ON to "RShift",
		AndroidKeyEvent.META_ALT_LEFT_ON to "LAlt",
		AndroidKeyEvent.META_ALT_RIGHT_ON to "RAlt",
		AndroidKeyEvent.META_META_LEFT_ON to "LGUI",
		AndroidKeyEvent.META_META_RIGHT_ON to "RGUI"
	)

	fun modifier(keyCode: Int): String? = when (keyCode) {
		AndroidKeyEvent.KEYCODE_CTRL_LEFT -> "LCtrl"
		AndroidKeyEvent.KEYCODE_CTRL_RIGHT -> "RCtrl"
		AndroidKeyEvent.KEYCODE_SHIFT_LEFT -> "LShift"
		AndroidKeyEvent.KEYCODE_SHIFT_RIGHT -> "RShift"
		AndroidKeyEvent.KEYCODE_ALT_LEFT -> "LAlt"
		AndroidKeyEvent.KEYCODE_ALT_RIGHT -> "RAlt"
		AndroidKeyEvent.KEYCODE_META_LEFT -> "LGUI"
		AndroidKeyEvent.KEYCODE_META_RIGHT -> "RGUI"
		else -> null
	}

	fun releasedModifier(keyCode: Int, held: String?): String? =
		modifier(keyCode)?.takeIf { it == held }

	fun format(keyCode: Int, metaState: Int, allowModifiers: Boolean): String? {
		val key = when {
			keyCode in AndroidKeyEvent.KEYCODE_A..AndroidKeyEvent.KEYCODE_Z ->
				('A'.code + keyCode - AndroidKeyEvent.KEYCODE_A).toChar().toString()
			keyCode in AndroidKeyEvent.KEYCODE_0..AndroidKeyEvent.KEYCODE_9 ->
				('0'.code + keyCode - AndroidKeyEvent.KEYCODE_0).toChar().toString()
			keyCode in AndroidKeyEvent.KEYCODE_F1..AndroidKeyEvent.KEYCODE_F12 ->
				"F${keyCode - AndroidKeyEvent.KEYCODE_F1 + 1}"
			else -> names[keyCode]
		} ?: return null
		if (!allowModifiers) return key
		return modifiers.filter { (mask, _) -> metaState and mask != 0 }
			.joinToString(separator = "+") { it.second }
			.let { if (it.isEmpty()) key else "$it+$key" }
	}
}

/** A keyboard shortcut recorder for the hotkey strings the native parser reads. */
@Composable
fun SchemaHotkeyRow(
	id: String,
	value: String,
	onValueChange: (String) -> Unit,
	modifier: Modifier = Modifier,
	enabled: Boolean = true,
	reason: String? = null,
	showReason: Boolean = true,
	label: String? = null,
	help: String? = null,
	semanticsLabel: String? = null,
	option: SettingsSchema.Option? = null
) {
	val resolvedOption = option ?: schemaOption(id)
	val title = label ?: resolvedOption?.label ?: id
	val takesModifiers = id != "amiberry.left_amiga" && id != "amiberry.right_amiga"
	var recording by remember { mutableStateOf(false) }
	var heldModifier by remember { mutableStateOf<String?>(null) }
	val recorderFocus = remember { FocusRequester() }
	fun stop() {
		recording = false
		heldModifier = null
	}
	LaunchedEffect(recording) {
		if (recording) recorderFocus.requestFocus()
	}
	fun record(keyCode: Int, metaState: Int) {
		if (keyCode == AndroidKeyEvent.KEYCODE_ESCAPE && metaState == 0) {
			stop()
			return
		}
		AndroidHotkeyFormat.format(keyCode, metaState, takesModifiers)?.let {
			onValueChange(it)
			stop()
		}
	}
	Column(
		modifier = modifier
			.fillMaxWidth()
			.focusRequester(recorderFocus)
			.focusable()
			.onPreviewKeyEvent { event ->
				if (!recording) return@onPreviewKeyEvent false
				val native = event.nativeKeyEvent
				when (event.type) {
					KeyEventType.KeyDown -> if (native.repeatCount == 0) {
						AndroidHotkeyFormat.modifier(native.keyCode)?.let { heldModifier = it }
							?: record(native.keyCode, native.metaState)
						true
					} else true
					KeyEventType.KeyUp -> {
						AndroidHotkeyFormat.releasedModifier(native.keyCode, heldModifier)?.let { modifier ->
							onValueChange(modifier)
							stop()
						}
						true
					}
					else -> true
				}
			}
	) {
		Row(verticalAlignment = Alignment.CenterVertically) {
			OutlinedTextField(
				value = if (recording) stringResource(R.string.settings_hotkey_recording) else value.ifEmpty { stringResource(R.string.placeholder_none) },
				onValueChange = {},
				readOnly = true,
				enabled = enabled,
				label = { Text(title) },
				modifier = Modifier.weight(1f)
			)
			Button(
				onClick = { if (recording) stop() else recording = true },
				enabled = enabled,
				modifier = Modifier.dpadFocusIndicator()
			) { Text(stringResource(if (recording) R.string.action_cancel else R.string.settings_hotkey_record)) }
			if (value.isNotEmpty()) {
				Button(onClick = { stop(); onValueChange("") }, enabled = enabled, modifier = Modifier.dpadFocusIndicator()) {
					Text(stringResource(R.string.action_clear))
				}
			}
		}
		RowFooter(help ?: resolvedOption?.help, enabled, rowReason(resolvedOption, enabled, reason, showReason))
	}
}
