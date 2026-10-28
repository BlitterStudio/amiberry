/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the keyboard shortcut field: a Mac's key codes as the HID usages
 * the layer spells, how stored shortcuts read, and what a recording makes
 * of a sequence of key presses.
 */

@testable import AmiberryNativeUI
import Carbon.HIToolbox
import XCTest

final class HotkeyFieldTests: XCTestCase {
	func testMacKeyCodesAreHIDUsages() {
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_ANSI_A)), 0x04)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_ANSI_0)), 0x27)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_F12)), 0x45)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_F20)), 0x6F)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_Escape)), 0x29)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_Delete)), 0x2A, "the Delete key is Backspace")
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_ForwardDelete)), 0x4C)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_Help)), 0x49, "Help is where PC keyboards have Insert")
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_ANSI_Keypad5)), 0x5D)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_UpArrow)), 0x52)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_ANSI_Semicolon)), 0x33)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_RightControl)), 0xE4)
		XCTAssertEqual(MacKeyCodes.hidUsage(UInt16(kVK_Command)), 0xE3)
		XCTAssertNil(MacKeyCodes.hidUsage(UInt16(kVK_Function)))
	}

	func testEveryMacKeyIsAShortcutOrAModifier() {
		for (code, usage) in MacKeyCodes.usages where HotkeyRecording.modifier(forUsage: usage) == nil {
			// Keypad + is the one key the value cannot spell (its name holds
			// the separator).
			if usage == 0x57 {
				XCTAssertNil(HotkeyCatalog.format(usage: usage, modifiers: []))
			} else {
				XCTAssertNotNil(HotkeyCatalog.format(usage: usage, modifiers: []), "key code \(code)")
			}
		}
	}

	func testStoredShortcutsRead() {
		XCTAssertEqual(HotkeyText.display("LCtrl+F12"), "Left \u{2303} + F12")
		XCTAssertEqual(HotkeyText.display("RCtrl"), "Right \u{2303}")
		XCTAssertEqual(HotkeyText.display("LShift+RGUI+Backspace"), "Left \u{21E7} + Right \u{2318} + Delete")
		XCTAssertEqual(HotkeyText.display("guide"), "Guide")
		XCTAssertEqual(HotkeyText.display(""), "None")
		XCTAssertEqual(HotkeyText.spoken("LCtrl+F12"), "Left Control F12")
		XCTAssertEqual(HotkeyText.spoken(""), "None")
	}

	func testARecordingTakesAKeyWithItsModifiers() {
		var recording = HotkeyRecording()
		XCTAssertEqual(recording.press(usage: 0xE0), .waiting)
		XCTAssertEqual(recording.press(usage: 0x45), .record("LCtrl+F12"))
	}

	func testAKeyOnlyBindingRecordsTheKeyWithoutItsModifiers() {
		// The Amiga keys are matched by key alone, so a modifier held with
		// the key is not part of the binding (and a lone modifier is none).
		var recording = HotkeyRecording()
		XCTAssertEqual(recording.press(usage: 0xE0, withModifiers: false), .waiting)
		XCTAssertEqual(recording.press(usage: 0x04, withModifiers: false), .record("A"))
		XCTAssertFalse(HotkeyCatalog.takesModifiers("amiberry.left_amiga"))
		XCTAssertFalse(HotkeyCatalog.takesModifiers("amiberry.right_amiga"))
		XCTAssertTrue(HotkeyCatalog.takesModifiers("amiberry.quit_amiberry"))
		XCTAssertTrue(HotkeyCatalog.takesModifiers("default_quit_key"))
	}

	func testALoneModifierRecordsWhenReleased() {
		var recording = HotkeyRecording()
		XCTAssertEqual(recording.press(usage: 0xE4), .waiting)
		XCTAssertEqual(recording.release(usage: 0xE4), .record("RCtrl"))
		// Two modifiers let go without a key record nothing; the next try
		// starts afresh.
		var twice = HotkeyRecording()
		_ = twice.press(usage: 0xE0)
		_ = twice.press(usage: 0xE1)
		XCTAssertEqual(twice.release(usage: 0xE1), .waiting)
		XCTAssertEqual(twice.release(usage: 0xE0), .waiting)
		_ = twice.press(usage: 0xE6)
		XCTAssertEqual(twice.release(usage: 0xE6), .record("RAlt"))
	}

	func testEscapeCancelsUnlessModified() {
		var recording = HotkeyRecording()
		XCTAssertEqual(recording.press(usage: 0x29), .cancel)
		var shifted = HotkeyRecording()
		_ = shifted.press(usage: 0xE1)
		XCTAssertEqual(shifted.press(usage: 0x29), .record("LShift+Escape"))
	}

	func testAKeyAShortcutCannotNameIsIgnored() {
		var recording = HotkeyRecording()
		XCTAssertEqual(recording.press(usage: 0x57), .waiting) // Keypad +
		XCTAssertEqual(recording.press(usage: 0x56), .record("Keypad -"))
	}
}
