/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the front-end logic shared with iOS (apple/Shared/UI), run in the
 * macOS app: the on-screen keyboard's keys and the requesters' file rules.
 */

@testable import AmiberryNativeUI
import Foundation
import XCTest

@MainActor
final class AmigaKeyboardTests: XCTestCase {
	private var posted: [EmulatorAction] = []

	private func keyboard() -> AmigaKeyboardModel {
		AmigaKeyboardModel { [weak self] in self?.posted.append($0) }
	}

	private func key(_ code: Int) throws -> AmigaKey {
		try XCTUnwrap(AmigaKeyboardLayout.allKeys.first { $0.code == code })
	}

	func testEveryKeyIsAnAmigaKeyOnce() {
		let codes = AmigaKeyboardLayout.allKeys.map(\.code)
		XCTAssertEqual(Set(codes).count, codes.count, "a key appears twice")
		XCTAssertTrue(codes.allSatisfy { (0...0x7F).contains($0) })
		for row in AmigaKeyboardLayout.main {
			XCTAssertEqual(row.reduce(0) { $0 + $1.width }, 15, "rows line up: \(row.map(\.label))")
		}
	}

	func testALatchedModifierComesUpAfterTheNextKey() throws {
		let keyboard = keyboard()
		let shift = try key(0x60), a = try key(0x20), b = try key(0x35)
		keyboard.touchDown(shift)
		keyboard.touchUp(shift)
		keyboard.touchDown(a)
		XCTAssertEqual(keyboard.down, [0x60, 0x20])
		keyboard.touchUp(a)
		keyboard.touchDown(b)
		keyboard.touchUp(b)
		XCTAssertEqual(posted, [
			.key(code: 0x60, pressed: true), .key(code: 0x20, pressed: true), .key(code: 0x20, pressed: false),
			.key(code: 0x60, pressed: false), .key(code: 0x35, pressed: true), .key(code: 0x35, pressed: false),
		])
		XCTAssertTrue(keyboard.down.isEmpty)
	}

	func testTappingALatchedModifierAgainLetsItGo() throws {
		let keyboard = keyboard()
		let ctrl = try key(0x63)
		keyboard.touchDown(ctrl)
		keyboard.touchDown(ctrl)
		XCTAssertEqual(posted, [.key(code: 0x63, pressed: true), .key(code: 0x63, pressed: false)])
		XCTAssertTrue(keyboard.latched.isEmpty)
	}

	func testCapsLockStaysDownUntilTappedAgain() throws {
		let keyboard = keyboard()
		let caps = try key(AmigaKey.capsLock), a = try key(0x20)
		keyboard.touchDown(caps)
		keyboard.touchDown(a)
		keyboard.touchUp(a)
		XCTAssertEqual(keyboard.down, [AmigaKey.capsLock], "a key typed does not release Caps Lock")
		keyboard.releaseAll()
		XCTAssertEqual(keyboard.down, [AmigaKey.capsLock], "hiding the keyboard keeps the Amiga's Caps Lock")
		keyboard.touchDown(caps)
		XCTAssertTrue(keyboard.down.isEmpty)
		XCTAssertEqual(posted.last, .key(code: AmigaKey.capsLock, pressed: false))
	}

	func testCapsLockFollowsTheEmulation() throws {
		let keyboard = keyboard()
		keyboard.syncCapsLock(true)
		XCTAssertEqual(keyboard.down, [AmigaKey.capsLock])
		XCTAssertTrue(posted.isEmpty, "following the LED sends nothing")
		keyboard.touchDown(try key(AmigaKey.capsLock))
		XCTAssertEqual(posted, [.key(code: AmigaKey.capsLock, pressed: false)], "one tap turns it off")
	}

	func testHidingReleasesHeldAndLatchedKeys() throws {
		let keyboard = keyboard()
		keyboard.touchDown(try key(0x64))
		keyboard.touchDown(try key(0x4C))
		posted = []
		keyboard.releaseAll()
		XCTAssertEqual(Set(posted), [.key(code: 0x64, pressed: false), .key(code: 0x4C, pressed: false)])
		XCTAssertTrue(keyboard.down.isEmpty)
		XCTAssertTrue(keyboard.latched.isEmpty)
		keyboard.touchUp(try key(0x4C))
		XCTAssertEqual(posted.count, 2, "a key released by hiding is not released twice")
	}

