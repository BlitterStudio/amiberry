/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the Library as its user meets it: the search, the order the
 * machines come in, duplicating, deleting and renaming one, and the colours
 * of their icons.
 */

import AmiberrySettings
@testable import AmiberryNativeUI
import SwiftUI
import XCTest

@MainActor
final class LibraryViewTests: XCTestCase {
	/// Saved machines with a tag only this test's names carry; removed with
	/// every file that tag names (copies included) when the test ends.
	private func makeMachines(_ machines: [(name: String, description: String, category: String)]) throws -> String {
		let folder = try MediaLibrary.path(of: .configurations)
		let tag = "lvt" + UUID().uuidString.prefix(8).lowercased()
		addTeardownBlock {
			let names = (try? FileManager.default.contentsOfDirectory(atPath: folder)) ?? []
			for name in names where name.hasPrefix(tag) {
				try? FileManager.default.removeItem(atPath: (folder as NSString).appendingPathComponent(name))
			}
		}
		for machine in machines {
			let file = URL(fileURLWithPath: folder).appendingPathComponent("\(tag) \(machine.name).uae")
			try "config_description=\(machine.description)\nconfig_category=\(machine.category)\n"
				.write(to: file, atomically: true, encoding: .utf8)
		}
		return tag
	}

	private func library() -> LibraryModel {
		LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in })
	}

	func testSearchMatchesEveryWordIgnoringCaseAndAccents() throws {
		let tag = try makeMachines([
			("Zebra Eclair", "Lotus II on an A500", "Games"),
			("Workshop", "Workbench 1.3 with a hard drive", "Utilities"),
		])
		let library = library()
		func names(_ category: String?, _ query: String) -> [String] {
			library.machines(in: category, matching: query).map(\.name)
		}
		XCTAssertEqual(names(nil, tag), ["\(tag) Workshop", "\(tag) Zebra Eclair"])
		XCTAssertEqual(names(nil, "\(tag.uppercased()) ÉCLAIR"), ["\(tag) Zebra Eclair"], "case and accents are ignored")
		XCTAssertEqual(names(nil, "\(tag) lotus a500"), ["\(tag) Zebra Eclair"], "the description is searched")
		XCTAssertEqual(names(nil, "\(tag) utilities"), ["\(tag) Workshop"], "the category is searched")
		XCTAssertEqual(names(nil, "\(tag) zebra workbench"), [], "every word must match")
		XCTAssertEqual(names("Utilities", tag), ["\(tag) Workshop"], "only the category's machines")
		XCTAssertEqual(names("Games", "\(tag) workbench"), [])
		XCTAssertEqual(library.machines(in: nil, matching: ""), library.machines)
		XCTAssertEqual(library.machines(in: nil, matching: "  "), library.machines)
		XCTAssertEqual(library.machines(in: "Games", matching: ""), library.machines.filter { $0.category == "Games" })
	}

	func testMachinesAreSortedByNameAsTheFinderSortsThem() throws {
		let tag = try makeMachines([
			("banana 10", "", ""),
			("Zebra", "", ""),
			("Banana 9", "", ""),
			("apple", "", ""),
		])
		let names = library().machines.map(\.name).filter { $0.hasPrefix(tag) }
		XCTAssertEqual(names, ["\(tag) apple", "\(tag) Banana 9", "\(tag) banana 10", "\(tag) Zebra"])
	}

	func testDuplicatesAreNamedAsTheFinderNamesThem() throws {
		let tag = try makeMachines([("Turrican", "Turrican II on an A500", "Games")])
		let library = library()
		let original = try XCTUnwrap(library.machines.first { $0.name == "\(tag) Turrican" })

		let copy = try XCTUnwrap(library.duplicate(original))
		XCTAssertEqual(copy.name, "\(tag) Turrican copy")
		XCTAssertEqual(copy.description, "Turrican II on an A500")
		XCTAssertEqual(copy.category, "Games")
		XCTAssertTrue(FileManager.default.fileExists(atPath: copy.path))
		XCTAssertTrue(FileManager.default.fileExists(atPath: original.path), "the original stays")

		let second = try XCTUnwrap(library.duplicate(original))
		XCTAssertEqual(second.name, "\(tag) Turrican copy 2")
		XCTAssertEqual(second.description, "Turrican II on an A500")
		XCTAssertEqual(second.category, "Games")

		let third = try XCTUnwrap(library.duplicate(copy))
		XCTAssertEqual(third.name, "\(tag) Turrican copy 3", "a copy's copy is numbered, not \"copy copy\"")
		XCTAssertEqual(library.machines.filter { $0.name.hasPrefix(tag) }.count, 4, "the Library lists the copies")
		XCTAssertNil(library.notice)
		XCTAssertEqual(LibraryModel.copyBase(of: "copy"), "copy", "a name that is only the word stays")
		XCTAssertEqual(LibraryModel.copyBase(of: "Disk copy 12"), "Disk")
		XCTAssertEqual(LibraryModel.copyBase(of: "Photocopy"), "Photocopy")
	}

	func testAMachineDeletedLeavesTheLibrary() throws {
		let tag = try makeMachines([("Doomed", "", ""), ("Kept", "", "")])
		// The removed file goes to a folder of this test's, not the Trash.
		let bin = FileManager.default.temporaryDirectory.appendingPathComponent("lvt-bin-" + UUID().uuidString)
		try FileManager.default.createDirectory(at: bin, withIntermediateDirectories: true)
		addTeardownBlock { try? FileManager.default.removeItem(at: bin) }
		let library = LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in }, remove: { url in
			let moved = bin.appendingPathComponent(url.lastPathComponent)
			try FileManager.default.moveItem(at: url, to: moved)
			return moved
		})
		let doomed = try XCTUnwrap(library.machines.first { $0.name == "\(tag) Doomed" })

		let removed = library.delete(doomed)
		XCTAssertNil(library.notice)
		XCTAssertEqual(removed?.lastPathComponent, "\(tag) Doomed.uae")
		let folder = try MediaLibrary.path(of: .configurations)
		let left = try FileManager.default.contentsOfDirectory(atPath: folder).filter { $0.hasPrefix(tag) }
		XCTAssertEqual(left, ["\(tag) Kept.uae"], "the file leaves the configurations folder")
		XCTAssertEqual(library.machines.map(\.name).filter { $0.hasPrefix(tag) }, ["\(tag) Kept"])
	}

	func testRenamingAMachineKeepsWhatItDeclares() throws {
		let tag = try makeMachines([("Lemmings", "Lemmings on an A500", "Games"), ("Speedball", "", "")])
		let library = library()
		let lemmings = try XCTUnwrap(library.machines.first { $0.name == "\(tag) Lemmings" })

		let renamed = try XCTUnwrap(library.rename(lemmings, to: "  \(tag) Lemmings 2 "))
		XCTAssertEqual(renamed.name, "\(tag) Lemmings 2", "the name is trimmed")
		XCTAssertEqual(URL(fileURLWithPath: renamed.path).pathExtension, "uae")
		XCTAssertEqual(renamed.description, "Lemmings on an A500")
		XCTAssertEqual(renamed.category, "Games")
		XCTAssertFalse(FileManager.default.fileExists(atPath: lemmings.path), "the file is renamed, not copied")
		XCTAssertEqual(library.machines.map(\.name).filter { $0.hasPrefix(tag) }, ["\(tag) Lemmings 2", "\(tag) Speedball"])

		let shouted = try XCTUnwrap(library.rename(renamed, to: "\(tag) LEMMINGS 2"), "a change of case is not a taken name")
		XCTAssertEqual(shouted.name, "\(tag) LEMMINGS 2")
		XCTAssertNil(library.notice)
	}

	func testRenamingRefusesATakenOrInvalidName() throws {
		let tag = try makeMachines([("Lemmings", "", ""), ("Speedball", "", "")])
		let library = library()
		let lemmings = try XCTUnwrap(library.machines.first { $0.name == "\(tag) Lemmings" })

		XCTAssertNil(library.rename(lemmings, to: "\(tag) Speedball"))
		XCTAssertEqual(library.notice, "A machine named \u{201C}\(tag) Speedball\u{201D} already exists. Choose another name.")
		for name in ["\(tag) A/B", "\(tag) A:B", ".\(tag)", "  "] {
			library.notice = nil
			XCTAssertNil(library.rename(lemmings, to: name), name)
			XCTAssertNotNil(library.notice, "\(name) is refused with a notice")
		}
		XCTAssertTrue(FileManager.default.fileExists(atPath: lemmings.path), "a refused name changes nothing")
		XCTAssertEqual(library.machines.map(\.name).filter { $0.hasPrefix(tag) }, ["\(tag) Lemmings", "\(tag) Speedball"])
	}

	func testAMachineKeepsItsIconColour() {
		XCTAssertEqual(Machine.tint(for: "Workbench 3.1"), Machine.tint(for: "Workbench 3.1"))
		// Fixed values: the colour does not change from launch to launch.
		XCTAssertEqual(Machine.tintIndex(for: "Workbench 3.1"), 4)
		XCTAssertEqual(Machine.tintIndex(for: "A500"), 7)
		XCTAssertEqual(Machine.tint(for: "A500"), Machine.tints[7])
		let used = Set(["A500", "A1200", "CD32", "Workbench 3.1", "Lotus II", "Amiga 1200"].map(Machine.tintIndex(for:)))
		XCTAssertGreaterThan(used.count, 1, "machines get different colours")
	}
}
