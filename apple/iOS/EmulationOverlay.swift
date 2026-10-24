/*
 * Copyright (C) 2026 BlitterStudio
 * The native layer over the emulation on iOS: a window above SDL's that
 * passes touches through to the emulation except where its own controls
 * are. It holds the settings and keyboard buttons shown while an emulation
 * runs, the on-screen Amiga keyboard, and presents the settings screen,
 * alerts and file pickers.
 */

import SwiftUI
import UIKit

@MainActor
@Observable
final class OverlayState {
	var keyboardVisible = false
	var settingsVisible = false
	/// Re-read when the settings screen closes, where they change.
	var keyboardOptions = KeyboardOptions()
	let status = StatusMonitor()
	/// Where the overlay's controls are, in window coordinates: the window
	/// keeps the touches there and passes the rest to the emulation.
	@ObservationIgnored var controlFrames: [String: CGRect] = [:]

	/// The emulation runs (or is paused) under nothing else.
	var showsControls: Bool { status.status.runState != .stopped && !settingsVisible }
}

@MainActor
final class EmulationOverlay {
	static let shared = EmulationOverlay()

	let state = OverlayState()
	let keyboard = AmigaKeyboardModel()
	private var window: PassthroughWindow?

	/// Creates the overlay window once SDL has started the application.
	func install() {
		guard window == nil else { return }
		let window: PassthroughWindow
		if let scene = activeScene() {
			window = PassthroughWindow(windowScene: scene)
		} else {
			window = PassthroughWindow(frame: UIScreen.main.bounds)
		}
		window.windowLevel = .normal + 1
		window.backgroundColor = .clear
		window.keepsTouch = { [state] point in state.controlFrames.values.contains { $0.contains(point) } }
		let host = UIHostingController(rootView: OverlayView(state: state, keyboard: keyboard))
		host.view.backgroundColor = .clear
		host.view.isMultipleTouchEnabled = true
		window.rootViewController = host
		window.isHidden = false
		self.window = window
		state.status.start()
		state.keyboardOptions = KeyboardOptions.active()
	}

	/// The controller to present alerts and pickers from: the topmost one
	/// presented over the overlay, once one being dismissed (a picker that
	/// just answered) has gone, as UIKit presents from no other.
	func presenter() -> UIViewController? {
		install()
		var top = window?.rootViewController
		while let presented = top?.presentedViewController {
			if presented.isBeingDismissed {
				let leaving = presented
				let deadline = Date(timeIntervalSinceNow: 2)
				while leaving.presentingViewController != nil, Date() < deadline {
					_ = RunLoop.current.run(mode: .default, before: Date(timeIntervalSinceNow: 0.02))
				}
				break
			}
			top = presented
		}
		return top
	}

	func presentSettings(_ controller: UIViewController) -> Bool {
		hideKeyboard()
		guard let presenter = presenter() else { return false }
		presenter.present(controller, animated: false)
		guard controller.presentingViewController != nil else { return false }
		// Its text fields need the key window.
		window?.makeKey()
		state.settingsVisible = true
		return true
	}

	func dismissSettings(_ controller: UIViewController?) {
		// From the controller that presented it: dismissing the screen
		// itself would only close what it presents (the editor over the
		// Library) and leave the screen up.
		(controller?.presentingViewController ?? controller)?.dismiss(animated: false)
		state.settingsVisible = false
		state.keyboardOptions = KeyboardOptions.active()
		// Hardware keys go back to the emulation.
		emulationWindow()?.makeKey()
	}

	func toggleKeyboard() {
		if state.keyboardVisible {
			hideKeyboard()
		} else if state.showsControls {
			state.keyboardOptions = KeyboardOptions.active()
			state.keyboardVisible = state.keyboardOptions.enabled
		}
	}

	func hideKeyboard() {
		keyboard.releaseAll()
		state.keyboardVisible = false
	}

