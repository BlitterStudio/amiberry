package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.FileCategory
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

/**
 * The folders the launcher's repositories follow, each the one its
 * amiberry.conf path setting resolves to — the emulator's own resolution
 * (an override line, else the base content root's folder), the category
 * mapping the repositories use, and the reset that drops a setting's line.
 */
class ContentFoldersTest {

	@get:Rule
	val tempDir = TemporaryFolder()

	private fun settings(content: String?): File? {
		if (content == null) return null
		return tempDir.newFile().also { it.writeText(content) }
	}

	@Test
	fun `a setting without a line resolves to the base content root's folder`() {
		val external = tempDir.newFolder("external")

		assertEquals(
			File(external, "Floppies"),
			ContentFolders.effective("floppy_path", settings("write_logfile=yes\n"), external)
		)
		assertEquals(
			File(external, "ROMs"),
			ContentFolders.effective("rom_path", null, external)
		)
	}

	@Test
	fun `the setting's own line names the folder`() {
		val external = tempDir.newFolder("external-override")
		val disks = tempDir.newFolder("disks")
		val conf = settings("floppy_path=${disks.path}\n")
		assertEquals(disks, ContentFolders.effective("floppy_path", conf, external))
	}

	@Test
	fun `an explicit base content root moves every folder it does not override`() {
		val external = tempDir.newFolder("external-base")
		val content = tempDir.newFolder("content")
		val disks = tempDir.newFolder("content-disks")
		val conf = settings(
			"""
				base_content_path=${content.path}
				floppy_path=${disks.path}
			""".trimIndent()
		)

		assertEquals(disks, ContentFolders.effective("floppy_path", conf, external))
		assertEquals(
			File(content, "SaveStates"),
			ContentFolders.effective("savestate_dir", conf, external)
		)
	}

	@Test
	fun `a base superseded by its own lines keeps their folders out`() {
		val external = tempDir.newFolder("external-skip")
		val active = tempDir.newFolder("active")
		val serialized = tempDir.newFolder("serialized")
		val conf = settings(
			"""
				base_content_path=${active.path}
				config_path=${File(serialized, "Configurations").path}
				rom_path=${File(serialized, "ROMs").path}
				floppy_path=${File(serialized, "Floppies").path}
			""".trimIndent()
		)

		assertEquals(
			File(active, "Configurations"),
			ContentFolders.effective("config_path", conf, external)
		)
	}

	@Test
	fun `the retroarch config defaults into the configuration folder and plugins to none`() {
		val external = tempDir.newFolder("external-optional")

		assertNull(ContentFolders.effective("plugins_dir", null, external))
		assertNull(ContentFolders.effective("floppy_sounds_dir", null, external))
		assertEquals(
			File(File(external, "Configurations"), "retroarch.cfg"),
			ContentFolders.effective("retroarch_config", null, external)
		)
	}

	@Test
	fun `each media category follows its own setting`() {
		assertEquals("rom_path", ContentFolders.categoryKey(FileCategory.ROMS))
		assertEquals("floppy_path", ContentFolders.categoryKey(FileCategory.FLOPPIES))
		assertEquals("harddrive_path", ContentFolders.categoryKey(FileCategory.HARD_DRIVES))
		assertEquals("cdrom_path", ContentFolders.categoryKey(FileCategory.CD_IMAGES))
		assertEquals("whdload_arch_path", ContentFolders.categoryKey(FileCategory.WHDLOAD_GAMES))
	}

	@Test
	fun `the kickstarts live under the whdboot folder in effect`() {
		val external = tempDir.newFolder("external-kickstarts")
		val whdboot = tempDir.newFolder("whdboot")

		assertEquals(
			File(File(whdboot, "save-data"), "Kickstarts"),
			ContentFolders.kickstartsDir(settings("whdboot_path=${whdboot.path}"), external)
		)
		assertEquals(
			File(File(File(external, "WHDBoot"), "save-data"), "Kickstarts"),
			ContentFolders.kickstartsDir(null, external)
		)
	}

	@Test
	fun `a folder change rises on the changes flow`() {
		val before = ContentFolders.changes.value
		ContentFolders.notifyChanged()
		assertEquals(before + 1, ContentFolders.changes.value)
		assertFalse(ContentFolders.changes.value == before)
	}

	@Test
	fun `a path setting the base does not derive keeps its own line`() {
		val external = tempDir.newFolder("external-unmanaged")
		val sounds = tempDir.newFolder("sounds")
		val conf = settings(
			"""
				retroarch_config=/storage/configs/retroarch.cfg
				floppy_sounds_dir=${sounds.path}
				plugins_dir=/storage/plugins
			""".trimIndent()
		)

		assertEquals(File("/storage/configs/retroarch.cfg"), ContentFolders.effective("retroarch_config", conf, external))
		assertEquals(sounds, ContentFolders.effective("floppy_sounds_dir", conf, external))
		assertEquals(File("/storage/plugins"), ContentFolders.effective("plugins_dir", conf, external))
	}

	@Test
	fun `a dot-relative value resolves against the app's storage, as native loads it`() {
		val external = tempDir.newFolder("external-relative")
		val conf = settings("floppy_path=./Floppies/\nfloppy_sounds_dir=./sounds")

		assertEquals(
			File(external, "Floppies"),
			ContentFolders.effective("floppy_path", conf, external)
		)
		assertEquals(
			File(external, "sounds"),
			ContentFolders.effective("floppy_sounds_dir", conf, external)
		)
	}

	@Test
	fun `a dot-dot value resolves above the storage, a bare name stays as it is`() {
		val external = tempDir.newFolder("external-parent")
		val sibling = File(external.parentFile, "Elsewhere")
		val conf = settings("rom_path=../Elsewhere\ncdrom_path=CDs")

		assertEquals(sibling, ContentFolders.effective("rom_path", conf, external))
		// No "./" prefix is not one of the base-relative spellings; native
		// leaves it to the working directory, so the folder is not resolved.
		assertEquals(File("CDs"), ContentFolders.effective("cdrom_path", conf, external))
	}

	@Test
	fun `a dot alone names the storage itself and an absolute path stands`() {
		val external = tempDir.newFolder("external-dot")
		val absolute = tempDir.newFolder("absolute-disks")
		val conf = settings("saveimage_dir=./\nharddrive_path=${absolute.path}")

		assertEquals(external, ContentFolders.effective("saveimage_dir", conf, external))
		assertEquals(absolute, ContentFolders.effective("harddrive_path", conf, external))
	}

	@Test
	fun `a relative configurations folder resolves against the storage too`() {
		val external = tempDir.newFolder("external-config")
		val conf = settings("config_path=./MyMachines")

		assertEquals(
			File(external, "MyMachines"),
			ContentFolders.effective("config_path", conf, external)
		)
	}
}
