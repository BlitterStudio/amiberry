/*
 * Copyright (C) 2026 BlitterStudio
 * The designed editor pages: Display & Sound, Input and Storage arranged
 * by task rather than by layer group. Coverage stays complete: what a
 * page's groups hold and its layout does not place shows under More.
 */

import SwiftUI

/// A section of a designed page.
struct LayoutSection: Identifiable, Equatable {
	enum Content: Equatable {
		/// These options, in this order.
		case options([String])
		/// Numbered entries of which the filled ones and one empty one show
		/// (the disk swapper: twenty rows, mostly unused).
		case growing([String])
		/// The mounted hard drives and folders.
		case mounts
		/// The custom controls of each joystick port.
		case customControls
	}

	let title: String
	let content: Content

	var id: String { title }

	var ids: [String] {
		switch content {
		case let .options(ids), let .growing(ids): return ids
		case .mounts, .customControls: return []
		}
	}
}

extension SettingsPage {
	/// The designed page's sections; nil for the generated pages.
	var layout: [LayoutSection]? {
		switch self {
		case .displaySound: return SettingsPage.displaySoundLayout
		case .input: return SettingsPage.inputLayout
		case .storage: return SettingsPage.storageLayout
		case .overview, .hardware, .portsMisc, .advanced: return nil
		}
	}

	/// The options of the page's groups its layout does not place.
	var unplaced: [SettingDescriptor] {
		guard let layout else { return [] }
		let placed = Set(layout.flatMap(\.ids))
		return groups.flatMap(SettingsCatalog.descriptors(in:)).filter { !placed.contains($0.id) }
	}

	private static let displaySoundLayout: [LayoutSection] = [
		LayoutSection(title: "Window & Full Screen", content: .options([
			"gfx_display", "gfx_fullscreen_amiga", "gfx_fullscreen_picasso",
			"gfx_width_windowed", "gfx_height_windowed", "gfx_resize_windowed", "amiberry.borderless",
		])),
		LayoutSection(title: "Picture", content: .options([
			"gfx_resolution", "gfx_overscanmode", "gfx_autoresolution", "gfx_autoresolution_vga",
			"amiberry.gfx_correct_aspect", "amiberry.scaling_method", "gfx_linemode", "gfx_lores_mode",
			"gfx_center_horizontal", "gfx_center_vertical", "gfx_ntscpixelspal",
		])),
		LayoutSection(title: "Crop & Position", content: .options([
			"amiberry.gfx_auto_crop", "amiberry.gfx_manual_crop", "amiberry.gfx_manual_crop_width",
			"amiberry.gfx_manual_crop_height", "amiberry.gfx_horizontal_offset", "amiberry.gfx_vertical_offset",
		])),
		LayoutSection(title: "Shaders & Bezels", content: .options([
			"amiberry.shader", "amiberry.shader_rtg", "amiberry.use_bezel", "amiberry.use_custom_bezel",
			"amiberry.custom_bezel",
		])),
		LayoutSection(title: "Colour", content: .options([
			"gfx_luminance", "gfx_contrast", "gfx_gamma", "gfx_gamma_r", "gfx_gamma_g", "gfx_gamma_b",
			"gfx_blacker_than_black", "gfx_monochrome", "gfx_atari_palette_fix", "gfx_flickerfixer",
		])),
		LayoutSection(title: "Sync", content: .options([
			"gfx_vsync", "gfx_vsync_picasso", "gfx_vrr_monitor", "gfx_monitorblankdelay",
			"displaydata_pal", "displaydata_ntsc",
		])),
		LayoutSection(title: "Sound Output", content: .options([
			"amiberry.soundcard_default", "amiberry.soundcard", "sound_output", "sound_auto",
			"sound_frequency", "sound_max_buff",
		])),
		LayoutSection(title: "Volume", content: .options([
			"sound_volume", "sound_volume_paula", "sound_volume_cd", "sound_volume_ahi", "sound_volume_midi",
		])),
		LayoutSection(title: "Stereo & Filter", content: .options([
			"sound_channels", "sound_stereo_separation", "sound_stereo_mixing_delay",
			"sound_stereo_swap_paula", "sound_stereo_swap_ahi", "sound_interpol", "sound_filter", "sound_filter_type",
		])),
		LayoutSection(title: "Drive Sounds", content: .options((0..<4).flatMap {
			["floppy\($0)sound", "floppy\($0)soundvolume_disk", "floppy\($0)soundvolume_empty"]
		})),
	]

