/*
 * Copyright (C) 2026 BlitterStudio
 * The Library's Media and System pages: the disks, CDs and WHDLoad archives
 * in their folders and those played last, each playable at once or in a
 * saved machine; and which models the installed Kickstart ROMs can run.
 */

import SwiftUI
#if os(macOS)
import AppKit
#endif

/// A shelf of the Library's Media section: the media played last, or the
/// files of one of the media folders.
enum MediaShelf: String, CaseIterable, Hashable, Sendable {
	case recent, floppies, cds, whdload

	/// The shelves that list a folder, in the order Recent groups them.
	static let folderShelves: [MediaShelf] = [.floppies, .cds, .whdload]

	var title: String {
		switch self {
		case .recent: return "Recent"
		case .floppies: return "Floppy Disks"
		case .cds: return "CDs"
		case .whdload: return "WHDLoad"
		}
	}

	var systemImage: String {
		switch self {
		case .recent: return "clock"
		case .floppies: return "externaldrive"
		case .cds: return "opticaldisc"
		case .whdload: return "archivebox"
		}
	}

	/// The heading of the shelf's files in Recent.
	var sectionTitle: String {
		self == .floppies ? "Disks" : title
	}

	/// The folder the shelf lists; nil for Recent.
	var folder: MediaFolder? {
		switch self {
		case .recent: return nil
		case .floppies: return .floppies
		case .cds: return .cds
		case .whdload: return .whdloadArchives
		}
	}

	/// The recent list the shelf's files are recorded in when played.
	var recentList: MediaList? {
		switch self {
		case .recent: return nil
		case .floppies: return .recentFloppies
		case .cds: return .recentCDs
		case .whdload: return .recentWHDLoad
		}
	}

	/// The files the shelf lists: those quick play takes for its kind
	/// (quick_play in amiberry_actions.cpp, QuickPlay.extensions), so every
	/// file listed plays.
	var extensions: Set<String> {
		switch self {
		case .recent: return []
		case .floppies: return ["adf", "adz", "dms", "ipf", "zip", "7z"]
		case .cds: return ["cue", "iso", "chd"]
		case .whdload: return ["lha"]
		}
	}

	/// The setting a saved machine takes the file in; nil for media that
	/// only plays in the machine quick play makes for it (WHDLoad).
	var driveSetting: String? {
		switch self {
		case .floppies: return "floppy0"
		case .cds: return "cdimage0"
		case .recent, .whdload: return nil
		}
	}

	/// The folder shelf a file belongs on, by its extension.
	static func shelf(for url: URL) -> MediaShelf? {
		let ext = url.pathExtension.lowercased()
		return folderShelves.first { $0.extensions.contains(ext) }
	}

	/// The shelf's files in the folder and its subfolders (hidden files and
	/// the insides of packages left out), sorted by name as the Finder sorts
	/// them. At most `limit` are listed, so a folder of a whole collection
	/// cannot make a list too long to use; `truncated` says some were left.
	func items(in folder: URL, limit: Int = MediaShelf.scanLimit) -> ShelfScan {
		let keys: Set<URLResourceKey> = [.isRegularFileKey, .fileSizeKey, .contentModificationDateKey]
		guard let walk = FileManager.default.enumerator(at: folder, includingPropertiesForKeys: Array(keys),
			options: [.skipsHiddenFiles, .skipsPackageDescendants]) else { return ShelfScan(items: [], truncated: false) }
		var items: [MediaItem] = []
		var truncated = false
		for case let file as URL in walk where extensions.contains(file.pathExtension.lowercased()) {
			guard let values = try? file.resourceValues(forKeys: keys), values.isRegularFile == true else { continue }
			guard items.count < limit else {
				truncated = true
				break
			}
			items.append(MediaItem(path: file.path, kind: self, size: values.fileSize.map { Int64($0) },
				modified: values.contentModificationDate))
		}
		items.sort { $0.name.localizedStandardCompare($1.name) == .orderedAscending }
		return ShelfScan(items: items, truncated: truncated)
	}

	/// How many files a folder shelf lists at most.
	static let scanLimit = 5000

