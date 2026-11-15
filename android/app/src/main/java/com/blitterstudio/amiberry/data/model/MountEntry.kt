package com.blitterstudio.amiberry.data.model

import java.io.File
import java.io.RandomAccessFile
import java.util.zip.GZIPInputStream
import java.util.zip.ZipInputStream

/**
 * A storage mount as cfgfile writes it. Directories and hardfiles have a
 * filesystem2/hardfile2 line plus a uaehfN twin; CD and tape units have only
 * their uaehfN line. Physical host drives are intentionally not represented.
 */
enum class MountKind(val token: String) {
	Directory("dir"),
	Hardfile("hdf"),
	Cd("cd"),
	Tape("tape");

	companion object {
		fun fromToken(token: String): MountKind? = entries.firstOrNull { it.token == token.lowercase() }
	}
}

data class MountEntry(
	val kind: MountKind,
	val path: String = "",
	val device: String = "",
	val volume: String = "",
	val readOnly: Boolean = false,
	val bootPriority: Int = 0,
	val sectors: Int = 32,
	val surfaces: Int = 1,
	val reserved: Int = 2,
	val blockSize: Int = 512,
	val filesystem: String = "",
	/** The cfgfile controller spelling, including the actual unit number. */
	val controller: String = "uae0",
	/** CD/tape device_emu_unit, encoded in their kind token. */
	val unit: Int = 0,
	val injectIcons: Boolean = false,
	val highCyl: Int = 0,
	val physicalGeometry: String = "",
	val geometry: String = "",
	val compactFlash: Boolean = false,
	val featureLevel: String = "",
	val flags: String = "",
	val lock: Boolean = false,
	val identity: Boolean = false,
	/** Future cfgfile tail tokens that the launcher does not otherwise name. */
	val extras: List<String> = emptyList()
) {
	val bootable: Boolean get() = bootPriority != BOOTPRI_NOAUTOBOOT && bootPriority != BOOTPRI_NOAUTOMOUNT

	companion object {
		const val MAX_MOUNTS = 50
		const val BOOTPRI_NOAUTOBOOT = -128
		const val BOOTPRI_NOAUTOMOUNT = -129
	}
}

/** Launcher-side defaults mirror add_filesys_config and hardfile_testrdb. */
object MountDefaults {
	/** A new drive's entry; callers check [canAdd] first (use [added]). */
	fun newMount(kind: MountKind, path: String, settings: EmulatorSettings, schema: SettingsSchema?): MountEntry {
		val (template, controllerUnit) = placement(kind, settings, schema) ?: (defaultController(kind, settings, schema) to 0)
		val controller = template
		val emuUnit = freeEmuUnit(kind, settings.hardDrives) ?: 0
		return when (kind) {
			MountKind.Directory -> MountEntry(
				kind = kind, path = path, device = firstFreeDevice(settings.hardDrives),
				volume = volumeName(path), controller = "uae0"
			)
			MountKind.Hardfile -> {
				val geometry = hdfGeometry(path)
				MountEntry(
					kind = kind, path = path, device = firstFreeDevice(settings.hardDrives),
					sectors = geometry.sectors, surfaces = geometry.surfaces, reserved = geometry.reserved,
					blockSize = geometry.blockSize, controller = controller.replace("%d", controllerUnit.toString())
				)
			}
			MountKind.Cd -> MountEntry(
				kind = kind, path = "", readOnly = true, sectors = 0, surfaces = 0, reserved = 0,
				blockSize = 2048, controller = controller.replace("%d", controllerUnit.toString()), unit = emuUnit
			)
			MountKind.Tape -> MountEntry(
				kind = kind, path = path, sectors = 0, surfaces = 0, reserved = 0,
				blockSize = 512, controller = controller.replace("%d", controllerUnit.toString()), unit = emuUnit
			)
		}
	}

	/**
	 * [mount] pointed at [path]. A hardfile is probed again, as the panel
	 * does when an image is picked: an RDB disk mounted with a partition's
	 * geometry reads as one partition. With no RDB found, an entry in RDB
	 * mode gets the plain geometry and a set geometry stays.
	 * The same rule as amiberry_settings_mount_field_set("path").
	 */
	fun withPath(mount: MountEntry, path: String): MountEntry {
		if (mount.kind != MountKind.Hardfile || path == mount.path) return mount.copy(path = path)
		val geometry = rdbGeometry(path) ?: if (mount.sectors == 0) plainGeometry else
			return mount.copy(path = path)
		return mount.copy(
			path = path, sectors = geometry.sectors, surfaces = geometry.surfaces,
			reserved = geometry.reserved, blockSize = geometry.blockSize
		)
	}

