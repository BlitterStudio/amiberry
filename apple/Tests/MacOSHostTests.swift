/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the native macOS front end (apple/macOS) against the real
 * layer: the settings window's pages and actions, and the menu commands.
 */

import AmiberrySettings
import AppKit
@testable import AmiberryNativeUI
import SwiftUI
import XCTest

@MainActor
final class SettingsPagesTests: XCTestCase {
	func testEveryGroupHasOneSection() {
		let placed = SettingsPage.placing.flatMap(\.groups)
		XCTAssertEqual(Set(placed).count, placed.count, "no group in two sections")
		for group in SettingsCatalog.groups {
			XCTAssertTrue(placed.contains(group), "\(group) has no section")
		}
		XCTAssertEqual(Set(SettingsPage.advanced.groups), Set(SettingsCatalog.groups), "All Settings lists every group")
		for id in SettingsPage.overviewOptions {
			XCTAssertNotNil(SettingsCatalog.descriptor(for: id), id)
		}
		XCTAssertEqual(SettingsPage.groupTitle("future_group"), "Future Group")
	}

	func testDesignedPagesShowEachOptionOnce() throws {
		for page in SettingsPage.placing {
			guard let layout = page.layout else { continue }
			let mine = Set(page.groups.flatMap(SettingsCatalog.descriptors(in:)).map(\.id))
			let placed = layout.flatMap(\.ids)
			XCTAssertEqual(Set(placed).count, placed.count, "\(page.title): an option placed twice")
			for id in placed {
				XCTAssertTrue(mine.contains(id), "\(page.title): \(id) is not an option of the page's groups")
			}
			XCTAssertEqual(Set(placed).union(page.unplaced.map(\.id)), mine, page.title)
			XCTAssertTrue(Set(placed).isDisjoint(with: page.unplaced.map(\.id)), page.title)
		}
	}

	func testTheSwapperShowsItsEntriesAndOneEmpty() {
		let ids = (0..<20).map { "diskimage\($0)" }
		XCTAssertEqual(SettingsPage.visibleEntries(ids) { _ in false }, ["diskimage0"])
		XCTAssertEqual(SettingsPage.visibleEntries(ids) { $0 == "diskimage2" }, ["diskimage0", "diskimage1", "diskimage2", "diskimage3"],
			"a gap keeps the entries after it")
		XCTAssertEqual(SettingsPage.visibleEntries(ids) { _ in true }, ids, "all filled: no row past the last")
	}

	func testOptionsAreNamedInTheirSectionsWords() throws {
		XCTAssertEqual(OptionSection.label("DF0: image", in: "DF0:"), "Image")
		XCTAssertEqual(OptionSection.label("Port 1 device", in: "Port 1"), "Device")
		XCTAssertEqual(OptionSection.label("MMU", in: "MMU"), "MMU", "a label that is the section's name stays")
		XCTAssertEqual(OptionSection.label("Floppy drive emulation speed", in: "Floppy Drives"), "Floppy drive emulation speed",
			"only a whole name at the start is dropped")
		XCTAssertEqual(OptionSection.label("Port 10 device", in: "Port 1"), "Port 10 device")
		// Every section's options still read as distinct names: the designed
		// pages' and the generated ones' (a section per layer group, as
		// All Settings and search show them).
		var sections: [(title: String, ids: [String])] = SettingsPage.placing.flatMap { page in
			(page.layout ?? []).map { ($0.title, $0.ids) }
		}
		sections += SettingsCatalog.groups.map { (SettingsPage.groupTitle($0), SettingsCatalog.descriptors(in: $0).map(\.id)) }
		for section in sections {
			let labels = section.ids.compactMap(SettingsCatalog.descriptor(for:)).map { OptionSection.label($0.label, in: section.title) }
			XCTAssertEqual(Set(labels).count, labels.count, "\(section.title): \(labels)")
			XCTAssertFalse(labels.contains(""), section.title)
		}
	}

	func testASectionSaysEachReasonOnce() throws {
		let constrained = SettingsCatalog.descriptors.filter { $0.constraint?.isEmpty == false }
		let byReason = Dictionary(grouping: constrained) { $0.constraint ?? "" }
		let shared = try XCTUnwrap(byReason.values.first { $0.count >= 2 }, "two options with the same reason")
		let startsWithLabel = { (descriptor: SettingDescriptor) in
			(descriptor.constraint ?? "").lowercased().hasPrefix(descriptor.label.lowercased())
		}
		let single = try XCTUnwrap(constrained.first { $0.constraint != shared[0].constraint && !startsWithLabel($0) })
		let selfNamed = try XCTUnwrap(constrained.first { $0.constraint != shared[0].constraint && startsWithLabel($0) },
			"MMU: \"MMU emulation needs…\"")
		let section = Array(shared.prefix(2)) + [single, selfNamed]
		let unavailable = Set(section.map(\.id))
		XCTAssertEqual(OptionSection.constraintNotes(section) { !unavailable.contains($0) }, [
			try XCTUnwrap(shared[0].constraint),
			"\(single.label.replacingOccurrences(of: ":", with: "")): \(try XCTUnwrap(single.constraint))",
			try XCTUnwrap(selfNamed.constraint),
		], "a shared reason once, unnamed; a reason of one option names it unless it already does")
		XCTAssertEqual(OptionSection.constraintNotes(section) { _ in true }, [], "available options need no reason")
		let drive = single.labelled("DF1: image")
		XCTAssertEqual(OptionSection.constraintNotes([drive]) { _ in false }, ["DF1 image: \(try XCTUnwrap(single.constraint))"],
			"a name with a colon is not followed by a second one")
	}

	func testEditedOptionsMarkThePagesShowingThem() throws {
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		XCTAssertEqual(model.store.changedOptions, [])
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		XCTAssertTrue(model.store.set(current == "true" ? "false" : "true", for: "immediate_blits"))
		let edited = model.store.changedOptions
		XCTAssertEqual(edited, ["immediate_blits"])
		let marked = SettingsPage.placing.filter { $0.shows(anyOf: edited) }
		XCTAssertEqual(marked.count, 1, "an option is on one designed or generated page")
		XCTAssertTrue(SettingsPage.overview.shows(anyOf: ["floppy0"]) && SettingsPage.storage.shows(anyOf: ["floppy0"]),
			"the Overview's own options mark it as well as their page")
		try model.store.revert()
		XCTAssertEqual(model.store.changedOptions, [])
		XCTAssertFalse(SettingsPage.placing.contains { $0.isEdited(in: model.store) })

		// Hard drives are no option, but Storage lays them out.
		XCTAssertTrue(model.store.addMount("dir,rw,DH9:Work:\(NSTemporaryDirectory()),0"))
		XCTAssertEqual(SettingsPage.placing.filter { $0.isEdited(in: model.store) }, [.storage])
		try model.store.revert()
		XCTAssertFalse(SettingsPage.storage.isEdited(in: model.store))

		// Boards are no option either, but Hardware lays them out; its row
		// redraws without the mark when the edit is reverted.
		XCTAssertTrue(model.store.setBoardValue("true", field: "enabled", type: "a2091", unit: 0))
		XCTAssertTrue(SettingsPage.hardware.isEdited(in: model.store))
		let redrawn = expectation(description: "the Hardware row is told its mark changed")
		withObservationTracking {
			_ = SettingsPage.hardware.isEdited(in: model.store)
		} onChange: {
			redrawn.fulfill()
		}
		try model.store.revert()
		wait(for: [redrawn], timeout: 1)
		XCTAssertFalse(SettingsPage.hardware.isEdited(in: model.store))
	}

	func testAdjustmentsReadAsTheControlsShowValues() throws {
		let cpu = try XCTUnwrap(SettingsCatalog.descriptor(for: "cpu_model"))
		let choice = try XCTUnwrap(cpu.choices.first { $0.value != $0.label }, "a choice whose label is not its value")
		let adjusted = SettingAdjustment(optionID: cpu.id, requested: "", adjusted: choice.value, reason: "Needs a 32-bit CPU")
		XCTAssertEqual(adjusted.sentence(for: cpu, named: true), "\(cpu.label) will be \(choice.label): Needs a 32-bit CPU")
		XCTAssertEqual(adjusted.sentence(for: cpu, named: false), "Will be \(choice.label): Needs a 32-bit CPU")
		let toggle = try XCTUnwrap(SettingsCatalog.descriptors.first { $0.kind == .bool })
		let off = SettingAdjustment(optionID: toggle.id, requested: "true", adjusted: "false", reason: nil)
		XCTAssertEqual(off.sentence(for: toggle, named: true), "\(toggle.label) will be off")
		XCTAssertEqual(off.sentence(for: nil, named: true), "\(toggle.id) will be false", "an option the catalogue lacks reads raw")
	}

	func testEveryPageRendersEveryOption() throws {
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		for page in SettingsPage.sidebar {
			let view = NSHostingView(rootView: PageView(page: page, model: model).frame(width: 700, height: 500))
			view.frame = NSRect(x: 0, y: 0, width: 700, height: 500)
			view.layoutSubtreeIfNeeded()
			XCTAssertGreaterThan(view.fittingSize.height, 0, page.title)
		}
		let root = NSHostingView(rootView: SettingsRootView(model: model))
		root.frame = NSRect(x: 0, y: 0, width: 900, height: 620)
		root.layoutSubtreeIfNeeded()
	}

