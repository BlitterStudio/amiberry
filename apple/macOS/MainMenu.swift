/*
 * Copyright (C) 2026 BlitterStudio
 * Menu bar commands of the native macOS front end. The emulation's commands
 * post a runtime action, which the emulator runs between frames; the File
 * menu's act through the Library, and the Help menu's open the web pages.
 *
 * Shortcuts use Control-Command: Command alone is the Amiga key, and the
 * emulated programs need Amiga-letter combinations. The plain-⌘ ones (the
 * Edit menu's, Settings and SDL's own) are taken off while an emulation
 * window is key (commandShortcuts); Left ⌘Q then asks to quit, quits or
 * goes to the Amiga, as set (leftCommandQ).
 */

import AppKit

@MainActor
final class MenuCommands: NSObject, NSMenuItemValidation {
	static let shared = MenuCommands()

	/// Posts an action and reports its failure; replaced by the tests.
	var post: (EmulatorAction) -> Void = { ReportedActions.post($0) }
	/// The emulator's run state; replaced by the tests.
	var runState: () -> EmulatorStatus.RunState = { EmulatorStatus.current().runState }
	/// The emulator's drives and their media; replaced by the tests.
	var status: () -> EmulatorStatus = { EmulatorStatus.current() }
	/// Whether the settings window is up (it has the media then); replaced
	/// by the tests.
	var settingsOpen: () -> Bool = { SettingsPresenter.shared.isVisible }
	/// The Library, while it shows; replaced by the tests.
	var library: () -> LibraryModel? = { SettingsPresenter.shared.library }
	/// Whether an emulation window is full screen; replaced by the tests.
	var fullScreen: () -> Bool = {
		NSApplication.shared.windows.contains { EmulationChrome.isEmulation($0) && $0.styleMask.contains(.fullScreen) }
	}
	/// Opens a web page; replaced by the tests.
	var openURL: (URL) -> Void = { _ = NSWorkspace.shared.open($0) }
	/// What Left ⌘Q does while an emulation window is key (the host
	/// setting; a value it cannot hold asks, as the default does); replaced
	/// by the tests.
	var leftCommandQMode: () -> LeftCommandQ = {
		LeftCommandQ(rawValue: (try? HostSettings.value(of: LeftCommandQ.settingID)) ?? "") ?? .ask
	}
	/// Whether an emulation window is key; replaced by the tests.
	var emulationKey: () -> Bool = { EmulationChrome.isEmulation(NSApplication.shared.keyWindow) }
	/// Asks whether to quit, and whether to stop asking (the checkbox);
	/// replaced by the tests.
	var confirmQuit: () -> (quit: Bool, stopAsking: Bool) = {
		let (button, stopAsking) = Alerts.show(title: "Quit Amiberry?",
			message: "The machine stops. Anything it has not saved is lost.",
			buttons: ["Quit", "Cancel"], suppression: "Don\u{2019}t ask again")
		return (button == 0, stopAsking)
	}
	/// Stores a new Left ⌘Q mode and saves amiberry.conf, as the app
	/// settings' picker does; replaced by the tests. A failure is told
	/// before the app quits.
	var storeLeftCommandQMode: (LeftCommandQ) -> Void = { mode in
		try? HostSettings.set(mode.rawValue, for: LeftCommandQ.settingID)
		if !HostSettings.save() {
			Alerts.show(title: "The settings could not be saved.",
				message: "Left \u{2318}Q asks again the next time Amiberry runs.", buttons: ["OK"])
		}
	}

	static let fileTitle = "File"
	static let editTitle = "Edit"
	/// The plain-⌘ shortcuts taken off while an emulation window is key,
	/// and what each was, to put back.
	private var takenShortcuts: [(item: NSMenuItem, key: String, modifiers: NSEvent.ModifierFlags)] = []
	static let emulationTitle = "Emulation"
	static let mediaTitle = "Media"
	static let viewTitle = "View"
	static let helpTitle = "Help"

	/// The Help menu's pages: nil is a separator.
	static let helpLinks: [(title: String, url: URL)?] = [
		("Amiberry Help", URL(string: "https://github.com/BlitterStudio/amiberry/wiki")!),
		("Quick Start Guide", URL(string: "https://github.com/BlitterStudio/amiberry/wiki/Quick-start-guide")!),
		("Troubleshooting", URL(string: "https://github.com/BlitterStudio/amiberry/wiki/Troubleshooting")!),
		nil,
		("Report an Issue\u{2026}", URL(string: "https://github.com/BlitterStudio/amiberry/issues")!),
		("Amiberry Website", URL(string: "https://amiberry.com")!),
	]

