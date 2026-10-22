/*
 * Copyright (C) 2026 BlitterStudio
 * The on-screen Amiga keyboard's layout and key logic (the iOS view is
 * apple/iOS/AmigaKeyboardView.swift). Keys are Amiga raw key codes, which
 * the keyboard hardware defines (src/include/keyboard.h names them AK_*);
 * they reach the emulation through EmulatorAction.key.
 */

import Observation

/// A key of the on-screen keyboard.
struct AmigaKey: Hashable, Identifiable, Sendable {
	/// The Amiga raw key code.
	let code: Int
	let label: String
	/// What the key types with Shift, shown above the label.
	let shifted: String?
	/// Width in key units.
	let width: Double

	init(_ code: Int, _ label: String, _ shifted: String? = nil, width: Double = 1) {
		self.code = code
		self.label = label
		self.shifted = shifted
		self.width = width
	}

	var id: Int { code }

	enum Behaviour {
		/// Down while touched.
		case plain
		/// Shift, Ctrl, Alt, Amiga: a tap holds it down for the next key.
		case modifier
		/// Caps Lock: down until tapped again, as its LED shows.
		case toggle
	}

	var behaviour: Behaviour {
		switch code {
		case AmigaKey.capsLock: return .toggle
		case 0x60, 0x61, 0x63, 0x64, 0x65, 0x66, 0x67: return .modifier
		default: return .plain
		}
	}

	static let capsLock = 0x62
}

/// The keys, row by row, as on an Amiga 500 (US); every row is 15 keys
/// wide.
enum AmigaKeyboardLayout {
	static let main: [[AmigaKey]] = [
		[
			AmigaKey(0x45, "Esc", width: 1.5),
			AmigaKey(0x50, "F1"), AmigaKey(0x51, "F2"), AmigaKey(0x52, "F3"), AmigaKey(0x53, "F4"),
			AmigaKey(0x54, "F5"), AmigaKey(0x55, "F6"), AmigaKey(0x56, "F7"), AmigaKey(0x57, "F8"),
			AmigaKey(0x58, "F9"), AmigaKey(0x59, "F10"),
			AmigaKey(0x46, "Del", width: 1.5), AmigaKey(0x5F, "Help", width: 2),
		],
		[
			AmigaKey(0x00, "`", "~"), AmigaKey(0x01, "1", "!"), AmigaKey(0x02, "2", "@"), AmigaKey(0x03, "3", "#"),
			AmigaKey(0x04, "4", "$"), AmigaKey(0x05, "5", "%"), AmigaKey(0x06, "6", "^"), AmigaKey(0x07, "7", "&"),
			AmigaKey(0x08, "8", "*"), AmigaKey(0x09, "9", "("), AmigaKey(0x0A, "0", ")"), AmigaKey(0x0B, "-", "_"),
			AmigaKey(0x0C, "=", "+"), AmigaKey(0x0D, "\\", "|"), AmigaKey(0x41, "\u{232B}"),
		],
		[
			AmigaKey(0x42, "Tab", width: 1.5),
			AmigaKey(0x10, "Q"), AmigaKey(0x11, "W"), AmigaKey(0x12, "E"), AmigaKey(0x13, "R"), AmigaKey(0x14, "T"),
			AmigaKey(0x15, "Y"), AmigaKey(0x16, "U"), AmigaKey(0x17, "I"), AmigaKey(0x18, "O"), AmigaKey(0x19, "P"),
			AmigaKey(0x1A, "[", "{"), AmigaKey(0x1B, "]", "}"), AmigaKey(0x44, "Return", width: 1.5),
		],
		[
			AmigaKey(0x63, "Ctrl", width: 1.5), AmigaKey(capsLockCode, "Caps", width: 1.5),
			AmigaKey(0x20, "A"), AmigaKey(0x21, "S"), AmigaKey(0x22, "D"), AmigaKey(0x23, "F"), AmigaKey(0x24, "G"),
			AmigaKey(0x25, "H"), AmigaKey(0x26, "J"), AmigaKey(0x27, "K"), AmigaKey(0x28, "L"),
			AmigaKey(0x29, ";", ":"), AmigaKey(0x2A, "'", "\""), AmigaKey(0x2B, "#"),
		],
		[
			AmigaKey(0x60, "Shift", width: 1.25), AmigaKey(0x30, "<", ">"),
			AmigaKey(0x31, "Z"), AmigaKey(0x32, "X"), AmigaKey(0x33, "C"), AmigaKey(0x34, "V"), AmigaKey(0x35, "B"),
			AmigaKey(0x36, "N"), AmigaKey(0x37, "M"), AmigaKey(0x38, ",", "<"), AmigaKey(0x39, ".", ">"),
			AmigaKey(0x3A, "/", "?"), AmigaKey(0x61, "Shift", width: 2.75),
		],
		[
			AmigaKey(0x64, "Alt", width: 1.25), AmigaKey(0x66, "Amiga", width: 1.5),
			AmigaKey(0x40, "", width: 9.5),
			AmigaKey(0x67, "Amiga", width: 1.5), AmigaKey(0x65, "Alt", width: 1.25),
		],
	]

