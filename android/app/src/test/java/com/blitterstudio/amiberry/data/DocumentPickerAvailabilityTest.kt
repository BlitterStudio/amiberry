package com.blitterstudio.amiberry.data

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class DocumentPickerAvailabilityTest {

	@Test
	fun `the Google TV documents stub and a missing handler are not pickers`() {
		assertFalse(DocumentPickerAvailability.isUsablePicker(null))
		assertFalse(DocumentPickerAvailability.isUsablePicker("com.android.tv.frameworkpackagestubs"))
	}

	@Test
	fun `the documents UI or the chooser is a picker`() {
		assertTrue(DocumentPickerAvailability.isUsablePicker("com.google.android.documentsui"))
		assertTrue(DocumentPickerAvailability.isUsablePicker("android"))
	}
}
