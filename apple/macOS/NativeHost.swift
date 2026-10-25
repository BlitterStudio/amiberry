/*
 * Copyright (C) 2026 BlitterStudio
 * The macOS front end's alerts and file panels, and its menu bar entry
 * point (the shared entry points are apple/Shared/UI/HostEntryPoints.swift).
 */

import AppKit
import UniformTypeIdentifiers

@_cdecl("amiberry_native_ui_install_menus")
public func amiberryNativeUIInstallMenus() {
	MainActor.assumeIsolated {
		guard let mainMenu = NSApplication.shared.mainMenu else { return }
		MenuCommands.shared.install(into: mainMenu)
	}
}

@_cdecl("amiberry_native_ui_keyboard_toggle")
public func amiberryNativeUIKeyboardToggle() {
	MainActor.assumeIsolated {
		MacKeyboard.shared.toggle()
	}
}

@_cdecl("amiberry_native_ui_keyboard_hide")
public func amiberryNativeUIKeyboardHide() {
	MainActor.assumeIsolated {
		MacKeyboard.shared.hide()
	}
}

@_cdecl("amiberry_native_ui_keyboard_visible")
public func amiberryNativeUIKeyboardVisible() -> Bool {
	MainActor.assumeIsolated {
		MacKeyboard.shared.isVisible
	}
}

/// SDL has created the emulation window (its NSWindow): the toolbar goes on.
@_cdecl("amiberry_native_ui_emulation_window")
public func amiberryNativeUIEmulationWindow(_ window: UnsafeMutableRawPointer) {
	// The address crosses into the main actor; SDL calls on the main thread.
	let address = UInt(bitPattern: window)
	MainActor.assumeIsolated {
		guard let pointer = UnsafeMutableRawPointer(bitPattern: address) else { return }
		EmulationChrome.shared.attach(to: Unmanaged<NSWindow>.fromOpaque(pointer).takeUnretainedValue())
	}
}

@MainActor
enum Alerts {
	/// An alert attached to the frontmost window as a sheet, which does not
	/// block the caller (the emulation runs on); a modal alert when no window
	/// is up.
	static func showSheet(title: String, message: String) {
		let alert = NSAlert()
		alert.messageText = title
		alert.informativeText = message
		alert.addButton(withTitle: "OK")
		let window = NSApplication.shared.keyWindow ?? NSApplication.shared.mainWindow
			?? NSApplication.shared.windows.first { $0.isVisible && !($0 is NSPanel) }
		guard let window, window.attachedSheet == nil else {
			_ = show(title: title, message: message, buttons: ["OK"])
			return
		}
		alert.beginSheetModal(for: window)
	}

	/// Runs a modal alert; returns the index of the button chosen and whether
	/// the suppression checkbox was ticked.
	@discardableResult
	static func show(title: String, message: String, buttons: [String], suppression: String? = nil) -> (Int, Bool) {
		let alert = NSAlert()
		alert.messageText = title
		alert.informativeText = message
		for label in buttons {
			alert.addButton(withTitle: label)
		}
		if let suppression {
			alert.showsSuppressionButton = true
			alert.suppressionButton?.title = suppression
		}
		NSApplication.shared.activate(ignoringOtherApps: true)
		let response = alert.runModal()
		let index = response.rawValue - NSApplication.ModalResponse.alertFirstButtonReturn.rawValue
		return (max(0, min(index, buttons.count - 1)), alert.suppressionButton?.state == .on)
	}
}

@MainActor
enum FilePanels {
	/// Asks for a file to open or, for a save state, a file to save to;
	/// nil when cancelled. The pick is granted for later launches (a
	/// security-scoped bookmark on a Mac App Store build).
	static func choose(_ request: FileRequest, title: String, startingAt startPath: String) -> String? {
		let start = startDirectory(startPath)
		NSApplication.shared.activate(ignoringOtherApps: true)
		if request == .saveState {
			let panel = NSSavePanel()
			panel.message = title
			panel.directoryURL = start
			panel.nameFieldStringValue = "state.uss"
			if let uss = UTType(filenameExtension: "uss") {
				panel.allowedContentTypes = [uss]
			}
			panel.allowsOtherFileTypes = false
			guard panel.runModal() == .OK, let url = panel.url else { return nil }
			return url.path
		}
		let panel = NSOpenPanel()
		panel.message = title
		panel.directoryURL = start
		panel.canChooseFiles = true
		panel.canChooseDirectories = false
		panel.allowsMultipleSelection = false
		if request == .loadState, let uss = UTType(filenameExtension: "uss") {
			panel.allowedContentTypes = [uss]
		}
		guard panel.runModal() == .OK, let url = panel.url else { return nil }
		return granted(url.path)
	}