	private fun defaultController(kind: MountKind, settings: EmulatorSettings, schema: SettingsSchema?): String {
		val offered = schema?.mountControllersFor(kind, settings).orEmpty()
		// A CD drive goes on SCSI when the machine has a SCSI controller the
		// panel counts (amiberry_settings_mount_default: an A2091, GVP Series
		// II or A4091 board, or the A3000/A4000T built-in SCSI), else IDE.
		if (kind == MountKind.Cd) {
			val scsiBoard = settings.boards.any { it.typeId in setOf("a2091", "gvp", "a4091") && it.unit == 0 }
			val builtInScsi = settings.optionValue("scsi_a3000") == "true" || settings.optionValue("scsi_a4000t") == "true"
			if (scsiBoard || builtInScsi) offered.firstOrNull { it.id == "scsi%d" }?.let { return it.id }
		}
		// The Android editor can open a configuration before the desktop-generated
		// schema is installed, so preserve a cfgfile-compatible auto controller.
		return offered.firstOrNull()?.id ?: when (kind) {
			MountKind.Directory, MountKind.Hardfile, MountKind.Tape -> "uae%d"
			MountKind.Cd -> "ide%d"
		}
	}

	private fun firstFreeDevice(mounts: List<MountEntry>): String {
		val used = mounts.filter { it.kind == MountKind.Directory || it.kind == MountKind.Hardfile }
			.map { it.device.lowercase() }.toSet()
		return generateSequence(0) { it + 1 }.map { "DH$it" }.first { it.lowercase() !in used }
	}

	/** The first cdN/tapeN unit no entry of its kind uses; null when all MAX_EMU_UNITS are taken. */
	internal fun freeEmuUnit(kind: MountKind, mounts: List<MountEntry>): Int? =
		(0 until MAX_EMU_UNITS).firstOrNull { candidate -> mounts.none { it.kind == kind && it.unit == candidate } }

	/** cfgfile reads cd0..cd7 and tape0..tape7 (MAX_TOTAL_SCSI_DEVICES). */
	private const val MAX_EMU_UNITS = 8

	/** Whether a drive of [kind] can be added: the list has room, and a CD or tape drive a free unit of each kind. */
	fun canAdd(kind: MountKind, settings: EmulatorSettings, schema: SettingsSchema?): Boolean {
		if (settings.hardDrives.size >= MountEntry.MAX_MOUNTS) return false
		if ((kind == MountKind.Cd || kind == MountKind.Tape) && freeEmuUnit(kind, settings.hardDrives) == null) return false
		return kind == MountKind.Directory || placement(kind, settings, schema) != null
	}

	/**
	 * The controller and unit a new drive of [kind] goes on: the default
	 * controller's first free unit, else the first other offered controller
	 * with one (two CD drives fill IDE (Auto)); null when none is free.
	 */
	private fun placement(kind: MountKind, settings: EmulatorSettings, schema: SettingsSchema?): Pair<String, Int>? {
		val preferred = defaultController(kind, settings, schema)
		val others = schema?.mountControllersFor(kind, settings).orEmpty().map { it.id }.filter { it != preferred }
		return (listOf(preferred) + others).firstNotNullOfOrNull { template ->
			freeControllerUnit(template, kind, settings.hardDrives, schema)?.let { template to it }
		}
	}

	/** [settings] with a new drive of [kind] for [path] at the end; unchanged when [canAdd] says no. */
	fun added(settings: EmulatorSettings, kind: MountKind, path: String, schema: SettingsSchema?): EmulatorSettings =
		if (!canAdd(kind, settings, schema)) settings
		else settings.copy(hardDrives = settings.hardDrives + newMount(kind, path, settings, schema))

	/**
	 * The first unit of controller [template] ("ide%d") that no drive uses,
	 * among the units the schema gives it for [kind] (add_filesys_config
	 * refuses a controller with none free); null when all are taken.
	 */
	private fun freeControllerUnit(template: String, kind: MountKind, mounts: List<MountEntry>, schema: SettingsSchema?): Int? {
		val pattern = Regex("^" + template.split("%d").joinToString("(\\d+)") { Regex.escape(it) } + "$")
		val used = mounts.mapNotNull { pattern.matchEntire(it.controller)?.groupValues?.get(1)?.toIntOrNull() }.toSet()
		val row = schema?.mountControllers?.firstOrNull { it.id == template }
		val candidates = row?.units?.filter { it.supports(kind) }?.map { it.value } ?: (0 until MountEntry.MAX_MOUNTS).toList()
		return candidates.firstOrNull { it !in used }
	}

	/**
	 * The folder's name, or an archive's without its extension, with ':', ','
	 * and '"' turned into '_' as default_volume_name does in the C layer.
	 */
	internal fun volumeName(path: String): String {
		val file = File(path.trimEnd('/', '\\'))
		val name = if (file.isFile) file.name.substringBeforeLast('.') else file.name
		val replaced = name.map { if (it in ":,\"") '_' else it }.joinToString("")
		return MountNames.clean(replaced, device = false).ifBlank { "Harddrive" }
	}

	internal data class Geometry(val sectors: Int, val surfaces: Int, val reserved: Int, val blockSize: Int)

