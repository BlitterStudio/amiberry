package com.blitterstudio.amiberry.overlay

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * The on-screen keyboard's keys and logic, ported from the Apple tests
 * (apple/Tests/SharedUITests.swift): every Amiga key once, latching,
 * Caps Lock, release on hide, and the layouts' legends.
 */
class AmigaKeyboardTest {

	private val posted = mutableListOf<Pair<Int, Boolean>>()

	private fun keyboard() = AmigaKeyboardModel(post = { code, pressed -> posted.add(code to pressed) })

	private fun key(code: Int): AmigaKey =
		AmigaKeyboardLayout.ALL_KEYS.first { it.code == code }

	@Test
	fun `every key is an Amiga key once`() {
		val codes = AmigaKeyboardLayout.ALL_KEYS.map { it.code }
		assertEquals(codes.distinct().size, codes.size)
		assertTrue(codes.all { it in 0..0x7F })
		AmigaKeyboardLayout.MAIN.forEach { row ->
			assertEquals(15.0, row.sumOf { it.width }, 0.0)
		}
	}

	@Test
	fun `a latched modifier comes up after the next key`() {
		val keyboard = keyboard()
		val shift = key(0x60)
		val a = key(0x20)
		val b = key(0x35)
		keyboard.touchDown(shift)
		keyboard.touchUp(shift)
		keyboard.touchDown(a)
		assertEquals(setOf(0x60, 0x20), keyboard.down)
		keyboard.touchUp(a)
		keyboard.touchDown(b)
		keyboard.touchUp(b)
		assertEquals(
			listOf(
				0x60 to true, 0x20 to true, 0x20 to false,
				0x60 to false, 0x35 to true, 0x35 to false
			),
			posted
		)
		assertTrue(keyboard.down.isEmpty())
	}

	@Test
	fun `tapping a latched modifier again lets it go`() {
		val keyboard = keyboard()
		val ctrl = key(0x63)
		keyboard.touchDown(ctrl)
		keyboard.touchDown(ctrl)
		assertEquals(listOf(0x63 to true, 0x63 to false), posted)
		assertTrue(keyboard.latched.isEmpty())
	}

	@Test
	fun `caps lock stays down until tapped again`() {
		val keyboard = keyboard()
		val caps = key(AmigaKey.CAPS_LOCK)
		val a = key(0x20)
		keyboard.touchDown(caps)
		keyboard.touchDown(a)
		keyboard.touchUp(a)
		assertEquals(setOf(AmigaKey.CAPS_LOCK), keyboard.down)
		keyboard.releaseAll()
		assertEquals("hiding the keyboard keeps the Amiga's Caps Lock", setOf(AmigaKey.CAPS_LOCK), keyboard.down)
		keyboard.touchDown(caps)
		assertTrue(keyboard.down.isEmpty())
		assertEquals(0x62 to false, posted.last())
	}

	@Test
	fun `caps lock follows the emulation`() {
		val keyboard = keyboard()
		keyboard.syncCapsLock(true)
		assertEquals(setOf(AmigaKey.CAPS_LOCK), keyboard.down)
		assertTrue(posted.isEmpty())
		keyboard.touchDown(key(AmigaKey.CAPS_LOCK))
		assertEquals("one tap turns it off", listOf(0x62 to false), posted)
	}

	@Test
	fun `hiding releases held and latched keys`() {
		val keyboard = keyboard()
		keyboard.touchDown(key(0x64))
		keyboard.touchDown(key(0x4C))
		posted.clear()
		keyboard.releaseAll()
		assertEquals(setOf(0x64 to false, 0x4C to false), posted.toSet())
		assertTrue(keyboard.down.isEmpty())
		assertTrue(keyboard.latched.isEmpty())
		keyboard.touchUp(key(0x4C))
		assertEquals("a key released by hiding is not released twice", 2, posted.size)
	}

