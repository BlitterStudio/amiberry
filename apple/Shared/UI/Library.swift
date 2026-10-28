/*
 * Copyright (C) 2026 BlitterStudio
 * The Library: the saved machines (configurations) the apps open to.
 * Playing one loads it as the pending configuration and starts it;
 * editing one loads it and opens the editor (SettingsRootView). Its Media
 * shelves and Kickstart ROMs page are in MediaBrowser.swift.
 */

import SwiftUI
#if os(macOS)
import AppKit
#endif

/// A saved configuration, shown as a machine.
struct Machine: Identifiable, Hashable {
	let path: String
	/// The file name without its extension.
	let name: String
	let description: String
	let category: String
	var id: String { path }

	/// The colours a machine's icon is tinted with.
	static let tints: [Color] = [.blue, .indigo, .purple, .pink, .red, .orange, .green, .teal, .cyan, .brown]

	/// Which of the tints a name gets: picked with a fixed hash (FNV-1a of
	/// its UTF-8), so a machine keeps its colour from launch to launch,
	/// which Swift's hashValue (seeded per launch) would not.
	static func tintIndex(for name: String) -> Int {
		var hash: UInt64 = 0xcbf2_9ce4_8422_2325
		for byte in name.utf8 {
			hash ^= UInt64(byte)
			hash = hash &* 0x0000_0100_0000_01b3
		}
		return Int(hash % UInt64(tints.count))
	}

	static func tint(for name: String) -> Color {
		tints[tintIndex(for: name)]
	}

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
	/// The models (MachinePresets IDs) that have every ROM of one of their
	/// configurations installed.
	private(set) var readyModels: Set<String> = []
	/// The Media section's shelves.
	let media: MediaShelves

	@ObservationIgnored private let finish: () -> Void
	@ObservationIgnored private let openEditor: (_ machine: Machine?, _ preset: (id: String, configuration: Int)?) -> Void
	@ObservationIgnored private let remove: (URL) throws -> URL?

	/// finish closes the Library (the emulator then runs the pending
	/// configuration); openEditor shows the editor for a machine loaded as
	/// the pending configuration, or for a new one (with a model applied).
	/// remove takes a deleted machine's file away and returns where it went
	/// (nil when it is gone for good); tests pass one that keeps the file
	/// in their own folder.
	init(
		emulating: Bool, finish: @escaping () -> Void,
		openEditor: @escaping (_ machine: Machine?, _ preset: (id: String, configuration: Int)?) -> Void,
		remove: @escaping (URL) throws -> URL? = LibraryModel.removeFile
	) {
		self.emulating = emulating
		self.finish = finish
		self.openEditor = openEditor
		self.remove = remove
		media = MediaShelves()
		reload()
	}

	/// Moves the file to the Trash on a Mac, where it can be put back from,
	/// and returns its copy there; iOS has no Trash, so there it is removed.
	nonisolated static func removeFile(_ url: URL) throws -> URL? {
		#if os(macOS)
		var trashed: NSURL?
		try FileManager.default.trashItem(at: url, resultingItemURL: &trashed)
		return trashed as URL?
		#else
		try FileManager.default.removeItem(at: url)
		return nil
		#endif
	}

	/// The categories machines declare, sorted; machines without one are
	/// listed under "Other".
	var categories: [String] {
		Set(machines.map(\.category).filter { !$0.isEmpty }).sorted { $0.localizedStandardCompare($1) == .orderedAscending }
	}

	/// The machines of a category (every machine for nil) whose name,
	/// description or category contains each word of the query, ignoring
	/// case and accents as the Finder's search does; an empty query
	/// matches every machine.
	func machines(in category: String?, matching query: String = "") -> [Machine] {
		let terms = query.split(whereSeparator: \.isWhitespace).map(String.init)
		return machines.filter { machine in
			guard category == nil || machine.category == category else { return false }
			return terms.allSatisfy { term in
				machine.name.localizedStandardContains(term)
					|| machine.description.localizedStandardContains(term)
					|| machine.category.localizedStandardContains(term)
			}
		}
	}

