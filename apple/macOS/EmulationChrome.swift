/*
 * Copyright (C) 2026 BlitterStudio
 * The emulation window's toolbar: Drives, Pause, Reset, State, the status
 * LEDs and frame rate, Settings and Full Screen. SDL owns the window; the
 * toolbar sits in its title bar, so the emulation's drawable keeps its size.
 */

import AppKit
import SwiftUI

@MainActor
final class EmulationChrome: NSObject, NSToolbarDelegate, NSToolbarItemValidation, NSMenuDelegate,
	NSMenuItemValidation {
	static let shared = EmulationChrome()

	private let monitor = StatusMonitor()
	/// The fixed menus' entries, by menu title; filled as they open.
	private var menuEntries: [String: [(String, AnyObject, Selector)]] = [:]
	/// Each emulation window's full-screen observers: an RTG screen on a
	/// second emulated monitor has a window of its own.
	private var watched: [ObjectIdentifier: (window: WeakWindow, observers: [NSObjectProtocol])] = [:]

	private final class WeakWindow {
		weak var window: NSWindow?
		init(_ window: NSWindow) { self.window = window }
	}

	private enum Item {
		static let drives = NSToolbarItem.Identifier("amiberry.drives")
		static let pause = NSToolbarItem.Identifier("amiberry.pause")
		static let reset = NSToolbarItem.Identifier("amiberry.reset")
		static let state = NSToolbarItem.Identifier("amiberry.state")
		static let status = NSToolbarItem.Identifier("amiberry.status")
		static let settings = NSToolbarItem.Identifier("amiberry.settings")
		static let fullScreen = NSToolbarItem.Identifier("amiberry.fullscreen")
	}

	/// Puts the toolbar on the emulation window SDL just created. A window
	/// created full screen gets it too: it shows once the window is one.
	func attach(to window: NSWindow) {
		guard window.toolbar == nil else { return }
		let toolbar = NSToolbar(identifier: "AmiberryEmulation")
		toolbar.delegate = self
		toolbar.displayMode = .iconOnly
		toolbar.allowsUserCustomization = false
		// Adding a toolbar must grow the frame around the content, not move
		// or shrink it: SDL places the window by its content and saves where
		// that is, so a shifted content would creep down at every creation.
		let content = window.contentRect(forFrameRect: window.frame)
		let fullScreen = window.styleMask.contains(.fullScreen)
		window.toolbarStyle = .unifiedCompact
		// The toolbar takes the title's room (a 640-point Amiga window has
		// no more); the title stays for the Window menu and Mission Control.
		window.titleVisibility = .hidden
		window.toolbar = toolbar
		toolbar.isVisible = !fullScreen
		if window.styleMask.contains(.titled), !fullScreen, window.contentRect(forFrameRect: window.frame) != content {
			window.setFrame(window.frameRect(forContentRect: content), display: true)
		}
		observeFullScreen(of: window)
		MacKeyboard.shared.attach(to: window)
		monitor.start()
	}

	/// Full screen (a Space; SDL's desktop full screen) has no toolbar: the
	/// emulation gets the whole display. SDL is the window's delegate, so
	/// this watches the notifications.
	private func observeFullScreen(of window: NSWindow) {
		let key = ObjectIdentifier(window)
		let center = NotificationCenter.default
		func observe(_ name: Notification.Name, _ toolbarVisible: Bool?) -> NSObjectProtocol {
			center.addObserver(forName: name, object: window, queue: .main) { _ in
				MainActor.assumeIsolated { EmulationChrome.shared.windowChanged(key, toolbarVisible: toolbarVisible) }
			}
		}
		watched[key]?.observers.forEach(center.removeObserver)
		watched[key] = (WeakWindow(window), [
			observe(NSWindow.willEnterFullScreenNotification, false),
			observe(NSWindow.didExitFullScreenNotification, true),
			// Closed (SDL destroys it): nothing more to watch.
			observe(NSWindow.willCloseNotification, nil),
		])
	}

	private func windowChanged(_ key: ObjectIdentifier, toolbarVisible: Bool?) {
		guard let entry = watched[key] else { return }
		if let toolbarVisible {
			entry.window.window?.toolbar?.isVisible = toolbarVisible
		} else {
			entry.observers.forEach(NotificationCenter.default.removeObserver)
			watched[key] = nil
		}
	}

	// MARK: NSToolbarDelegate

	func toolbarDefaultItemIdentifiers(_ toolbar: NSToolbar) -> [NSToolbarItem.Identifier] {
		[Item.drives, Item.pause, Item.reset, Item.state, .flexibleSpace, Item.status, .flexibleSpace,
		 Item.settings, Item.fullScreen]
	}

	func toolbarAllowedItemIdentifiers(_ toolbar: NSToolbar) -> [NSToolbarItem.Identifier] {
		toolbarDefaultItemIdentifiers(toolbar)
	}

	func toolbar(
		_ toolbar: NSToolbar, itemForItemIdentifier identifier: NSToolbarItem.Identifier,
		willBeInsertedIntoToolbar flag: Bool
	) -> NSToolbarItem? {
		let commands = MenuCommands.shared
		switch identifier {
		case Item.drives:
			// A menu rebuilt from the status as it opens. (A popover from a
			// title bar button showed far below it on SDL's window.)
			return menu(identifier, "opticaldiscdrive", "Drives", "Insert and eject disks and CDs", [])
		case Item.pause:
			return button(identifier, "pause", "Pause", "Pause or resume the emulation", #selector(togglePause(_:)))
		case Item.reset:
			return menu(identifier, "arrow.counterclockwise", "Reset", "Reset the Amiga", [
				("Reset", commands, #selector(MenuCommands.reset(_:))),
				("Keyboard Reset", self, #selector(keyboardReset(_:))),
				("Hard Reset", commands, #selector(MenuCommands.hardReset(_:))),
			])
		case Item.state:
			return menu(identifier, "camera.on.rectangle", "State", "Save or load the machine's state", [
				("Quick Save State", commands, #selector(MenuCommands.quickSave(_:))),
				("Quick Load State", commands, #selector(MenuCommands.quickLoad(_:))),
				("Save State\u{2026}", commands, #selector(MenuCommands.saveState(_:))),
				("Load State\u{2026}", commands, #selector(MenuCommands.loadState(_:))),
			])
		case Item.status:
			let item = NSToolbarItem(itemIdentifier: identifier)
			item.view = NSHostingView(rootView: StatusLEDsView(monitor: monitor))
			// On a narrow window the commands stay and the status goes.
			item.visibilityPriority = .low
			return labelled(item, "Status", "Power, drive activity and frame rate")
		case Item.settings:
			return button(identifier, "gearshape", "Settings", "Change this machine's settings", #selector(openSettings(_:)))
		case Item.fullScreen:
			return button(identifier, "arrow.up.left.and.arrow.down.right", "Full Screen",
				"Switch between window and full screen", #selector(toggleFullscreen(_:)))
		default:
			return nil
		}
	}

	// MARK: Validation

	/// Like the menu: what needs a running emulation, or the media the
	/// settings window holds, waits for them. Pause reads Resume while paused.
	/// (The menus' entries are validated as the menu bar's are.)
	func validateToolbarItem(_ item: NSToolbarItem) -> Bool {
		let state = monitor.status.runState
		let settingsOpen = SettingsPresenter.shared.isVisible
		switch item.itemIdentifier {
		case Item.pause:
			item.image = symbol(state == .paused ? "play" : "pause", state == .paused ? "Resume" : "Pause")
			item.label = state == .paused ? "Resume" : "Pause"
			return state != .stopped && !settingsOpen
		case Item.settings:
			return state != .stopped && !settingsOpen
		default:
			return state != .stopped
		}
	}

	// MARK: Commands

	@objc func togglePause(_ sender: Any?) { MenuCommands.shared.togglePause(sender) }

	@objc func openSettings(_ sender: Any?) { MenuCommands.shared.openSettings(sender) }

	@objc func toggleFullscreen(_ sender: Any?) { MenuCommands.shared.toggleFullscreen(sender) }

	@objc func keyboardReset(_ sender: Any?) {
		MenuCommands.shared.post(.reset(hard: false, keyboard: true))
	}

	@objc func insertSwapperEntry(_ sender: NSMenuItem) {
		guard let target = sender.representedObject as? [Int], target.count == 2 else { return }
		MenuCommands.shared.post(.insertSwapperEntry(index: target[0], drive: target[1]))
	}

	// MARK: The Drives menu

	/// Every toolbar menu is filled as it opens, the same way: a toolbar
	/// menu is a pull-down, whose first item is the button's title and never
	/// shows.
	func menuNeedsUpdate(_ menu: NSMenu) {
		if menu.title == "Drives" {
			DrivesMenu.fill(menu, status: EmulatorStatus.current(),
				swapper: (0..<EmulatorActions.swapperEntryCount).map(EmulatorActions.swapperEntry),
				commands: MenuCommands.shared, swapperTarget: self, pullDown: true)
			return
		}
		menu.removeAllItems()
		menu.addItem(NSMenuItem(title: menu.title, action: nil, keyEquivalent: ""))
		for (title, target, action) in menuEntries[menu.title] ?? [] {
			let entry = NSMenuItem(title: title, action: action, keyEquivalent: "")
			entry.target = target
			menu.addItem(entry)
		}
	}

	/// This one's own entries, like the menu bar's: a reset needs a running
	/// emulation, a swapper insert also the media the settings window holds.
	func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
		let state = EmulatorStatus.current().runState
		switch menuItem.action {
		case #selector(keyboardReset(_:)):
			return state != .stopped
		case #selector(insertSwapperEntry(_:)):
			return state != .stopped && !SettingsPresenter.shared.isVisible
		default:
			return true
		}
	}

	// MARK: Building items

	private func symbol(_ name: String, _ description: String) -> NSImage {
		NSImage(systemSymbolName: name, accessibilityDescription: description) ?? NSImage()
	}

	private func labelled(_ item: NSToolbarItem, _ label: String, _ tip: String) -> NSToolbarItem {
		item.label = label
		item.paletteLabel = label
		item.toolTip = tip
		return item
	}

	private func button(
		_ identifier: NSToolbarItem.Identifier, _ image: String, _ label: String, _ tip: String, _ action: Selector
	) -> NSToolbarItem {
		let item = NSToolbarItem(itemIdentifier: identifier)
		item.image = symbol(image, label)
		item.target = self
		item.action = action
		item.isBordered = true
		item.autovalidates = true
		item.visibilityPriority = .high
		return labelled(item, label, tip)
	}

	private func menu(
		_ identifier: NSToolbarItem.Identifier, _ image: String, _ label: String, _ tip: String,
		_ entries: [(String, AnyObject, Selector)]
	) -> NSMenuToolbarItem {
		let item = NSMenuToolbarItem(itemIdentifier: identifier)
		item.image = symbol(image, label)
		item.showsIndicator = true
		menuEntries[label] = entries
		let menu = NSMenu(title: label)
		menu.delegate = self
		menuNeedsUpdate(menu)
		item.menu = menu
		item.label = label
		item.paletteLabel = label
		item.toolTip = tip
		return item
	}
}

/// The power LED, a LED per floppy drive (and the hard drive and CD when
/// there are any), and the frame rate.
struct StatusLEDsView: View {
	let monitor: StatusMonitor

	var body: some View {
		let status = monitor.status
		HStack(spacing: 10) {
			LED(label: "Power", lit: status.powerLED, color: .red)
			ForEach(Array(status.floppies.enumerated()), id: \.offset) { index, drive in
				if !drive.disabled {
					LED(label: "DF\(index)", lit: drive.motorOn, color: drive.writing ? .orange : .green)
				}
			}
			if let activity = status.hardDriveActivity {
				LED(label: "HD", lit: activity > 0, color: .orange)
			}
			if let activity = status.cdActivity {
				LED(label: "CD", lit: activity > 0, color: .blue)
			}
			Text(status.runState == .paused ? "Paused" : StatusLEDsView.fps(status.fpsTenths))
				.font(.caption.monospacedDigit())
				.foregroundStyle(.secondary)
				.frame(minWidth: 56, alignment: .trailing)
		}
		.padding(.horizontal, 6)
		.fixedSize()
		.accessibilityElement(children: .combine)
	}

	static func fps(_ tenths: Int) -> String {
		tenths > 0 ? String(format: "%.1f fps", Double(tenths) / 10) : ""
	}
}

private struct LED: View {
	let label: String
	let lit: Bool
	let color: Color

	var body: some View {
		VStack(spacing: 1) {
			RoundedRectangle(cornerRadius: 1.5)
				.fill(lit ? color : color.opacity(0.18))
				.frame(width: 14, height: 5)
			Text(label).font(.system(size: 8, weight: .medium)).foregroundStyle(.secondary)
		}
		.accessibilityLabel("\(label) \(lit ? "on" : "off")")
	}
}

/// The Drives menu: each drive with its disk, Insert and Eject (the menu
/// bar's commands), the CD when there is one, and the disk swapper's entries
/// to insert into a drive.
@MainActor
enum DrivesMenu {
	static func fill(
		_ menu: NSMenu, status: EmulatorStatus, swapper: [String], commands: MenuCommands, swapperTarget: AnyObject,
		pullDown: Bool = false
	) {
		menu.removeAllItems()
		if pullDown {
			// A toolbar menu is a pull-down: its first item is the button's
			// title, never shown.
			menu.addItem(NSMenuItem(title: menu.title, action: nil, keyEquivalent: ""))
		}
		let drives = status.floppies.indices.filter { !status.floppies[$0].disabled }
		for drive in drives {
			header(menu, "DF\(drive):", status.floppies[drive].mediaPath)
			let insert = entry(menu, "Insert Disk\u{2026}", commands, #selector(MenuCommands.insertFloppy(_:)))
			insert.tag = drive
			let eject = entry(menu, "Eject", commands, #selector(MenuCommands.ejectFloppy(_:)))
			eject.tag = drive
			if status.floppies[drive].mediaPath.isEmpty {
				eject.action = nil
			}
			menu.addItem(.separator())
		}
		if status.cdActivity != nil || !status.cdPath.isEmpty {
			header(menu, "CD", status.cdPath)
			entry(menu, "Insert CD\u{2026}", commands, #selector(MenuCommands.insertCD(_:)))
			let eject = entry(menu, "Eject CD", commands, #selector(MenuCommands.ejectCD(_:)))
			if status.cdPath.isEmpty {
				eject.action = nil
			}
			menu.addItem(.separator())
		}
		let entries = swapper.enumerated().filter { !$0.element.isEmpty }
		let swap = NSMenuItem(title: "Disk Swapper", action: nil, keyEquivalent: "")
		let list = NSMenu(title: "Disk Swapper")
		for (index, path) in entries {
			let item = NSMenuItem(title: (path as NSString).lastPathComponent, action: nil, keyEquivalent: "")
			item.toolTip = path
			let into = NSMenu(title: item.title)
			for drive in drives {
				let target = NSMenuItem(title: "Insert into DF\(drive):",
					action: #selector(EmulationChrome.insertSwapperEntry(_:)), keyEquivalent: "")
				target.target = swapperTarget
				target.representedObject = [index, drive]
				into.addItem(target)
			}
			item.submenu = into
			list.addItem(item)
		}
		if entries.isEmpty {
			list.addItem(NSMenuItem(title: "No Entries", action: nil, keyEquivalent: ""))
		}
		swap.submenu = list
		menu.addItem(swap)
	}

	/// A drive's name and its disk, not a command.
	private static func header(_ menu: NSMenu, _ name: String, _ path: String) {
		let item = NSMenuItem.sectionHeader(title: "\(name) \(path.isEmpty ? "Empty" : (path as NSString).lastPathComponent)")
		item.toolTip = path.isEmpty ? nil : path
		menu.addItem(item)
	}

	@discardableResult
	private static func entry(_ menu: NSMenu, _ title: String, _ target: AnyObject, _ action: Selector) -> NSMenuItem {
		let item = NSMenuItem(title: title, action: action, keyEquivalent: "")
		item.target = target
		item.indentationLevel = 1
		menu.addItem(item)
		return item
	}
}
