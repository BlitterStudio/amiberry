/*
 * Copyright (C) 2026 BlitterStudio
 * The iOS front end's alerts and file requesters, and its overlay entry
 * points (the shared entry points are apple/Shared/UI/HostEntryPoints.swift).
 *
 * The emulator asks for these synchronously, as it does on macOS, so each
 * runs the main run loop until the user answers. The emulator's own loop
 * pumps the run loop the same way (SDL_PumpEvents), so UIKit keeps working.
 */

import UIKit
import UniformTypeIdentifiers

@_cdecl("amiberry_native_ui_install_menus")
public func amiberryNativeUIInstallMenus() {
	MainActor.assumeIsolated {
		OpenedFiles.install()
		EmulationOverlay.shared.install()
	}
}

/// Files opened with the app (Open in, from Files or another app). iOS
/// hands over a security-scoped URL; SDL passes on only its path, which no
/// longer opens a file outside Amiberry's folder. So the file is copied into
/// the folder its type belongs in while the URL still opens it, and SDL gets
/// the copy. One iOS copied into Documents/Inbox, where the app cannot
/// write, moves the same way.
@MainActor
enum OpenedFiles {
	private static var installed = false

	/// Wraps SDL's scene delegate's handleURL:, which both the URLs a launch
	/// brings and those opened later go through. Before SDL's first event
	/// pump, so the launch's are caught too.
	static func install() {
		guard !installed else { return }
		installed = true
		let selector = NSSelectorFromString("handleURL:")
		guard let type = NSClassFromString("SDLUIKitSceneDelegate"),
			let method = class_getInstanceMethod(type, selector) else {
			// SDL renamed it: files from other apps then fail to open with a
			// notice rather than silently.
			NSLog("Amiberry: SDL's scene delegate has no handleURL:; files opened from other apps are not copied in")
			return
		}
		typealias Handler = @convention(c) (AnyObject, Selector, NSURL) -> Void
		let original = unsafeBitCast(method_getImplementation(method), to: Handler.self)
		let wrapper: @convention(block) (AnyObject, NSURL) -> Void = { delegate, url in
			let handed = MainActor.assumeIsolated { imported(url as URL) }
			switch handed {
			case .original: original(delegate, selector, url)
			case let .copy(copy): original(delegate, selector, copy as NSURL)
			case .refused: break
			}
		}
		method_setImplementation(method, imp_implementationWithBlock(wrapper))
	}

	enum Handed {
		/// Passed on as it is: not a file to play, or already in Amiberry's
		/// folder.
		case original
		/// The copy in Amiberry's folder.
		case copy(URL)
		/// Could not be copied in; the user has been told why.
		case refused
	}

	static func imported(_ url: URL) -> Handed {
		guard QuickPlay.accepts(url) else { return .original }
		let inbox = AppFiles.contains(url) && url.deletingLastPathComponent().lastPathComponent == "Inbox"
		if AppFiles.contains(url), !inbox {
			return .original
		}
		do {
			let copy = try AppFiles.importFile(url, into: AppFiles.folder(forMedia: url))
			if inbox {
				try? FileManager.default.removeItem(at: url)
			}
			return .copy(URL(fileURLWithPath: copy))
		} catch {
			Alerts.show(title: "Cannot Open", message: "\u{201C}\(url.lastPathComponent)\u{201D} could not be copied into Amiberry\u{2019}s folder: \(error.localizedDescription)", buttons: ["OK"])
			return .refused
		}
	}
}

@_cdecl("amiberry_native_ui_keyboard_toggle")
public func amiberryNativeUIKeyboardToggle() {
	MainActor.assumeIsolated {
		EmulationOverlay.shared.toggleKeyboard()
	}
}

@_cdecl("amiberry_native_ui_keyboard_hide")
public func amiberryNativeUIKeyboardHide() {
	MainActor.assumeIsolated {
		EmulationOverlay.shared.hideKeyboard()
	}
}

@_cdecl("amiberry_native_ui_background_task_begin")
public func amiberryNativeUIBackgroundTaskBegin() {
	MainActor.assumeIsolated {
		BackgroundTask.begin()
	}
}

@_cdecl("amiberry_native_ui_background_task_end")
public func amiberryNativeUIBackgroundTaskEnd() {
	MainActor.assumeIsolated {
		BackgroundTask.end()
	}
}

