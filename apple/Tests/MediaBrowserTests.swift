/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the Library's Media shelves as their user meets them: which
 * files a folder shelf lists and in what order, the cap on a huge folder,
 * the search, what Recent lists, taking files off Recent and clearing it,
 * and playing a disk in a saved machine.
 */

import AmiberrySettings
@testable import AmiberryNativeUI
import XCTest

@MainActor
final class MediaBrowserTests: XCTestCase {
	/// An empty folder of the test's own, removed when the test ends.
	private func makeFolder() throws -> URL {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("media-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		addTeardownBlock { try? FileManager.default.removeItem(at: folder) }
		return folder
	}

	/// Creates the files (and the folders on their way) under the folder.
	private func touch(_ names: [String], in folder: URL, size: Int = 16) throws {
		for name in names {
			let file = folder.appendingPathComponent(name)
			try FileManager.default.createDirectory(at: file.deletingLastPathComponent(), withIntermediateDirectories: true)
			XCTAssertTrue(FileManager.default.createFile(atPath: file.path, contents: Data(count: size)), name)
		}
	}

	func testAFolderShelfListsItsFilesRecursivelyInFinderOrder() throws {
		let folder = try makeFolder()
		try touch([
			"Game 10.adf", "Game 9.ADF", "Sub/Disk.ipf", "Sub/Deeper/Archive.zip", "notes.txt", "Track.iso",
			".Hidden.adf", ".git/Inside.adf", "Tool.app/Contents/Packaged.adf", "Demo.lha",
		], in: folder)

		XCTAssertEqual(MediaShelf.floppies.items(in: folder).items.map(\.name),
			["Archive.zip", "Disk.ipf", "Game 9.ADF", "Game 10.adf"],
			"disk images only, from subfolders too, numbers in order; no hidden files or package contents")
		XCTAssertEqual(MediaShelf.cds.items(in: folder).items.map(\.name), ["Track.iso"])
		XCTAssertEqual(MediaShelf.whdload.items(in: folder).items.map(\.name), ["Demo.lha"])
		let disk = try XCTUnwrap(MediaShelf.floppies.items(in: folder).items.first { $0.name == "Disk.ipf" })
		XCTAssertEqual(disk.kind, .floppies)
		XCTAssertEqual(disk.size, 16)
		XCTAssertNotNil(disk.modified)

		// Every file a shelf lists plays.
		for shelf in MediaShelf.folderShelves {
			XCTAssertTrue(shelf.extensions.isSubset(of: QuickPlay.extensions), "\(shelf)")
		}
	}

	func testTheScanStopsAtItsCap() throws {
		let folder = try makeFolder()
		try touch((1...12).map { "Disk \($0).adf" }, in: folder)

		let capped = MediaShelf.floppies.items(in: folder, limit: 5)
		XCTAssertEqual(capped.items.count, 5)
		XCTAssertTrue(capped.truncated)
		let whole = MediaShelf.floppies.items(in: folder, limit: 12)
		XCTAssertEqual(whole.items.count, 12)
		XCTAssertFalse(whole.truncated, "a folder of exactly the cap is complete")
	}

	func testTheSearchMatchesEveryWordOfTheName() throws {
		let folder = try makeFolder()
		try touch(["Lotus Turbo Challenge.adf", "Éclair Demo.adf", "Turrican II.ipf"], in: folder)
		let items = MediaShelf.floppies.items(in: folder).items

		XCTAssertEqual(items.matching("").map(\.name), items.map(\.name))
		XCTAssertEqual(items.matching("  ").map(\.name), items.map(\.name))
		XCTAssertEqual(items.matching("tur").map(\.name), ["Lotus Turbo Challenge.adf", "Turrican II.ipf"])
		XCTAssertEqual(items.matching("TURBO lotus").map(\.name), ["Lotus Turbo Challenge.adf"])
		XCTAssertEqual(items.matching("eclair").map(\.name), ["Éclair Demo.adf"], "case and accents are ignored")
		XCTAssertEqual(items.matching("lotus ipf").map(\.name), [], "every word must match")
	}

	func testRecentListsWhatWasPlayedLastAndMarksWhatIsGone() throws {
		let folder = try makeFolder()
		let tag = "mbt" + UUID().uuidString.prefix(8).lowercased()
		try touch(["\(tag) First.adf", "\(tag) Gone.adf", "\(tag) Last.adf", "\(tag) Disc.iso"], in: folder)
		for name in ["First.adf", "Gone.adf", "Last.adf"] {
			try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) \(name)").path, to: .recentFloppies)
		}
		try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) Disc.iso").path, to: .recentCDs)
		try FileManager.default.removeItem(at: folder.appendingPathComponent("\(tag) Gone.adf"))

