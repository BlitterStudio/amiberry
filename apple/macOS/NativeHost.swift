/*
 * Copyright (C) 2026 BlitterStudio
 * C entry points of the native macOS front end, called by
 * src/osdep/native_ui_host.cpp on the emulator's main thread where the
 * built-in GUI would show.
 */

import AppKit
import AmiberrySettings
import UniformTypeIdentifiers

/// What `amiberry_native_ui_choose_file` asks for (native_host_file in C++).
enum FileRequest: Int32 {
	case floppy = 0, cd, loadState, saveState
}

@_cdecl("amiberry_native_ui_settings_show")
public func amiberryNativeUISettingsShow(_ emulating: Bool) {
	MainActor.assumeIsolated {
		SettingsWindowController.shared.show(emulating: emulating)
	}
}

@_cdecl("amiberry_native_ui_settings_visible")
public func amiberryNativeUISettingsVisible() -> Bool {
	MainActor.assumeIsolated {
		SettingsWindowController.shared.isVisible
	}
}

@_cdecl("amiberry_native_ui_settings_dismiss")
public func amiberryNativeUISettingsDismiss() {
	MainActor.assumeIsolated {
		SettingsWindowController.shared.dismiss()
	}
}

@_cdecl("amiberry_native_ui_settings_commit")
public func amiberryNativeUISettingsCommit() {
	MainActor.assumeIsolated {
		SettingsWindowController.shared.model?.commitForExternalRequest()
	}
}

@_cdecl("amiberry_native_ui_settings_devices_changed")
public func amiberryNativeUISettingsDevicesChanged() {
	MainActor.assumeIsolated {
		SettingsWindowController.shared.model?.store.reload()
	}
}

@_cdecl("amiberry_native_ui_install_menus")
public func amiberryNativeUIInstallMenus() {
	MainActor.assumeIsolated {
		guard let mainMenu = NSApplication.shared.mainMenu else { return }
		MenuCommands.shared.install(into: mainMenu)
	}
}

@_cdecl("amiberry_native_ui_choose_file")
public func amiberryNativeUIChooseFile(
	_ kind: Int32, _ title: UnsafePointer<CChar>, _ startPath: UnsafePointer<CChar>,
	_ chosen: UnsafeMutablePointer<CChar>, _ size: Int
) -> Bool {
	// The C strings are read here; only Swift values reach the main actor.
	let titleText = String(cString: title)
	let start = String(cString: startPath)
	guard let request = FileRequest(rawValue: kind) else { return false }
	let path = MainActor.assumeIsolated {
		FilePanels.choose(request, title: titleText, startingAt: start)
	}
	guard let path else { return false }
	let bytes = Array(path.utf8CString)
	guard bytes.count <= size else { return false }
	for (index, byte) in bytes.enumerated() {
		chosen[index] = byte
	}
	return true
}

@_cdecl("amiberry_native_ui_message")
public func amiberryNativeUIMessage(_ title: UnsafePointer<CChar>, _ message: UnsafePointer<CChar>) {
	let titleText = String(cString: title)
	let messageText = String(cString: message)
	MainActor.assumeIsolated {
		_ = Alerts.show(title: titleText, message: messageText, buttons: ["OK"])
	}
}

/// Shows an alert with `buttons` (the first is the default) and an optional
/// suppression checkbox; returns the index of the button chosen.
@_cdecl("amiberry_native_ui_alert")
public func amiberryNativeUIAlert(
	_ title: UnsafePointer<CChar>, _ message: UnsafePointer<CChar>,
	_ buttons: UnsafePointer<UnsafePointer<CChar>?>?, _ buttonCount: Int32,
	_ suppression: UnsafePointer<CChar>?, _ suppressed: UnsafeMutablePointer<Bool>?
) -> Int32 {
	var labels: [String] = []
	if let buttons {
		for i in 0..<Int(max(buttonCount, 0)) {
			if let label = buttons[i] {
				labels.append(String(cString: label))
			}
		}
	}
	let titleText = String(cString: title)
	let messageText = String(cString: message)
	let suppressionLabel = suppression.map { String(cString: $0) }
	let buttonLabels = labels.isEmpty ? ["OK"] : labels
	let (choice, didSuppress) = MainActor.assumeIsolated {
		Alerts.show(title: titleText, message: messageText, buttons: buttonLabels, suppression: suppressionLabel)
	}
	suppressed?.pointee = didSuppress
	return Int32(choice)
}

@MainActor
enum Alerts {
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
		MediaLibrary.grantAccess(to: url.path)
		return url.path
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
		MediaLibrary.grantAccess(to: url.path)
		return url.path
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
