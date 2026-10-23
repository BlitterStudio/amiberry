/*
 * Copyright (C) 2026 BlitterStudio
 * Where picked files live. An iOS app keeps access only to files in its own
 * container, so the iOS requesters use a file there as it is and copy one
 * from elsewhere (another app, iCloud Drive) into the folder its kind
 * belongs in. Shared so the macOS tests cover it.
 */

import AmiberrySettings
import Foundation

/// What a requester asks for. The first four are native_host_file in
/// src/osdep/native_ui_host.h (amiberry_native_ui_choose_file); the rest
/// are the settings UI's own.
enum FileRequest: Int32 {
	case floppy = 0, cd, loadState, saveState
	/// A hard disk image to mount.
	case hardfile
	/// A Kickstart, extended or cartridge ROM.
	case rom
	/// Any other file an option names.
	case file

	/// The request for an option holding a path.
	static func forOption(_ id: String) -> FileRequest {
		if id.hasPrefix("floppy") || id.hasPrefix("diskimage") {
			return .floppy
		}
		if id.hasPrefix("kickstart") || id == "cart_file" {
			return .rom
		}
		return .file
	}
}

@MainActor
enum AppFiles {
	/// Whether the file is inside `root` (the app's container by default).
	static func contains(_ url: URL, root: String = NSHomeDirectory()) -> Bool {
		let base = canonicalPath(root)
		let path = canonicalPath(url.path)
		return path == base || path.hasPrefix(base.hasSuffix("/") ? base : base + "/")
	}

	/// The path with every symlink resolved (/var is /private/var), the
	/// way realpath() does it: through the nearest part that exists, so a
	/// file not there yet compares like one that is. Foundation's
	/// resolvingSymlinksInPath drops /private only for existing paths.
	static func canonicalPath(_ path: String) -> String {
		var existing = URL(fileURLWithPath: path).standardizedFileURL
		var rest: [String] = []
		while true {
			if let resolved = realpath(existing.path, nil) {
				defer { free(resolved) }
				var result = URL(fileURLWithPath: String(cString: resolved))
				for component in rest.reversed() {
					result.appendPathComponent(component)
				}
				return result.path
			}
			guard existing.path != "/" else { return path }
			rest.append(existing.lastPathComponent)
			existing.deleteLastPathComponent()
		}
	}

	/// The folder a picked file of this kind is copied into.
	static func folder(for request: FileRequest) throws -> URL {
		let folder: MediaFolder
		switch request {
		case .floppy: folder = .floppies
		case .cd: folder = .cds
		case .loadState, .saveState: folder = .saveStates
		case .hardfile: folder = .hardDrives
		case .rom: folder = .roms
		case .file:
			// Next to the media folders (Documents/Amiberry/Imports).
			let floppies = URL(fileURLWithPath: try MediaLibrary.path(of: .floppies), isDirectory: true)
			return floppies.deletingLastPathComponent().appendingPathComponent("Imports", isDirectory: true)
		}
		return URL(fileURLWithPath: try MediaLibrary.path(of: folder), isDirectory: true)
	}

	/// Copies a file into `folder` and returns the copy's path. A file of
	/// the same name and contents there already is used as it is (picking
	/// the same disk again does not copy it again); another of the same name
	/// gets a numbered name ("Disk 2.adf").
	static func importFile(_ url: URL, into folder: URL) throws -> String {
		let scoped = url.startAccessingSecurityScopedResource()
		defer {
			if scoped {
				url.stopAccessingSecurityScopedResource()
			}
		}
		let manager = FileManager.default
		try manager.createDirectory(at: folder, withIntermediateDirectories: true)
		let name = url.deletingPathExtension().lastPathComponent
		let suffix = url.pathExtension.isEmpty ? "" : "." + url.pathExtension
		var number = 1
		while true {
			let candidate = folder.appendingPathComponent(number == 1 ? name + suffix : "\(name) \(number)\(suffix)")
			if !manager.fileExists(atPath: candidate.path) {
				try manager.copyItem(at: url, to: candidate)
				return candidate.path
			}
			if manager.contentsEqual(atPath: candidate.path, andPath: url.path) {
				return candidate.path
			}
			number += 1
		}
	}

	/// Whether a typed state name is the autosave's
	/// (AMIBERRY_AUTOSAVE_STATE_NAME), which the app replaces on its own.
	static func isReservedStateName(_ name: String) -> Bool {
		let trimmed = name.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()
		let reserved = AMIBERRY_AUTOSAVE_STATE_NAME.lowercased()
		return trimmed == reserved || trimmed + ".uss" == reserved
	}

	/// The state file a typed name saves to, in `folder`, ending in ".uss"
	/// as the emulator spells it (it adds ".uss" to any other ending, so
	/// "Game.USS" would be saved as "Game.USS.uss"). nil for a name that is
	/// empty or holds a path separator.
	static func stateFile(named name: String, in folder: URL) -> URL? {
		var base = name.trimmingCharacters(in: .whitespacesAndNewlines)
		if base.lowercased().hasSuffix(".uss") {
			base.removeLast(4)
		}
		guard !base.isEmpty, !base.contains("/"), base != ".", base != ".." else { return nil }
		return folder.appendingPathComponent(base + ".uss")
	}
}
