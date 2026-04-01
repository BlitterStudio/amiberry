package com.blitterstudio.amiberry.ui.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import com.blitterstudio.amiberry.data.AppPreferences

private val AmiberryRed = Color(0xFFCC3333)
private val AmiberryRedDark = Color(0xFF992222)
private val AmiberryBlue = Color(0xFF3366CC)

private val DarkColorScheme = darkColorScheme(
	primary = AmiberryRed,
	secondary = AmiberryBlue,
	tertiary = Color(0xFF66BB6A)
)

private val LightColorScheme = lightColorScheme(
	primary = AmiberryRedDark,
	secondary = AmiberryBlue,
	tertiary = Color(0xFF388E3C)
)

@Composable
fun AmiberryTheme(
	content: @Composable () -> Unit
) {
	val context = LocalContext.current
	val prefs = AppPreferences.getInstance(context)
	val useDynamicColor by prefs.useDynamicColor
	val themeMode by prefs.themeMode
	val dynamicColor = useDynamicColor && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S

	val darkTheme = when (themeMode) {
		"dark" -> true
		"light" -> false
		else -> isSystemInDarkTheme()
	}

	val colorScheme = when {
		dynamicColor -> {
			if (darkTheme) dynamicDarkColorScheme(context) else dynamicLightColorScheme(context)
		}
		darkTheme -> DarkColorScheme
		else -> LightColorScheme
	}

	MaterialTheme(
		colorScheme = colorScheme,
		content = content
	)
}
