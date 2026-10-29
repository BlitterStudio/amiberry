package com.blitterstudio.amiberry.ui.components.settings

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import com.blitterstudio.amiberry.data.model.SettingText

/** The text [text] reads as, launcher resources resolved (arguments that are SettingText too). */
fun Context.settingText(text: SettingText): String = when (text) {
	is SettingText.Plain -> text.text
	is SettingText.Resource -> if (text.args.isEmpty()) {
		getString(text.id)
	} else {
		getString(text.id, *text.args.map { if (it is SettingText) settingText(it) else it }.toTypedArray())
	}
}

/** [text] as people read it, in the current configuration's language. */
@Composable
fun settingText(text: SettingText): String {
	// Read the configuration so a locale change recomposes, as stringResource does.
	LocalConfiguration.current
	return LocalContext.current.settingText(text)
}
