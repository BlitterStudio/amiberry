package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class IntentImportExecutorTest {

	@Test
	fun `mediaLaunchFor launches WHDLoad with control config`() {
		assertEquals(
			IntentImportExecutor.Launch.WhdLoad(
				lhaPath = "/games/Lotus.lha",
				configPath = "/configs/controls.uae"
			),
			IntentImportExecutor.mediaLaunchFor(
				category = FileCategory.WHDLOAD_GAMES,
				importedPath = "/games/Lotus.lha",
				configPath = "/configs/controls.uae"
			)
		)
	}

	@Test
	fun `mediaLaunchFor launches floppy imports as A500 quickstart`() {
		assertEquals(
			IntentImportExecutor.Launch.QuickStart(
				model = AmigaModel.A500,
				floppyPath = "/floppies/Lotus.adf",
				floppy1Path = null,
				cdPath = null,
				configPath = "/configs/quickstart.uae"
			),
			IntentImportExecutor.mediaLaunchFor(
				category = FileCategory.FLOPPIES,
				importedPath = "/floppies/Lotus.adf",
				configPath = "/configs/quickstart.uae"
			)
		)
	}

	@Test
	fun `mediaLaunchFor launches CD imports as CD32 quickstart`() {
		assertEquals(
			IntentImportExecutor.Launch.QuickStart(
				model = AmigaModel.CD32,
				floppyPath = null,
				floppy1Path = null,
				cdPath = "/cds/Game.chd",
				configPath = "/configs/quickstart.uae"
			),
			IntentImportExecutor.mediaLaunchFor(
				category = FileCategory.CD_IMAGES,
				importedPath = "/cds/Game.chd",
				configPath = "/configs/quickstart.uae"
			)
		)
	}

	@Test
	fun `mediaLaunchFor leaves ROM and hard drive imports in storage without auto launch`() {
		assertNull(
			IntentImportExecutor.mediaLaunchFor(
				category = FileCategory.ROMS,
				importedPath = "/roms/kick.rom",
				configPath = "/configs/ignored.uae"
			)
		)
		assertNull(
			IntentImportExecutor.mediaLaunchFor(
				category = FileCategory.HARD_DRIVES,
				importedPath = "/hds/System.hdf",
				configPath = "/configs/ignored.uae"
			)
		)
	}
}
