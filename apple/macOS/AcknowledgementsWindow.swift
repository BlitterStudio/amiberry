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
		self.window = window
		presentWindow(hostWindowContent(LicencesView().windowSize(.acknowledgements)), in: window,
			autosave: "AmiberryAcknowledgements", size: WindowSize.acknowledgements.ideal)
	}

	func windowWillClose(_ notification: Notification) {
		releaseWindowContent(window)
	}

	private func makeWindow() -> NSWindow {
		// Full-size content, as the Library's: without it the split view's
		// detail slides under the toolbar once the window is resized.
		let window = NSWindow(
			contentRect: NSRect(origin: .zero, size: WindowSize.acknowledgements.ideal),
			styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
			backing: .buffered, defer: false)
		// The title shows in the toolbar's principal item (LicencesView); an
		// NSWindow title would also render, truncated, in the sidebar's slot.
		// It is kept, hidden, for the Window menu and Mission Control.
		window.title = "Acknowledgements"
		window.titleVisibility = .hidden
		window.isReleasedWhenClosed = false
		window.minSize = WindowSize.acknowledgements.min
		window.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
		window.animationBehavior = .none
		window.delegate = self
		return window
	}
}
