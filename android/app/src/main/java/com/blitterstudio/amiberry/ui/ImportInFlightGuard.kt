package com.blitterstudio.amiberry.ui

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

internal class ImportInFlightGuard {
	var isImporting by mutableStateOf(false)
		private set

	fun begin(): Boolean {
		if (isImporting) {
			return false
		}
		isImporting = true
		return true
	}

	fun finish() {
		isImporting = false
	}
}

@Composable
internal fun rememberImportInFlightGuard(): ImportInFlightGuard =
	remember { ImportInFlightGuard() }

internal fun CoroutineScope.launchImportGuarded(
	guard: ImportInFlightGuard,
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