	func reload() {
		// Sorted as the Finder sorts names: "Amiga 9" before "Amiga 10".
		machines = MediaLibrary.entries(.configurations).map { entry in
			let info = MediaLibrary.configurationInfo(entry.path)
			let name = (entry.name as NSString).deletingPathExtension
			return Machine(path: entry.path, name: name, description: info?.description ?? "",
				category: info?.category ?? "")
		}
		.sorted { $0.name.localizedStandardCompare($1.name) == .orderedAscending }
		// ROMs copied into the ROMs folder meanwhile (with Files, or the
		// Finder) count at once: the emulator rescans only at its first start.
		_ = MediaLibrary.rescanROMs()
		updateROMStatus()
		// Media added or removed meanwhile, as the machines.
		media.reload()
	}

	private func updateROMStatus() {
		kickstartInstalled = MachinePresets.anyKickstartInstalled()
		readyModels = Set(MachinePresets.all.filter { MachinePresets.isRunnable($0) }.map(\.id))
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

	/// Plays a disk or CD in a saved machine: loads the machine, puts the
	/// file in its first drive (DF0: or the CD drive) and starts it (restarts
	/// a running emulation). The file is recorded as played, as quick play
	/// records it.
	func play(_ media: URL, in machine: Machine) {
		guard !editing else { return }
		guard let shelf = MediaShelf.shelf(for: media), let setting = shelf.driveSetting,
			let recent = shelf.recentList else {
			notice = "\u{201C}\(media.lastPathComponent)\u{201D} is not a disk or CD image."
			return
		}
		let path: String
		do {
			path = try QuickPlay.prepare(media)
		} catch {
			notice = "\u{201C}\(media.lastPathComponent)\u{201D} could not be opened: \(error.localizedDescription)"
			return
		}
		Task { @MainActor in
			guard await run(.loadConfiguration(path: machine.path)) else { return }
			guard insert(path, into: setting, machine: machine) else { return }
			try? MediaLibrary.addRecent(path, to: recent)
			if emulating, !(await run(.applySettings(.restart))) {
				return
			}
			finish()
		}
	}

	/// Sets the drive of the pending configuration to the file; false (with
	/// a notice) when the layer refused it, or for a CD when the machine has
	/// no CD drive: an empty cdimage0 is no drive, and setting it would add
	/// one the machine was never set up for.
	private func insert(_ path: String, into setting: String, machine: Machine) -> Bool {
		do {
			let store = try SettingsStore()
			if setting == "cdimage0", (store.value(of: setting) ?? "").isEmpty {
				notice = "\(machine.name) has no CD drive. Play the CD on its own, or in a CD32 or CDTV machine."
				return false
			}
			guard store.set(path, for: setting) else {
				notice = store.lastError.map { SettingsWindowModel.describe($0) } ?? "\(setting) could not be set."
				return false
			}
			try store.commit()
			return true
		} catch {
			notice = SettingsWindowModel.describe(error)
			return false
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
		updateROMStatus()
		let runnable = MachinePresets.all.filter { MachinePresets.computerIDs.contains($0.id) && readyModels.contains($0.id) }
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

	/// Asks for a disk, CD, WHDLoad archive or configuration and plays it.
	func chooseFileToPlay() {
		if let path = FilePanels.choose(.media, title: "Play a disk, CD, WHDLoad archive or configuration", startingAt: "") {
			quickPlay(URL(fileURLWithPath: path))
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

	/// Takes the machine's file away with remove (to the Trash on a Mac).
	/// Where the file went: its copy in the Trash, or nil when it was
	/// removed (iOS) or could not be (with a notice).
	@discardableResult
	func delete(_ machine: Machine) -> URL? {
		// Read again whatever happened: a file that could not be removed may
		// have gone meanwhile.
		defer { reload() }
		do {
			return try remove(URL(fileURLWithPath: machine.path))
		} catch {
			#if os(macOS)
			notice = "\(machine.name) could not be moved to the Trash: \(error.localizedDescription)"
			#else
			notice = "\(machine.name) could not be deleted: \(error.localizedDescription)"
			#endif
			return nil
		}
	}

	/// Renames the machine's file, keeping its extension (Workbench.UAE
	/// stays .UAE), by the editor's Save rules
	/// (`SettingsWindowModel.nameProblem`) and not to another machine's name.
	/// The renamed machine, or nil (with a notice) when it was refused or
	/// could not be renamed.
	@discardableResult
	func rename(_ machine: Machine, to name: String) -> Machine? {
		let trimmed = name.trimmingCharacters(in: .whitespaces)
		if let problem = SettingsWindowModel.nameProblem(trimmed) {
			notice = problem
			return nil
		}
		guard trimmed != machine.name else { return machine }
		let source = URL(fileURLWithPath: machine.path)
		let ext = source.pathExtension.isEmpty ? "uae" : source.pathExtension
		let target = source.deletingLastPathComponent().appendingPathComponent(trimmed).appendingPathExtension(ext)
		// A change of case only ("workbench" to "Workbench") finds the file
		// itself on a case-insensitive volume: that one is not taken.
		if FileManager.default.fileExists(atPath: target.path), !LibraryModel.sameFile(source, target) {
			notice = "A machine named \u{201C}\(trimmed)\u{201D} already exists. Choose another name."
			return nil
		}
		do {
			try FileManager.default.moveItem(at: source, to: target)
		} catch {
			notice = "\(machine.name) could not be renamed: \(error.localizedDescription)"
			return nil
		}
		reload()
		let renamed = (target.path as NSString).standardizingPath
		return machines.first { ($0.path as NSString).standardizingPath == renamed }
	}

	/// Whether both URLs name one file (as two spellings of a name do on a
	/// case-insensitive volume).
	private static func sameFile(_ a: URL, _ b: URL) -> Bool {
		let key: Set<URLResourceKey> = [.fileResourceIdentifierKey]
		guard let first = try? a.resourceValues(forKeys: key).fileResourceIdentifier,
			let second = try? b.resourceValues(forKeys: key).fileResourceIdentifier else { return false }
		return first.isEqual(second)
	}

	/// Copies the machine's file beside it, named as the Finder names a
	/// duplicate ("Name copy", then "Name copy 2", ...); the copy keeps the
	/// description and category the file declares. The copy, or nil (with
	/// a notice) when it could not be made.
	@discardableResult
	func duplicate(_ machine: Machine) -> Machine? {
		let source = URL(fileURLWithPath: machine.path)
		let folder = source.deletingLastPathComponent()
		let ext = source.pathExtension.isEmpty ? "uae" : source.pathExtension
		let base = LibraryModel.copyBase(of: machine.name)
		var number = 1
		var target: URL
		repeat {
			let name = number == 1 ? "\(base) copy" : "\(base) copy \(number)"
			target = folder.appendingPathComponent(name).appendingPathExtension(ext)
			number += 1
		} while FileManager.default.fileExists(atPath: target.path)
		do {
			try FileManager.default.copyItem(at: source, to: target)
		} catch {
			notice = "\(machine.name) could not be duplicated: \(error.localizedDescription)"
			return nil
		}
		reload()
		let copied = (target.path as NSString).standardizingPath
		return machines.first { ($0.path as NSString).standardizingPath == copied }
	}

	/// The name copies are numbered from, as the Finder numbers them: a
	/// copy's copy is "X copy 2", not "X copy copy".
	static func copyBase(of name: String) -> String {
		guard let range = name.range(of: #" copy( [0-9]+)?$"#, options: .regularExpression), range.lowerBound > name.startIndex else {
			return name
		}
		return String(name[..<range.lowerBound])
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

/// What the Library shows: machines (all of them or a category's), a Media
/// shelf, or the Kickstart ROMs.
enum LibraryFilter: Hashable {
	case all
	case category(String)
	case media(MediaShelf)
	case roms

	var title: String {
		switch self {
		case .all: return "All Machines"
		case let .category(name): return name
		case let .media(shelf): return shelf.title
		case .roms: return "Kickstart ROMs"
		}
	}

	var category: String? {
		if case let .category(name) = self { return name }
		return nil
	}

	/// The search field's prompt.
	var searchPrompt: String {
		switch self {
		case .all, .category: return "Search Machines"
		case let .media(shelf): return "Search \(shelf.title)"
		case .roms: return "Search Models"
		}
	}
}

struct LibraryView: View {
	@Bindable var model: LibraryModel
	@State private var filter: LibraryFilter? = .all
	@State private var pendingDelete: Machine?
	/// The machine being renamed, and the name its alert's field holds.
	@State private var pendingRename: Machine?
	@State private var newName = ""
	@State private var query = ""
	/// The machine picked in the grid (on the Mac): Return plays it, Delete
	/// asks to move it to the Trash, and the arrow keys move to another.
	@State private var selection: Machine.ID?
	/// How many cards a row of the grid holds, for the up and down arrows.
	@State private var columns = 1
	/// While a file is dragged over the Library.
	@State private var dropTargeted = false
	/// The "No Kickstart ROMs" advice was closed: machines run on AROS by
	/// choice, and the Kickstart ROMs page still offers the import.
	@AppStorage("ROMAdviceDismissed") private var romAdviceDismissed = false
	@FocusState private var gridFocused: Bool
	#if os(iOS)
	@Environment(\.horizontalSizeClass) private var sizeClass
	/// The app settings sheet (the Mac has its Settings window).
	@State private var showingAppSettings = false
	#endif

	/// The cards' narrowest width and the gap between them; the grid fits
	/// as many to a row as these allow.
	private static let cardWidth: CGFloat = 240
	private static let cardSpacing: CGFloat = 16

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
				LibraryBanner(systemImage: "exclamationmark.triangle.fill", tint: .orange, close: { model.notice = nil }) {
					Text(notice)
						.textSelection(.enabled)
						.fixedSize(horizontal: false, vertical: true)
				}
			}
		}
		.disabled(model.busy || model.editing)
		.overlay {
			if dropTargeted {
				DropToPlayOverlay()
			}
		}
		// Quick play: a disk, CD, WHDLoad archive or configuration dropped on
		// the Library runs at once.
		.dropDestination(for: URL.self) { urls, _ in
			guard let url = urls.first(where: QuickPlay.accepts) else { return false }
			model.quickPlay(url)
			return true
		} isTargeted: { targeted in
			dropTargeted = targeted
		}
		// A category whose last machine went lists nothing: back to them all.
		.onChange(of: model.categories) { _, categories in
			if let category = filter?.category, !categories.contains(category) {
				filter = .all
			}
		}
		// The app keeps itself current: machines saved, copied or removed in
		// the Finder or the Files app meanwhile (and media and ROMs added)
		// count as Amiberry comes back to the front.
		#if os(macOS)
		.onReceive(NotificationCenter.default.publisher(for: NSApplication.didBecomeActiveNotification)) { _ in
			model.reload()
		}
		#else
		.onReceive(NotificationCenter.default.publisher(for: UIApplication.didBecomeActiveNotification)) { _ in
			model.reload()
		}
		#endif
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
			MachineCard.deleteQuestion(pendingDelete?.name ?? ""),
			isPresented: Binding(get: { pendingDelete != nil }, set: { if !$0 { pendingDelete = nil } }),
			titleVisibility: .visible
		) {
			Button(MachineCard.deleteCommand, role: .destructive) {
				if let machine = pendingDelete { model.delete(machine) }
				pendingDelete = nil
			}
			Button("Cancel", role: .cancel) { pendingDelete = nil }
		} message: {
			#if os(macOS)
			Text("\u{201C}\(pendingDelete?.name ?? "")\u{201D} moves to the Trash; you can put it back from there.")
			#else
			Text("The configuration file is removed.")
			#endif
		}
		.alert(
			"Rename \u{201C}\(pendingRename?.name ?? "")\u{201D}",
			isPresented: Binding(get: { pendingRename != nil }, set: { if !$0 { pendingRename = nil } })
		) {
			TextField("Name", text: $newName)
				.accessibilityIdentifier("library.rename.name")
			Button("Rename") {
				// A refused name leaves the machine as it was; the notice says why.
				if let machine = pendingRename, let renamed = model.rename(machine, to: newName) {
					selection = renamed.id
				}
				pendingRename = nil
			}
			Button("Cancel", role: .cancel) { pendingRename = nil }
		}
	}

	/// The machines the grid shows: the chosen category's, narrowed by the
	/// search.
	private var shownMachines: [Machine] {
		model.machines(in: (filter ?? .all).category, matching: query)
	}

	@ViewBuilder
	private var navigation: some View {
		#if os(iOS)
		if sizeClass == .compact {
			// An iPhone lists the machines at once; the menu picks a
			// category, a Media shelf or the ROMs instead.
			NavigationStack {
				content
					.safeAreaInset(edge: .top, spacing: 0) { romNotice }
					.navigationTitle((filter ?? .all).title)
					.searchable(text: $query, prompt: (filter ?? .all).searchPrompt)
					.toolbar {
						ToolbarItem(placement: .topBarLeading) { categoryMenu }
						ToolbarItem { appSettingsButton }
						ToolbarItem { playFileButton }
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
					Label("All Machines", systemImage: "square.grid.2x2")
						.badge(model.machines.count)
						.tag(LibraryFilter.all)
					ForEach(model.categories, id: \.self) { name in
						Label(name, systemImage: "folder")
							.badge(model.machines(in: name).count)
							.tag(LibraryFilter.category(name))
					}
				}
				Section("Media") {
					ForEach(MediaShelf.allCases, id: \.self) { shelf in
						Label(shelf.title, systemImage: shelf.systemImage)
							.tag(LibraryFilter.media(shelf))
					}
				}
				Section("System") {
					Label("Kickstart ROMs", systemImage: "memorychip")
						.tag(LibraryFilter.roms)
				}
			}
			#if os(macOS)
			.navigationSplitViewColumnWidth(min: 170, ideal: 200)
			#endif
			.navigationTitle("Library")
		} detail: {
			detail
				.navigationTitle((filter ?? .all).title)
				#if os(macOS)
				.navigationSubtitle(subtitle)
				#else
				// A large title sits under the ROM notice's inset, blurred by
				// the bar: the title goes in the bar itself.
				.navigationBarTitleDisplayMode(.inline)
				#endif
				.searchable(text: $query, placement: Self.searchPlacement, prompt: (filter ?? .all).searchPrompt)
				.toolbar {
					#if os(iOS)
					ToolbarItem { appSettingsButton }
					#endif
					ToolbarItem { playFileButton }
					ToolbarItem { newMachineMenu }
				}
		}
	}

	/// In the Mac's toolbar, as the Finder's and Mail's searches are.
	private static var searchPlacement: SearchFieldPlacement {
		#if os(macOS)
		.toolbar
		#else
		.automatic
		#endif
	}

	/// How many machines are listed: "1 machine", "5 machines".
	private static func count(of machines: Int) -> String {
		machines == 1 ? "1 machine" : "\(machines) machines"
	}

	#if os(macOS)
	/// Under the title: how many machines or files are listed, or how
	/// many models are ready.
	private var subtitle: String {
		switch filter ?? .all {
		case .all, .category:
			return Self.count(of: shownMachines.count)
		case let .media(shelf):
			let items = (model.media.items[shelf] ?? []).matching(query).count
			return items == 1 ? "1 item" : "\(items) items"
		case .roms:
			let models = KickstartROMsView.models
			return "\(models.filter { model.readyModels.contains($0.id) }.count) of \(models.count) ready"
		}
	}
	#endif

	/// The detail column's content: the machines, with the ROM notice over
	/// them. On iOS the notice is a safe-area inset over the machines; on
	/// the Mac an inset on the split view's detail breaks its layout (the
	/// columns lay out at an oversized ideal height, their top - the
	/// sidebar's rows, the banner - above the visible window, and resizing
	/// does not bring it back), so there it rides at the top of the stack.
	@ViewBuilder private var detail: some View {
		#if os(iOS)
		content
			.safeAreaInset(edge: .top, spacing: 0) { romNotice }
		#else
		VStack(spacing: 0) {
			romNotice
			content
		}
		#endif
	}

	/// Under the toolbar, over the machines, until it is closed. The
	/// Kickstart ROMs page gives the advice itself: there only an import's
	/// report shows.
	@ViewBuilder private var romNotice: some View {
		let advice = filter != .roms && !romAdviceDismissed
		if (advice && !model.kickstartInstalled) || model.romReport != nil {
			ROMNotice(model: model, showsAdvice: advice, dismissAdvice: { romAdviceDismissed = true })
		}
	}

	#if os(iOS)
	private var appSettingsButton: some View {
		Button { showingAppSettings = true } label: { Label("Settings", systemImage: "gearshape") }
			.accessibilityIdentifier("library.settings")
	}
	#endif

	/// Making machines: one from each model, and the ROMs the models need.
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
		} label: {
			Label("New Machine", systemImage: "plus")
		}
		.help("New Machine")
	}

	private var playFileButton: some View {
		Button { model.chooseFileToPlay() } label: { Label("Play a File\u{2026}", systemImage: "play.square") }
			.help("Play a disk, CD, WHDLoad archive or configuration")
			.accessibilityIdentifier("library.playFile")
	}

	private var categoryMenu: some View {
		Menu {
			Picker("Show", selection: $filter) {
				Section("Machines") {
					Text("All Machines").tag(LibraryFilter?.some(.all))
					ForEach(model.categories, id: \.self) { name in
						Text(name).tag(LibraryFilter?.some(.category(name)))
					}
				}
				Section("Media") {
					ForEach(MediaShelf.allCases, id: \.self) { shelf in
						Label(shelf.title, systemImage: shelf.systemImage).tag(LibraryFilter?.some(.media(shelf)))
					}
				}
				Section("System") {
					Label("Kickstart ROMs", systemImage: "memorychip").tag(LibraryFilter?.some(.roms))
				}
			}
		} label: {
			Label("Category", systemImage: "line.3.horizontal.decrease.circle")
		}
	}

	/// The machines for a machine filter; a Media shelf or the ROMs page
	/// otherwise.
	@ViewBuilder
	private var content: some View {
		switch filter ?? .all {
		case .all, .category:
			VStack(spacing: 0) {
				sections
			}
		case let .media(shelf):
			// A shelf of its own: its selection and scan start afresh.
			MediaShelfView(model: model, shelf: shelf, query: query)
				.id(shelf)
		case .roms:
			KickstartROMsView(model: model, query: query)
		}
	}

	@ViewBuilder
	private var sections: some View {
		let machines = shownMachines
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
		if model.machines.isEmpty {
			EmptyLibraryView(model: model)
		} else if machines.isEmpty {
			ContentUnavailableView.search(text: query.trimmingCharacters(in: .whitespaces))
				.frame(maxWidth: .infinity, maxHeight: .infinity)
		} else {
			grid(of: machines)
		}
	}

	private func grid(of machines: [Machine]) -> some View {
		ScrollViewReader { scroller in
			ScrollView {
				LazyVGrid(columns: [GridItem(.adaptive(minimum: Self.cardWidth), spacing: Self.cardSpacing)],
					spacing: Self.cardSpacing) {
					ForEach(machines) { machine in
						MachineCard(
							machine: machine, model: model, selected: selection == machine.id,
							select: { select(machine) },
							duplicate: {
								if let copy = model.duplicate(machine) {
									selection = copy.id
								}
							},
							rename: {
								newName = machine.name
								pendingRename = machine
							},
							delete: { pendingDelete = machine })
					}
				}
				.onGeometryChange(for: CGFloat.self) { geometry in
					geometry.size.width
				} action: { width in
					// As the adaptive column lays them out.
					columns = max(1, Int((width + Self.cardSpacing) / (Self.cardWidth + Self.cardSpacing)))
				}
				.padding(16)
			}
			#if os(macOS)
			// The grid takes the keyboard as the Finder's icon view does.
			.focusable()
			.focused($gridFocused)
			.focusEffectDisabled()
			.onMoveCommand { direction in
				move(direction, in: machines, scroller: scroller)
			}
			.onKeyPress(.return) {
				guard let machine = machines.first(where: { $0.id == selection }) else { return .ignored }
				model.play(machine)
				return .handled
			}
			.onDeleteCommand {
				if let machine = machines.first(where: { $0.id == selection }) {
					pendingDelete = machine
				}
			}
			#else
			// Pulled down, the Library reads the folders again: files the
			// Files app added beside a running Amiberry (Split View).
			.refreshable { model.reload() }
			#endif
		}
	}

	private func select(_ machine: Machine) {
		selection = machine.id
		gridFocused = true
	}

	#if os(macOS)
	/// Left and right go to the previous and next machine, up and down to
	/// the one a row away; with none picked, any arrow picks the first.
	private func move(_ direction: MoveCommandDirection, in machines: [Machine], scroller: ScrollViewProxy) {
		guard !machines.isEmpty else { return }
		let step: Int
		switch direction {
		case .left: step = -1
		case .right: step = 1
		case .up: step = -columns
		case .down: step = columns
		@unknown default: return
		}
		let target: Int
		if let current = machines.firstIndex(where: { $0.id == selection }) {
			target = current + step
			guard machines.indices.contains(target) else { return }
		} else {
			target = 0
		}
		selection = machines[target].id
		withAnimation {
			scroller.scrollTo(machines[target].id)
		}
	}
	#endif
}

/// A saved machine in the grid: its icon, name, description and category,
/// with Play as its one prominent action. On the Mac a click picks it and
/// a double click plays it, as in the Finder.
struct MachineCard: View {
	let machine: Machine
	let model: LibraryModel
	let selected: Bool
	let select: () -> Void
	let duplicate: () -> Void
	let rename: () -> Void
	let delete: () -> Void
	@State private var hovering = false

