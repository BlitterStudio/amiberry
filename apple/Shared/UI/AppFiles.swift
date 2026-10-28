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
	/// A file to play: a disk, CD, WHDLoad archive or configuration
	/// (quick play); iOS keeps it in the folder its type belongs in.
	case media

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

	/// Whether an option holding a path names a folder: its ID says so
	/// (amiberry.parallel_port:folder).
	static func namesFolder(_ id: String) -> Bool {
		id.hasSuffix(":folder")
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
		case .file, .media:
			// Next to the media folders (Documents/Amiberry/Imports).
			let floppies = URL(fileURLWithPath: try MediaLibrary.path(of: .floppies), isDirectory: true)
			return floppies.deletingLastPathComponent().appendingPathComponent("Imports", isDirectory: true)
		}
		return URL(fileURLWithPath: try MediaLibrary.path(of: folder), isDirectory: true)
	}

	/// The folder a file to play belongs in, by its type: its Media shelf's
	/// folder, the configurations for a machine.
	static func folder(forMedia url: URL) throws -> URL {
		let folder: MediaFolder
		if let shelf = MediaShelf.shelf(for: url)?.folder {
			folder = shelf
		} else if ["uae", "rp9"].contains(url.pathExtension.lowercased()) {
			folder = .configurations
		} else {
			return try self.folder(for: .media)
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
		if url.pathExtension.lowercased() == "cue" {
			return try importCueSheet(url, into: folder)
		}
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

	/// A cue sheet is nothing without its tracks: it goes into a folder of
	/// its own (track files are often all called "Track 01.bin") with every
	/// file it names, which keep their places beside it. A sheet whose tracks
	/// cannot be read from where it is (another app's folder grants the sheet
	/// alone) is not imported. A folder of the same name holding the same
	/// sheet and the same tracks is used as it is; two CDs whose sheets read
	/// alike (disc.cue naming Track 01.bin) are told apart by their tracks.
	private static func importCueSheet(_ url: URL, into folder: URL) throws -> String {
		let manager = FileManager.default
		let data = try Data(contentsOf: url)
		let tracks = cueTracks(String(data: data, encoding: .utf8) ?? String(decoding: data, as: UTF8.self))
		let source = url.deletingLastPathComponent()
		let stem = url.deletingPathExtension().lastPathComponent
		var number = 1
		while true {
			let candidate = folder.appendingPathComponent(number == 1 ? stem : "\(stem) \(number)", isDirectory: true)
			let sheet = candidate.appendingPathComponent(url.lastPathComponent)
			if !manager.fileExists(atPath: candidate.path) {
				try manager.createDirectory(at: candidate, withIntermediateDirectories: true)
				do {
					try manager.copyItem(at: url, to: sheet)
					var unreadable: [String] = []
					for track in tracks {
						// Beside the sheet or under it only.
						guard !track.hasPrefix("/"), !track.split(separator: "/").contains("..") else {
							unreadable.append(track)
							continue
						}
						let target = candidate.appendingPathComponent(track)
						do {
							try manager.createDirectory(at: target.deletingLastPathComponent(), withIntermediateDirectories: true)
							try manager.copyItem(at: source.appendingPathComponent(track), to: target)
						} catch {
							unreadable.append(track)
						}
					}
					guard unreadable.isEmpty else {
						throw CueTracksUnreadable(sheet: url.lastPathComponent, tracks: unreadable)
					}
				} catch {
					try? manager.removeItem(at: candidate)
					throw error
				}
				return sheet.path
			}
			if manager.contentsEqual(atPath: sheet.path, andPath: url.path), tracks.allSatisfy({
				manager.contentsEqual(atPath: candidate.appendingPathComponent($0).path,
					andPath: source.appendingPathComponent($0).path)
			}) {
				return sheet.path
			}
			number += 1
		}
	}

	/// The files a cue sheet's FILE lines name, as written (relative to the
	/// sheet), each once. Read as the emulator's parsecue() reads them: the
	/// words split at any white space, a quoted name runs to its closing
	/// quote (or the end of the line).
	static func cueTracks(_ sheet: String) -> [String] {
		var tracks: [String] = []
		for line in sheet.split(whereSeparator: \.isNewline) {
			var rest = line.drop(while: \.isWhitespace)
			let directive = rest.prefix(while: { !$0.isWhitespace })
			guard String(directive).caseInsensitiveCompare("FILE") == .orderedSame else { continue }
			rest = rest.dropFirst(directive.count).drop(while: \.isWhitespace)
			let name = rest.first == "\""
				? rest.dropFirst().prefix(while: { $0 != "\"" })
				: rest.prefix(while: { !$0.isWhitespace })
			if !name.isEmpty, !tracks.contains(String(name)) {
				tracks.append(String(name))
			}
		}
		return tracks
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

/// A cue sheet whose track files cannot be read from where it is.
struct CueTracksUnreadable: LocalizedError {
	let sheet: String
	let tracks: [String]

	var errorDescription: String? {
		"\u{201C}\(sheet)\u{201D} needs \(tracks.joined(separator: ", ")), which Amiberry cannot read from there. Copy the CD\u{2019}s whole folder into Amiberry\u{2019}s CDs folder with the Files app, or use the CD as one .chd or .iso file."
	}
}
