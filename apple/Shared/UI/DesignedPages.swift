/*
 * Copyright (C) 2026 BlitterStudio
 * The designed editor pages: Display & Sound, Input, Storage and Ports &
 * Misc arranged by task rather than by layer group. Coverage stays
 * complete: what a page's groups hold and its layout does not place shows
 * under More.
 */

import SwiftUI

/// Options a page shows only while they are available: those that mean
/// nothing until another option enables them (a disabled drive's image, an
/// accelerator's jumpers before a board is picked). All Settings and search
/// still list them.
struct HiddenWhenUnavailable: Equatable {
	var ids: Set<String> = []
	/// Options whose IDs start with one of these: "cpuboard_settings"
	/// covers the jumpers' "cpuboard_settings:<name>" options too.
	var prefixes: [String] = []
	/// Options that show only once one of the options before them is in
	/// use (a second Z2 board after the first): ID → the options of which
	/// one must be set. One that is set itself always shows.
	var after: [String: [String]] = [:]

	func contains(_ id: String) -> Bool {
		ids.contains(id) || prefixes.contains { id.hasPrefix($0) }
	}

	/// The descriptors that show: all but the hidden ones unavailable now.
	func shown(_ descriptors: [SettingDescriptor], available: (String) -> Bool) -> [SettingDescriptor] {
		descriptors.filter { !contains($0.id) || available($0.id) }
	}

	/// The descriptors that show, also leaving out those that follow
	/// options not in use yet.
	func shown(_ descriptors: [SettingDescriptor], available: (String) -> Bool,
		value: (String) -> String?) -> [SettingDescriptor] {
		shown(descriptors, available: available).filter { descriptor in
			guard let before = after[descriptor.id] else { return true }
			return HiddenWhenUnavailable.inUse(value(descriptor.id)) || before.contains { HiddenWhenUnavailable.inUse(value($0)) }
		}
	}

