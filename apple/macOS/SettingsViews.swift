/*
 * Copyright (C) 2026 BlitterStudio
 * The settings window's views. Every control is generated from a layer
 * descriptor, and availability, ranges and choices come from the session,
 * so no option rule is written here.
 */

import AppKit
import SwiftUI

struct SettingsRootView: View {
	@Bindable var model: SettingsWindowModel
	@State private var search = ""

	var body: some View {
		NavigationSplitView {
			List(selection: $model.page) {
				ForEach(SettingsPage.sections(), id: \.title) { section in
					Section(section.title) {
						ForEach(section.pages) { page in
							Label(page.title, systemImage: page.symbol).tag(page)
						}
					}
				}
			}
			.navigationSplitViewColumnWidth(min: 190, ideal: 210)
		} detail: {
			if search.trimmingCharacters(in: .whitespaces).isEmpty {
				PageView(page: model.page, model: model)
					.navigationTitle(model.page.title)
			} else {
				SearchResultsView(query: search, store: model.store)
					.navigationTitle("Search")
			}
		}
		.searchable(text: $search, placement: .sidebar, prompt: "Search all settings")
		.safeAreaInset(edge: .bottom, spacing: 0) {
			FooterView(model: model)
		}
		.environment(model)
		.frame(minWidth: 720, minHeight: 480)
	}
}

struct PageView: View {
	let page: SettingsPage
	let model: SettingsWindowModel

	var body: some View {
		switch page {
		case .quickStart:
			QuickStartPage(model: model)
		case let .group(id):
			GroupPage(group: id, store: model.store)
		case .hardDrives:
			HardDrivesPage(model: model)
		case .paths:
			PathsPage(model: model)
		}
	}
}

/// Every option whose label, help or key matches the query, by group.
struct SearchResultsView: View {
	let query: String
	let store: SettingsStore

	static func matches(_ query: String) -> [SettingDescriptor] {
		let terms = query.lowercased().split(separator: " ").map(String.init)
		return SettingsCatalog.descriptors.filter { descriptor in
			let text = "\(descriptor.label) \(descriptor.help) \(descriptor.id)".lowercased()
			return terms.allSatisfy(text.contains)
		}
	}

	var body: some View {
		let results = SearchResultsView.matches(query)
		Form {
			if results.isEmpty {
				Text("No settings match \u{201C}\(query)\u{201D}.").foregroundStyle(.secondary)
			}
			ForEach(SettingsCatalog.groups.filter { group in results.contains { $0.group == group } }, id: \.self) { group in
				Section(SettingsPage.groupTitle(group)) {
					ForEach(results.filter { $0.group == group }) { descriptor in
						OptionRow(descriptor: descriptor, store: store)
					}
				}
			}
		}
		.formStyle(.grouped)
	}
}

/// Every option of a layer group, in the layer's order.
struct GroupPage: View {
	let group: String
	let store: SettingsStore

	var body: some View {
		Form {
			ForEach(SettingsCatalog.descriptors(in: group)) { descriptor in
				OptionRow(descriptor: descriptor, store: store)
			}
		}
		.formStyle(.grouped)
	}
}

/// One option, with the control its type calls for.
struct OptionRow: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore

	var body: some View {
		let available = store.isAvailable(descriptor.id)
		VStack(alignment: .leading, spacing: 4) {
			control
				.disabled(!available)
			if !available, let constraint = descriptor.constraint {
				Text(constraint).font(.caption).foregroundStyle(.secondary)
			}
		}
		.help(descriptor.help)
	}

	private var value: String { store.value(of: descriptor.id) ?? "" }

	private func set(_ newValue: String) {
		store.set(newValue, for: descriptor.id)
	}

	@ViewBuilder private var control: some View {
		switch descriptor.kind {
		case .bool:
			Toggle(descriptor.label, isOn: Binding(get: { value == "true" }, set: { set($0 ? "true" : "false") }))
		case .choice:
			ChoiceControl(descriptor: descriptor, store: store)
		case .integer, .real:
			NumberControl(descriptor: descriptor, store: store)
		case .path, .string:
			TextControl(descriptor: descriptor, store: store)
		}
	}
}