	/// The recent lists' paths, each most recent first.
	@MainActor
	static func recentPaths() -> [RecentList] {
		folderShelves.compactMap { shelf in
			shelf.recentList.map { RecentList(shelf: shelf, paths: MediaLibrary.entries($0).map(\.path)) }
		}
	}

	/// The files of the recent lists that can be played from the shelf, in
	/// their lists' order. A file no longer there stays listed, marked
	/// missing, so it can be taken off its list rather than vanish
	/// unexplained. A list also holds what the Drives menu inserted, which
	/// can be a kind quick play does not take (an .scp disk, an .nrg CD).
	static func recentItems(from lists: [RecentList]) -> [MediaItem] {
		let keys: Set<URLResourceKey> = [.isRegularFileKey, .fileSizeKey, .contentModificationDateKey]
		var seen = Set<String>()
		var items: [MediaItem] = []
		for list in lists {
			for path in list.paths
			where list.shelf.extensions.contains(URL(fileURLWithPath: path).pathExtension.lowercased())
				&& seen.insert(path).inserted {
				guard FileManager.default.fileExists(atPath: path) else {
					items.append(MediaItem(path: path, kind: list.shelf, size: nil, modified: nil, missing: true))
					continue
				}
				guard let values = try? URL(fileURLWithPath: path).resourceValues(forKeys: keys),
					values.isRegularFile == true else { continue }
				items.append(MediaItem(path: path, kind: list.shelf, size: values.fileSize.map { Int64($0) },
					modified: values.contentModificationDate))
			}
		}
		return items
	}
}

/// What a folder shelf's scan found.
struct ShelfScan: Sendable {
	let items: [MediaItem]
	/// Whether files were left out, past `MediaShelf.scanLimit`.
	let truncated: Bool
}

/// One recent list's paths, most recent first.
struct RecentList: Sendable {
	let shelf: MediaShelf
	let paths: [String]
}

/// A disk, CD or WHDLoad archive on a shelf.
struct MediaItem: Identifiable, Hashable, Sendable {
	let path: String
	/// The file name, with its extension (a disk can be an ADF or an IPF).
	let name: String
	/// The folder shelf its kind belongs on (never Recent).
	let kind: MediaShelf
	let size: Int64?
	let modified: Date?
	/// A recent file no longer at its path (deleted, moved, or on a volume
	/// not connected): it cannot play, only leave its list.
	let missing: Bool
	var id: String { path }
	var url: URL { URL(fileURLWithPath: path) }

	init(path: String, kind: MediaShelf, size: Int64?, modified: Date?, missing: Bool = false) {
		self.path = path
		name = (path as NSString).lastPathComponent
		self.kind = kind
		self.size = size
		self.modified = modified
		self.missing = missing
	}

	/// The line under the name: "880 KB · 12 Oct 2026", or that the file is
	/// missing. Sizes count in binary units, so a disk reads as the 880 KB
	/// an Amiga disk holds.
	var details: String {
		if missing {
			return "File not found"
		}
		var parts: [String] = []
		if let size {
			parts.append(ByteCountFormatter.string(fromByteCount: size, countStyle: .memory))
		}
		if let modified {
			parts.append(modified.formatted(date: .abbreviated, time: .omitted))
		}
		return parts.joined(separator: " \u{00B7} ")
	}
}

extension Array where Element == MediaItem {
	/// The items whose name contains each word of the query, ignoring case
	/// and accents as the Finder's search does; an empty query matches all.
	func matching(_ query: String) -> [MediaItem] {
		let terms = query.split(whereSeparator: \.isWhitespace).map(String.init)
		guard !terms.isEmpty else { return self }
		return filter { item in terms.allSatisfy { item.name.localizedStandardContains($0) } }
	}
}

/// What the Media shelves list. A shelf is scanned when it is first shown
/// and again as the Library reloads; folders are walked off the main
/// actor, so a large one never stalls the Library.
@MainActor
@Observable
final class MediaShelves {
	/// Each scanned shelf's items; a shelf not scanned yet has none.
	private(set) var items: [MediaShelf: [MediaItem]] = [:]
	/// The shelves that listed only the first `MediaShelf.scanLimit` files.
	private(set) var truncated: Set<MediaShelf> = []
	/// Counts each shelf's scans, so only the latest one is published.
	@ObservationIgnored private var generations: [MediaShelf: Int] = [:]

