package com.blitterstudio.amiberry.data

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.mutableStateOf
import androidx.core.content.edit

class AppPreferences private constructor(context: Context) {

	private val prefs: SharedPreferences =
		context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

	var useDynamicColor = mutableStateOf(prefs.getBoolean(KEY_DYNAMIC_COLOR, true))
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

	fun setHasSeenWelcome(seen: Boolean) {
		hasSeenWelcome.value = seen
		prefs.edit { putBoolean(KEY_HAS_SEEN_WELCOME, seen) }
	}

	companion object {
		private const val PREFS_NAME = "amiberry_prefs"
		private const val KEY_DYNAMIC_COLOR = "use_dynamic_color"
		private const val KEY_HAS_SEEN_WELCOME = "has_seen_welcome"
		private const val KEY_STORAGE_PERMISSION_REQUESTED = "storage_permission_requested"
		private const val KEY_LAST_WHDLOAD = "last_whdload_path"
		private const val KEY_LAUNCH_COUNT = "emulator_launch_count"
		private const val FIRST_REVIEW_LAUNCH = 5
		private const val REVIEW_INTERVAL = 20

		@Volatile
		private var instance: AppPreferences? = null

		fun getInstance(context: Context): AppPreferences =
			instance ?: synchronized(this) {
				instance ?: AppPreferences(context).also { instance = it }
			}
	}
}
