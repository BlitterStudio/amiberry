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

/// ROM files to import: those chosen, and those of chosen folders the ROM
/// scan reads (ROM-sized, with a ROM's extension), with Amiga Forever's
/// rom.key, which decodes its encrypted ones.
enum ROMImport {
	static let largestROM = 2 * 1024 * 1024
	/// As the emulator's scan takes them (isromext), and U plus a digit.
	static let extensions: Set<String> = ["rom", "bin", "a1000", "a500", "a600", "a1200", "a3000", "a4000", "cdtv", "cd32", "roz"]

	#if os(iOS)
	static let prompt = "Choose Kickstart ROM files"
	#else
	static let prompt = "Choose Kickstart ROM files or a folder of them"
	#endif

	static func isScanned(_ file: URL) -> Bool {
		let name = file.lastPathComponent.lowercased()
		if name == "rom.key" {
			return true
		}
		let ext = file.pathExtension.lowercased()
		return extensions.contains(ext) || (ext.count >= 2 && ext.first == "u" && ext.dropFirst().first?.isNumber == true)
	}

	static func files(in urls: [URL]) -> [URL] {
		urls.flatMap { url -> [URL] in
			let scoped = url.startAccessingSecurityScopedResource()
			defer {
				if scoped {
					url.stopAccessingSecurityScopedResource()
				}
			}
			// Asked of the file: a folder's URL need not end in a slash.
			guard (try? url.resourceValues(forKeys: [.isDirectoryKey]))?.isDirectory == true else { return [url] }
			let keys: [URLResourceKey] = [.isRegularFileKey, .fileSizeKey]
			guard let walk = FileManager.default.enumerator(at: url, includingPropertiesForKeys: keys,
				options: [.skipsHiddenFiles, .skipsPackageDescendants]) else { return [] }
			return walk.compactMap { $0 as? URL }.filter { file in
				guard let values = try? file.resourceValues(forKeys: Set(keys)), values.isRegularFile == true else { return false }
				return isScanned(file) && (values.fileSize ?? 0) <= largestROM
			}
		}
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
		guard FilePanels.grantCompanions(of: url.path) else {
			throw CocoaError(.fileReadNoPermission)
		}
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
	/// Whether a Kickstart ROM is installed: until one is, the Library says
	/// machines boot the built-in AROS and offers to import ROMs.
	private(set) var kickstartInstalled = true
	/// What the last ROM import found.
	var romReport: String?
	/// While the WHDBooter files download (WHDLoad, RTB files, ...): the
	/// line the Library shows; nil otherwise.
	private(set) var downloadProgress: String?

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
		// ROMs copied into the ROMs folder meanwhile (with Files, or the
		// Finder) count at once: the emulator rescans only at its first start.
		_ = MediaLibrary.rescanROMs()
		kickstartInstalled = MachinePresets.anyKickstartInstalled()
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

		// A WHDLoad archive boots through WHDLoad (or JST with "Use JST
		// instead of WHDLoad" on), which is not bundled with Amiberry: its
		// licence forbids redistribution. Offer the download before playing;
		// nothing is fetched without the user asking for it. Other media
		// (disks, CDs, configurations) never needs the tools.
		if ["lha", "lzh"].contains(url.pathExtension.lowercased()),
			let missingTool = missingWHDBootTool() {
			offerWHDBootDownload(tool: missingTool) { [weak self] in
				self?.startQuickPlay(path: path)
			}
			return
		}
		startQuickPlay(path: path)
	}

	private func startQuickPlay(path: String) {
		Task { @MainActor in
			guard await run(.quickPlay(path: path)) else { return }
			if emulating, !(await run(.applySettings(.restart))) {
				return
			}
			finish()
		}
	}

	/// The WHDLoad tool the next boot needs and does not have: "WHDLoad",
	/// "JST" (when it is selected instead) or nil when everything is in place.
	private func missingWHDBootTool() -> String? {
		let status = WHDBootTools.status()
		guard status.downloadsAvailable else { return nil }
		let useJST = (try? HostSettings.value(of: "use_jst_instead_of_whd")) == "true"
		if useJST {
			return status.jstInstalled ? nil : "JST"
		}
		return status.whdloadInstalled ? nil : "WHDLoad"
	}

	/// Asks for the on-demand WHDBoot files and downloads them; `then`
	/// runs (with the files in place) only when the tool is installed.
	private func offerWHDBootDownload(tool: String, then: @escaping () -> Void) {
		if WHDBootTools.status().emulationRunning {
			notice = "Stop the machine before downloading \(tool)."
			return
		}
		let source = tool == "JST" ? "Aminet" : "whdload.de"
		let (choice, _) = Alerts.show(title: "Download \(tool)?",
			message: "\(tool) is needed to boot WHDLoad archives, but it is not installed. "
				+ "\(tool) comes from \(source) and is not bundled with Amiberry.\n\n"
				+ "Download it now, with the kickstart RTB files and the other WHDBooter files?",
			buttons: ["Download", "Cancel"])
		guard choice == 0 else { return }
		Task { @MainActor in
			guard await downloadWHDBootFiles(named: tool) else { return }
			then()
		}
	}

