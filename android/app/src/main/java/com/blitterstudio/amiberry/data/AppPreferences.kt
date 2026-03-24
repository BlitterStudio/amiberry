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

	fun setDynamicColor(enabled: Boolean) {
		useDynamicColor.value = enabled
		prefs.edit { putBoolean(KEY_DYNAMIC_COLOR, enabled) }
	}

	companion object {
		private const val PREFS_NAME = "amiberry_prefs"
		private const val KEY_DYNAMIC_COLOR = "use_dynamic_color"

		@Volatile
		private var instance: AppPreferences? = null

		fun getInstance(context: Context): AppPreferences =
			instance ?: synchronized(this) {
				instance ?: AppPreferences(context).also { instance = it }
			}
	}
}
