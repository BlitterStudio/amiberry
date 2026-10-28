/*
 * Copyright (C) 2026 BlitterStudio
 * While the emulation is paused, its window says so over the picture: the
 * picture dims under a "Paused" badge. Full screen has no toolbar, so the
 * toolbar's status would not show it there. The overlay is a borderless
 * child panel that takes no clicks or keys, shown only while paused.
 */

import AppKit
import SwiftUI

@MainActor
final class PauseOverlay {
	static let shared = PauseOverlay()

	private var panel: NSPanel?
	private weak var window: NSWindow?
	private var monitor: StatusMonitor?
	private var observers: [NSObjectProtocol] = []

	/// The emulation window SDL created, and the monitor of its status.
	func attach(to window: NSWindow, monitor: StatusMonitor) {
		hide()
		self.window = window
		let first = self.monitor == nil
		self.monitor = monitor
		observers.forEach(NotificationCenter.default.removeObserver)
		observers = [NSWindow.didResizeNotification, NSWindow.didEnterFullScreenNotification,
			NSWindow.didExitFullScreenNotification].map { name in
			NotificationCenter.default.addObserver(forName: name, object: window, queue: .main) { _ in
				MainActor.assumeIsolated { PauseOverlay.shared.place() }
			}
		}
		if first {
			observe()
		}
		refresh()
	}

	/// Shows or hides the overlay as the run state changes.
	private func observe() {
		guard let monitor else { return }
		withObservationTracking {
			_ = monitor.status
		} onChange: {
			Task { @MainActor in
				PauseOverlay.shared.refresh()
				PauseOverlay.shared.observe()
			}
		}
	}

	/// Shows the overlay while paused with no settings window up, hides it
	/// otherwise. The settings windows call it as they show and close: a
	/// paused emulation stays paused then, so no status change does.
	func refresh() {
		// The settings window holds its own pause, ended by its Resume
		// button: the badge's shortcut would do nothing then.
		if monitor?.status.runState == .paused, !SettingsPresenter.shared.isVisible {
			show()
		} else {
			hide()
		}
	}

	private func show() {
		guard let window, window.isVisible else { return }
		let panel = self.panel ?? makePanel()
		self.panel = panel
		if panel.parent !== window {
			panel.parent?.removeChildWindow(panel)
			window.addChildWindow(panel, ordered: .above)
		}
		place()
		panel.orderFront(nil)
	}

	private func hide() {
		guard let panel else { return }
		panel.parent?.removeChildWindow(panel)
		panel.orderOut(nil)
	}

	/// Over the emulation window's content (not its toolbar).
	private func place() {
		guard let panel, let window, panel.parent != nil else { return }
		let content = window.contentLayoutRect
		panel.setFrame(window.convertToScreen(content), display: true)
	}

	private func makePanel() -> NSPanel {
		let panel = NSPanel(contentRect: NSRect(x: 0, y: 0, width: 640, height: 400),
			styleMask: [.borderless, .nonactivatingPanel], backing: .buffered, defer: true)
		panel.isFloatingPanel = false
		panel.becomesKeyOnlyIfNeeded = true
		panel.ignoresMouseEvents = true
		panel.hidesOnDeactivate = false
		panel.isReleasedWhenClosed = false
		panel.backgroundColor = .clear
		panel.isOpaque = false
		panel.hasShadow = false
		panel.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
		panel.contentView = NSHostingView(rootView: PausedBadge())
		return panel
	}
}

/// The dimmed picture and the badge.
struct PausedBadge: View {
	var body: some View {
		ZStack {
			Color.black.opacity(0.35)
			VStack(spacing: 6) {
				Label("Paused", systemImage: "pause.fill")
					.font(.title2.weight(.semibold))
				Text("Press \u{2303}\u{2318}P or Resume to continue")
					.font(.callout)
					.foregroundStyle(.secondary)
			}
			.padding(.horizontal, 22)
			.padding(.vertical, 14)
			.background(.regularMaterial, in: RoundedRectangle(cornerRadius: 14))
			.environment(\.colorScheme, .dark)
		}
		.accessibilityElement(children: .combine)
		.accessibilityLabel("Paused")
	}
}
