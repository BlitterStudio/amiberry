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
	/** The compatibility level of the preset the settings were made from (0 the best compatibility). */
	val baseLevel: Int = 1,

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
	val analogMouseSpeed: Int = EmulatorSettingsConstraints.DEFAULT_ANALOG_MOUSE_SPEED,

	/**
	 * The schema options without a field of their own (see [FIELD_IDS]):
	 * option ID to .uae value, as a preset sets them or a configuration file
	 * had them. An option missing here has the emulator's default.
	 */
	val options: Map<String, String> = emptyMap()
) {
	/** The on-screen keyboard is reachable by touch or by a gamepad toggle (vkbd_enabled). */
	val onScreenKeyboardAvailable: Boolean
		get() = onScreenKeyboardAvailable(onScreenKeyboardButton, onScreenKeyboardToggle)

	/** Cycle-exact in either mode (DMA/memory or full). */
	val isCycleExact: Boolean
		get() = cycleExact != "false"

	/**
	 * The settings as the schema's state tables key them: .uae values by
	 * option ID (every field's, the [options], and the emulator's default
	 * for an option neither sets), with the JIT as off or on (the tables'
	 * one cache size stands for every enabled size).
	 */
	fun schemaState(schema: SettingsSchema? = SettingsSchema.current): Map<String, String> {
		val state = HashMap<String, String>(schema?.defaults.orEmpty())
		state.putAll(options)
		for ((id, field) in FIELDS) state[id] = field.get(this)
		state["cachesize"] = jitState(jitCacheSize)
		return state
	}

	/**
	 * The .uae value of the schema option [id]: its field's, or the one in
	 * [options]; null when [options] does not set it (the emulator's default
	 * applies).
	 */
	fun optionValue(id: String): String? = FIELDS[id]?.get?.invoke(this) ?: options[id]

	/**
	 * [id] set to [value] (.uae spelling) with the changes the emulator's
	 * setting makes with it, as the schema's transition table recorded them
	 * from the settings layer's setters (a 68000 drops the JIT, cycle-exact
	 * turns immediate blits off). A value the layer refuses in this state
	 * turns immediate blits off); the fast RAM boards' tables make the MB
	 * and KB keys of a board agree. A value the layer refuses in this state
	 */
	fun withOption(id: String, value: String, schema: SettingsSchema? = SettingsSchema.current): EmulatorSettings {
		val stateValue = if (id == "cachesize") jitState(value.toIntOrNull() ?: return this) else value
		val state = schemaState(schema)
		val transition = schema?.transitions?.of(state, id, stateValue)
			?: schema?.memoryTransitions?.firstNotNullOfOrNull { it.of(state, id, stateValue) }
		if (transition?.refused == true) return this
		var result = withField(id, value, schema) ?: return this
		transition?.writes?.forEach { (field, written) ->
			// An enabled JIT keeps the cache size chosen.
			if (field == "cachesize" && written != "0" && result.jitCacheSize > 0) return@forEach
			result = result.withField(field, written, schema) ?: result
		}
		return result
	}

	/**
	 * [id] stored as [value]: in its field, or in [options] for an option
	 * without one; a stored default leaves the option unset, so the file
	 * writes what the emulator saves (a fast RAM board's one size under one
	 * key, not two keys that disagree).
	 */
	private fun withField(id: String, value: String, schema: SettingsSchema? = SettingsSchema.current): EmulatorSettings? {
		val field = FIELDS[id]
		if (field == null) {
			val stored = value.takeIf { it != schema?.option(id)?.defaultOnAndroid }
			return copy(options = stored?.let { options + (id to it) } ?: options - id)
		}
		return field.set?.invoke(this, value)
	}

	/**
	 * [model]'s preset configuration [config] at the model's default level,
	 * keeping these settings' Android controls and shader, and their
	 * [options] of the input and on-screen groups the user set: a model
	 * change replaces the machine, not how the user plays. An option the new
	 * preset sets, or one still at the value the current preset gave it (a
	 * CD32's pad mode), goes with the machine.
	 */
	fun withModel(model: AmigaModel, config: Int, schema: SettingsSchema? = SettingsSchema.current): EmulatorSettings {
		val machine = fromModel(model, config)
		val preset = baseModel.levelValues(baseConfig, baseLevel)
		val played = options.filter { (id, value) ->
			id !in machine.options && preset[id] != value && schema?.option(id)?.group in PLAY_GROUPS
		}
		return machine.copy(
			shader = shader,
			joyport0 = joyport0,
			joyport1 = joyport1,
			onScreenJoystick = onScreenJoystick,
			onScreenKeyboardButton = onScreenKeyboardButton,
			onScreenKeyboardNumpad = onScreenKeyboardNumpad,
			onScreenKeyboardToggle = onScreenKeyboardToggle,
			joyport0MouseMap = joyport0MouseMap,
			joyport1MouseMap = joyport1MouseMap,
			analogMouseSpeed = analogMouseSpeed,
			options = machine.options + played
		)
	}

	/**
	 * The compatibility level of [baseModel]'s configuration [baseConfig]
	 * these settings are at: the level whose values they have for the
	 * options the levels set differently ([baseLevel] where several levels
	 * build the same machine, as a 68040's best two do); null once one of
	 * those was edited.
	 */
	val compatibilityLevel: Int?
		get() {
			val levels = baseModel.configs.getOrNull(baseConfig)?.levels ?: return null
			val keys = levelKeys(levels)
			fun matches(level: Int) =
				keys.all { id -> (optionValue(id) ?: defaultOf(id)) == (levels[level][id] ?: defaultOf(id)) }
			return baseLevel.takeIf { it in levels.indices && matches(it) } ?: levels.indices.firstOrNull(::matches)
		}

	/**
	 * The settings at compatibility [level] of [baseModel]'s configuration:
	 * the options the levels set differently take that level's values (the
	 * default where it sets none); the rest stay as they are.
	 */
	fun withCompatibilityLevel(level: Int): EmulatorSettings {
		val levels = baseModel.configs.getOrNull(baseConfig)?.levels ?: return this
		val target = levels.getOrNull(level) ?: return this
		var result = copy(baseLevel = level)
		for (id in levelKeys(levels)) {
			val value = target[id]
			result = when {
				id in FIELDS -> result.withField(id, value ?: defaultOf(id) ?: continue) ?: result
				value == null -> result.copy(options = result.options - id)
				else -> result.copy(options = result.options + (id to value))
			}
		}
		return result
	}

	companion object {
		/** The JIT as the schema's tables enumerate it: "0" off, their one cache size on. */
		internal fun jitState(cacheSize: Int): String = if (cacheSize > 0) JIT_ON else "0"

		private const val JIT_ON = "8192"

		/** The settings groups that describe how the user plays rather than the machine (kept by [withModel]). */
		private val PLAY_GROUPS = setOf("input", "onscreen")

		fun onScreenKeyboardAvailable(button: Boolean, toggle: String?): Boolean =
			button || toggle != ""

		/** The Android emulator's default for an integer option (the schema's), or [fallback] without a schema. */
		internal fun schemaInt(id: String, fallback: Int): Int =
			SettingsSchema.current?.options?.get(id)?.defaultOnAndroid?.toIntOrNull() ?: fallback

		/**
		 * The schema options with a field of their own, which ConfigGenerator
		 * writes itself; every other schema option is kept in [options].
		 */
		val FIELD_IDS: Set<String> get() = FIELDS.keys

		/** The Android emulator's default of the option [id]; null when the schema does not describe it. */
		private fun defaultOf(id: String): String? = SettingsSchema.current?.option(id)?.defaultOnAndroid

		/** The machine options a configuration's [levels] set differently, which pick its level. */
		private fun levelKeys(levels: List<Map<String, String>>): Set<String> {
			val ids = levels.flatMapTo(LinkedHashSet()) { it.keys }
			return ids.filterTo(LinkedHashSet()) { id ->
				FIELDS[id]?.machine != false && levels.map { it[id] ?: defaultOf(id) }.distinct().size > 1
			}
		}

		/**
		 * The machine one of a model's preset configurations builds at
		 * compatibility [level] (the emulator's Quick Start configuration, read
		 * from the bundled settings schema; by default the model's default
		 * level). A level lists what differs from a fresh configuration, so
		 * every machine field it leaves out takes the emulator's default and
		 * the options it sets without a field fill [options]. The launcher's
		 * own controls, shader and media keep the launcher's defaults.
		 */
		fun fromModel(model: AmigaModel, config: Int = 0, level: Int = model.defaultLevel): EmulatorSettings {
			val index = model.configIndex(config)
			val chosen = model.levelIndex(level)
			val values = model.levelValues(index, chosen)
			var settings = EmulatorSettings(baseModel = model, baseConfig = index, baseLevel = chosen)
			for ((id, field) in FIELDS) {
				if (!field.machine) continue
				val value = values[id] ?: defaultOf(id) ?: continue
				settings = field.set?.invoke(settings, value) ?: settings
			}
			return settings.copy(options = values.filterKeys { it !in FIELDS })
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
		val soundChannelOptions: List<Pair<String, String>> get() = choices("sound_channels")
		val floppyTypeOptions: List<Pair<Int, String>> get() = intChoices("floppy0type")
		/** The pointer speeds the emulator offers for the left stick as mouse. */
		val analogMouseSpeeds: List<Int> get() = intChoices("input.joymouse_speed_analog").map { it.first }
	}
}

/**
 * A field of [EmulatorSettings] holding one schema option: its .uae value
 * ([get]) and the settings with a value stored ([set]; null for a value it
 * cannot hold, and no setter for a field derived from others). [machine]:
 * part of the machine a preset describes; false for the launcher's own
 * controls, shader and media (ROM files, disks, the CD image).
 */
private class Field(
	val get: (EmulatorSettings) -> String,
	val set: ((EmulatorSettings, String) -> EmulatorSettings?)?,
	val machine: Boolean = true
)

private fun textField(
	get: (EmulatorSettings) -> String,
	machine: Boolean = true,
	set: (EmulatorSettings, String) -> EmulatorSettings
) = Field(get, set, machine)

private fun intField(
	get: (EmulatorSettings) -> Int,
	machine: Boolean = true,
	set: (EmulatorSettings, Int) -> EmulatorSettings
) = Field({ get(it).toString() }, { s, v -> v.toIntOrNull()?.let { set(s, it) } }, machine)

private fun boolField(
	get: (EmulatorSettings) -> Boolean,
	machine: Boolean = true,
	set: (EmulatorSettings, Boolean) -> EmulatorSettings
) = Field({ get(it).toString() }, { s, v -> set(s, v == "true") }, machine)

/** An on/off field the option stores as 1 and 0. */
private fun switchField(
	get: (EmulatorSettings) -> Boolean,
	machine: Boolean = true,
	set: (EmulatorSettings, Boolean) -> EmulatorSettings
) = Field({ if (get(it)) "1" else "0" }, { s, v -> v.toIntOrNull()?.let { set(s, it > 0) } }, machine)

/** The schema options [EmulatorSettings] has a field for, by option ID. */
private val FIELDS: Map<String, Field> = linkedMapOf(
	"cpu_model" to intField({ it.cpuModel }) { s, v -> s.copy(cpuModel = v) },
	"cpu_compatible" to boolField({ it.cpuCompatible }) { s, v -> s.copy(cpuCompatible = v) },
	"cpu_24bit_addressing" to boolField({ it.address24Bit }) { s, v -> s.copy(address24Bit = v) },
	"cpu_speed" to textField({ it.cpuSpeed }) { s, v -> s.copy(cpuSpeed = v) },
	"fpu_model" to intField({ it.fpuModel }) { s, v -> s.copy(fpuModel = v) },
	"cachesize" to intField({ it.jitCacheSize }) { s, v -> s.copy(jitCacheSize = v) },
	"compfpu" to boolField({ it.jitFpu }) { s, v -> s.copy(jitFpu = v) },
	"chipset" to textField({ it.chipset }) { s, v -> s.copy(chipset = v) },
	"immediate_blits" to boolField({ it.immediateBlits }) { s, v -> s.copy(immediateBlits = v) },
	"collision_level" to textField({ it.collisionLevel }) { s, v -> s.copy(collisionLevel = v) },
	"cycle_exact" to textField({ it.cycleExact }) { s, v -> s.copy(cycleExact = v) },
	"ntsc" to boolField({ it.ntsc }) { s, v -> s.copy(ntsc = v) },
	"chipmem_size" to intField({ it.chipRam }) { s, v -> s.copy(chipRam = v) },
	"bogomem_size" to intField({ it.slowRam }) { s, v -> s.copy(slowRam = v) },
	"fastmem_size" to intField({ it.fastRam }) { s, v -> s.copy(fastRam = v) },
	"z3mem_size" to intField({ it.z3Ram }) { s, v -> s.copy(z3Ram = v) },
	"kickstart_rom_file" to textField({ it.romFile }, machine = false) { s, v -> s.copy(romFile = v) },
	"kickstart_ext_rom_file" to textField({ it.romExtFile }, machine = false) { s, v -> s.copy(romExtFile = v) },
	"floppy0" to textField({ it.floppy0 }, machine = false) { s, v -> s.copy(floppy0 = v) },
	"floppy0type" to intField({ it.floppy0Type }) { s, v -> s.copy(floppy0Type = v) },
	"floppy1" to textField({ it.floppy1 }, machine = false) { s, v -> s.copy(floppy1 = v) },
	"floppy1type" to intField({ it.floppy1Type }) { s, v -> s.copy(floppy1Type = v) },
	"floppy2" to textField({ it.floppy2 }, machine = false) { s, v -> s.copy(floppy2 = v) },
	"floppy2type" to intField({ it.floppy2Type }) { s, v -> s.copy(floppy2Type = v) },
	"floppy3" to textField({ it.floppy3 }, machine = false) { s, v -> s.copy(floppy3 = v) },
	"floppy3type" to intField({ it.floppy3Type }) { s, v -> s.copy(floppy3Type = v) },
	"floppy0sound" to intField({ it.floppy0Sound }) { s, v -> s.copy(floppy0Sound = v) },
	"floppy1sound" to intField({ it.floppy1Sound }) { s, v -> s.copy(floppy1Sound = v) },
	"floppy2sound" to intField({ it.floppy2Sound }) { s, v -> s.copy(floppy2Sound = v) },
	"floppy3sound" to intField({ it.floppy3Sound }) { s, v -> s.copy(floppy3Sound = v) },
	"floppy0soundvolume_disk" to intField({ it.floppy0SoundVolumeDisk }) { s, v -> s.copy(floppy0SoundVolumeDisk = v) },
	"floppy1soundvolume_disk" to intField({ it.floppy1SoundVolumeDisk }) { s, v -> s.copy(floppy1SoundVolumeDisk = v) },
	"floppy2soundvolume_disk" to intField({ it.floppy2SoundVolumeDisk }) { s, v -> s.copy(floppy2SoundVolumeDisk = v) },
	"floppy3soundvolume_disk" to intField({ it.floppy3SoundVolumeDisk }) { s, v -> s.copy(floppy3SoundVolumeDisk = v) },
	"floppy0soundvolume_empty" to intField({ it.floppy0SoundVolumeEmpty }) { s, v -> s.copy(floppy0SoundVolumeEmpty = v) },
	"floppy1soundvolume_empty" to intField({ it.floppy1SoundVolumeEmpty }) { s, v -> s.copy(floppy1SoundVolumeEmpty = v) },
	"floppy2soundvolume_empty" to intField({ it.floppy2SoundVolumeEmpty }) { s, v -> s.copy(floppy2SoundVolumeEmpty = v) },
	"floppy3soundvolume_empty" to intField({ it.floppy3SoundVolumeEmpty }) { s, v -> s.copy(floppy3SoundVolumeEmpty = v) },
	// The CD image is media the launcher supplies (a preset's "empty" drive
	// without a disc included): "" writes no CD drive.
	"cdimage0" to textField({ it.cdImage }, machine = false) { s, v -> s.copy(cdImage = v) },
	"sound_output" to textField({ it.soundOutput }) { s, v -> s.copy(soundOutput = v) },
	"sound_frequency" to intField({ it.soundFreq }) { s, v -> s.copy(soundFreq = v) },
	"sound_max_buff" to intField({ it.soundBuffer }) { s, v -> s.copy(soundBuffer = v) },
	"sound_channels" to textField({ it.soundChannels }) { s, v -> s.copy(soundChannels = v) },
	"sound_volume" to intField({ it.soundVolume }) { s, v -> s.copy(soundVolume = v) },
	"sound_volume_paula" to intField({ it.soundVolumePaula }) { s, v -> s.copy(soundVolumePaula = v) },
	"sound_volume_cd" to intField({ it.soundVolumeCd }) { s, v -> s.copy(soundVolumeCd = v) },
	"sound_volume_ahi" to intField({ it.soundVolumeAhi }) { s, v -> s.copy(soundVolumeAhi = v) },
	"sound_volume_midi" to intField({ it.soundVolumeMidi }) { s, v -> s.copy(soundVolumeMidi = v) },
	"amiberry.gfx_correct_aspect" to switchField({ it.correctAspect }) { s, v -> s.copy(correctAspect = v) },
	"amiberry.gfx_auto_crop" to boolField({ it.autoCrop }) { s, v -> s.copy(autoCrop = v) },
	"amiberry.scaling_method" to intField({ it.scalingMethod }) { s, v -> s.copy(scalingMethod = v) },
	"gfx_autoresolution" to intField({ it.gfxAutoresolution }) { s, v -> s.copy(gfxAutoresolution = v) },
	"amiberry.shader" to textField({ it.shader }, machine = false) { s, v -> s.copy(shader = v) },
	"joyport0" to textField({ it.joyport0 }, machine = false) { s, v -> s.copy(joyport0 = v) },
	"joyport1" to textField({ it.joyport1 }, machine = false) { s, v -> s.copy(joyport1 = v) },
	"joyport0mousemap" to switchField({ it.joyport0MouseMap }, machine = false) { s, v -> s.copy(joyport0MouseMap = v) },
	"joyport1mousemap" to switchField({ it.joyport1MouseMap }, machine = false) { s, v -> s.copy(joyport1MouseMap = v) },
	"input.joymouse_speed_analog" to intField({ it.analogMouseSpeed }, machine = false) { s, v ->
		s.copy(analogMouseSpeed = v)
	},
	"amiberry.onscreen_joystick" to boolField({ it.onScreenJoystick }, machine = false) { s, v ->
		s.copy(onScreenJoystick = v)
	},
	// Derived from the touch button and the gamepad toggle.
	"amiberry.vkbd_enabled" to Field({ it.onScreenKeyboardAvailable.toString() }, null, machine = false),
	"amiberry.vkbd_touch_button" to boolField({ it.onScreenKeyboardButton }, machine = false) { s, v ->
		s.copy(onScreenKeyboardButton = v)
	},
	"amiberry.vkbd_numpad" to boolField({ it.onScreenKeyboardNumpad }, machine = false) { s, v ->
		s.copy(onScreenKeyboardNumpad = v)
	},
	// "default" is the written spelling of no toggle chosen (null).
	"amiberry.vkbd_toggle" to textField({ it.onScreenKeyboardToggle ?: "default" }, machine = false) { s, v ->
		s.copy(onScreenKeyboardToggle = v.takeIf { it != "default" })
	},
	// The launcher always skips the emulator's GUI.
	"use_gui" to Field({ "no" }, null, machine = false)
)
