/*
 * Copyright (C) 2026 BlitterStudio
 * A keyboard shortcut (SettingDescriptor.Kind.hotkey): the binding as it
 * reads, Record to take the next key press, a menu of what cannot be
 * pressed (Escape, which cancels a recording, and controller buttons) and
 * Clear. The layer spells every value (HotkeyCatalog); this file only turns
 * key events into HID usages and values into text.
 */

import SwiftUI
#if os(macOS)
import AppKit
import Carbon.HIToolbox
#else
import UIKit
#endif

/// How a stored shortcut reads: modifiers as the Mac's symbols with their
/// side ("Right ⌃"), a controller button by its name, "None" when unbound.
enum HotkeyText {
	/// The modifier tokens the emulator reads, and the ImGui GUI's longer
	/// spellings of them as a key.
	private static let modifiers: [String: (symbol: String, spoken: String)] = [
		"LCtrl": ("Left \u{2303}", "Left Control"), "RCtrl": ("Right \u{2303}", "Right Control"),
		"LShift": ("Left \u{21E7}", "Left Shift"), "RShift": ("Right \u{21E7}", "Right Shift"),
		"LAlt": ("Left \u{2325}", "Left Option"), "RAlt": ("Right \u{2325}", "Right Option"),
		"LGUI": ("Left \u{2318}", "Left Command"), "RGUI": ("Right \u{2318}", "Right Command"),
		"LeftCtrl": ("Left \u{2303}", "Left Control"), "RightCtrl": ("Right \u{2303}", "Right Control"),
		"LeftShift": ("Left \u{21E7}", "Left Shift"), "RightShift": ("Right \u{21E7}", "Right Shift"),
		"LeftAlt": ("Left \u{2325}", "Left Option"), "RightAlt": ("Right \u{2325}", "Right Option"),
		"LeftSuper": ("Left \u{2318}", "Left Command"), "RightSuper": ("Right \u{2318}", "Right Command"),
	]

	/// Key names as an Apple keyboard labels the keys: SDL's Backspace is
	/// the Delete key and its Delete the Forward Delete key. The ImGui GUI
	/// stores the arrows and Return under its own names.
	private static let keys: [String: String] = [
		"Backspace": "Delete", "Delete": "Forward Delete", "Enter": "Return",
		"Left": "Left Arrow", "Right": "Right Arrow", "Up": "Up Arrow", "Down": "Down Arrow",
		"LeftArrow": "Left Arrow", "RightArrow": "Right Arrow", "UpArrow": "Up Arrow", "DownArrow": "Down Arrow",
	]

	/// The binding as the field shows it.
	static func display(_ value: String) -> String {
		text(of: value, spoken: false)
	}

	/// The binding as VoiceOver reads it: the modifiers' names, not symbols.
	static func spoken(_ value: String) -> String {
		text(of: value, spoken: true)
	}

	private static func text(of value: String, spoken: Bool) -> String {
		if value.isEmpty {
			return "None"
		}
		if let button = HotkeyCatalog.buttons.first(where: { $0.value == value }) {
			return button.label
		}
		var parts = value.components(separatedBy: "+")
		let key = parts.removeLast()
		guard !key.isEmpty else { return value }
		let names = parts.map { token in modifiers[token].map { spoken ? $0.spoken : $0.symbol } ?? token }
		let keyName = modifiers[key].map { spoken ? $0.spoken : $0.symbol } ?? keys[key] ?? key
		return (names + [keyName]).joined(separator: spoken ? " " : " + ")
	}
}

/// The key presses of one recording, turned into a value as the GUI's
/// capture records one: a key with the modifiers held, or a modifier key
/// pressed and released with no other key. Escape alone cancels; a key a
/// shortcut cannot name (Keypad +) is ignored. Keys are USB HID keyboard
/// usages, the modifiers 0xE0 (Left Control) to 0xE7 (Right Command).
struct HotkeyRecording {
	enum Outcome: Equatable {
		case waiting
		case record(String)
		case cancel
	}

