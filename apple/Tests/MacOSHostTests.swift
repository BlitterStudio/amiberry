/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the native macOS front end (apple/macOS) against the real
 * layer: the settings window's pages and actions, and the menu commands.
 */

import AppKit
@testable import AmiberryNativeUI
import SwiftUI
import XCTest

@MainActor
final class SettingsPagesTests: XCTestCase {
	func testEveryGroupHasOnePage() {
		let pages = SettingsPage.sections().flatMap(\.pages)
		XCTAssertEqual(Set(pages).count, pages.count, "no page twice")
		for group in SettingsCatalog.groups {
			XCTAssertTrue(pages.contains(.group(group)), group)
		}
		// A group the sidebar does not know yet still gets a page.
		let sections = SettingsPage.sections(groups: SettingsCatalog.groups + ["future_group"])
		XCTAssertEqual(sections.last?.title, "Other")
		XCTAssertEqual(sections.last?.pages, [.group("future_group")])
		XCTAssertEqual(SettingsPage.groupTitle("future_group"), "Future Group")
	}

	func testEveryPageRendersEveryOption() throws {
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		for page in SettingsPage.sections().flatMap(\.pages) {
			let view = NSHostingView(rootView: PageView(page: page, model: model).frame(width: 700, height: 500))
			view.frame = NSRect(x: 0, y: 0, width: 700, height: 500)
			view.layoutSubtreeIfNeeded()
			XCTAssertGreaterThan(view.fittingSize.height, 0, page.title)
		}
		let root = NSHostingView(rootView: SettingsRootView(model: model))
		root.frame = NSRect(x: 0, y: 0, width: 900, height: 620)
		root.layoutSubtreeIfNeeded()
	}

	func testSearchFindsOptionsByLabelHelpAndKey() {
		XCTAssertTrue(SearchResultsView.matches("cpu_model").contains { $0.id == "cpu_model" })
		XCTAssertTrue(SearchResultsView.matches("24-bit").contains { $0.id == "cpu_24bit_addressing" })
		XCTAssertTrue(SearchResultsView.matches("nothing matches this text").isEmpty)
		let both = SearchResultsView.matches("jit fpu")
		XCTAssertTrue(both.contains { $0.id == "compfpu" })
	}
}

@MainActor
final class SettingsWindowModelTests: XCTestCase {
	private var posted: [EmulatorAction] = []
	private var dismissed = 0

	private func model(emulating: Bool) throws -> SettingsWindowModel {
		try SettingsWindowModel(
			emulating: emulating, dismiss: { [weak self] in self?.dismissed += 1 },
			post: { [weak self] in self?.posted.append($0); return true })
	}


