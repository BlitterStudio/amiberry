/*
 * Copyright (C) 2026 BlitterStudio
 * The designed editor pages: Hardware, Display & Sound, Input, Storage and
 * Ports & Misc arranged by task rather than by layer group. Coverage stays
 * complete: what a page's groups hold and its layout does not place shows
 * under More, and the expert rows sit in the page's collapsed Advanced
 * sections rather than off the page.
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
		/// The expansion boards (units, not options).
		case boards
	}

	let title: String
	let content: Content
	/// Whether the section starts collapsed behind a disclosure: the page's
	/// Advanced sections, which hold what a task does not need every day
	/// but must stay on the page. The model remembers the disclosure.
	let collapsed: Bool
	/// The section's options that show only while available.
	let hiddenWhenUnavailable: HiddenWhenUnavailable

	init(title: String, content: Content, hiddenWhenUnavailable: Set<String> = [], prefixes: [String] = [],
		after: [String: [String]] = [:], collapsed: Bool = false) {
		self.title = title
		self.content = content
		self.collapsed = collapsed
		self.hiddenWhenUnavailable = HiddenWhenUnavailable(ids: hiddenWhenUnavailable, prefixes: prefixes, after: after)
	}

	var id: String { title }

	/// The descriptors the section renders, in order: each id's, with the
	/// family a prefix covers (the accelerator's jumpers) after its base —
	/// the id the prefix names — in the catalogue's order. Rendering and
	/// placement (`places`) both read this, so an option can never count
	/// as placed without being rendered.
	var descriptors: [SettingDescriptor] {
		var rendered: [SettingDescriptor] = []
		for id in ids {
			guard let descriptor = SettingsCatalog.descriptor(for: id) else { continue }
			rendered.append(descriptor)
			if hiddenWhenUnavailable.prefixes.contains(id) {
				rendered += SettingsCatalog.descriptors.filter { $0.id != id && $0.id.hasPrefix(id) }
			}
		}
		// A prefix whose base the section does not name: its family still
		// renders, at the end.
		for prefix in hiddenWhenUnavailable.prefixes where !ids.contains(prefix) {
			rendered += SettingsCatalog.descriptors.filter { $0.id.hasPrefix(prefix) }
		}
		return rendered
	}

	/// Whether the section renders an option.
	func places(_ id: String) -> Bool {
		descriptors.contains { $0.id == id }
	}


	var ids: [String] {
		switch content {
		case let .options(ids), let .growing(ids): return ids
		case .mounts, .customControls, .boards: return []
		}
	}
}

extension SettingsPage {
	/// The designed page's sections; nil for the generated pages.
	var layout: [LayoutSection]? {
		switch self {
		case .hardware: return SettingsPage.hardwareLayout
		case .displaySound: return SettingsPage.displaySoundLayout
		case .input: return SettingsPage.inputLayout
		case .storage: return SettingsPage.storageLayout
		case .portsMisc: return SettingsPage.portsMiscLayout
		case .overview, .advanced: return nil
		}
	}

	/// The options a generated page shows only while they are available
	/// (a designed page's sections say it for theirs): the Overview's
	/// second drive.
	var hiddenWhenUnavailable: HiddenWhenUnavailable {
		switch self {
		case .overview: return HiddenWhenUnavailable(ids: ["floppy1"])
		case .hardware, .displaySound, .input, .storage, .portsMisc, .advanced:
			// The designed pages' sections hold their own rules.
			return HiddenWhenUnavailable()
		}
	}

	/// The options of the page's groups no designed page's sections place
	/// (the shortcuts sit on Input although their group is Ports &
	/// Misc's): the page's More rows.
	var unplaced: [SettingDescriptor] {
		guard let layout else { return [] }
		let placed: (String) -> Bool = { id in
			SettingsPage.placing.contains { page in
				page.layout?.contains { $0.places(id) } ?? false
			}
		}
		return groups.flatMap(SettingsCatalog.descriptors(in:)).filter { !placed($0.id) }
	}

	/// Whether the page shows an option: a section of its layout places it
	/// (a designed page can place another page's group's options, as Input
	/// places the shortcut hotkeys), or the option's group is one it covers
	/// and no other page places it (the More rows).
	func places(_ id: String) -> Bool {
		if self == .overview, SettingsPage.overviewOptions.contains(id) {
			return true
		}
		return SettingsPage.pageOfOption[id] == self
	}

	/// The collapsed sections of the designed pages holding an option, by
	/// title: what a search hit expands so the option can be found where it
	/// lives.
	static func collapsedSections(containing id: String) -> Set<String> {
		var titles: Set<String> = []
		for page in placing {
			for section in page.layout ?? [] where section.collapsed {
				if section.ids.contains(id) {
					titles.insert(section.title)
				}
			}
		}
		return titles
	}

	/// Each option's page: the designed page whose sections render it,
	/// else the page covering its group. The sidebar's edited mark and the
	/// coverage tests read it; every option must land in it.
	static let pageOfOption: [String: SettingsPage] = {
		var pages: [String: SettingsPage] = [:]
		for page in placing {
			for layoutSection in page.layout ?? [] {
				for descriptor in layoutSection.descriptors where pages[descriptor.id] == nil {
					pages[descriptor.id] = page
				}
			}
		}
		for id in overviewOptions where pages[id] == nil {
			pages[id] = .overview
		}
		for page in placing {
			for descriptor in page.groups.flatMap(SettingsCatalog.descriptors(in:)) where pages[descriptor.id] == nil {
				pages[descriptor.id] = page
			}
		}
		return pages
	}()

	/// The second to fourth memory boards of a kind appear once the one
	/// before is fitted: most machines have one or none, so the page shows
	/// only the rows a machine can use.
	private static let memoryBoardsAfter: [String: [String]] = {
		var after: [String: [String]] = [:]
		for board in 2...4 {
			let previous = board == 2 ? "" : "\(board - 1)"
			let z2 = ["fastmem\(previous)_size", "fastmem\(previous)_size_k"]
			after["fastmem\(board)_size"] = z2
			after["fastmem\(board)_size_k"] = z2
			after["z3mem\(board)_size"] = ["z3mem\(previous)_size"]
		}
		return after
	}()

	private static let hardwareLayout: [LayoutSection] = [
		LayoutSection(title: "CPU", content: .options([
			"cpu_model", "cpu_compatible", "cpu_24bit_addressing", "cpu_speed", "fpu_model",
		])),
		LayoutSection(title: "CPU · Advanced", content: .options([
			"cpu_throttle", "cpu_multiplier", "amiberry.cpu_idle", "cpu_threaded", "cachesize",
			"compfpu", "comp_constjump", "comp_flushmode", "comp_nf", "comp_catchfault", "comp_trustbyte",
			"cpu_no_unimplemented", "cpu_data_cache", "mmu_model", "fpu_strict", "fpu_softfloat",
			"fpu_no_unimplemented",
		]), collapsed: true),
		LayoutSection(title: "Chipset", content: .options([
			"chipset", "chipset_compatible", "ntsc", "cycle_exact", "immediate_blits", "collision_level",
		])),
		// The chipset group's experts and the Advanced Chipset group's
		// compatibility internals, which the presets set for a machine.
		LayoutSection(title: "Chipset · Advanced", content: .options([
			"waiting_blits", "keyboard_type", "keyboard_nkro", "display_optimizations", "hvcsync",
			"rtc", "chipset_rtc_adjust", "ciaatod", "cia_overlay", "a1000ram", "df0idhw", "cd32cd",
			"cd32c2p", "cd32nvram", "cdtvcd", "cdtvram", "cdtv-cr", "ksmirror_e0", "ksmirror_a8",
			"color_burst", "resetwarning", "cia_todbug", "cs_bytecustomwritebug", "memory_pattern",
			"rom_is_slow", "toshiba_gary", "z3_autoconfig", "1mchipjumper", "ciaa_type",
			"unmapped_address_space", "eclocksync", "ramsey", "fatgary", "agnusrev", "deniserev",
			"agnusmodel", "agnussize", "denisemodel",
		]), collapsed: true),
		LayoutSection(title: "Memory", content: .options([
			"chipmem_size", "bogomem_size", "fastmem_size", "z3mem_size",
			"fastmem2_size", "fastmem2_size_k", "fastmem3_size", "fastmem3_size_k", "fastmem4_size",
			"fastmem4_size_k", "z3mem2_size", "z3mem3_size", "z3mem4_size",
		]), after: SettingsPage.memoryBoardsAfter),
		LayoutSection(title: "Memory · Advanced", content: .options([
			"fastmem_size_k", "z3mapping", "a3000mem_size", "mbresmem_size", "megachipmem_size",
		]), collapsed: true),
		LayoutSection(title: "ROMs", content: .options([
			"kickstart_rom_file", "kickstart_ext_rom_file", "cart_file",
		])),
		LayoutSection(title: "ROMs · Advanced", content: .options([
			"boot_rom_uae", "uaeboard", "flash_file", "rtc_file", "maprom", "kickshifter",
		]), collapsed: true),
		LayoutSection(title: "Accelerator board", content: .options([
			"cpuboard_type", "cpuboardmem1_size", "cpuboard_rom_file", "cpuboard_settings",
		]), hiddenWhenUnavailable: ["cpuboardmem1_size", "cpuboard_rom_file"], prefixes: ["cpuboard_settings"]),
		LayoutSection(title: "RTG board", content: .options([
			"gfxcard_type", "gfxcard_size", "gfxcard_options:monitor", "gfxcard_options:autoswitch",
			"gfxcard_options:initial", "gfxcard_hardware_sprite",
		])),
		LayoutSection(title: "RTG · Advanced", content: .options([
			"gfxcard_options", "rtg_modes", "rtg_modes:8bit", "rtg_modes:16bit", "rtg_modes:24bit",
			"rtg_modes:32bit", "gfx_filter_autoscale_rtg", "gfx_filter_autoscale_limit_rtg",
			"gfx_backbuffers_rtg", "gfxcard_zerocopy", "gfxcard_multithread", "gfxcard_hardware_vblank",
			"amiberry.rtg_scale_allow", "amiberry.rtg_scale_aspect_ratio", "amiberry.rtg_vblank",
			"zz9000_net", "zz9000_int2", "gfx_display_rtg",
		]), collapsed: true),
		LayoutSection(title: "Expansions", content: .options([
			"bsdsocket_emu", "scsi", "sana2",
		])),
		LayoutSection(title: "Expansion Boards", content: .boards),
		LayoutSection(title: "On-board interfaces", content: .options([
			"pcmcia", "ide", "scsi_a3000", "scsi_a4000t",
		])),
		LayoutSection(title: "PowerPC", content: .options([
			"ppc_model", "ppc_implementation", "ppc_cpu_idle",
		]), collapsed: true),
		LayoutSection(title: "x86 Bridgeboard", content: .options([
			"cpu_x86_throttle",
		]), collapsed: true),
		LayoutSection(title: "Genlock", content: .options([
			"genlock", "genlockmode", "genlock_image", "genlock_video", "genlock_mix", "genlock_alpha",
			"genlock_aspect",
		]), collapsed: true),
	]

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
			"gfx_vsync", "gfx_vsync_picasso", "gfx_vrr_monitor",
		])),
		LayoutSection(title: "Sync · Advanced", content: .options([
			"gfx_monitorblankdelay", "displaydata_pal", "displaydata_ntsc",
		]), collapsed: true),
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
		// The shortcut hotkeys are input although the layer files them
		// under misc with the Ports & Misc page's groups.
		LayoutSection(title: "Keyboard Shortcuts", content: .options([
			"amiberry.open_gui", "amiberry.quit_amiberry", "amiberry.action_replay", "amiberry.fullscreen_toggle",
			"amiberry.minimize", "amiberry.screenshot", "amiberry.debugger_trigger", "amiberry.left_amiga",
			"amiberry.right_amiga",
		])),
		LayoutSection(title: "Controller Shortcuts", content: .options([
			"amiberry.use_retroarch_menu", "amiberry.use_retroarch_quit", "amiberry.use_retroarch_reset",
		])),
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
			"serial_port", "amiberry.midiout_device_name", "amiberry.midiin_device_name",
		])),
		LayoutSection(title: "Serial & MIDI · Advanced", content: .options([
			"serial_on_demand", "serial_hardware_ctsrts", "serial_status", "serial_ri", "serial_direct",
			"serial_translate", "uaeserial", "amiberry.midirouter",
		]), collapsed: true),
		LayoutSection(title: "Copy Protection", content: .options(["dongle"])),
		LayoutSection(title: "Startup", content: .options([
			"use_gui", "cpu_reset_pause", "warpboot",
		])),
		LayoutSection(title: "Status Line", content: .options(["show_leds", "show_leds_rtg", "power_led_dim"])),
		LayoutSection(title: "Host Integration", content: .options(["clipboard_sharing", "synchronize_clock"])),
		// The desktop's window and pointer-capture options, which the layer
		// hides on iOS (no window of its own there).
		LayoutSection(title: "Window", content: .options([
			"amiberry.start_minimized", "amiberry.always_on_top", "amiberry.inactive_minimize",
			"amiberry.blank_monitors",
		])),
		LayoutSection(title: "Mouse & Keyboard", content: .options([
			"amiberry.start_not_captured", "amiberry.active_capture_automatically", "amiberry.ctrl_alt_release",
			"amiberry.alt_tab_release", "amiberry.right_control_is_right_win", "amiberry.kbd_led_num",
			"amiberry.kbd_led_scr", "amiberry.kbd_led_cap",
		])),
		LayoutSection(title: "Safety & Debugging", content: .options([
			"floppy_write_protect", "harddrive_write_protect", "cpu_halt_auto_reset", "native_code", "log_illegal_mem",
			"debug_mem",
		]), collapsed: true),
		LayoutSection(title: "Compatibility", content: .options(["uae_hide_autoconfig", "scsidevice_disable"]),
			collapsed: true),
		// The layer's own order: active, inactive, minimized.
		LayoutSection(title: "Priority", content: .options(SettingsCatalog.descriptors(in: "priority").map(\.id)),
			collapsed: true),
	]

	/// The entries of a growing section that show: the filled ones up to the
	/// last, and one empty one to fill next.
	static func visibleEntries(_ ids: [String], filled: (String) -> Bool) -> [String] {
		let last = ids.lastIndex(where: filled) ?? -1
		return Array(ids.prefix(min(last + 2, ids.count)))
	}
}

/// A designed page: its sections, then More for what they do not place.
/// A collapsed section hides its rows behind a disclosure the model
/// remembers; a section whose options the platform does not have does not
/// show at all.
struct DesignedSections: View {
	let page: SettingsPage
	let model: SettingsWindowModel

	var body: some View {
		ForEach(page.layout ?? []) { section in
			switch section.content {
			case .options:
				// What the section renders is what it places: ids and the
				// families its prefixes cover (LayoutSection.descriptors).
				let descriptors = section.descriptors
				if !descriptors.isEmpty {
					OptionSection(title: section.title, descriptors: descriptors, store: model.store,
						hiddenWhenUnavailable: section.hiddenWhenUnavailable,
						disclosure: section.collapsed ? Binding(
							get: { model.sectionExpanded(section.title) },
							set: { model.setSectionExpanded($0, section.title) }) : nil)
				}
			case let .growing(ids):
				GrowingSection(title: section.title, ids: ids, model: model)
			case .mounts:
				MountSections(model: model)
			case .customControls:
				CustomControlsSections(store: model.store)
			case .boards:
				BoardsSections(store: model.store)
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