	static let escapeUsage: UInt32 = 0x29

	private(set) var held: HotkeyModifiers = []
	private var modifierPresses = 0
	private var keyPressed = false

	static func modifier(forUsage usage: UInt32) -> HotkeyModifiers? {
		switch usage {
		case 0xE0: .leftControl
		case 0xE1: .leftShift
		case 0xE2: .leftOption
		case 0xE3: .leftCommand
		case 0xE4: .rightControl
		case 0xE5: .rightShift
		case 0xE6: .rightOption
		case 0xE7: .rightCommand
		default: nil
		}
	}

	mutating func press(usage: UInt32, withModifiers: Bool = true) -> Outcome {
		if let modifier = Self.modifier(forUsage: usage) {
			if withModifiers {
				held.insert(modifier)
				modifierPresses += 1
			}
			return .waiting
		}
		keyPressed = true
		if usage == Self.escapeUsage, held.isEmpty {
			return .cancel
		}
		return HotkeyCatalog.format(usage: usage, modifiers: withModifiers ? held : []).map { Outcome.record($0) } ?? .waiting
	}

	mutating func release(usage: UInt32) -> Outcome {
		guard let modifier = Self.modifier(forUsage: usage), held.contains(modifier) else { return .waiting }
		held.remove(modifier)
		if !keyPressed, modifierPresses == 1, let value = HotkeyCatalog.format(usage: 0, modifiers: modifier) {
			return .record(value)
		}
		// Several modifiers let go without a key: the next try starts afresh.
		if held.isEmpty {
			modifierPresses = 0
			keyPressed = false
		}
		return .waiting
	}
}

/// Which field records: one at a time, so a key press never binds two
/// shortcuts.
@MainActor @Observable
final class HotkeyRecorderFocus {
	static let shared = HotkeyRecorderFocus()
	var owner: UUID?
}

struct HotkeyField: View {
	let label: String
	let value: String
	/// Whether the menu offers the controller buttons.
	let buttons: Bool
	/// Whether a modifier held with the key is part of the binding; false
	/// for one the emulator matches by key alone (the Amiga keys), where a
	/// modifier would fire on the bare key too.
	var modifiers = true
	let set: (String) -> Void
	@State private var id = UUID()
	@State private var recording = HotkeyRecording()
	#if os(macOS)
	@State private var monitor = HotkeyKeyMonitor()
	#endif

	private var isRecording: Bool { HotkeyRecorderFocus.shared.owner == id }

	var body: some View {
		LabeledContent(label) {
			HStack(spacing: 6) {
				Text(isRecording ? "Press a key\u{2026}" : HotkeyText.display(value))
					.foregroundStyle(isRecording || value.isEmpty ? .secondary : .primary)
					.lineLimit(1)
					.accessibilityLabel(isRecording ? "\(label): waiting for a key" : "\(label): \(HotkeyText.spoken(value))")
				Button(isRecording ? "Cancel" : "Record") {
					isRecording ? stop() : start()
				}
				.buttonStyle(.borderless)
				.foregroundStyle(Color.accentColor)
				.accessibilityLabel(isRecording ? "Cancel recording \(label)" : "Record \(label)")
				.accessibilityHint(isRecording ? "" : "Then press the key or combination to use. Escape cancels.")
				Menu {
					// Escape cancels a recording, so it is bound from here.
					Button("Escape Key") {
						stop()
						if let escape = HotkeyCatalog.format(usage: HotkeyRecording.escapeUsage, modifiers: []) {
							set(escape)
						}
					}
					if buttons {
						Section("Controller Buttons") {
							ForEach(HotkeyCatalog.buttons) { button in
								Button(button.label) {
									stop()
									set(button.value)
								}
							}
						}
					}
				} label: {
					Image(systemName: "ellipsis.circle")
				}
				.menuStyle(.button)
				.buttonStyle(.borderless)
				.fixedSize()
				.accessibilityLabel(buttons ? "Choose Escape or a controller button for \(label)" : "Choose Escape for \(label)")
				if !value.isEmpty {
					Button {
						stop()
						set("")
					} label: {
						Image(systemName: "xmark.circle.fill")
					}
					.buttonStyle(.borderless)
					.foregroundStyle(.secondary)
					.accessibilityLabel("Clear \(label)")
				}
			}
			#if os(iOS)
			.background {
				HotkeyCapture(active: isRecording) { usage, down in
					handle(usage: usage, down: down)
				}
				.frame(width: 0, height: 0)
				.accessibilityHidden(true)
			}
			#endif
		}
		#if os(macOS)
		.onChange(of: isRecording) { _, now in
			if now {
				monitor.start { event in handle(event) }
			} else {
				monitor.stop()
			}
		}
		#endif
		.onDisappear { stop() }
	}

