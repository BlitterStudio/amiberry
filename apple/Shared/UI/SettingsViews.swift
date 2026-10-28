/*
 * Copyright (C) 2026 BlitterStudio
 * The native settings views, shared by macOS and iOS. Every control is
 * generated from a layer descriptor, and availability, ranges and choices
 * come from the session, so no option rule is written here. File and folder
 * requesters are each platform's FilePanels.
 */

import SwiftUI

struct SettingsRootView: View {
	@Bindable var model: SettingsWindowModel
	@State private var search = ""
	/// The sidebar's selection: nil when a collapsed split view (iPhone)
	/// goes back to the list. The page shown stays model.page.
	@State private var selection: SettingsPage?

	var body: some View {
		NavigationSplitView {
			List(selection: $selection) {
				ForEach(SettingsPage.sidebar) { page in
					HStack {
						Label(page.title, systemImage: page.symbol)
						Spacer(minLength: 4)
						// A new machine is all model: only an existing one's
						// edits are worth pointing out page by page.
						if page != .advanced, !model.isNewMachine, page.isEdited(in: model.store) {
							Circle()
								.fill(Color.accentColor)
								.frame(width: 7, height: 7)
								.help("Changed since the settings opened")
								.accessibilityLabel("Edited")
						}
					}
					.tag(page)
				}
			}
			#if os(macOS)
			// As System Settings: the page list is the only way between pages,
			// so it cannot be collapsed away.
			.toolbar(removing: .sidebarToggle)
			// An iPad's sidebar keeps the system width: a narrower one wraps
			// "Display & Sound" and cuts the search field's prompt.
			.navigationSplitViewColumnWidth(min: 190, ideal: 210)
			#endif
		} detail: {
			Group {
				if search.trimmingCharacters(in: .whitespaces).isEmpty {
					PageView(page: model.page, model: model)
						#if os(iOS)
						.navigationTitle(model.page.title)
						#endif
				} else {
					SearchResultsView(query: search, store: model.store)
						#if os(iOS)
						.navigationTitle("Search")
						#endif
				}
			}
			#if os(macOS)
			// A Mac window is titled by its document: the machine, with the
			// page under it.
			.navigationTitle(model.title)
			.navigationSubtitle(search.trimmingCharacters(in: .whitespaces).isEmpty ? model.page.title : "Search")
			#endif
			.safeAreaInset(edge: .top, spacing: 0) {
				ApplyBanner(model: model)
			}
			#if os(macOS)
			// On the detail column, so the page's scroll view stops above the
			// buttons and the last rows of a long page stay reachable.
			.safeAreaInset(edge: .bottom, spacing: 0) {
				FooterView(model: model)
			}
			#endif
		}
		.searchable(text: $search, placement: .sidebar, prompt: "Search all settings")
		#if os(iOS)
		.safeAreaInset(edge: .bottom, spacing: 0) {
			FooterView(model: model)
		}
		#endif
		.environment(model)
		.onAppear { selection = model.page }
		.onChange(of: selection) {
			if let selection {
				model.page = selection
			}
		}
		#if os(macOS)
		.frame(minWidth: 720, minHeight: 480)
		#endif
	}
}

struct PageView: View {
	let page: SettingsPage
	let model: SettingsWindowModel

	var body: some View {
		// A page of its own identity: pages sharing a branch below must not
		// share its scroll position or state.
		content.id(page)
	}

	@ViewBuilder private var content: some View {
		switch page {
		case .overview:
			OverviewPage(model: model)
		case .displaySound, .input, .storage, .portsMisc:
			Form {
				DesignedSections(page: page, model: model)
			}
			.formStyle(.grouped)
		case .hardware:
			Form {
				GroupSections(groups: page.groups, store: model.store, hiddenWhenUnavailable: page.hiddenWhenUnavailable)
				BoardsSections(store: model.store)
			}
			.formStyle(.grouped)
		case .advanced:
			Form {
				GroupSections(groups: page.groups, store: model.store)
			}
			.formStyle(.grouped)
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
		if results.isEmpty {
			ContentUnavailableView.search(text: query)
		} else {
			Form {
				ForEach(SettingsCatalog.groups.filter { group in results.contains { $0.group == group } }, id: \.self) { group in
					OptionSection(title: SettingsPage.groupTitle(group), descriptors: results.filter { $0.group == group },
						store: store)
				}
			}
			.formStyle(.grouped)
		}
	}
}

/// A section per layer group, each with its options in the layer's order.
struct GroupSections: View {
	let groups: [String]
	let store: SettingsStore
	var hiddenWhenUnavailable = HiddenWhenUnavailable()

	var body: some View {
		ForEach(groups.filter { !SettingsCatalog.descriptors(in: $0).isEmpty }, id: \.self) { group in
			OptionSection(title: SettingsPage.groupTitle(group), descriptors: SettingsCatalog.descriptors(in: group),
				store: store, hiddenWhenUnavailable: hiddenWhenUnavailable)
		}
	}
}

/// A section of options. Options the machine cannot have now are dimmed,
/// and why is said once under the section rather than under each of them:
/// five serial options waiting for a serial port say so once. Options are
/// named in the section's words: "Image" under "DF0:", not "DF0: image".
/// The hidden ones that are unavailable do not show at all.
struct OptionSection: View {
	let title: String
	let descriptors: [SettingDescriptor]
	let store: SettingsStore
	var hiddenWhenUnavailable = HiddenWhenUnavailable()

