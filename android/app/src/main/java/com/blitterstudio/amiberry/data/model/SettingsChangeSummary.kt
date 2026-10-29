package com.blitterstudio.amiberry.data.model

import com.blitterstudio.amiberry.R

/**
 * One changed setting: what it is called and how it reads before and after.
 * [optionId] is the settings layer option the row shows, which a revert sets
 * back; null for the launcher's own rows (model, compatibility, resolution,
 * hard drives).
 */
data class SettingsChange(
	val label: SettingText,
	val before: SettingText,
	val after: SettingText,
	val optionId: String? = null
)

/**
 * The review-changes list: every setting that differs, named by the
 * settings layer's label (the launcher's own for launcher-only settings)
 * with values as the controls show them — choice labels, on/off, numbers in
 * their unit, file names — never the stored tokens.
 */
object SettingsChangeSummary {

	fun diff(
		before: EmulatorSettings,
		after: EmulatorSettings,
		schema: SettingsSchema? = SettingsSchema.current
	): List<SettingsChange> = Diff(schema, before, after).apply {
		launcher(R.string.settings_cpu_model_preset) { SettingText.Plain(it.baseModel.displayName) }
		launcher(R.string.settings_change_model_configuration, ::presetConfiguration)
		launcher(R.string.settings_compatibility_level) { settings ->
			SettingText.Plain(schema?.compatibilityLevels?.getOrNull(settings.baseLevel) ?: settings.baseLevel.toString())
		}

		option("cpu_model") { it.cpuModel.toString() }
		option("cpu_speed") { it.cpuSpeed }
		option("cpu_compatible") { it.cpuCompatible.toString() }
		option("cpu_24bit_addressing") { it.address24Bit.toString() }
		option("fpu_model") { it.fpuModel.toString() }
		option("cachesize") { it.jitCacheSize.toString() }
		option("compfpu") { it.jitFpu.toString() }

		option("chipset") { it.chipset }
		option("cycle_exact") { it.cycleExact }
		option("immediate_blits") { it.immediateBlits.toString() }
		option("ntsc") { it.ntsc.toString() }
		option("collision_level") { it.collisionLevel }

		option("chipmem_size") { it.chipRam.toString() }
		option("bogomem_size") { it.slowRam.toString() }
		option("fastmem_size") { it.fastRam.toString() }
		option("z3mem_size") { it.z3Ram.toString() }

		launcher(R.string.settings_display_resolution_label) {
			SettingText.Resource(R.string.settings_change_resolution_value, listOf(it.gfxWidth, it.gfxHeight))
		}
		option("amiberry.gfx_correct_aspect") { if (it.correctAspect) "1" else "0" }
		option("amiberry.gfx_auto_crop") { it.autoCrop.toString() }
		option("amiberry.scaling_method") { it.scalingMethod.toString() }
		option("gfx_autoresolution") { it.gfxAutoresolution.toString() }
		optionText("amiberry.shader") { LauncherChoices.shader(it.shader) }

		option("sound_output") { it.soundOutput }
		option("sound_channels") { it.soundChannels }
		option("sound_frequency") { it.soundFreq.toString() }
		option("sound_max_buff") { it.soundBuffer.toString() }
		option("sound_volume") { it.soundVolume.toString() }
		option("sound_volume_paula") { it.soundVolumePaula.toString() }
		option("sound_volume_cd") { it.soundVolumeCd.toString() }
		option("sound_volume_ahi") { it.soundVolumeAhi.toString() }
		option("sound_volume_midi") { it.soundVolumeMidi.toString() }
		drives.forEach { drive ->
			option("floppy${drive.index}sound") { drive.sound(it).toString() }
			option("floppy${drive.index}soundvolume_disk") { drive.volumeDisk(it).toString() }
			option("floppy${drive.index}soundvolume_empty") { drive.volumeEmpty(it).toString() }
		}

		optionText("joyport0") { LauncherChoices.portDevice(it.joyport0) }
		optionText("joyport1") { LauncherChoices.portDevice(it.joyport1) }
		option("joyport0mousemap") { if (it.joyport0MouseMap) "1" else "0" }
		option("joyport1mousemap") { if (it.joyport1MouseMap) "1" else "0" }
		option("input.joymouse_speed_analog") { it.analogMouseSpeed.toString() }
		option("amiberry.onscreen_joystick") { it.onScreenJoystick.toString() }
		option("amiberry.vkbd_touch_button") { it.onScreenKeyboardButton.toString() }
		option("amiberry.vkbd_numpad") { it.onScreenKeyboardNumpad.toString() }
		optionText("amiberry.vkbd_toggle") { LauncherChoices.oskToggle(it.onScreenKeyboardToggle) }

		optionText("kickstart_rom_file") { romFile(it.romFile) }
		optionText("kickstart_ext_rom_file") { romFile(it.romExtFile) }
		drives.forEach { drive ->
			option("floppy${drive.index}type") { drive.type(it).toString() }
			optionText("floppy${drive.index}") { file(drive.path(it)) }
		}
		optionText("cdimage0") { file(it.cdImage) }
		hardDrives()
		options()
	}.changes

