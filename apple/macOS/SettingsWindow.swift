/*
 * Copyright (C) 2026 BlitterStudio
 * The macOS settings window, shown by the emulator in place of the built-in
 * GUI (SettingsRootView in apple/Shared/UI).
 */

import AppKit
import SwiftUI

/// The settings window, shown by the emulator in place of the built-in GUI.
/// The iOS counterpart is apple/iOS/SettingsPresenter.swift.
@MainActor
final class SettingsPresenter: NSObject, NSWindowDelegate {
	static let shared = SettingsPresenter()

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