	/// What removing a machine is called: a Mac moves it to the Trash, iOS
	/// (which has none) deletes it.
	#if os(macOS)
	static let deleteCommand = "Move to Trash"
	static func deleteQuestion(_ name: String) -> String { "Move \u{201C}\(name)\u{201D} to the Trash?" }
	#else
	static let deleteCommand = "Delete"
	static func deleteQuestion(_ name: String) -> String { "Delete \u{201C}\(name)\u{201D}?" }
	#endif

	var body: some View {
		let shape = RoundedRectangle(cornerRadius: 12, style: .continuous)
		VStack(alignment: .leading, spacing: 10) {
			HStack(alignment: .top, spacing: 12) {
				MachineIcon(name: machine.name)
				VStack(alignment: .leading, spacing: 2) {
					Text(machine.name).font(.headline).lineLimit(1)
					Text(machine.description.isEmpty ? " " : machine.description)
						.font(.callout)
						.foregroundStyle(.secondary)
						.lineLimit(2, reservesSpace: true)
				}
				Spacer(minLength: 0)
			}
			HStack(spacing: 8) {
				if !machine.category.isEmpty {
					Text(machine.category)
						.font(.caption)
						.foregroundStyle(.secondary)
						.lineLimit(1)
						.padding(.horizontal, 8)
						.padding(.vertical, 2)
						.background(Capsule().fill(Color.primary.opacity(0.08)))
				}
				Spacer(minLength: 0)
				Button("Edit") { model.edit(machine) }
					.buttonStyle(.bordered)
					.controlSize(.small)
					.accessibilityLabel("Edit \(machine.name)")
					.accessibilityIdentifier("library.edit.\(machine.name)")
				Button { model.play(machine) } label: {
					Image(systemName: "play.circle.fill")
						.font(.title)
						.foregroundStyle(Color.accentColor)
				}
				.buttonStyle(.borderless)
				.help("Play")
				.accessibilityLabel("Play \(machine.name)")
				.accessibilityIdentifier("library.play.\(machine.name)")
			}
		}
		.padding(12)
		.frame(maxWidth: .infinity, alignment: .leading)
		.background(shape.fill(Color.primary.opacity(hovering ? 0.08 : 0.04)))
		.overlay(shape.strokeBorder(selected ? Color.accentColor : Color.primary.opacity(0.1), lineWidth: selected ? 3 : 1))
		.contentShape(shape)
		.onHover { hovering = $0 }
		#if os(macOS)
		.onTapGesture(count: 2) { model.play(machine) }
		.simultaneousGesture(TapGesture().onEnded { select() })
		#endif
		.contextMenu {
			Button("Play") { model.play(machine) }
			Button("Edit") { model.edit(machine) }
			Button("Duplicate", action: duplicate)
			Button("Rename\u{2026}", action: rename)
			#if os(macOS)
			Button("Show in Finder") {
				NSWorkspace.shared.activateFileViewerSelecting([URL(fileURLWithPath: machine.path)])
			}
			#endif
			Divider()
			Button("\(Self.deleteCommand)\u{2026}", role: .destructive, action: delete)
		}
		.accessibilityElement(children: .contain)
		.accessibilityLabel([machine.name, machine.description, machine.category].filter { !$0.isEmpty }.joined(separator: ", "))
		.accessibilityAddTraits(selected ? .isSelected : [])
		// VoiceOver reaches the context menu's commands as the card's actions.
		.accessibilityAction { model.play(machine) }
		.accessibilityAction(named: "Play") { model.play(machine) }
		.accessibilityAction(named: "Edit") { model.edit(machine) }
		.accessibilityAction(named: "Duplicate", duplicate)
		.accessibilityAction(named: "Rename", rename)
		.accessibilityAction(named: Text(Self.deleteCommand), delete)
	}
}

/// A machine's icon: a computer on a rounded tile, tinted from its name.
struct MachineIcon: View {
	let name: String

