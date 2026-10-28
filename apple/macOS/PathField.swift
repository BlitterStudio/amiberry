/*
 * Copyright (C) 2026 BlitterStudio
 * A file or folder a setting names, shown the way the Finder shows it: its
 * icon and name rather than its path. Choose… picks another; a file or
 * folder dropped on the row takes its place; the context menu shows it in
 * the Finder, copies its path or removes it.
 */

import AppKit
import SwiftUI
import UniformTypeIdentifiers

struct PathField: View {
	let label: String
	let path: String
	/// Opens the requester; the path picked (and granted), nil when cancelled.
	let choose: () -> String?
	/// Stores a path ("" removes it); false when it was refused.
	let set: (String) -> Bool
	/// A folder is wanted, not a file: drops of the other kind are refused.
	var folder = false
	/// Whether the setting can be left empty (a drive with no disk).
	var removable = true
	/// Whether a dropped item can stand for the setting as it is (the log
	/// is a file in a chosen folder, so it takes no drops).
	var acceptsDrops = true
	/// Whether a path that is not there is flagged: an input the machine
	/// needs is, a file the emulator writes (or may never need) is not.
	var warnsWhenMissing = true
	/// The setting's whole name for the Remove button's VoiceOver label,
	/// when the row shows a shorter one ("Image" under a "DF0:" section);
	/// nil for the label.
	var accessibilityName: String?
	@State private var targeted = false

	var body: some View {
		LabeledContent(label) {
			HStack(spacing: 6) {
				if path.isEmpty {
					Text("None")
						.foregroundStyle(.secondary)
				} else {
					Image(nsImage: PathField.icon(for: path))
						.resizable()
						.frame(width: 16, height: 16)
						.accessibilityHidden(true)
					Text(PathField.displayName(path))
						.lineLimit(1)
						.truncationMode(.middle)
						.help(path)
					if warnsWhenMissing, !FileManager.default.fileExists(atPath: path) {
						Image(systemName: "exclamationmark.triangle.fill")
							.foregroundStyle(.yellow)
							.help("\u{201C}\(path)\u{201D} is not there.")
							.accessibilityLabel("Missing")
					}
				}
				if removable, !path.isEmpty {
					Button {
						_ = set("")
					} label: {
						Image(systemName: "xmark.circle.fill")
					}
					.buttonStyle(.borderless)
					.foregroundStyle(.secondary)
					.help("Remove")
					.accessibilityLabel("Remove \(accessibilityName ?? label)")
				}
				Button("Choose\u{2026}") {
					if let picked = choose() {
						_ = set(picked)
					}
				}
			}
		}
		.padding(.vertical, 1)
		.background {
			if targeted {
				RoundedRectangle(cornerRadius: 6)
					.strokeBorder(Color.accentColor, lineWidth: 2)
					.padding(-4)
			}
		}
		.contextMenu {
			Button("Choose\u{2026}") {
				if let picked = choose() {
					_ = set(picked)
				}
			}
			if !path.isEmpty {
				Button("Show in Finder") { PathField.reveal(path) }
					.disabled(!FileManager.default.fileExists(atPath: path))
				Button("Copy Path") {
					NSPasteboard.general.clearContents()
					NSPasteboard.general.setString(path, forType: .string)
				}
				if removable {
					Divider()
					Button("Remove") { _ = set("") }
				}
			}
		}
		.dropDestination(for: URL.self) { urls, _ in
			guard acceptsDrops, let url = urls.first, PathField.isAcceptable(url, folder: folder),
				let granted = FilePanels.grantDropped(url)
			else { return false }
			return set(granted)
		} isTargeted: { targeted = acceptsDrops && $0 }
	}

	/// The name shown for a path: its last part ("/" for the root).
	static func displayName(_ path: String) -> String {
		let name = (path as NSString).lastPathComponent
		return name.isEmpty ? path : name
	}

	/// Whether a dropped item is of the kind the setting holds.
	static func isAcceptable(_ url: URL, folder: Bool) -> Bool {
		guard url.isFileURL else { return false }
		let isDirectory = (try? url.resourceValues(forKeys: [.isDirectoryKey]))?.isDirectory == true
		return isDirectory == folder
	}

	/// The Finder's icon for the item, or its type's when it is not there.
	static func icon(for path: String) -> NSImage {
		if FileManager.default.fileExists(atPath: path) {
			return NSWorkspace.shared.icon(forFile: path)
		}
		let type = UTType(filenameExtension: (path as NSString).pathExtension) ?? .data
		return NSWorkspace.shared.icon(for: type)
	}

	static func reveal(_ path: String) {
		NSWorkspace.shared.activateFileViewerSelecting([URL(fileURLWithPath: path)])
	}
}
