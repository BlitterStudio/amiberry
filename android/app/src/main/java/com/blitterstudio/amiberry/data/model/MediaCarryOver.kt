package com.blitterstudio.amiberry.data.model

/** A drive whose medium a model change can eject. */
enum class MediaDrive(val label: String) {
	DF0("DF0"),
	DF1("DF1"),
	DF2("DF2"),
	DF3("DF3"),
	CD("CD")
}

/** A new model's settings with the media kept from before, and the drives whose medium had to go. */
data class MediaCarryOver(val settings: EmulatorSettings, val dropped: List<MediaDrive>)

object MediaCarryOverRules {

	/**
	 * [next], the settings a model change built, with the disks and CD inserted in
	 * [previous]: a model change replaces the machine, not what is in its drives. A disk
	 * stays when the new machine has floppy drives (its drive enabled, as inserting a
	 * disk does), the CD when it has a CD drive; the others are [MediaCarryOver.dropped].
	 */
	fun keepMedia(previous: EmulatorSettings, next: EmulatorSettings): MediaCarryOver {
		val dropped = mutableListOf<MediaDrive>()
		var kept = next
		val hasFloppyDrives = next.floppy0Type != -1
		fun driveType(nextType: Int, previousType: Int): Int =
			nextType.takeIf { it != -1 } ?: previousType.takeIf { it != -1 } ?: 0
		if (previous.floppy0.isNotBlank()) {
			if (hasFloppyDrives) kept = kept.copy(floppy0 = previous.floppy0) else dropped += MediaDrive.DF0
		}
		if (previous.floppy1.isNotBlank()) {
			if (hasFloppyDrives) {
				kept = kept.copy(floppy1 = previous.floppy1, floppy1Type = driveType(next.floppy1Type, previous.floppy1Type))
			} else {
				dropped += MediaDrive.DF1
			}
		}
		if (previous.floppy2.isNotBlank()) {
			if (hasFloppyDrives) {
				kept = kept.copy(floppy2 = previous.floppy2, floppy2Type = driveType(next.floppy2Type, previous.floppy2Type))
			} else {
				dropped += MediaDrive.DF2
			}
		}
		if (previous.floppy3.isNotBlank()) {
			if (hasFloppyDrives) {
				kept = kept.copy(floppy3 = previous.floppy3, floppy3Type = driveType(next.floppy3Type, previous.floppy3Type))
			} else {
				dropped += MediaDrive.DF3
			}
		}
		if (previous.cdImage.isNotBlank()) {
			if (next.baseModel.hasCd) kept = kept.copy(cdImage = previous.cdImage) else dropped += MediaDrive.CD
		}
		return MediaCarryOver(kept, dropped)
	}
}