	private val plainGeometry = Geometry(32, 1, 2, 512)

	/** The geometry a new hardfile entry for [path] starts with: its RDB's, else the plain partition's. */
	internal fun hdfGeometry(path: String): Geometry = rdbGeometry(path) ?: plainGeometry

	/**
	 * hardfile_testrdb (src/osdep/amiberry_gui.cpp): an RDB in one of the
	 * disk's first 16 blocks means RDB mode; null when there is none.
	 */
	internal fun rdbGeometry(path: String): Geometry? {
		val file = File(path)
		if (!file.isFile) return null
		val bytes = runCatching { diskHead(file) }.getOrNull() ?: return null
		fun u(i: Int) = bytes[i].toInt() and 0xff
		fun tag(offset: Int, text: String, zeros: Int) =
			text.indices.all { u(offset + it) == text[it].code } && (0 until zeros).all { u(offset + text.length + it) == 0 }
		for (offset in 0..bytes.size - 512 step 512) {
			val babe = u(offset) == 0xba && u(offset + 1) == 0xbe // A2090
			// RDSK, or ADIDE's "encoded" RDSK.
			val adide = u(offset) == 0x53 && u(offset + 1) == 0x10 && u(offset + 2) == 0x9b &&
				u(offset + 3) == 0x13 && u(offset + 4) == 0 && u(offset + 5) == 0
			if (!tag(offset, "RDSK", 3) && !tag(offset, "CDSK", 3) && !tag(offset, "DRKS", 2) && !adide && !babe) continue
			val blockSize = if (babe) 512 else
				(u(offset + 16) shl 24) or (u(offset + 17) shl 16) or (u(offset + 18) shl 8) or u(offset + 19)
			return Geometry(0, 0, 0, if (blockSize >= 512) blockSize else 512)
		}
		return null
	}

	private const val HEAD = 16 * 512

	/**
	 * The first 16 blocks of the disk the image holds, read the way the
	 * core's hdf_open sees them: a dynamic VHD through its block table, a
	 * gzip (.hdz) or zip image unpacked, anything else as raw sectors
	 * (a fixed VHD keeps its footer at the end). Zeros past the disk's end.
	 */
	private fun diskHead(file: File): ByteArray {
		val out = ByteArray(HEAD)
		RandomAccessFile(file, "r").use { raf ->
			val magic = ByteArray(8)
			val got = if (raf.length() >= 8) { raf.readFully(magic); 8 } else 0
			fun be(b: ByteArray, at: Int, n: Int) = (0 until n).fold(0L) { v, i -> (v shl 8) or (b[at + i].toLong() and 0xff) }
			when {
				got == 8 && magic.decodeToString() == "conectix" -> {
					// Dynamic VHD: the footer's copy at 0 points to the dynamic
					// header, which points to the block table.
					val footer = ByteArray(512).also { raf.seek(0); raf.readFully(it) }
					val header = ByteArray(1024).also { raf.seek(be(footer, 0x10, 8)); raf.readFully(it) }
					val table = be(header, 0x10, 8)
					val blockSize = be(header, 0x20, 4)
					raf.seek(table)
					val first = raf.readInt().toLong() and 0xffffffffL
					if (first != 0xffffffffL) {
						// Each block starts with its sector bitmap, padded to a sector.
						val bitmap = ((blockSize / 512 / 8 + 511) / 512) * 512
						raf.seek(first * 512 + bitmap)
						readUpTo(raf::read, out, minOf(HEAD.toLong(), blockSize).toInt())
					}
				}
				got >= 2 && magic[0] == 0x1f.toByte() && magic[1] == 0x8b.toByte() ->
					GZIPInputStream(file.inputStream()).use { readUpTo(it::read, out, HEAD) }
				got >= 4 && magic.decodeToString(0, 4) == "PK\u0003\u0004" ->
					ZipInputStream(file.inputStream()).use { zip ->
						generateSequence { zip.nextEntry }.firstOrNull { !it.isDirectory }?.let { readUpTo(zip::read, out, HEAD) }
					}
				else -> {
					raf.seek(0)
					readUpTo(raf::read, out, HEAD)
				}
			}
		}
		return out
	}

	private fun readUpTo(read: (ByteArray, Int, Int) -> Int, into: ByteArray, count: Int) {
		var done = 0
		while (done < count) {
			val n = read(into, done, count - done)
			if (n <= 0) break
			done += n
		}
	}
}

/**
 * AmigaDOS device and volume names as a mount line can carry them: the line
 * separates them with ':' and ',', and quotes cannot be escaped there; the
 * core drops ';' (validatedevicename, validatevolumename) and spaces from
 * device names.
 */
object MountNames {
	fun allowed(char: Char, device: Boolean): Boolean =
		char !in ":,;\"" && !char.isISOControl() && !(device && char.isWhitespace())

	fun clean(name: String, device: Boolean): String = name.filter { allowed(it, device) }.take(30)
}