	// A configuration can hold a value outside an option's range today: a
	// manual crop height above 288 saved before single-line output limited
	// it, here with auto crop on so the window does not even apply. Showing
	// it, on a page that is open when the configuration is loaded (so the
	// field's text changes to it), must not count as an invalid entry that
	// blocks closing.
	func testAStoredValueOutsideTheRangeIsNotAnInvalidEntry() throws {
		let file = FileManager.default.temporaryDirectory.appendingPathComponent("crop-\(UUID().uuidString).uae")
		try "gfx_linemode=none\namiberry.gfx_auto_crop=true\namiberry.gfx_manual_crop_height=400\n"
			.write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		// As the Library's Edit does: load the machine, then open the editor.
		XCTAssertNotNil(EmulatorActions.post(.loadConfiguration(path: file.path)))
		ActionPump.runPending()
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		let id = "amiberry.gfx_manual_crop_height"
		let page = try XCTUnwrap(SettingsPage.sidebar.first { $0.title.contains("Display") }, "the Display page")
		// In a window (off screen) with the model in the environment, as in
		// the editor: onAppear and onChange run there, and fields report to
		// the model.
		let window = NSWindow(contentRect: NSRect(x: -10000, y: -10000, width: 700, height: 2000),
			styleMask: .borderless, backing: .buffered, defer: false)
		window.isReleasedWhenClosed = false
		defer { window.close() }
		let view = NSHostingView(rootView: PageView(page: page, model: model)
			.environment(model).frame(width: 700, height: 2000))
		window.contentView = view
		window.orderFrontRegardless()
		view.layoutSubtreeIfNeeded()
		RunLoop.main.run(until: Date().addingTimeInterval(0.2))
		XCTAssertEqual(model.store.value(of: id), "400")
		XCTAssertLessThan(try XCTUnwrap(model.store.range(for: id)).upperBound, 400, "the stored value is out of range")
		XCTAssertEqual(model.invalidDrafts, [:], "nothing was typed")
		XCTAssertEqual(model.store.value(of: id), "400", "showing the value leaves it alone")
	}

	func testSearchFindsOptionsByLabelHelpAndKey() {
		XCTAssertTrue(SearchResultsView.matches("cpu_model").contains { $0.id == "cpu_model" })
		XCTAssertTrue(SearchResultsView.matches("24-bit").contains { $0.id == "cpu_24bit_addressing" })
		XCTAssertTrue(SearchResultsView.matches("nothing matches this text").isEmpty)
		let both = SearchResultsView.matches("jit fpu")
		XCTAssertTrue(both.contains { $0.id == "compfpu" })
	}

	func testTheCDDriveShowsWhatTheLayerStores() throws {
		let store = try SettingsStore()
		let id = CDDriveControl.optionID
		XCTAssertTrue(store.set("", for: id))
		XCTAssertEqual(CDDriveControl.summary(store.value(of: id) ?? "?"), "No CD Drive")
		XCTAssertTrue(store.set("empty", for: id))
		XCTAssertEqual(CDDriveControl.summary(store.value(of: id) ?? "?"), "Empty Drive")
		XCTAssertNil(CDDriveControl.imagePath(store.value(of: id) ?? "?"))
		// A path with a comma, as the configuration quotes it.
		let disc = FileManager.default.temporaryDirectory.appendingPathComponent("Demo, Disc.iso").path
		XCTAssertTrue(store.set("\(disc),image", for: id))
		let stored = try XCTUnwrap(store.value(of: id))
		XCTAssertEqual(CDDriveControl.imagePath(stored), disc)
		XCTAssertEqual(CDDriveControl.summary(stored), "Demo, Disc.iso")
	}

	func testVolumesReadAsVolumes() throws {
		let cd = try XCTUnwrap(SettingsCatalog.descriptor(for: "sound_volume_cd"))
		XCTAssertTrue(cd.reversed)
		XCTAssertTrue(cd.level, "a volume is set by feel: a slider")
		XCTAssertEqual(cd.unit, "%")
		// The configuration's 20 (attenuation) is an 80% volume, and back.
		XCTAssertEqual(cd.shown(try XCTUnwrap(Double(cd.defaultValue))), 80)
		XCTAssertEqual(cd.stored(fromShown: 80), 20)
		XCTAssertEqual(cd.shown(0...100), 0...100)
		XCTAssertEqual(cd.shown(10...30), 70...90)
		XCTAssertTrue(cd.labelled("CD").reversed, "a designed page's label keeps the direction")
		// The slider is at the level the stored attenuation means, between
		// the speakers, and reads in percent.
		XCTAssertEqual(cd.shownLevel(of: cd.defaultValue, in: 0...100), 80)
		XCTAssertEqual(cd.shownLevel(of: "100", in: 0...100), 0)
		XCTAssertEqual(cd.shownLevel(of: "", in: 0...100), 80, "no number: the default's level")
		XCTAssertEqual(cd.levelText(80, signed: false), "80%")
		XCTAssertEqual(cd.levelEndSymbols(in: 0...100)?.low, "speaker.fill")
		XCTAssertEqual(cd.levelEndSymbols(in: 0...100)?.high, "speaker.wave.3.fill")
		let delay = try XCTUnwrap(SettingsCatalog.descriptor(for: "sound_stereo_mixing_delay"))
		XCTAssertFalse(delay.reversed)
		XCTAssertEqual(delay.shown(3), 3)
	}

	/// A descriptor as the layer declares one, for the rules no option of
	/// the catalogue pins down.
	private func declared(_ type: amiberry_setting_type, _ range: ClosedRange<Double>, step: Double = 1,
		reversed: Bool = false, scale: Double = 0, unit: String? = nil) -> SettingDescriptor {
		var d = amiberry_setting_descriptor()
		d.id = UnsafePointer(strdup("test_option"))
		d.group = UnsafePointer(strdup("test"))
		d.label = UnsafePointer(strdup("Test option"))
		d.help = UnsafePointer(strdup(""))
		d.type = type
		d.min = range.lowerBound
		d.max = range.upperBound
		d.step = step
		d.default_value = UnsafePointer(strdup("0"))
		d.reversed = reversed
		d.unit = unit.flatMap { UnsafePointer(strdup($0)) }
		d.scale = scale
		return SettingDescriptor(d)
	}

	func testShownNumbersGoBackToTheStoredOnes() {
		// Reversed and scaled: a level stored as tenths of attenuation.
		let both = declared(AMIBERRY_SETTING_INT, 0...1000, step: 5, reversed: true, scale: 10)
		XCTAssertEqual(both.shown(0), 100)
		XCTAssertEqual(both.shown(250), 75)
		XCTAssertEqual(both.stored(fromShown: 75), 250)
		XCTAssertEqual(both.stored(fromShown: 74.8), 250, "rounded to the step in stored units")
		XCTAssertEqual(both.shown(0...1000), 0...100)
		for stored in stride(from: 0.0, through: 1000, by: 5) {
			XCTAssertEqual(both.stored(fromShown: both.shown(stored)), stored)
		}
		// Scaled alone: a gamma in tenths, either side of zero.
		let gamma = declared(AMIBERRY_SETTING_INT, -1000...1000, scale: 10)
		XCTAssertEqual(gamma.shown(125), 12.5)
		XCTAssertEqual(gamma.stored(fromShown: 12.5), 125)
		XCTAssertEqual(gamma.stored(fromShown: -0.3), -3)
		XCTAssertEqual(gamma.shown(-1000...1000), -100...100)
		for stored in stride(from: -1000.0, through: 1000, by: 1) {
			XCTAssertEqual(gamma.stored(fromShown: gamma.shown(stored)), stored)
		}
		// Neither: shown as stored, a real rounded to its step.
		let plain = declared(AMIBERRY_SETTING_REAL, 0...10, step: 0.5)
		XCTAssertEqual(plain.scale, 1, "a scale of 0 is none")
		XCTAssertEqual(plain.shown(2.5), 2.5)
		XCTAssertEqual(plain.stored(fromShown: 2.3), 2.5)
		XCTAssertEqual(plain.stored(fromShown: 2.5), 2.5)
	}

	func testNumbersReadInTheirUnitAndScale() {
		let gamma = declared(AMIBERRY_SETTING_INT, -1000...1000, scale: 10)
		XCTAssertEqual(gamma.shownText("125"), "12.5")
		XCTAssertEqual(gamma.shownText("0"), "0")
		XCTAssertEqual(gamma.storedText(gamma.stored(fromShown: 12.5)), "125")
		XCTAssertEqual(gamma.levelText(12.5, signed: true), "+12.5")
		XCTAssertEqual(gamma.levelText(0, signed: true), "0")
		XCTAssertEqual(gamma.levelText(-3, signed: true), "-3")
		XCTAssertEqual(gamma.levelEndSymbols(in: gamma.shown(-1000...1000))?.low, "minus")
		// Typed as it reads; nothing rounded behind the user's back.
		XCTAssertEqual(gamma.storedNumber(typed: "12.5", in: -1000...1000), 125)
		XCTAssertEqual(gamma.storedNumber(typed: " -3 ", in: -1000...1000), -30)
		XCTAssertNil(gamma.storedNumber(typed: "12.54", in: -1000...1000), "gamma goes by tenths")
		XCTAssertNil(gamma.storedNumber(typed: "150", in: -1000...1000), "out of range")
		XCTAssertNil(gamma.storedNumber(typed: "much", in: -1000...1000))
		XCTAssertEqual(gamma.rangeSentence(-1000...1000), "Enter a number from -100 to 100 in steps of 0.1.")
		let width = declared(AMIBERRY_SETTING_INT, 320...1920, unit: "px")
		XCTAssertEqual(width.displayValue("720"), "720 px")
		XCTAssertEqual(width.rangeSentence(320...1920), "Enter a number from 320 to 1920 px.")
		XCTAssertNil(width.levelEndSymbols(in: 320...1920))
		XCTAssertEqual(declared(AMIBERRY_SETTING_INT, 0...100, unit: "%").withUnit("80"), "80%")
	}

