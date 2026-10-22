/*
 * Copyright (C) 2026 BlitterStudio
 * The native macOS settings window: what the emulator shows instead of the
 * built-in GUI. Every option comes from the settings layer; the window
 * edits one session and commits it when the user starts or resumes.
 */

import AppKit
import Observation
import SwiftUI

/// A page of the settings window.
enum SettingsPage: Hashable, Identifiable {
	/// Machine presets.
	case quickStart
	/// The options of a layer group.
	case group(String)
	/// The mounted drives.
	case hardDrives
	/// The emulator's folders.
	case paths

	var id: Self { self }

	var title: String {
		switch self {
		case .quickStart: return "Quick Start"
		case let .group(id): return SettingsPage.groupTitle(id)
		case .hardDrives: return "Hard Drives"
		case .paths: return "Paths"
		}
	}

	var symbol: String {
		switch self {
		case .quickStart: return "sparkles"
		case .hardDrives: return "externaldrive"
		case .paths: return "folder"
		case let .group(id): return SettingsPage.groupSymbols[id] ?? "slider.horizontal.3"
		}
	}

	/// The sidebar: machine, media and host pages, then any group the layer
	/// adds that has no place yet, so every option stays reachable.
	static func sections(groups: [String] = SettingsCatalog.groups) -> [(title: String, pages: [SettingsPage])] {
		let machine = ["cpu", "mmu", "fpu", "cpu_speed", "jit", "ppc", "x86", "chipset", "adv_chipset", "genlock",
			"memory", "rom"]
		let media = ["floppy", "diskswapper", "cd", "filesystem", "drawbridge"]
		let host = ["display", "sound", "input", "onscreen"]
		let known = Set(machine + media + host)
		let present = Set(groups)
		func pages(_ ids: [String]) -> [SettingsPage] { ids.filter(present.contains).map { .group($0) } }
		var sections: [(String, [SettingsPage])] = [
			("Machine", [.quickStart] + pages(machine)),
			("Media", pages(media) + [.hardDrives]),
			("Host", pages(host) + [.paths]),
		]
		let other = groups.filter { !known.contains($0) }.map { SettingsPage.group($0) }
		if !other.isEmpty {
			sections.append(("Other", other))
		}
		return sections
	}

	static func groupTitle(_ id: String) -> String {
		if let title = groupTitles[id] {
			return title
		}
		return id.split(separator: "_").map { $0.prefix(1).uppercased() + $0.dropFirst() }.joined(separator: " ")
	}

	private static let groupTitles: [String: String] = [
		"cpu": "CPU", "mmu": "MMU", "fpu": "FPU", "cpu_speed": "CPU Speed", "jit": "JIT Compiler",
		"ppc": "PowerPC", "x86": "x86 Bridgeboard", "chipset": "Chipset", "adv_chipset": "Advanced Chipset",
		"genlock": "Genlock", "memory": "Memory", "rom": "ROMs", "floppy": "Floppy Drives",
		"diskswapper": "Disk Swapper", "cd": "CD", "filesystem": "Filesystem", "drawbridge": "DrawBridge",
		"display": "Display", "sound": "Sound", "input": "Input", "onscreen": "On-screen Controls",
	]

	private static let groupSymbols: [String: String] = [
		"cpu": "cpu", "mmu": "memorychip", "fpu": "function", "cpu_speed": "speedometer", "jit": "bolt",
		"ppc": "cpu", "x86": "pc", "chipset": "square.grid.3x3", "adv_chipset": "gearshape.2",
		"genlock": "tv", "memory": "memorychip", "rom": "doc.badge.gearshape", "floppy": "opticaldiscdrive",
		"diskswapper": "arrow.triangle.2.circlepath", "cd": "opticaldisc", "filesystem": "folder.badge.gearshape",
		"drawbridge": "cable.connector", "display": "display", "sound": "speaker.wave.2", "input": "gamecontroller",
		"onscreen": "hand.tap",
	]
}

/// The state of one settings window: its session and what the user is
/// about to do with it.
@MainActor
@Observable
final class SettingsWindowModel {
	let store: SettingsStore
	/// Whether an emulation runs under the window (resume) or starts after it.
	let emulating: Bool
	var page: SettingsPage = .quickStart
	/// The outcome of the last folder or mount change, when it failed.
	var notice: String?
	/// Text typed into fields that is not a valid value, by option: the
	/// window does not close over it, and the field shows it again when it
	/// comes back into view.
	var invalidDrafts: [String: String] = [:]

	@ObservationIgnored private let dismiss: () -> Void
	@ObservationIgnored private let post: (EmulatorAction) -> Bool

	init(
		emulating: Bool, dismiss: @escaping () -> Void,
		post: @escaping (EmulatorAction) -> Bool = { EmulatorActions.post($0) != nil }
	) throws {
		store = try SettingsStore()
		self.emulating = emulating
		self.dismiss = dismiss
		self.post = post
		if emulating {
			page = .group("cpu")
		}
	}