/// Keeps the app running in the background until the autosave is written
/// (native_host_request_autosave).
@MainActor
enum BackgroundTask {
	private static var task = UIBackgroundTaskIdentifier.invalid

	static func begin() {
		guard task == .invalid else { return }
		task = UIApplication.shared.beginBackgroundTask(withName: "Autosave") { end() }
	}

	static func end() {
		guard task != .invalid else { return }
		UIApplication.shared.endBackgroundTask(task)
		task = .invalid
	}
}

@_cdecl("amiberry_native_ui_set_modal_wait_hook")
public func amiberryNativeUISetModalWaitHook(_ hook: (@convention(c) () -> Void)?) {
	MainActor.assumeIsolated {
		ModalWait.hook = hook
	}
}

@MainActor
enum ModalWait {
	/// Run on every turn of a wait: the emulator's loops do not run
	/// meanwhile, and a requested autosave must still be written.
	static var hook: (@convention(c) () -> Void)?

	/// Presents `controller` and runs the main run loop until `done` holds
	/// or the controller is gone: never presented (UIKit refuses to present
	/// from a controller that is being dismissed, and only logs it) or
	/// dismissed some other way. False when it was never presented.
	@discardableResult
	static func present(_ controller: UIViewController, from presenter: UIViewController, animated: Bool,
		until done: () -> Bool) -> Bool {
		presenter.present(controller, animated: animated)
		guard controller.presentingViewController != nil else { return false }
		while !done(), controller.presentingViewController != nil {
			_ = RunLoop.current.run(mode: .default, before: Date(timeIntervalSinceNow: 0.05))
			hook?()
		}
		return true
	}
}

@MainActor
enum Alerts {
	/// Shows an alert and waits for an answer; returns the index of the
	/// button chosen and whether the suppression choice was taken. iOS
	/// alerts have no checkbox: a suppression label adds a last button that
	/// answers as the last of `buttons` (the one that changes nothing) and
	/// suppresses.
	@discardableResult
	static func show(title: String, message: String, buttons: [String], suppression: String? = nil) -> (Int, Bool) {
		let labels = buttons.isEmpty ? ["OK"] : buttons
		guard let presenter = EmulationOverlay.shared.presenter() else { return (0, false) }
		let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
		var answer: (Int, Bool)?
		for (index, label) in labels.enumerated() {
			let action = UIAlertAction(title: label, style: .default) { _ in answer = (index, false) }
			alert.addAction(action)
			if index == 0 {
				alert.preferredAction = action
			}
		}
		if let suppression {
			alert.addAction(UIAlertAction(title: suppression, style: .default) { _ in answer = (labels.count - 1, true) })
		}
		ModalWait.present(alert, from: presenter, animated: false) { answer != nil }
		// Gone without an answer: the default (first) button.
		return answer ?? (0, false)
	}

	/// An alert that does not wait (the emulation runs on).
	static func showSheet(title: String, message: String) {
		guard let presenter = EmulationOverlay.shared.presenter() else { return }
		let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
		alert.addAction(UIAlertAction(title: "OK", style: .default))
		presenter.present(alert, animated: false)
	}

	/// Asks for a name; nil when cancelled.
	static func askName(title: String, message: String, initial: String) -> String? {
		guard let presenter = EmulationOverlay.shared.presenter() else { return nil }
		let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
		alert.addTextField { field in
			field.text = initial
			field.autocapitalizationType = .none
			field.autocorrectionType = .no
			field.clearButtonMode = .whileEditing
		}
		var answer: String??
		alert.addAction(UIAlertAction(title: "Cancel", style: .cancel) { _ in answer = .some(nil) })
		let save = UIAlertAction(title: "Save", style: .default) { [weak alert] _ in
			answer = .some(alert?.textFields?.first?.text ?? "")
		}
		alert.addAction(save)
		alert.preferredAction = save
		ModalWait.present(alert, from: presenter, animated: false) { answer != nil }
		return answer ?? nil
	}
}