	/// Scans the shelf when it has not been yet. Recent is read every time:
	/// what was played since changes it, and it is a few dozen files.
	func show(_ shelf: MediaShelf) {
		if shelf == .recent || items[shelf] == nil {
			rescan(shelf)
		}
	}

	/// Scans again every shelf that has been shown.
	func reload() {
		for shelf in items.keys {
			rescan(shelf)
		}
	}

	func rescan(_ shelf: MediaShelf) {
		let generation = (generations[shelf] ?? 0) + 1
		generations[shelf] = generation
		let scan: Task<ShelfScan, Never>
		if shelf == .recent {
			let lists = MediaShelf.recentPaths()
			scan = Task.detached(priority: .userInitiated) {
				ShelfScan(items: MediaShelf.recentItems(from: lists), truncated: false)
			}
		} else if let folder = shelf.folder, let path = try? MediaLibrary.path(of: folder), !path.isEmpty {
			let url = URL(fileURLWithPath: path, isDirectory: true)
			scan = Task.detached(priority: .userInitiated) { shelf.items(in: url) }
		} else {
			publish(ShelfScan(items: [], truncated: false), for: shelf)
			return
		}
		Task { [weak self] in
			let result = await scan.value
			guard let self, self.generations[shelf] == generation else { return }
			self.publish(result, for: shelf)
		}
	}

	private func publish(_ result: ShelfScan, for shelf: MediaShelf) {
		items[shelf] = result.items
		if result.truncated {
			truncated.insert(shelf)
		} else {
			truncated.remove(shelf)
		}
	}

	/// Takes a Recent item off its recent list and reads Recent again; false
	/// when the emulator's settings could not be saved (the list is then
	/// unchanged).
	@discardableResult
	func removeFromRecent(_ item: MediaItem) -> Bool {
		guard let list = item.kind.recentList else { return false }
		let removed = (try? MediaLibrary.removeRecent(item.path, from: list)) != nil
		rescan(.recent)
		return removed
	}

	/// Empties every recent list Recent shows and reads it again; false when
	/// the emulator's settings could not be saved for one of them.
	@discardableResult
	func clearRecent() -> Bool {
		var cleared = true
		for list in MediaShelf.folderShelves.compactMap(\.recentList) {
			do {
				try MediaLibrary.clearRecent(list)
			} catch {
				cleared = false
			}
		}
		rescan(.recent)
		return cleared
	}
}

/// A Media shelf: its files in a list, each played with a double click (a
/// tap on iOS), Return or its play button; the context menu plays it in a
/// saved machine too. On Recent the context menu takes a file off the list
/// and the toolbar clears it; a file no longer there shows dimmed, with
/// only that removal to offer.
struct MediaShelfView: View {
	let model: LibraryModel
	let shelf: MediaShelf
	let query: String
	@State private var selection: MediaItem.ID?
	@State private var confirmingClear = false

