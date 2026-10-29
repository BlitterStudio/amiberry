package com.blitterstudio.amiberry.ui

import androidx.compose.runtime.staticCompositionLocalOf

/**
 * What the app shell does when an action would leave unsaved edits to a named configuration
 * behind: leaving Settings or starting the emulator from it asks Save / Don't Save / Cancel first.
 */
object UnsavedChangesGuard {

	/** The action that waits for an answer. */
	enum class Trigger {
		/** Leaving the Settings screen (another tab, or Back). */
		Leave,

		/** Starting the emulator from Settings. */
		Start
	}

	enum class Choice { Save, DontSave, Cancel }

	/** A pending question: [trigger] waits for an answer before running [proceed]. */
	class Pending(val trigger: Trigger, val proceed: () -> Unit)

	/** What answering [Choice] means for the open configuration and the waiting action. */
	data class Resolution(
		/** Write the edits to the configuration's file first; the action runs only once that succeeds. */
		val saveFirst: Boolean,
		/** Put the editor back to the last saved values. */
		val discardEdits: Boolean,
		/** Run the waiting action. */
		val proceed: Boolean
	)

	/** The question to ask before [proceed] runs, or null when there are no unsaved edits and it may run now. */
	fun request(trigger: Trigger, isDirty: Boolean, proceed: () -> Unit): Pending? =
		if (isDirty) Pending(trigger, proceed) else null

	/**
	 * Don't Save on leaving puts the editor back to the saved values, as the edits are left
	 * behind; on Start it starts the emulator with the edits and leaves them open and unsaved,
	 * so the file stays as it was.
	 */
	fun resolve(trigger: Trigger, choice: Choice): Resolution = when (choice) {
		Choice.Save -> Resolution(saveFirst = true, discardEdits = false, proceed = true)
		Choice.DontSave -> Resolution(
			saveFirst = false,
			discardEdits = trigger == Trigger.Leave,
			proceed = true
		)
		Choice.Cancel -> Resolution(saveFirst = false, discardEdits = false, proceed = false)
	}
}

/** Lets a screen ask the app shell before starting the emulator with unsaved edits. */
fun interface StartGuard {
	/** Run [proceed] now, or after the user has answered the unsaved-changes question. */
	fun confirmStart(proceed: () -> Unit)
}

/** The shell's [StartGuard]; outside the shell (previews, tests) the action just runs. */
val LocalStartGuard = staticCompositionLocalOf { StartGuard { proceed -> proceed() } }