	private func start() {
		recording = HotkeyRecording()
		HotkeyRecorderFocus.shared.owner = id
	}

	private func stop() {
		if isRecording {
			HotkeyRecorderFocus.shared.owner = nil
		}
		#if os(macOS)
		monitor.stop()
		#endif
	}

	private func handle(usage: UInt32, down: Bool) {
		guard isRecording else { return }
		// A binding without modifiers ignores them: a modifier held with the
		// key would be stored as part of it and fire on the bare key too.
		let uses = down ? recording.press(usage: usage, withModifiers: modifiers) : recording.release(usage: usage)
		switch uses {
		case .record(let text):
			stop()
			set(text)
		case .cancel:
			stop()
		case .waiting:
			break
		}
	}

	#if os(macOS)
	/// Takes every key event while recording, so a key combination never
	/// reaches a menu or another control. True: the event is used up.
	private func handle(_ event: NSEvent) -> Bool {
		switch event.type {
		case .keyDown:
			if !event.isARepeat, let usage = MacKeyCodes.hidUsage(event.keyCode) {
				handle(usage: usage, down: true)
			}
			return true
		case .keyUp:
			return true
		case .flagsChanged:
			// A modifier or Caps Lock changed: which key, and whether it is
			// down now, from the side-specific flags.
			guard let usage = MacKeyCodes.hidUsage(event.keyCode) else { return true }
			if let modifier = HotkeyRecording.modifier(forUsage: usage) {
				handle(usage: usage, down: MacKeyCodes.isDown(modifier, in: event.modifierFlags))
			} else {
				handle(usage: usage, down: true)
			}
			return true
		default:
			return false
		}
	}
	#endif
}

#if os(macOS)
/// The key events of the window a field records in (a local monitor sees
/// every Amiberry window's): keys typed into the running emulation beside
/// the settings reach the Amiga, not the recording.
@MainActor
final class HotkeyKeyMonitor {
	private var monitor: Any?

	func start(_ handle: @escaping @MainActor (NSEvent) -> Bool) {
		stop()
		// Record was just clicked there, so that window is key.
		let window = NSApplication.shared.keyWindow
		monitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .keyUp, .flagsChanged]) { event in
			let used = MainActor.assumeIsolated {
				(window == nil || event.window === window) && handle(event)
			}
			return used ? nil : event
		}
	}

	func stop() {
		if let monitor {
			NSEvent.removeMonitor(monitor)
		}
		monitor = nil
	}
}