	func testOnOffOptionsAreSwitches() throws {
		XCTAssertTrue(SettingDescriptor.isSwitch(kind: .integer, range: 0...1, choices: []))
		XCTAssertTrue(declared(AMIBERRY_SETTING_INT, 0...1).isSwitch)
		XCTAssertFalse(SettingDescriptor.isSwitch(kind: .integer, range: 0...2, choices: []))
		XCTAssertFalse(SettingDescriptor.isSwitch(kind: .real, range: 0...1, choices: []))
		func choices(_ pairs: [(String, String)]) -> [SettingChoice] {
			pairs.map { SettingChoice(value: $0.0, label: $0.1) }
		}
		for off in ["Disabled", "Off", "No", "None"] {
			XCTAssertTrue(SettingDescriptor.isSwitch(kind: .choice, range: nil, choices: choices([("0", off), ("1", "On")])), off)
		}
		XCTAssertFalse(SettingDescriptor.isSwitch(kind: .choice, range: nil, choices: choices([("0", "Auto"), ("1", "Manual")])))
		XCTAssertFalse(SettingDescriptor.isSwitch(kind: .choice, range: nil,
			choices: choices([("1", "Enabled"), ("0", "Disabled")])), "off comes first")
		XCTAssertFalse(SettingDescriptor.isSwitch(kind: .choice, range: nil,
			choices: choices([("0", "Off"), ("1", "On"), ("2", "Auto")])))
		let autoResolution = try XCTUnwrap(HostSettings.all.first { $0.id == "default_gfx_autoresolution" })
		XCTAssertTrue(autoResolution.descriptor.isSwitch)
		XCTAssertEqual(autoResolution.descriptor.displayValue("1"), "on")
		// A threshold the GUI copied in (33%) is no on/off value: the
		// option keeps its pop-up for it, rather than a switch showing off.
		XCTAssertTrue(autoResolution.descriptor.showsAsSwitch("0"))
		XCTAssertFalse(autoResolution.descriptor.showsAsSwitch("33"))
		let mouseMap = try XCTUnwrap(SettingsCatalog.descriptor(for: "joyport0mousemap"))
		XCTAssertFalse(mouseMap.isSwitch, "a stick mask (2 is the right stick) is no on/off value")
	}

	func testUnavailableOptionsHideOnlyWhereAPageSaysSo() throws {
		let hidden = HiddenWhenUnavailable(ids: ["floppy1"], prefixes: ["cpuboard_settings"])
		XCTAssertTrue(hidden.contains("cpuboard_settings"))
		XCTAssertTrue(hidden.contains("cpuboard_settings:maprom"), "a board's jumpers")
		XCTAssertFalse(hidden.contains("cpuboard_type"))
		let ids = ["floppy0", "floppy1", "cpuboard_type", "cpuboard_settings"]
		let descriptors = ids.compactMap(SettingsCatalog.descriptor(for:))
		XCTAssertEqual(descriptors.count, ids.count)
		XCTAssertEqual(hidden.shown(descriptors) { _ in true }.map(\.id), ids)
		XCTAssertEqual(hidden.shown(descriptors) { _ in false }.map(\.id), ["floppy0", "cpuboard_type"],
			"the others stay, dimmed")
		XCTAssertEqual(hidden.shown(descriptors) { $0 != "floppy1" }.map(\.id), ["floppy0", "cpuboard_type", "cpuboard_settings"])
		// The pages: a disabled drive's mode and image, a silent drive's
		// volumes, an absent accelerator's jumpers, memory and ROM.
		let hardware = SettingsPage.hardware.hiddenWhenUnavailable
		for id in ["cpuboard_settings", "cpuboard_settings:hwrev", "cpuboardmem1_size", "cpuboard_rom_file"] {
			XCTAssertTrue(hardware.contains(id), id)
		}
		XCTAssertFalse(hardware.contains("cpuboard_type"))
		XCTAssertTrue(SettingsPage.overview.hiddenWhenUnavailable.contains("floppy1"))
		XCTAssertEqual(SettingsPage.advanced.hiddenWhenUnavailable, HiddenWhenUnavailable(), "All Settings lists everything")
		let storage = try XCTUnwrap(SettingsPage.storage.layout?.first { $0.title == "DF1:" })
		XCTAssertTrue(storage.hiddenWhenUnavailable.contains("floppy1subtype"))
		XCTAssertTrue(storage.hiddenWhenUnavailable.contains("floppy1"))
		XCTAssertFalse(storage.hiddenWhenUnavailable.contains("floppy1type"), "the type is what enables the drive")
		let sounds = try XCTUnwrap(SettingsPage.displaySound.layout?.first { $0.title == "Drive Sounds" })
		XCTAssertTrue(sounds.hiddenWhenUnavailable.contains("floppy2soundvolume_disk"))
		XCTAssertFalse(sounds.hiddenWhenUnavailable.contains("floppy2sound"))
		// A section hides only its own options.
		for page in SettingsPage.placing {
			for section in page.layout ?? [] {
				XCTAssertTrue(section.hiddenWhenUnavailable.ids.isSubset(of: Set(section.ids)), section.title)
			}
		}
	}

	func testFurtherMemoryBoardsAppearOnceTheOneBeforeIsFitted() throws {
		let hardware = SettingsPage.hardware.hiddenWhenUnavailable
		let ids = ["fastmem_size", "fastmem_size_k", "fastmem2_size", "fastmem2_size_k", "fastmem3_size",
			"z3mem_size", "z3mem2_size", "z3mem3_size"]
		let descriptors = try ids.map { try XCTUnwrap(SettingsCatalog.descriptor(for: $0), $0) }
		func shown(_ values: [String: String]) -> [String] {
			hardware.shown(descriptors, available: { _ in true }, value: { values[$0] ?? "0" }).map(\.id)
		}
		XCTAssertEqual(shown([:]), ["fastmem_size", "fastmem_size_k", "z3mem_size"],
			"one board of each kind while none is fitted")
		XCTAssertEqual(shown(["fastmem_size": "4"]),
			["fastmem_size", "fastmem_size_k", "fastmem2_size", "fastmem2_size_k", "z3mem_size"])
		XCTAssertEqual(shown(["fastmem_size_k": "512"]),
			["fastmem_size", "fastmem_size_k", "fastmem2_size", "fastmem2_size_k", "z3mem_size"], "a board below 1 MB counts")
		XCTAssertTrue(shown(["fastmem3_size": "2"]).contains("fastmem3_size"), "a fitted board always shows")
		XCTAssertEqual(shown(["z3mem_size": "64", "z3mem2_size": "none"]).filter { $0.hasPrefix("z3") },
			["z3mem_size", "z3mem2_size"])
	}

	func testPortsAndMiscIsLaidOutByTask() throws {
		let layout = try XCTUnwrap(SettingsPage.portsMisc.layout)
		// Every option of its groups once, or under More: the designed
		// pages' coverage test covers it with the others.
		let placed = layout.flatMap(\.ids)
		let mine = Set(SettingsPage.portsMisc.groups.flatMap(SettingsCatalog.descriptors(in:)).map(\.id))
		XCTAssertEqual(Set(placed).count, placed.count)
		XCTAssertEqual(Set(placed).union(SettingsPage.portsMisc.unplaced.map(\.id)), mine)
		let section = { (title: String) in layout.first { $0.title == title }?.ids ?? [] }
		XCTAssertEqual(section("Priority"), SettingsCatalog.descriptors(in: "priority").map(\.id), "the group's own order")
		let hotkeys = SettingsCatalog.descriptors(in: "misc").filter { $0.kind == .hotkey }.map(\.id)
		XCTAssertFalse(hotkeys.isEmpty)
		XCTAssertEqual(Set(section("Keyboard Shortcuts")), Set(hotkeys))
		XCTAssertTrue(section("Serial & MIDI").contains("serial_port"))
		XCTAssertTrue(section("Safety & Debugging").contains("floppy_write_protect"))
	}