	var body: some View {
		let shown = hiddenWhenUnavailable.shown(descriptors, available: { store.isAvailable($0) },
			value: { store.value(of: $0) })
			.map { $0.labelled(OptionSection.label($0.label, in: title)) }
		let notes = OptionSection.constraintNotes(shown) { store.isAvailable($0) }
		Section {
			ForEach(shown) { descriptor in
				OptionRow(descriptor: descriptor, store: store, showsConstraint: false)
			}
		} header: {
			Text(title)
		} footer: {
			if !notes.isEmpty {
				Text(notes.joined(separator: "\n"))
			}
		}
	}

	/// An option's label without the section's name it starts with: "DF0:
	/// image" under "DF0:" is "Image", "Port 1 device" under "Port 1" is
	/// "Device". A label that is the section's name, or only starts with
	/// the same letters ("Floppy drive…" under "Floppy Drives"), stays.
	static func label(_ label: String, in title: String) -> String {
		let section = title.hasSuffix(":") ? String(title.dropLast()) : title
		guard !section.isEmpty, label.count > section.count, label.lowercased().hasPrefix(section.lowercased()) else {
			return label
		}
		let after = label.dropFirst(section.count)
		guard after.first == " " || after.first == ":" else { return label }
		let rest = after.drop { $0 == " " || $0 == ":" }
		guard let initial = rest.first else { return label }
		return initial.uppercased() + rest.dropFirst()
	}

	/// Why the unavailable options are, each reason once in the order the
	/// options come: a reason only one option has names it, unless it
	/// starts with the option's name already.
	static func constraintNotes(_ descriptors: [SettingDescriptor], available: (String) -> Bool) -> [String] {
		var reasons: [String] = []
		var labels: [String: [String]] = [:]
		for descriptor in descriptors where !available(descriptor.id) {
			guard let reason = descriptor.constraint, !reason.isEmpty else { continue }
			if labels[reason] == nil {
				reasons.append(reason)
			}
			labels[reason, default: []].append(descriptor.label)
		}
		return reasons.map { reason in
			let named = labels[reason] ?? []
			guard named.count == 1, !reason.lowercased().hasPrefix(named[0].lowercased()) else { return reason }
			// "DF1 image: Needs…", not "DF1: image: Needs…".
			return "\(named[0].replacingOccurrences(of: ":", with: "")): \(reason)"
		}
	}
}

/// What defines the machine at a glance: its model, Kickstart, memory and
/// first drives, and its name in the Library. A new machine starts with
/// its model; an existing one with what it is, the model to start over
/// from coming last.
struct OverviewPage: View {
	let model: SettingsWindowModel

	var body: some View {
		Form {
			AutosaveSection(model: model)
			if model.isNewMachine {
				ModelSections(model: model)
			}
			OptionSection(title: "At a glance",
				descriptors: SettingsPage.overviewOptions.compactMap(SettingsCatalog.descriptor(for:)), store: model.store,
				hiddenWhenUnavailable: SettingsPage.overview.hiddenWhenUnavailable)
			GroupSections(groups: SettingsPage.overview.groups, store: model.store)
			if !model.isNewMachine {
				ModelSections(model: model)
			}
		}
		.formStyle(.grouped)
	}
}

/// The iOS app's last session, saved as it went to the background.
struct AutosaveSection: View {
	let model: SettingsWindowModel

	var body: some View {
		if let autosave = model.autosave {
			Section {
				Button("Resume Last Session") { model.resumeAutosave(autosave.path) }
			} footer: {
				Text("Saved \(autosave.date.formatted(date: .abbreviated, time: .shortened)), when Amiberry last went to the background.")
			}
		}
	}
}

/// One option, with the control its type calls for.
struct OptionRow: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore
	/// Whether the row says why it is unavailable; an OptionSection says it
	/// under the section instead.
	var showsConstraint = true

