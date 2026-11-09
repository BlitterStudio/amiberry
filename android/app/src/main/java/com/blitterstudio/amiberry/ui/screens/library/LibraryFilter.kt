package com.blitterstudio.amiberry.ui.screens.library

import androidx.annotation.StringRes
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Album
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.SaveAlt
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material.icons.filled.GridView
import androidx.compose.material.icons.filled.Folder
import androidx.compose.ui.graphics.vector.ImageVector
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.FileCategory

/**
 * A shelf of the Library's Media section: the media played last, or the files of one
 * of the media folders.
 */
enum class MediaShelf(val category: FileCategory?) {
	Recent(null),
	FloppyDisks(FileCategory.FLOPPIES),
	CompactDiscs(FileCategory.CD_IMAGES),
	WhdLoad(FileCategory.WHDLOAD_GAMES),
	HardDrives(FileCategory.HARD_DRIVES);
	val titleRes: Int
		get() = when (this) {
			Recent -> R.string.library_media_recent
			FloppyDisks -> R.string.library_media_floppies
			CompactDiscs -> R.string.library_media_cds
			WhdLoad -> R.string.library_media_whdload
			HardDrives -> R.string.library_media_hard_drives
		}

	val icon: ImageVector
		get() = when (this) {
			Recent -> Icons.Default.History
			FloppyDisks -> Icons.Default.SaveAlt
			CompactDiscs -> Icons.Default.Album
			WhdLoad -> Icons.Default.SportsEsports
			HardDrives -> Icons.Default.Storage
		}

	/** The shelf a folder category lists, or null for a category the Library does not shelve. */
	companion object {
		fun forCategory(category: FileCategory): MediaShelf? = entries.firstOrNull { it.category == category }
	}
}

/** What the Library shows: the machines (all of them or one category's), a Media shelf, or the Kickstart ROMs. */
sealed interface LibraryFilter {
	data object AllMachines : LibraryFilter

	/** One of the categories the saved machines declare (config_category). */
	data class Category(val name: String) : LibraryFilter

	data class Media(val shelf: MediaShelf) : LibraryFilter

	data object KickstartRoms : LibraryFilter

	@get:StringRes
	val titleRes: Int
		get() = when (this) {
			AllMachines -> R.string.library_all_machines
			is Category -> R.string.library_all_machines
			is Media -> shelf.titleRes
			KickstartRoms -> R.string.library_kickstart_roms
		}

	/** The name the chosen filter is kept under across recreation: stable and unique. */
	val persistName: String
		get() = when (this) {
			AllMachines -> "all"
			is Category -> "category:$name"
			is Media -> "media:${shelf.name}"
			KickstartRoms -> "roms"
		}

	/** The search field's prompt, naming what the filter lists. */
	@get:StringRes
	val searchPromptRes: Int
		get() = when (this) {
			AllMachines, is Category -> R.string.library_search_machines
			is Media -> R.string.library_search_media
			KickstartRoms -> R.string.library_search_roms
		}

	companion object {
		/** The Library's filters in the sidebar's order, with each machine category under Machines. */
		fun all(categories: List<String>): List<LibraryFilter> =
			listOf(AllMachines) + categories.map { Category(it) } +
				MediaShelf.entries.map { Media(it) } + KickstartRoms

		/** The filter [persistName] names; All Machines for a name that is not one (a category gone). */
		fun fromName(persistName: String): LibraryFilter = when {
			persistName == "all" || persistName.isEmpty() -> AllMachines
			persistName == "roms" -> KickstartRoms
			persistName.startsWith("category:") -> Category(persistName.removePrefix("category:"))
			persistName.startsWith("media:") ->
				MediaShelf.entries.firstOrNull { it.name == persistName.removePrefix("media:") }
					?.let { Media(it) } ?: AllMachines
			else -> AllMachines
		}
	}
}

/** The icon of a filter's row in the side list or the filter menu. */
val LibraryFilter.icon: ImageVector
	get() = when (this) {
		LibraryFilter.AllMachines -> Icons.Default.GridView
		is LibraryFilter.Category -> Icons.Default.Folder
		is LibraryFilter.Media -> shelf.icon
		LibraryFilter.KickstartRoms -> Icons.Default.Memory
	}

/** The categories the machines declare, sorted as names sort for a person. */
internal fun machineCategories(categoryOf: List<String>): List<String> =
	categoryOf.filter { it.isNotBlank() }.distinct().sortedWith(compareBy({ it.lowercase() }, { it }))