	func testTheKickstartPopUpOffersAROSTheScannedROMsAndAnother() throws {
		let rom = try XCTUnwrap(SettingsCatalog.descriptor(for: KickstartROMControl.optionID))
		XCTAssertEqual(rom.kind, .path)
		let roms = [
			MediaEntry(path: "/ROMs/kick13.rom", name: "KS ROM v1.3 (A500,A1000,A2000)"),
			MediaEntry(path: "/ROMs/kick31.rom", name: "KS ROM v3.1 (A1200)"),
			MediaEntry(path: "/ROMs/Forever/amiga-os-310-a1200.rom", name: "KS ROM v3.1 (A1200)"),
		]
		let scanned = KickstartROMControl.choices(roms: roms, current: "/ROMs/kick13.rom")
		XCTAssertEqual(scanned.map(\.value), ["", "/ROMs/kick13.rom", "/ROMs/kick31.rom", "/ROMs/Forever/amiga-os-310-a1200.rom"])
		XCTAssertEqual(scanned.map(\.label), [
			"AROS (built-in)", "KS ROM v1.3 (A500,A1000,A2000)",
			"KS ROM v3.1 (A1200) \u{2014} kick31.rom", "KS ROM v3.1 (A1200) \u{2014} amiga-os-310-a1200.rom",
		], "a name two ROMs share says which file")
		// A ROM picked outside the scan shows by its file name, after AROS.
		let other = KickstartROMControl.choices(roms: roms, current: "/Elsewhere/my.rom")
		XCTAssertEqual(other.prefix(2).map(\.value), ["", "/Elsewhere/my.rom"])
		XCTAssertEqual(other[1].label, "my.rom")
		XCTAssertEqual(KickstartROMControl.choices(roms: [], current: "").map(\.label), ["AROS (built-in)"])
	}
}

@MainActor
final class BoardsAndControlsTests: XCTestCase {
	func testBoardsAreAddedAndRemovedThroughTheStore() throws {
		let store = try SettingsStore()
		let a2091 = try XCTUnwrap(BoardCatalog.type("a2091"))
		XCTAssertEqual(BoardsSections.category(of: a2091), "SCSI")
		XCTAssertEqual(store.boardValue("enabled", type: "a2091", unit: 0), "false")
		XCTAssertFalse(store.hasChanges)
		XCTAssertTrue(store.setBoardValue("true", field: "enabled", type: "a2091", unit: 0))
		XCTAssertEqual(store.boardValue("enabled", type: "a2091", unit: 0), "true")
		XCTAssertTrue(store.hasChanges, "a board edit is a change to commit")
		XCTAssertFalse(store.setBoardValue("nope", field: "subtype", type: "a2091", unit: 0))
		XCTAssertNotNil(store.lastError)
		XCTAssertTrue(store.setBoardValue("false", field: "enabled", type: "a2091", unit: 0))
		// Refused text in a board field is named after the unit and field.
		XCTAssertEqual(SettingsWindowModel.draftLabel("board:a2091:1:rom_file"),
			"\(a2091.label) #2 \(a2091.fields.first { $0.id == "rom_file" }?.label ?? "")")
		XCTAssertEqual(SettingsWindowModel.draftLabel("cpu_model"), SettingsCatalog.descriptor(for: "cpu_model")?.label)
		// Every board type has a heading in the Add Board menu.
		for type in BoardCatalog.types {
			XCTAssertTrue(BoardsSections.categories.contains { $0.title == BoardsSections.category(of: type) }, type.id)
		}
	}

	func testABoardChangeWaitsForARestart() throws {
		let model = try SettingsWindowModel(emulating: true, dismiss: {}, post: { _ in true })
		XCTAssertFalse(model.boardsWaitForRestart)
		XCTAssertTrue(model.store.setBoardValue("true", field: "enabled", type: "a2091", unit: 0))
		XCTAssertTrue(model.boardsWaitForRestart, "the running emulation has the board only after a restart")
		XCTAssertTrue(model.store.setBoardValue("false", field: "enabled", type: "a2091", unit: 0))
		XCTAssertFalse(model.boardsWaitForRestart)
	}

	func testCustomControlsNeedAJoystick() throws {
		let store = try SettingsStore()
		// The test host has no joystick on the joystick port.
		XCTAssertNil(store.customControl("a", port: 1, hotkey: false))
		XCTAssertFalse(store.setCustomControl("Space", control: "a", port: 1, hotkey: false))
		XCTAssertFalse(store.hasChanges)
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		for page in [SettingsPage.hardware, .input] {
			let view = NSHostingView(rootView: PageView(page: page, model: model).frame(width: 700, height: 500))
			view.frame = NSRect(x: 0, y: 0, width: 700, height: 500)
			view.layoutSubtreeIfNeeded()
		}
	}
}

@MainActor
final class EmulationChromeTests: XCTestCase {
	/// A running emulator's status: DF0: holds the disk, the drives listed
	/// are disabled, and a CD drive (activity 0 or more) holds the CD.
	static func status(disk: String, disabled: [Int] = [], cd: Int32 = -1, cdPath: String = "") -> EmulatorStatus {
		var raw = amiberry_actions_status()
		raw.run_state = AMIBERRY_RUN_STATE_RUNNING
		raw.cd_activity = cd
		raw.hd_activity = -1
		withUnsafeMutableBytes(of: &raw.floppy) { bytes in
			let drives = bytes.bindMemory(to: amiberry_drive_status.self)
			for index in drives.indices {
				drives[index].disabled = disabled.contains(index)
			}
			withUnsafeMutableBytes(of: &drives[0].media_path) { path in
				path.copyBytes(from: Array(disk.utf8) + [0])
			}
		}
		withUnsafeMutableBytes(of: &raw.cd_path) { path in
			path.copyBytes(from: Array(cdPath.utf8) + [0])
		}
		return EmulatorStatus(raw)
	}

	/// Puts the toolbar on `window` without the saved layout, so the test
	/// reads and writes nothing in the user's defaults.
	private func attach(_ window: NSWindow) {
		let chrome = EmulationChrome.shared
		let autosaves = chrome.autosavesToolbar
		chrome.autosavesToolbar = false
		defer { chrome.autosavesToolbar = autosaves }
		chrome.attach(to: window)
		XCTAssertEqual(window.toolbar?.autosavesConfiguration, false)
	}

	func testTheDrivesMenuFollowsTheDrives() {
		let menu = NSMenu()
		DrivesMenu.fill(menu, status: Self.status(disk: "/disks/Lotus.adf", disabled: [2, 3]),
			swapper: ["/disks/Lotus.adf", "", "/disks/Lotus2.adf"] + Array(repeating: "", count: 17),
			commands: MenuCommands.shared, swapperTarget: EmulationChrome.shared)
		let titles = menu.items.map(\.title)
		XCTAssertEqual(titles.filter { $0.hasPrefix("DF") }, ["DF0: Lotus.adf", "DF1: Empty"], "disabled drives are left out")
		XCTAssertFalse(titles.contains { $0.hasPrefix("CD") }, "no CD drive, no CD entries")
		let ejects = menu.items.filter { $0.title == "Eject" }
		XCTAssertEqual(ejects.map(\.tag), [0, 1])
		XCTAssertNotNil(ejects[0].action)
		XCTAssertNil(ejects[1].action, "nothing to eject from an empty drive")
		let swapper = try? XCTUnwrap(menu.items.last?.submenu)
		XCTAssertEqual(swapper?.items.map(\.title), ["Lotus.adf", "Lotus2.adf"])
		let into = swapper?.items.last?.submenu?.items
		XCTAssertEqual(into?.map(\.title), ["Insert into DF0:", "Insert into DF1:"])
		XCTAssertEqual(into?.last?.representedObject as? [Int], [2, 1])
	}

	func testTheDrivesMenuHasTheCDWhenThereIsOne() {
		let menu = NSMenu()
		DrivesMenu.fill(menu, status: Self.status(disk: "", cd: 0), swapper: Array(repeating: "", count: 20),
			commands: MenuCommands.shared, swapperTarget: EmulationChrome.shared)
		XCTAssertTrue(menu.items.contains { $0.title == "CD Empty" })
		XCTAssertNil(menu.items.first { $0.title == "Eject CD" }?.action)
		XCTAssertEqual(menu.items.last?.submenu?.items.map(\.title), ["No Entries"])
	}

	func testEachDriveOffersTheRecentMedia() throws {
		let menu = NSMenu()
		let disks = (0..<12).map { "/disks/Disk \($0).adf" }
		DrivesMenu.fill(menu, status: Self.status(disk: "", disabled: [2, 3], cd: 0), swapper: Array(repeating: "", count: 20),
			commands: MenuCommands.shared, swapperTarget: EmulationChrome.shared,
			recentDisks: disks, recentCDs: ["/cds/Demo.cue"])
		let recents = menu.items.filter { $0.title == "Insert Recent" }.compactMap(\.submenu)
		XCTAssertEqual(recents.count, 3, "DF0:, DF1: and the CD")
		XCTAssertEqual(recents[0].items.map(\.title), Array(disks.prefix(DrivesMenu.recentLimit)).map { ($0 as NSString).lastPathComponent },
			"the most recent first, up to the limit")
		let df1 = try XCTUnwrap(recents[1].items.first?.representedObject as? DrivesMenu.RecentInsert)
		XCTAssertEqual(df1.drive, 1)
		XCTAssertEqual(df1.path, disks[0])
		let cd = try XCTUnwrap(recents[2].items.first?.representedObject as? DrivesMenu.RecentInsert)
		XCTAssertNil(cd.drive, "a CD goes into the CD drive")
		XCTAssertEqual(cd.path, "/cds/Demo.cue")

		let none = NSMenu()
		DrivesMenu.fill(none, status: Self.status(disk: ""), swapper: Array(repeating: "", count: 20),
			commands: MenuCommands.shared, swapperTarget: EmulationChrome.shared)
		XCTAssertFalse(none.items.contains { $0.title == "Insert Recent" }, "no recent media, no submenu")
	}