	@Test
	fun `a cancelled touch types nothing`() {
		val keyboard = keyboard()
		val shift = key(0x60)
		val a = key(0x20)
		keyboard.touchDown(shift)
		keyboard.touchUp(shift)
		keyboard.touchDown(a)
		keyboard.touchCancelled(a)
		assertEquals("the latched Shift waits for a key that is typed", setOf(0x60), keyboard.down)
		val ctrl = key(0x63)
		keyboard.touchDown(ctrl)
		keyboard.touchCancelled(ctrl)
		assertTrue("a cancelled modifier touch latches nothing", 0x63 !in keyboard.latched)
		assertEquals(listOf(0x63 to true, 0x63 to false), posted.takeLast(2))
		// A cancelled second tap leaves the latch on.
		keyboard.touchDown(shift)
		keyboard.touchCancelled(shift)
		assertTrue(0x60 in keyboard.latched)
		assertEquals(listOf(0x60 to false, 0x60 to true), posted.takeLast(2))
	}

	@Test
	fun `caps lock is a plain key when the keyboard model has no caps led`() {
		// The UAE keyboard model's status follows the host keyboard's Caps,
		// so nothing can show this keyboard's own Caps back (see
		// KeyboardOptions.capsLatches): every tap is one toggle of the
		// Amiga's Caps, and a held Caps comes up like any key.
		val keyboard = AmigaKeyboardModel({ code, pressed -> posted.add(code to pressed) }, capsLatches = false)
		val caps = key(AmigaKey.CAPS_LOCK)
		keyboard.touchDown(caps)
		keyboard.touchUp(caps)
		assertEquals(listOf(0x62 to true, 0x62 to false), posted)
		keyboard.syncCapsLock(true)
		assertTrue(keyboard.down.isEmpty())
		assertTrue(keyboard.latched.isEmpty())
		keyboard.touchDown(caps)
		keyboard.releaseAll()
		assertEquals(0x62 to false, posted.last())
		assertTrue(keyboard.down.isEmpty())
	}

	@Test
	fun `only the MCU keyboard models latch caps lock`() {
		fun options(vararg pairs: Pair<String, String>) =
			pairs.associate { (id, value) -> id to SettingState(value, true) }
		assertFalse(KeyboardOptions.from(options()).capsLatches)
		assertFalse(KeyboardOptions.from(options(KeyboardOptions.KEYBOARD_TYPE to "UAE")).capsLatches)
		assertFalse(KeyboardOptions.from(options(KeyboardOptions.KEYBOARD_TYPE to "disconnected")).capsLatches)
		assertTrue(KeyboardOptions.from(options(KeyboardOptions.KEYBOARD_TYPE to "a500_6570-036")).capsLatches)
		assertTrue(KeyboardOptions.from(options(KeyboardOptions.KEYBOARD_TYPE to "a1200_6805")).capsLatches)
	}

	@Test
	fun `layouts change legends not codes`() {
		val y = key(0x15)
		val z = key(0x31)
		val three = key(0x03)
		assertEquals("Z", AmigaKeyboardLayout.legend(y, "DE").label)
		assertEquals("Y", AmigaKeyboardLayout.legend(z, "de").label)
		assertEquals("£", AmigaKeyboardLayout.legend(three, "UK").shifted)
		assertEquals("Y", AmigaKeyboardLayout.legend(y, "US").label)
		assertEquals("Y", AmigaKeyboardLayout.legend(y, "unknown").label)
	}

	@Test
	fun `keyboard options fall back over unknown values`() {
		assertEquals(
			KeyboardOptions(),
			KeyboardOptions.from(
				mapOf(
					KeyboardOptions.LANGUAGE to SettingState("", true),
					KeyboardOptions.NUMPAD to SettingState("false", true),
					KeyboardOptions.TRANSPARENCY to SettingState("0", true)
				)
			)
		)
		assertEquals(
			KeyboardOptions(keypad = true, layout = "DE", opacity = 0.4f),
			KeyboardOptions.from(
				mapOf(
					KeyboardOptions.LANGUAGE to SettingState("DE", true),
					KeyboardOptions.NUMPAD to SettingState("true", true),
					KeyboardOptions.TRANSPARENCY to SettingState("40", true)
				)
			)
		)
		assertEquals(
			1f,
			KeyboardOptions.from(mapOf(KeyboardOptions.TRANSPARENCY to SettingState("140", true))).opacity
		)
	}
}