	func testStartCommitsAndCloses() throws {
		let model = try model(emulating: false)
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(model.store.set(other, for: "immediate_blits"))
		model.startOrResume()
		XCTAssertEqual(dismissed, 1)
		XCTAssertEqual(posted, [], "nothing runs yet, so nothing to apply")
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), other, "committed")
		XCTAssertTrue(model.store.set(current, for: "immediate_blits"))
		try model.store.commit()
	}

	func testClosingTheWindow() throws {
		try model(emulating: false).closeRequested()
		XCTAssertEqual(posted, [.quit], "closing before anything runs quits")
		posted = []
		try model(emulating: true).closeRequested()
		XCTAssertEqual(posted, [.applySettings(.live)], "closing over an emulation resumes it")
		XCTAssertEqual(dismissed, 2)
	}

	func testResetAndRestartApplyWithTheirStep() throws {
		let running = try model(emulating: true)
		running.reset()
		running.restart()
		XCTAssertEqual(posted, [.applySettings(.reset), .applySettings(.restart)])
	}

	func testWaitingListsChangesTheRunningEmulationDoesNotUseYet() throws {
		XCTAssertTrue(try model(emulating: false).waiting.isEmpty, "nothing runs")
		let running = try model(emulating: true)
		// Chip RAM only changes at a reset.
		let chip = try XCTUnwrap(SettingsCatalog.descriptor(for: "chipmem_size"))
		XCTAssertEqual(chip.applyClass, .reset)
		let active = try SettingsSession.activeValue(of: chip.id)
		// Earlier tests may have stored chip RAM the emulation does not use yet.
		let waitedBefore = running.waiting[.reset]?.contains { $0.id == chip.id } ?? false
		let other = try XCTUnwrap(running.store.choices(for: chip.id).first {
			$0.value != active && running.store.isChoiceAvailable($0.value, for: chip.id) })
		XCTAssertTrue(running.store.set(other.value, for: chip.id))
		XCTAssertTrue(running.waiting[.reset]?.contains { $0.id == chip.id } ?? false)
		XCTAssertNil(running.waiting[.live], "live options never wait")
		try running.store.revert()
		XCTAssertEqual(running.waiting[.reset]?.contains { $0.id == chip.id } ?? false, waitedBefore)
	}

	func testInvalidFieldsKeepTheWindowOpen() throws {
		let model = try model(emulating: true)
		model.invalidDrafts["cpu_multiplier"] = "abc"
		model.startOrResume()
		model.reset()
		XCTAssertEqual(dismissed, 0)
		XCTAssertEqual(posted, [])
		XCTAssertTrue(model.notice?.hasPrefix("Correct ") ?? false)
		model.invalidDrafts.removeAll()
		model.startOrResume()
		XCTAssertEqual(dismissed, 1)
	}

	func testRefusedPostsKeepTheWindowOpen() throws {
		let model = try SettingsWindowModel(
			emulating: true, dismiss: { [weak self] in self?.dismissed += 1 }, post: { _ in false })
		model.startOrResume()
		model.reset()
		model.quit()
		XCTAssertEqual(dismissed, 0)
		XCTAssertEqual(model.notice, "The emulator did not accept the request. Try again.")
	}

	func testExternalRequestsStoreTheEdits() throws {
		let model = try model(emulating: true)
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(model.store.set(other, for: "immediate_blits"))
		model.commitForExternalRequest()
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), other)
		XCTAssertTrue(model.store.set(current, for: "immediate_blits"))
		try model.store.commit()
	}

	func testMountFieldsAreSpelledAsTheWriterSpellsThem() {
		XCTAssertEqual(HardDrivesPage.mountField("/Volumes/Work"), "/Volumes/Work")
		XCTAssertEqual(HardDrivesPage.mountField("/a,b"), "\"/a,b\"")
		XCTAssertEqual(HardDrivesPage.mountField("/a "), "\"/a \"")
		XCTAssertNil(HardDrivesPage.mountField("/a\"b"))
	}

	func testMountedPathsWithCommasAreAccepted() throws {
		let created = FileManager.default.temporaryDirectory.appendingPathComponent("a,b \(UUID().uuidString)")
		try FileManager.default.createDirectory(at: created, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: created) }
		let real = try XCTUnwrap(realpath(created.path, nil))
		defer { free(real) }
		let path = String(cString: real)
		let session = try SettingsSession()
		let before = try session.mounts().count
		try session.addMount("dir,rw,DH9:Work:\(try XCTUnwrap(HardDrivesPage.mountField(path))),0")
		XCTAssertEqual(try session.mounts().count, before + 1)
		XCTAssertTrue(try session.mounts().last?.contains("a,") ?? false)
	}

	func testErrorsReadAsSentences() {
		XCTAssertEqual(SettingsWindowModel.describe(SettingsError.invalidValue(id: "cpu_model", value: "1")),
			"CPU model: \u{201C}1\u{201D} is not a valid value.")
		XCTAssertTrue(SettingsWindowModel.describe(SettingsError.unavailable("cpu_24bit_addressing"))
			.hasPrefix("24-bit addressing is not available now."))
	}
}

@MainActor
final class MenuCommandsTests: XCTestCase {
	// No setUp (nonisolated before Swift 6.2): XCTest makes an instance per
	// test, so these start fresh for each.
	private lazy var commands: MenuCommands = {
		let commands = MenuCommands()
		commands.post = { [weak self] in self?.posted.append($0) }
		commands.runState = { [weak self] in self?.state ?? .stopped }
		commands.settingsOpen = { [weak self] in self?.settingsOpen ?? false }
		return commands
	}()
	private var posted: [EmulatorAction] = []
	private var state = EmulatorStatus.RunState.running
	private var settingsOpen = false


	private func menu() -> NSMenu {
		let main = NSMenu(title: "Main")
		let app = NSMenuItem(title: "Amiberry", action: nil, keyEquivalent: "")
		app.submenu = NSMenu(title: "Amiberry")
		app.submenu?.addItem(withTitle: "About Amiberry", action: nil, keyEquivalent: "")
		app.submenu?.addItem(.separator())
		app.submenu?.addItem(withTitle: "Quit Amiberry", action: nil, keyEquivalent: "q")
		main.addItem(app)
		commands.install(into: main)
		return main
	}

	private func item(_ main: NSMenu, _ menu: String, _ title: String) throws -> NSMenuItem {
		let submenu = try XCTUnwrap(main.items.first { $0.title == menu }?.submenu, menu)
		return try XCTUnwrap(submenu.items.first { $0.title == title }, title)
	}

