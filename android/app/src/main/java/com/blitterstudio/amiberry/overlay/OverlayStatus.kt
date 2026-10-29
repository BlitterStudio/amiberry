package com.blitterstudio.amiberry.overlay

/** One floppy drive as the running emulation reports it; [path] is "" when it is empty. */
data class DriveStatus(val drive: Int, val disabled: Boolean, val path: String)

/**
 * The media of the emulator's status snapshot (amiberry_actions_status):
 * the pending ones, so a disk inserted in the menu shows at once.
 */
data class OverlayStatus(
	/** CD drive LED level, -1 when the machine has no CD drive. */
	val cdActivity: Int,
	val cdPath: String,
	val drives: List<DriveStatus>
) {
	/** The drives the machine has. */
	val enabledDrives: List<DriveStatus>
		get() = drives.filterNot { it.disabled }

	/** Whether the machine has a CD drive, or a CD is inserted. */
	val hasCd: Boolean
		get() = cdActivity >= 0 || cdPath.isNotEmpty()

	companion object {
		const val FLOPPY_DRIVES = 4
		private const val FIXED_FIELDS = 2

		val EMPTY = OverlayStatus(
			cdActivity = -1,
			cdPath = "",
			drives = List(FLOPPY_DRIVES) { DriveStatus(it, disabled = it > 0, path = "") }
		)

		/**
		 * Reads [OverlayNative.status]: CD activity and CD path, then
		 * disabled and path for each drive. Null for anything shorter.
		 */
		fun parse(fields: Array<String>?): OverlayStatus? {
			if (fields == null || fields.size < FIXED_FIELDS + FLOPPY_DRIVES * 2) return null
			return OverlayStatus(
				cdActivity = fields[0].toIntOrNull() ?: -1,
				cdPath = fields[1],
				drives = List(FLOPPY_DRIVES) { drive ->
					val at = FIXED_FIELDS + drive * 2
					DriveStatus(drive, disabled = fields[at] == "1", path = fields[at + 1])
				}
			)
		}
	}
}
