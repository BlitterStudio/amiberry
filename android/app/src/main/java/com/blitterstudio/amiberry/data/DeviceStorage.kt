package com.blitterstudio.amiberry.data

import android.content.Context
import android.os.Build
import android.os.Environment
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import com.blitterstudio.amiberry.R
import java.io.File
import java.io.IOException
import java.util.ArrayDeque

/**
 * The device's shared storage and removable volumes (USB sticks, SD cards) as plain
 * folders, for the in-app file browser. It needs All Files Access (Android 11+) or
 * READ_EXTERNAL_STORAGE (Android 10); TVs without a document picker have no other way in.
 */
object DeviceStorage {

	enum class RootKind { Internal, Downloads, Removable }

	data class Root(val label: String, val dir: File, val kind: RootKind)

	data class Entry(val file: File, val isDirectory: Boolean)

	/** Internal storage, its Download folder, then every mounted removable volume. */
	fun roots(context: Context): List<Root> {
		val roots = mutableListOf<Root>()
		val storageManager = context.getSystemService(StorageManager::class.java)
		val volumes = storageManager?.storageVolumes.orEmpty().filter { volume ->
			volume.state == Environment.MEDIA_MOUNTED || volume.state == Environment.MEDIA_MOUNTED_READ_ONLY
		}
		val primary = volumes.firstOrNull { it.isPrimary }?.let { volumeDirectory(it) }
			?: Environment.getExternalStorageDirectory()
		roots += Root(context.getString(R.string.device_browser_internal_storage), primary, RootKind.Internal)
		File(primary, Environment.DIRECTORY_DOWNLOADS).takeIf { it.isDirectory }?.let { downloads ->
			roots += Root(context.getString(R.string.device_browser_downloads), downloads, RootKind.Downloads)
		}
		volumes.filter { !it.isPrimary }.forEach { volume ->
			val dir = volumeDirectory(volume) ?: return@forEach
			roots += Root(volume.getDescription(context) ?: dir.name, dir, RootKind.Removable)
		}
		return roots
	}

	private fun volumeDirectory(volume: StorageVolume): File? =
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
			volume.directory
		} else if (volume.isPrimary) {
			Environment.getExternalStorageDirectory()
		} else {
			volume.uuid?.let { File("/storage", it) }
		}

	/**
	 * The folders and the files with one of [extensions] in [dir], folders first, each
	 * group by name; hidden entries are left out. Null when the folder cannot be read.
	 */
	fun entries(dir: File, extensions: Set<String>): List<Entry>? {
		val children = dir.listFiles() ?: return null
		return children
			.filter { !it.name.startsWith(".") && (it.isDirectory || it.isSupportedFile(extensions)) }
			.map { Entry(it, it.isDirectory) }
			.sortedWith(compareBy<Entry> { !it.isDirectory }.thenBy { it.file.name.lowercase() })
	}

	/**
	 * Every file with one of [extensions] below [dir], at most [MAX_DEPTH] folders deep.
	 * Each folder is read once, so links that loop back are harmless. Folders that are,
	 * or lie inside, one of [excluded] (the app's own storage) are skipped.
	 */
	fun supportedFilesUnder(dir: File, extensions: Set<String>, excluded: List<File> = emptyList()): List<File> {
		val files = mutableListOf<File>()
		val visited = mutableSetOf<String>()
		val excludedPaths = excluded.mapNotNull { canonicalOrNull(it) }
		val pending = ArrayDeque<Pair<File, Int>>()
		pending.add(dir to 0)
		while (pending.isNotEmpty()) {
			val (current, depth) = pending.removeFirst()
			val canonical = canonicalOrNull(current) ?: continue
			if (excludedPaths.any { canonical == it || canonical.startsWith(it + File.separator) }) continue
			if (!visited.add(canonical)) continue
			val children = current.listFiles() ?: continue
			children.sortedBy { it.name.lowercase() }.forEach { child ->
				when {
					child.name.startsWith(".") -> Unit
					child.isDirectory -> if (depth < MAX_DEPTH) pending.add(child to depth + 1)
					child.isSupportedFile(extensions) -> files += child
				}
			}
		}
		return files
	}

	private fun canonicalOrNull(file: File): String? = try {
		file.canonicalPath
	} catch (_: IOException) {
		null
	}

	private fun File.isSupportedFile(extensions: Set<String>): Boolean =
		isFile && extension.lowercase() in extensions

	private const val MAX_DEPTH = 16
}
