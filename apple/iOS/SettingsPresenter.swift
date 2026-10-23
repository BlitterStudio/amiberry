/*
 * Copyright (C) 2026 BlitterStudio
 * The iOS settings screen, shown by the emulator in place of the built-in
 * GUI (SettingsRootView in apple/Shared/UI) over the emulation. The macOS
 * counterpart is apple/macOS/SettingsWindow.swift.
 */

import SwiftUI
import UIKit

@MainActor
final class SettingsPresenter {
	static let shared = SettingsPresenter()

	private var controller: UIViewController?
	private(set) var model: SettingsWindowModel?

	/// Whether the screen is presented; the emulator waits until it is
	/// dismissed.
	var isVisible: Bool { model != nil }

	func show(emulating: Bool) {
		let model: SettingsWindowModel
		do {
			model = try SettingsWindowModel(emulating: emulating, dismiss: { [weak self] in self?.dismiss() })
		} catch {
			Alerts.show(title: "Settings", message: SettingsWindowModel.describe(error), buttons: ["OK"])
			return
		}
		let controller = UIHostingController(rootView: SettingsRootView(model: model))
		controller.modalPresentationStyle = .fullScreen
		// Closed by its Start or Resume button, which commit the session.
		controller.isModalInPresentation = true
		guard EmulationOverlay.shared.presentSettings(controller) else {
			return
		}
		self.controller = controller
		self.model = model
	}

	func dismiss() {
		let controller = self.controller
		self.controller = nil
		model = nil
		EmulationOverlay.shared.dismissSettings(controller)
	}
}
