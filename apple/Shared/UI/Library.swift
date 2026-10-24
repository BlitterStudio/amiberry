/*
 * Copyright (C) 2026 BlitterStudio
 * The Library: the saved machines (configurations) the apps open to.
 * Playing one loads it as the pending configuration and starts it;
 * editing one loads it and opens the editor (SettingsRootView).
 */

import SwiftUI

/// A saved configuration, shown as a machine.
struct Machine: Identifiable, Hashable {
	let path: String
	/// The file name without its extension.
	let name: String
	let description: String
	let category: String
	var id: String { path }

	/// The machine the pending configuration came from (loaded or saved),
	/// when its file is still there.
	@MainActor
	static func current() -> Machine? {
		guard let path = MediaLibrary.currentConfiguration(), FileManager.default.fileExists(atPath: path) else {
			return nil
		}
		let info = MediaLibrary.configurationInfo(path)
		let name = ((path as NSString).lastPathComponent as NSString).deletingPathExtension
		return Machine(path: path, name: name, description: info?.description ?? "", category: info?.category ?? "")
	}
}

/// What can be played straight from a file (dropped, opened or chosen).
enum QuickPlay {
	static let extensions: Set<String> = ["adf", "adz", "dms", "ipf", "zip", "7z", "lha", "cue", "iso", "chd", "uae", "rp9"]

	static func accepts(_ url: URL) -> Bool {
		url.isFileURL && extensions.contains(url.pathExtension.lowercased())
	}

	/// The path the emulator plays. A Mac keeps access to the file itself
	/// (granted for later launches on a Mac App Store build); an iOS app
	/// only to its own files, so one from elsewhere is copied in first.
	@MainActor
	static func prepare(_ url: URL) throws -> String {
		#if os(iOS)
		if AppFiles.contains(url) {
			return url.path
		}
		return try AppFiles.importFile(url, into: AppFiles.folder(forMedia: url))
		#else
		_ = MediaLibrary.grantAccess(to: url.path)
		return url.path
		#endif
	}
}

@MainActor
@Observable
final class LibraryModel {
	private(set) var machines: [Machine] = []
	/// What went wrong with the last request, shown in the Library.
	var notice: String?
	/// While a machine is being loaded.
	private(set) var busy = false
	/// While an editor opened from the Library is up: the Library waits, so
	/// another load cannot replace the machine being edited.
	var editing = false
	/// Whether an emulation runs (a machine played now restarts it).
	let emulating: Bool

	@ObservationIgnored private let finish: () -> Void
	@ObservationIgnored private let openEditor: (_ machine: Machine?, _ preset: (id: String, configuration: Int)?) -> Void

	/// finish closes the Library (the emulator then runs the pending
	/// configuration); openEditor shows the editor for a machine loaded as
	/// the pending configuration, or for a new one (with a model applied).
	init(
		emulating: Bool, finish: @escaping () -> Void,
		openEditor: @escaping (_ machine: Machine?, _ preset: (id: String, configuration: Int)?) -> Void
	) {
		self.emulating = emulating
		self.finish = finish
		self.openEditor = openEditor
		reload()
	}

	/// The categories machines declare, sorted; machines without one are
	/// listed under "Other".
	var categories: [String] {
		Set(machines.map(\.category).filter { !$0.isEmpty }).sorted { $0.localizedStandardCompare($1) == .orderedAscending }
	}

	func machines(in category: String?) -> [Machine] {
		guard let category else { return machines }
		return machines.filter { $0.category == category }
	}

	func reload() {
		machines = MediaLibrary.entries(.configurations).map { entry in
			let info = MediaLibrary.configurationInfo(entry.path)
			let name = (entry.name as NSString).deletingPathExtension
			return Machine(path: entry.path, name: name, description: info?.description ?? "",
				category: info?.category ?? "")
		}
	}

	/// Loads the machine and starts it (restarts a running emulation).
	func play(_ machine: Machine) {
		guard !editing else { return }
		Task { @MainActor in
			guard await run(.loadConfiguration(path: machine.path)) else { return }
			if emulating, !(await run(.applySettings(.restart))) {
				return
			}
			finish()
		}
	}

	/// Quick play: the machine for a media file (a disk, CD, WHDLoad archive
	/// or configuration) runs as a played machine does. The emulator picks
	/// the machine (amiberry_actions_quick_play).
	func quickPlay(_ url: URL) {
		guard !editing else {
			// Said now: the Library shows the editor's banner meanwhile.
			Alerts.show(title: "Close the Editor First",
				message: "\u{201C}\(url.lastPathComponent)\u{201D} would replace the machine being edited. Close the editor, then open the file again.",
				buttons: ["OK"])
			return
		}
		guard QuickPlay.accepts(url) else {
			notice = "\u{201C}\(url.lastPathComponent)\u{201D} is not a disk, CD, WHDLoad archive or configuration."
			return
		}
		let path: String
		do {
			path = try QuickPlay.prepare(url)
		} catch {
			notice = "\u{201C}\(url.lastPathComponent)\u{201D} could not be opened: \(error.localizedDescription)"
			return
		}
		Task { @MainActor in
			guard await run(.quickPlay(path: path)) else { return }
			if emulating, !(await run(.applySettings(.restart))) {
				return
			}
			finish()
		}
	}