	var body: some View {
		Group {
			if let all = model.media.items[shelf] {
				let shown = all.matching(query)
				if all.isEmpty {
					empty
				} else if shown.isEmpty {
					ContentUnavailableView.search(text: query.trimmingCharacters(in: .whitespaces))
				} else {
					list(of: shown, all: all)
				}
			} else {
				ProgressView()
			}
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.onAppear { model.media.show(shelf) }
		.toolbar {
			if shelf == .recent {
				ToolbarItem {
					Button { confirmingClear = true } label: {
						Label("Clear Recent\u{2026}", systemImage: "clock.badge.xmark")
					}
					.help("Forget the disks, CDs and WHDLoad archives played last")
					.disabled((model.media.items[.recent] ?? []).isEmpty)
					.accessibilityIdentifier("library.media.clearRecent")
				}
			}
		}
		.confirmationDialog("Clear the Recent list?", isPresented: $confirmingClear, titleVisibility: .visible) {
			Button("Clear Recent", role: .destructive) {
				if !model.media.clearRecent() {
					model.notice = Self.recentNotSaved
				}
			}
			Button("Cancel", role: .cancel) {}
		} message: {
			Text("Amiberry forgets the disks, CDs and WHDLoad archives played last. The files themselves are not deleted.")
		}
	}

	/// What the Library says when a change to Recent could not be kept.
	static let recentNotSaved = "Recent could not be changed: Amiberry\u{2019}s settings file could not be saved."

	private func removeFromRecent(_ item: MediaItem) {
		if !model.media.removeFromRecent(item) {
			model.notice = Self.recentNotSaved
		}
	}

	private func list(of shown: [MediaItem], all: [MediaItem]) -> some View {
		List(selection: $selection) {
			if shelf == .recent {
				ForEach(MediaShelf.folderShelves, id: \.self) { kind in
					let items = shown.filter { $0.kind == kind }
					if !items.isEmpty {
						Section(kind.sectionTitle) {
							rows(of: items)
						}
					}
				}
			} else {
				Section {
					rows(of: shown)
				} footer: {
					if model.media.truncated.contains(shelf) {
						Text("Only the first \(MediaShelf.scanLimit.formatted()) files of the folder are listed.")
					}
				}
			}
		}
		.contextMenu(forSelectionType: MediaItem.ID.self) { ids in
			if let item = all.first(where: { ids.contains($0.id) }) {
				menu(for: item)
			}
		} primaryAction: { ids in
			// A missing file has nothing to play; its row offers the removal.
			if let item = all.first(where: { ids.contains($0.id) }), !item.missing {
				model.quickPlay(item.url)
			}
		}
		#if os(iOS)
		// Pulled down, the shelf reads its folder again.
		.refreshable { model.reload() }
		#endif
	}

	private func rows(of items: [MediaItem]) -> some View {
		ForEach(items) { item in
			MediaRow(item: item, play: { model.quickPlay(item.url) }, remove: { removeFromRecent(item) })
		}
	}

	@ViewBuilder
	private func menu(for item: MediaItem) -> some View {
		if item.missing {
			Button("Remove from Recent") { removeFromRecent(item) }
		} else {
			Button("Play") { model.quickPlay(item.url) }
			if item.kind.driveSetting != nil {
				Menu("Play In") {
					ForEach(model.machines) { machine in
						Button(machine.name) { model.play(item.url, in: machine) }
					}
				}
				.disabled(model.machines.isEmpty)
			}
		}
		#if os(macOS)
		Divider()
		if !item.missing {
			Button("Show in Finder") { PathField.reveal(item.path) }
		}
		Button("Copy Path") {
			NSPasteboard.general.clearContents()
			NSPasteboard.general.setString(item.path, forType: .string)
		}
		#endif
		if shelf == .recent, !item.missing {
			Divider()
			Button("Remove from Recent") { removeFromRecent(item) }
		}
	}

	/// A shelf with nothing on it: where its files go, and another way to
	/// play one.
	private var empty: some View {
		ContentUnavailableView {
			Label(emptyTitle, systemImage: shelf.systemImage)
		} description: {
			Text(emptyDescription)
		} actions: {
			#if os(macOS)
			if let folder = shelf.folder {
				Button("Show Folder in Finder") {
					if let path = try? MediaLibrary.path(of: folder), !path.isEmpty {
						_ = NSWorkspace.shared.open(URL(fileURLWithPath: path, isDirectory: true))
					}
				}
			}
			#endif
			Button("Play a File\u{2026}") { model.chooseFileToPlay() }
		}
	}

	private var emptyTitle: String {
		switch shelf {
		case .recent: return "Nothing Played Yet"
		case .floppies: return "No Floppy Disks"
		case .cds: return "No CDs"
		case .whdload: return "No WHDLoad Archives"
		}
	}

	private var emptyDescription: String {
		switch shelf {
		case .recent: return "The disks, CDs and WHDLoad archives you play are listed here."
		case .floppies: return "Put disk images in the Floppies folder, or drop one on the Library to play it."
		case .cds: return "Put CD images in the CDs folder, or drop one on the Library to play it."
		case .whdload: return "Put WHDLoad archives in the WHDLoad folder, or drop one on the Library to play it."
		}
	}
}

/// A file on a shelf: its icon, name, size and date, and a play button. A
/// missing file is dimmed, its play button replaced by Remove from Recent.
private struct MediaRow: View {
	let item: MediaItem
	let play: () -> Void
	let remove: () -> Void

