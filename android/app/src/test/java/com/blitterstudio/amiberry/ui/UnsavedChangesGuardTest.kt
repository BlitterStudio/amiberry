package com.blitterstudio.amiberry.ui

import com.blitterstudio.amiberry.ui.UnsavedChangesGuard.Choice
import com.blitterstudio.amiberry.ui.UnsavedChangesGuard.Resolution
import com.blitterstudio.amiberry.ui.UnsavedChangesGuard.Trigger
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Test

class UnsavedChangesGuardTest {

	@Test
	fun `start without unsaved edits runs at once`() {
		assertNull(UnsavedChangesGuard.request(Trigger.Start, isDirty = false) {})
	}

	@Test
	fun `start with unsaved edits waits for an answer`() {
		var started = 0
		val pending = UnsavedChangesGuard.request(Trigger.Start, isDirty = true) { started++ }

		assertNotNull(pending)
		assertEquals(Trigger.Start, pending!!.trigger)
		assertEquals(0, started)
		pending.proceed()
		assertEquals(1, started)
	}

	@Test
	fun `save writes first and then starts`() {
		assertEquals(
			Resolution(saveFirst = true, discardEdits = false, proceed = true),
			UnsavedChangesGuard.resolve(Trigger.Start, Choice.Save)
		)
	}

	@Test
	fun `dont save starts with the edits and keeps them unsaved`() {
		assertEquals(
			Resolution(saveFirst = false, discardEdits = false, proceed = true),
			UnsavedChangesGuard.resolve(Trigger.Start, Choice.DontSave)
		)
	}

	@Test
	fun `dont save on leaving puts the saved values back`() {
		assertEquals(
			Resolution(saveFirst = false, discardEdits = true, proceed = true),
			UnsavedChangesGuard.resolve(Trigger.Leave, Choice.DontSave)
		)
	}

	@Test
	fun `cancel neither saves nor starts`() {
		Trigger.entries.forEach { trigger ->
			assertEquals(
				Resolution(saveFirst = false, discardEdits = false, proceed = false),
				UnsavedChangesGuard.resolve(trigger, Choice.Cancel)
			)
		}
	}
}
