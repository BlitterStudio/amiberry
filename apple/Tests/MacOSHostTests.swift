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
	func testEveryGroupHasOneSection() {
		let placed = SettingsPage.placing.flatMap(\.groups)
		XCTAssertEqual(Set(placed).count, placed.count, "no group in two sections")
		for group in SettingsCatalog.groups {
			XCTAssertTrue(placed.contains(group), "\(group) has no section")
		}
		XCTAssertEqual(Set(SettingsPage.advanced.groups), Set(SettingsCatalog.groups), "All Settings lists every group")
		for id in SettingsPage.overviewOptions {
			XCTAssertNotNil(SettingsCatalog.descriptor(for: id), id)
		}
		XCTAssertEqual(SettingsPage.groupTitle("future_group"), "Future Group")
	}

	func testEveryPageRendersEveryOption() throws {
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		for page in SettingsPage.sidebar {
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
final class BoardsAndControlsTests: XCTestCase {
	func testBoardsAreAddedAndRemovedThroughTheStore() throws {
		let store = try SettingsStore()
		let a2091 = try XCTUnwrap(BoardCatalog.type("a2091"))
		XCTAssertEqual(BoardsSections.category(of: a2091), "SCSI")
		XCTAssertEqual(store.boardValue("enabled", type: "a2091", unit: 0), "false")
		XCTAssertFalse(store.hasChanges)
		XCTAssertTrue(store.setBoardValue("true", field: "enabled", type: "a2091", unit: 0))
		XCTAssertEqual(store.boardValue("enabled", type: "a2091", unit: 0), "true")
		XCTAssertTrue(store.hasChanges, "a board edit is a change to commit")
		XCTAssertFalse(store.setBoardValue("nope", field: "subtype", type: "a2091", unit: 0))
		XCTAssertNotNil(store.lastError)
		XCTAssertTrue(store.setBoardValue("false", field: "enabled", type: "a2091", unit: 0))
		// Refused text in a board field is named after the unit and field.
		XCTAssertEqual(SettingsWindowModel.draftLabel("board:a2091:1:rom_file"),
			"\(a2091.label) #2 \(a2091.fields.first { $0.id == "rom_file" }?.label ?? "")")
		XCTAssertEqual(SettingsWindowModel.draftLabel("cpu_model"), SettingsCatalog.descriptor(for: "cpu_model")?.label)
		// Every board type has a heading in the Add Board menu.
		for type in BoardCatalog.types {
			XCTAssertTrue(BoardsSections.categories.contains { $0.title == BoardsSections.category(of: type) }, type.id)
		}
	}

	func testABoardChangeWaitsForARestart() throws {
		let model = try SettingsWindowModel(emulating: true, dismiss: {}, post: { _ in true })
		XCTAssertFalse(model.boardsWaitForRestart)
		XCTAssertTrue(model.store.setBoardValue("true", field: "enabled", type: "a2091", unit: 0))
		XCTAssertTrue(model.boardsWaitForRestart, "the running emulation has the board only after a restart")
		XCTAssertTrue(model.store.setBoardValue("false", field: "enabled", type: "a2091", unit: 0))
		XCTAssertFalse(model.boardsWaitForRestart)
	}

	func testCustomControlsNeedAJoystick() throws {
		let store = try SettingsStore()
		// The test host has no joystick on the joystick port.
		XCTAssertNil(store.customControl("a", port: 1, hotkey: false))
		XCTAssertFalse(store.setCustomControl("Space", control: "a", port: 1, hotkey: false))
		XCTAssertFalse(store.hasChanges)
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		for page in [SettingsPage.hardware, .input] {
			let view = NSHostingView(rootView: PageView(page: page, model: model).frame(width: 700, height: 500))
			view.frame = NSRect(x: 0, y: 0, width: 700, height: 500)
			view.layoutSubtreeIfNeeded()
		}
	}
}

@MainActor
final class LibraryTests: XCTestCase {
	func testLibraryListsSavedMachinesWithTheirMetadata() throws {
		let folder = try MediaLibrary.path(of: .configurations)
		let name = "library-\(UUID().uuidString)"
		let file = URL(fileURLWithPath: folder).appendingPathComponent(name + ".uae")
		try "config_description=Lotus II on an A500\nconfig_category=Games\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		let library = LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in })
		let machine = try XCTUnwrap(library.machines.first { $0.name == name })
		XCTAssertEqual(machine.description, "Lotus II on an A500")
		XCTAssertEqual(machine.category, "Games")
		XCTAssertTrue(library.categories.contains("Games"))
		XCTAssertTrue(library.machines(in: "Games").contains(machine))
		XCTAssertFalse(library.machines(in: "Nothing").contains(machine))
		let view = NSHostingView(rootView: LibraryView(model: library))
		view.frame = NSRect(x: 0, y: 0, width: 860, height: 560)
		view.layoutSubtreeIfNeeded()
	}

	func testSaveDoesNotReplaceAnotherMachine() async throws {
		let folder = try MediaLibrary.path(of: .configurations)
		let name = "taken-\(UUID().uuidString)"
		let file = URL(fileURLWithPath: folder).appendingPathComponent(name + ".uae")
		try "config_description=Keep me\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		let saved = await model.save(name: name, description: "Other", category: "")
		XCTAssertFalse(saved)
		XCTAssertTrue(model.notice?.contains("already exists") ?? false, model.notice ?? "")
		XCTAssertEqual(try String(contentsOf: file, encoding: .utf8), "config_description=Keep me\n")
		// Nothing was committed either.
		XCTAssertNotEqual(try SettingsSession.activeValue(of: "config_description"), "Other")
		XCTAssertNotEqual(try SettingsSession().value(of: "config_description"), "Other")
		let slashed = await model.save(name: "a/b", description: "", category: "")
		XCTAssertFalse(slashed)
		XCTAssertTrue(model.notice?.contains("cannot contain") ?? false, model.notice ?? "")
	}

	func testANewMachineShowsItsModelInQuickStart() throws {
		let a1200 = try XCTUnwrap(MachinePresets.preset(for: "a1200"))
		// The pending configuration was saved as a machine: it has that file.
		let saved = FileManager.default.temporaryDirectory.appendingPathComponent("current-\(UUID().uuidString).uae")
		defer { try? FileManager.default.removeItem(at: saved) }
		XCTAssertNotNil(EmulatorActions.post(.saveConfiguration(path: saved.path)))
		ActionPump.runPending()
		XCTAssertEqual(MediaLibrary.currentConfiguration().map { URL(fileURLWithPath: $0).standardizedFileURL.path },
			saved.standardizedFileURL.path)
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		model.startNewMachine(preset: a1200.id, configuration: 0)
		XCTAssertNil(MediaLibrary.currentConfiguration(), "a new machine is no saved file's")
		XCTAssertEqual(model.page, .overview)
		XCTAssertEqual(model.newMachineModel?.id, "a1200")
		XCTAssertEqual(model.store.value(of: "chipset"), "aga")
	}

	func testTheLibraryWaitsWhileAnEditorIsOpen() {
		var opened = 0
		let library = LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in opened += 1 })
		library.editing = true
		library.newMachine(preset: "a500")
		XCTAssertEqual(opened, 0, "the open editor is not replaced")
		library.editing = false
		library.newMachine(preset: "a500")
		XCTAssertEqual(opened, 1)
	}

	func testSavingAMachineInPlaceKeepsItsFile() async throws {
		let folder = try MediaLibrary.path(of: .configurations)
		let name = "Upper-\(UUID().uuidString)"
		let file = URL(fileURLWithPath: folder).appendingPathComponent(name + ".UAE")
		try "config_description=Old\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		let machine = Machine(path: file.path, name: name, description: "Old", category: "")
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true },
			machine: machine, closeToLibrary: {})
		// The save runs as an action: pump the queue as the emulator would.
		let saving = Task { await model.save(name: name, description: "New", category: "") }
		var saved: Bool?
		for _ in 0..<200 where saved == nil {
			ActionPump.runPending()
			try await Task.sleep(for: .milliseconds(10))
			if model.notice != nil || MediaLibrary.configurationInfo(file.path)?.description == "New" {
				saved = await saving.value
			}
		}
		XCTAssertEqual(saved, true, model.notice ?? "")
		XCTAssertEqual(MediaLibrary.configurationInfo(file.path)?.description, "New")
		// One file, spelled as before.
		let names = try FileManager.default.contentsOfDirectory(atPath: folder).filter { $0.hasPrefix(name) }
		XCTAssertEqual(names, [name + ".UAE"])
	}

	func testClosingAnEditorFromTheLibraryGoesBackToIt() throws {
		var closed = 0
		var dismissed = 0
		let model = try SettingsWindowModel(
			emulating: false, dismiss: { dismissed += 1 }, post: { _ in true },
			machine: nil, closeToLibrary: { closed += 1 })
		XCTAssertTrue(model.fromLibrary)
		model.closeRequested()
		XCTAssertEqual(closed, 1, "back to the Library")
		XCTAssertEqual(dismissed, 0, "nothing quits or starts")
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
		XCTAssertEqual(MountSections.mountField("/Volumes/Work"), "/Volumes/Work")
		XCTAssertEqual(MountSections.mountField("/a,b"), "\"/a,b\"")
		XCTAssertEqual(MountSections.mountField("/a "), "\"/a \"")
		XCTAssertNil(MountSections.mountField("/a\"b"))
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
		try session.addMount("dir,rw,DH9:Work:\(try XCTUnwrap(MountSections.mountField(path))),0")
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