	/// Downloads the WHDBooter files, showing progress in the Library.
	/// False when they failed (or were cancelled) and the tool is missing.
	private func downloadWHDBootFiles(named tool: String) async -> Bool {
		busy = true
		defer { busy = false }
		guard let task = WHDBootDownloadController.shared.start(onProgress: { [weak self]
			label, step, totalSteps, bytesDone, bytesTotal in
			let text = WHDBootTools.progressText(label: label, step: step, totalSteps: totalSteps,
				bytesDone: bytesDone, bytesTotal: bytesTotal)
			Task { @MainActor in
				self?.downloadProgress = text
			}
		}) else {
			downloadProgress = nil
			notice = WHDBootTools.status().emulationRunning
				? "Stop the machine before downloading \(tool)."
				: "A WHDBooter download is already in progress."
			return false
		}
		let outcome = await task.value
		downloadProgress = nil
		let status = WHDBootTools.status()
		let installed = tool == "JST" ? status.jstInstalled : status.whdloadInstalled
		if !installed {
			if case .busy = outcome {
				notice = "Stop the machine before downloading \(tool)."
			} else {
				let source = tool == "JST" ? "Aminet" : "whdload.de"
				notice = "\(tool) could not be downloaded from \(source). "
					+ "Try again from App Settings \u{2192} WHDLoad."
			}
			return false
		}
		if case .partialFailure = outcome {
			notice = "Some WHDBooter files failed to download; \(tool) itself is installed."
		}
		return true
	}

	/// Copies ROM files (and the ROM-sized files of folders) into the ROMs
	/// folder and rescans it; says what the scan found.
	func importROMs(_ urls: [URL]) {
		// Cancelled.
		guard !urls.isEmpty else { return }
		// A chosen folder's access covers its files only while it is held
		// (a Mac App Store build): held until they are copied.
		let scoped = urls.filter { $0.startAccessingSecurityScopedResource() }
		defer { scoped.forEach { $0.stopAccessingSecurityScopedResource() } }
		let files = ROMImport.files(in: urls)
		guard !files.isEmpty else {
			romReport = "No ROM files were chosen."
			return
		}
		// The list counts the built-in ROMs too: what is new is the difference.
		let before = MediaLibrary.rescanROMs()
		do {
			let folder = URL(fileURLWithPath: try MediaLibrary.path(of: .roms), isDirectory: true)
			for file in files {
				_ = try AppFiles.importFile(file, into: folder)
			}
		} catch {
			romReport = "The ROMs could not be copied: \(error.localizedDescription)"
			return
		}
		let added = MediaLibrary.rescanROMs() - before
		kickstartInstalled = MachinePresets.anyKickstartInstalled()
		let runnable = MachinePresets.all.filter { MachinePresets.computerIDs.contains($0.id) && MachinePresets.isRunnable($0) }
		if added <= 0 {
			romReport = "No new ROMs found in those files."
		} else if runnable.isEmpty {
			romReport = "\(added) ROM\(added == 1 ? "" : "s") added, but no Kickstart: machines still boot AROS."
		} else {
			romReport = "Ready: " + runnable.map(\.label).joined(separator: ", ") + "."
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
			} else if let progress = model.downloadProgress {
				HStack(spacing: 8) {
					ProgressView()
					Text("Downloading WHDBooter files: \(progress)")
						.foregroundStyle(.secondary)
				}
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
					.safeAreaInset(edge: .top, spacing: 0) { romNotice }
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
				.safeAreaInset(edge: .top, spacing: 0) { romNotice }
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

	/// Under the toolbar, over the machines.
	@ViewBuilder private var romNotice: some View {
		if !model.kickstartInstalled || model.romReport != nil {
			ROMNotice(model: model)
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
				Button { model.newMachine(preset: preset.id) } label: {
					if MachinePresets.isRunnable(preset) {
						Text(preset.label)
					} else {
						Label("\(preset.label) (ROM not installed)", systemImage: "exclamationmark.triangle")
					}
				}
			}
			Divider()
			Button("Import ROMs\u{2026}") { model.importROMs(FilePanels.chooseROMs(title: ROMImport.prompt)) }
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

/// Until a Kickstart is installed: what machines boot meanwhile, and the
/// way to import ROMs; then what an import found.
struct ROMNotice: View {
	let model: LibraryModel

	var body: some View {
		HStack(alignment: .firstTextBaseline, spacing: 12) {
			Image(systemName: model.kickstartInstalled ? "checkmark.circle" : "memorychip")
				.foregroundStyle(model.kickstartInstalled ? Color.green : Color.orange)
			VStack(alignment: .leading, spacing: 2) {
				if !model.kickstartInstalled {
					Text("No Kickstart ROMs").font(.headline)
					Text("Machines boot the free AROS ROM. For the Amiga\u{2019}s own, import your Kickstart ROM files (from Amiga Forever or dumped from your Amiga).")
						.foregroundStyle(.secondary)
						.fixedSize(horizontal: false, vertical: true)
				}
				if let report = model.romReport {
					Text(report).font(.callout)
				}
			}
			Spacer(minLength: 8)
			if !model.kickstartInstalled {
				Button("Import ROMs\u{2026}") {
					model.importROMs(FilePanels.chooseROMs(title: ROMImport.prompt))
				}
				.accessibilityIdentifier("library.importROMs")
			} else {
				Button("OK") { model.romReport = nil }
			}
		}
		.padding(.horizontal, 16)
		.padding(.vertical, 10)
		.background(.bar)
	}
}
