/*
 * Copyright (C) 2026 BlitterStudio
 * Folders, media lists and sandbox access (src/osdep/settings/amiberry_media.h).
 * Every call runs on the emulator's main thread.
 */

import AmiberrySettings
import Foundation

/// A folder the emulator uses.
public enum MediaFolder: CaseIterable, Hashable, Sendable {
	case roms, floppies, cds, hardDrives, configurations, saveStates, screenshots, whdloadArchives

	var layerValue: amiberry_folder {
		switch self {
		case .roms: return AMIBERRY_FOLDER_ROMS
		case .floppies: return AMIBERRY_FOLDER_FLOPPIES
		case .cds: return AMIBERRY_FOLDER_CDS
		case .hardDrives: return AMIBERRY_FOLDER_HARD_DRIVES
		case .configurations: return AMIBERRY_FOLDER_CONFIGURATIONS
		case .saveStates: return AMIBERRY_FOLDER_SAVESTATES
		case .screenshots: return AMIBERRY_FOLDER_SCREENSHOTS
		case .whdloadArchives: return AMIBERRY_FOLDER_WHDLOAD_ARCHIVES
		}
	}
}

/// A list of media a front end offers for picking.
public enum MediaList: CaseIterable, Hashable, Sendable {
	/// Most recent first.
	case recentFloppies, recentCDs, recentWHDLoad
	/// The host's CD drives.
	case cdDrives
	/// .uae files in the configurations folder, by name.
	case configurations
	/// .uss files in the save states folder in effect, by name.
	case saveStates

	var layerValue: amiberry_media_list {
		switch self {
		case .recentFloppies: return AMIBERRY_MEDIA_RECENT_FLOPPIES
		case .recentCDs: return AMIBERRY_MEDIA_RECENT_CDS
		case .recentWHDLoad: return AMIBERRY_MEDIA_RECENT_WHDLOAD
		case .cdDrives: return AMIBERRY_MEDIA_CD_DRIVES
		case .configurations: return AMIBERRY_MEDIA_CONFIGURATIONS
		case .saveStates: return AMIBERRY_MEDIA_SAVESTATES
		}
	}

	/// The lists `MediaLibrary.addRecent` records into.
	public var isRecentList: Bool {
		self == .recentFloppies || self == .recentCDs || self == .recentWHDLoad
	}
}

/// One entry of a media list: its full path and a name to show.
public struct MediaEntry: Hashable, Sendable, Identifiable {
	public let path: String
	public let name: String
	public var id: String { path }
}

@MainActor
public enum MediaLibrary {
	/// The version of amiberry_media.h this module was built against.
	public static let apiVersion = Int(AMIBERRY_MEDIA_API_VERSION)

	public static func path(of folder: MediaFolder) throws -> String {
		let (text, status) = readLayerString(capacity: 1024) { amiberry_folder_get(folder.layerValue, $0, $1) }
		try SettingsError.check(status, id: "\(folder)")
		return text
	}

	/// Points the folder at `path` and saves the emulator's settings file,
	/// as the Paths panel does (granting sandbox access first on a Mac App
	/// Store build).
	public static func setPath(_ path: String, for folder: MediaFolder) throws {
		try SettingsError.check(amiberry_folder_set(folder.layerValue, path), id: "\(folder)", value: path)
	}

	/// A fresh snapshot of the list.
	public static func entries(_ list: MediaList) -> [MediaEntry] {
		let count = amiberry_media_refresh(list.layerValue)
		return (0..<count).compactMap { entry(list, at: $0) }
	}

	private static func entry(_ list: MediaList, at index: Int) -> MediaEntry? {
		var size = 1024
		while size <= layerStringLimit {
			var path = [CChar](repeating: 0, count: size)
			var name = [CChar](repeating: 0, count: size)
			let status = path.withUnsafeMutableBufferPointer { p in
				name.withUnsafeMutableBufferPointer { n in
					amiberry_media_entry(list.layerValue, index, p.baseAddress, p.count, n.baseAddress, n.count)
				}
			}
			if status == AMIBERRY_SETTINGS_OK {
				return MediaEntry(path: decodedString(path), name: decodedString(name))
			}
			if status != AMIBERRY_SETTINGS_BUFFER_TOO_SMALL {
				return nil
			}
			size *= 2
		}
		return nil
	}

	/// Records a picked file as the most recent entry of a recent list and
	/// saves the emulator's settings.
	public static func addRecent(_ path: String, to list: MediaList) throws {
		try SettingsError.check(amiberry_media_add_recent(list.layerValue, path), id: "\(list)", value: path)
	}

	/// Rescans the ROMs folder after ROM files were added (setting the
	/// folder rescans it too); the number of ROMs the scan recognised. Every
	/// key file in the folder decodes first: another Amiga Forever set's key
	/// is kept beside rom.key under another name ("rom 2.key").
	@MainActor
	public static func rescanROMs() -> Int {
		if let folder = try? path(of: .roms),
			let names = try? FileManager.default.contentsOfDirectory(atPath: folder) {
			for name in names where name.lowercased().hasSuffix(".key") {
				amiberry_media_add_rom_key((folder as NSString).appendingPathComponent(name))
			}
		}
		return Int(amiberry_media_rescan_roms())
	}

	/// Keeps access to a file or folder the user picked across restarts (a
	/// security-scoped bookmark on a Mac App Store build); false when that
	/// failed. Elsewhere there is nothing to keep.
	@discardableResult
	public static func grantAccess(to path: String) -> Bool {
		amiberry_media_grant_access(path)
	}

	/// The configuration file the pending configuration came from or was
	/// last saved to; nil for a default configuration or an RP9 package.
	public static func currentConfiguration() -> String? {
		let (text, status) = readLayerString(capacity: 1024) { amiberry_media_current_configuration($0, $1) }
		guard status == AMIBERRY_SETTINGS_OK, !text.isEmpty else { return nil }
		return text
	}

	/// Forgets the configuration file: the pending configuration becomes a
	/// new machine.
	public static func forgetCurrentConfiguration() {
		amiberry_media_forget_current_configuration()
	}

	/// The description and category a configuration file declares, read
	/// from its header; nil when the file cannot be read.
	public static func configurationInfo(_ path: String) -> (description: String, category: String)? {
		var description = [CChar](repeating: 0, count: 512)
		var category = [CChar](repeating: 0, count: 512)
		let status = description.withUnsafeMutableBufferPointer { d in
			category.withUnsafeMutableBufferPointer { c in
				amiberry_media_configuration_info(path, d.baseAddress, d.count, c.baseAddress, c.count)
			}
		}
		guard status == AMIBERRY_SETTINGS_OK else { return nil }
		return (decodedString(description), decodedString(category))
	}
}
