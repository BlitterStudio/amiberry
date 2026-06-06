package com.blitterstudio.amiberry.ui

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

internal class LaunchInFlightGuard {
	var isLaunching by mutableStateOf(false)
		private set

	fun begin(): Boolean {
		if (isLaunching) {
			return false
		}
		isLaunching = true
		return true
	}

	fun finish() {
		isLaunching = false
	}
}

@Composable
internal fun rememberLaunchInFlightGuard(): LaunchInFlightGuard =
	remember { LaunchInFlightGuard() }

internal fun CoroutineScope.launchGuarded(
	guard: LaunchInFlightGuard,
	block: suspend () -> Unit
): Job? {
	if (!guard.begin()) {
		return null
	}

	return launch {
		try {
			block()
		} finally {
			guard.finish()
		}
	}
}