	func testACancelledTouchTypesNothing() throws {
		let keyboard = keyboard()
		let shift = try key(0x60), a = try key(0x20)
		keyboard.touchDown(shift)
		keyboard.touchUp(shift)
		keyboard.touchDown(a)
		keyboard.touchCancelled(a)
		XCTAssertEqual(keyboard.down, [0x60], "the latched Shift waits for a key that is typed")
		let ctrl = try key(0x63)
		keyboard.touchDown(ctrl)
		keyboard.touchCancelled(ctrl)
		XCTAssertFalse(keyboard.latched.contains(0x63), "a cancelled modifier touch latches nothing")
		XCTAssertEqual(posted.suffix(2), [.key(code: 0x63, pressed: true), .key(code: 0x63, pressed: false)])
		// A cancelled second tap leaves the latch on.
		keyboard.touchDown(shift)
		keyboard.touchCancelled(shift)
		XCTAssertTrue(keyboard.latched.contains(0x60))
		XCTAssertEqual(posted.suffix(2), [.key(code: 0x60, pressed: false), .key(code: 0x60, pressed: true)])
	}

	func testLayoutsChangeLegendsNotCodes() throws {
		let y = try key(0x15), z = try key(0x31), three = try key(0x03)
		XCTAssertEqual(AmigaKeyboardLayout.legend(of: y, layout: "DE").label, "Z")
		XCTAssertEqual(AmigaKeyboardLayout.legend(of: z, layout: "de").label, "Y")
		XCTAssertEqual(AmigaKeyboardLayout.legend(of: three, layout: "UK").shifted, "\u{00A3}")
		XCTAssertEqual(AmigaKeyboardLayout.legend(of: y, layout: "US").label, "Y")
		XCTAssertEqual(AmigaKeyboardLayout.legend(of: y, layout: "unknown").label, "Y")
	}
}

@MainActor
final class AppFilesTests: XCTestCase {
	// No setUp/tearDown (nonisolated before Swift 6.2): made on first use,
	// removed after the test.
	private lazy var folder: URL = {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("appfiles-\(UUID().uuidString)")
		try? FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		addTeardownBlock { try? FileManager.default.removeItem(at: folder) }
		return folder
	}()

	private func file(_ name: String, _ contents: String) -> URL {
		let url = folder.appendingPathComponent(name)
		FileManager.default.createFile(atPath: url.path, contents: Data(contents.utf8))
		return url
	}

	func testContainmentSeesThroughSymlinksAndPrefixes() {
		let root = folder.appendingPathComponent("Container").path
		XCTAssertTrue(AppFiles.contains(URL(fileURLWithPath: root + "/Documents/disk.adf"), root: root))
		XCTAssertTrue(AppFiles.contains(URL(fileURLWithPath: root), root: root))
		XCTAssertFalse(AppFiles.contains(URL(fileURLWithPath: root + "Other/disk.adf"), root: root),
			"a sibling sharing the prefix is outside")
		// /var is a symlink to /private/var, as on iOS.
		XCTAssertTrue(AppFiles.contains(URL(fileURLWithPath: "/private/var/tmp/x"), root: "/var/tmp"))
	}

	func testImportCopiesOnceAndNumbersOtherFilesOfTheSameName() throws {
		let source = folder.appendingPathComponent("source")
		let target = folder.appendingPathComponent("target")
		try FileManager.default.createDirectory(at: source, withIntermediateDirectories: true)
		let disk = file("source/Disk.adf", "one")
		let first = try AppFiles.importFile(disk, into: target)
		XCTAssertEqual(first, target.appendingPathComponent("Disk.adf").path)
		XCTAssertEqual(try AppFiles.importFile(disk, into: target), first, "the same disk again is not copied again")
		try Data("two".utf8).write(to: disk)
		XCTAssertEqual(try AppFiles.importFile(disk, into: target), target.appendingPathComponent("Disk 2.adf").path)
		XCTAssertEqual(try String(contentsOfFile: first, encoding: .utf8), "one", "the earlier copy is kept")
	}

	func testCueSheetsNameTheirTracks() {
		let sheet = "REM a comment\n"
			+ "FILE \"Game (Track 1).bin\" BINARY\n"
			+ "  TRACK 01 MODE1/2352\n"
			+ "file Track02.wav WAVE\n"
			+ "FILE\t\"Track03.bin\"\tBINARY\r\n"
			+ "\tFILE Track04.wav\tWAVE\n"
			+ "FILE \"Game (Track 1).bin\" BINARY\n"
			+ "FILES are not tracks\n"
			+ "FILE \"unterminated BINARY\n"
		// As the emulator's parser: any white space, an open quote to the end.
		XCTAssertEqual(AppFiles.cueTracks(sheet),
			["Game (Track 1).bin", "Track02.wav", "Track03.bin", "Track04.wav", "unterminated BINARY"])
	}

