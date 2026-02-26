package com.blitterstudio.amiberry.ui.viewmodel

import android.app.Application
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import com.blitterstudio.amiberry.data.ConfigGenerator
import com.blitterstudio.amiberry.data.FileRepository
import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.ui.hasTouchScreen
import kotlinx.coroutines.flow.StateFlow

class SettingsViewModel(application: Application) : AndroidViewModel(application) {

	private val repository = FileRepository.getInstance(application)

	var settings by mutableStateOf(EmulatorSettings())
		private set

	val availableRoms: StateFlow<List<AmigaFile>> = repository.roms
	val availableFloppies: StateFlow<List<AmigaFile>> = repository.floppies
	val availableCds: StateFlow<List<AmigaFile>> = repository.cdImages

	init {
		settings = applyConstraints(settings)
		repository.rescan()
	}

	fun applyModel(model: AmigaModel) {
		settings = EmulatorSettings.fromModel(model)
	}

	fun updateSettings(transform: (EmulatorSettings) -> EmulatorSettings) {
		val newSettings = transform(settings)
		settings = applyConstraints(newSettings)
	}

	fun generateLaunchArgs(): Array<String> {
		return ConfigGenerator.generateLaunchArgs(getApplication(), settings)
	}

	/**
	 * Enforce hardware constraints between interdependent settings.
	 */
	private fun applyConstraints(s: EmulatorSettings): EmulatorSettings {
		var result = s

		// 68000/68010 must use 24-bit addressing
		if (result.cpuModel <= 68010) {
			result = result.copy(address24Bit = true, z3Ram = 0, jitCacheSize = 0)
		}

		// JIT requires 68020+
		if (result.cpuModel < 68020 && result.jitCacheSize > 0) {
			result = result.copy(jitCacheSize = 0)
		}

		// Z3 RAM requires 32-bit addressing
		if (result.address24Bit && result.z3Ram > 0) {
			result = result.copy(z3Ram = 0)
		}

		// FPU: only internal for 68040/68060
		if (result.fpuModel == 68040 && result.cpuModel < 68040) {
			result = result.copy(fpuModel = 0)
		}

		// On-screen joystick requires a touchscreen
		if (result.joyport1 == "onscreen_joy" && !hasTouchScreen(getApplication())) {
			result = result.copy(joyport1 = "joy0", onScreenJoystick = false)
		}

		return result
	}
}