	func testTheToolbarKeepsTheContentWhereSDLPutIt() {
		let window = NSWindow(contentRect: NSRect(x: 200, y: 200, width: 720, height: 568),
			styleMask: [.titled, .closable, .miniaturizable], backing: .buffered, defer: false)
		defer { window.close() }
		window.isReleasedWhenClosed = false
		let content = window.contentRect(forFrameRect: window.frame)
		attach(window)
		XCTAssertNotNil(window.toolbar)
		XCTAssertEqual(window.contentRect(forFrameRect: window.frame), content, "same size and place")
		// A second emulated monitor's window (an RTG screen) has its own.
		let second = NSWindow(contentRect: NSRect(x: 300, y: 300, width: 640, height: 480),
			styleMask: [.titled], backing: .buffered, defer: false)
		second.isReleasedWhenClosed = false
		defer { second.close() }
		attach(second)
		NotificationCenter.default.post(name: NSWindow.willEnterFullScreenNotification, object: window)
		XCTAssertEqual(window.toolbar?.isVisible, false, "full screen is the emulation's alone")
		XCTAssertEqual(second.toolbar?.isVisible, true, "the other window keeps its toolbar")
		NotificationCenter.default.post(name: NSWindow.didExitFullScreenNotification, object: window)
		XCTAssertEqual(window.toolbar?.isVisible, true)
		NotificationCenter.default.post(name: NSWindow.willEnterFullScreenNotification, object: second)
		XCTAssertEqual(second.toolbar?.isVisible, false)
	}

	func testTheToolbarMenusShowEveryEntry() throws {
		let window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 720, height: 568),
			styleMask: [.titled], backing: .buffered, defer: false)
		window.isReleasedWhenClosed = false
		defer { window.close() }
		attach(window)
		let items = try XCTUnwrap(window.toolbar).items
		let reset = try XCTUnwrap(items.first { $0.label == "Reset" } as? NSMenuToolbarItem)
		// A pull-down's first item is its title: the commands follow it.
		XCTAssertEqual(reset.menu.items.dropFirst().map(\.title), ["Reset", "Keyboard Reset", "Hard Reset"])
		let state = try XCTUnwrap(items.first { $0.label == "State" } as? NSMenuToolbarItem)
		XCTAssertEqual(state.menu.items.dropFirst().map(\.title).first, "Quick Save State")
	}

	func testTheFrameRateReadsInTenths() {
		XCTAssertEqual(StatusLEDsView.fps(499), "49.9 fps")
		XCTAssertEqual(StatusLEDsView.fps(0), "", "no rate before the first frames")
	}
}

@MainActor
final class QuickPlayTests: XCTestCase {
	func testQuickPlayTakesMediaAndConfigurations() {
		for name in ["Game.ADF", "game.lha", "disc.iso", "disc.cue", "disc.chd", "Lotus.uae", "pack.rp9", "d.ipf", "d.dms", "d.adz", "d.zip", "d.7z"] {
			XCTAssertTrue(QuickPlay.accepts(URL(fileURLWithPath: "/tmp/\(name)")), name)
		}
		for name in ["notes.txt", "state.uss", "kick.rom", "disk"] {
			XCTAssertFalse(QuickPlay.accepts(URL(fileURLWithPath: "/tmp/\(name)")), name)
		}
		XCTAssertFalse(QuickPlay.accepts(URL(string: "https://example.com/game.adf")!), "a file, not a link")
	}
}

@MainActor
final class WindowHostingTests: XCTestCase {
	private let autosave = "AmiberryWindowHostingTest"

	override func tearDown() {
		UserDefaults.standard.removeObject(forKey: "NSWindow Frame \(autosave)")
		super.tearDown()
	}

	/// A dismissed editor's content must not stay alive with its window:
	/// it owns the native settings session.
	func testReleasedWindowContentIsFreed() {
		let window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 400, height: 300),
			styleMask: [.titled, .closable], backing: .buffered, defer: true)
		window.isReleasedWhenClosed = false
		weak var hosted: NSHostingController<Text>?
		autoreleasepool {
			let controller = hostWindowContent(Text("content"))
			hosted = controller
			presentWindow(controller, in: window, autosave: autosave, size: NSSize(width: 400, height: 300))
		}
		RunLoop.main.run(until: Date().addingTimeInterval(0.1))
		XCTAssertNotNil(hosted, "the window keeps its content while shown")

		autoreleasepool {
			window.orderOut(nil)
			releaseWindowContent(window)
			RunLoop.main.run(until: Date().addingTimeInterval(0.1))
		}
		XCTAssertNil(hosted, "released content is freed while the window object is kept")
		XCTAssertNil(window.contentView)
	}
}

@MainActor
final class ROMImportTests: XCTestCase {
	private func folder() throws -> URL {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("roms-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder.appendingPathComponent("nested"), withIntermediateDirectories: true)
		addTeardownBlock { try? FileManager.default.removeItem(at: folder) }
		return folder
	}

	func testFoldersGiveTheirROMSizedFiles() throws {
		let folder = try folder()
		let create = { (name: String, size: Int) in
			FileManager.default.createFile(atPath: folder.appendingPathComponent(name).path, contents: Data(count: size))
		}
		create("kick31.rom", 512 * 1024)
		create("nested/kick13.rom", 256 * 1024)
		create("rom.key", 1024)
		create("kick.a1200", 512 * 1024)
		create("cart.U1", 64 * 1024)
		create("game.hdf", 3 * 1024 * 1024)
		create("big.rom", 3 * 1024 * 1024)
		create("readme.txt", 100)
		create("disk.adf", 901_120)
		create(".hidden.rom", 512 * 1024)
		let names = ROMImport.files(in: [folder]).map(\.lastPathComponent).sorted()
		XCTAssertEqual(names, ["cart.U1", "kick.a1200", "kick13.rom", "kick31.rom", "rom.key"],
			"what the ROM scan reads, ROM-sized, and the key; not disks, text, hidden or oversized files")
		let single = folder.appendingPathComponent("game.hdf")
		XCTAssertEqual(ROMImport.files(in: [single]), [single], "a file chosen is taken as it is")
	}

	func testEveryKeyInTheROMsFolderIsKept() throws {
		let folder = try folder()
		let roms = URL(fileURLWithPath: try MediaLibrary.path(of: .roms), isDirectory: true)
		try FileManager.default.createDirectory(at: roms, withIntermediateDirectories: true)
		addTeardownBlock {
			for name in ["rom.key", "rom 2.key"] {
				try? FileManager.default.removeItem(at: roms.appendingPathComponent(name))
			}
		}
		// Two Amiga Forever sets' keys: the second is kept beside the first.
		for contents in ["first key", "second key"] {
			let key = folder.appendingPathComponent("rom.key")
			try Data(contents.utf8).write(to: key)
			_ = try AppFiles.importFile(key, into: roms)
		}
		XCTAssertEqual(try String(contentsOf: roms.appendingPathComponent("rom.key"), encoding: .utf8), "first key")
		XCTAssertEqual(try String(contentsOf: roms.appendingPathComponent("rom 2.key"), encoding: .utf8), "second key")
		_ = MediaLibrary.rescanROMs() // both decode (amiberry_media_add_rom_key); nothing to refuse
	}

	func testImportingWhatIsNoROMSaysSo() throws {
		let folder = try folder()
		let file = folder.appendingPathComponent("notarom.bin")
		FileManager.default.createFile(atPath: file.path, contents: Data((0..<524_288).map { UInt8($0 % 251) }))
		let roms = URL(fileURLWithPath: try MediaLibrary.path(of: .roms), isDirectory: true)
		addTeardownBlock {
			try? FileManager.default.removeItem(at: roms.appendingPathComponent("notarom.bin"))
			_ = MediaLibrary.rescanROMs()
		}
		let model = LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in })
		XCTAssertFalse(model.kickstartInstalled, "the tests' home has no ROMs")
		model.importROMs([])
		XCTAssertNil(model.romReport, "cancelled: nothing to say")
		model.importROMs([file])
		XCTAssertEqual(model.romReport, "No new ROMs found in those files.")
		XCTAssertFalse(model.kickstartInstalled)
		XCTAssertTrue(FileManager.default.fileExists(atPath: roms.appendingPathComponent("notarom.bin").path))
	}
}

@MainActor
final class AppSettingsTests: XCTestCase {
	func testAppSettingsCoverEveryHostSetting() throws {
		let model = AppSettingsModel()
		let grouped = AppSettingsModel.groups.map(\.id)
		for setting in HostSettings.all {
			XCTAssertTrue(grouped.contains(setting.group), setting.id)
			XCTAssertNotNil(model.values[setting.id], setting.id)
		}
		// Every group has a title of its own, not its ID.
		for group in AppSettingsModel.groups {
			XCTAssertFalse(group.title.hasPrefix("Global"), group.id)
		}
		let view = NSHostingView(rootView: AppSettingsView(model: model).frame(width: 640, height: 600))
		view.frame = NSRect(x: 0, y: 0, width: 640, height: 600)
		view.layoutSubtreeIfNeeded()
	}