	var body: some View {
		let available = store.isAvailable(descriptor.id)
		VStack(alignment: .leading, spacing: 4) {
			control
				.disabled(!available)
				// A grouped form dims a disabled control but not its label:
				// the whole row reads as unavailable.
				.foregroundStyle(available ? .primary : .secondary)
				.accessibilityHint(!available ? descriptor.constraint ?? "" : "")
			if showsConstraint, !available, let constraint = descriptor.constraint {
				Text(constraint).font(.caption).foregroundStyle(.secondary)
			}
			// What the emulator's checks will make of the value, where it is
			// set (the footer lists them all, for the other pages').
			if let adjustment = store.adjustments.first(where: { $0.optionID == descriptor.id }) {
				Text(adjustment.sentence(for: descriptor, named: false))
					.font(.caption)
					.foregroundStyle(.orange)
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
			if descriptor.showsAsSwitch(value) {
				SwitchControl(value: value, set: { store.set($0, for: descriptor.id) }) { Text(descriptor.label) }
			} else {
				ChoiceControl(descriptor: descriptor, store: store)
			}
		case .integer, .real:
			if descriptor.showsAsSwitch(value) {
				SwitchControl(value: value, set: { store.set($0, for: descriptor.id) }) { Text(descriptor.label) }
			} else if descriptor.level {
				LevelControl(descriptor: descriptor, value: value,
					range: store.range(for: descriptor.id) ?? descriptor.range ?? 0...0,
					set: { store.set($0, for: descriptor.id) }) { Text(descriptor.label) }
			} else {
				NumberControl(descriptor: descriptor, store: store)
			}
		case .path, .string:
			if descriptor.id == CDDriveControl.optionID {
				CDDriveControl(descriptor: descriptor, store: store)
			} else if descriptor.id == KickstartROMControl.optionID {
				KickstartROMControl(descriptor: descriptor, store: store)
			} else {
				TextControl(descriptor: descriptor, store: store)
			}
		case .hotkey:
			HotkeyField(label: descriptor.label, value: value, buttons: HotkeyCatalog.takesButtons(descriptor.id),
				set: { store.set($0, for: descriptor.id) })
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

/// An integer or real option, typed as it reads (`SettingDescriptor.shown`):
/// in its unit and scale. Typed text is stored while it reads as a value of
/// the option, so nothing typed is lost when the window closes; text that
/// does not puts back the value the field started with, and shows why.
struct NumberControl: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore
	@Environment(SettingsWindowModel.self) private var model: SettingsWindowModel?
	@State private var draft = ""
	@State private var valueBeforeEditing: String?
	@FocusState private var editing: Bool

	var body: some View {
		let range = store.range(for: descriptor.id) ?? descriptor.range ?? 0...0
		let invalid = !isStoredValue(draft) && descriptor.storedNumber(typed: draft, in: range) == nil
		VStack(alignment: .trailing, spacing: 2) {
			LabeledContent(descriptor.label) {
				NumberEntry(descriptor: descriptor, value: store.value(of: descriptor.id) ?? "", range: range,
					invalid: invalid, set: { save($0) }) {
					TextField(descriptor.label, text: $draft)
						.focused($editing)
				}
			}
			if invalid {
				Text(descriptor.rangeSentence(range)).font(.caption).foregroundStyle(.red)
			}
		}
		.onAppear {
			valueBeforeEditing = store.value(of: descriptor.id)
			draft = model?.invalidDrafts[descriptor.id] ?? descriptor.shownText(store.value(of: descriptor.id) ?? "")
		}
		.onChange(of: store.revision) {
			if !editing, model?.invalidDrafts[descriptor.id] == nil, !isStoredValue(draft) {
				draft = descriptor.shownText(store.value(of: descriptor.id) ?? "")
			}
		}
		.onChange(of: editing) {
			if editing, model?.invalidDrafts[descriptor.id] == nil {
				valueBeforeEditing = store.value(of: descriptor.id)
			}
		}
		.onChange(of: draft) {
			// The stored value as it is: a configuration can hold one outside
			// the range the option has now (a crop height saved for double
			// lines). It was not typed, so there is nothing to refuse.
			if isStoredValue(draft) {
				model?.invalidDrafts[descriptor.id] = nil
				return
			}
			if let value = descriptor.storedNumber(typed: draft, in: range), save(value) {
				model?.invalidDrafts[descriptor.id] = nil
			} else {
				if let original = valueBeforeEditing, original != store.value(of: descriptor.id) {
					store.set(original, for: descriptor.id)
				}
				model?.invalidDrafts[descriptor.id] = draft
			}
		}
	}

	/// The draft reads as the option's stored value.
	private func isStoredValue(_ text: String) -> Bool {
		let stored = store.value(of: descriptor.id) ?? ""
		let trimmed = text.trimmingCharacters(in: .whitespaces)
		if trimmed == descriptor.shownText(stored) {
			return true
		}
		guard let typed = Double(trimmed), let current = Double(stored) else { return false }
		return SettingDescriptor.same(typed, descriptor.shown(current))
	}

	/// False when the layer refuses the value.
	@discardableResult
	private func save(_ number: Double) -> Bool {
		let text = descriptor.storedText(number)
		guard Double(text) != Double(store.value(of: descriptor.id) ?? "") else { return true }
		return store.set(text, for: descriptor.id)
	}
}

/// A number's field as the machine editor and the app settings show it:
/// the value as it reads, its unit after it, and a stepper by the
/// option's step. The row's own field decides when typed text applies.
struct NumberEntry<Field: View>: View {
	let descriptor: SettingDescriptor
	/// The stored value.
	let value: String
	/// The stored numbers the option takes now.
	let range: ClosedRange<Double>
	var invalid = false
	/// Sets a stored number (the stepper's).
	let set: (Double) -> Void
	@ViewBuilder let field: () -> Field

	var body: some View {
		let shownRange = descriptor.shown(range)
		let current = Double(value).map { descriptor.shown($0) } ?? shownRange.lowerBound
		HStack(spacing: 6) {
			field()
				.labelsHidden()
				.multilineTextAlignment(.trailing)
				.frame(width: 90)
				.foregroundStyle(invalid ? Color.red : Color.primary)
			if let unit = descriptor.unit {
				Text(unit).foregroundStyle(.secondary)
			}
			Stepper(
				descriptor.label,
				value: Binding(get: { current }, set: {
					let stored = descriptor.stored(fromShown: min(max($0, shownRange.lowerBound), shownRange.upperBound))
					set(min(max(stored, range.lowerBound), range.upperBound))
				}),
				in: shownRange, step: descriptor.shownStep
			)
			.labelsHidden()
		}
	}
}

/// An option stored as "1" on and "0" off (`SettingDescriptor.isSwitch`),
/// as a switch.
struct SwitchControl<Label: View>: View {
	let value: String
	let set: (String) -> Void
	@ViewBuilder let label: () -> Label

	var body: some View {
		Toggle(isOn: Binding(get: { value == "1" }, set: { set($0 ? "1" : "0") }), label: label)
	}
}

/// A level set by feel (`SettingDescriptor.level`): a slider over the range
/// it takes now, with the value it reads at the trailing edge in its unit.
/// A volume lies between the speaker symbols; a level either side of zero
/// (the gamma) between minus and plus, reading with its sign.
struct LevelControl<Label: View>: View {
	let descriptor: SettingDescriptor
	/// The stored value.
	let value: String
	/// The stored numbers the option takes now.
	let range: ClosedRange<Double>
	let set: (String) -> Void
	@ViewBuilder let label: () -> Label
	/// The level while the slider is dragged: stored once, on release, not
	/// at every step of the drag (App Settings write amiberry.conf per set).
	@State private var dragging: Double?

	var body: some View {
		let shownRange = descriptor.shown(range)
		// A range of one value leaves nothing to slide.
		let movable = shownRange.lowerBound < shownRange.upperBound
		let level = dragging ?? descriptor.shownLevel(of: value, in: shownRange)
		let text = descriptor.levelText(level, signed: shownRange.lowerBound < 0)
		LabeledContent {
			HStack(spacing: 8) {
				slider(level, in: movable ? shownRange : shownRange.lowerBound...(shownRange.lowerBound + 1))
					.labelsHidden()
					.disabled(!movable)
					.accessibilityValue(text)
					#if os(macOS)
					.frame(minWidth: 140, idealWidth: 200, maxWidth: 240)
					#endif
				Text(text)
					.monospacedDigit()
					.foregroundStyle(.secondary)
					.frame(minWidth: 48, alignment: .trailing)
					.accessibilityHidden(true)
			}
		} label: {
			label()
		}
	}

	/// A continuous slider: the value is rounded to the option's step when
	/// stored. A stepped one draws a tick per step, thousands for a gamma
	/// in tenths, which made laying a page out take seconds. A drag is
	/// stored when it ends; a key press (no drag) at once.
	@ViewBuilder private func slider(_ level: Double, in shownRange: ClosedRange<Double>) -> some View {
		let binding = Binding(get: { level }, set: { (new: Double) in
			if dragging != nil {
				dragging = new
			} else {
				save(new, in: shownRange)
			}
		})
		let editing = { (started: Bool) in
			if started {
				dragging = level
			} else if let last = dragging {
				dragging = nil
				save(last, in: shownRange)
			}
		}
		if let ends = descriptor.levelEndSymbols(in: shownRange) {
			Slider(value: binding, in: shownRange) {
				label()
			} minimumValueLabel: {
				Image(systemName: ends.low).foregroundStyle(.secondary).accessibilityHidden(true)
			} maximumValueLabel: {
				Image(systemName: ends.high).foregroundStyle(.secondary).accessibilityHidden(true)
			} onEditingChanged: { editing($0) }
		} else {
			Slider(value: binding, in: shownRange) {
				label()
			} onEditingChanged: { editing($0) }
		}
	}

	private func save(_ level: Double, in shownRange: ClosedRange<Double>) {
		let stored = descriptor.stored(fromShown: min(max(level, shownRange.lowerBound), shownRange.upperBound))
		let text = descriptor.storedText(min(max(stored, range.lowerBound), range.upperBound))
		guard Double(text) != Double(value) else { return }
		set(text)
	}
}

/// The Kickstart ROM as a pop-up: Automatic (no file: a kick.rom in the
/// usual places, else AROS), the built-in AROS ROM, the ROMs the last scan
/// of the ROMs folder recognised, by name, and Choose File… for another,
/// which opens in the ROMs folder.
struct KickstartROMControl: View {
	static let optionID = "kickstart_rom_file"
	/// Choose File…'s tag: no path holds a NUL.
	private static let chooseTag = "\u{0}choose"
	let descriptor: SettingDescriptor
	let store: SettingsStore
	@State private var roms: [MediaEntry] = []

	var body: some View {
		let value = store.value(of: descriptor.id) ?? ""
		Picker(descriptor.label, selection: Binding(get: { value }, set: { pick($0) })) {
			ForEach(KickstartROMControl.choices(roms: roms, current: value)) { choice in
				Text(choice.label).tag(choice.value)
			}
			Divider()
			Text("Choose File\u{2026}").tag(KickstartROMControl.chooseTag)
		}
		.help(value)
		.onAppear { roms = MediaLibrary.entries(.kickstartROMs) }
	}

	private func pick(_ tag: String) {
		guard tag == KickstartROMControl.chooseTag else {
			store.set(tag, for: descriptor.id)
			return
		}
		// The panel opens once the pop-up has closed. An empty start is the
		// ROMs folder.
		Task { @MainActor in
			let title = SettingsCatalog.descriptor(for: descriptor.id)?.label ?? descriptor.label
			if let path = FilePanels.choose(.rom, title: title, startingAt: "") {
				store.set(path, for: descriptor.id)
			}
		}
	}

	/// The pop-up's entries: Automatic for no file; the built-in AROS ROM,
	/// stored as memory.cpp's explicit ":AROS" (an RP9's "<path>:AROS"
	/// reads as it too), since no file still looks for a kick.rom first; a
	/// file the scan did not list (one picked elsewhere) by its name; then
	/// the scanned ROMs by name, those two ROMs share followed by each
	/// one's file name.
	static func choices(roms: [MediaEntry], current: String) -> [SettingChoice] {
		var counts: [String: Int] = [:]
		for rom in roms {
			counts[rom.name, default: 0] += 1
		}
		let builtIn = current.hasSuffix(":AROS")
		var list = [
			SettingChoice(value: "", label: "Automatic"),
			SettingChoice(value: builtIn ? current : ":AROS", label: "AROS (built-in)"),
		]
		if !current.isEmpty, !builtIn, !roms.contains(where: { $0.path == current }) {
			list.append(SettingChoice(value: current, label: (current as NSString).lastPathComponent))
		}
		for rom in roms {
			let shared = (counts[rom.name] ?? 0) > 1
			list.append(SettingChoice(value: rom.path,
				label: shared ? "\(rom.name) \u{2014} \((rom.path as NSString).lastPathComponent)" : rom.name))
		}
		return list
	}
}

/// The CD drive, which the layer keeps as a text value ("" for no drive,
/// "empty" for a drive without a disc, else the image and its fields): a
/// pop-up offers those, and Choose Image… a disc, as a drive's image is
/// picked rather than typed.
struct CDDriveControl: View {
	static let optionID = "cdimage0"
	let descriptor: SettingDescriptor
	let store: SettingsStore

	var body: some View {
		let value = store.value(of: descriptor.id) ?? ""
		let image = CDDriveControl.imagePath(value)
		LabeledContent(descriptor.label) {
			Menu {
				Button("No CD Drive") { store.set("", for: descriptor.id) }
				Button("Empty Drive") { store.set("empty", for: descriptor.id) }
				Divider()
				Button("Choose Image\u{2026}") {
					let title = SettingsCatalog.descriptor(for: descriptor.id)?.label ?? descriptor.label
					if let path = FilePanels.choose(.cd, title: title, startingAt: image ?? "") {
						// As the configuration writes an image: its path, then
						// its type.
						store.set("\(path),image", for: descriptor.id)
					}
				}
			} label: {
				Text(CDDriveControl.summary(value))
			}
			.fixedSize()
			.help(value)
		}
	}

	/// The image a value names, nil for no drive or an empty one. Read as
	/// the configuration reader does (cfgfile.cpp's parse_cdimage_entry):
	/// the fields follow the last comma, unless a dot after it shows the
	/// comma was the file name's.
	static func imagePath(_ value: String) -> String? {
		guard !value.isEmpty, value != "empty" else { return nil }
		if value.hasPrefix("\"") {
			let rest = value.dropFirst()
			return rest.firstIndex(of: "\"").map { String(rest[..<$0]) } ?? String(rest)
		}
		guard let comma = value.lastIndex(of: ","),
			!value[value.index(after: comma)...].contains(".")
		else { return value }
		// A drive's fields with no image (",image") is a drive without one.
		return comma == value.startIndex ? nil : String(value[..<comma])
	}

	/// What the pop-up shows: the state, or the image's file name.
	static func summary(_ value: String) -> String {
		guard !value.isEmpty else { return "No CD Drive" }
		guard let path = imagePath(value) else { return "Empty Drive" }
		// A host drive or a device name is shown as it is.
		return path.contains("/") ? (path as NSString).lastPathComponent : path
	}
}

/// A path or literal value, stored as it is typed. A Mac shows a path as
/// the Finder does (PathField): its icon and name, with Choose… and drops.
struct TextControl: View {
	let descriptor: SettingDescriptor
	let store: SettingsStore
	@Environment(SettingsWindowModel.self) private var model: SettingsWindowModel?
	@State private var draft = ""

	var body: some View {
		#if os(macOS)
		if descriptor.kind == .path {
			let request = FileRequest.forOption(descriptor.id)
			let folder = FileRequest.namesFolder(descriptor.id)
			let path = store.value(of: descriptor.id) ?? ""
			// The row may be named in its section's words ("Image" under
			// "DF0:"); a requester and VoiceOver, out of that section, get the
			// option's whole name.
			let fullName = SettingsCatalog.descriptor(for: descriptor.id)?.label ?? descriptor.label
			PathField(label: descriptor.label, path: path,
				choose: {
					folder ? FilePanels.chooseFolder(title: fullName, startingAt: path)
						: FilePanels.choose(request, title: fullName, startingAt: path)
				},
				set: { store.set($0, for: descriptor.id) },
				folder: folder,
				// Disks and ROMs are read: one that is gone is worth a flag.
				// Other files may be ones the emulator writes (flash, clock).
				warnsWhenMissing: request == .floppy || request == .rom,
				accessibilityName: fullName)
		} else {
			field
		}
		#else
		field
		#endif
	}

	private var field: some View {
		LabeledContent(descriptor.label) {
			HStack(spacing: 6) {
				if descriptor.kind == .path {
					// A path (only an iOS app's rows come here) shows the file's
					// name: the app's container path means nothing on a phone,
					// and nobody types one there.
					Text(draft.isEmpty ? "None" : (draft as NSString).lastPathComponent)
						.foregroundStyle(draft.isEmpty ? .secondary : .primary)
						.lineLimit(1)
						.truncationMode(.middle)
					if !draft.isEmpty {
						Button {
							draft = ""
						} label: {
							Image(systemName: "xmark.circle.fill")
						}
						.buttonStyle(.borderless)
						.foregroundStyle(.secondary)
						.accessibilityLabel("Remove \(SettingsCatalog.descriptor(for: descriptor.id)?.label ?? descriptor.label)")
					}
					Button("Choose\u{2026}") {
						let title = SettingsCatalog.descriptor(for: descriptor.id)?.label ?? descriptor.label
						if let path = FilePanels.choose(.forOption(descriptor.id), title: title, startingAt: draft) {
							draft = path
						}
					}
					.buttonStyle(.borderless)
					.foregroundStyle(Color.accentColor)
				} else {
					// The prompt shows where an empty value is. A Mac's grouped
					// form draws fields without a border, which read as fixed
					// labels: typed values get one, as in the app settings.
					// iOS's sit at the trailing edge, as the pickers' do.
					TextField(descriptor.label, text: $draft, prompt: Text("None"))
						.labelsHidden()
						#if os(macOS)
						.textFieldStyle(.roundedBorder)
						.multilineTextAlignment(.leading)
						.frame(maxWidth: 240)
						#else
						.multilineTextAlignment(.trailing)
						#endif
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

/// The model a machine starts from. A new machine follows its pickers at
/// once, as the built-in Quickstart does; over a machine that exists
/// already, applying one replaces its settings, so that waits for Replace
/// Settings… and asks first.
struct ModelSections: View {
	let model: SettingsWindowModel
	@State private var presetID: String
	@State private var configuration: Int
	@State private var compatibility = 1
	@State private var confirmingReplace = false

	/// A new machine's Overview shows the model it started from. Set once
	/// here: sections inside a Form appear again as they scroll into view.
	init(model: SettingsWindowModel) {
		self.model = model
		let id = model.newMachineModel?.id ?? MachinePresets.all.first?.id ?? ""
		_presetID = State(initialValue: id)
		_configuration = State(initialValue: model.newMachineModel?.configuration ?? 0)
		_compatibility = State(initialValue: model.newMachineCompatibility ?? MachinePresets.defaultCompatibility(for: id))
	}

	var body: some View {
		let preset = MachinePresets.preset(for: presetID)
		Section {
			Picker("Model", selection: $presetID) {
				ForEach(MachinePresets.all) { preset in
					Text(preset.label).tag(preset.id)
				}
			}
			.onChange(of: presetID) {
				// Another model's configurations and levels: its first and
				// its default, set before a new machine takes the model.
				configuration = 0
				compatibility = MachinePresets.defaultCompatibility(for: presetID)
				applyToNewMachine()
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
					Label("The Kickstart ROM this configuration needs is not installed: the machine boots the free AROS ROM instead.",
						systemImage: "exclamationmark.triangle.fill")
						.foregroundStyle(.secondary)
						.symbolRenderingMode(.multicolor)
				}
			}
			if !model.isNewMachine {
				HStack {
					Spacer()
					Button("Replace Settings\u{2026}") { confirmingReplace = true }
						.disabled(preset == nil)
						.accessibilityIdentifier("overview.applyModel")
				}
			}
		} header: {
			Text(model.isNewMachine ? "Start From" : "Start Over from a Model")
		} footer: {
			Text(model.isNewMachine
				? "Choosing another model sets the machine up as that model; its other settings go back to the model\u{2019}s."
				: "Replacing the settings sets every one of them to the model\u{2019}s; the machine keeps its name, description and category.")
		}
		.onChange(of: configuration) { applyToNewMachine() }
		.onChange(of: compatibility) { applyToNewMachine() }
		.confirmationDialog(
			"Replace this machine\u{2019}s settings with the \(preset?.label ?? "") model?",
			isPresented: $confirmingReplace, titleVisibility: .visible
		) {
			Button("Replace Settings", role: .destructive) { apply() }
			Button("Cancel", role: .cancel) {}
		} message: {
			Text("Every setting of \u{201C}\(model.title)\u{201D} changes to the model\u{2019}s. Nothing is saved until you save the machine.")
		}
	}

	private func apply() {
		model.applyModel(presetID, configuration: configuration, compatibility: compatibility)
	}

	/// A new machine follows its pickers: the model applies once for each
	/// change, whichever picker made it.
	private func applyToNewMachine() {
		guard model.isNewMachine, MachinePresets.preset(for: presetID) != nil else { return }
		let current = model.newMachineModel
		let currentCompatibility = model.newMachineCompatibility ?? MachinePresets.defaultCompatibility(for: presetID)
		guard current?.id != presetID || current?.configuration != configuration || currentCompatibility != compatibility
		else { return }
		apply()
	}
}

/// The mounted hard drives (folders and hardfiles).
struct MountSections: View {
	let model: SettingsWindowModel

	var body: some View {
		let mounts = model.store.mounts
		Group {
			Section("Hard drives") {
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
		guard let field = MountSections.mountField(path) else {
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
		guard let path = FilePanels.choose(.hardfile, title: "Choose a hardfile to mount", startingAt: "") else { return }
		let device = freeDevice()
		add({ "hdf,rw,\(device):\($0),32,1,2,512,0,,uae0" }, path: path)
	}
}

/// Over a running emulation, what the edits wait for: a reset or a restart
/// brings them in. A restart covers a reset's too.
struct ApplyBanner: View {
	let model: SettingsWindowModel

	var body: some View {
		let waiting = model.waiting
		let restart = (waiting[.restart] ?? []).map(\.label) + (model.boardsWaitForRestart ? ["Expansion boards"] : [])
		let reset = (waiting[.reset] ?? []).map(\.label)
		if !restart.isEmpty || !reset.isEmpty {
			HStack(alignment: .firstTextBaseline, spacing: 10) {
				Image(systemName: restart.isEmpty ? "arrow.counterclockwise.circle" : "arrow.triangle.2.circlepath.circle")
					.foregroundStyle(.orange)
					.accessibilityHidden(true)
				VStack(alignment: .leading, spacing: 2) {
					Text(restart.isEmpty ? "These changes take effect at a reset" : "These changes take effect when the machine restarts")
						.font(.headline)
					Text((restart + reset).joined(separator: ", "))
						.foregroundStyle(.secondary)
						.lineLimit(2)
				}
				.accessibilityElement(children: .combine)
				Spacer(minLength: 8)
				if restart.isEmpty {
					Button("Reset Now") { model.reset() }
						.accessibilityIdentifier("apply.reset")
				} else {
					Button("Restart Now") { model.restart() }
						.accessibilityIdentifier("apply.restart")
				}
			}
			.padding(.horizontal, 16)
			.padding(.vertical, 10)
			// Tinted bar material: the page scrolls under it, as under the
			// footer, and must not show through.
			.background(Color.orange.opacity(0.12))
			.background(.bar)
		}
	}
}

extension SettingAdjustment {
	/// "CPU model will be 68020 (A1200): …", or without the name next to
	/// the option itself.
	func sentence(for descriptor: SettingDescriptor?, named: Bool) -> String {
		let value = descriptor?.displayValue(adjusted) ?? adjusted
		let head = named ? "\(descriptor?.label ?? optionID) will be \(value)" : "Will be \(value)"
		return head + (reason.map { ": \($0)" } ?? "")
	}
}

extension SettingDescriptor {
	/// A value as the controls show it: a choice's label, on or off, a
	/// number as it reads in its unit.
	func displayValue(_ value: String) -> String {
		if isSwitch {
			return value == "1" ? "on" : value == "0" ? "off" : value
		}
		switch kind {
		case .bool: return value == "true" ? "on" : "off"
		case .choice: return choices.first { $0.value == value }?.label ?? value
		case .integer, .real: return Double(value).map { withUnit(shownText(number: shown($0))) } ?? value
		default: return value
		}
	}

	/// The step of the shown number (`shown(_:)`): the option's step, 1 for
	/// none, over the scale.
	var shownStep: Double {
		let step = self.step.flatMap { $0 > 0 ? $0 : nil } ?? 1
		return max(step / scale, 0.0001)
	}

	/// A shown number as text, to the decimals of the shown step ("12.5"
	/// for a step of 0.1); a real option without a step as it is.
	func shownText(number: Double) -> String {
		let free = kind == .real && (step ?? 0) <= 0
		return SettingDescriptor.plain(number, decimals: free ? nil : SettingDescriptor.decimals(of: shownStep))
	}

	/// A stored value as the number it reads as; text that is no number as
	/// it is.
	func shownText(_ stored: String) -> String {
		Double(stored).map { shownText(number: shown($0)) } ?? stored
	}

	/// A stored number as the value text the layer takes: whole for an
	/// integer, to the step's decimals for a real.
	func storedText(_ number: Double) -> String {
		if kind == .integer {
			return SettingDescriptor.plain(number, decimals: 0)
		}
		let step = self.step ?? 0
		return SettingDescriptor.plain(number, decimals: step > 0 ? SettingDescriptor.decimals(of: step) : nil)
	}

	/// The stored number typed text means: nil unless it is a number that
	/// is one of the option's values, as it reads, within `range` (stored
	/// numbers). Nothing is rounded behind the user's back: 12.34 is no
	/// gamma, which goes by tenths.
	func storedNumber(typed text: String, in range: ClosedRange<Double>) -> Double? {
		guard let typed = Double(text.trimmingCharacters(in: .whitespaces)), typed.isFinite else { return nil }
		let number = stored(fromShown: typed)
		guard SettingDescriptor.same(shown(number), typed), range.contains(number) else { return nil }
		return number
	}

	/// What a field asks for when its text is no value of the option.
	func rangeSentence(_ range: ClosedRange<Double>) -> String {
		let shownRange = shown(range)
		var sentence = "Enter a number from \(shownText(number: shownRange.lowerBound)) to "
			+ withUnit(shownText(number: shownRange.upperBound))
		if let step, step > 0, !SettingDescriptor.same(shownStep, 1) {
			sentence += " in steps of \(SettingDescriptor.plain(shownStep, decimals: SettingDescriptor.decimals(of: shownStep)))"
		}
		return sentence + "."
	}

	/// A number with the unit after it: "80%", "720 px".
	func withUnit(_ text: String) -> String {
		guard let unit, !unit.isEmpty else { return text }
		return unit == "%" ? text + unit : "\(text) \(unit)"
	}

	/// A level as its slider reads it: "80%", "0", and "+12.5" for a level
	/// that can go either side of zero.
	func levelText(_ level: Double, signed: Bool) -> String {
		let text = shownText(number: level)
		return withUnit(signed && (Double(text) ?? 0) > 0 ? "+" + text : text)
	}

	/// The level a stored value reads as, within the shown range: the
	/// default's for a value that is no number.
	func shownLevel(of stored: String, in shownRange: ClosedRange<Double>) -> Double {
		guard let number = Double(stored) ?? Double(defaultValue) else { return shownRange.lowerBound }
		return min(max(shown(number), shownRange.lowerBound), shownRange.upperBound)
	}

	/// The symbols at a level slider's ends: speakers for a volume (a
	/// reversed percentage), minus and plus for a level either side of zero.
	func levelEndSymbols(in shownRange: ClosedRange<Double>) -> (low: String, high: String)? {
		if reversed, unit == "%" {
			return ("speaker.fill", "speaker.wave.3.fill")
		}
		if shownRange.lowerBound < 0, shownRange.upperBound > 0 {
			return ("minus", "plus")
		}
		return nil
	}

	/// Whether two numbers are the same value, but for floating point's
	/// last digits.
	static func same(_ a: Double, _ b: Double) -> Bool {
		abs(a - b) <= 1e-9 * max(1, abs(a), abs(b))
	}

	/// The decimals a step needs: 0 for 1, 1 for 0.5, 2 for 0.01.
	static func decimals(of step: Double) -> Int {
		var digits = 0
		var value = abs(step)
		while digits < 6, !same(value, value.rounded()) {
			value *= 10
			digits += 1
		}
		return digits
	}

	/// A number without a trailing ".0" or a minus on zero, rounded to
	/// `decimals` when given.
	private static func plain(_ number: Double, decimals: Int?) -> String {
		var number = number
		if let decimals {
			let factor = pow(10, Double(decimals))
			number = (number * factor).rounded() / factor
		}
		if number == 0 {
			return "0"
		}
		if number == number.rounded(), abs(number) < 1e15 {
			return String(Int64(number))
		}
		return String(number)
	}
}

struct FooterView: View {
	@Bindable var model: SettingsWindowModel
	@State private var confirmingRevert = false

	var body: some View {
		HStack(alignment: .center, spacing: 10) {
			VStack(alignment: .leading, spacing: 2) {
				if let error = model.store.lastError {
					Label(SettingsWindowModel.describe(error), systemImage: "exclamationmark.triangle.fill")
						.foregroundStyle(.red)
				} else if let notice = model.notice {
					Label(notice, systemImage: "exclamationmark.triangle.fill")
						.foregroundStyle(.red)
				}
				ForEach(model.store.adjustments) { adjustment in
					Label(adjustment.sentence(for: SettingsCatalog.descriptor(for: adjustment.optionID), named: true),
						systemImage: "info.circle.fill")
						.foregroundStyle(.orange)
				}
			}
			.font(.callout)
			.lineLimit(2)
			Spacer()
			// Quitting stays in the application menu (⌘Q), out of reach of a
			// stray click beside Reset and Resume.
			if model.emulating {
				// Resets the Amiga with the changes; not Revert's undo. Kept
				// apart from Resume, the default button, so one does not hit
				// the other.
				Button("Reset Amiga") { model.reset() }
					.help("Apply the changes and reset the Amiga")
					.padding(.trailing, 14)
			}
			if model.fromLibrary {
				// Back to the Library; unsaved edits are asked about first.
				Button("Close") { model.closeRequested() }
					.help("Close the settings and return to the Library")
					.accessibilityIdentifier("editor.library")
			}
			if !model.isNewMachine {
				Button("Revert\u{2026}") { confirmingRevert = true }
					.disabled(!model.canRevert)
					.help("Drop the changes made since the settings opened")
					.accessibilityIdentifier("editor.revert")
			}
			Button("Save\u{2026}") { model.showingSaveSheet = true }
				.accessibilityIdentifier("editor.save")
			if model.emulating {
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
		// However it closes (Escape, a swipe), a close-after-save request ends.
		.sheet(isPresented: $model.showingSaveSheet, onDismiss: { model.saveSheetFinished(saved: false) }) {
			SaveMachineSheet(model: model)
		}
		.confirmationDialog("Discard your changes to \u{201C}\(model.title)\u{201D}?",
			isPresented: $confirmingRevert, titleVisibility: .visible) {
			Button("Discard Changes", role: .destructive) { model.revert() }
			Button("Cancel", role: .cancel) {}
		} message: {
			Text("The settings go back to how they were when you opened them.")
		}
	}
}

/// Saves the machine under a name, with its description and category.
struct SaveMachineSheet: View {
	let model: SettingsWindowModel
	@State private var name = ""
	@State private var description = ""
	@State private var category = ""
	@State private var working = false
	/// The categories the saved machines use, offered so a machine joins
	/// one as it is spelled.
	@State private var categories: [String] = []

	var body: some View {
		VStack(alignment: .leading, spacing: 12) {
			Text("Save Machine").font(.headline)
			Form {
				TextField("Name", text: $name)
				TextField("Description", text: $description, prompt: Text("Optional"))
				HStack(spacing: 6) {
					TextField("Category", text: $category, prompt: Text("Optional"))
					if !categories.isEmpty {
						Menu {
							ForEach(categories, id: \.self) { name in
								Button(name) { category = name }
							}
						} label: {
							Image(systemName: "chevron.up.chevron.down")
						}
						.menuStyle(.button)
						.buttonStyle(.borderless)
						.menuIndicator(.hidden)
						.fixedSize()
						.accessibilityLabel("Existing categories")
					}
				}
			}
			if let notice = model.notice {
				Label(notice, systemImage: "exclamationmark.triangle.fill")
					.foregroundStyle(.red)
					.font(.callout)
			}
			HStack {
				Spacer()
				Button("Cancel") { model.saveSheetFinished(saved: false) }
					.keyboardShortcut(.cancelAction)
				Button("Save") {
					working = true
					Task { @MainActor in
						let saved = await model.save(name: name, description: description, category: category)
						working = false
						if saved {
							model.saveSheetFinished(saved: true)
						}
					}
				}
				.keyboardShortcut(.defaultAction)
				.disabled(working || name.trimmingCharacters(in: .whitespaces).isEmpty)
				.accessibilityIdentifier("save.confirm")
			}
		}
		.padding(20)
		.frame(minWidth: 380)
		.onAppear {
			// A message left from the editor (a field to correct) is not this
			// sheet's.
			model.notice = nil
			name = model.suggestedName
			description = model.store.value(of: "config_description") ?? ""
			category = model.store.value(of: "config_category") ?? ""
			categories = SaveMachineSheet.savedCategories()
		}
	}

	/// The categories of the saved machines, sorted as the Finder sorts.
	static func savedCategories() -> [String] {
		let names = MediaLibrary.entries(.configurations).compactMap { MediaLibrary.configurationInfo($0.path)?.category }
		return Set(names.filter { !$0.isEmpty }).sorted { $0.localizedStandardCompare($1) == .orderedAscending }
	}
}