	/// Loads the machine and opens the editor on it.
	func edit(_ machine: Machine) {
		guard !editing else { return }
		Task { @MainActor in
			guard await run(.loadConfiguration(path: machine.path)) else { return }
			openEditor(machine, nil)
		}
	}

	/// A new machine from a model: the editor opens with it applied.
	func newMachine(preset: String? = nil, configuration: Int = 0) {
		guard !editing else { return }
		openEditor(nil, preset.map { ($0, configuration) })
	}

	/// The state saved as the iOS app last went to the background, when
	/// there is one.
	var autosave: (path: String, date: Date)? {
		emulating ? nil : SettingsWindowModel.autosaveState()
	}

	/// Restores the autosave state; the emulator closes the Library as the
	/// state loads.
	func resumeAutosave(_ path: String) {
		Task { @MainActor in
			_ = await run(.loadState(path: path))
		}
	}

	func delete(_ machine: Machine) {
		do {
			try FileManager.default.removeItem(atPath: machine.path)
		} catch {
			notice = "\(machine.name) could not be deleted: \(error.localizedDescription)"
		}
		reload()
	}

	/// Runs the action and waits for it; false (with a notice) when it was
	/// refused or failed.
	private func run(_ action: EmulatorAction) async -> Bool {
		guard let ticket = EmulatorActions.post(action) else {
			notice = "The emulator did not accept the request. Try again."
			return false
		}
		busy = true
		defer { busy = false }
		while true {
			switch ticket.poll() {
			case .pending:
				try? await Task.sleep(for: .milliseconds(20))
			case let .failed(message):
				notice = message
				return false
			case .done, .unknown:
				notice = nil
				return true
			}
		}
	}
}

/// Which machines the Library lists.
enum LibraryFilter: Hashable {
	case all
	case category(String)

	var title: String {
		switch self {
		case .all: return "All Machines"
		case let .category(name): return name
		}
	}

	var category: String? {
		if case let .category(name) = self { return name }
		return nil
	}
}

struct LibraryView: View {
	@Bindable var model: LibraryModel
	@State private var filter: LibraryFilter? = .all
	@State private var pendingDelete: Machine?
	#if os(iOS)
	@Environment(\.horizontalSizeClass) private var sizeClass
	/// The app settings sheet (the Mac has its Settings window).
	@State private var showingAppSettings = false
	#endif

	var body: some View {
		navigation
		.safeAreaInset(edge: .bottom, spacing: 0) {
			if model.editing {
				Text("Close the editor to play or edit another machine.")
					.foregroundStyle(.secondary)
					.frame(maxWidth: .infinity, alignment: .leading)
					.padding(.horizontal, 16)
					.padding(.vertical, 8)
					.background(.bar)
			} else if let notice = model.notice {
				Text(notice)
					.foregroundStyle(.red)
					.frame(maxWidth: .infinity, alignment: .leading)
					.padding(.horizontal, 16)
					.padding(.vertical, 8)
					.background(.bar)
			}
		}
		.disabled(model.busy || model.editing)
		// Quick play: a disk, CD, WHDLoad archive or configuration dropped on
		// the Library runs at once.
		.dropDestination(for: URL.self) { urls, _ in
			guard let url = urls.first(where: QuickPlay.accepts) else { return false }
			model.quickPlay(url)
			return true
		}
		#if os(iOS)
		// The Configurations folder may have changed meanwhile.
		.sheet(isPresented: $showingAppSettings, onDismiss: { model.reload() }) {
			NavigationStack {
				AppSettingsView(model: AppSettingsModel())
					.navigationTitle("Settings")
					.toolbar {
						ToolbarItem(placement: .confirmationAction) {
							Button("Done") { showingAppSettings = false }
						}
					}
			}
		}
		#endif
		.confirmationDialog(
			"Delete \u{201C}\(pendingDelete?.name ?? "")\u{201D}?",
			isPresented: Binding(get: { pendingDelete != nil }, set: { if !$0 { pendingDelete = nil } }),
			titleVisibility: .visible
		) {
			Button("Delete", role: .destructive) {
				if let machine = pendingDelete { model.delete(machine) }
				pendingDelete = nil
			}
			Button("Cancel", role: .cancel) { pendingDelete = nil }
		} message: {
			Text("The configuration file is removed.")
		}
	}

	@ViewBuilder
	private var navigation: some View {
		#if os(iOS)
		if sizeClass == .compact {
			// An iPhone lists the machines at once; categories filter them.
			NavigationStack {
				content
					.navigationTitle((filter ?? .all).title)
					.toolbar {
						ToolbarItem(placement: .topBarLeading) { categoryMenu }
						ToolbarItem { appSettingsButton }
						ToolbarItem { newMachineMenu }
					}
			}
		} else {
			splitView
		}
		#else
		splitView
		#endif
	}

