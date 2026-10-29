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
	/** cycle_exact as stored: "false", "memory" (DMA/memory accesses only) or "true" (full). */
	val cycleExact: String = "false",
	val ntsc: Boolean = false,

	// Memory (in config file units)
	val chipRam: Int = 1,  // chipmem_size: -1=128KB, 0=256KB, else half-megabytes (1=512KB, 4=2MB); see chipRamKb
	val slowRam: Int = 2,  // 256KB units: 0=none, 2=512KB, 4=1MB
	val fastRam: Int = 0,  // megabytes
	val z3Ram: Int = 0,  // megabytes

	// ROM
	val romFile: String = "",
	val romExtFile: String = "",

	// Floppy drives. The types are the emulator's drive types (the schema's
	// floppyNtype choices): -1 disabled, 0 3.5" DD, 1 3.5" HD, 2 5.25" (40),
	// 7 5.25" (80), 3 3.5" ESCOM, 8 FloppyBridge.
	val floppy0: String = "",
	val floppy0Type: Int = 0,
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
	// Volumes as the configuration stores them: attenuation, 0 is full volume
	// and 100 silent (the schema marks them reversed; a front end shows 100 - value).
	val soundVolume: Int = schemaInt("sound_volume", 0),
	val soundVolumePaula: Int = schemaInt("sound_volume_paula", 0),
	val soundVolumeCd: Int = schemaInt("sound_volume_cd", 20),
	val soundVolumeAhi: Int = schemaInt("sound_volume_ahi", 0),
	val soundVolumeMidi: Int = schemaInt("sound_volume_midi", 0),
	// Drive sounds (floppyNsound: 0 off, 1 the built-in A500 drive, -1 external
	// samples) and their click volumes, attenuation as above.
	val floppy0Sound: Int = schemaInt("floppy0sound", 0),
	val floppy1Sound: Int = schemaInt("floppy1sound", 0),
	val floppy2Sound: Int = schemaInt("floppy2sound", 0),
	val floppy3Sound: Int = schemaInt("floppy3sound", 0),
	val floppy0SoundVolumeDisk: Int = schemaInt("floppy0soundvolume_disk", 33),
	val floppy1SoundVolumeDisk: Int = schemaInt("floppy1soundvolume_disk", 33),
	val floppy2SoundVolumeDisk: Int = schemaInt("floppy2soundvolume_disk", 33),
	val floppy3SoundVolumeDisk: Int = schemaInt("floppy3soundvolume_disk", 33),
	val floppy0SoundVolumeEmpty: Int = schemaInt("floppy0soundvolume_empty", 33),
	val floppy1SoundVolumeEmpty: Int = schemaInt("floppy1soundvolume_empty", 33),
	val floppy2SoundVolumeEmpty: Int = schemaInt("floppy2soundvolume_empty", 33),
	val floppy3SoundVolumeEmpty: Int = schemaInt("floppy3soundvolume_empty", 33),

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

	/** Cycle-exact in either mode (DMA/memory or full). */
	val isCycleExact: Boolean
		get() = cycleExact != "false"

	/**
	 * Another CPU, with the changes the emulator's CPU setting makes with it
	 * (set_cpu_model in src/osdep/settings/settings_cpu.cpp): a 68000/010 is
	 * 24-bit and has no JIT, and keeps an external FPU only outside More
	 * compatible and cycle-exact modes; a 68030 starts 32-bit; a 68040/060
	 * is 32-bit and uses its internal FPU; below a 68040 an internal FPU
	 * becomes a 68881.
	 */
	fun withCpuModel(cpu: Int): EmulatorSettings {
		if (cpu == cpuModel) return this
		val externalFpu = if (fpuModel != 0 && fpuModel != 68882) 68881 else fpuModel
		return when {
			cpu <= 68010 -> copy(
				cpuModel = cpu,
				address24Bit = true,
				jitCacheSize = 0,
				fpuModel = if (cpuCompatible || isCycleExact) 0 else externalFpu
			)
			cpu <= 68030 -> copy(
				cpuModel = cpu,
				address24Bit = if (cpu == 68030) false else address24Bit,
				fpuModel = externalFpu
			)
			else -> copy(
				cpuModel = cpu,
				address24Bit = false,
				fpuModel = if (fpuModel != 0) cpu else 0
			)
		}
	}

	/**
	 * Another cycle-exact mode, with the changes the emulator's setting makes
	 * with it (set_cycle_exact in src/osdep/settings/settings_chipset.cpp): a
	 * 68000/010 has no DMA-only mode, so it runs fully cycle-exact; up to a
	 * 68030 the CPU runs at real speed in More compatible mode; immediate
	 * blits and the JIT are turned off.
	 */
	fun withCycleExact(value: String): EmulatorSettings {
		if (value == "false") return copy(cycleExact = value)
		val slowCpu = cpuModel <= 68030
		return copy(
			cycleExact = if (cpuModel < 68020) "true" else value,
			cpuSpeed = if (slowCpu) "real" else cpuSpeed,
			cpuCompatible = cpuCompatible || slowCpu,
			immediateBlits = false,
			jitCacheSize = 0
		)
	}

	/**
	 * Another JIT cache size; turning the JIT on leaves cycle-exact mode and
	 * turns the JIT FPU on, as the emulator's setting does (set_cachesize in
	 * src/osdep/settings/settings_cpu.cpp; the launcher has no softfloat FPU).
	 */
	fun withJitCacheSize(size: Int): EmulatorSettings {
		val enabling = jitCacheSize == 0 && size > 0
		return if (enabling) {
			copy(jitCacheSize = size, cycleExact = "false", jitFpu = true)
		} else {
			copy(jitCacheSize = size)
		}
	}

	companion object {
		fun onScreenKeyboardAvailable(button: Boolean, toggle: String?): Boolean =
			button || toggle != ""

		/** The schema's default for an integer option, or [fallback] without one. */
		internal fun schemaInt(id: String, fallback: Int): Int =
			SettingsSchema.current?.options?.get(id)?.default?.toIntOrNull() ?: fallback

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
				cycleExact = v["cycle_exact"] ?: defaults.cycleExact,
				ntsc = flag("ntsc", defaults.ntsc),
				chipRam = int("chipmem_size", defaults.chipRam),
				slowRam = int("bogomem_size", defaults.slowRam),
				fastRam = int("fastmem_size", defaults.fastRam),
				z3Ram = int("z3mem_size", defaults.z3Ram),
				floppy0Type = int("floppy0type", defaults.floppy0Type),
				floppy1Type = int("floppy1type", defaults.floppy1Type),
				floppy2Type = int("floppy2type", defaults.floppy2Type),
				floppy3Type = int("floppy3type", defaults.floppy3Type)
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
		val cycleExactOptions: List<Pair<String, String>> get() = choices("cycle_exact")
		val floppyTypeOptions: List<Pair<Int, String>> get() = intChoices("floppy0type")
		/** The pointer speeds the emulator offers for the left stick as mouse. */
		val analogMouseSpeeds: List<Int> get() = intChoices("input.joymouse_speed_analog").map { it.first }
	}
}
