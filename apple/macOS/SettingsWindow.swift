/*
 * Copyright (C) 2026 BlitterStudio
 * The macOS windows the emulator shows in place of the built-in GUI: the
 * Library (LibraryView in apple/Shared/UI) before an emulation starts, and
 * the editor (SettingsRootView), opened from the Library or over a running
 * emulation.
 */

import AppKit
import SwiftUI

/// The Library and editor windows. The emulator waits while either is
/// up. The iOS counterpart is apple/iOS/SettingsPresenter.swift.
@MainActor
final class SettingsPresenter: NSObject, NSWindowDelegate {
	static let shared = SettingsPresenter()

	private var window: NSWindow?
	private var libraryWindow: NSWindow?
	private(set) var model: SettingsWindowModel?
	private(set) var library: LibraryModel?

	/// Whether a window is presented, minimised or not; the emulator waits
	/// until both are dismissed.
	var isVisible: Bool { model != nil || library != nil }

	/// Before an emulation starts the Library shows; over a running one,
	/// the editor.
	func show(emulating: Bool) {
		if emulating {
			showEditor(emulating: true, machine: Machine.current(), preset: nil)
		} else {
			showLibrary()
		}
	}

	func showLibrary() {
		// A load that completes after the Library closed (an IPC reset, or
		// the window closed meanwhile) acts on nothing.
		let library = LibraryModel(
			emulating: false,
			finish: { [weak self] in
				guard self?.library != nil else { return }
				self?.dismiss()
			},
			openEditor: { [weak self] machine, preset in
				guard self?.library != nil else { return }
				self?.showEditor(emulating: false, machine: machine, preset: preset)
			})
		let window = libraryWindow ?? makeWindow(autosave: "AmiberryLibrary", size: NSSize(width: 860, height: 560))
		window.contentViewController = NSHostingController(rootView: LibraryView(model: library))
		window.title = "Amiberry"
		libraryWindow = window
		self.library = library
		NSApplication.shared.activate(ignoringOtherApps: true)
		window.makeKeyAndOrderFront(nil)
	}

	private func showEditor(emulating: Bool, machine: Machine?, preset: (id: String, configuration: Int)?) {
		let fromLibrary = library != nil && !emulating
		let model: SettingsWindowModel
		do {
			model = try SettingsWindowModel(
				emulating: emulating, dismiss: { [weak self] in self?.dismiss() },
				machine: machine,
				closeToLibrary: fromLibrary ? { [weak self] in self?.closeEditor() } : nil)
		} catch {
			Alerts.show(title: "Settings", message: SettingsWindowModel.describe(error), buttons: ["OK"])
			return
		}
		if let preset {
			model.startNewMachine(preset: preset.id, configuration: preset.configuration)
		}
		let window = self.window ?? makeWindow(autosave: "AmiberrySettings", size: NSSize(width: 900, height: 620))
		window.contentViewController = NSHostingController(rootView: SettingsRootView(model: model))
		window.title = emulating ? "Amiberry Settings" : (machine?.name ?? "New Machine")
		self.window = window
		self.model = model
		if fromLibrary {
			library?.editing = true
		}
		NSApplication.shared.activate(ignoringOtherApps: true)
		window.makeKeyAndOrderFront(nil)
	}

	/// Closes the editor alone; the Library shows the machines saved meanwhile.
	private func closeEditor() {
		window?.orderOut(nil)
		window?.contentViewController = nil
		model = nil
		library?.editing = false
		library?.reload()
	}

	/// Closes every window: the emulator starts or resumes.
	func dismiss() {
		window?.orderOut(nil)
		window?.contentViewController = nil
		model = nil
		libraryWindow?.orderOut(nil)
		libraryWindow?.contentViewController = nil
		library = nil
	}

	func windowShouldClose(_ sender: NSWindow) -> Bool {
		if sender === libraryWindow {
			// Closing the Library before anything runs quits, as closing
			// the built-in GUI does.
			if model == nil {
				guard EmulatorActions.post(.quit) != nil else {
					library?.notice = "The emulator did not accept the request. Try again."
					return false
				}
				dismiss()
			}
			return false
		}
		model?.closeRequested()
		return false
	}

	private func makeWindow(autosave: String, size: NSSize) -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(origin: .zero, size: size),
			styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
			backing: .buffered, defer: false)
		window.isReleasedWhenClosed = false
		window.setContentSize(size)
		window.center()
		window.setFrameAutosaveName(autosave)
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
