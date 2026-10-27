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
		let window = libraryWindow ?? makeWindow(size: NSSize(width: 860, height: 560))
		window.title = "Amiberry"
		libraryWindow = window
		self.library = library
		presentWindow(hostWindowContent(LibraryView(model: library).windowSize(.library)), in: window,
			autosave: "AmiberryLibrary", size: NSSize(width: 860, height: 560))
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
		let window = self.window ?? makeWindow(size: NSSize(width: 900, height: 620))
		window.title = emulating ? "Amiberry Settings" : (machine?.name ?? "New Machine")
		self.window = window
		self.model = model
		if fromLibrary {
			library?.editing = true
		}
		presentWindow(hostWindowContent(SettingsRootView(model: model).windowSize(.editor)), in: window,
			autosave: "AmiberrySettings", size: NSSize(width: 900, height: 620))
	}

	/// Closes the editor alone; the Library shows the machines saved meanwhile.
	private func closeEditor() {
		window?.orderOut(nil)
		releaseWindowContent(window)
		model = nil
		library?.editing = false
		library?.reload()
	}

	/// Closes every window: the emulator starts or resumes.
	func dismiss() {
		window?.orderOut(nil)
		releaseWindowContent(window)
		model = nil
		libraryWindow?.orderOut(nil)
		releaseWindowContent(libraryWindow)
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

	private func makeWindow(size: NSSize) -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(origin: .zero, size: size),
			styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
			backing: .buffered, defer: false)
		window.isReleasedWhenClosed = false
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

/// The app settings window (Cmd-,): the emulator's own settings, beside the
/// Library, the editor or a running emulation.
@MainActor
final class AppSettingsWindow: NSObject, NSWindowDelegate {
	static let shared = AppSettingsWindow()

	private var window: NSWindow?

	func show() {
		let window = self.window ?? makeWindow()
		self.window = window
		presentWindow(hostWindowContent(AppSettingsView(model: AppSettingsModel()).windowSize(.appSettings)), in: window,
			autosave: "AmiberryAppSettings", size: NSSize(width: 640, height: 600))
	}

	func windowWillClose(_ notification: Notification) {
		releaseWindowContent(window)
		// The Configurations folder may have changed meanwhile.
		SettingsPresenter.shared.library?.reload()
	}

	private func makeWindow() -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(origin: .zero, size: NSSize(width: 640, height: 600)),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered, defer: false)
		window.title = "Settings"
		window.isReleasedWhenClosed = false
		window.minSize = NSSize(width: 520, height: 400)
		window.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
		window.animationBehavior = .none
		window.delegate = self
		return window
	}
}

/// The windows' sizes. A hosting controller sizes its window to the SwiftUI
/// view's ideal size, each time its content is set: without one of their
/// own, the Library came up taller than the screen and came back from an
/// emulation 300 points wide.
struct WindowSize {
	let min: NSSize
	let ideal: NSSize

	static let library = WindowSize(min: NSSize(width: 720, height: 480), ideal: NSSize(width: 860, height: 560))
	static let editor = WindowSize(min: NSSize(width: 720, height: 480), ideal: NSSize(width: 900, height: 620))
	static let appSettings = WindowSize(min: NSSize(width: 520, height: 400), ideal: NSSize(width: 640, height: 600))
	static let acknowledgements = WindowSize(min: NSSize(width: 860, height: 680), ideal: NSSize(width: 860, height: 680))
}

extension View {
	func windowSize(_ size: WindowSize) -> some View {
		frame(minWidth: size.min.width, idealWidth: size.ideal.width, maxWidth: .infinity,
			minHeight: size.min.height, idealHeight: size.ideal.height, maxHeight: .infinity)
	}
}

/// Installs a window's SwiftUI content and hands the window its frame.
/// A window's contentViewController is sized by AppKit from the content's
/// ideal size when it is installed (a NavigationSplitView's ideal height
/// is far taller than any window), and the SwiftUI tree keeps that first
/// oversized layout: on a fresh install, with no saved frame, the top of
/// the content, the sidebar's rows and the Library's ROM banner, ends up
/// above the visible area, and resizing the window does not bring it
/// back. The hosted view is installed as the content view instead, so the
/// window keeps the frame it is given - a saved one winning over the
/// intended size - and the view autoresizes with it. The window retains
/// the controller, which owns the view.
nonisolated(unsafe) private var hostedWindowControllerKey: UInt8 = 0

@MainActor
func presentWindow<V: View>(_ hosting: NSHostingController<V>, in window: NSWindow, autosave: String, size: NSSize) {
	objc_setAssociatedObject(window, &hostedWindowControllerKey, hosting, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
	window.contentView = hosting.view
	// setFrameUsingName is true only when a saved frame was applied;
	// setFrameAutosaveName's result just says whether the name was free.
	if !window.setFrameUsingName(autosave) {
		window.setContentSize(size)
		window.center()
	}
	window.setFrameAutosaveName(autosave)
	NSApplication.shared.activate(ignoringOtherApps: true)
	window.makeKeyAndOrderFront(nil)
	DispatchQueue.main.async {
		hosting.view.needsLayout = true
		hosting.view.layoutSubtreeIfNeeded()
	}
}

/// Drops a window's SwiftUI content installed by presentWindow, so the
/// hierarchy and its models (an editor's native settings session) are
/// released while the window object is kept for the next presentation.
@MainActor
func releaseWindowContent(_ window: NSWindow?) {
	guard let window else { return }
	window.contentView = nil
	objc_setAssociatedObject(window, &hostedWindowControllerKey, nil, .OBJC_ASSOCIATION_RETAIN_NONATOMIC)
}

/// Hosts a window's SwiftUI content without sizing the window from it;
/// presentWindow(_:in:autosave:size:) installs the result.
@MainActor
func hostWindowContent<V: View>(_ view: V) -> NSHostingController<V> {
	let hosting = NSHostingController(rootView: view)
	hosting.sizingOptions = []
	return hosting
}