	func testARefusedValueSaysWhy() throws {
		let model = AppSettingsModel()
		let deadzone = try XCTUnwrap(HostSettings.all.first { $0.id == "default_joystick_deadzone" })
		let before = model.value(of: deadzone.id)
		XCTAssertFalse(model.set("101", for: deadzone))
		XCTAssertEqual(model.notice, "Joystick dead zone: \u{201C}101\u{201D} is not a valid value.")
		XCTAssertEqual(model.value(of: deadzone.id), before)
	}
}

@MainActor
final class LibraryTests: XCTestCase {
	func testLibraryListsSavedMachinesWithTheirMetadata() throws {
		let folder = try MediaLibrary.path(of: .configurations)
		let name = "library-\(UUID().uuidString)"
		let file = URL(fileURLWithPath: folder).appendingPathComponent(name + ".uae")
		try "config_description=Lotus II on an A500\nconfig_category=Games\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		let library = LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in })
		let machine = try XCTUnwrap(library.machines.first { $0.name == name })
		XCTAssertEqual(machine.description, "Lotus II on an A500")
		XCTAssertEqual(machine.category, "Games")
		XCTAssertTrue(library.categories.contains("Games"))
		XCTAssertTrue(library.machines(in: "Games").contains(machine))
		XCTAssertFalse(library.machines(in: "Nothing").contains(machine))
		let view = NSHostingView(rootView: LibraryView(model: library))
		view.frame = NSRect(x: 0, y: 0, width: 860, height: 560)
		view.layoutSubtreeIfNeeded()
	}

	func testSaveDoesNotReplaceAnotherMachine() async throws {
		let folder = try MediaLibrary.path(of: .configurations)
		let name = "taken-\(UUID().uuidString)"
		let file = URL(fileURLWithPath: folder).appendingPathComponent(name + ".uae")
		try "config_description=Keep me\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		let saved = await model.save(name: name, description: "Other", category: "")
		XCTAssertFalse(saved)
		XCTAssertTrue(model.notice?.contains("already exists") ?? false, model.notice ?? "")
		XCTAssertEqual(try String(contentsOf: file, encoding: .utf8), "config_description=Keep me\n")
		// Nothing was committed either.
		XCTAssertNotEqual(try SettingsSession.activeValue(of: "config_description"), "Other")
		XCTAssertNotEqual(try SettingsSession().value(of: "config_description"), "Other")
		let slashed = await model.save(name: "a/b", description: "", category: "")
		XCTAssertFalse(slashed)
		XCTAssertTrue(model.notice?.contains("cannot contain") ?? false, model.notice ?? "")
	}

	func testANewMachineShowsItsModelInQuickStart() throws {
		let a1200 = try XCTUnwrap(MachinePresets.preset(for: "a1200"))
		// The pending configuration was saved as a machine: it has that file.
		let saved = FileManager.default.temporaryDirectory.appendingPathComponent("current-\(UUID().uuidString).uae")
		defer { try? FileManager.default.removeItem(at: saved) }
		XCTAssertNotNil(EmulatorActions.post(.saveConfiguration(path: saved.path)))
		ActionPump.runPending()
		XCTAssertEqual(MediaLibrary.currentConfiguration().map { URL(fileURLWithPath: $0).standardizedFileURL.path },
			saved.standardizedFileURL.path)
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true })
		model.startNewMachine(preset: a1200.id, configuration: 0)
		XCTAssertNil(MediaLibrary.currentConfiguration(), "a new machine is no saved file's")
		XCTAssertEqual(model.page, .overview)
		XCTAssertEqual(model.newMachineModel?.id, "a1200")
		XCTAssertEqual(model.store.value(of: "chipset"), "aga")
	}

	func testTheLibraryWaitsWhileAnEditorIsOpen() {
		var opened = 0
		let library = LibraryModel(emulating: false, finish: {}, openEditor: { _, _ in opened += 1 })
		library.editing = true
		library.newMachine(preset: "a500")
		XCTAssertEqual(opened, 0, "the open editor is not replaced")
		library.editing = false
		library.newMachine(preset: "a500")
		XCTAssertEqual(opened, 1)
	}

	func testSavingAMachineInPlaceKeepsItsFile() async throws {
		let folder = try MediaLibrary.path(of: .configurations)
		let name = "Upper-\(UUID().uuidString)"
		let file = URL(fileURLWithPath: folder).appendingPathComponent(name + ".UAE")
		try "config_description=Old\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		let machine = Machine(path: file.path, name: name, description: "Old", category: "")
		let model = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true },
			machine: machine, closeToLibrary: {})
		// The save runs as an action: pump the queue as the emulator would.
		let saving = Task { await model.save(name: name, description: "New", category: "") }
		var saved: Bool?
		for _ in 0..<200 where saved == nil {
			ActionPump.runPending()
			try await Task.sleep(for: .milliseconds(10))
			if model.notice != nil || MediaLibrary.configurationInfo(file.path)?.description == "New" {
				saved = await saving.value
			}
		}
		XCTAssertEqual(saved, true, model.notice ?? "")
		XCTAssertEqual(MediaLibrary.configurationInfo(file.path)?.description, "New")
		// One file, spelled as before.
		let names = try FileManager.default.contentsOfDirectory(atPath: folder).filter { $0.hasPrefix(name) }
		XCTAssertEqual(names, [name + ".UAE"])
	}

	func testClosingAnEditorFromTheLibraryGoesBackToIt() throws {
		var closed = 0
		var dismissed = 0
		let model = try SettingsWindowModel(
			emulating: false, dismiss: { dismissed += 1 }, post: { _ in true },
			machine: nil, closeToLibrary: { closed += 1 })
		XCTAssertTrue(model.fromLibrary)
		model.closeRequested()
		XCTAssertEqual(closed, 1, "back to the Library")
		XCTAssertEqual(dismissed, 0, "nothing quits or starts")
	}
}

@MainActor
final class SettingsWindowModelTests: XCTestCase {
	private var posted: [EmulatorAction] = []
	private var dismissed = 0

	private func model(emulating: Bool) throws -> SettingsWindowModel {
		try SettingsWindowModel(
			emulating: emulating, dismiss: { [weak self] in self?.dismissed += 1 },
			post: { [weak self] in self?.posted.append($0); return true })
	}


