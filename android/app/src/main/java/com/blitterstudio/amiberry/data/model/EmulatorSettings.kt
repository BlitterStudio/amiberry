package com.blitterstudio.amiberry.data.model

/**
 * All emulator settings that the native Android UI can configure.
 * Maps to .uae config file keys used by cfgfile.cpp.
 */
data class EmulatorSettings(
	// Model base
	val baseModel: AmigaModel = AmigaModel.A500,
	/** The preset configuration of baseModel the emulator starts from (see ModelRomAvailability). */
	val baseConfig: Int = 0,

	// CPU
	val cpuModel: Int = 68000,
	val cpuCompatible: Boolean = true,
	val address24Bit: Boolean = true,
	val cpuSpeed: String = "real",  // "max" or "real"
	val fpuModel: Int = 0,  // 0=none, 68881, 68882
	val jitCacheSize: Int = 0,  // 0=disabled, else KB
	val jitFpu: Boolean = false,

	// Chipset
	val chipset: String = "ocs",  // ocs, ecs_agnus, ecs_denise, ecs, aga
	val immediateBlits: Boolean = false,
	val collisionLevel: String = "full",  // none, sprites, playfields, full
	val cycleExact: Boolean = false,
	val ntsc: Boolean = false,

	// Memory (in config file units)
	val chipRam: Int = 1,  // chipmem_size: -1=128KB, 0=256KB, else half-megabytes (1=512KB, 4=2MB); see chipRamKb
	val slowRam: Int = 2,  // 256KB units: 0=none, 2=512KB, 4=1MB
	val fastRam: Int = 0,  // megabytes
	val z3Ram: Int = 0,  // megabytes

	// ROM
	val romFile: String = "",
	val romExtFile: String = "",

	// Floppy drives
	val floppy0: String = "",
	val floppy0Type: Int = 0,  // 0=3.5"DD, 1=3.5"HD, -1=disabled
	val floppy1: String = "",
	val floppy1Type: Int = -1,
	val floppy2: String = "",
	val floppy2Type: Int = -1,
	val floppy3: String = "",
	val floppy3Type: Int = -1,

	// CD
	val cdImage: String = "",

	// Hard drives (hardfiles)
	val hardDrives: List<HardDrive> = emptyList(),

	// Sound
	val soundOutput: String = "exact",  // none, interrupts, normal, exact
	val soundFreq: Int = EmulatorSettingsConstraints.DEFAULT_SOUND_FREQUENCY,
	val soundBuffer: Int = EmulatorSettingsConstraints.DEFAULT_SOUND_BUFFER,
	val soundChannels: String = "stereo",

	// Display
	val gfxWidth: Int = 720,
	val gfxHeight: Int = 568,
	val correctAspect: Boolean = true,
	val autoCrop: Boolean = false,
	val scalingMethod: Int = -1,        // -1 Auto, 0 Nearest, 1 Linear, 2 Integer
	val gfxAutoresolution: Int = 0,     // 0 Disabled, 1 Always On, 10/33/66 = % (raw native value)
	val shader: String = "none",

	// Input
	val joyport0: String = "mouse",
	val joyport1: String = "onscreen_joy",
	val onScreenJoystick: Boolean = true,
	/** KB touch button on the on-screen joystick overlay (vkbd_touch_button).
	 * Independent of the gamepad toggle: a gamepad-only device can hide it and still type. */
	val onScreenKeyboardButton: Boolean = true,
	val onScreenKeyboardNumpad: Boolean = false,
	/** SDL gamepad button name that toggles the on-screen keyboard (vkbd_toggle).
	 * null = key absent, emulator default applies; "" = explicitly disabled; else a button name. */
	val onScreenKeyboardToggle: String? = null,
	val joyport0MouseMap: Boolean = false,  // left analog stick + shoulders act as the Amiga mouse
	val joyport1MouseMap: Boolean = false,
	/** Pointer speed when the left stick drives the mouse (input.joymouse_speed_analog). */
	val analogMouseSpeed: Int = EmulatorSettingsConstraints.DEFAULT_ANALOG_MOUSE_SPEED
) {
	/** The on-screen keyboard is reachable by touch or by a gamepad toggle (vkbd_enabled). */
	val onScreenKeyboardAvailable: Boolean
		get() = onScreenKeyboardAvailable(onScreenKeyboardButton, onScreenKeyboardToggle)

	companion object {
		fun onScreenKeyboardAvailable(button: Boolean, toggle: String?): Boolean =
			button || toggle != ""

		/**
		 * The settings one of a model's preset configurations sets (the
		 * emulator's Quick Start configuration, read from the bundled settings
		 * schema); the rest keep the launcher's defaults.
		 */
		fun fromModel(model: AmigaModel, config: Int = 0): EmulatorSettings {
			val index = config.coerceIn(0, model.configs.size - 1)
			val v = model.configs[index].values
			val defaults = EmulatorSettings(baseModel = model, baseConfig = index)
			fun int(id: String, fallback: Int) = v[id]?.toIntOrNull() ?: fallback
			fun flag(id: String, fallback: Boolean) = v[id]?.let { it == "true" } ?: fallback
			return defaults.copy(
				cpuModel = int("cpu_model", defaults.cpuModel),
				cpuCompatible = flag("cpu_compatible", defaults.cpuCompatible),
				address24Bit = flag("cpu_24bit_addressing", defaults.address24Bit),
				cpuSpeed = v["cpu_speed"]?.takeIf { it == "real" || it == "max" } ?: defaults.cpuSpeed,
				fpuModel = int("fpu_model", defaults.fpuModel),
				jitCacheSize = int("cachesize", defaults.jitCacheSize),
				jitFpu = flag("compfpu", defaults.jitFpu),
				chipset = v["chipset"] ?: defaults.chipset,
				cycleExact = v["cycle_exact"]?.let { it != "false" } ?: defaults.cycleExact,
				ntsc = flag("ntsc", defaults.ntsc),
				chipRam = int("chipmem_size", defaults.chipRam),
				slowRam = int("bogomem_size", defaults.slowRam),
				fastRam = int("fastmem_size", defaults.fastRam),
				z3Ram = int("z3mem_size", defaults.z3Ram),
				floppy0Type = int("floppy0type", defaults.floppy0Type),
				floppy1Type = int("floppy1type", defaults.floppy1Type)
			)
		}


		// The choices the emulator offers for these options (value, label),
		// from the bundled settings schema.
		private fun choices(id: String): List<Pair<String, String>> =
			SettingsSchema.current?.options?.get(id)?.choices?.map { it.value to it.label }.orEmpty()

		private fun intChoices(id: String): List<Pair<Int, String>> =
			choices(id).mapNotNull { (value, label) -> value.toIntOrNull()?.let { it to label } }

		val chipRamOptions: List<Pair<Int, String>> get() = intChoices("chipmem_size")

		/** The size of a chipmem_size value: -1 and 0 are 128 KB and 256 KB, the rest half megabytes. */
		fun chipRamKb(value: Int): Int = when {
			value < 0 -> 128
			value == 0 -> 256
			else -> value * 512
		}
		val slowRamOptions: List<Pair<Int, String>> get() = intChoices("bogomem_size")
		val fastRamOptions: List<Pair<Int, String>> get() = intChoices("fastmem_size")
		val z3RamOptions: List<Pair<Int, String>> get() = intChoices("z3mem_size")
		val cpuModels: List<Int> get() = intChoices("cpu_model").map { it.first }
		val chipsetOptions: List<Pair<String, String>> get() = choices("chipset")
		val soundOutputOptions: List<Pair<String, String>> get() = choices("sound_output")
		val soundChannelOptions: List<Pair<String, String>> get() = choices("sound_channels")
	}
}