struct ChoiceControl: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore

	var body: some View {
		let value = store.value(of: descriptor.id) ?? ""
		var choices = store.choices(for: descriptor.id)
		// A configuration file can hold a value outside the list (a device
		// that is gone); it still shows as selected.
		if !value.isEmpty, !choices.contains(where: { $0.value == value }) {
			choices.insert(SettingChoice(value: value, label: value), at: 0)
		}
		return Picker(descriptor.label, selection: Binding(get: { value }, set: { store.set($0, for: descriptor.id) })) {
			ForEach(choices) { choice in
				Text(choice.label).tag(choice.value)
					.disabled(choice.value != value && !store.isChoiceAvailable(choice.value, for: descriptor.id))
			}
		}
	}
}

/// An integer or real option. Typed text is stored while it reads as a
/// number in range, so nothing typed is lost when the window closes; text
/// that does not puts back the value the field started with, and shows why.
struct NumberControl: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore
	@Environment(SettingsWindowModel.self) private var model: SettingsWindowModel?
	@State private var draft = ""
	@State private var valueBeforeEditing: String?
	@FocusState private var editing: Bool

	var body: some View {
		let range = store.range(for: descriptor.id) ?? descriptor.range ?? 0...0
		let current = Double(store.value(of: descriptor.id) ?? "") ?? range.lowerBound
		let invalid = number(draft, in: range) == nil
		VStack(alignment: .trailing, spacing: 2) {
			LabeledContent(descriptor.label) {
				HStack(spacing: 6) {
					TextField(descriptor.label, text: $draft)
						.labelsHidden()
						.multilineTextAlignment(.trailing)
						.frame(width: 90)
						.focused($editing)
						.foregroundStyle(invalid ? Color.red : Color.primary)
					Stepper(
						descriptor.label,
						value: Binding(get: { current }, set: { save(min(max($0, range.lowerBound), range.upperBound)) }),
						in: range, step: max(descriptor.step ?? 1, 0.0001)
					)
					.labelsHidden()
				}
			}
			if invalid {
				Text("Enter a number from \(format(range.lowerBound)) to \(format(range.upperBound)).")
					.font(.caption).foregroundStyle(.red)
			}
		}
		.onAppear {
			valueBeforeEditing = store.value(of: descriptor.id)
			draft = model?.invalidDrafts[descriptor.id] ?? store.value(of: descriptor.id) ?? ""
		}
		.onChange(of: store.revision) {
			let stored = store.value(of: descriptor.id) ?? ""
			if !editing, model?.invalidDrafts[descriptor.id] == nil, Double(draft) != Double(stored) {
				draft = stored
			}
		}
		.onChange(of: editing) {
			if editing, model?.invalidDrafts[descriptor.id] == nil {
				valueBeforeEditing = store.value(of: descriptor.id)
			}
		}
		.onChange(of: draft) {
			if let value = number(draft, in: range), save(value) {
				model?.invalidDrafts[descriptor.id] = nil
			} else {
				if let original = valueBeforeEditing, original != store.value(of: descriptor.id) {
					store.set(original, for: descriptor.id)
				}
				model?.invalidDrafts[descriptor.id] = draft
			}
		}
	}

	/// The draft as a value of this option, or nil: a number in range, and
	/// a whole one for an integer option (no rounding behind the user's back).
	private func number(_ text: String, in range: ClosedRange<Double>) -> Double? {
		guard let value = Double(text.trimmingCharacters(in: .whitespaces)), range.contains(value),
			descriptor.kind != .integer || value.rounded() == value
		else { return nil }
		return value
	}

	private func format(_ number: Double) -> String {
		descriptor.kind == .integer ? String(Int(number.rounded())) : String(number)
	}

	/// False when the layer refuses the value (for example one off its step).
	@discardableResult
	private func save(_ number: Double) -> Bool {
		let text = format(number)
		guard Double(text) != Double(store.value(of: descriptor.id) ?? "") else { return true }
		return store.set(text, for: descriptor.id)
	}
}

