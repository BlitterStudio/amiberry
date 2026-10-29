package com.blitterstudio.amiberry.data

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.mutableStateOf
import androidx.core.content.edit
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.EmulatorSettingsConstraints
import org.json.JSONObject

class AppPreferences private constructor(context: Context) {

	private val prefs: SharedPreferences =
		context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

	init {
		migrateLegacyKeyboardSwitch()
	}

	var useDynamicColor = mutableStateOf(prefs.getBoolean(KEY_DYNAMIC_COLOR, true))
		private set

	/** Theme mode: "system", "light", or "dark" */
	var themeMode = mutableStateOf(prefs.getString(KEY_THEME_MODE, "system") ?: "system")
		private set

	var hasSeenWelcome = mutableStateOf(prefs.getBoolean(KEY_HAS_SEEN_WELCOME, false))
		private set

	var hasRequestedStoragePermission: Boolean
		get() = prefs.getBoolean(KEY_STORAGE_PERMISSION_REQUESTED, false)
		set(value) { prefs.edit { putBoolean(KEY_STORAGE_PERMISSION_REQUESTED, value) } }

	var lastWhdloadPath: String
		get() = prefs.getString(KEY_LAST_WHDLOAD, "") ?: ""
		set(value) { prefs.edit { putString(KEY_LAST_WHDLOAD, value) } }

	/**
	 * Whether the open-GUI path during emulation (Back, Start, the swipe)
	 * shows the built-in ImGui settings instead of the in-game menu. Read
	 * by the emulator process as it starts, so a change applies to the next
	 * launch.
	 */
	var useClassicInGameMenu = mutableStateOf(prefs.getBoolean(KEY_CLASSIC_IN_GAME_MENU, false))
		private set

	fun setClassicInGameMenu(enabled: Boolean) {
		useClassicInGameMenu.value = enabled
		prefs.edit { putBoolean(KEY_CLASSIC_IN_GAME_MENU, enabled) }
	}

	fun rememberedAndroidControls(): EmulatorSettings =
		EmulatorSettings(
			joyport0 = prefs.getString(KEY_LAST_JOYPORT0, DEFAULT_JOYPORT0) ?: DEFAULT_JOYPORT0,
			joyport1 = prefs.getString(KEY_LAST_JOYPORT1, DEFAULT_JOYPORT1) ?: DEFAULT_JOYPORT1,
			onScreenJoystick = prefs.getBoolean(KEY_LAST_ON_SCREEN_JOYSTICK, DEFAULT_ON_SCREEN_JOYSTICK),
			onScreenKeyboardButton = prefs.getBoolean(KEY_LAST_ON_SCREEN_KEYBOARD_BUTTON, DEFAULT_ON_SCREEN_KEYBOARD),
			onScreenKeyboardNumpad = prefs.getBoolean(KEY_LAST_ON_SCREEN_KEYBOARD_NUMPAD, false),
			onScreenKeyboardToggle = prefs.getString(KEY_LAST_ON_SCREEN_KEYBOARD_TOGGLE, null),
			joyport0MouseMap = prefs.getBoolean(KEY_LAST_JOYPORT0_MOUSEMAP, false),
			joyport1MouseMap = prefs.getBoolean(KEY_LAST_JOYPORT1_MOUSEMAP, false),
			analogMouseSpeed = prefs.getInt(KEY_LAST_ANALOG_MOUSE_SPEED, EmulatorSettingsConstraints.DEFAULT_ANALOG_MOUSE_SPEED)
		)

	fun applyRememberedAndroidControls(settings: EmulatorSettings, explicitKeys: Set<String>): EmulatorSettings =
		AndroidControlSettings.withFallback(
			settings = settings,
			explicitKeys = explicitKeys,
			fallback = rememberedAndroidControls()
		)