	private var splitView: some View {
		NavigationSplitView {
			List(selection: $filter) {
				Section("Machines") {
					Label("All Machines", systemImage: "square.grid.2x2").tag(LibraryFilter.all)
					ForEach(model.categories, id: \.self) { name in
						Label(name, systemImage: "folder").tag(LibraryFilter.category(name))
					}
				}
			}
			.navigationSplitViewColumnWidth(min: 170, ideal: 200)
			.navigationTitle("Library")
		} detail: {
			content
				.navigationTitle((filter ?? .all).title)
				.toolbar {
					#if os(iOS)
					ToolbarItem { appSettingsButton }
					#endif
					ToolbarItem { newMachineMenu }
					ToolbarItem {
						Button { model.reload() } label: { Label("Refresh", systemImage: "arrow.clockwise") }
					}
				}
		}
	}

	#if os(iOS)
	private var appSettingsButton: some View {
		Button { showingAppSettings = true } label: { Label("Settings", systemImage: "gearshape") }
			.accessibilityIdentifier("library.settings")
	}
	#endif

	private var newMachineMenu: some View {
		Menu {
			ForEach(MachinePresets.all) { preset in
				Button(preset.label) { model.newMachine(preset: preset.id) }
			}
			Divider()
			Button("Play a File\u{2026}") {
				if let path = FilePanels.choose(.media, title: "Play a disk, CD, WHDLoad archive or configuration", startingAt: "") {
					model.quickPlay(URL(fileURLWithPath: path))
				}
			}
		} label: {
			Label("New Machine", systemImage: "plus")
		}
	}

	private var categoryMenu: some View {
		Menu {
			Picker("Category", selection: $filter) {
				Text("All Machines").tag(LibraryFilter?.some(.all))
				ForEach(model.categories, id: \.self) { name in
					Text(name).tag(LibraryFilter?.some(.category(name)))
				}
			}
		} label: {
			Label("Category", systemImage: "line.3.horizontal.decrease.circle")
		}
	}

	private var content: some View {
		VStack(spacing: 0) {
			sections
		}
	}

	@ViewBuilder
	private var sections: some View {
		let machines = model.machines(in: (filter ?? .all).category)
		if let autosave = model.autosave {
			HStack {
				VStack(alignment: .leading) {
					Text("Resume your last session").font(.headline)
					Text("Saved \(autosave.date.formatted(date: .abbreviated, time: .shortened)), when Amiberry last went to the background.")
						.font(.callout)
						.foregroundStyle(.secondary)
				}
				Spacer()
				Button("Resume") { model.resumeAutosave(autosave.path) }
					.buttonStyle(.borderedProminent)
			}
			.padding(12)
			.background(RoundedRectangle(cornerRadius: 10).fill(.quaternary))
			.padding([.horizontal, .top], 16)
		}
		if machines.isEmpty {
			EmptyLibraryView(model: model)
		} else {
			ScrollView {
				LazyVGrid(columns: [GridItem(.adaptive(minimum: 220), spacing: 16)], spacing: 16) {
					ForEach(machines) { machine in
						MachineCard(machine: machine, model: model, delete: { pendingDelete = machine })
					}
				}
				.padding(16)
			}
		}
	}
}

struct MachineCard: View {
	let machine: Machine
	let model: LibraryModel
	let delete: () -> Void

	var body: some View {
		VStack(alignment: .leading, spacing: 6) {
			Text(machine.name).font(.headline).lineLimit(1)
			Text(machine.description.isEmpty ? " " : machine.description)
				.font(.callout)
				.foregroundStyle(.secondary)
				.lineLimit(2, reservesSpace: true)
			HStack {
				Button { model.play(machine) } label: { Label("Play", systemImage: "play.fill") }
					.buttonStyle(.borderedProminent)
					.accessibilityIdentifier("library.play.\(machine.name)")
				Button("Edit") { model.edit(machine) }
					.accessibilityIdentifier("library.edit.\(machine.name)")
				Spacer()
				if !machine.category.isEmpty {
					Text(machine.category).font(.caption).foregroundStyle(.secondary)
				}
			}
		}
		.padding(12)
		.frame(maxWidth: .infinity, alignment: .leading)
		.background(RoundedRectangle(cornerRadius: 10).strokeBorder(.quaternary))
		.contextMenu {
			Button("Play") { model.play(machine) }
			Button("Edit") { model.edit(machine) }
			Divider()
			Button("Delete\u{2026}", role: .destructive, action: delete)
		}
	}
}

/// No saved machines yet: start from a model.
struct EmptyLibraryView: View {
	let model: LibraryModel

	var body: some View {
		VStack(spacing: 16) {
			Image(systemName: "desktopcomputer").font(.system(size: 44)).foregroundStyle(.secondary)
			Text("No saved machines yet").font(.title3)
			Text("Start from a model, set it up, then save it to find it here.")
				.foregroundStyle(.secondary)
				.multilineTextAlignment(.center)
			HStack {
				ForEach(MachinePresets.all.prefix(4)) { preset in
					Button(preset.label) { model.newMachine(preset: preset.id) }
				}
			}
		}
		.padding(32)
		.frame(maxWidth: .infinity, maxHeight: .infinity)
	}
}