	/// Options whose stored value the running emulation does not use yet,
	/// by the step that brings them in (reset or restart).
	var waiting: [ApplyClass: [SettingDescriptor]] {
		_ = store.revision
		guard emulating else { return [:] }
		var result: [ApplyClass: [SettingDescriptor]] = [:]
		for descriptor in SettingsCatalog.descriptors where descriptor.applyClass != .live {
			guard let value = store.value(of: descriptor.id),
				let active = try? SettingsSession.activeValue(of: descriptor.id), active != value
			else { continue }
			result[descriptor.applyClass, default: []].append(descriptor)
		}
		return result
	}

	/// Start (or resume) with the session's changes.
	func startOrResume() {
		guard fieldsValid(), commit() else { return }
		// What applies live reaches the emulation between frames.
		if emulating, !request(.applySettings(.live)) {
			return
		}
		dismiss()
	}

	/// Hard reset with the session's changes; changes that wait for a reset
	/// come in with it.
	func reset() {
		guard fieldsValid(), commit(), request(.applySettings(.reset)) else { return }
		dismiss()
	}

	/// Restart the emulation with the session's changes.
	func restart() {
		guard fieldsValid(), commit(), request(.applySettings(.restart)) else { return }
		dismiss()
	}

	func quit() {
		guard request(.quit) else { return }
		dismiss()
	}

	/// Posts an action; when the emulator refuses it (a full queue), the
	/// window stays open with a notice so the user can try again.
	private func request(_ action: EmulatorAction) -> Bool {
		guard post(action) else {
			notice = "The emulator did not accept the request. Try again."
			return false
		}
		return true
	}

	/// The window's close button: resume, or quit when nothing runs yet (as
	/// closing the built-in GUI does).
	func closeRequested() {
		if emulating {
			startOrResume()
		} else {
			quit()
		}
	}

	/// A reset, quit or state load from the menu or IPC closes the window:
	/// the edits are stored first (fields with invalid text keep their
	/// previous value).
	func commitForExternalRequest() {
		_ = commit()
	}

	private func fieldsValid() -> Bool {
		guard invalidDrafts.isEmpty else {
			let labels = invalidDrafts.keys.sorted().map { SettingsCatalog.descriptor(for: $0)?.label ?? $0 }
			notice = "Correct \(labels.joined(separator: ", ")) first."
			return false
		}
		return true
	}

	private func commit() -> Bool {
		// A field still being edited stores its text as editing ends.
		NSApplication.shared.keyWindow?.makeFirstResponder(nil)
		do {
			try store.commit()
			return true
		} catch {
			notice = SettingsWindowModel.describe(error)
			return false
		}
	}

	static func describe(_ error: Error) -> String {
		guard let error = error as? SettingsError else { return "\(error)" }
		switch error {
		case let .unknownID(id): return "Unknown setting: \(id)"
		case let .invalidValue(id, value):
			let label = SettingsCatalog.descriptor(for: id)?.label ?? id
			return value.isEmpty ? "\(label): not a valid value." : "\(label): \u{201C}\(value)\u{201D} is not a valid value."
		case let .unavailable(id):
			let descriptor = SettingsCatalog.descriptor(for: id)
			let reason = descriptor?.constraint.map { " \($0)" } ?? ""
			return "\(descriptor?.label ?? id) is not available now.\(reason)"
		case let .valueTooLong(id): return "\(SettingsCatalog.descriptor(for: id)?.label ?? id): the value is too long."
		case .outOfMemory: return "Out of memory."
		}
	}
}

/// The settings window, shown by the emulator in place of the built-in GUI.
@MainActor
final class SettingsWindowController: NSObject, NSWindowDelegate {
	static let shared = SettingsWindowController()

	private var window: NSWindow?
	private(set) var model: SettingsWindowModel?

	/// Whether the window is presented, minimised or not; the emulator
	/// waits until it is dismissed.
	var isVisible: Bool { model != nil }

	func show(emulating: Bool) {
		let model: SettingsWindowModel
		do {
			model = try SettingsWindowModel(emulating: emulating, dismiss: { [weak self] in self?.dismiss() })
		} catch {
			Alerts.show(title: "Settings", message: SettingsWindowModel.describe(error), buttons: ["OK"])
			return
		}
		let window = self.window ?? makeWindow()
		window.contentViewController = NSHostingController(rootView: SettingsRootView(model: model))
		window.title = emulating ? "Amiberry Settings" : "Amiberry"
		if self.window == nil {
			window.setContentSize(NSSize(width: 900, height: 620))
			window.center()
		}
		self.window = window
		self.model = model
		NSApplication.shared.activate(ignoringOtherApps: true)
		window.makeKeyAndOrderFront(nil)
	}

	func dismiss() {
		window?.orderOut(nil)
		window?.contentViewController = nil
		model = nil
	}

	func windowShouldClose(_ sender: NSWindow) -> Bool {
		model?.closeRequested()
		return false
	}

	private func makeWindow() -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(x: 0, y: 0, width: 900, height: 620),
			styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
			backing: .buffered, defer: false)
		window.isReleasedWhenClosed = false
		window.setFrameAutosaveName("AmiberrySettings")
		window.minSize = NSSize(width: 720, height: 480)
		// Over a full-screen emulation the window shows on the emulation's
		// Space, as the built-in GUI shows inside it.
		window.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
		// Ordering animations need the application's run loop, which the
		// emulation loop only pumps for events: show and hide at once.
		window.animationBehavior = .none
		window.delegate = self
		return window
	}
}
