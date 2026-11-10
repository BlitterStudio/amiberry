/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the designed pages' sections: every option lands in exactly
 * one page section (a collapsed Advanced section included) and in All
 * Settings, the expert rows sit behind the disclosures, and a search hit
 * expands the collapsed section it lands in.
 */

@testable import AmiberryNativeUI
import XCTest

@MainActor
final class DesignedPagesTests: XCTestCase {
	private func section(_ title: String, of page: SettingsPage) throws -> LayoutSection {
		try XCTUnwrap(page.layout?.first { $0.title == title }, "\(page.title): no section \(title)")
	}

	func testEveryOptionLandsInExactlyOnePageSection() throws {
		var seen: [String: SettingsPage] = [:]
		for page in SettingsPage.placing {
			for layoutSection in page.layout ?? [] {
				for id in layoutSection.ids {
					if let first = seen[id] {
						XCTFail("\(id) is placed on two pages: \(first.title) and \(page.title)")
						continue
					}
					seen[id] = page
					XCTAssertNotNil(SettingsCatalog.descriptor(for: id), "\(id) is no option the layer has")
				}
			}
			// The generated Overview's groups are its section.
			if page.layout == nil {
				for descriptor in page.groups.flatMap(SettingsCatalog.descriptors(in:)) where seen[descriptor.id] == nil {
					seen[descriptor.id] = page
				}
			}
		}
		for descriptor in SettingsCatalog.descriptors {
			let page = try XCTUnwrap(seen[descriptor.id] ?? SettingsPage.pageOfOption[descriptor.id],
				"\(descriptor.id) (group \(descriptor.group)) is placed on no page section")
			XCTAssertEqual(SettingsPage.pageOfOption[descriptor.id], page, descriptor.id)
			// All Settings keeps every option reachable.
			XCTAssertTrue(SettingsPage.advanced.groups.contains(descriptor.group), descriptor.group)
			XCTAssertTrue(SettingsCatalog.descriptors(in: descriptor.group).contains { $0.id == descriptor.id })
		}
		// A section's prefixes place the families they cover (the
		// accelerator's jumpers), as many ids as the layer has of them.
		XCTAssertEqual(SettingsPage.pageOfOption["cpuboard_settings:scsi"], .hardware)
		// The Overview's groups are the Overview's alone.
		for id in SettingsCatalog.descriptors(in: "config").map(\.id) {
			XCTAssertEqual(SettingsPage.pageOfOption[id], .overview, id)
		}
		// An id the catalogue lacks can be on no page (a layer change the
		// front end has not followed).
		XCTAssertNil(SettingsPage.pageOfOption["not_an_option"])
	}

	/// The Hardware page's designed sections, with its expert rows behind
	/// collapsed Advanced disclosures instead of off the page.
	func testTheHardwarePageHoldsItsExpertsBehindDisclosures() throws {
		let layout = try XCTUnwrap(SettingsPage.hardware.layout)
		let collapsed = ["CPU · Advanced", "Chipset · Advanced", "Memory · Advanced", "ROMs · Advanced",
			"RTG · Advanced", "PowerPC", "x86 Bridgeboard", "Genlock"]
		for title in collapsed {
			XCTAssertTrue(try section(title, of: .hardware).collapsed, title)
		}
		for title in ["CPU", "Chipset", "Memory", "ROMs", "Accelerator board", "RTG board", "Expansions",
			"On-board interfaces"] {
			XCTAssertFalse(try section(title, of: .hardware).collapsed, title)
		}
		// The on-board interfaces stay on the page; the rest of the
		// Advanced Chipset group sits behind the chipset disclosure.
		let interfaces = Set(try section("On-board interfaces", of: .hardware).ids)
		XCTAssertEqual(interfaces, Set(["pcmcia", "ide", "scsi_a3000", "scsi_a4000t"]))
		let chipsetAdvanced = Set(try section("Chipset · Advanced", of: .hardware).ids)
		for id in SettingsCatalog.descriptors(in: "adv_chipset").map(\.id) where !interfaces.contains(id) {
			XCTAssertTrue(chipsetAdvanced.contains(id), "\(id) left the page")
		}
		// ppc, x86 and genlock stay, collapsed, in the layer's order.
		for (title, group) in [("PowerPC", "ppc"), ("x86 Bridgeboard", "x86"), ("Genlock", "genlock")] {
			XCTAssertEqual(try section(title, of: .hardware).ids,
				SettingsCatalog.descriptors(in: group).map(\.id), title)
		}
		// The sections the audit designed, in its order (the boards editor
		// has no options of its own).
		let visible = layout.filter { !$0.collapsed && !$0.ids.isEmpty }.map(\.title)
		XCTAssertEqual(visible, ["CPU", "Chipset", "Memory", "ROMs", "Accelerator board",
			"RTG board", "Expansions", "On-board interfaces"])
		XCTAssertEqual(layout.first { $0.content == .boards }?.title, "Expansion Boards")
	}

