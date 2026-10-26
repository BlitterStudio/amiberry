/*
 * Copyright (C) 2026 BlitterStudio
 * The Acknowledgements window: the licences screen, opened from the
 * application menu. The iOS counterpart is a row in the settings sheet.
 */

import AppKit
import SwiftUI
/// The licences window. It stays up beside the Library or a running emulation.
@MainActor
final class AcknowledgementsWindow: NSObject, NSWindowDelegate {
	static let shared = AcknowledgementsWindow()

	private var window: NSWindow?

	func show() {
		let window = self.window ?? makeWindow()
		window.contentViewController = NSHostingController(
			rootView: LicencesView().windowSize(.acknowledgements))
		self.window = window
		NSApplication.shared.activate(ignoringOtherApps: true)
		window.makeKeyAndOrderFront(nil)
	}

	func windowWillClose(_ notification: Notification) {
		window?.contentViewController = nil
	}

	private func makeWindow() -> NSWindow {
		let size = WindowSize.acknowledgements.ideal
		// Full-size content, as the Library's: without it the split view's
		// detail slides under the toolbar once the window is resized.
		let window = NSWindow(
			contentRect: NSRect(origin: .zero, size: size),
			styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
			backing: .buffered, defer: false)
		// The title shows in the toolbar's principal item (LicencesView); an
		// NSWindow title would also render, truncated, in the sidebar's slot.
		window.title = ""
		window.isReleasedWhenClosed = false
		// Keep the size the user left the window at; min and ideal are the
		// same, so a first open is the size the licence texts are laid out for.
		if !window.setFrameUsingName("AmiberryAcknowledgements") {
			window.setContentSize(size)
			window.center()
		}
		window.setFrameAutosaveName("AmiberryAcknowledgements")
		window.minSize = WindowSize.acknowledgements.min
		window.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
		window.animationBehavior = .none
		window.delegate = self
		return window
	}
}