	/// The cursor keys, level with the main block's last two rows (nil is
	/// a gap of one key).
	static let cursor: [[AmigaKey?]] = [
		[], [], [], [],
		[nil, AmigaKey(0x4C, "\u{2191}")],
		[AmigaKey(0x4F, "\u{2190}"), AmigaKey(0x4D, "\u{2193}"), AmigaKey(0x4E, "\u{2192}")],
	]

	/// The numeric keypad, level with the main block's first five rows.
	static let keypad: [[AmigaKey]] = [
		[AmigaKey(0x5A, "("), AmigaKey(0x5B, ")"), AmigaKey(0x5C, "/"), AmigaKey(0x5D, "*")],
		[AmigaKey(0x3D, "7"), AmigaKey(0x3E, "8"), AmigaKey(0x3F, "9"), AmigaKey(0x4A, "-")],
		[AmigaKey(0x2D, "4"), AmigaKey(0x2E, "5"), AmigaKey(0x2F, "6"), AmigaKey(0x5E, "+")],
		[AmigaKey(0x1D, "1"), AmigaKey(0x1E, "2"), AmigaKey(0x1F, "3"), AmigaKey(0x43, "Enter")],
		[AmigaKey(0x0F, "0", width: 2), AmigaKey(0x3C, ".")],
	]

	private static let capsLockCode = AmigaKey.capsLock

	static var allKeys: [AmigaKey] {
		main.flatMap { $0 } + cursor.flatMap { $0.compactMap { $0 } } + keypad.flatMap { $0 }
	}

	/// The keycap of a key in a layout ("US", "UK", "DE", "FR", as the
	/// amiberry.vkbd_language option holds): the codes are the same, only
	/// the legends differ.
	static func legend(of key: AmigaKey, layout: String) -> (label: String, shifted: String?) {
		guard let override = overrides[layout.uppercased()]?[key.code] else {
			return (key.label, key.shifted)
		}
		return override
	}

	private static let overrides: [String: [Int: (label: String, shifted: String?)]] = [
		"UK": [
			0x0D: ("#", "~"), 0x2B: ("\\", "|"), 0x02: ("2", "\""), 0x03: ("3", "\u{00A3}"),
			0x2A: ("'", "@"), 0x30: ("\\", "|"),
		],
		"DE": [
			0x15: ("Z", nil), 0x31: ("Y", nil), 0x1A: ("\u{00DC}", nil), 0x1B: ("+", "*"),
			0x29: ("\u{00D6}", nil), 0x2A: ("\u{00C4}", nil), 0x00: ("^", "\u{00B0}"),
			0x0B: ("\u{00DF}", "?"), 0x0C: ("\u{00B4}", "`"), 0x3A: ("-", "_"), 0x38: (",", ";"),
			0x39: (".", ":"), 0x0D: ("#", "'"), 0x30: ("<", ">"), 0x02: ("2", "\""), 0x03: ("3", "\u{00A7}"),
			0x06: ("6", "&"), 0x07: ("7", "/"), 0x08: ("8", "("), 0x09: ("9", ")"), 0x0A: ("0", "="),
		],
		"FR": [
			0x10: ("A", nil), 0x11: ("Z", nil), 0x20: ("Q", nil), 0x31: ("W", nil), 0x29: ("M", nil),
			0x37: (",", "?"), 0x38: (";", "."), 0x39: (":", "/"), 0x3A: ("!", nil),
			0x01: ("&", "1"), 0x02: ("\u{00E9}", "2"), 0x03: ("\"", "3"), 0x04: ("'", "4"), 0x05: ("(", "5"),
			0x06: ("-", "6"), 0x07: ("\u{00E8}", "7"), 0x08: ("_", "8"), 0x09: ("\u{00E7}", "9"),
			0x0A: ("\u{00E0}", "0"), 0x0B: (")", nil), 0x0C: ("=", "+"), 0x1A: ("^", nil), 0x1B: ("$", nil),
			0x00: ("@", "#"), 0x2A: ("\u{00F9}", nil), 0x30: ("<", ">"),
		],
	]
}