	/// The sync experts leave the Display & Sound page's Sync section for
	/// its collapsed Advanced disclosure.
	func testTheSyncExpertsSitBehindADisclosure() throws {
		XCTAssertEqual(try section("Sync", of: .displaySound).ids,
			["gfx_vsync", "gfx_vsync_picasso", "gfx_vrr_monitor"])
		let advanced = try section("Sync · Advanced", of: .displaySound)
		XCTAssertTrue(advanced.collapsed)
		XCTAssertEqual(advanced.ids, ["gfx_monitorblankdelay", "displaydata_pal", "displaydata_ntsc"])
	}

	/// The shortcut rows are the Input page's: a designed page may place
	/// another page's group's options, and the sidebar's edited mark
	/// follows the placement.
	func testTheShortcutsAreTheInputPageOptions() throws {
		let keyboard = try section("Keyboard Shortcuts", of: .input)
		let hotkeys = SettingsCatalog.descriptors(in: "misc").filter { $0.kind == .hotkey }.map(\.id)
		XCTAssertEqual(Set(keyboard.ids).subtracting(["amiberry.use_retroarch_menu",
			"amiberry.use_retroarch_quit", "amiberry.use_retroarch_reset"]), Set(hotkeys))
		XCTAssertEqual(try section("Controller Shortcuts", of: .input).ids,
			["amiberry.use_retroarch_menu", "amiberry.use_retroarch_quit", "amiberry.use_retroarch_reset"])
		for shortcut in hotkeys {
			XCTAssertTrue(SettingsPage.input.places(shortcut), shortcut)
			XCTAssertFalse(SettingsPage.portsMisc.places(shortcut), "\(shortcut) left Ports & Misc")
		}
		XCTAssertTrue(SettingsPage.input.shows(anyOf: ["amiberry.open_gui"]))
		XCTAssertFalse(SettingsPage.portsMisc.shows(anyOf: ["amiberry.open_gui"]))
	}

	/// A collapsed section opens with the user's tap and stays open across
	/// changes of page; a search hit in one expands it on its page, and
	/// closing it again wins.
	func testCollapsedSectionsOpenAndStayOpenAndASearchExpandsThem() throws {
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		XCTAssertFalse(model.sectionExpanded("CPU · Advanced"))
		model.setSectionExpanded(true, "CPU · Advanced")
		model.page = .input
		XCTAssertTrue(model.sectionExpanded("CPU · Advanced"), "remembered per section across page changes")
		model.setSectionExpanded(false, "CPU · Advanced")
		XCTAssertFalse(model.sectionExpanded("CPU · Advanced"))

		// A search hit inside a collapsed section expands it.
		let reveal = { (query: String) in
			model.revealSections(Set(SearchResultsView.matches(query)
				.flatMap { SettingsPage.collapsedSections(containing: $0.id) }))
		}
		reveal("MapROM")
		XCTAssertTrue(model.sectionExpanded("ROMs · Advanced"), "the hit's section shows expanded on its page")
		// The user can close it again.
		model.setSectionExpanded(false, "ROMs · Advanced")
		XCTAssertFalse(model.sectionExpanded("ROMs · Advanced"))
		// A search without a hit in a section does not keep it revealed;
		// one with a hit reopens it.
		reveal("cpu_model")
		XCTAssertFalse(model.sectionExpanded("ROMs · Advanced"))
		reveal("MapROM")
		XCTAssertTrue(model.sectionExpanded("ROMs · Advanced"))
	}
}