	private static let inputLayout: [LayoutSection] = [
		LayoutSection(title: "Port 1", content: .options([
			"joyport0", "joyport0mode", "joyport0autofire", "joyport0mousemap",
		])),
		LayoutSection(title: "Port 2", content: .options([
			"joyport1", "joyport1mode", "joyport1autofire", "joyport1mousemap",
		])),
		LayoutSection(title: "Parallel Port Adapter", content: .options([
			"joyport2", "joyport2autofire", "joyport3", "joyport3autofire",
		])),
		LayoutSection(title: "Mouse", content: .options([
			"input.mouse_speed", "input.autoswitch", "input.multi_mouse", "magic_mouse",
			"amiberry.middle_mouse", "magic_mousecursor", "tablet_library", "absolute_mouse",
		])),
		LayoutSection(title: "Joystick", content: .options([
			"input.joystick_deadzone", "input.autofire_speed", "input.joystick_up_button",
			"input.joymouse_speed_digital", "input.joymouse_speed_analog",
		])),
		LayoutSection(title: "Custom Controls", content: .customControls),
		LayoutSection(title: "On-screen Controls", content: .options([
			"amiberry.onscreen_joystick", "amiberry.vkbd_enabled", "amiberry.vkbd_language",
			"amiberry.vkbd_numpad", "amiberry.vkbd_transparency", "amiberry.vkbd_touch_button",
			"amiberry.vkbd_toggle", "amiberry.use_retroarch_vkbd",
		])),
	]

	private static let storageLayout: [LayoutSection] = [
		LayoutSection(title: "Floppy Drives", content: .options(["floppy_speed"])),
	] + (0..<4).map {
		LayoutSection(title: "DF\($0):", content: .options(["floppy\($0)", "floppy\($0)type", "floppy\($0)subtype"]))
	} + [
		LayoutSection(title: "Disk Swapper", content: .growing((0..<20).map { "diskimage\($0)" })),
		LayoutSection(title: "CD", content: .options(["cdimage0", "cd_speed", "amiberry.map_cd_drives"])),
		LayoutSection(title: "Hard Drives", content: .mounts),
		LayoutSection(title: "File System", content: .options(["filesys_custom_uaefsdb", "filesys_no_fsdb"])),
		LayoutSection(title: "DrawBridge", content: .options([
			"amiberry.drawbridge_driver", "amiberry.drawbridge_drive_cable", "amiberry.drawbridge_serial_autodetect",
			"amiberry.drawbridge_serial_port", "amiberry.drawbridge_smartspeed", "amiberry.drawbridge_autocache",
		])),
	]

	/// The entries of a growing section that show: the filled ones up to the
	/// last, and one empty one to fill next.
	static func visibleEntries(_ ids: [String], filled: (String) -> Bool) -> [String] {
		let last = ids.lastIndex(where: filled) ?? -1
		return Array(ids.prefix(min(last + 2, ids.count)))
	}
}

/// A designed page: its sections, then More for what they do not place.
struct DesignedSections: View {
	let page: SettingsPage
	let model: SettingsWindowModel

	var body: some View {
		ForEach(page.layout ?? []) { section in
			switch section.content {
			case let .options(ids):
				Section(section.title) { rows(ids) }
			case let .growing(ids):
				GrowingSection(title: section.title, ids: ids, model: model)
			case .mounts:
				MountSections(model: model)
			case .customControls:
				CustomControlsSections(store: model.store)
			}
		}
		let more = page.unplaced
		if !more.isEmpty {
			Section("More") {
				ForEach(more) { OptionRow(descriptor: $0, store: model.store) }
			}
		}
	}

	private func rows(_ ids: [String]) -> some View {
		OptionRows(ids: ids, store: model.store)
	}
}

private struct OptionRows: View {
	let ids: [String]
	let store: SettingsStore

	var body: some View {
		ForEach(ids.compactMap(SettingsCatalog.descriptor(for:))) { descriptor in
			OptionRow(descriptor: descriptor, store: store)
		}
	}
}

/// Numbered entries showing the filled ones and one empty one. It never
/// shrinks while the page shows: clearing an entry to retype it must not
/// take its row away mid-edit.
private struct GrowingSection: View {
	let title: String
	let ids: [String]
	let model: SettingsWindowModel
	@State private var grown = 0

	var body: some View {
		// Refused text counts as filled: its row must stay where it can be
		// corrected.
		let visible = SettingsPage.visibleEntries(ids) {
			!(model.store.value(of: $0) ?? "").isEmpty || model.invalidDrafts[$0] != nil
		}.count
		Section(title) {
			OptionRows(ids: Array(ids.prefix(max(visible, grown))), store: model.store)
		}
		.onChange(of: visible, initial: true) { grown = max(grown, visible) }
	}
}
