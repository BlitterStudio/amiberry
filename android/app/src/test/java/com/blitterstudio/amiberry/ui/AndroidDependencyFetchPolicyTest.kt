package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Test
import java.io.File

class AndroidDependencyFetchPolicyTest {
	@Test
	fun androidFetchContentUpdatesAreNotDisconnected() {
		val dependencies = File("../../cmake/Dependencies.cmake").readText()
		val androidBlock = dependencies.substringAfter("if(ANDROID)").substringBefore("else()")

		assertFalse(
			"Android FetchContent builds must allow updates so pinned SDL tag changes cannot reuse stale native sources.",
			Regex("""FETCHCONTENT_UPDATES_DISCONNECTED\s+ON""").containsMatchIn(androidBlock)
		)
	}
}