	/// Adds the commands to the menu bar SDL created: Settings in the
	/// application menu, a File menu after it, Emulation, Media and View
	/// menus before Window, and a Help menu last. Adding them again changes
	/// nothing.
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
			// About shows the standard panel with credits: what the emulator
			// is built on and where to read more.
			if let about = appMenu.items.first(where: {
				$0.action == #selector(NSApplication.orderFrontStandardAboutPanel(_:)) || $0.title.hasPrefix("About ")
			}) {
				about.target = self
				about.action = #selector(showAbout(_:))
			}
		}

		let emulation = NSMenu(title: MenuCommands.emulationTitle)
		emulation.addItem(item("Machine Settings\u{2026}", #selector(openSettings(_:)), key: "e"))
		emulation.addItem(.separator())
		emulation.addItem(item("Pause", #selector(togglePause(_:)), key: "p"))
		emulation.addItem(item("Reset", #selector(reset(_:)), key: "r"))
		emulation.addItem(item("Keyboard Reset", #selector(keyboardReset(_:))))
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
		view.addItem(item("Enter Full Screen", #selector(toggleFullscreen(_:)), key: "f"))
		view.addItem(.separator())
		// The responder chain's: the emulation window's toolbar can be laid
		// out to taste (the other windows' cannot, so it is off there).
		view.addItem(NSMenuItem(title: "Customize Toolbar\u{2026}",
			action: #selector(NSWindow.runToolbarCustomizationPalette(_:)), keyEquivalent: ""))

		// The Library's commands, so they are found where a Mac app has them.
		let file = NSMenu(title: MenuCommands.fileTitle)
		let presets = NSMenu(title: "New Machine")
		for preset in MachinePresets.all {
			// Titled by its label until the menu opens: whether its ROM is
			// installed is asked then (validateMenuItem), since the ROM check
			// builds the model's settings, which needs the emulator's input
			// devices, and the menus are installed before they exist.
			let entry = item(preset.label, #selector(newMachine(_:)))
			entry.representedObject = preset.id
			presets.addItem(entry)
		}
		file.addItem(holder(presets))
		file.addItem(item("Play a File\u{2026}", #selector(playFile(_:))))
		file.addItem(.separator())
		file.addItem(item("Import ROMs\u{2026}", #selector(importROMs(_:))))

		// Text editing for the Library, settings and Save fields: AppKit's
		// text fields only cut, copy and paste through these items. Their
		// ⌘ shortcuts are the Amiga key's while an emulation window is key,
		// so they are taken off then (commandShortcuts).
		let edit = NSMenu(title: MenuCommands.editTitle)
		for entry in MenuCommands.editEntries {
			guard let entry else {
				edit.addItem(.separator())
				continue
			}
			// The responder chain's: the focused field takes them.
			let editItem = NSMenuItem(title: entry.title, action: entry.action, keyEquivalent: entry.key)
			editItem.keyEquivalentModifierMask = entry.modifiers
			edit.addItem(editItem)
		}

		let help = NSMenu(title: MenuCommands.helpTitle)
		for link in MenuCommands.helpLinks {
			guard let link else {
				help.addItem(.separator())
				continue
			}
			let entry = item(link.title, #selector(openHelpLink(_:)))
			entry.representedObject = link.url
			help.addItem(entry)
		}

		mainMenu.insertItem(holder(file), at: min(1, mainMenu.items.count))
		mainMenu.insertItem(holder(edit), at: min(2, mainMenu.items.count))
		if let windowMenu = mainMenu.items.first(where: { $0.submenu?.title == "Window" })?.submenu {
			// SDL's Window menu has a Toggle Full Screen of its own on the
			// same ⌃⌘F, which switches the window behind the emulator's back:
			// View's is the one that keeps the emulator's setting.
			for entry in windowMenu.items where entry.action == #selector(NSWindow.toggleFullScreen(_:)) {
				windowMenu.removeItem(entry)
			}
			// SDL quits when its window closes: Close asks to return to the
			// Library for an emulation window instead (closeWindow).
			for entry in windowMenu.items where entry.action == #selector(NSWindow.performClose(_:)) {
				entry.target = self
				entry.action = #selector(closeWindow(_:))
			}
		}
		let windowIndex = mainMenu.items.firstIndex { $0.submenu == NSApplication.shared.windowsMenu }
			?? mainMenu.items.count
		for (offset, menu) in [emulation, media, view].enumerated() {
			mainMenu.insertItem(holder(menu), at: min(windowIndex + offset, mainMenu.items.count))
		}
		mainMenu.addItem(holder(help))
		// Only the application's own menu bar gets the system's search field
		// in its Help menu, and has its shortcuts follow the key window.
		if mainMenu === NSApplication.shared.mainMenu {
			NSApplication.shared.helpMenu = help
			followKeyWindow(NSApplication.shared.keyWindow)
			let center = NotificationCenter.default
			// The notification's window: as the app is activated, the window
			// becomes key before NSApp.keyWindow names it. Its address crosses
			// into the main actor, which posts the notification.
			center.addObserver(forName: NSWindow.didBecomeKeyNotification, object: nil, queue: .main) { note in
				let address = (note.object as? NSWindow).map { UInt(bitPattern: Unmanaged.passUnretained($0).toOpaque()) }
				MainActor.assumeIsolated {
					let window = address.flatMap { UnsafeMutableRawPointer(bitPattern: $0) }
						.map { Unmanaged<NSWindow>.fromOpaque($0).takeUnretainedValue() }
					MenuCommands.shared.followKeyWindow(window)
				}
			}
			// With no window key (the emulation's minimised, say) the
			// shortcuts are the app's again. The next key window is known
			// once the change is over.
			center.addObserver(forName: NSWindow.didResignKeyNotification, object: nil, queue: .main) { _ in
				Task { @MainActor in MenuCommands.shared.followKeyWindow(NSApplication.shared.keyWindow) }
			}
			// Left ⌘Q does what the setting says (leftCommandQ).
			// SDL's event pump takes its events through NSApp, so the
			// emulation window's keys come by here first.
			_ = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { event in
				let taken = MainActor.assumeIsolated { MenuCommands.shared.leftCommandQ(event) }
				return taken ? nil : event
			}
		}
	}

	/// The Edit menu: title, action, ⌘ shortcut; nil is a separator.
	static let editEntries: [(title: String, action: Selector, key: String, modifiers: NSEvent.ModifierFlags)?] = [
		("Undo", Selector(("undo:")), "z", [.command]),
		("Redo", Selector(("redo:")), "z", [.command, .shift]),
		nil,
		("Cut", #selector(NSText.cut(_:)), "x", [.command]),
		("Copy", #selector(NSText.copy(_:)), "c", [.command]),
		("Paste", #selector(NSText.paste(_:)), "v", [.command]),
		("Delete", #selector(NSText.delete(_:)), "", []),
		("Select All", #selector(NSText.selectAll(_:)), "a", [.command]),
	]

	/// Takes the menu bar's plain-⌘ shortcuts off while an emulation window
	/// is key, and puts them back while another is.
	func followKeyWindow(_ window: NSWindow?) {
		guard let mainMenu = NSApplication.shared.mainMenu else { return }
		commandShortcuts(on: !EmulationChrome.isEmulation(window), in: mainMenu)
	}

	/// Puts the menu's plain-⌘ shortcuts back (a window other than an
	/// emulation's is key) or takes them off (an emulation window is):
	/// Command alone is the Amiga key, and a menu item takes its key
	/// equivalent even while disabled, so ⌘Q would quit and ⌘C copy instead
	/// of reaching the Amiga. The ⌃⌘ ones stay. Each shortcut is read as it
	/// is taken off, so SDL's items and any added since the menus were
	/// installed are covered too.
	func commandShortcuts(on: Bool, in menu: NSMenu) {
		if on {
			for (item, key, modifiers) in takenShortcuts {
				item.keyEquivalent = key
				item.keyEquivalentModifierMask = modifiers
			}
			takenShortcuts.removeAll()
			return
		}
		var items = menu.items
		while let item = items.popLast() {
			items += item.submenu?.items ?? []
			let modifiers = item.keyEquivalentModifierMask
			guard !item.keyEquivalent.isEmpty, modifiers.contains(.command), !modifiers.contains(.control) else { continue }
			takenShortcuts.append((item, item.keyEquivalent, modifiers))
			item.keyEquivalent = ""
			item.keyEquivalentModifierMask = []
		}
	}

	/// NX_DEVICELCMDKEYMASK and NX_DEVICERCMDKEYMASK (IOKit's IOLLEvent.h):
	/// the device-dependent bits of an event's modifier flags that tell the
	/// left Command key from the right one. A menu shortcut cannot.
	static let leftCommandBit: UInt = 0x08
	static let rightCommandBit: UInt = 0x10

	/// Whether a key press is Q with the left Command key alone: no other
	/// modifier and the right Command key up. Q is what the layout types
	/// with Command held (as a menu matches it), or without.
	static func isLeftCommandQ(_ event: NSEvent) -> Bool {
		guard event.type == .keyDown,
			event.modifierFlags.intersection([.shift, .control, .option, .command]) == .command,
			[event.characters, event.charactersIgnoringModifiers].contains(where: { $0?.lowercased() == "q" })
		else { return false }
		let flags = event.modifierFlags.rawValue
		return flags & leftCommandBit != 0 && flags & rightCommandBit == 0
	}

	/// Left ⌘Q while an emulation window is key: what the setting says.
	/// Asking, the requester's "Don't ask again" makes Left ⌘Q quit at once
	/// from then on, when Quit is chosen (the app settings bring the
	/// question back). Either way the Q never reaches the Amiga. Right ⌘Q,
	/// and Left ⌘Q set to go to the Amiga, are the Amiga's (Right or Left
	/// Amiga-Q). Returns whether the key press was taken.
	func leftCommandQ(_ event: NSEvent) -> Bool {
		guard MenuCommands.isLeftCommandQ(event), emulationKey(), runState() != .stopped, !settingsOpen()
		else { return false }
		// Held down, the key repeats: the repeats go where the press went,
		// so the Amiga never gets a Q it did not get first.
		if event.isARepeat {
			return leftCommandQMode() != .amiga
		}
		switch leftCommandQMode() {
		case .amiga:
			return false
		case .quit:
			post(.quit)
		case .ask:
			let answer = confirmQuit()
			guard answer.quit else { break }
			if answer.stopAsking {
				storeLeftCommandQMode(.quit)
			}
			post(.quit)
		}
		return true
	}

	private func holder(_ menu: NSMenu) -> NSMenuItem {
		let holder = NSMenuItem(title: menu.title, action: nil, keyEquivalent: "")
		holder.submenu = menu
		return holder
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

	@objc func showAbout(_ sender: Any?) {
		NSApplication.shared.activate(ignoringOtherApps: true)
		var options: [NSApplication.AboutPanelOptionKey: Any] = [.credits: MenuCommands.aboutCredits]
		// The panel puts the build in brackets after the version; a build
		// numbered as its version would read twice ("8.5.0 (8.5.0)").
		let info = Bundle.main.infoDictionary
		if let build = info?["CFBundleVersion"] as? String, build == info?["CFBundleShortVersionString"] as? String {
			options[.version] = ""
		}
		NSApplication.shared.orderFrontStandardAboutPanel(options: options)
	}

	/// The About panel's credits: the emulation core it is built on, the
	/// Acknowledgements for the rest, and the project's site.
	static var aboutCredits: NSAttributedString {
		let paragraph = NSMutableParagraphStyle()
		paragraph.alignment = .center
		let body: [NSAttributedString.Key: Any] = [
			.font: NSFont.systemFont(ofSize: NSFont.smallSystemFontSize),
			.foregroundColor: NSColor.secondaryLabelColor,
			.paragraphStyle: paragraph,
		]
		let credits = NSMutableAttributedString(
			string: "An Amiga emulator built on the UAE and WinUAE emulation core.\n"
				+ "Amiberry \u{203A} Acknowledgements lists every component and its licence.\n",
			attributes: body)
		var link = body
		link[.link] = URL(string: "https://amiberry.com")
		credits.append(NSAttributedString(string: "amiberry.com", attributes: link))
		return credits
	}

	@objc func togglePause(_ sender: Any?) {
		post(runState() == .paused ? .resume : .pause)
	}

	@objc func reset(_ sender: Any?) { post(.reset(hard: false, keyboard: false)) }
	@objc func keyboardReset(_ sender: Any?) { post(.reset(hard: false, keyboard: true)) }
	/// Stops the machine for the Library; what it has not saved is lost,
	/// so this asks first. The window's close button comes here too, also
	/// while the machine's settings are up: those are dealt with first.
	@objc func returnToLibrary(_ sender: Any?) {
		guard !settingsOpen() else {
			NSSound.beep()
			return
		}
		let (button, _) = Alerts.show(title: "Return to the Library?",
			message: "The machine stops. Anything it has not saved is lost.", buttons: ["Return to Library", "Cancel"])
		if button == 0 {
			post(.stop)
		}
	}

	/// Window › Close: an emulation window asks to return to the Library
	/// (closing it would quit the emulator); any other window closes.
	@objc func closeWindow(_ sender: Any?) {
		guard let window = NSApplication.shared.keyWindow else { return }
		if EmulationChrome.isEmulation(window) {
			returnToLibrary(sender)
		} else {
			window.performClose(sender)
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

	/// A new machine from the model the item names: the Library's editor
	/// opens with it applied.
	@objc func newMachine(_ sender: NSMenuItem) {
		guard let id = sender.representedObject as? String else { return }
		library()?.newMachine(preset: id)
	}

	/// The Library asks for the file: the command is only enabled while it
	/// shows (libraryReady()).
	@objc func playFile(_ sender: Any?) {
		library()?.chooseFileToPlay()
	}

	@objc func importROMs(_ sender: Any?) {
		guard let model = library() else { return }
		model.importROMs(FilePanels.chooseROMs(title: ROMImport.prompt))
	}

	@objc func openHelpLink(_ sender: NSMenuItem) {
		guard let url = sender.representedObject as? URL else { return }
		openURL(url)
	}

	/// Emulation commands and the requesters need a running emulation (the
	/// settings window has the media then); Pause reads Resume while paused,
	/// and Full Screen says which way it switches. A drive's commands follow
	/// the drive: none for a disabled one, Eject only with media in it. The
	/// File menu's commands need the Library, and wait while it loads a
	/// machine or its editor is up.
	func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
		let state = runState()
		switch menuItem.action {
		case #selector(togglePause(_:)):
			menuItem.title = state == .paused ? "Resume" : "Pause"
			// The settings window holds its own pause; its Resume button ends it.
			return state != .stopped && !settingsOpen()
		case #selector(saveState(_:)), #selector(loadState(_:)), #selector(insertCD(_:)):
			return state != .stopped && !settingsOpen()
		case #selector(insertFloppy(_:)):
			let drives = status().floppies
			return state != .stopped && !settingsOpen() && drives.indices.contains(menuItem.tag)
				&& !drives[menuItem.tag].disabled
		case #selector(ejectFloppy(_:)):
			let drives = status().floppies
			return drives.indices.contains(menuItem.tag) && !drives[menuItem.tag].disabled
				&& !drives[menuItem.tag].mediaPath.isEmpty
		case #selector(ejectCD(_:)):
			return !status().cdPath.isEmpty
		case #selector(toggleFullscreen(_:)):
			menuItem.title = fullScreen() ? "Exit Full Screen" : "Enter Full Screen"
			return state != .stopped
		case #selector(reset(_:)), #selector(keyboardReset(_:)), #selector(hardReset(_:)), #selector(quickSave(_:)),
			#selector(quickLoad(_:)), #selector(screenshot(_:)):
			return state != .stopped
		case #selector(newMachine(_:)):
			// The marks follow ROMs imported since the menu was built.
			if let id = menuItem.representedObject as? String,
				let preset = MachinePresets.all.first(where: { $0.id == id }) {
				menuItem.title = MenuCommands.newMachineTitle(preset)
			}
			return libraryReady()
		case #selector(playFile(_:)), #selector(importROMs(_:)):
			return libraryReady()
		case #selector(returnToLibrary(_:)):
			return state != .stopped && !settingsOpen()
		case #selector(closeWindow(_:)):
			guard let window = NSApplication.shared.keyWindow else { return false }
			return EmulationChrome.isEmulation(window)
				? state != .stopped && !settingsOpen() : window.styleMask.contains(.closable)
		case #selector(openSettings(_:)):
			// The machine's editor opens over a running emulation; before one
			// runs, the Library and its editor are what shows.
			return state != .stopped && !settingsOpen()
		default:
			return true
		}
	}

	/// A model's name, marked as the Library marks it when no ROM it can
	/// run is installed.
	static func newMachineTitle(_ preset: MachinePreset) -> String {
		MachinePresets.isRunnable(preset) ? preset.label : "\(preset.label) (ROM not installed)"
	}

	private func libraryReady() -> Bool {
		guard let model = library() else { return false }
		return !model.editing && !model.busy
	}
}

/// What Left ⌘Q does while an emulation window is key: the host setting
/// macos_left_command_q's values.
enum LeftCommandQ: String {
	case amiga = "0"
	case ask = "1"
	case quit = "2"

	static let settingID = "macos_left_command_q"
}
