package com.blitterstudio.amiberry.ui.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.ui.findActivity

private val AmiberryRed = Color(0xFFCC3333)
private val AmiberryRedDark = Color(0xFF992222)
private val AmiberryBlue = Color(0xFF3366CC)

private val DarkColorScheme = darkColorScheme(
	primary = AmiberryRed,
	onPrimary = Color.White,
	primaryContainer = AmiberryRedDark,
	onPrimaryContainer = Color(0xFFFFDAD6),
	inversePrimary = AmiberryRedDark,
	secondary = AmiberryBlue,
	onSecondary = Color.White,
	secondaryContainer = Color(0xFF1E3C78),
	onSecondaryContainer = Color(0xFFD9E2FF),
	tertiary = Color(0xFF66BB6A),
	onTertiary = Color(0xFF00390F),
	tertiaryContainer = Color(0xFF145322),
	onTertiaryContainer = Color(0xFFB7F2B7),
	background = Color(0xFF121212),
	onBackground = Color(0xFFE6E1E1),
	surface = Color(0xFF121212),
	onSurface = Color(0xFFE6E1E1),
	surfaceVariant = Color(0xFF47464A),
	onSurfaceVariant = Color(0xFFC9C5CA),
	surfaceTint = AmiberryRed,
	inverseSurface = Color(0xFFE6E1E1),
	inverseOnSurface = Color(0xFF303033),
	outline = Color(0xFF938F94),
	outlineVariant = Color(0xFF47464A),
	scrim = Color.Black,
	surfaceDim = Color(0xFF121212),
	surfaceBright = Color(0xFF393939),
	surfaceContainerLowest = Color(0xFF0D0D0D),
	surfaceContainerLow = Color(0xFF1B1B1B),
	surfaceContainer = Color(0xFF1F1F1F),
	surfaceContainerHigh = Color(0xFF2A2A2A),
	surfaceContainerHighest = Color(0xFF353535)
)

private val LightColorScheme = lightColorScheme(
	primary = AmiberryRedDark,
	onPrimary = Color.White,
	primaryContainer = Color(0xFFFFDAD6),
	onPrimaryContainer = Color(0xFF410006),
	inversePrimary = Color(0xFFFFB3AD),
	secondary = AmiberryBlue,
	onSecondary = Color.White,
	secondaryContainer = Color(0xFFD9E2FF),
	onSecondaryContainer = Color(0xFF001A41),
	tertiary = Color(0xFF388E3C),
	onTertiary = Color.White,
	tertiaryContainer = Color(0xFFB7F2B7),
	onTertiaryContainer = Color(0xFF002108),
	background = Color(0xFFFFF8F7),
	onBackground = Color(0xFF211A1A),
	surface = Color(0xFFFFF8F7),
	onSurface = Color(0xFF211A1A),
	surfaceVariant = Color(0xFFF5DDDB),
	onSurfaceVariant = Color(0xFF534342),
	surfaceTint = AmiberryRedDark,
	inverseSurface = Color(0xFF362F2F),
	inverseOnSurface = Color(0xFFFFEDEC),
	outline = Color(0xFF857372),
	outlineVariant = Color(0xFFD8C2C0),
	scrim = Color.Black,
	surfaceDim = Color(0xFFE7D6D5),
	surfaceBright = Color(0xFFFFF8F7),
	surfaceContainerLowest = Color.White,
	surfaceContainerLow = Color(0xFFFFF0EF),
	surfaceContainer = Color(0xFFFFE9E8),
	surfaceContainerHigh = Color(0xFFF9E3E1),
	surfaceContainerHighest = Color(0xFFF3DDDC)
)

@Composable
fun AmiberryTheme(
	content: @Composable () -> Unit
) {
	val context = LocalContext.current
	val view = LocalView.current
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

	SideEffect {
		val window = view.context.findActivity().window
		WindowCompat.getInsetsController(window, view).apply {
			isAppearanceLightStatusBars = !darkTheme
			isAppearanceLightNavigationBars = !darkTheme
		}
	}

	MaterialTheme(
		colorScheme = colorScheme,
		content = content
	)
}
