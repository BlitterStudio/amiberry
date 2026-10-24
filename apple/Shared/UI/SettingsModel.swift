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
			"memory", "accelerator", "rtg", "expansions", "rom"]
		let media = ["floppy", "diskswapper", "cd", "filesystem", "drawbridge"]
		let host = ["display", "filter", "sound", "input", "io", "onscreen", "misc", "config", "priority"]
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
		"misc": "Miscellaneous", "config": "Configuration", "priority": "Priority", "rtg": "RTG Board", "accelerator": "Accelerator Board",
		"expansions": "Expansions", "filter": "Filters & Shaders",
		"io": "IO Ports",
	]

	private static let groupSymbols: [String: String] = [
		"cpu": "cpu", "mmu": "memorychip", "fpu": "function", "cpu_speed": "speedometer", "jit": "bolt",
		"ppc": "cpu", "x86": "pc", "chipset": "square.grid.3x3", "adv_chipset": "gearshape.2",
		"genlock": "tv", "memory": "memorychip", "rom": "doc.badge.gearshape", "floppy": "opticaldiscdrive",
		"diskswapper": "arrow.triangle.2.circlepath", "cd": "opticaldisc", "filesystem": "folder.badge.gearshape",
		"drawbridge": "cable.connector", "display": "display", "sound": "speaker.wave.2", "input": "gamecontroller",
		"onscreen": "hand.tap", "misc": "slider.horizontal.3", "config": "text.document", "priority": "gauge.with.dots.needle.33percent",
		"rtg": "rectangle.on.rectangle", "accelerator": "bolt.horizontal", "expansions": "puzzlepiece.extension",
		"filter": "camera.filters", "io": "cable.connector.horizontal",
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

	/// The machine being edited, opened from the Library: nil for a new one
	/// until it is saved.
	private(set) var machine: Machine?
	/// The model a new machine started from, which Quick Start shows.
	private(set) var newMachineModel: (id: String, configuration: Int)?
	/// Whether the editor was opened from the Library (it closes back to it).
	let fromLibrary: Bool

	@ObservationIgnored private let dismiss: () -> Void
	@ObservationIgnored private let post: (EmulatorAction) -> Bool
	@ObservationIgnored private let closeToLibrary: (() -> Void)?

	/// closeToLibrary, when the editor was opened from the Library, closes
	/// the editor alone (the Library stays up); dismiss closes everything,
	/// so the emulator starts or resumes.
	init(
		emulating: Bool, dismiss: @escaping () -> Void,
		post: @escaping (EmulatorAction) -> Bool = { EmulatorActions.post($0) != nil },
		machine: Machine? = nil, closeToLibrary: (() -> Void)? = nil
	) throws {
		store = try SettingsStore()
		self.emulating = emulating
		self.dismiss = dismiss
		self.post = post
		self.machine = machine
		self.closeToLibrary = closeToLibrary
		fromLibrary = closeToLibrary != nil
		if emulating {
			page = .group("cpu")
		}
	}

	/// A new machine from a model: the model is applied, and Quick Start
	/// opens showing it.
	func startNewMachine(preset id: String, configuration: Int) {
		// A new machine is no saved file's: Settings over it later must not
		// offer to save it over the machine edited before.
		MediaLibrary.forgetCurrentConfiguration()
		_ = store.applyPreset(id, configuration: configuration)
		newMachineModel = (id, configuration)
		page = .quickStart
	}

	/// The name the machine is saved under: its file name, or empty for a
	/// new one.
	var machineName: String { machine?.name ?? "" }

	/// Saves the machine as a configuration in the configurations folder,
	/// with its description and category: the session's edits are committed
	/// first, so the file is what the editor shows. Returns whether it saved.
	func save(name: String, description: String, category: String) async -> Bool {
		let trimmed = name.trimmingCharacters(in: .whitespaces)
		guard !trimmed.isEmpty else {
			notice = "Enter a name for the machine."
			return false
		}
		guard !trimmed.contains("/"), !trimmed.contains(":"), !trimmed.hasPrefix(".") else {
			notice = "A name cannot contain / or :, or start with a dot."
			return false
		}
		let folder: String
		do {
			folder = try MediaLibrary.path(of: .configurations)
		} catch {
			notice = SettingsWindowModel.describe(error)
			return false
		}
		// The machine being edited, saved under its own name, keeps its file
		// as it is spelled (Workbench.UAE stays Workbench.UAE); any other
		// name is a new file in the configurations folder.
		let path = machine.flatMap { $0.name == trimmed ? $0.path : nil }
			?? URL(fileURLWithPath: folder).appendingPathComponent(trimmed + ".uae").path
		// Saving over another machine replaces it: only the machine being
		// edited (its file, by full path) is saved over in place. Checked
		// before anything is committed, so a refused save changes nothing.
		let sameFile = machine.map {
			URL(fileURLWithPath: $0.path).standardizedFileURL.path == URL(fileURLWithPath: path).standardizedFileURL.path
		} ?? false
		if !sameFile, FileManager.default.fileExists(atPath: path) {
			notice = "A machine named \u{201C}\(trimmed)\u{201D} already exists. Choose another name."
			return false
		}
		guard fieldsValid() else { return false }
		guard store.set(description, for: "config_description"), store.set(category, for: "config_category") else {
			notice = store.lastError.map(SettingsWindowModel.describe) ?? "The description or category is not valid."
			return false
		}
		guard commit() else { return false }
		guard let ticket = EmulatorActions.post(.saveConfiguration(path: path)) else {
			notice = "The emulator did not accept the request. Try again."
			return false
		}
		while true {
			switch ticket.poll() {
			case .pending:
				try? await Task.sleep(for: .milliseconds(20))
			case let .failed(message):
				notice = message
				return false
			case .done, .unknown:
				notice = nil
				// The editor now edits the machine just saved.
				machine = Machine(path: path, name: trimmed, description: description, category: category)
				return true
			}
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
		emulating ? nil : SettingsWindowModel.autosaveState()
	}

	/// The iOS autosave state file and when it was written, when there is one.
	static func autosaveState() -> (path: String, date: Date)? {
		#if os(iOS)
		guard let folder = try? MediaLibrary.path(of: .saveStates) else { return nil }
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
		} else if let closeToLibrary {
			// Back to the Library, with the edits kept as the pending
			// configuration.
			guard fieldsValid(), commit() else { return }
			closeToLibrary()
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
