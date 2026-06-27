package com.blitterstudio.amiberry.ui.screens

import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class QuickStartLayoutTest {

	@Test
	fun `floating start action is only shown when a launch is available`() {
		val quickStartScreen = File("src/main/java/com/blitterstudio/amiberry/ui/screens/QuickStartScreen.kt")
			.readText()

		assertTrue(
			"QuickStartScreen should not show the floating Start action when canStart is false.",
			Regex("""floatingActionButton = \{\s*if \(canStart\) \{[\s\S]*?ExtendedFloatingActionButton""")
				.containsMatchIn(quickStartScreen)
		)
	}

	@Test
	fun `quick start screen exposes mode selection effective summary and rich recent cards`() {
		val quickStartScreen = File("src/main/java/com/blitterstudio/amiberry/ui/screens/QuickStartScreen.kt")
			.readText()

		assertTrue(quickStartScreen.contains("QuickStartModeSelector("))
		assertTrue(quickStartScreen.contains("EffectiveSummaryCard("))
		assertTrue(quickStartScreen.contains("RecentLaunches.details(entry)"))
	}

	@Test
	fun `quick start setup warning includes ROM readiness diagnostics`() {
		val quickStartScreen = File("src/main/java/com/blitterstudio/amiberry/ui/screens/QuickStartScreen.kt")
			.readText()

		assertTrue(quickStartScreen.contains("RomReadinessDiagnostics.from(roms)"))
		assertTrue(quickStartScreen.contains("RomReadinessSummary("))
	}
}