	func testStartCommitsAndCloses() throws {
		let model = try model(emulating: false)
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(model.store.set(other, for: "immediate_blits"))
		model.startOrResume()
		XCTAssertEqual(dismissed, 1)
		XCTAssertEqual(posted, [], "nothing runs yet, so nothing to apply")
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), other, "committed")
		XCTAssertTrue(model.store.set(current, for: "immediate_blits"))
		try model.store.commit()
	}

	func testClosingTheWindow() throws {
		try model(emulating: false).closeRequested()
		XCTAssertEqual(posted, [.quit], "closing before anything runs quits")
		posted = []
		try model(emulating: true).closeRequested()
		XCTAssertEqual(posted, [.applySettings(.live)], "closing over an emulation resumes it")
		XCTAssertEqual(dismissed, 2)
	}

	func testUnsavedEditsAreAskedAboutWhenClosingToTheLibrary() throws {
		var closed = 0
		var asked: [String] = []
		var answer = UnsavedChoice.cancel
		let model = try SettingsWindowModel(
			emulating: false, dismiss: { [weak self] in self?.dismissed += 1 }, post: { _ in true },
			machine: nil, closeToLibrary: { closed += 1 })
		model.askAboutUnsavedChanges = { name in
			asked.append(name)
			return answer
		}
		model.closeRequested()
		XCTAssertEqual(asked, [], "nothing edited: nothing to ask")
		XCTAssertEqual(closed, 1)

		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(model.store.set(other, for: "immediate_blits"))
		model.closeRequested()
		XCTAssertEqual(asked, ["New Machine"], "an unsaved machine is named as the window is")
		XCTAssertEqual(closed, 1, "Cancel keeps the editor open")

		answer = .save
		model.closeRequested()
		XCTAssertTrue(model.showingSaveSheet, "a new machine is named before it is saved")
		XCTAssertEqual(closed, 1)
		model.saveSheetFinished(saved: false)
		XCTAssertFalse(model.showingSaveSheet)
		XCTAssertEqual(closed, 1, "a cancelled save keeps the editor open")
		model.saveSheetFinished(saved: true)
		XCTAssertEqual(closed, 1, "a later save from the footer does not close it")
		model.closeRequested()
		model.saveSheetFinished(saved: true)
		XCTAssertEqual(closed, 2, "saved after closing asked for it: back to the Library")

		answer = .discard
		model.closeRequested()
		XCTAssertEqual(closed, 3)
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), current, "discarded edits are not committed")
		XCTAssertEqual(dismissed, 0, "nothing quits or starts")
	}

	func testANewMachineIsTheModelLastPicked() throws {
		let new = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true }, closeToLibrary: {})
		new.startNewMachine(preset: "a1200", configuration: 0)
		XCTAssertEqual(new.store.value(of: "config_description"), "",
			"not the core's default description, which would become the Library card's subtitle")
		new.applyModel("a500", configuration: 1, compatibility: 0)
		XCTAssertEqual(new.newMachineModel?.id, "a500", "the Overview shows the model picked last")
		XCTAssertEqual(new.newMachineModel?.configuration, 1)
		XCTAssertEqual(new.newMachineCompatibility, 0)
		XCTAssertNotEqual(new.store.value(of: "chipset"), "aga", "the A1200 gave way to the A500")
		XCTAssertEqual(new.store.value(of: "config_description"), "")

		let file = FileManager.default.temporaryDirectory.appendingPathComponent("Saved-\(UUID().uuidString).uae")
		let existing = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true },
			machine: Machine(path: file.path, name: "Saved", description: "", category: ""), closeToLibrary: {})
		XCTAssertTrue(existing.store.set("My demo machine", for: "config_description"))
		XCTAssertTrue(existing.store.set("Demos", for: "config_category"))
		existing.applyModel("a500", configuration: 0, compatibility: 0)
		XCTAssertNil(existing.newMachineModel, "a saved machine replaced by a model is still that machine")
		XCTAssertEqual(existing.store.value(of: "chipset"), "ocs")
		XCTAssertEqual(existing.store.value(of: "config_description"), "My demo machine", "the machine keeps its description")
		XCTAssertEqual(existing.store.value(of: "config_category"), "Demos")
	}

	func testANewMachineIsOfferedItsModelsNameNumberedPastSavedOnes() throws {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("names-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: folder) }
		XCTAssertEqual(SettingsWindowModel.name(for: "Amiga 1200", in: folder.path), "Amiga 1200")
		XCTAssertTrue(FileManager.default.createFile(atPath: folder.appendingPathComponent("Amiga 1200.uae").path, contents: Data()))
		XCTAssertEqual(SettingsWindowModel.name(for: "Amiga 1200", in: folder.path), "Amiga 1200 2")
		XCTAssertTrue(FileManager.default.createFile(atPath: folder.appendingPathComponent("Amiga 1200 2.uae").path, contents: Data()))
		XCTAssertEqual(SettingsWindowModel.name(for: "Amiga 1200", in: folder.path), "Amiga 1200 3")
		let arcade = SettingsWindowModel.name(for: "American Laser Games / Picmatic", in: folder.path)
		XCTAssertFalse(arcade.contains("/") || arcade.contains(":"), "a name Save would refuse: \(arcade)")

		let new = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true }, closeToLibrary: {})
		new.startNewMachine(preset: "a1200", configuration: 0)
		XCTAssertTrue(new.suggestedName.hasPrefix(try XCTUnwrap(MachinePresets.preset(for: "a1200")?.label)))
		let saved = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true },
			machine: Machine(path: folder.appendingPathComponent("Mine.uae").path, name: "Mine", description: "", category: ""),
			closeToLibrary: {})
		XCTAssertEqual(saved.suggestedName, "Mine", "a saved machine keeps its name")
	}

	func testStartingASavedMachineWithEditsAsksFirst() throws {
		var dismissed = 0
		let file = FileManager.default.temporaryDirectory.appendingPathComponent("Saved-\(UUID().uuidString).uae")
		let model = try SettingsWindowModel(emulating: false, dismiss: { dismissed += 1 }, post: { _ in true },
			machine: Machine(path: file.path, name: "Saved", description: "", category: ""), closeToLibrary: {})
		var answer = UnsavedChoice.cancel
		var asked: [String] = []
		model.askAboutUnsavedChanges = { name in
			asked.append(name)
			return answer
		}
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(model.store.set(other, for: "immediate_blits"))
		model.startOrResume()
		XCTAssertEqual(asked, ["Saved"])
		XCTAssertEqual(dismissed, 0, "Cancel neither starts nor saves")
		XCTAssertFalse(FileManager.default.fileExists(atPath: file.path))

		answer = .discard
		model.startOrResume()
		XCTAssertEqual(dismissed, 1, "Don't Save starts as edited")
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), other)
		XCTAssertFalse(FileManager.default.fileExists(atPath: file.path), "the file is left as it was")
		let restore = try SettingsSession()
		try restore.setValue(current, for: "immediate_blits")
		try restore.commit()
	}

	func testRevertDropsTheEdits() throws {
		let model = try model(emulating: false)
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		XCTAssertFalse(model.canRevert, "nothing to revert yet")
		XCTAssertTrue(model.store.set(current == "true" ? "false" : "true", for: "immediate_blits"))
		model.invalidDrafts["sound_frequency"] = "abc"
		XCTAssertTrue(model.canRevert)
		model.revert()
		XCTAssertEqual(model.store.value(of: "immediate_blits"), current)
		XCTAssertFalse(model.canRevert)
		XCTAssertEqual(model.invalidDrafts, [:], "refused text goes with the edits")
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), current, "nothing was committed")

		let new = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true }, closeToLibrary: {})
		new.startNewMachine(preset: try XCTUnwrap(MachinePresets.all.first).id, configuration: 0)
		XCTAssertFalse(new.canRevert, "a new machine's model is not an edit to revert")
	}

	func testTheWindowIsTitledByItsMachine() throws {
		let file = FileManager.default.temporaryDirectory.appendingPathComponent("Workbench-\(UUID().uuidString).uae")
		let machine = Machine(path: file.path, name: "Workbench", description: "", category: "")
		XCTAssertEqual(try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true }, machine: machine).title, "Workbench")
		let new = try SettingsWindowModel(emulating: false, dismiss: {}, post: { _ in true }, closeToLibrary: {})
		XCTAssertEqual(new.title, "New Machine")
		XCTAssertFalse(new.isNewMachine, "no model applied yet")
		XCTAssertEqual(try model(emulating: true).title, "Machine Settings", "a running machine with no saved file")
	}

	func testResetAndRestartApplyWithTheirStep() throws {
		let running = try model(emulating: true)
		running.reset()
		running.restart()
		XCTAssertEqual(posted, [.applySettings(.reset), .applySettings(.restart)])
	}

	func testWaitingListsChangesTheRunningEmulationDoesNotUseYet() throws {
		XCTAssertTrue(try model(emulating: false).waiting.isEmpty, "nothing runs")
		let running = try model(emulating: true)
		// Chip RAM only changes at a reset.
		let chip = try XCTUnwrap(SettingsCatalog.descriptor(for: "chipmem_size"))
		XCTAssertEqual(chip.applyClass, .reset)
		let active = try SettingsSession.activeValue(of: chip.id)
		// Earlier tests may have stored chip RAM the emulation does not use yet.
		let waitedBefore = running.waiting[.reset]?.contains { $0.id == chip.id } ?? false
		let other = try XCTUnwrap(running.store.choices(for: chip.id).first {
			$0.value != active && running.store.isChoiceAvailable($0.value, for: chip.id) })
		XCTAssertTrue(running.store.set(other.value, for: chip.id))
		XCTAssertTrue(running.waiting[.reset]?.contains { $0.id == chip.id } ?? false)
		XCTAssertNil(running.waiting[.live], "live options never wait")
		try running.store.revert()
		XCTAssertEqual(running.waiting[.reset]?.contains { $0.id == chip.id } ?? false, waitedBefore)
	}

	func testInvalidFieldsKeepTheWindowOpen() throws {
		let model = try model(emulating: true)
		model.invalidDrafts["cpu_multiplier"] = "abc"
		model.startOrResume()
		model.reset()
		XCTAssertEqual(dismissed, 0)
		XCTAssertEqual(posted, [])
		XCTAssertTrue(model.notice?.hasPrefix("Correct ") ?? false)
		model.invalidDrafts.removeAll()
		model.startOrResume()
		XCTAssertEqual(dismissed, 1)
	}

	func testRefusedPostsKeepTheWindowOpen() throws {
		let model = try SettingsWindowModel(
			emulating: true, dismiss: { [weak self] in self?.dismissed += 1 }, post: { _ in false })
		model.startOrResume()
		model.reset()
		model.quit()
		XCTAssertEqual(dismissed, 0)
		XCTAssertEqual(model.notice, "The emulator did not accept the request. Try again.")
	}

	func testExternalRequestsStoreTheEdits() throws {
		let model = try model(emulating: true)
		let current = try XCTUnwrap(model.store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(model.store.set(other, for: "immediate_blits"))
		model.commitForExternalRequest()
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), other)
		XCTAssertTrue(model.store.set(current, for: "immediate_blits"))
		try model.store.commit()
	}

	func testMountFieldsAreSpelledAsTheWriterSpellsThem() {
		XCTAssertEqual(MountSections.mountField("/Volumes/Work"), "/Volumes/Work")
		XCTAssertEqual(MountSections.mountField("/a,b"), "\"/a,b\"")
		XCTAssertEqual(MountSections.mountField("/a "), "\"/a \"")
		XCTAssertNil(MountSections.mountField("/a\"b"))
	}

	func testMountedPathsWithCommasAreAccepted() throws {
		let created = FileManager.default.temporaryDirectory.appendingPathComponent("a,b \(UUID().uuidString)")
		try FileManager.default.createDirectory(at: created, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: created) }
		let real = try XCTUnwrap(realpath(created.path, nil))
		defer { free(real) }
		let path = String(cString: real)
		let session = try SettingsSession()
		let before = try session.mounts().count
		try session.addMount("dir,rw,DH9:Work:\(try XCTUnwrap(MountSections.mountField(path))),0")
		XCTAssertEqual(try session.mounts().count, before + 1)
		XCTAssertTrue(try session.mounts().last?.contains("a,") ?? false)
	}

	func testErrorsReadAsSentences() {
		XCTAssertEqual(SettingsWindowModel.describe(SettingsError.invalidValue(id: "cpu_model", value: "1")),
			"CPU model: \u{201C}1\u{201D} is not a valid value.")
		XCTAssertTrue(SettingsWindowModel.describe(SettingsError.unavailable("cpu_24bit_addressing"))
			.hasPrefix("24-bit addressing is not available now."))
	}
}