	fun saveAndroidControls(settings: EmulatorSettings) {
		prefs.edit {
			putString(KEY_LAST_JOYPORT0, settings.joyport0)
			putString(KEY_LAST_JOYPORT1, settings.joyport1)
			putBoolean(KEY_LAST_ON_SCREEN_JOYSTICK, settings.onScreenJoystick)
			putBoolean(KEY_LAST_ON_SCREEN_KEYBOARD_BUTTON, settings.onScreenKeyboardButton)
			putBoolean(KEY_LAST_ON_SCREEN_KEYBOARD_NUMPAD, settings.onScreenKeyboardNumpad)
			putString(KEY_LAST_ON_SCREEN_KEYBOARD_TOGGLE, settings.onScreenKeyboardToggle)
			putBoolean(KEY_LAST_JOYPORT0_MOUSEMAP, settings.joyport0MouseMap)
			putBoolean(KEY_LAST_JOYPORT1_MOUSEMAP, settings.joyport1MouseMap)
			putInt(KEY_LAST_ANALOG_MOUSE_SPEED, settings.analogMouseSpeed)
		}
	}

	/**
	 * [KEY_LAST_ON_SCREEN_KEYBOARD] used to hold the keyboard master switch. Off meant
	 * no keyboard at all, so carry it over as button off plus a disabled gamepad toggle;
	 * otherwise the default toggle would re-enable the keyboard after an upgrade.
	 */
	private fun migrateLegacyKeyboardSwitch() {
		if (!prefs.contains(KEY_LAST_ON_SCREEN_KEYBOARD)) return
		val keyboardEnabled = prefs.getBoolean(KEY_LAST_ON_SCREEN_KEYBOARD, DEFAULT_ON_SCREEN_KEYBOARD)
		prefs.edit {
			putBoolean(KEY_LAST_ON_SCREEN_KEYBOARD_BUTTON, keyboardEnabled)
			if (!keyboardEnabled) putString(KEY_LAST_ON_SCREEN_KEYBOARD_TOGGLE, "")
			remove(KEY_LAST_ON_SCREEN_KEYBOARD)
		}
	}

	/**
	 * Recently launched items, most recent first. Observable by Compose —
	 * reading [recentLaunches] in a composable triggers recomposition when
	 * [addRecentLaunch] updates the list.
	 *
	 * Each entry is a JSON object: {"type":"quickstart","model":"A500","df0":"...","df1":"","cd":""}
	 * or {"type":"config","path":"..."} etc.
	 */
	var recentLaunches = mutableStateOf(loadRecentLaunches())
		private set

	private fun loadRecentLaunches(): List<JSONObject> {
		val raw = prefs.getString(KEY_RECENT_LAUNCHES, "[]") ?: "[]"
		return RecentLaunches.parse(raw)
	}

	fun addRecentLaunch(entry: JSONObject) {
		replaceRecentLaunches(RecentLaunches.add(recentLaunches.value, entry))
	}

	fun removeRecentLaunch(entry: JSONObject) {
		replaceRecentLaunches(RecentLaunches.remove(recentLaunches.value, entry))
	}

	fun clearRecentLaunches() {
		replaceRecentLaunches(emptyList())
	}

	/** Keep Recent pointing at a configuration after it is renamed. */
	fun renameRecentConfig(oldPath: String, newPath: String) {
		val updated = RecentLaunches.replaceConfigPath(recentLaunches.value, oldPath, newPath)
		if (updated.map { it.toString() } != recentLaunches.value.map { it.toString() }) {
			replaceRecentLaunches(updated)
		}
	}

	/** Take a configuration the user deleted in the app out of Recent. */
	fun forgetRecentConfig(path: String) {
		val updated = RecentLaunches.removeConfigPath(recentLaunches.value, path)
		if (updated.size != recentLaunches.value.size) {
			replaceRecentLaunches(updated)
		}
	}