	var body: some View {
		RoundedRectangle(cornerRadius: 10, style: .continuous)
			.fill(Machine.tint(for: name).gradient)
			.frame(width: 44, height: 44)
			.overlay {
				Image(systemName: "desktopcomputer")
					.font(.title2)
					.foregroundStyle(.white)
			}
			.accessibilityHidden(true)
	}
}

/// No saved machines yet: start from a model, or play a file.
struct EmptyLibraryView: View {
	let model: LibraryModel

	var body: some View {
		ContentUnavailableView {
			Label("No Saved Machines", systemImage: "desktopcomputer")
		} description: {
			Text("Start from a model, set it up, then save it to find it here. Or play a disk, CD or WHDLoad archive straight away.")
		} actions: {
			HStack {
				ForEach(MachinePresets.all.prefix(4)) { preset in
					Button(preset.label) { model.newMachine(preset: preset.id) }
				}
			}
			.buttonStyle(.bordered)
			Button("Play a File\u{2026}") { model.chooseFileToPlay() }
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
	}
}

/// A strip over or under the machines saying what happened: an icon, the
/// message, what can be done about it, and a close button when it can be
/// put away.
struct LibraryBanner<Message: View, Actions: View>: View {
	let systemImage: String
	let tint: Color
	var close: (() -> Void)?
	@ViewBuilder let message: Message
	@ViewBuilder let actions: Actions