@MainActor
enum FilePanels {
	/// Asks for a file through the Files picker; nil when cancelled. A file
	/// outside Amiberry's own folder is copied into it (into the folder its
	/// kind belongs in): an iOS app keeps access only to its own files. A
	/// save state is named and saved in the save state folder.
	static func choose(_ request: FileRequest, title: String, startingAt startPath: String) -> String? {
		if request == .saveState {
			return chooseStateName(startingAt: startPath)
		}
		let types: [UTType]
		switch request {
		case .loadState: types = [UTType(filenameExtension: "uss") ?? .data]
		// What quick play takes, so nothing else is copied in.
		case .media: types = QuickPlay.extensions.sorted().compactMap { UTType(filenameExtension: $0) }
		default: types = [.item]
		}
		guard let url = pick(types: types, startingAt: startPath), !url.hasDirectoryPath else { return nil }
		if AppFiles.contains(url) {
			return url.path
		}
		do {
			let folder = request == .media ? try AppFiles.folder(forMedia: url) : try AppFiles.folder(for: request)
			return try AppFiles.importFile(url, into: folder)
		} catch {
			Alerts.show(
				title: "Cannot Import",
				message: "\u{201C}\(url.lastPathComponent)\u{201D} could not be copied into Amiberry's folder: \(error.localizedDescription)",
				buttons: ["OK"])
			return nil
		}
	}

	/// Asks for a folder; nil when cancelled. Only a folder inside
	/// Amiberry's own folder can be used, as access to others would not last.
	static func chooseFolder(title: String, startingAt startPath: String) -> String? {
		guard let url = pick(types: [.folder], startingAt: startPath) else { return nil }
		guard AppFiles.contains(url) else {
			Alerts.show(
				title: "Choose a Folder in Amiberry",
				message: "Amiberry can only keep using folders inside its own folder (On My iPhone or On My iPad \u{203A} Amiberry). Copy the folder there with the Files app, then choose it.",
				buttons: ["OK"])
			return nil
		}
		return url.path
	}

	private static func pick(types: [UTType], startingAt startPath: String) -> URL? {
		guard let presenter = EmulationOverlay.shared.presenter() else { return nil }
		let picker = UIDocumentPickerViewController(forOpeningContentTypes: types, asCopy: false)
		picker.allowsMultipleSelection = false
		picker.directoryURL = startDirectory(startPath)
		// Answered with Open or Cancel only: a swipe would dismiss it without
		// telling the delegate, and the wait would never end.
		picker.isModalInPresentation = true
		let delegate = PickerDelegate()
		picker.delegate = delegate
		ModalWait.present(picker, from: presenter, animated: true) { delegate.finished }
		return delegate.url
	}

	private static func chooseStateName(startingAt startPath: String) -> String? {
		guard let folder = startDirectory(startPath) ?? (try? MediaLibrary.path(of: .saveStates)).map({ URL(fileURLWithPath: $0) })
		else { return nil }
		var name = "State"
		while true {
			guard let typed = Alerts.askName(title: "Save State", message: "Name the state.", initial: name) else {
				return nil
			}
			name = typed.trimmingCharacters(in: .whitespaces)
			if AppFiles.isReservedStateName(name) {
				Alerts.show(
					title: "Save State",
					message: "\u{201C}\(name)\u{201D} is kept for the state saved when Amiberry goes to the background. Choose another name.",
					buttons: ["OK"])
				continue
			}
			guard let file = AppFiles.stateFile(named: name, in: folder) else {
				Alerts.show(title: "Save State", message: "Enter a name without \u{201C}/\u{201D}.", buttons: ["OK"])
				continue
			}
			if FileManager.default.fileExists(atPath: file.path) {
				let (choice, _) = Alerts.show(
					title: "Replace \u{201C}\(file.lastPathComponent)\u{201D}?",
					message: "A state with this name exists.", buttons: ["Replace", "Cancel"])
				guard choice == 0 else { continue }
			}
			return file.path
		}
	}

	/// The folder to open the picker in: the path itself when it is a
	/// folder, else the folder holding it.
	static func startDirectory(_ path: String) -> URL? {
		guard !path.isEmpty else { return nil }
		var isDirectory: ObjCBool = false
		if FileManager.default.fileExists(atPath: path, isDirectory: &isDirectory), isDirectory.boolValue {
			return URL(fileURLWithPath: path, isDirectory: true)
		}
		return URL(fileURLWithPath: path).deletingLastPathComponent()
	}
}

@MainActor
private final class PickerDelegate: NSObject, UIDocumentPickerDelegate {
	var finished = false
	var url: URL?

	func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
		url = urls.first
		finished = true
	}

	func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
		finished = true
	}
}