	private func activeScene() -> UIWindowScene? {
		let scenes = UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }
		return scenes.first { $0.activationState == .foregroundActive } ?? scenes.first
	}

	/// SDL's window.
	private func emulationWindow() -> UIWindow? {
		let windows = window?.windowScene?.windows ?? UIApplication.shared.connectedScenes
			.compactMap { $0 as? UIWindowScene }.flatMap(\.windows)
		return windows.first { $0 !== window && !$0.isHidden }
	}
}

/// Hands touches that hit none of its controls to the window below (the
/// emulation's).
final class PassthroughWindow: UIWindow {
	/// Whether a point is on one of the overlay's SwiftUI controls, which
	/// have no views of their own to hit.
	var keepsTouch: (CGPoint) -> Bool = { _ in false }

	override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
		guard let hit = super.hitTest(point, with: event) else { return nil }
		guard let root = rootViewController?.view else { return hit }
		// Presented screens (settings, alerts, pickers) take their touches.
		guard hit.isDescendant(of: root) else { return hit }
		// SwiftUI answers for its whole hosting view; only a control under
		// the point keeps the touch.
		for subview in root.subviews.reversed() where subview.hitTest(subview.convert(point, from: root), with: event) != nil {
			return hit
		}
		// SwiftUI draws its buttons without views of their own: the hosting
		// view answers for them, so the controls' frames decide.
		if keepsTouch(point) {
			return hit
		}
		return hit === root ? nil : hit
	}
}

struct OverlayView: View {
	let state: OverlayState
	let keyboard: AmigaKeyboardModel

	var body: some View {
		ZStack(alignment: .topTrailing) {
			Color.clear
				.allowsHitTesting(false)
			if state.showsControls {
				HStack(spacing: 12) {
					// As the built-in keyboard: there when the configuration
					// enables it (amiberry.vkbd_enabled).
					if state.keyboardOptions.enabled {
						OverlayButton(symbol: "keyboard", label: "Keyboard", active: state.keyboardVisible) {
							EmulationOverlay.shared.toggleKeyboard()
						}
					}
					OverlayButton(symbol: "gearshape", label: "Settings", active: false) {
						EmulationOverlay.shared.hideKeyboard()
						_ = EmulatorActions.post(.openSettings)
					}
				}
				.overlayControl("buttons", state: state)
				.padding(.top, 8)
				.padding(.trailing, 12)
			}
			if state.showsControls, state.keyboardVisible {
				VStack {
					Spacer()
					AmigaKeyboardView(model: keyboard, options: state.keyboardOptions)
						.overlayControl("keyboard", state: state)
				}
				.transition(.move(edge: .bottom))
			}
		}
		.ignoresSafeArea(.keyboard)
		.onAppear { keyboard.syncCapsLock(state.status.status.capsLock) }
		.onChange(of: state.status.status.capsLock) { keyboard.syncCapsLock(state.status.status.capsLock) }
		// The emulation resuming (or starting) follows a settings apply, which
		// runs after the settings screen closes: the options are read again.
		.onChange(of: state.status.status.runState) { state.keyboardOptions = KeyboardOptions.active() }
	}
}

extension View {
	/// Marks a control of the overlay, so the window keeps the touches on it.
	func overlayControl(_ id: String, state: OverlayState) -> some View {
		onGeometryChange(for: CGRect.self) { $0.frame(in: .global) } action: { state.controlFrames[id] = $0 }
			.onDisappear { state.controlFrames[id] = nil }
	}
}

private struct OverlayButton: View {
	let symbol: String
	let label: String
	let active: Bool
	let action: () -> Void

	var body: some View {
		Button(action: action) {
			Image(systemName: symbol)
				.font(.system(size: 17, weight: .semibold))
				.frame(width: 44, height: 44)
				.foregroundStyle(active ? Color.accentColor : Color.white)
				.background(.ultraThinMaterial, in: Circle())
		}
		.opacity(0.75)
		.accessibilityLabel(label)
	}
}
