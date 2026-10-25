/*
 * Copyright (C) 2026 BlitterStudio
 * Menu bar commands of the native macOS front end. Every command posts a
 * runtime action, which the emulator runs between frames.
 *
 * Shortcuts use Control-Command: Command alone is the Amiga key, and the
 * emulated programs need Amiga-letter combinations.
 */

import AppKit

@MainActor
final class MenuCommands: NSObject, NSMenuItemValidation {
	static let shared = MenuCommands()

	/// Posts an action and reports its failure; replaced by the tests.
	var post: (EmulatorAction) -> Void = { ReportedActions.post($0) }
	/// The emulator's run state; replaced by the tests.
	var runState: () -> EmulatorStatus.RunState = { EmulatorStatus.current().runState }
	/// Whether the settings window is up (it has the media then); replaced
	/// by the tests.
	var settingsOpen: () -> Bool = { SettingsPresenter.shared.isVisible }

	static let emulationTitle = "Emulation"
	static let mediaTitle = "Media"
	static let viewTitle = "View"

	/// Adds the commands to the menu bar SDL created: Settings in the
	/// application menu, and Emulation, Media and View menus before Window.
	/// Adding them again changes nothing.
	func install(into mainMenu: NSMenu) {
		guard !mainMenu.items.contains(where: { $0.submenu?.title == MenuCommands.emulationTitle }) else { return }

		if let appMenu = mainMenu.items.first?.submenu {
			// SDL's application menu has a Settings (Preferences) item of its
			// own, with nothing behind it: it opens the app settings now (the
			// emulator's own settings; a machine's are in Emulation).
			if let existing = appMenu.items.first(where: { $0.keyEquivalent == "," }) {
				existing.title = "Settings\u{2026}"
				existing.target = self
				existing.action = #selector(openAppSettings(_:))
				existing.keyEquivalentModifierMask = [.command]
			} else {
				let settings = item("Settings\u{2026}", #selector(openAppSettings(_:)), key: ",", modifiers: [.command])
				let index = appMenu.items.firstIndex(where: { $0.isSeparatorItem }).map { $0 + 1 } ?? appMenu.items.count
				appMenu.insertItem(settings, at: index)
				appMenu.insertItem(.separator(), at: index + 1)
			}
			if appMenu.items.contains(where: { $0.title == "Acknowledgements\u{2026}" }) == false {
				let acknowledgements = item("Acknowledgements\u{2026}", #selector(openAcknowledgements(_:)))
				let settingsIndex = appMenu.items.firstIndex { $0.action == #selector(openAppSettings(_:)) }
				appMenu.insertItem(acknowledgements, at: (settingsIndex ?? appMenu.items.count - 1) + 1)
			}
		}

		let emulation = NSMenu(title: MenuCommands.emulationTitle)
		emulation.addItem(item("Machine Settings\u{2026}", #selector(openSettings(_:)), key: "e"))
		emulation.addItem(.separator())
		emulation.addItem(item("Pause", #selector(togglePause(_:)), key: "p"))
		emulation.addItem(item("Reset", #selector(reset(_:)), key: "r"))
		emulation.addItem(item("Hard Reset", #selector(hardReset(_:)), key: "r", modifiers: [.control, .command, .shift]))
		emulation.addItem(.separator())
		emulation.addItem(item("Quick Save State", #selector(quickSave(_:)), key: "s"))
		emulation.addItem(item("Quick Load State", #selector(quickLoad(_:)), key: "l"))
		emulation.addItem(item("Save State\u{2026}", #selector(saveState(_:))))
		emulation.addItem(item("Load State\u{2026}", #selector(loadState(_:))))
		emulation.addItem(.separator())
		emulation.addItem(item("Save Screenshot", #selector(screenshot(_:))))
		emulation.addItem(.separator())
		emulation.addItem(item("Return to Library", #selector(returnToLibrary(_:))))

		let media = NSMenu(title: MenuCommands.mediaTitle)
		for drive in 0..<EmulatorActions.floppyDriveCount {
			let insert = item("Insert Disk in DF\(drive):\u{2026}", #selector(insertFloppy(_:)), key: "\(drive)")
			insert.tag = drive
			media.addItem(insert)
		}
		media.addItem(.separator())
		for drive in 0..<EmulatorActions.floppyDriveCount {
			let eject = item("Eject DF\(drive):", #selector(ejectFloppy(_:)))
			eject.tag = drive
			media.addItem(eject)
		}
		media.addItem(.separator())
		media.addItem(item("Insert CD\u{2026}", #selector(insertCD(_:))))
		media.addItem(item("Eject CD", #selector(ejectCD(_:))))

		let view = NSMenu(title: MenuCommands.viewTitle)
		view.addItem(item("Toggle Full Screen", #selector(toggleFullscreen(_:)), key: "f"))

		let windowIndex = mainMenu.items.firstIndex { $0.submenu == NSApplication.shared.windowsMenu }
			?? mainMenu.items.count
		for (offset, menu) in [emulation, media, view].enumerated() {
			let holder = NSMenuItem(title: menu.title, action: nil, keyEquivalent: "")
			holder.submenu = menu
			mainMenu.insertItem(holder, at: min(windowIndex + offset, mainMenu.items.count))
		}
	}

	private func item(
		_ title: String, _ action: Selector, key: String = "",
		modifiers: NSEvent.ModifierFlags = [.control, .command]
	) -> NSMenuItem {
		let item = NSMenuItem(title: title, action: action, keyEquivalent: key)
		item.keyEquivalentModifierMask = key.isEmpty ? [] : modifiers
		item.target = self
		return item
	}

	// MARK: Commands

	@objc func openSettings(_ sender: Any?) { post(.openSettings) }

	@objc func openAppSettings(_ sender: Any?) { AppSettingsWindow.shared.show() }

	@objc func openAcknowledgements(_ sender: Any?) { AcknowledgementsWindow.shared.show() }

	@objc func togglePause(_ sender: Any?) {
		post(runState() == .paused ? .resume : .pause)
	}

	@objc func reset(_ sender: Any?) { post(.reset(hard: false, keyboard: false)) }
	/// Stops the machine for the Library; what it has not saved is lost,
	/// so this asks first.
	@objc func returnToLibrary(_ sender: Any?) {
		let (button, _) = Alerts.show(title: "Return to the Library?",
			message: "The machine stops. Anything it has not saved is lost.", buttons: ["Return to Library", "Cancel"])
		if button == 0 {
			post(.stop)
		}
	}
	@objc func hardReset(_ sender: Any?) { post(.reset(hard: true, keyboard: false)) }
	@objc func quickSave(_ sender: Any?) { post(.saveStateSlot(0)) }
	@objc func quickLoad(_ sender: Any?) { post(.loadStateSlot(0)) }

	// The requesters run where the hotkeys run them (gui_display), which
	// pauses the emulation and releases input while they show.
	@objc func saveState(_ sender: Any?) { post(.chooseMedia(.saveState)) }
	@objc func loadState(_ sender: Any?) { post(.chooseMedia(.loadState)) }

	@objc func screenshot(_ sender: Any?) {
		let folder = (try? MediaLibrary.path(of: .screenshots)) ?? ""
		post(.screenshot(path: MenuCommands.screenshotPath(in: folder, at: Date())))
	}

	/// A screenshot file name that names no existing file: the time to the
	/// millisecond, then a number if that is taken.
	static func screenshotPath(in folder: String, at date: Date) -> String {
		let formatter = DateFormatter()
		formatter.locale = Locale(identifier: "en_US_POSIX")
		formatter.dateFormat = "yyyy-MM-dd 'at' HH.mm.ss.SSS"
		let base = URL(fileURLWithPath: folder).appendingPathComponent("Amiberry \(formatter.string(from: date))")
		var path = base.appendingPathExtension("png").path
		var number = 2
		while FileManager.default.fileExists(atPath: path) {
			path = base.path + " \(number).png"
			number += 1
		}
		return path
	}

	@objc func insertFloppy(_ sender: NSMenuItem) { post(.chooseMedia(.floppy(drive: sender.tag))) }

	@objc func ejectFloppy(_ sender: NSMenuItem) { post(.ejectFloppy(drive: sender.tag)) }

	@objc func insertCD(_ sender: Any?) { post(.chooseMedia(.cd)) }

	@objc func ejectCD(_ sender: Any?) { post(.ejectCD) }
	@objc func toggleFullscreen(_ sender: Any?) { post(.toggleFullscreen) }

	/// Emulation commands and the requesters need a running emulation (the
	/// settings window has the media then); Pause reads Resume while paused.
	func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
		let state = runState()
		switch menuItem.action {
		case #selector(togglePause(_:)):
			menuItem.title = state == .paused ? "Resume" : "Pause"
			// The settings window holds its own pause; its Resume button ends it.
			return state != .stopped && !settingsOpen()
		case #selector(saveState(_:)), #selector(loadState(_:)), #selector(insertFloppy(_:)), #selector(insertCD(_:)):
			return state != .stopped && !settingsOpen()
		case #selector(reset(_:)), #selector(hardReset(_:)), #selector(quickSave(_:)), #selector(quickLoad(_:)),
			#selector(screenshot(_:)), #selector(toggleFullscreen(_:)):
			return state != .stopped
		case #selector(returnToLibrary(_:)):
			return state != .stopped && !settingsOpen()
		case #selector(openSettings(_:)):
			// The machine's editor opens over a running emulation; before one
			// runs, the Library and its editor are what shows.
			return state != .stopped && !settingsOpen()
		default:
			return true
		}
	}
}