/// What the on-screen keyboard has down. A modifier tapped stays down until
/// the next other key is released (Shift then A types a capital A); Caps
/// Lock stays down until it is tapped again. Every press is released:
/// `releaseAll()` lets go of everything when the keyboard hides.
@MainActor
@Observable
final class AmigaKeyboardModel {
	/// Keys down, whether touched or latched.
	private(set) var down: Set<Int> = []
	/// Modifiers and Caps Lock held down by a tap.
	private(set) var latched: Set<Int> = []
	/// For a modifier or Caps Lock being touched: whether the touch latched
	/// it (true) or let it go (false), undone if the touch is cancelled.
	@ObservationIgnored private var toggledByTouch: [Int: Bool] = [:]

	@ObservationIgnored private let post: (EmulatorAction) -> Void

	init(post: @escaping (EmulatorAction) -> Void = { _ = EmulatorActions.post($0) }) {
		self.post = post
	}

	/// A finger went down on the key.
	func touchDown(_ key: AmigaKey) {
		switch key.behaviour {
		case .plain:
			guard !down.contains(key.code) else { return }
			send(key.code, pressed: true)
		case .modifier, .toggle:
			let latching = !latched.contains(key.code)
			toggledByTouch[key.code] = latching
			setLatched(key.code, latching)
		}
	}

	private func setLatched(_ code: Int, _ on: Bool) {
		if on {
			latched.insert(code)
		} else {
			latched.remove(code)
		}
		send(code, pressed: on)
	}

	/// The finger lifted (or left the key).
	func touchUp(_ key: AmigaKey) {
		toggledByTouch[key.code] = nil
		guard key.behaviour == .plain, down.contains(key.code) else { return }
		send(key.code, pressed: false)
		// One key typed with the latched modifiers: they come up after it.
		for code in latched.sorted() where code != AmigaKey.capsLock {
			latched.remove(code)
			send(code, pressed: false)
		}
	}

	/// The system took the touch away (a system gesture, too many fingers):
	/// nothing was typed. A plain key comes up; a modifier or Caps Lock goes
	/// back to how it was before the touch; the other latched modifiers stay
	/// for the next key.
	func touchCancelled(_ key: AmigaKey) {
		switch key.behaviour {
		case .plain:
			guard down.contains(key.code) else { return }
			send(key.code, pressed: false)
		case .modifier, .toggle:
			guard let latchedByTouch = toggledByTouch.removeValue(forKey: key.code),
				latched.contains(key.code) == latchedByTouch
			else { return }
			setLatched(key.code, !latchedByTouch)
		}
	}

	/// Lets go of every key, latched modifiers included, when the keyboard
	/// hides or the emulation stops taking input. Caps Lock stays as it is:
	/// it is the Amiga's state, not a key held.
	func releaseAll() {
		for code in down.sorted() where code != AmigaKey.capsLock {
			send(code, pressed: false)
		}
		latched = latched.filter { $0 == AmigaKey.capsLock }
		toggledByTouch.removeAll()
	}

	private func send(_ code: Int, pressed: Bool) {
		if pressed {
			down.insert(code)
		} else {
			down.remove(code)
		}
		post(.key(code: code, pressed: pressed))
	}
}