	private class Diff(
		private val schema: SettingsSchema?,
		private val before: EmulatorSettings,
		private val after: EmulatorSettings
	) {
		val changes = mutableListOf<SettingsChange>()

		private fun add(label: SettingText, old: SettingText, new: SettingText, optionId: String? = null) {
			if (old != new) changes += SettingsChange(label, old, new, optionId)
		}

		/** A launcher-only setting, named by a launcher string. */
		fun launcher(labelRes: Int, text: (EmulatorSettings) -> SettingText) {
			add(SettingText.Resource(labelRes), text(before), text(after))
		}

		private fun label(id: String): SettingText = SettingText.Plain(schema?.label(id) ?: id)

		/** A layer option whose values the launcher names ([text]): devices, files, shaders. */
		fun optionText(id: String, text: (EmulatorSettings) -> SettingText) {
			add(label(id), text(before), text(after), id)
		}

		/** A layer option compared by its stored value, shown as the controls show it. */
		fun option(id: String, stored: (EmulatorSettings) -> String) {
			val old = stored(before)
			val new = stored(after)
			if (old != new) changes += SettingsChange(label(id), value(id, old), value(id, new), id)
		}

		private fun value(id: String, stored: String): SettingText {
			val option = schema?.option(id) ?: return SettingText.Plain(stored)
			return when {
				option.type == SettingsSchema.TYPE_BOOL && (stored == "true" || stored == "false") -> onOff(stored == "true")
				option.showsAsSwitch(stored) -> onOff(stored == "1")
				else -> SettingText.Plain(option.valueText(stored, stored, stored))
			}
		}

		private fun onOff(on: Boolean) = SettingText.Resource(if (on) R.string.settings_value_on else R.string.settings_value_off)

		/** Hard drives by position: added, removed, or a changed image, read-only flag or boot priority. */
		fun hardDrives() {
			val count = maxOf(before.hardDrives.size, after.hardDrives.size)
			for (index in 0 until count) {
				val old = before.hardDrives.getOrNull(index)
				val new = after.hardDrives.getOrNull(index)
				val label = SettingText.Resource(R.string.settings_storage_hard_drive_label, listOf(index))
				if (old == null || new == null) {
					add(label, old?.let { file(it.path) } ?: none, new?.let { file(it.path) } ?: none)
					continue
				}
				add(label, file(old.path), file(new.path))
				add(
					SettingText.Resource(R.string.settings_change_hard_drive_read_only, listOf(index)),
					onOff(old.readOnly),
					onOff(new.readOnly)
				)
				add(
					SettingText.Resource(R.string.settings_change_hard_drive_boot_priority, listOf(index)),
					SettingText.Plain(old.bootPriority.toString()),
					SettingText.Plain(new.bootPriority.toString())
				)
			}
		}

		/**
		 * The options without a field, in the layer's order (an option a
		 * side leaves out has the emulator's default there).
		 */
		fun options() {
			val ids = before.options.keys + after.options.keys
			val order = schema?.options?.keys.orEmpty()
			val ordered = order.filter { it in ids } + ids.filter { it !in order }.sorted()
			for (id in ordered) {
				val default = schema?.option(id)?.defaultOnAndroid.orEmpty()
				option(id) { it.options[id] ?: default }
			}
		}
	}
}

/** One floppy drive's fields. */
private class Drive(
	val index: Int,
	val path: (EmulatorSettings) -> String,
	val type: (EmulatorSettings) -> Int,
	val sound: (EmulatorSettings) -> Int,
	val volumeDisk: (EmulatorSettings) -> Int,
	val volumeEmpty: (EmulatorSettings) -> Int
)

private val drives = listOf(
	Drive(0, { it.floppy0 }, { it.floppy0Type }, { it.floppy0Sound }, { it.floppy0SoundVolumeDisk }, { it.floppy0SoundVolumeEmpty }),
	Drive(1, { it.floppy1 }, { it.floppy1Type }, { it.floppy1Sound }, { it.floppy1SoundVolumeDisk }, { it.floppy1SoundVolumeEmpty }),
	Drive(2, { it.floppy2 }, { it.floppy2Type }, { it.floppy2Sound }, { it.floppy2SoundVolumeDisk }, { it.floppy2SoundVolumeEmpty }),
	Drive(3, { it.floppy3 }, { it.floppy3Type }, { it.floppy3Sound }, { it.floppy3SoundVolumeDisk }, { it.floppy3SoundVolumeEmpty })
)

private val none = SettingText.Resource(R.string.settings_option_none)

/** A path by its file name, or None for no file. */
private fun file(path: String): SettingText =
	if (path.isBlank()) none else SettingText.Plain(LauncherChoices.fileName(path))

/** A ROM path by its file name; the bundled AROS ROM by name. */
private fun romFile(path: String): SettingText =
	if (ModelRomAvailability.isBuiltInArosRom(path)) SettingText.Resource(R.string.rom_aros_builtin) else file(path)

/** The model's preset configuration by its label. */
private fun presetConfiguration(settings: EmulatorSettings): SettingText {
	val config = settings.baseModel.configs.getOrNull(settings.baseConfig)
		?: return SettingText.Plain((settings.baseConfig + 1).toString())
	return SettingText.Plain(config.label)
}
