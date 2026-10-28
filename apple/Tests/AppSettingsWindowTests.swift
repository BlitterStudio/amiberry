/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the Mac Settings window's sidebar against the real layer.
 */

import AmiberrySettings
import AppKit
@testable import AmiberryNativeUI
import XCTest

@MainActor
final class AppSettingsWindowTests: XCTestCase {
	/// A misspelled symbol name draws nothing in the sidebar.
	func testEverySidebarSymbolExists() {
		for group in AppSettingsModel.groups {
			XCTAssertNotNil(NSImage(systemSymbolName: group.symbol, accessibilityDescription: nil),
				"\(group.id): \(group.symbol) is not an SF Symbol")
		}
		XCTAssertNotNil(NSImage(systemSymbolName: AppSettingsModel.fallbackSymbol, accessibilityDescription: nil))
	}

	/// What the remove button offers must be a path the layer has and lets
	/// go of: removing it leaves it empty.
	func testOptionalPathsCanBeRemoved() throws {
		let model = AppSettingsModel()
		for id in HostSettingRow.optionalPaths {
			let setting = try XCTUnwrap(HostSettings.all.first { $0.id == id }, id)
			XCTAssertEqual(setting.kind, .path, id)
			let before = model.value(of: id)
			XCTAssertTrue(model.set("", for: setting), "\(id): \(model.notice ?? "")")
			XCTAssertEqual(model.value(of: id), "", id)
			if !before.isEmpty {
				XCTAssertTrue(model.set(before, for: setting), id)
			}
		}
	}

	func testSearchFindsSettingsByEveryWord() {
		let all = AppSettingsModel.groups.flatMap { $0.settings(matching: "") }
		XCTAssertEqual(all.count, HostSettings.all.count, "an empty search shows everything")
		let found = AppSettingsModel.groups.flatMap { $0.settings(matching: "JOYSTICK dead") }.map(\.id)
		XCTAssertEqual(found, ["default_joystick_deadzone"], "every word, any case, in label, help or key")
		XCTAssertTrue(AppSettingsModel.groups.allSatisfy { $0.settings(matching: "nothing-is-called-this").isEmpty })
	}

	/// A joystick the settings name that is not connected now is offered
	/// first, under the layer's name for it, not its token.
	func testStoredDeviceOutsideTheHostIsOfferedAsNotConnected() throws {
		let model = AppSettingsModel()
		let setting = try XCTUnwrap(HostSettings.all.first { $0.id == "default_controller1" })
		let before = model.value(of: setting.id)
		defer { model.set(before, for: setting) }
		XCTAssertTrue(model.set("joy31", for: setting), model.notice ?? "")
		let offered = model.offeredChoices(for: setting)
		XCTAssertEqual(offered.first?.value, "joy31")
		XCTAssertEqual(offered.first?.label, "Joystick 32 (not connected)")
		XCTAssertEqual(offered.filter { $0.value == "joy31" }.count, 1, "listed once")
	}

	/// The layer's list is shown as it is; only a value it does not list (a
	/// fixed list's stray value from an edited file) is added, by its value.
	func testOfferedAddsOnlyAnUnlistedValue() {
		let list = [SettingChoice(value: "0", label: "Windowed"), SettingChoice(value: "2", label: "Full-window")]
		XCTAssertEqual(AppSettingsModel.offered(list, current: "2"), list)
		XCTAssertEqual(AppSettingsModel.offered(list, current: "7").first, SettingChoice(value: "7", label: "7"))
	}
}
