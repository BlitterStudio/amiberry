package com.blitterstudio.amiberry.data.model

import java.io.File
import java.io.RandomAccessFile

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
	fun newMount(kind: MountKind, path: String, settings: EmulatorSettings, schema: SettingsSchema?): MountEntry {
		val controller = defaultController(kind, settings, schema)
		val controllerUnit = firstFreeUnit(controller, settings.hardDrives)
		val emuUnit = firstFreeEmuUnit(kind, settings.hardDrives)
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

	private fun defaultController(kind: MountKind, settings: EmulatorSettings, schema: SettingsSchema?): String {
		val offered = schema?.mountControllersFor(kind, settings).orEmpty()
		if (kind == MountKind.Cd) {
			val hasScsi = settings.boards.any { board ->
				schema?.boardTypes?.firstOrNull { it.id == board.typeId }?.categories?.contains("scsi") == true
			}
			if (hasScsi) offered.firstOrNull { it.id == "scsi%d" }?.let { return it.id }
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

	private fun firstFreeEmuUnit(kind: MountKind, mounts: List<MountEntry>): Int =
		generateSequence(0) { it + 1 }.first { candidate ->
			mounts.none { it.kind == kind && it.unit == candidate }
		}

	private fun firstFreeUnit(controller: String, mounts: List<MountEntry>): Int {
		val template = controllerTemplate(controller)
		val used = mounts.filter { controllerTemplate(it.controller) == template }
			.map { Regex("""^(?:uae|ide|scsi|custom)(\d+)""").find(it.controller)?.groupValues?.get(1)?.toIntOrNull() }
			.filterNotNull().toSet()
		return generateSequence(0) { it + 1 }.first { it !in used }
	}

	private fun controllerTemplate(value: String): String {
		val match = Regex("""^(uae|ide|scsi|custom)\d+""").find(value) ?: return value
		return value.replaceRange(match.range, match.groupValues[1] + "#")
	}

	/** The folder's name, or an archive's without its extension; ':' and ',' cannot be in a label. */
	internal fun volumeName(path: String): String {
		val file = File(path.trimEnd('/', '\\'))
		val name = if (file.isFile) file.name.substringBeforeLast('.') else file.name
		return name.replace(':', '_').replace(',', '_').ifBlank { "Harddrive" }
	}

	internal data class Geometry(val sectors: Int, val surfaces: Int, val reserved: Int, val blockSize: Int)

	/** hardfile_testrdb (src/osdep/amiberry_gui.cpp): an RDB in one of the first 16 blocks means RDB mode. */
	internal fun hdfGeometry(path: String): Geometry {
		val plain = Geometry(32, 1, 2, 512)
		val file = File(path)
		if (!file.isFile) return plain
		val bytes = ByteArray(16 * 512)
		val count = runCatching {
			RandomAccessFile(file, "r").use { raf ->
				val wanted = minOf(bytes.size.toLong(), raf.length()).toInt()
				raf.readFully(bytes, 0, wanted)
				wanted
			}
		}.getOrDefault(0)
		fun u(i: Int) = bytes[i].toInt() and 0xff
		fun tag(offset: Int, text: String, zeros: Int) =
			text.indices.all { u(offset + it) == text[it].code } && (0 until zeros).all { u(offset + text.length + it) == 0 }
		for (offset in 0..count - 512 step 512) {
			val babe = u(offset) == 0xba && u(offset + 1) == 0xbe // A2090
			// RDSK, or ADIDE's "encoded" RDSK.
			val adide = u(offset) == 0x53 && u(offset + 1) == 0x10 && u(offset + 2) == 0x9b &&
				u(offset + 3) == 0x13 && u(offset + 4) == 0 && u(offset + 5) == 0
			if (!tag(offset, "RDSK", 3) && !tag(offset, "CDSK", 3) && !tag(offset, "DRKS", 2) && !adide && !babe) continue
			val blockSize = if (babe) 512 else
				(u(offset + 16) shl 24) or (u(offset + 17) shl 16) or (u(offset + 18) shl 8) or u(offset + 19)
			return Geometry(0, 0, 0, if (blockSize >= 512) blockSize else 512)
		}
		return plain
	}
}
