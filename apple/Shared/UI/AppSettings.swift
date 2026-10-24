/*
 * Copyright (C) 2026 BlitterStudio
 * The app settings: the emulator's own settings (amiberry.conf), not a
 * machine's. They apply at once and are saved as they change. The Mac
 * shows them in the Settings window (Cmd-,), iOS from the Library.
 */

import SwiftUI

@MainActor
@Observable
final class AppSettingsModel {
	/// Every host setting's value, by ID.
	private(set) var values: [String: String] = [:]
	/// What went wrong with the last change, shown under the settings.
	var notice: String?

	init() {
		reload()
	}

	func value(of id: String) -> String {
		values[id] ?? ""
	}

	func reload() {
		var read: [String: String] = [:]
		for setting in HostSettings.all {
			read[setting.id] = (try? HostSettings.value(of: setting.id)) ?? ""
		}
		values = read
	}

	/// Sets a setting and saves amiberry.conf; false (with a notice) when
	/// the value was refused or the file could not be written.
	@discardableResult
	func set(_ value: String, for setting: HostSetting) -> Bool {
		do {
			try HostSettings.set(value, for: setting.id)
		} catch {
			notice = AppSettingsModel.describe(error, setting: setting)
			reload()
			return false
		}
		reload()
		guard HostSettings.save() else {
			notice = "The settings could not be saved."
			return false
		}
		notice = nil
		return true
	}

	static func describe(_ error: Error, setting: HostSetting) -> String {
		switch error as? SettingsError {
		case let .invalidValue(_, value):
			return value.isEmpty ? "\(setting.label): not a valid value."
				: "\(setting.label): \u{201C}\(value)\u{201D} is not a valid value."
		case .unavailable:
			return "\(setting.label): the folder could not be kept accessible."
		default:
			return "\(setting.label) could not be changed."
		}
	}

	/// The groups in the order the layer lists them, with their titles.
	static let groups: [(id: String, title: String)] = {
		var seen: [String] = []
		for setting in HostSettings.all where !seen.contains(setting.group) {
			seen.append(setting.group)
		}
		return seen.map { ($0, title(of: $0)) }
	}()

	static func title(of group: String) -> String {
		switch group {
		case "global_startup": return "Startup"
		case "global_input": return "Input Defaults"
		case "global_display": return "Display Defaults"
		case "global_sound": return "Sound Defaults"
		case "global_whdload": return "WHDLoad"
		case "global_touch": return "Touch Controls"
		case "global_visual": return "Appearance"
		case "global_paths": return "Folders & Logging"
		case "global_updates": return "Updates"
		case "global_advanced": return "Advanced"
		default: return SettingsPage.groupTitle(group)
		}
	}
}

struct AppSettingsView: View {
	@Bindable var model: AppSettingsModel

	var body: some View {
		Form {
			if let notice = model.notice {
				Section {
					Text(notice).foregroundStyle(.red)
				}
			}
			ForEach(AppSettingsModel.groups, id: \.id) { group in
				Section(group.title) {
					ForEach(HostSettings.all.filter { $0.group == group.id }) { setting in
						HostSettingRow(setting: setting, model: model)
					}
				}
			}
		}
		.formStyle(.grouped)
		.onAppear { model.reload() }
	}
}

struct HostSettingRow: View {
	let setting: HostSetting
	let model: AppSettingsModel
	@State private var draft = ""
	@FocusState private var editing: Bool

	var body: some View {
		control.help(setting.descriptor.help)
	}

	private var value: String { model.value(of: setting.id) }

	@ViewBuilder private var control: some View {
		switch setting.kind {
		case .bool:
			Toggle(setting.label, isOn: Binding(get: { value == "true" }, set: { model.set($0 ? "true" : "false", for: setting) }))
		case .choice:
			let current = value
			let choices = setting.descriptor.choices.contains { $0.value == current } || current.isEmpty
				? setting.descriptor.choices : [SettingChoice(value: current, label: current)] + setting.descriptor.choices
			Picker(setting.label, selection: Binding(get: { current }, set: { model.set($0, for: setting) })) {
				ForEach(choices) { Text($0.label).tag($0.value) }
			}
		case .integer, .real:
			let range = setting.descriptor.range ?? 0...0
			let number = Double(value) ?? range.lowerBound
			LabeledContent(setting.label) {
				HStack(spacing: 6) {
					draftField
						.multilineTextAlignment(.trailing)
						.frame(width: 90)
					Stepper(setting.label, value: Binding(get: { number }, set: {
						model.set(text(of: min(max($0, range.lowerBound), range.upperBound)), for: setting)
					}), in: range, step: setting.descriptor.step ?? 1)
					.labelsHidden()
				}
			}
		case .path, .string:
			LabeledContent(setting.label) {
				HStack(spacing: 6) {
					draftField
					if setting.kind == .path {
						Button("Choose\u{2026}") { choose() }
					}
				}
			}
		}
	}

	/// Typed text applies when it is submitted or the field is left, not as
	/// each letter is typed (a half-typed folder must not become the
	/// emulator's folder), and is not lost when the settings close.
	private var draftField: some View {
		TextField(setting.label, text: $draft)
			.labelsHidden()
			.focused($editing)
			.onSubmit { commitDraft() }
			.onChange(of: editing) { if !editing { commitDraft() } }
			.onDisappear { commitDraft() }
			.onAppear { draft = value }
			.onChange(of: value) { if !editing { draft = value } }
	}

	private func text(of number: Double) -> String {
		setting.kind == .integer ? String(Int(number)) : String(format: "%g", number)
	}

	private func commitDraft() {
		if draft != value, !model.set(draft, for: setting) {
			draft = value
		}
	}

	private func choose() {
		let path: String?
		switch setting.id {
		case "retroarch_config":
			path = FilePanels.choose(.file, title: setting.label, startingAt: draft)
		case "logfile_path":
			// The log is written, not opened: a folder for it, keeping the
			// file name.
			let name = draft.isEmpty ? "amiberry.log" : (draft as NSString).lastPathComponent
			path = FilePanels.chooseFolder(title: setting.label, startingAt: (draft as NSString).deletingLastPathComponent)
				.map { ($0 as NSString).appendingPathComponent(name) }
		default:
			path = FilePanels.chooseFolder(title: setting.label, startingAt: draft)
		}
		if let path, model.set(path, for: setting) {
			draft = path
		}
	}
}
