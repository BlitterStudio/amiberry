package com.blitterstudio.amiberry.data

import java.io.File

/**
 * Which saved configuration Settings has open, kept right when the Configurations
 * screen renames or deletes files underneath it.
 */
object OpenConfigTracking {

	/** The open configuration: its name and file, or nulls for an unnamed configuration. */
	data class Tracked(val name: String?, val path: String?)

	val Unnamed = Tracked(name = null, path = null)

	/** Whether [tracked] is the configuration file at [path]. */
	fun isOpen(tracked: Tracked, path: String): Boolean {
		val trackedPath = tracked.path ?: return false
		return ConfigFiles.sameConfigPath(trackedPath, path)
	}

	/** After the file at [oldPath] became [renamed]: Settings follows it when it was the open one. */
	fun afterRename(tracked: Tracked, oldPath: String, renamed: File): Tracked =
		if (isOpen(tracked, oldPath)) {
			Tracked(name = renamed.nameWithoutExtension, path = renamed.absolutePath)
		} else {
			tracked
		}

	/**
	 * After the file at [deletedPath] was deleted: when it was the open one, Settings keeps
	 * its values as an unnamed configuration, which a later Save asks a name for.
	 */
	fun afterDelete(tracked: Tracked, deletedPath: String): Tracked =
		if (isOpen(tracked, deletedPath)) Unnamed else tracked
}
