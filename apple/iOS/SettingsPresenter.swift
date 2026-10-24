/*
 * Copyright (C) 2026 BlitterStudio
 * The iOS screens the emulator shows in place of the built-in GUI: the
 * Library (LibraryView in apple/Shared/UI) before an emulation starts, with
 * the editor (SettingsRootView) presented over it, and the editor alone
 * over a running emulation. The macOS counterpart is
 * apple/macOS/SettingsWindow.swift.
 */

import SwiftUI
import UIKit

/// The editor shown over the Library, when there is one.
@MainActor
@Observable
final class EditorSlot {
	var model: SettingsWindowModel?
}

/// The Library with the editor presented over it.
struct LibraryHostView: View {
	let library: LibraryModel
	@Bindable var slot: EditorSlot

	var body: some View {
		LibraryView(model: library)
			.fullScreenCover(isPresented: Binding(get: { slot.model != nil }, set: { if !$0 { slot.model = nil } })) {
				if let model = slot.model {
					SettingsRootView(model: model)
				}
			}
	}
}

@MainActor
final class SettingsPresenter {
	static let shared = SettingsPresenter()

	private var controller: UIViewController?
	private let slot = EditorSlot()
	private var library: LibraryModel?
	private var editorModel: SettingsWindowModel?

	/// The editor's model, over the Library or the emulation.
	var model: SettingsWindowModel? { editorModel ?? slot.model }

	/// Whether a screen is presented; the emulator waits until it is
	/// dismissed.
	var isVisible: Bool { controller != nil }

	func show(emulating: Bool) {
		if emulating {
			showEditor()
		} else {
			showLibrary()
		}
	}

	private func showLibrary() {
		// A load that completes after the Library closed acts on nothing.
		let library = LibraryModel(
			emulating: false,
			finish: { [weak self] in
				guard self?.library != nil else { return }
				self?.dismiss()
			},
			openEditor: { [weak self] machine, preset in
				guard self?.library != nil else { return }
				self?.openEditor(machine: machine, preset: preset)
			})
		let controller = UIHostingController(rootView: LibraryHostView(library: library, slot: slot))
		controller.modalPresentationStyle = .fullScreen
		controller.isModalInPresentation = true
		guard EmulationOverlay.shared.presentSettings(controller) else {
			return
		}
		self.library = library
		self.controller = controller
	}

	private func openEditor(machine: Machine?, preset: (id: String, configuration: Int)?) {
		do {
			let model = try SettingsWindowModel(
				emulating: false, dismiss: { [weak self] in self?.dismiss() }, machine: machine,
				closeToLibrary: { [weak self] in
					self?.slot.model = nil
					self?.library?.reload()
				})
			if let preset {
				_ = model.store.applyPreset(preset.id, configuration: preset.configuration)
				model.page = .quickStart
			}
			slot.model = model
		} catch {
			Alerts.show(title: "Settings", message: SettingsWindowModel.describe(error), buttons: ["OK"])
		}
	}

	private func showEditor() {
		let model: SettingsWindowModel
		do {
			model = try SettingsWindowModel(emulating: true, dismiss: { [weak self] in self?.dismiss() })
		} catch {
			Alerts.show(title: "Settings", message: SettingsWindowModel.describe(error), buttons: ["OK"])
			return
		}
		let controller = UIHostingController(rootView: SettingsRootView(model: model))
		controller.modalPresentationStyle = .fullScreen
		// Closed by its Resume button, which commits the session.
		controller.isModalInPresentation = true
		guard EmulationOverlay.shared.presentSettings(controller) else {
			return
		}
		self.controller = controller
		editorModel = model
	}

	func dismiss() {
		let controller = self.controller
		self.controller = nil
		editorModel = nil
		slot.model = nil
		library = nil
		EmulationOverlay.shared.dismissSettings(controller)
	}
}