@MainActor
final class MenuCommandsTests: XCTestCase {
	// No setUp (nonisolated before Swift 6.2): XCTest makes an instance per
	// test, so these start fresh for each.
	private lazy var commands: MenuCommands = {
		let commands = MenuCommands()
		commands.post = { [weak self] in self?.posted.append($0) }
		commands.runState = { [weak self] in self?.state ?? .stopped }
		commands.settingsOpen = { [weak self] in self?.settingsOpen ?? false }
		commands.status = { [weak self] in self?.drives ?? EmulationChromeTests.status(disk: "") }
		return commands
	}()
	private var posted: [EmulatorAction] = []
	private var state = EmulatorStatus.RunState.running
	private var settingsOpen = false
	/// The drives' status; none holds media when nil.
	private var drives: EmulatorStatus?


	private func menu() -> NSMenu {
		let main = NSMenu(title: "Main")
		let app = NSMenuItem(title: "Amiberry", action: nil, keyEquivalent: "")
		app.submenu = NSMenu(title: "Amiberry")
		app.submenu?.addItem(withTitle: "About Amiberry", action: nil, keyEquivalent: "")
		app.submenu?.addItem(.separator())
		app.submenu?.addItem(withTitle: "Quit Amiberry", action: nil, keyEquivalent: "q")
		main.addItem(app)
		commands.install(into: main)
		return main
	}

	private func item(_ main: NSMenu, _ menu: String, _ title: String) throws -> NSMenuItem {
		let submenu = try XCTUnwrap(main.items.first { $0.title == menu }?.submenu, menu)
		return try XCTUnwrap(submenu.items.first { $0.title == title }, title)
	}

	private func run(_ item: NSMenuItem) {
		_ = (item.target as? NSObject)?.perform(item.action, with: item)
	}

	func testInstallsOnceWithControlCommandShortcuts() throws {
		let main = menu()
		let before = main.items.count
		commands.install(into: main)
		XCTAssertEqual(main.items.count, before, "installed once")
		XCTAssertEqual(main.items.map(\.title), ["Amiberry", "File", "Edit", "Emulation", "Media", "View", "Help"])
		let settings = try item(main, "Amiberry", "Settings\u{2026}")
		XCTAssertEqual(settings.keyEquivalent, ",")
		// Command alone is the Amiga key: while an emulation window is key,
		// no menu item may take an Amiga-letter combination from it. The
		// plain-⌘ shortcuts (Settings, Edit's, Quit) are taken off then.
		commands.commandShortcuts(on: false, in: main)
		var entries = main.items
		while let entry = entries.popLast() {
			entries += entry.submenu?.items ?? []
			guard !entry.keyEquivalent.isEmpty else { continue }
			XCTAssertEqual(entry.keyEquivalentModifierMask.intersection([.control, .command]), [.control, .command], entry.title)
		}
		commands.commandShortcuts(on: true, in: main)
		let paste = try item(main, "Edit", "Paste")
		XCTAssertEqual(paste.keyEquivalent, "v", "a text window gets the usual editing shortcuts")
		XCTAssertEqual(paste.keyEquivalentModifierMask, [.command])
		XCTAssertEqual(try item(main, "Edit", "Redo").keyEquivalentModifierMask, [.command, .shift])
		XCTAssertEqual(settings.keyEquivalent, ",")
		XCTAssertEqual(try item(main, "Amiberry", "Quit Amiberry").keyEquivalent, "q")
	}

	func testReusesTheSettingsItemSDLCreated() throws {
		let main = NSMenu(title: "Main")
		let app = NSMenuItem(title: "Amiberry", action: nil, keyEquivalent: "")
		app.submenu = NSMenu(title: "Amiberry")
		app.submenu?.addItem(withTitle: "About Amiberry", action: nil, keyEquivalent: "")
		app.submenu?.addItem(.separator())
		app.submenu?.addItem(withTitle: "Preferences\u{2026}", action: nil, keyEquivalent: ",")
		main.addItem(app)
		commands.install(into: main)
		let commaItems = try XCTUnwrap(app.submenu).items.filter { $0.keyEquivalent == "," }
		XCTAssertEqual(commaItems.count, 1, "one Settings item")
		XCTAssertEqual(commaItems.first?.title, "Settings\u{2026}")
		// Cmd-, is the app settings; a machine's are in Emulation.
		XCTAssertEqual(commaItems.first?.action, #selector(MenuCommands.openAppSettings(_:)))
	}

	func testCommandsPostTheirActions() throws {
		let main = menu()
		run(try item(main, "Emulation", "Machine Settings\u{2026}"))
		run(try item(main, "Emulation", "Reset"))
		run(try item(main, "Emulation", "Keyboard Reset"))
		run(try item(main, "Emulation", "Hard Reset"))
		run(try item(main, "Emulation", "Quick Save State"))
		run(try item(main, "Emulation", "Save State\u{2026}"))
		run(try item(main, "Media", "Insert Disk in DF1:\u{2026}"))
		run(try item(main, "Media", "Eject DF2:"))
		run(try item(main, "Media", "Eject CD"))
		run(try item(main, "View", "Enter Full Screen"))
		run(try item(main, "Emulation", "Load State\u{2026}"))
		run(try item(main, "Media", "Insert CD\u{2026}"))
		XCTAssertEqual(posted, [
			.openSettings, .reset(hard: false, keyboard: false), .reset(hard: false, keyboard: true),
			.reset(hard: true, keyboard: false),
			.saveStateSlot(0), .chooseMedia(.saveState), .chooseMedia(.floppy(drive: 1)),
			.ejectFloppy(drive: 2), .ejectCD, .toggleFullscreen, .chooseMedia(.loadState), .chooseMedia(.cd),
		])
	}

	func testScreenshotNamesNeverOverwrite() throws {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("shots-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: folder) }
		let now = Date()
		let first = MenuCommands.screenshotPath(in: folder.path, at: now)
		FileManager.default.createFile(atPath: first, contents: Data())
		let second = MenuCommands.screenshotPath(in: folder.path, at: now)
		XCTAssertNotEqual(first, second)
		XCTAssertTrue(second.hasSuffix(" 2.png"))
	}

	func testPauseFollowsTheRunState() throws {
		let main = menu()
		let pause = try item(main, "Emulation", "Pause")
		XCTAssertTrue(commands.validateMenuItem(pause))
		run(pause)
		state = .paused
		XCTAssertTrue(commands.validateMenuItem(pause))
		XCTAssertEqual(pause.title, "Resume")
		run(pause)
		XCTAssertEqual(posted, [.pause, .resume])
		state = .stopped
		XCTAssertFalse(commands.validateMenuItem(pause))
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Emulation", "Reset")))
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Media", "Insert CD\u{2026}")))
		drives = EmulationChromeTests.status(disk: "", cd: 0, cdPath: "/cds/Demo.cue")
		XCTAssertTrue(commands.validateMenuItem(try item(main, "Media", "Eject CD")), "ejecting needs no running emulation")
		// The settings window has the media while it is up.
		state = .paused
		settingsOpen = true
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Media", "Insert CD\u{2026}")))
		XCTAssertFalse(commands.validateMenuItem(try item(main, "Emulation", "Save State\u{2026}")))
		XCTAssertTrue(commands.validateMenuItem(try item(main, "Emulation", "Reset")))
		XCTAssertFalse(commands.validateMenuItem(pause), "the settings window holds the pause")
	}

	func testTheMediaMenuFollowsTheDrives() throws {
		let main = menu()
		func enabled(_ title: String) throws -> Bool {
			commands.validateMenuItem(try item(main, "Media", title))
		}
		drives = EmulationChromeTests.status(disk: "/disks/Lotus.adf", disabled: [2, 3])
		XCTAssertTrue(try enabled("Eject DF0:"))
		XCTAssertFalse(try enabled("Eject DF1:"), "nothing to eject from an empty drive")
		XCTAssertFalse(try enabled("Eject DF2:"), "a disabled drive")
		XCTAssertTrue(try enabled("Insert Disk in DF1:\u{2026}"))
		XCTAssertFalse(try enabled("Insert Disk in DF3:\u{2026}"), "a disabled drive takes no disk")
		XCTAssertFalse(try enabled("Eject CD"), "no CD in the drive")
		drives = EmulationChromeTests.status(disk: "", cd: 0, cdPath: "/cds/Demo.cue")
		XCTAssertTrue(try enabled("Eject CD"))
		XCTAssertFalse(try enabled("Eject DF0:"))
		XCTAssertTrue(try enabled("Insert Disk in DF3:\u{2026}"))
	}

	func testKeyboardResetNeedsARunningEmulation() throws {
		let main = menu()
		let keyboardReset = try item(main, "Emulation", "Keyboard Reset")
		XCTAssertEqual(keyboardReset.keyEquivalent, "", "no shortcut")
		XCTAssertTrue(commands.validateMenuItem(keyboardReset))
		state = .stopped
		XCTAssertFalse(commands.validateMenuItem(keyboardReset))
	}
}