	/// Whether a value sets something: not empty, 0 or none.
	static func inUse(_ value: String?) -> Bool {
		guard let value = value?.trimmingCharacters(in: .whitespaces) else { return false }
		return !["", "0", "none"].contains(value.lowercased())
	}
}

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
	/// The section's options that show only while available.
	let hiddenWhenUnavailable: HiddenWhenUnavailable

	init(title: String, content: Content, hiddenWhenUnavailable: Set<String> = []) {
		self.title = title
		self.content = content
		self.hiddenWhenUnavailable = HiddenWhenUnavailable(ids: hiddenWhenUnavailable)
	}

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
		case .portsMisc: return SettingsPage.portsMiscLayout
		case .overview, .hardware, .advanced: return nil
		}
	}

	/// The options a generated page shows only while they are available
	/// (a designed page's sections say it for theirs): the Overview's
	/// second drive, and the accelerator board's jumpers, memory and boot
	/// ROM, which mean nothing until a board is picked.
	var hiddenWhenUnavailable: HiddenWhenUnavailable {
		switch self {
		case .overview: return HiddenWhenUnavailable(ids: ["floppy1"])
		case .hardware:
			// The second to fourth memory boards of a kind appear once the
			// one before is fitted: most machines have one or none, so the
			// page shows only the rows a machine can use.
			var after: [String: [String]] = [:]
			for board in 2...4 {
				let previous = board == 2 ? "" : "\(board - 1)"
				let z2 = ["fastmem\(previous)_size", "fastmem\(previous)_size_k"]
				after["fastmem\(board)_size"] = z2
				after["fastmem\(board)_size_k"] = z2
				after["z3mem\(board)_size"] = ["z3mem\(previous)_size"]
			}
			return HiddenWhenUnavailable(ids: ["cpuboardmem1_size", "cpuboard_rom_file"], prefixes: ["cpuboard_settings"],
				after: after)
		case .displaySound, .input, .storage, .portsMisc, .advanced: return HiddenWhenUnavailable()
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
		}), hiddenWhenUnavailable: Set((0..<4).flatMap {
			// A drive's volumes while its sound is on.
			["floppy\($0)soundvolume_disk", "floppy\($0)soundvolume_empty"]
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
		// The drive type first: it is what enables the mode and image below
		// it, which a disabled drive does not show.
		LayoutSection(title: "DF\($0):", content: .options(["floppy\($0)type", "floppy\($0)subtype", "floppy\($0)"]),
			hiddenWhenUnavailable: ["floppy\($0)subtype", "floppy\($0)"])
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

	private static let portsMiscLayout: [LayoutSection] = [
		LayoutSection(title: "Printer & Sampler", content: .options([
			"amiberry.parallel_port", "amiberry.parallel_port:folder", "amiberry.samplersoundcard", "sampler_stereo",
		])),
		LayoutSection(title: "Serial & MIDI", content: .options([
			"serial_port", "serial_on_demand", "serial_hardware_ctsrts", "serial_status", "serial_ri", "serial_direct",
			"serial_translate", "uaeserial", "amiberry.midiout_device_name", "amiberry.midiin_device_name",
			"amiberry.midirouter",
		])),
		LayoutSection(title: "Copy Protection", content: .options(["dongle"])),
		LayoutSection(title: "Startup & Window", content: .options([
			"use_gui", "amiberry.start_minimized", "warpboot", "cpu_reset_pause", "amiberry.always_on_top",
			"amiberry.gui_always_on_top", "amiberry.inactive_minimize", "amiberry.blank_monitors",
		])),
		LayoutSection(title: "Mouse & Keyboard", content: .options([
			"amiberry.start_not_captured", "amiberry.active_capture_automatically", "amiberry.ctrl_alt_release",
			"amiberry.alt_tab_release", "amiberry.right_control_is_right_win",
		])),
		LayoutSection(title: "Keyboard Shortcuts", content: .options([
			"amiberry.open_gui", "amiberry.quit_amiberry", "amiberry.action_replay", "amiberry.fullscreen_toggle",
			"amiberry.minimize", "amiberry.screenshot", "amiberry.debugger_trigger", "amiberry.left_amiga",
			"amiberry.right_amiga",
		])),
		LayoutSection(title: "Controller Shortcuts", content: .options([
			"amiberry.use_retroarch_menu", "amiberry.use_retroarch_quit", "amiberry.use_retroarch_reset",
		])),
		LayoutSection(title: "Keyboard Lights", content: .options([
			"amiberry.kbd_led_num", "amiberry.kbd_led_scr", "amiberry.kbd_led_cap",
		])),
		LayoutSection(title: "Status Line", content: .options(["show_leds", "show_leds_rtg", "power_led_dim"])),
		LayoutSection(title: "Host Integration", content: .options(["clipboard_sharing", "synchronize_clock"])),
		LayoutSection(title: "Safety & Debugging", content: .options([
			"floppy_write_protect", "harddrive_write_protect", "cpu_halt_auto_reset", "native_code", "log_illegal_mem",
			"debug_mem",
		])),
		LayoutSection(title: "Compatibility", content: .options(["uae_hide_autoconfig", "scsidevice_disable"])),
		// The layer's own order: active, inactive, minimized.
		LayoutSection(title: "Priority", content: .options(SettingsCatalog.descriptors(in: "priority").map(\.id))),
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
				OptionSection(title: section.title, descriptors: ids.compactMap(SettingsCatalog.descriptor(for:)),
					store: model.store, hiddenWhenUnavailable: section.hiddenWhenUnavailable)
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
			OptionSection(title: "More", descriptors: more, store: model.store)
		}
	}
}

/// Options in a section titled `title`, named in its words.
private struct OptionRows: View {
	let title: String
	let ids: [String]
	let store: SettingsStore

	var body: some View {
		ForEach(ids.compactMap(SettingsCatalog.descriptor(for:))) { descriptor in
			OptionRow(descriptor: descriptor.labelled(OptionSection.label(descriptor.label, in: title)), store: store)
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
			OptionRows(title: title, ids: Array(ids.prefix(max(visible, grown))), store: model.store)
		}
		.onChange(of: visible, initial: true) { grown = max(grown, visible) }
	}
}