	func testACueSheetComesWithItsTracksOrNotAtAll() throws {
		let source = folder.appendingPathComponent("source")
		let target = folder.appendingPathComponent("CDs")
		try FileManager.default.createDirectory(at: source, withIntermediateDirectories: true)
		let cue = file("source/Game.cue", "FILE \"Game.bin\" BINARY\nFILE audio/Track02.wav WAVE\n")
		_ = file("source/Game.bin", "data")
		try FileManager.default.createDirectory(at: source.appendingPathComponent("audio"), withIntermediateDirectories: true)
		_ = file("source/audio/Track02.wav", "audio")

		let imported = try AppFiles.importFile(cue, into: target)
		XCTAssertEqual(imported, target.appendingPathComponent("Game/Game.cue").path, "in a folder of its own")
		XCTAssertEqual(try String(contentsOfFile: target.appendingPathComponent("Game/Game.bin").path, encoding: .utf8), "data")
		XCTAssertTrue(FileManager.default.fileExists(atPath: target.appendingPathComponent("Game/audio/Track02.wav").path))
		XCTAssertEqual(try AppFiles.importFile(cue, into: target), imported, "the same sheet again is not copied again")

		// Another CD whose sheet reads the same: its own tracks, its own folder.
		let other = folder.appendingPathComponent("other")
		try FileManager.default.createDirectory(at: other.appendingPathComponent("audio"), withIntermediateDirectories: true)
		let otherCue = file("other/Game.cue", "FILE \"Game.bin\" BINARY\nFILE audio/Track02.wav WAVE\n")
		_ = file("other/Game.bin", "other data")
		_ = file("other/audio/Track02.wav", "audio")
		XCTAssertEqual(try AppFiles.importFile(otherCue, into: target), target.appendingPathComponent("Game 2/Game.cue").path)
		XCTAssertEqual(try String(contentsOfFile: target.appendingPathComponent("Game 2/Game.bin").path, encoding: .utf8), "other data")
		XCTAssertEqual(try String(contentsOfFile: target.appendingPathComponent("Game/Game.bin").path, encoding: .utf8), "data",
			"the first CD keeps its tracks")

		let lonely = file("source/Lonely.cue", "FILE \"Missing.bin\" BINARY\n")
		XCTAssertThrowsError(try AppFiles.importFile(lonely, into: target)) { error in
			XCTAssertEqual((error as? CueTracksUnreadable)?.tracks, ["Missing.bin"])
		}
		XCTAssertFalse(FileManager.default.fileExists(atPath: target.appendingPathComponent("Lonely").path),
			"nothing is left of a sheet without its tracks")
		let escaping = file("source/Escaping.cue", "FILE \"../secret.bin\" BINARY\n")
		XCTAssertThrowsError(try AppFiles.importFile(escaping, into: target))
	}

	func testStateNames() {
		XCTAssertEqual(AppFiles.stateFile(named: " Game ", in: folder)?.lastPathComponent, "Game.uss")
		XCTAssertEqual(AppFiles.stateFile(named: "Game.USS", in: folder)?.lastPathComponent, "Game.uss",
			"the emulator would save Game.USS as Game.USS.uss")
		XCTAssertNil(AppFiles.stateFile(named: ".uss", in: folder))
		XCTAssertNil(AppFiles.stateFile(named: "  ", in: folder))
		XCTAssertNil(AppFiles.stateFile(named: "a/b", in: folder))
		XCTAssertNil(AppFiles.stateFile(named: "..", in: folder))
	}

	func testTheAutosaveNameIsKeptForTheApp() {
		XCTAssertTrue(AppFiles.isReservedStateName("Autosave"))
		XCTAssertTrue(AppFiles.isReservedStateName(" autosave.USS "))
		XCTAssertFalse(AppFiles.isReservedStateName("Autosave 2"))
	}

	func testPathOptionsAskForTheirKindOfFile() {
		XCTAssertEqual(FileRequest.forOption("floppy2"), .floppy)
		XCTAssertEqual(FileRequest.forOption("diskimage11"), .floppy)
		XCTAssertEqual(FileRequest.forOption("kickstart_ext_rom_file"), .rom)
		XCTAssertEqual(FileRequest.forOption("cart_file"), .rom)
		XCTAssertEqual(FileRequest.forOption("flash_file"), .file)
	}
}
