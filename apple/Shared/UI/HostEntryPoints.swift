/*
 * Copyright (C) 2026 BlitterStudio
 * C entry points of the native front ends, called by
 * src/osdep/native_ui_host.cpp on the emulator's main thread where the
 * built-in GUI would show. Each platform provides SettingsPresenter, Alerts
 * and FilePanels (apple/macOS, apple/iOS).
 */

import Foundation

@_cdecl("amiberry_native_ui_settings_show")
public func amiberryNativeUISettingsShow(_ emulating: Bool) {
	MainActor.assumeIsolated {
		SettingsPresenter.shared.show(emulating: emulating)
	}
}

@_cdecl("amiberry_native_ui_settings_visible")
public func amiberryNativeUISettingsVisible() -> Bool {
	MainActor.assumeIsolated {
		SettingsPresenter.shared.isVisible
	}
}

@_cdecl("amiberry_native_ui_settings_dismiss")
public func amiberryNativeUISettingsDismiss() {
	MainActor.assumeIsolated {
		SettingsPresenter.shared.dismiss()
	}
}

@_cdecl("amiberry_native_ui_settings_commit")
public func amiberryNativeUISettingsCommit() {
	MainActor.assumeIsolated {
		SettingsPresenter.shared.model?.commitForExternalRequest()
	}
}

@_cdecl("amiberry_native_ui_settings_devices_changed")
public func amiberryNativeUISettingsDevicesChanged() {
	MainActor.assumeIsolated {
		SettingsPresenter.shared.model?.store.reload()
	}
}

/// A file opened with the app (the Dock icon, Finder's Open With, iOS's
/// Open in) while the settings UI shows: the Library plays it.
@_cdecl("amiberry_native_ui_open_file")
public func amiberryNativeUIOpenFile(_ path: UnsafePointer<CChar>) {
	let file = String(cString: path)
	MainActor.assumeIsolated {
		let url = URL(fileURLWithPath: file)
		if let library = SettingsPresenter.shared.library {
			library.quickPlay(url)
		} else {
			// The settings over a running emulation: once they close, the
			// running machine takes the file as a drop (a disk goes in DF0:).
			Alerts.show(title: "Close the Settings First",
				message: "Close the settings, then open \u{201C}\(url.lastPathComponent)\u{201D} again.",
				buttons: ["OK"])
		}
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
