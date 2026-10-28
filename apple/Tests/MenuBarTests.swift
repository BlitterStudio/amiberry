/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the menu bar's File and Help menus, the View menu's full
 * screen command and the shortcuts that step aside for the Amiga key
 * (apple/macOS/MainMenu.swift).
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
}