	var body: some View {
		HStack(alignment: .firstTextBaseline, spacing: 12) {
			Image(systemName: systemImage)
				.foregroundStyle(tint)
				.accessibilityHidden(true)
			VStack(alignment: .leading, spacing: 2) {
				message
			}
			.frame(maxWidth: .infinity, alignment: .leading)
			actions
			if let close {
				Button(action: close) {
					Image(systemName: "xmark")
				}
				.buttonStyle(.borderless)
				.help("Close")
				.accessibilityLabel("Close")
			}
		}
		.padding(.horizontal, 16)
		.padding(.vertical, 10)
		.background(.bar)
	}
}

extension LibraryBanner where Actions == EmptyView {
	init(systemImage: String, tint: Color, close: (() -> Void)? = nil, @ViewBuilder message: () -> Message) {
		self.init(systemImage: systemImage, tint: tint, close: close, message: message, actions: { EmptyView() })
	}
}

/// Over the Library while a file is dragged onto it: where it goes, and
/// what dropping it does.
private struct DropToPlayOverlay: View {
	var body: some View {
		let shape = RoundedRectangle(cornerRadius: 14, style: .continuous)
		shape
			.fill(Color.accentColor.opacity(0.08))
			.overlay(shape.strokeBorder(Color.accentColor, style: StrokeStyle(lineWidth: 3, dash: [10, 6])))
			.overlay {
				Label("Drop to Play", systemImage: "play.circle.fill")
					.font(.title2.weight(.semibold))
					.foregroundStyle(Color.accentColor)
					.padding(.horizontal, 16)
					.padding(.vertical, 10)
					.background(.regularMaterial, in: Capsule())
			}
			.padding(10)
			.allowsHitTesting(false)
	}
}

/// Until a Kickstart is installed: what machines boot meanwhile, and the
/// way to import ROMs; then what an import found, until it is closed.
struct ROMNotice: View {
	let model: LibraryModel
	/// Whether the advice to import ROMs shows (the Kickstart ROMs page
	/// gives it itself, so there only an import's report shows).
	var showsAdvice = true
	/// Closes the advice for good (an import's report closes on its own).
	var dismissAdvice: () -> Void = {}

	var body: some View {
		let advice = showsAdvice && !model.kickstartInstalled
		let close: () -> Void = {
			if advice {
				dismissAdvice()
			}
			model.romReport = nil
		}
		LibraryBanner(
			systemImage: model.kickstartInstalled ? "checkmark.circle" : "memorychip",
			tint: model.kickstartInstalled ? .green : .orange,
			close: close
		) {
			if advice {
				Text("No Kickstart ROMs").font(.headline)
				Text("Machines boot the free AROS ROM. For the Amiga\u{2019}s own, import your Kickstart ROM files (from Amiga Forever or dumped from your Amiga).")
					.foregroundStyle(.secondary)
					.lineLimit(4)
					.fixedSize(horizontal: false, vertical: true)
			}
			if let report = model.romReport {
				Text(report).font(.callout)
			}
		} actions: {
			if advice {
				Button("Import ROMs\u{2026}") {
					model.importROMs(FilePanels.chooseROMs(title: ROMImport.prompt))
				}
				.accessibilityIdentifier("library.importROMs")
			}
		}
	}
}