	private func run(_ item: NSMenuItem) {
		_ = (item.target as? NSObject)?.perform(item.action, with: item)
	}

	func testInstallsOnceWithControlCommandShortcuts() throws {
		let main = menu()
		let before = main.items.count
		commands.install(into: main)
		XCTAssertEqual(main.items.count, before, "installed once")
		XCTAssertEqual(main.items.map(\.title), ["Amiberry", "Emulation", "Media", "View"])
		let settings = try item(main, "Amiberry", "Settings\u{2026}")
		XCTAssertEqual(settings.keyEquivalent, ",")
		let reset = try item(main, "Emulation", "Reset")
		XCTAssertEqual(reset.keyEquivalentModifierMask, [.control, .command], "Command alone is the Amiga key")
	}

	func testReusesTheSettingsItemSDLCreated() throws {
		let main = NSMenu(title: "Main")
		let app = NSMenuItem(title: "Amiberry", action: nil, keyEquivalent: "")
		app.submenu = NSMenu(title: "Amiberry")
		app.submenu?.addItem(withTitle: "About Amiberry", action: nil, keyEquivalent: "")
		app.submenu?.addItem(.separator())
		app.submenu?.addItem(withTitle: "Preferences\u{2026}", action: nil, keyEquivalent: ",")
		main.addItem(app)
		commands.install(into: main)
		let commaItems = try XCTUnwrap(app.submenu).items.filter { $0.keyEquivalent == "," }
		XCTAssertEqual(commaItems.count, 1, "one Settings item")
		XCTAssertEqual(commaItems.first?.title, "Settings\u{2026}")
		run(try XCTUnwrap(commaItems.first))
		XCTAssertEqual(posted, [.openSettings])
	}

	func testCommandsPostTheirActions() throws {
		let main = menu()
		run(try item(main, "Amiberry", "Settings\u{2026}"))
		run(try item(main, "Emulation", "Reset"))
		run(try item(main, "Emulation", "Hard Reset"))
		run(try item(main, "Emulation", "Quick Save State"))
		run(try item(main, "Emulation", "Save State\u{2026}"))
		run(try item(main, "Media", "Insert Disk in DF1:\u{2026}"))
		run(try item(main, "Media", "Eject DF2:"))
		run(try item(main, "Media", "Eject CD"))
		run(try item(main, "View", "Toggle Full Screen"))
		run(try item(main, "Emulation", "Load State\u{2026}"))
		run(try item(main, "Media", "Insert CD\u{2026}"))
		XCTAssertEqual(posted, [
			.openSettings, .reset(hard: false, keyboard: false), .reset(hard: true, keyboard: false),
			.saveStateSlot(0), .chooseMedia(.saveState), .chooseMedia(.floppy(drive: 1)),
			.ejectFloppy(drive: 2), .ejectCD, .toggleFullscreen, .chooseMedia(.loadState), .chooseMedia(.cd),
		])
	}

	func testScreenshotNamesNeverOverwrite() throws {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("shots-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: folder) }
		let now = Date()
		let first = MenuCommands.screenshotPath(in: folder.path, at: now)
		FileManager.default.createFile(atPath: first, contents: Data())
		let second = MenuCommands.screenshotPath(in: folder.path, at: now)
		XCTAssertNotEqual(first, second)
		XCTAssertTrue(second.hasSuffix(" 2.png"))
	}

	func testPauseFollowsTheRunState() throws {
		let main = menu()
		let pause = try item(main, "Emulation", "Pause")
		XCTAssertTrue(commands.validateMenuItem(pause))
		run(pause)
		state = .paused
		XCTAssertTrue(commands.validateMenuItem(pause))
		XCTAssertEqual(pause.title, "Resume")
		run(pause)
		XCTAssertEqual(posted, [.pause, .resume])
		state = .stopped
		XCTAssertFalse(commands.validateMenuItem(pause))
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Emulation", "Reset")))
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Media", "Insert CD\u{2026}")))
		XCTAssertTrue(commands.validateMenuItem(try item(main, "Media", "Eject CD")))
		// The settings window has the media while it is up.
		state = .paused
		settingsOpen = true
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Media", "Insert CD\u{2026}")))
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Emulation", "Save State\u{2026}")))
		XCTAssertTrue(commands.validateMenuItem(try item(main, "Emulation", "Reset")))
		XCTAssertFalse(commands.validateMenuItem(pause), "the settings window holds the pause")
	}
}