	/// The pick, when access to it could be kept for later launches (only
	/// a Mac App Store build can fail to keep it).
	private static func granted(_ path: String) -> String? {
		guard MediaLibrary.grantAccess(to: path) else {
			Alerts.show(
				title: "Cannot Keep Access",
				message: "Amiberry could not keep access to \u{201C}\(path)\u{201D} for later launches, so it was not used. Choose it again or pick another location.",
				buttons: ["OK"])
			return nil
		}
		return grantCompanions(of: path) ? path : nil
	}

	/// CD images whose sheet names files beside it: the tracks.
	static let companionFormats: Set<String> = ["cue", "ccd", "mds"]

	/// Whether the image reads files beside it: a CD's tracks, or the
	/// rom.key an encrypted (Amiga Forever) Kickstart is decrypted with.
	static func readsFilesBeside(_ url: URL) -> Bool {
		let format = url.pathExtension.lowercased()
		if companionFormats.contains(format) {
			return true
		}
		guard format == "rom", let file = try? FileHandle(forReadingFrom: url) else { return false }
		defer { try? file.close() }
		return (try? file.read(upToCount: 11)) == Data("AMIROMTYPE1".utf8)
	}

	/// For an image that reads files beside it, the folder holding them as
	/// well: picking a file grants a Mac App Store build that file only, so
	/// when the folder cannot be read, the user is asked for it (once: it
	/// is then kept). True when the files beside it can be read; elsewhere
	/// the folder always can, and nothing is asked.
	static func grantCompanions(of path: String) -> Bool {
		let url = URL(fileURLWithPath: path)
		guard readsFilesBeside(url) else { return true }
		let folder = url.deletingLastPathComponent()
		if (try? FileManager.default.contentsOfDirectory(atPath: folder.path)) != nil {
			return true
		}
		let panel = NSOpenPanel()
		panel.message = "\u{201C}\(url.lastPathComponent)\u{201D} uses the files beside it. Allow Amiberry to use the folder that holds them."
		panel.prompt = "Allow"
		panel.directoryURL = folder
		panel.canChooseFiles = false
		panel.canChooseDirectories = true
		panel.allowsMultipleSelection = false
		NSApplication.shared.activate(ignoringOtherApps: true)
		guard panel.runModal() == .OK, let chosen = panel.url else { return false }
		// The folder, or one holding it, however either is spelled (/tmp is
		// /private/tmp).
		let chosenPath = chosen.resolvingSymlinksInPath().standardizedFileURL.path
		let folderPath = folder.resolvingSymlinksInPath().standardizedFileURL.path
		guard folderPath == chosenPath || folderPath.hasPrefix(chosenPath.hasSuffix("/") ? chosenPath : chosenPath + "/") else {
			Alerts.show(
				title: "Not That Folder",
				message: "Choose the folder that holds \u{201C}\(url.lastPathComponent)\u{201D}, or one that contains it.",
				buttons: ["OK"])
			return false
		}
		guard MediaLibrary.grantAccess(to: chosen.path) else {
			Alerts.show(
				title: "Cannot Keep Access",
				message: "Amiberry could not keep access to \u{201C}\(chosen.path)\u{201D} for later launches, so the image was not used.",
				buttons: ["OK"])
			return false
		}
		return true
	}

	/// ROM files, or folders of them, to import; empty when cancelled.
	static func chooseROMs(title: String) -> [URL] {
		let panel = NSOpenPanel()
		panel.message = title
		panel.canChooseFiles = true
		panel.canChooseDirectories = true
		panel.allowsMultipleSelection = true
		NSApplication.shared.activate(ignoringOtherApps: true)
		return panel.runModal() == .OK ? panel.urls : []
	}

	/// Asks for a folder; nil when cancelled.
	static func chooseFolder(title: String, startingAt startPath: String) -> String? {
		let panel = NSOpenPanel()
		panel.message = title
		panel.directoryURL = startDirectory(startPath)
		panel.canChooseFiles = false
		panel.canChooseDirectories = true
		panel.canCreateDirectories = true
		panel.allowsMultipleSelection = false
		NSApplication.shared.activate(ignoringOtherApps: true)
		guard panel.runModal() == .OK, let url = panel.url else { return nil }
		return granted(url.path)
	}

	/// The folder to open a panel in: the path itself when it is a folder,
	/// else the folder holding it.
	static func startDirectory(_ path: String) -> URL? {
		guard !path.isEmpty else { return nil }
		var isDirectory: ObjCBool = false
		if FileManager.default.fileExists(atPath: path, isDirectory: &isDirectory), isDirectory.boolValue {
			return URL(fileURLWithPath: path, isDirectory: true)
		}
		return URL(fileURLWithPath: path).deletingLastPathComponent()
	}
}