/// A path or literal value, stored as it is typed.
struct TextControl: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore
	@Environment(SettingsWindowModel.self) private var model: SettingsWindowModel?
	@State private var draft = ""

	var body: some View {
		LabeledContent(descriptor.label) {
			HStack(spacing: 6) {
				TextField(descriptor.label, text: $draft)
					.labelsHidden()
				if descriptor.kind == .path {
					Button("Choose\u{2026}") {
						if let path = FilePanels.choose(.floppy, title: descriptor.label, startingAt: draft) {
							draft = path
						}
					}
				}
			}
		}
		.onAppear { draft = model?.invalidDrafts[descriptor.id] ?? store.value(of: descriptor.id) ?? "" }
		.onChange(of: store.revision) {
			let stored = store.value(of: descriptor.id) ?? ""
			if draft != stored, model?.invalidDrafts[descriptor.id] == nil {
				draft = stored
			}
		}
		.onChange(of: draft) {
			// Text the layer refuses stays in the field and keeps the window
			// open until it is corrected.
			if draft == store.value(of: descriptor.id) || store.set(draft, for: descriptor.id) {
				model?.invalidDrafts[descriptor.id] = nil
			} else {
				model?.invalidDrafts[descriptor.id] = draft
			}
		}
	}
}

struct QuickStartPage: View {
	let model: SettingsWindowModel
	@State private var presetID = MachinePresets.all.first?.id ?? ""
	@State private var configuration = 0
	@State private var compatibility = 1

	var body: some View {
		let preset = MachinePresets.preset(for: presetID)
		Form {
			Section {
				Picker("Model", selection: $presetID) {
					ForEach(MachinePresets.all) { preset in
						Text(preset.label).tag(preset.id)
					}
				}
				.onChange(of: presetID) {
					configuration = 0
					compatibility = MachinePresets.defaultCompatibility(for: presetID)
				}
				if let preset {
					Picker("Configuration", selection: $configuration) {
						ForEach(Array(preset.configurations.enumerated()), id: \.offset) { index, label in
							Text(label).tag(index)
						}
					}
					Picker("Compatibility", selection: $compatibility) {
						ForEach(0...max(preset.compatibilityMax, 0), id: \.self) { level in
							if level < MachinePresets.compatibilityLevels.count {
								Text(MachinePresets.compatibilityLevels[level]).tag(level)
							}
						}
					}
					if !MachinePresets.isInstalled(preset, configuration: configuration) {
						Label("The ROMs this configuration needs are not installed.", systemImage: "exclamationmark.triangle")
							.foregroundStyle(.secondary)
					}
				}
			} footer: {
				Text("Applying a model replaces the machine settings with the model's, as the built-in Quickstart does.")
			}
			Section {
				Button("Apply Model") {
					model.store.applyPreset(presetID, configuration: configuration, compatibility: compatibility)
				}
				.disabled(preset == nil)
			}
		}
		.formStyle(.grouped)
		.onAppear { compatibility = MachinePresets.defaultCompatibility(for: presetID) }
	}
}

struct HardDrivesPage: View {
	let model: SettingsWindowModel

	var body: some View {
		let mounts = model.store.mounts
		Form {
			Section("Mounted drives") {
				if mounts.isEmpty {
					Text("No drives are mounted.").foregroundStyle(.secondary)
				}
				ForEach(Array(mounts.enumerated()), id: \.offset) { index, entry in
					HStack {
						Text(entry).font(.system(.body, design: .monospaced)).lineLimit(1).truncationMode(.middle)
						Spacer()
						Button("Remove") { model.store.removeMount(at: index) }
					}
				}
			}
			Section {
				HStack {
					Button("Add Folder\u{2026}") { addFolder() }
					Button("Add Hardfile\u{2026}") { addHardfile() }
				}
			} footer: {
				Text("Drives attach when the emulation starts or resets.")
			}
		}
		.formStyle(.grouped)
	}

	/// The first DHn device name the mounts do not use.
	private func freeDevice() -> String {
		let used = Set(model.store.mounts.compactMap { entry -> String? in
			entry.split(separator: ",").dropFirst(2).first?.split(separator: ":").first.map { $0.uppercased() }
		})
		var number = 0
		while used.contains("DH\(number)") {
			number += 1
		}
		return "DH\(number)"
	}

	/// A uaehfN path field: quoted when it holds a comma or starts or ends
	/// with a space, as the configuration writer does. nil for a path with a
	/// double quote, which a mount entry cannot hold (its path field is read
	/// without escapes, so a quote ends a quoted path).
	static func mountField(_ path: String) -> String? {
		guard !path.contains("\"") else { return nil }
		guard path.contains(",") || path.hasPrefix(" ") || path.hasSuffix(" ") else {
			return path
		}
		return "\"" + path + "\""
	}