/// A Mac's virtual key codes (kVK_*) as USB HID keyboard usages, for every
/// key a Mac keyboard sends: what SDL's Cocoa scancode table makes of them
/// (the Help key is Insert, where PC keyboards have it).
enum MacKeyCodes {
	static let usages: [Int: UInt32] = [
		kVK_ANSI_A: 0x04, kVK_ANSI_B: 0x05, kVK_ANSI_C: 0x06, kVK_ANSI_D: 0x07, kVK_ANSI_E: 0x08,
		kVK_ANSI_F: 0x09, kVK_ANSI_G: 0x0A, kVK_ANSI_H: 0x0B, kVK_ANSI_I: 0x0C, kVK_ANSI_J: 0x0D,
		kVK_ANSI_K: 0x0E, kVK_ANSI_L: 0x0F, kVK_ANSI_M: 0x10, kVK_ANSI_N: 0x11, kVK_ANSI_O: 0x12,
		kVK_ANSI_P: 0x13, kVK_ANSI_Q: 0x14, kVK_ANSI_R: 0x15, kVK_ANSI_S: 0x16, kVK_ANSI_T: 0x17,
		kVK_ANSI_U: 0x18, kVK_ANSI_V: 0x19, kVK_ANSI_W: 0x1A, kVK_ANSI_X: 0x1B, kVK_ANSI_Y: 0x1C,
		kVK_ANSI_Z: 0x1D,
		kVK_ANSI_1: 0x1E, kVK_ANSI_2: 0x1F, kVK_ANSI_3: 0x20, kVK_ANSI_4: 0x21, kVK_ANSI_5: 0x22,
		kVK_ANSI_6: 0x23, kVK_ANSI_7: 0x24, kVK_ANSI_8: 0x25, kVK_ANSI_9: 0x26, kVK_ANSI_0: 0x27,
		kVK_Return: 0x28, kVK_Escape: 0x29, kVK_Delete: 0x2A, kVK_Tab: 0x2B, kVK_Space: 0x2C,
		kVK_ANSI_Minus: 0x2D, kVK_ANSI_Equal: 0x2E, kVK_ANSI_LeftBracket: 0x2F, kVK_ANSI_RightBracket: 0x30,
		kVK_ANSI_Backslash: 0x31, kVK_ANSI_Semicolon: 0x33, kVK_ANSI_Quote: 0x34, kVK_ANSI_Grave: 0x35,
		kVK_ANSI_Comma: 0x36, kVK_ANSI_Period: 0x37, kVK_ANSI_Slash: 0x38, kVK_CapsLock: 0x39,
		kVK_F1: 0x3A, kVK_F2: 0x3B, kVK_F3: 0x3C, kVK_F4: 0x3D, kVK_F5: 0x3E, kVK_F6: 0x3F,
		kVK_F7: 0x40, kVK_F8: 0x41, kVK_F9: 0x42, kVK_F10: 0x43, kVK_F11: 0x44, kVK_F12: 0x45,
		kVK_Help: 0x49, kVK_Home: 0x4A, kVK_PageUp: 0x4B, kVK_ForwardDelete: 0x4C, kVK_End: 0x4D,
		kVK_PageDown: 0x4E, kVK_RightArrow: 0x4F, kVK_LeftArrow: 0x50, kVK_DownArrow: 0x51, kVK_UpArrow: 0x52,
		kVK_ANSI_KeypadClear: 0x53, kVK_ANSI_KeypadDivide: 0x54, kVK_ANSI_KeypadMultiply: 0x55,
		kVK_ANSI_KeypadMinus: 0x56, kVK_ANSI_KeypadPlus: 0x57, kVK_ANSI_KeypadEnter: 0x58,
		kVK_ANSI_Keypad1: 0x59, kVK_ANSI_Keypad2: 0x5A, kVK_ANSI_Keypad3: 0x5B, kVK_ANSI_Keypad4: 0x5C,
		kVK_ANSI_Keypad5: 0x5D, kVK_ANSI_Keypad6: 0x5E, kVK_ANSI_Keypad7: 0x5F, kVK_ANSI_Keypad8: 0x60,
		kVK_ANSI_Keypad9: 0x61, kVK_ANSI_Keypad0: 0x62, kVK_ANSI_KeypadDecimal: 0x63,
		kVK_ISO_Section: 0x64, kVK_ANSI_KeypadEquals: 0x67,
		kVK_F13: 0x68, kVK_F14: 0x69, kVK_F15: 0x6A, kVK_F16: 0x6B, kVK_F17: 0x6C, kVK_F18: 0x6D,
		kVK_F19: 0x6E, kVK_F20: 0x6F,
		kVK_Mute: 0x7F, kVK_VolumeUp: 0x80, kVK_VolumeDown: 0x81,
		kVK_JIS_KeypadComma: 0x85, kVK_JIS_Underscore: 0x87, kVK_JIS_Yen: 0x89,
		kVK_JIS_Kana: 0x90, kVK_JIS_Eisu: 0x91,
		kVK_Control: 0xE0, kVK_Shift: 0xE1, kVK_Option: 0xE2, kVK_Command: 0xE3,
		kVK_RightControl: 0xE4, kVK_RightShift: 0xE5, kVK_RightOption: 0xE6, kVK_RightCommand: 0xE7,
	]

