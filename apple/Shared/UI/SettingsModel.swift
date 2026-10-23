/*
 * Copyright (C) 2026 BlitterStudio
 * The native settings UI's model, shared by the macOS window and the iOS
 * screen: what the emulator shows instead of the built-in GUI. Every option
 * comes from the settings layer; the UI edits one session and commits it
 * when the user starts or resumes.
 */

import AmiberrySettings
import Foundation
import Observation
import SwiftUI
#if os(macOS)
import AppKit
#else
import UIKit
#endif

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
			"memory", "rtg", "rom"]
		let media = ["floppy", "diskswapper", "cd", "filesystem", "drawbridge"]
		let host = ["display", "sound", "input", "onscreen", "misc", "priority"]
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
		"misc": "Miscellaneous", "priority": "Priority", "rtg": "RTG Board",
	]

	private static let groupSymbols: [String: String] = [
		"cpu": "cpu", "mmu": "memorychip", "fpu": "function", "cpu_speed": "speedometer", "jit": "bolt",
		"ppc": "cpu", "x86": "pc", "chipset": "square.grid.3x3", "adv_chipset": "gearshape.2",
		"genlock": "tv", "memory": "memorychip", "rom": "doc.badge.gearshape", "floppy": "opticaldiscdrive",
		"diskswapper": "arrow.triangle.2.circlepath", "cd": "opticaldisc", "filesystem": "folder.badge.gearshape",
		"drawbridge": "cable.connector", "display": "display", "sound": "speaker.wave.2", "input": "gamecontroller",
		"onscreen": "hand.tap", "misc": "slider.horizontal.3", "priority": "gauge.with.dots.needle.33percent",
		"rtg": "rectangle.on.rectangle",
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

	/// The state saved as the iOS app last went to the background, when
	/// there is one and nothing runs yet. Only the iOS app writes it (on a
	/// Mac, a file of that name is the user's own).
	var autosave: (path: String, date: Date)? {
		#if os(iOS)
		guard !emulating, let folder = try? MediaLibrary.path(of: .saveStates) else { return nil }
		let path = URL(fileURLWithPath: folder).appendingPathComponent(AMIBERRY_AUTOSAVE_STATE_NAME).path
		guard let attributes = try? FileManager.default.attributesOfItem(atPath: path),
			let date = attributes[.modificationDate] as? Date
		else { return nil }
		return (path, date)
		#else
		return nil
		#endif
	}

	/// Restores the autosave state: the emulation starts from it, and the
	/// screen closes as for any state load. A state the emulator refuses
	/// leaves the screen up with the reason.
	func resumeAutosave(_ path: String) {
		guard let ticket = EmulatorActions.post(.loadState(path: path)) else {
			notice = "The emulator did not accept the request. Try again."
			return
		}
		Task { @MainActor [weak self] in
			while true {
				switch ticket.poll() {
				case .pending:
					try? await Task.sleep(for: .milliseconds(20))
				case let .failed(message):
					self?.notice = message
					return
				case .done, .unknown:
					return
				}
			}
		}
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
		#if os(macOS)
		NSApplication.shared.keyWindow?.makeFirstResponder(nil)
		#else
		UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
		#endif
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