	var body: some View {
		HStack(spacing: 10) {
			Group {
				icon
				VStack(alignment: .leading, spacing: 2) {
					Text(item.name).lineLimit(1).truncationMode(.middle)
					Text(item.details)
						.font(.caption)
						.foregroundStyle(.secondary)
				}
			}
			.opacity(item.missing ? 0.45 : 1)
			Spacer(minLength: 0)
			if item.missing {
				Button("Remove from Recent", action: remove)
					.buttonStyle(.bordered)
					.controlSize(.small)
					.accessibilityLabel("Remove \(item.name) from Recent")
					.accessibilityIdentifier("library.media.remove.\(item.name)")
			} else {
				Button(action: play) {
					Image(systemName: "play.circle.fill")
						.font(.title2)
						.foregroundStyle(Color.accentColor)
				}
				.buttonStyle(.borderless)
				.help("Play")
				.accessibilityLabel("Play \(item.name)")
				.accessibilityIdentifier("library.media.play.\(item.name)")
			}
		}
		.padding(.vertical, 2)
		#if os(macOS)
		.help(item.path)
		#endif
		.accessibilityElement(children: .contain)
		.accessibilityLabel([item.name, item.details].filter { !$0.isEmpty }.joined(separator: ", "))
	}

	/// A disk's icon on a Mac is the Finder's, which draws ADF and IPF
	/// images as disks; the Finder draws a CD image or an archive as a blank
	/// document, so those (and every file on iOS) show the shelf's symbol on
	/// a tinted tile, as a machine's icon does.
	@ViewBuilder private var icon: some View {
		#if os(macOS)
		if item.kind == .floppies {
			Image(nsImage: PathField.icon(for: item.path))
				.resizable()
				.frame(width: 32, height: 32)
				.accessibilityHidden(true)
		} else {
			tile
		}
		#else
		tile
		#endif
	}

	private var tile: some View {
		RoundedRectangle(cornerRadius: 7, style: .continuous)
			.fill(Color.accentColor.gradient)
			.frame(width: 32, height: 32)
			.overlay {
				Image(systemName: item.kind.systemImage)
					.font(.body)
					.foregroundStyle(.white)
			}
			.accessibilityHidden(true)
	}
}

/// Which models the installed Kickstart ROMs can run, and the ways to add
/// ROMs. A model without its ROM still runs, on the free AROS ROM.
struct KickstartROMsView: View {
	let model: LibraryModel
	let query: String

	/// The computers, and the CD consoles when the layer has them.
	static var models: [MachinePreset] {
		MachinePresets.all.filter { MachinePresets.computerIDs.contains($0.id) || ["cd32", "cdtv"].contains($0.id) }
	}

	var body: some View {
		let terms = query.split(whereSeparator: \.isWhitespace).map(String.init)
		let shown = Self.models.filter { preset in terms.allSatisfy { preset.label.localizedStandardContains($0) } }
		Form {
			Section {
				if shown.isEmpty {
					ContentUnavailableView.search(text: query.trimmingCharacters(in: .whitespaces))
				}
				ForEach(shown) { preset in
					LabeledContent(preset.label) {
						if model.readyModels.contains(preset.id) {
							Label("Ready", systemImage: "checkmark.circle.fill")
								.foregroundStyle(.green)
						} else {
							Text("ROM not installed \u{2014} boots AROS")
								.foregroundStyle(.secondary)
						}
					}
					.accessibilityIdentifier("library.roms.\(preset.id)")
				}
			} header: {
				Text("Models")
			} footer: {
				Text("A model is ready when the ROMs folder holds its Kickstart. Until then its machines boot the free AROS ROM.")
			}
			Section {
				Button("Import ROMs\u{2026}") {
					model.importROMs(FilePanels.chooseROMs(title: ROMImport.prompt))
				}
				.accessibilityIdentifier("library.roms.import")
				#if os(macOS)
				Button("Show ROMs Folder") {
					if let path = try? MediaLibrary.path(of: .roms), !path.isEmpty {
						_ = NSWorkspace.shared.open(URL(fileURLWithPath: path, isDirectory: true))
					}
				}
				#endif
			} footer: {
				Text("Import your Kickstart ROM files, from Amiga Forever or dumped from your Amiga: they are copied into the ROMs folder.")
			}
		}
		.formStyle(.grouped)
	}
}
