/*
 * Copyright (C) 2026 BlitterStudio
 * The app settings: the emulator's own settings (amiberry.conf), not a
 * machine's. They apply at once and are saved as they change. The Mac
 * shows them in the Settings window (Cmd-,), a page per group beside a
 * sidebar as System Settings does; iOS shows them in one form from the
 * Library.
 */

import SwiftUI

@MainActor
@Observable
final class AppSettingsModel {
	/// Every host setting's value, by ID.
	private(set) var values: [String: String] = [:]
	/// What each choice setting offers, by ID: its own list, or the host's
	/// devices, shaders and bezels as the layer listed them last (with the
	/// stored value first, labelled by the layer, when the host lacks it).
	private(set) var choices: [String: [SettingChoice]] = [:]
	/// What went wrong with the last change, shown under the settings.
	var notice: String?

	init() {
		reload()
		readChoices()
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

	/// Re-reads the host's devices (one may have been connected since) and
	/// every choice list: the settings call it each time they are shown.
	func refreshChoices() {
		HostSettings.refreshChoices()
		readChoices()
	}

	private func readChoices() {
		var read: [String: [SettingChoice]] = [:]
		for setting in HostSettings.all where setting.kind == .choice {
			read[setting.id] = HostSettings.choices(for: setting.id)
		}
		choices = read
	}

	/// What a choice setting's picker offers: the layer's list as it is.
	func offeredChoices(for setting: HostSetting) -> [SettingChoice] {
		AppSettingsModel.offered(choices[setting.id] ?? setting.descriptor.choices, current: value(of: setting.id))
	}

	/// The list, with the current value added first when the layer does
	/// not list it (a fixed list's stray value from an edited file), so a
	/// picker can show it, by its value.
	nonisolated static func offered(_ list: [SettingChoice], current: String) -> [SettingChoice] {
		guard !list.contains(where: { $0.value == current }) else { return list }
		return [SettingChoice(value: current, label: current.isEmpty ? "None" : current)] + list
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
		// A list follows the stored value (a device that is not connected
		// is listed only while it is stored), so the lists are read again.
		readChoices()
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

	/// The groups in the order the layer lists them, with their titles and
	/// symbols: the Mac's sidebar, iOS's sections.
	static let groups: [AppSettingsGroup] = {
		var seen: [String] = []
		for setting in HostSettings.all where !seen.contains(setting.group) {
			seen.append(setting.group)
		}
		return seen.map { AppSettingsGroup(id: $0, title: title(of: $0), symbol: symbol(of: $0)) }
	}()

	static func title(of group: String) -> String {
		switch group {
		case "global_input": return "Input Defaults"
		case "global_display": return "Display Defaults"
		case "global_sound": return "Sound Defaults"
		case "global_whdload": return "WHDLoad"
		#if os(macOS)
		// A Mac has no touch screen: the controls are drawn on the screen.
		case "global_touch": return "On-Screen Controls"
		#else
		case "global_touch": return "Touch Controls"
		#endif
		case "global_visual": return "Appearance"
		case "global_paths": return "Folders & Logging"
		case "global_updates": return "Updates"
		case "global_advanced": return "Advanced"
		default: return SettingsPage.groupTitle(group)
		}
	}

	/// The symbol of a group the layer adds before one is chosen here.
	static let fallbackSymbol = "gearshape"

	/// The SF Symbol shown beside a group in the Mac's sidebar.
	static func symbol(of group: String) -> String {
		switch group {
		case "global_input": return "keyboard"
		case "global_display": return "display"
		case "global_sound": return "speaker.wave.2"
		case "global_whdload": return "shippingbox"
		case "global_touch": return "hand.tap"
		case "global_visual": return "paintpalette"
		case "global_paths": return "folder"
		case "global_updates": return "arrow.down.circle"
		case "global_advanced": return "gearshape.2"
		default: return fallbackSymbol
		}
	}

	/// The settings' collapsed disclosures the user has expanded (the
	/// RetroArch defaults), remembered while the settings show.
	var expandedDisclosures: Set<String> = []

	/// Whether a collapsed disclosure shows its settings.
	func disclosureExpanded(_ key: String) -> Bool {
		expandedDisclosures.contains(key)
	}

	func setDisclosureExpanded(_ expanded: Bool, _ key: String) {
		if expanded {
			expandedDisclosures.insert(key)
		} else {
			expandedDisclosures.remove(key)
		}
	}
}

/// One section of a group's settings on its page. Most groups are one
/// section of their own title; the ones that mix concerns split (the
/// audit §3b), so a page says what its rows are about. `title` nil: the
/// group's own title stands for it.
struct AppSettingsSection: Identifiable {
	let title: String?
	/// The section's settings, in the layer's order.
	let settings: [HostSetting]
	/// The settings that sit inside a collapsed disclosure at the end of
	/// the section (the RetroArch defaults), by id.
	let disclosedTitle: String?
	let disclosed: Set<String>

	var id: String { title ?? "" }
}

/// A group of host settings: a page of the Mac's Settings window, a
/// section of iOS's, split into the sections the page shows.
struct AppSettingsGroup: Identifiable, Hashable {
	let id: String
	let title: String
	let symbol: String

	/// The group's settings, in the layer's order.
	var settings: [HostSetting] {
		HostSettings.all.filter { $0.group == id }
	}

	/// The group's settings whose label, help or key holds every word of
	/// the query (all of them for an empty one).
	func settings(matching query: String) -> [HostSetting] {
		let terms = query.lowercased().split(whereSeparator: \.isWhitespace).map(String.init)
		return settings.filter { setting in
			let text = "\(setting.label) \(setting.descriptor.help) \(setting.id)".lowercased()
			return terms.allSatisfy(text.contains)
		}
	}

	/// The group's settings as the sections of its page: what each setting
	/// is about sits together, in the layer's order within a section. A
	/// section this platform has none of does not show.
	var sections: [AppSettingsSection] {
		AppSettingsGroup.split(id, settings: settings).filter { !$0.settings.isEmpty }
	}

	/// The group's sections narrowed to the settings that match, empty
	/// ones left out.
	func sections(matching query: String) -> [AppSettingsSection] {
		let matching = Set(settings(matching: query).map(\.id))
		return sections.map { section in
			AppSettingsSection(title: section.title, settings: section.settings.filter { matching.contains($0.id) },
				disclosedTitle: section.disclosedTitle, disclosed: section.disclosed)
		}.filter { !$0.settings.isEmpty }
	}

	/// How the groups whose settings mix concerns split; a group not here
	/// is one section of its own title.
	private static func split(_ group: String, settings: [HostSetting]) -> [AppSettingsSection] {
		let inThisOrder = { (ids: [String]) in settings.filter { ids.contains($0.id) } }
		switch group {
		case "global_input":
			return [
				AppSettingsSection(title: "Controllers & Mouse", settings: inThisOrder([
					"default_controller1", "default_controller2", "default_controller3", "default_controller4",
					"default_mouse1", "default_mouse2", "input_default_mouse_speed", "default_joystick_deadzone",
					"input_keyboard_as_joystick_stop_keypresses", "rctrl_as_ramiga",
				]), disclosedTitle: nil, disclosed: []),
				AppSettingsSection(title: "Default Shortcuts", settings: inThisOrder([
					"macos_left_command_q", "default_open_gui_key", "default_quit_key", "default_ar_key",
					"default_fullscreen_toggle_key", "default_retroarch_quit", "default_retroarch_menu",
					"default_retroarch_reset",
				]), disclosedTitle: "RetroArch",
					disclosed: ["default_retroarch_quit", "default_retroarch_menu", "default_retroarch_reset"]),
			]
		case "global_display":
			return [
				AppSettingsSection(title: "Display Defaults", settings: inThisOrder([
					"default_line_mode", "default_horizontal_centering", "default_vertical_centering",
					"default_scaling_method", "default_gfx_autoresolution", "default_correct_aspect_ratio",
					"default_auto_crop",
				]), disclosedTitle: nil, disclosed: []),
				// The window sizes mean nothing on iOS (the layer hides
				// them there); the screen mode default stays with its
				// machine options.
				AppSettingsSection(title: "Window Defaults", settings: inThisOrder([
					"default_width", "default_height", "default_fullscreen_mode",
				]), disclosedTitle: nil, disclosed: []),
			]
		case "global_paths":
			let logging = ["write_logfile", "logfile_path"]
			return [
				AppSettingsSection(title: "Logging", settings: inThisOrder(logging),
					disclosedTitle: nil, disclosed: []),
				AppSettingsSection(title: "Folders", settings: settings.filter { !logging.contains($0.id) },
					disclosedTitle: nil, disclosed: []),
			]
		default:
			return [AppSettingsSection(title: nil, settings: settings, disclosedTitle: nil, disclosed: [])]
		}
	}
}

/// Every group in one form, a section each: iOS's settings sheet. The
/// search narrows it to the settings that match.
struct AppSettingsView: View {
	@Bindable var model: AppSettingsModel
	@State private var query = ""

	var body: some View {
		let searching = !query.trimmingCharacters(in: .whitespaces).isEmpty
		Form {
			if let notice = model.notice {
				Section {
					AppSettingsNotice(text: notice)
				}
			}
			ForEach(AppSettingsModel.groups) { group in
				ForEach(searching ? group.sections(matching: query) : group.sections) { section in
					Section(section.title ?? group.title) {
						AppSettingsSectionRows(group: group, section: section, model: model,
							flat: searching, showsDownloads: !searching)
					}
				}
			}
			#if os(iOS)
			if !searching {
				Section {
					NavigationLink {
						LicencesView()
							.navigationTitle("About & Licences")
					} label: {
						Label("About & Licences", systemImage: "info.circle")
					}
					.accessibilityIdentifier("settings.licences")
				}
			}
			#endif
		}
		.formStyle(.grouped)
		.overlay {
			if searching, !AppSettingsModel.groups.contains(where: { !$0.settings(matching: query).isEmpty }) {
				ContentUnavailableView.search(text: query)
			}
		}
		.searchable(text: $query, prompt: "Search Settings")
		.onAppear {
			model.reload()
			model.refreshChoices()
		}
	}
}

#if os(macOS)
/// The Mac's Settings window, laid out as System Settings: the groups in a
/// sidebar, the chosen group's settings beside it.
struct AppSettingsSplitView: View {
	@Bindable var model: AppSettingsModel
	/// The group shown, kept from one opening of the window to the next.
	@AppStorage("AppSettingsGroup") private var selected = ""
	@State private var query = ""

	/// The group shown: the remembered one, or the first when none is (or
	/// the remembered group is no longer listed).
	private var group: AppSettingsGroup? {
		AppSettingsModel.groups.first { $0.id == selected } ?? AppSettingsModel.groups.first
	}

	var body: some View {
		NavigationSplitView {
			List(selection: Binding<String?>(get: { group?.id }, set: { if let id = $0 { selected = id; query = "" } })) {
				ForEach(AppSettingsModel.groups) { group in
					Label(group.title, systemImage: group.symbol)
						.tag(Optional(group.id))
						.accessibilityIdentifier("appSettings.group.\(group.id)")
				}
			}
			// The group list is the only way between groups, as in System
			// Settings: it cannot be collapsed away.
			.toolbar(removing: .sidebarToggle)
			.navigationSplitViewColumnWidth(min: 180, ideal: 200)
		} detail: {
			if !query.trimmingCharacters(in: .whitespaces).isEmpty {
				// Every group's matches, as System Settings searches.
				let results = AppSettingsModel.groups.map { ($0, $0.sections(matching: query)) }.filter { !$0.1.isEmpty }
				if results.isEmpty {
					ContentUnavailableView.search(text: query)
				} else {
					Form {
						if let notice = model.notice {
							Section {
								AppSettingsNotice(text: notice)
							}
						}
						ForEach(results, id: \.0.id) { group, sections in
							ForEach(sections) { section in
								Section(section.title ?? group.title) {
									AppSettingsSectionRows(group: group, section: section, model: model, flat: true,
										showsDownloads: false)
								}
							}
						}
					}
					.formStyle(.grouped)
					.navigationTitle("Search")
				}
			} else if let group {
				// A page of its own identity: groups must not share a scroll
				// position, and leaving one commits its fields' drafts. Its
				// sections stand under the page's title, each with its own.
				Form {
					if let notice = model.notice {
						Section {
							AppSettingsNotice(text: notice)
						}
					}
					ForEach(group.sections) { section in
						Section {
							AppSettingsSectionRows(group: group, section: section, model: model, showsDownloads: true)
						} header: {
							if let title = section.title {
								Text(title)
							}
						}
					}
				}
				.formStyle(.grouped)
				.id(group.id)
				.navigationTitle(group.title)
			}
		}
		.searchable(text: $query, placement: .sidebar, prompt: "Search Settings")
		.onAppear {
			model.reload()
			model.refreshChoices()
		}
	}
}
#endif

/// One section's settings, with the settings its collapsed disclosure
/// holds (the RetroArch defaults) behind it and the WHDLoad group's file
/// downloads after them (not among search results: they are no setting).
/// Search results show the disclosed settings flat.
private struct AppSettingsSectionRows: View {
	let group: AppSettingsGroup
	let section: AppSettingsSection
	let model: AppSettingsModel
	var flat = false
	let showsDownloads: Bool

	var body: some View {
		ForEach(section.settings.filter { !section.disclosed.contains($0.id) }) { setting in
			HostSettingRow(setting: setting, model: model)
		}
		let disclosed = section.settings.filter { section.disclosed.contains($0.id) }
		if !disclosed.isEmpty {
			if flat {
				ForEach(disclosed) { setting in
					HostSettingRow(setting: setting, model: model)
				}
			} else if let title = section.disclosedTitle {
				let key = "\(group.id).\(title)"
				DisclosureGroup(title, isExpanded: Binding(
					get: { model.disclosureExpanded(key) },
					set: { model.setDisclosureExpanded($0, key) })) {
					ForEach(disclosed) { setting in
						HostSettingRow(setting: setting, model: model)
					}
				}
				.accessibilityIdentifier("appSettings.disclosure.\(title)")
			}
		}
		if showsDownloads, group.id == "global_whdload" {
			WHDBootFilesRow()
		}
	}
}

/// What went wrong with the last change, at the top of the settings shown.
private struct AppSettingsNotice: View {
	let text: String

	var body: some View {
		Label {
			Text(text)
		} icon: {
			Image(systemName: "exclamationmark.triangle.fill")
				.foregroundStyle(.yellow)
		}
	}
}

struct HostSettingRow: View {
	let setting: HostSetting
	let model: AppSettingsModel
	@State private var draft = ""
	/// The paths the emulator does without: set to "" they go back to none
	/// (amiberry.cpp's set_base_content_path, set_plugins_path), where the
	/// other folders would be left unset.
	static let optionalPaths: Set<String> = ["base_content_path", "plugins_dir", "retroarch_config"]
	@FocusState private var editing: Bool

	var body: some View {
		control.help(setting.descriptor.help)
	}

	private var value: String { model.value(of: setting.id) }

	/// The setting's name over what it does, as System Settings explains
	/// its rows: a grouped form shows a label's second text as a subtitle.
	/// A tooltip alone reached no one on a touch screen or the keyboard.
	@ViewBuilder private var rowLabel: some View {
		Text(setting.label)
		if !setting.descriptor.help.isEmpty {
			Text(setting.descriptor.help)
		}
	}

	@ViewBuilder private var control: some View {
		switch setting.kind {
		case .bool:
			Toggle(isOn: Binding(get: { value == "true" }, set: { model.set($0 ? "true" : "false", for: setting) })) {
				rowLabel
			}
		case .choice where setting.descriptor.showsAsSwitch(value), .integer where setting.descriptor.showsAsSwitch(value):
			SwitchControl(value: value, set: { model.set($0, for: setting) }) { rowLabel }
		case .choice:
			let current = value
			Picker(selection: Binding(get: { current }, set: { model.set($0, for: setting) })) {
				ForEach(model.offeredChoices(for: setting)) { Text($0.label).tag($0.value) }
			} label: {
				rowLabel
			}
		case .integer where setting.descriptor.level, .real where setting.descriptor.level:
			LevelControl(descriptor: setting.descriptor, value: value, range: setting.descriptor.range ?? 0...0,
				set: { model.set($0, for: setting) }) { rowLabel }
		case .integer, .real:
			LabeledContent {
				NumberEntry(descriptor: setting.descriptor, value: value, range: setting.descriptor.range ?? 0...0,
					set: { model.set(setting.descriptor.storedText($0), for: setting) }) {
					draftField()
				}
			} label: {
				rowLabel
			}
		case .path:
			#if os(macOS)
			// The RetroArch config is a file; every other path is a folder
			// the emulator uses (the log is a file in a chosen folder, so a
			// dropped item cannot stand for it). The optional ones go back to
			// none when removed; the others always name a folder.
			PathField(label: setting.label, path: value, choose: { choose() }, set: { model.set($0, for: setting) },
				folder: setting.id != "retroarch_config",
				removable: HostSettingRow.optionalPaths.contains(setting.id),
				acceptsDrops: setting.id != "logfile_path",
				// The log is written when logging starts, and the RetroArch
				// configuration is optional: only a missing folder matters.
				warnsWhenMissing: setting.id != "logfile_path" && setting.id != "retroarch_config")
			#else
			// The folder's name, not the app container's path to it, which
			// means nothing on iOS and is never typed there.
			LabeledContent(setting.label) {
				HStack(spacing: 6) {
					Text(value.isEmpty ? "None" : (value as NSString).lastPathComponent)
						.foregroundStyle(value.isEmpty ? .secondary : .primary)
						.lineLimit(1)
						.truncationMode(.middle)
					if HostSettingRow.optionalPaths.contains(setting.id), !value.isEmpty {
						Button {
							model.set("", for: setting)
						} label: {
							Image(systemName: "xmark.circle.fill")
						}
						.buttonStyle(.borderless)
						.foregroundStyle(.secondary)
						.accessibilityLabel("Remove \(setting.label)")
					}
					Button("Choose\u{2026}") {
						if let path = choose() {
							model.set(path, for: setting)
						}
					}
					.buttonStyle(.borderless)
					.foregroundStyle(Color.accentColor)
				}
			}
			#endif
		case .hotkey:
			// A key combination, or a controller button for the shortcuts
			// the emulator also answers on a gamepad.
			LabeledContent {
				HotkeyField(label: setting.label, value: value, buttons: HotkeyCatalog.takesButtons(setting.id),
					modifiers: HotkeyCatalog.takesModifiers(setting.id),
					set: { model.set($0, for: setting) })
					.labelsHidden()
			} label: {
				rowLabel
			}
		case .string:
			// An empty field shows its placeholder. A Mac's grouped form
			// draws fields without a border, which read as fixed labels:
			// these are typed in, so they get one.
			LabeledContent {
				draftField(prompt: Text("None"))
					#if os(macOS)
					.textFieldStyle(.roundedBorder)
					.multilineTextAlignment(.leading)
					.frame(maxWidth: 180)
					#else
					.multilineTextAlignment(.trailing)
					#endif
			} label: {
				rowLabel
			}
		}
	}

	/// Typed text applies when it is submitted or the field is left, not as
	/// each letter is typed (a half-typed folder must not become the
	/// emulator's folder), and is not lost when the settings close. A
	/// number is typed as it reads (`SettingDescriptor.shown`).
	private func draftField(prompt: Text? = nil) -> some View {
		TextField(setting.label, text: $draft, prompt: prompt)
			.labelsHidden()
			.focused($editing)
			.onSubmit { commitDraft() }
			.onChange(of: editing) { if !editing { commitDraft() } }
			.onDisappear { commitDraft() }
			.onAppear { draft = displayed(value) }
			.onChange(of: value) { if !editing { draft = displayed(value) } }
	}

	private var isNumber: Bool { setting.kind == .integer || setting.kind == .real }

	/// A value as its field shows it.
	private func displayed(_ value: String) -> String {
		isNumber ? setting.descriptor.shownText(value) : value
	}

	private func commitDraft() {
		guard draft != displayed(value) else { return }
		var typed = draft
		if isNumber {
			let range = setting.descriptor.range ?? 0...0
			guard let number = setting.descriptor.storedNumber(typed: draft, in: range) else {
				model.notice = "\(setting.label): \(setting.descriptor.rangeSentence(range))"
				draft = displayed(value)
				return
			}
			typed = setting.descriptor.storedText(number)
		}
		if typed != value, !model.set(typed, for: setting) {
			draft = displayed(value)
		}
	}

	/// The path picked (and granted) for the setting, nil when cancelled.
	private func choose() -> String? {
		switch setting.id {
		case "retroarch_config":
			return FilePanels.choose(.file, title: setting.label, startingAt: value)
		case "logfile_path":
			// The log is written, not opened: a folder for it, keeping the
			// file name.
			let name = value.isEmpty ? "amiberry.log" : (value as NSString).lastPathComponent
			return FilePanels.chooseFolder(title: setting.label, startingAt: (value as NSString).deletingLastPathComponent)
				.map { ($0 as NSString).appendingPathComponent(name) }
		default:
			return FilePanels.chooseFolder(title: setting.label, startingAt: value)
		}
	}
}

/// The WHDLoad group's on-demand files: WHDLoad by Bert Jahn (whdload.de),
/// the kickstart RTB files, JST and the optional Amiga libraries (the XPK
/// packer stack, MMULib's 68040.library, explode.library and OctaMED's
/// medplayer.library) from Aminet are not bundled with Amiberry — their
/// licences forbid redistribution — so this downloads or updates them,
/// with progress and cancellation, as the Paths panel's "Update WHDBooter
/// files" button does.
@MainActor
@Observable
private final class WHDBootFilesModel {
	private(set) var status = WHDBootTools.status()
	private(set) var running = false
	private(set) var progressText: String?
	private(set) var resultText: String?

	/// One download at a time, shared with the Library: a second start (or
	/// one while a machine runs) is refused with a reason.
	func start() {
		guard !running else { return }
		guard let task = WHDBootDownloadController.shared.start(onProgress: { [weak self]
			label, step, totalSteps, bytesDone, bytesTotal in
			let text = WHDBootTools.progressText(label: label, step: step, totalSteps: totalSteps,
				bytesDone: bytesDone, bytesTotal: bytesTotal)
			Task { @MainActor in
				self?.progressText = text
			}
		}) else {
			resultText = WHDBootTools.status().emulationRunning
				? "Stop the machine to download the WHDBooter files."
				: "A WHDBooter download is already in progress."
			return
		}
		running = true
		progressText = nil
		resultText = nil
		Task { @MainActor in
			finish(await task.value)
		}
	}

	func cancel() {
		WHDBootDownloadController.shared.cancel()
	}

	private func finish(_ outcome: WHDBootDownloadOutcome) {
		running = false
		status = WHDBootTools.status()
		switch outcome {
		case .success:
			progressText = nil
			resultText = "WHDBooter files updated."
		case .partialFailure:
			progressText = nil
			resultText = "Some WHDBooter files failed to download. Check the log for details."
		case .failure:
			progressText = nil
			resultText = "The WHDBooter files could not be downloaded. Check the log for details."
		case .cancelled:
			progressText = nil
			resultText = "Download cancelled."
		case .unavailable:
			progressText = nil
			resultText = "Downloads are not available in this build."
		case .busy:
			progressText = nil
			resultText = "Another WHDBooter download is already running, or the machine is running."
		}
	}
}

private struct WHDBootFilesRow: View {
	@State private var model = WHDBootFilesModel()

	/// A machine that runs unpaused holds the archive state the download
	/// uses (a paused machine, held by this settings window, is fine).
	private var machineRunning: Bool {
		WHDBootTools.status().emulationRunning
	}

	var body: some View {
		VStack(alignment: .leading, spacing: 8) {
			HStack {
				Button(model.status.whdloadInstalled ? "Update WHDLoad Files" : "Download WHDLoad Files…") {
					model.start()
				}
				.disabled(model.running || machineRunning)
				if model.running {
					Button("Cancel") { model.cancel() }
					Spacer()
					ProgressView()
						.help("Downloading the WHDBooter files")
				}
			}
			if machineRunning && !model.running {
				Text("Stop the machine to download the WHDBooter files.")
					.font(.footnote)
					.foregroundStyle(.secondary)
			}
			if let progress = model.progressText {
				Text(progress).font(.footnote).foregroundStyle(.secondary)
			} else if let result = model.resultText {
				Text(result).font(.footnote).foregroundStyle(.secondary)
			}
		Text("WHDLoad by Bert Jahn — whdload.de. Kickstart RTB files, JST and the "
			+ "optional Amiga libraries — the XPK packer stack by Dirk Stöcker, MMULib's "
			+ "68040.library by Thomas Richter, explode.library by Struijk/Brouwer and "
			+ "OctaMED's medplayer.library by Teijo Kinnunen — Aminet. "
			+ "host-quit, the boot disk and the game database — the Amiberry project. "
			+ "None of the downloaded files are bundled with or covered by Amiberry's "
			+ "licence.")
			.font(.footnote)
			.foregroundStyle(.secondary)
		Text(model.status.librariesInstalled
			? "Optional libraries: installed."
			: "Optional libraries: not installed yet — the download installs them.")
			.font(.footnote)
			.foregroundStyle(.secondary)
		}
	}
}
