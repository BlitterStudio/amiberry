/*
 * Copyright (C) 2026 BlitterStudio
 * The on-screen Amiga keyboard on the Mac: the shared keyboard in a panel
 * along the bottom of the emulation window. The panel never becomes the key
 * window, so typed keys keep going to the emulation; its keys are clicked.
 */

import AppKit
import SwiftUI

@MainActor
final class MacKeyboard: NSObject {
	static let shared = MacKeyboard()

	let model = AmigaKeyboardModel()
	private var panel: NSPanel?
	private weak var window: NSWindow?
	private var resizeObserver: NSObjectProtocol?

	var isVisible: Bool { panel?.isVisible == true }

	/// The emulation window SDL created; the keyboard goes along its bottom.
	func attach(to window: NSWindow) {
		hide()
		self.window = window
		if let resizeObserver {
			NotificationCenter.default.removeObserver(resizeObserver)
		}
		resizeObserver = NotificationCenter.default.addObserver(
			forName: NSWindow.didResizeNotification, object: window, queue: .main
		) { _ in
			MainActor.assumeIsolated { MacKeyboard.shared.place() }
		}
	}

	/// The keyboard hotkey and the on-screen joystick's keyboard button. As
	/// on iOS, it is there when the configuration enables it
	/// (amiberry.vkbd_enabled).
	func toggle() {
		if isVisible {
			hide()
		} else {
			show()
		}
	}

	func show() {
		guard let window, window.isVisible, EmulatorStatus.current().runState != .stopped else { return }
		let options = KeyboardOptions.active()
		guard options.enabled else { return }
		let panel = self.panel ?? makePanel()
		self.panel = panel
		panel.contentView = NSHostingView(rootView: AmigaKeyboardView(model: model, options: options, maxHeight: 240))
		window.addChildWindow(panel, ordered: .above)
		place()
		panel.orderFront(nil)
	}

	func hide() {
		model.releaseAll()
		guard let panel else { return }
		panel.parent?.removeChildWindow(panel)
		panel.orderOut(nil)
	}

	/// Along the bottom of the emulation window's content, its full width.
	private func place() {
		guard let panel, let window, panel.isVisible || panel.parent != nil else { return }
		let content = window.contentRect(forFrameRect: window.frame)
		let height = min(240, content.height / 2)
		panel.setFrame(NSRect(x: content.minX, y: content.minY, width: content.width, height: height), display: true)
	}

	private func makePanel() -> NSPanel {
		let panel = NSPanel(contentRect: NSRect(x: 0, y: 0, width: 640, height: 200),
			styleMask: [.borderless, .nonactivatingPanel], backing: .buffered, defer: true)
		panel.isFloatingPanel = true
		panel.becomesKeyOnlyIfNeeded = true
		panel.hidesOnDeactivate = false
		panel.isReleasedWhenClosed = false
		panel.backgroundColor = .clear
		panel.isOpaque = false
		panel.hasShadow = true
		panel.collectionBehavior = [.fullScreenAuxiliary, .moveToActiveSpace]
		return panel
	}
}