	private func add(_ entry: (String) -> String, path: String) {
		guard let field = HardDrivesPage.mountField(path) else {
			model.notice = "\u{201C}\(path)\u{201D} cannot be mounted: a mounted path cannot contain a double quote."
			return
		}
		model.notice = nil
		model.store.addMount(entry(field))
	}

	private func addFolder() {
		guard let path = FilePanels.chooseFolder(title: "Choose a folder to mount", startingAt: "") else { return }
		let volume = URL(fileURLWithPath: path).lastPathComponent
			.replacingOccurrences(of: ":", with: "_").replacingOccurrences(of: ",", with: "_")
		let device = freeDevice()
		add({ "dir,rw,\(device):\(volume.isEmpty ? "Work" : volume):\($0),0" }, path: path)
	}

	private func addHardfile() {
		guard let path = FilePanels.choose(.floppy, title: "Choose a hardfile to mount", startingAt: "") else { return }
		let device = freeDevice()
		add({ "hdf,rw,\(device):\($0),32,1,2,512,0,,uae0" }, path: path)
	}
}

struct PathsPage: View {
	let model: SettingsWindowModel
	@State private var revision = 0

	var body: some View {
		Form {
			Section {
				ForEach(MediaFolder.allCases, id: \.self) { folder in
					let path = (try? MediaLibrary.path(of: folder)) ?? ""
					LabeledContent(PathsPage.title(folder)) {
						HStack {
							Text(path).lineLimit(1).truncationMode(.middle).foregroundStyle(.secondary)
							Button("Choose\u{2026}") { choose(folder, from: path) }
						}
					}
				}
			} footer: {
				Text("Folders are saved at once, for every configuration.")
			}
		}
		.formStyle(.grouped)
		.id(revision)
	}

	static func title(_ folder: MediaFolder) -> String {
		switch folder {
		case .roms: return "ROMs"
		case .floppies: return "Floppies"
		case .cds: return "CD images"
		case .hardDrives: return "Hard drives"
		case .configurations: return "Configurations"
		case .saveStates: return "Save states"
		case .screenshots: return "Screenshots"
		case .whdloadArchives: return "WHDLoad archives"
		}
	}

	private func choose(_ folder: MediaFolder, from path: String) {
		guard let chosen = FilePanels.chooseFolder(title: "Choose the \(PathsPage.title(folder)) folder", startingAt: path)
		else { return }
		do {
			try MediaLibrary.setPath(chosen, for: folder)
			model.notice = nil
		} catch {
			model.notice = SettingsWindowModel.describe(error)
		}
		revision += 1
	}
}

struct FooterView: View {
	let model: SettingsWindowModel

	var body: some View {
		let waiting = model.waiting
		HStack(alignment: .center, spacing: 10) {
			VStack(alignment: .leading, spacing: 2) {
				if let error = model.store.lastError {
					Text(SettingsWindowModel.describe(error)).foregroundStyle(.red)
				} else if let notice = model.notice {
					Text(notice).foregroundStyle(.red)
				}
				ForEach(model.store.adjustments) { adjustment in
					let label = SettingsCatalog.descriptor(for: adjustment.optionID)?.label ?? adjustment.optionID
					Text("\(label) will be \(adjustment.adjusted)\(adjustment.reason.map { ": \($0)" } ?? "")")
						.foregroundStyle(.orange)
				}
				if let reset = waiting[.reset], !reset.isEmpty {
					Text("Waiting for a reset: \(reset.map(\.label).joined(separator: ", "))").foregroundStyle(.secondary)
				}
				if let restart = waiting[.restart], !restart.isEmpty {
					Text("Waiting for a restart: \(restart.map(\.label).joined(separator: ", "))").foregroundStyle(.secondary)
				}
			}
			.font(.callout)
			.lineLimit(2)
			Spacer()
			Button("Quit") { model.quit() }
			if model.emulating {
				if !(waiting[.restart] ?? []).isEmpty {
					Button("Restart") { model.restart() }
				}
				Button("Reset") { model.reset() }
				Button("Resume") { model.startOrResume() }
					.keyboardShortcut(.defaultAction)
			} else {
				Button("Start") { model.startOrResume() }
					.keyboardShortcut(.defaultAction)
			}
		}
		.padding(.horizontal, 16)
		.padding(.vertical, 10)
		.background(.bar)
	}
}