@MainActor
final class AppSettingsSectionsTests: XCTestCase {
	/// Every setting shows in exactly one section of its group, and no
	/// empty section shows.
	func testEverySettingIsInOneSectionOfItsGroup() {
		for group in AppSettingsModel.groups {
			let sections = group.sections
			XCTAssertFalse(sections.isEmpty, group.id)
			XCTAssertTrue(sections.allSatisfy { !$0.settings.isEmpty }, group.id)
			XCTAssertEqual(Set(sections.flatMap { $0.settings.map(\.id) }), Set(group.settings.map(\.id)), group.id)
			XCTAssertEqual(sections.map(\.id).count, Set(sections.map(\.id)).count, "section titles differ")
		}
	}

	func testTheInputDefaultsSplitIntoControllersAndShortcuts() throws {
		let group = try XCTUnwrap(AppSettingsModel.groups.first { $0.id == "global_input" })
		XCTAssertEqual(group.sections.map(\.title), ["Controllers & Mouse", "Default Shortcuts"])
		let controllers = try XCTUnwrap(group.sections.first)
		XCTAssertEqual(Set(controllers.settings.map(\.id)),
			Set(["default_controller1", "default_controller2", "default_controller3", "default_controller4",
				"default_mouse1", "default_mouse2", "input_default_mouse_speed", "default_joystick_deadzone",
				"input_keyboard_as_joystick_stop_keypresses", "rctrl_as_ramiga"]))
		// The RetroArch defaults sit inside the shortcuts, collapsed.
		let shortcuts = try XCTUnwrap(group.sections.last)
		XCTAssertEqual(shortcuts.disclosedTitle, "RetroArch")
		XCTAssertEqual(shortcuts.settings.map(\.id).filter { !shortcuts.disclosed.contains($0) },
			["macos_left_command_q", "default_open_gui_key", "default_quit_key", "default_ar_key",
				"default_fullscreen_toggle_key"])
		XCTAssertEqual(shortcuts.disclosed,
			Set(["default_retroarch_quit", "default_retroarch_menu", "default_retroarch_reset"]))
		// Search still finds a disclosed setting.
		XCTAssertEqual(group.sections(matching: "retroarch quit").flatMap { $0.settings.map(\.id) },
			["default_retroarch_quit"])
		// The disclosure remembers being opened.
		let model = AppSettingsModel()
		XCTAssertFalse(model.disclosureExpanded("global_input.RetroArch"))
		model.setDisclosureExpanded(true, "global_input.RetroArch")
		XCTAssertTrue(model.disclosureExpanded("global_input.RetroArch"))
		model.setDisclosureExpanded(false, "global_input.RetroArch")
		XCTAssertFalse(model.disclosureExpanded("global_input.RetroArch"))
	}

	func testTheDisplayDefaultsSplitFromTheWindowDefaults() throws {
		let group = try XCTUnwrap(AppSettingsModel.groups.first { $0.id == "global_display" })
		XCTAssertEqual(group.sections.map(\.title), ["Display Defaults", "Window Defaults"])
		// The window sizes follow their machine options onto iOS's one
		// full-screen window; the screen mode default stays with its.
		let width = try XCTUnwrap(HostSettings.all.first { $0.id == "default_width" })
		XCTAssertTrue(width.descriptor.hiddenPlatforms.contains(.iOS), "hidden on iOS")
		XCTAssertFalse(width.descriptor.hiddenPlatforms.contains(.macOS), "shown on the Mac")
		let height = try XCTUnwrap(HostSettings.all.first { $0.id == "default_height" })
		XCTAssertTrue(height.descriptor.hiddenPlatforms.contains(.iOS))
		let mode = try XCTUnwrap(HostSettings.all.first { $0.id == "default_fullscreen_mode" })
		XCTAssertEqual(mode.descriptor.hiddenPlatforms, SettingPlatforms(), "the full-window pick reaches iOS too")
	}

	func testThePathsSplitIntoLoggingAndFolders() throws {
		let group = try XCTUnwrap(AppSettingsModel.groups.first { $0.id == "global_paths" })
		XCTAssertEqual(group.sections.map(\.title), ["Logging", "Folders"])
		let logging = try XCTUnwrap(group.sections.first)
		XCTAssertEqual(Set(logging.settings.map(\.id)), Set(["write_logfile", "logfile_path"]))
		let folders = try XCTUnwrap(group.sections.last)
		XCTAssertFalse(folders.settings.map(\.id).contains("logfile_path"))
		XCTAssertTrue(folders.settings.allSatisfy { $0.kind == .path })
	}
}
