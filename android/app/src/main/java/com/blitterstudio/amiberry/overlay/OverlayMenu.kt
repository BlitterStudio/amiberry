package com.blitterstudio.amiberry.overlay

/** What a drive row inserts into. */
sealed interface MediaTarget {
	data class Floppy(val drive: Int) : MediaTarget
	data object Cd : MediaTarget
}

/** What a confirmation page asks about. */
enum class ConfirmAction { HARD_RESET, QUIT }

/** The menu's pages. */
sealed interface OverlayPage {
	data object Home : OverlayPage
	data object Media : OverlayPage
	data class Picker(val target: MediaTarget) : OverlayPage
	data object States : OverlayPage
	data object Controls : OverlayPage
	data object Settings : OverlayPage
	data class Confirm(val action: ConfirmAction) : OverlayPage
}

/**
 * Where the menu is: a stack of pages, Home at the bottom. Back leaves the
 * top page for the one under it, with focus on the item that opened it (a
 * D-pad user lands where they were); Back on Home closes the menu.
 */
class OverlayMenu private constructor(private val stack: List<Entry>) {
	/** A page, and the item to focus when it shows (null: its first item). */
	private data class Entry(val page: OverlayPage, val focus: String? = null)

	val page: OverlayPage
		get() = stack.last().page

	/** The key of the item to focus on [page]; null for its first item. */
	val focus: String?
		get() = stack.last().focus

	/** How deep the menu is; 1 on Home. */
	val depth: Int
		get() = stack.size

	/**
	 * Opens [page] over this one from the item [from], which gets the focus
	 * back when [page] closes.
	 */
	fun open(page: OverlayPage, from: String): OverlayMenu =
		OverlayMenu(stack.dropLast(1) + stack.last().copy(focus = from) + Entry(page))

	/** The page under this one; null on Home, where Back closes the menu. */
	fun back(): OverlayMenu? = if (stack.size > 1) OverlayMenu(stack.dropLast(1)) else null

	override fun equals(other: Any?): Boolean = other is OverlayMenu && other.stack == stack
	override fun hashCode(): Int = stack.hashCode()
	override fun toString(): String = "OverlayMenu(${stack.joinToString(" > ") { "${it.page}@${it.focus}" }})"

	companion object {
		/** The menu as it opens: Home, with focus on Resume. */
		val HOME = OverlayMenu(listOf(Entry(OverlayPage.Home, OverlayKeys.RESUME)))
	}
}

/** Focus keys of the menu's items. */
object OverlayKeys {
	const val RESUME = "resume"
	const val PAUSE = "pause"
	const val MEDIA = "media"
	const val STATES = "states"
	const val CONTROLS = "controls"
	const val SETTINGS = "settings"
	const val RESET = "reset"
	const val HARD_RESET = "hard_reset"
	const val ADVANCED = "advanced"
	const val QUIT = "quit"

	fun insert(target: MediaTarget): String = when (target) {
		is MediaTarget.Floppy -> "insert_df${target.drive}"
		MediaTarget.Cd -> "insert_cd"
	}
}