	/// Nil for a key without a usage (Fn). On an ISO keyboard the key left
	/// of 1 and the key right of Left Shift come swapped, and SDL swaps them
	/// back (SDL_cocoakeyboard.m); so does this, or a shortcut recorded on
	/// one would fire on the other.
	static func hidUsage(_ keyCode: UInt16) -> UInt32? {
		guard let usage = usages[Int(keyCode)] else { return nil }
		guard KBGetLayoutType(Int16(LMGetKbdType())) == PhysicalKeyboardLayoutType(kKeyboardISO) else { return usage }
		switch usage {
		case 0x35: return 0x64
		case 0x64: return 0x35
		default: return usage
		}
	}

	/// The side-specific modifier bits of an event's flags (IOKit's
	/// NX_DEVICE*KEYMASK), which tell the left key from the right.
	private static let deviceBits: [(HotkeyModifiers, UInt)] = [
		(.leftControl, 0x0001), (.leftShift, 0x0002), (.rightShift, 0x0004), (.leftCommand, 0x0008),
		(.rightCommand, 0x0010), (.leftOption, 0x0020), (.rightOption, 0x0040), (.rightControl, 0x2000),
	]

	static func isDown(_ modifier: HotkeyModifiers, in flags: NSEvent.ModifierFlags) -> Bool {
		deviceBits.contains { $0.0 == modifier && flags.rawValue & $0.1 != 0 }
	}
}
#else
/// Takes a hardware keyboard's presses while a field records: the view
/// becomes first responder, and UIKit reports each key as its HID usage
/// (UIKey.keyCode), modifiers as keys of their own (Left Control is 0xE0).
struct HotkeyCapture: UIViewRepresentable {
	let active: Bool
	let handle: @MainActor (UInt32, Bool) -> Void

	func makeUIView(context: Context) -> HotkeyCaptureView {
		HotkeyCaptureView()
	}

	func updateUIView(_ view: HotkeyCaptureView, context: Context) {
		view.handle = handle
		// The responder chain changes after this update, not during it.
		let active = active
		Task { @MainActor in
			if active, !view.isFirstResponder {
				view.becomeFirstResponder()
			} else if !active, view.isFirstResponder {
				view.resignFirstResponder()
			}
		}
	}
}

final class HotkeyCaptureView: UIView {
	var handle: (@MainActor (UInt32, Bool) -> Void)?

	override var canBecomeFirstResponder: Bool { true }

	override func pressesBegan(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
		let rest = take(presses, down: true)
		if !rest.isEmpty {
			super.pressesBegan(rest, with: event)
		}
	}

	override func pressesEnded(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
		let rest = take(presses, down: false)
		if !rest.isEmpty {
			super.pressesEnded(rest, with: event)
		}
	}

	/// Hands every keyboard press to the field; returns the others.
	private func take(_ presses: Set<UIPress>, down: Bool) -> Set<UIPress> {
		guard let handle else { return presses }
		var rest = Set<UIPress>()
		for press in presses {
			if let key = press.key {
				handle(UInt32(key.keyCode.rawValue), down)
			} else {
				rest.insert(press)
			}
		}
		return rest
	}
}
#endif