		let items = MediaShelf.recentItems(from: MediaShelf.recentPaths()).filter { $0.name.hasPrefix(tag) }
		XCTAssertEqual(items.map(\.name), ["\(tag) Last.adf", "\(tag) Gone.adf", "\(tag) First.adf", "\(tag) Disc.iso"],
			"most recent first, disks before CDs, a file no longer there still listed")
		XCTAssertEqual(items.map(\.kind), [.floppies, .floppies, .floppies, .cds])
		XCTAssertEqual(items.map(\.missing), [false, true, false, false], "only the deleted file is missing")
		XCTAssertEqual(items.first { $0.missing }?.details, "File not found")

		// The Drives menu records kinds quick play does not take.
		try touch(["\(tag) Flux.scp"], in: folder)
		let flux = folder.appendingPathComponent("\(tag) Flux.scp").path
		let first = folder.appendingPathComponent("\(tag) First.adf").path
		XCTAssertEqual(MediaShelf.recentItems(from: [RecentList(shelf: .floppies, paths: [flux, first])]).map(\.path), [first],
			"what the shelf cannot play is not listed")
	}

	/// The names of a recent list's entries that carry the tag, most recent
	/// first, from a fresh snapshot: the media layer reloads the list
	/// (amiberry_media_refresh) on every call.
	private func recent(_ list: MediaList, tagged tag: String) -> [String] {
		MediaLibrary.entries(list).map(\.name).filter { $0.hasPrefix(tag) }
	}

	/// The file names that carry the tag in one recent list of the
	/// emulator's settings file (amiberry.conf in the test home), most
	/// recent first: what the next start reads back.
	private func saved(_ option: String, tagged tag: String) throws -> [String] {
		let home = ProcessInfo.processInfo.environment["HOME"] ?? NSHomeDirectory()
		let file = URL(fileURLWithPath: home).appendingPathComponent("Library/Application Support/Amiberry/amiberry.conf")
		return try String(contentsOf: file, encoding: .utf8).split(separator: "\n").compactMap { line in
			guard line.hasPrefix(option + "=") else { return nil }
			let name = (String(line.dropFirst(option.count + 1)) as NSString).lastPathComponent
			return name.hasPrefix(tag) ? name : nil
		}
	}

	func testRemovingFromRecentKeepsTheOthersInOrder() throws {
		let folder = try makeFolder()
		let tag = "mbr" + UUID().uuidString.prefix(8).lowercased()
		let names = ["\(tag) A.adf", "\(tag) B.adf", "\(tag) C.adf"]
		try touch(names, in: folder)
		for name in names {
			try MediaLibrary.addRecent(folder.appendingPathComponent(name).path, to: .recentFloppies)
		}
		XCTAssertEqual(recent(.recentFloppies, tagged: tag), ["\(tag) C.adf", "\(tag) B.adf", "\(tag) A.adf"])

		try MediaLibrary.removeRecent(folder.appendingPathComponent("\(tag) B.adf").path, from: .recentFloppies)
		XCTAssertEqual(recent(.recentFloppies, tagged: tag), ["\(tag) C.adf", "\(tag) A.adf"],
			"the entry goes, the others keep their order")
		XCTAssertEqual(try saved("Diskfile", tagged: tag), ["\(tag) C.adf", "\(tag) A.adf"], "as the settings file keeps it")

		// A file deleted since it was played can be taken off too, by the
		// path the list gave for it.
		let gone = try XCTUnwrap(MediaLibrary.entries(.recentFloppies).first { $0.name == "\(tag) A.adf" })
		try FileManager.default.removeItem(atPath: gone.path)
		try MediaLibrary.removeRecent(gone.path, from: .recentFloppies)
		XCTAssertEqual(recent(.recentFloppies, tagged: tag), ["\(tag) C.adf"])

		// A path the list does not hold changes nothing.
		try MediaLibrary.removeRecent(folder.appendingPathComponent("\(tag) Never.adf").path, from: .recentFloppies)
		XCTAssertEqual(recent(.recentFloppies, tagged: tag), ["\(tag) C.adf"])
		XCTAssertThrowsError(try MediaLibrary.removeRecent(folder.appendingPathComponent("\(tag) C.adf").path, from: .configurations),
			"only the recent lists")
	}

	func testClearingARecentListLeavesTheOthers() throws {
		let folder = try makeFolder()
		let tag = "mbc" + UUID().uuidString.prefix(8).lowercased()
		try touch(["\(tag) Disk.adf", "\(tag) Disc.iso", "\(tag) Game.lha"], in: folder)
		try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) Disk.adf").path, to: .recentFloppies)
		try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) Disc.iso").path, to: .recentCDs)
		try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) Game.lha").path, to: .recentWHDLoad)

		try MediaLibrary.clearRecent(.recentCDs)
		XCTAssertEqual(MediaLibrary.entries(.recentCDs), [], "the list is empty")
		XCTAssertEqual(recent(.recentFloppies, tagged: tag), ["\(tag) Disk.adf"], "the other lists keep theirs")
		XCTAssertEqual(recent(.recentWHDLoad, tagged: tag), ["\(tag) Game.lha"])
		XCTAssertEqual(try saved("CDfile", tagged: tag), [], "as the settings file keeps them")
		XCTAssertEqual(try saved("Diskfile", tagged: tag), ["\(tag) Disk.adf"])
		XCTAssertEqual(try saved("WHDLoadfile", tagged: tag), ["\(tag) Game.lha"])
		XCTAssertThrowsError(try MediaLibrary.clearRecent(.configurations), "only the recent lists")
	}

	/// The Recent shelf's items with the tag once they satisfy `done`; the
	/// shelf reads its lists off the main actor.
	private func recentShelf(_ shelves: MediaShelves, tagged tag: String,
		until done: ([MediaItem]) -> Bool) async throws -> [MediaItem] {
		for _ in 0..<300 {
			if let items = shelves.items[.recent]?.filter({ $0.name.hasPrefix(tag) }), done(items) {
				return items
			}
			try await Task.sleep(for: .milliseconds(10))
		}
		XCTFail("the Recent shelf did not reach the expected state")
		return shelves.items[.recent]?.filter { $0.name.hasPrefix(tag) } ?? []
	}

	/// Remove from Recent takes a file off the shelf, a missing one too;
	/// Clear Recent empties the shelf, every kind of it.
	func testTheRecentShelfRemovesAFileAndClears() async throws {
		let folder = try makeFolder()
		let tag = "mbs" + UUID().uuidString.prefix(8).lowercased()
		try touch(["\(tag) Kept.adf", "\(tag) Gone.adf", "\(tag) Disc.iso"], in: folder)
		for name in ["Kept.adf", "Gone.adf"] {
			try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) \(name)").path, to: .recentFloppies)
		}
		try MediaLibrary.addRecent(folder.appendingPathComponent("\(tag) Disc.iso").path, to: .recentCDs)
		try FileManager.default.removeItem(at: folder.appendingPathComponent("\(tag) Gone.adf"))

		let shelves = MediaShelves()
		shelves.show(.recent)
		let shown = try await recentShelf(shelves, tagged: tag) { $0.count == 3 }
		let gone = try XCTUnwrap(shown.first { $0.missing })
		XCTAssertEqual(gone.name, "\(tag) Gone.adf")

		XCTAssertTrue(shelves.removeFromRecent(gone))
		let left = try await recentShelf(shelves, tagged: tag) { $0.count == 2 }
		XCTAssertEqual(left.map(\.name), ["\(tag) Kept.adf", "\(tag) Disc.iso"])
		XCTAssertEqual(recent(.recentFloppies, tagged: tag), ["\(tag) Kept.adf"], "the file left its list, not only the shelf")

		XCTAssertTrue(shelves.clearRecent())
		_ = try await recentShelf(shelves, tagged: tag) { $0.isEmpty }
		for list in MediaList.allCases where list.isRecentList {
			XCTAssertEqual(MediaLibrary.entries(list), [], "\(list)")
		}
	}

	/// Play In: the machine loads, then the disk goes in its DF0: and is
	/// recorded as played; the Library then closes.
	func testPlayingADiskInAMachineLoadsTheMachineWithTheDisk() async throws {
		let folder = try makeFolder()
		let machineFile = folder.appendingPathComponent("Play In Machine.uae")
		try "config_description=Play In\n".write(to: machineFile, atomically: true, encoding: .utf8)
		let disk = folder.appendingPathComponent("Play In Disk.adf")
		FileManager.default.createFile(atPath: disk.path, contents: Data(count: 901_120))
		let machine = Machine(path: machineFile.path, name: "Play In Machine", description: "Play In", category: "")

		let pump = ActionPump()
		pump.start(interval: 0.005)
		defer { pump.stop() }
		var finished = false
		let library = LibraryModel(emulating: false, finish: { finished = true }, openEditor: { _, _ in })
		library.play(disk, in: machine)
		for _ in 0..<300 where !finished && library.notice == nil {
			try await Task.sleep(for: .milliseconds(10))
		}

		XCTAssertNil(library.notice)
		XCTAssertTrue(finished, "the Library closes to start the machine")
		XCTAssertEqual(MediaLibrary.currentConfiguration().map { URL(fileURLWithPath: $0).standardizedFileURL.path },
			machineFile.standardizedFileURL.path, "the machine is the one loaded")
		XCTAssertEqual(try SettingsSession().value(of: "floppy0"), disk.path)
		XCTAssertEqual(MediaLibrary.entries(.recentFloppies).first?.name, "Play In Disk.adf")
	}
}