	private fun replaceRecentLaunches(entries: List<JSONObject>) {
		val trimmed = entries.take(RecentLaunches.MAX_ITEMS)
		prefs.edit { putString(KEY_RECENT_LAUNCHES, RecentLaunches.serialize(trimmed)) }
		recentLaunches.value = trimmed
	}

	/**
	 * Increment the emulator launch counter and return true if it's time
	 * to request an in-app review (after every Nth successful launch).
	 */
	fun incrementLaunchCountAndCheckReview(): Boolean {
		val count = prefs.getInt(KEY_LAUNCH_COUNT, 0) + 1
		prefs.edit { putInt(KEY_LAUNCH_COUNT, count) }
		// Request review after the 5th launch, then every 20 launches
		return count == FIRST_REVIEW_LAUNCH || (count > FIRST_REVIEW_LAUNCH && (count - FIRST_REVIEW_LAUNCH) % REVIEW_INTERVAL == 0)
	}

	fun setDynamicColor(enabled: Boolean) {
		useDynamicColor.value = enabled
		prefs.edit { putBoolean(KEY_DYNAMIC_COLOR, enabled) }
	}

	fun setThemeMode(mode: String) {
		themeMode.value = mode
		prefs.edit { putString(KEY_THEME_MODE, mode) }
	}

	fun setHasSeenWelcome(seen: Boolean) {
		hasSeenWelcome.value = seen
		prefs.edit { putBoolean(KEY_HAS_SEEN_WELCOME, seen) }
	}

	companion object {
		private const val PREFS_NAME = "amiberry_prefs"
		private const val KEY_DYNAMIC_COLOR = "use_dynamic_color"
		private const val KEY_THEME_MODE = "theme_mode"
		private const val KEY_HAS_SEEN_WELCOME = "has_seen_welcome"
		private const val KEY_STORAGE_PERMISSION_REQUESTED = "storage_permission_requested"
		private const val KEY_LAST_WHDLOAD = "last_whdload_path"
		private const val KEY_CLASSIC_IN_GAME_MENU = "classic_in_game_menu"
		private const val KEY_LAST_JOYPORT0 = "last_joyport0"
		private const val KEY_LAST_JOYPORT1 = "last_joyport1"
		private const val KEY_LAST_ON_SCREEN_JOYSTICK = "last_on_screen_joystick"
		/** Legacy keyboard master switch; migrated by [migrateLegacyKeyboardSwitch]. */
		private const val KEY_LAST_ON_SCREEN_KEYBOARD = "last_on_screen_keyboard"
		private const val KEY_LAST_ON_SCREEN_KEYBOARD_BUTTON = "last_on_screen_keyboard_button"
		private const val KEY_LAST_ON_SCREEN_KEYBOARD_NUMPAD = "last_on_screen_keyboard_numpad"
		private const val KEY_LAST_ON_SCREEN_KEYBOARD_TOGGLE = "last_on_screen_keyboard_toggle"
		private const val KEY_LAST_JOYPORT0_MOUSEMAP = "last_joyport0_mousemap"
		private const val KEY_LAST_JOYPORT1_MOUSEMAP = "last_joyport1_mousemap"
		private const val KEY_LAST_ANALOG_MOUSE_SPEED = "last_analog_mouse_speed"
		private const val KEY_RECENT_LAUNCHES = "recent_launches"
		private const val KEY_LAUNCH_COUNT = "emulator_launch_count"
		private const val FIRST_REVIEW_LAUNCH = 5
		private const val REVIEW_INTERVAL = 20
		private const val DEFAULT_JOYPORT0 = "mouse"
		private const val DEFAULT_JOYPORT1 = "onscreen_joy"
		private const val DEFAULT_ON_SCREEN_JOYSTICK = true
		private const val DEFAULT_ON_SCREEN_KEYBOARD = true

		@Volatile
		private var instance: AppPreferences? = null

		fun getInstance(context: Context): AppPreferences =
			instance ?: synchronized(this) {
				instance ?: AppPreferences(context).also { instance = it }
			}
	}
}
