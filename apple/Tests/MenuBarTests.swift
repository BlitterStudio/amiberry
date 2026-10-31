/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the menu bar's File and Help menus, the View menu's full
 * screen command, the shortcuts that step aside for the Amiga key and
 * Left ⌘Q (apple/macOS/MainMenu.swift).
 */

import AmiberrySettings
import AppKit
@testable import AmiberryNativeUI
import XCTest

@MainActor
final class MenuBarTests: XCTestCase {
	// No setUp (nonisolated before Swift 6.2): XCTest makes an instance per
	// test, so these start fresh for each.
	private lazy var commands: MenuCommands = {
		let commands = MenuCommands()
		commands.post = { _ in }
		commands.runState = { [weak self] in self?.state ?? .stopped }
		commands.settingsOpen = { false }
		commands.library = { [weak self] in self?.library }
		commands.fullScreen = { [weak self] in self?.fullScreen ?? false }
		commands.openURL = { _ in }
		return commands
	}()
	private var state = EmulatorStatus.RunState.running
	private var library: LibraryModel?
	private var fullScreen = false

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

	/// A menu bar as SDL makes it (the application menu's Preferences, Hide,
	/// Hide Others and Quit, a Window menu with Close and Minimize) with the
	/// commands installed.
	private func sdlMenu() -> NSMenu {
		let main = NSMenu(title: "Main")
		let appMenu = NSMenu(title: "Amiberry")
		appMenu.addItem(withTitle: "About Amiberry",
			action: #selector(NSApplication.orderFrontStandardAboutPanel(_:)), keyEquivalent: "")
		appMenu.addItem(.separator())
		appMenu.addItem(withTitle: "Preferences\u{2026}", action: nil, keyEquivalent: ",")
		appMenu.addItem(.separator())
		appMenu.addItem(withTitle: "Hide Amiberry", action: #selector(NSApplication.hide(_:)), keyEquivalent: "h")
		appMenu.addItem(withTitle: "Hide Others", action: #selector(NSApplication.hideOtherApplications(_:)),
			keyEquivalent: "h").keyEquivalentModifierMask = [.option, .command]
		appMenu.addItem(.separator())
		appMenu.addItem(withTitle: "Quit Amiberry", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
		main.addItem(withTitle: "Amiberry", action: nil, keyEquivalent: "").submenu = appMenu
		let windowMenu = NSMenu(title: "Window")
		windowMenu.addItem(withTitle: "Close", action: #selector(NSWindow.performClose(_:)), keyEquivalent: "w")
		windowMenu.addItem(withTitle: "Minimize", action: #selector(NSWindow.performMiniaturize(_:)), keyEquivalent: "m")
		windowMenu.addItem(withTitle: "Zoom", action: #selector(NSWindow.performZoom(_:)), keyEquivalent: "")
		main.addItem(withTitle: "Window", action: nil, keyEquivalent: "").submenu = windowMenu
		commands.install(into: main)
		return main
	}

	/// Every item of the menu and of its submenus.
	private func everyItem(_ menu: NSMenu) -> [NSMenuItem] {
		var pending = menu.items
		var all: [NSMenuItem] = []
		while let item = pending.popLast() {
			all.append(item)
			pending += item.submenu?.items ?? []
		}
		return all
	}

	private func submenu(_ main: NSMenu, _ title: String) throws -> NSMenu {
		try XCTUnwrap(main.items.first { $0.title == title }?.submenu, title)
	}

	private func item(_ main: NSMenu, _ menu: String, _ title: String) throws -> NSMenuItem {
		try XCTUnwrap(try submenu(main, menu).items.first { $0.title == title }, title)
	}

	private func run(_ item: NSMenuItem) {
		_ = (item.target as? NSObject)?.perform(item.action, with: item)
	}

	func testFileCommandsWaitForTheLibrary() throws {
		let main = menu()
		let presets = try XCTUnwrap(try item(main, "File", "New Machine").submenu)
		let entries = [try item(main, "File", "Play a File\u{2026}"), try item(main, "File", "Import ROMs\u{2026}")]
			+ presets.items
		XCTAssertFalse(entries.isEmpty)
		for entry in entries {
			XCTAssertFalse(commands.validateMenuItem(entry), "no Library: \(entry.title)")
		}
		var editorPresets: [String?] = []
		let model = LibraryModel(emulating: false, finish: {}, openEditor: { _, preset in editorPresets.append(preset?.id) })
		library = model
		for entry in entries {
			XCTAssertTrue(commands.validateMenuItem(entry), "the Library shows: \(entry.title)")
		}
		model.editing = true
		for entry in entries {
			XCTAssertFalse(commands.validateMenuItem(entry), "its editor is up: \(entry.title)")
		}
		model.editing = false
		let first = try XCTUnwrap(MachinePresets.all.first)
		run(try XCTUnwrap(presets.items.first))
		XCTAssertEqual(editorPresets, [first.id], "the editor opens with the model applied")
	}

	func testNewMachineListsEveryModel() throws {
		let main = menu()
		let presets = try XCTUnwrap(try item(main, "File", "New Machine").submenu)
		XCTAssertEqual(presets.items.count, MachinePresets.all.count)
		for (entry, preset) in zip(presets.items, MachinePresets.all) {
			XCTAssertEqual(entry.title, preset.label, "installed before the ROMs can be checked")
			// Opening the menu validates it, which marks what cannot run.
			_ = commands.validateMenuItem(entry)
			let expected = MachinePresets.isRunnable(preset) ? preset.label : "\(preset.label) (ROM not installed)"
			XCTAssertEqual(entry.title, expected)
		}
	}

	func testFullScreenSaysWhichWayItSwitches() throws {
		let entry = try XCTUnwrap(try submenu(menu(), "View").items.first)
		XCTAssertTrue(commands.validateMenuItem(entry))
		XCTAssertEqual(entry.title, "Enter Full Screen")
		XCTAssertEqual(entry.keyEquivalent, "f")
		fullScreen = true
		XCTAssertTrue(commands.validateMenuItem(entry))
		XCTAssertEqual(entry.title, "Exit Full Screen")
	}

	func testPlainCommandShortcutsStepAsideForTheEmulation() throws {
		let main = sdlMenu()
		// One added after the menus were installed is covered too.
		try submenu(main, "Window").addItem(withTitle: "Cycle Windows", action: nil, keyEquivalent: "`")
		let before = everyItem(main).map { ($0, $0.keyEquivalent, $0.keyEquivalentModifierMask) }
		XCTAssertEqual(try item(main, "Window", "Close").keyEquivalent, "w")
		XCTAssertEqual(try item(main, "Amiberry", "Quit Amiberry").keyEquivalent, "q")

		// An emulation window is key: ⌘W, ⌘M, ⌘H, ⌘Q and the rest are the
		// Amiga's, which a menu item would take even while disabled.
		commands.commandShortcuts(on: false, in: main)
		for entry in everyItem(main) where !entry.keyEquivalent.isEmpty {
			XCTAssertTrue(entry.keyEquivalentModifierMask.contains(.control),
				"\(entry.title) takes ⌘\(entry.keyEquivalent) from the Amiga")
		}
		XCTAssertEqual(try item(main, "Emulation", "Pause").keyEquivalent, "p", "the ⌃⌘ shortcuts stay")

		// Another emulation window becoming key changes nothing; a window of
		// the app's own gets every shortcut back as it was.
		commands.commandShortcuts(on: false, in: main)
		commands.commandShortcuts(on: true, in: main)
		for (entry, key, modifiers) in before {
			XCTAssertEqual(entry.keyEquivalent, key, entry.title)
			XCTAssertEqual(entry.keyEquivalentModifierMask, modifiers, entry.title)
		}
		XCTAssertEqual(try item(main, "Amiberry", "Hide Others").keyEquivalentModifierMask, [.option, .command])
	}

	private func keyDown(_ key: String, _ flags: NSEvent.ModifierFlags, device: UInt,
		unmodified: String? = nil, repeats: Bool = false) throws -> NSEvent {
		try XCTUnwrap(NSEvent.keyEvent(with: .keyDown, location: .zero,
			modifierFlags: NSEvent.ModifierFlags(rawValue: flags.rawValue | device), timestamp: 0, windowNumber: 0,
			context: nil, characters: key, charactersIgnoringModifiers: unmodified ?? key, isARepeat: repeats,
			keyCode: 12))
	}

	func testLeftCommandQFollowsItsSettingAndRightCommandQIsTheAmigas() throws {
		var posted: [EmulatorAction] = []
		var asked = 0
		var answer = (quit: false, stopAsking: false)
		var mode = LeftCommandQ.amiga
		var stored: [LeftCommandQ] = []
		var emulationKey = true
		commands.post = { posted.append($0) }
		commands.leftCommandQMode = { mode }
		commands.storeLeftCommandQMode = {
			stored.append($0)
			mode = $0
		}
		commands.emulationKey = { emulationKey }
		commands.confirmQuit = {
			asked += 1
			return answer
		}
		let left = try keyDown("q", .command, device: MenuCommands.leftCommandBit)

		XCTAssertFalse(commands.leftCommandQ(left), "set to the Amiga, Left Amiga-Q is the Amiga's")
		mode = .ask
		XCTAssertFalse(commands.leftCommandQ(try keyDown("q", .command, device: MenuCommands.rightCommandBit)),
			"Right Amiga-Q, the Amiga's menu shortcut, always reaches it")
		XCTAssertFalse(commands.leftCommandQ(try keyDown("q", .command,
			device: MenuCommands.leftCommandBit | MenuCommands.rightCommandBit)), "both Command keys")
		XCTAssertFalse(commands.leftCommandQ(try keyDown("q", [.command, .option], device: MenuCommands.leftCommandBit)))
		XCTAssertFalse(commands.leftCommandQ(try keyDown("w", .command, device: MenuCommands.leftCommandBit)))
		emulationKey = false
		XCTAssertFalse(commands.leftCommandQ(left), "another window has the menu's ⌘Q")
		emulationKey = true
		var settingsOpen = true
		commands.settingsOpen = { settingsOpen }
		XCTAssertFalse(commands.leftCommandQ(left), "the settings window holds the emulation")
		settingsOpen = false
		state = .stopped
		XCTAssertFalse(commands.leftCommandQ(left))
		XCTAssertEqual(asked, 0)

		state = .running
		let repeated = try keyDown("q", .command, device: MenuCommands.leftCommandBit, repeats: true)
		XCTAssertTrue(commands.leftCommandQ(repeated), "held down: the repeats do not reach the Amiga either")
		XCTAssertEqual(asked, 0, "nor ask again")
		mode = .amiga
		XCTAssertFalse(commands.leftCommandQ(repeated), "the Amiga's repeats stay the Amiga's")
		mode = .ask
		// A non-Latin layout types its own letter, and Q with Command held.
		answer = (quit: false, stopAsking: false)
		XCTAssertTrue(commands.leftCommandQ(try keyDown("q", .command, device: MenuCommands.leftCommandBit,
			unmodified: "\u{439}")))
		XCTAssertEqual(asked, 1)

		answer = (quit: false, stopAsking: true)
		XCTAssertTrue(commands.leftCommandQ(left), "cancelled, the Q still does not reach the Amiga")
		XCTAssertEqual(asked, 2)
		XCTAssertEqual(posted, [])
		XCTAssertEqual(stored, [], "Cancel keeps asking, checkbox or not")

		answer = (quit: true, stopAsking: false)
		XCTAssertTrue(commands.leftCommandQ(try keyDown("Q", [.command, .capsLock], device: MenuCommands.leftCommandBit)),
			"Caps Lock on")
		XCTAssertEqual(posted, [.quit])
		XCTAssertEqual(stored, [])

		answer = (quit: true, stopAsking: true)
		XCTAssertTrue(commands.leftCommandQ(left))
		XCTAssertEqual(stored, [.quit], "Don't ask again")
		XCTAssertEqual(posted, [.quit, .quit])
		XCTAssertTrue(commands.leftCommandQ(left))
		XCTAssertEqual(asked, 4, "quits without asking from then on")
		XCTAssertEqual(posted, [.quit, .quit, .quit])
	}
}
