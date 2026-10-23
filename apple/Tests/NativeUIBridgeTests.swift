/*
 * Copyright (C) 2026 BlitterStudio
 * Tests of the Swift bridge against the real settings layer: the bundle is
 * loaded into the emulator by --native-ui-tests (tests/test_native_ui_bridge.sh)
 * where the built-in GUI would open, so sessions, presets, actions and media
 * run on a fully started emulator with no emulation running.
 */

import AmiberryNativeUI
import Foundation
import XCTest

@MainActor
final class SettingsCatalogTests: XCTestCase {
	func testDescriptorsMirrorTheLayer() throws {
		let descriptors = SettingsCatalog.descriptors
		XCTAssertFalse(descriptors.isEmpty)
		XCTAssertEqual(Set(descriptors.map(\.id)).count, descriptors.count, "option IDs are unique")

		let cpu = try XCTUnwrap(SettingsCatalog.descriptor(for: "cpu_model"))
		XCTAssertEqual(cpu.kind, .choice)
		XCTAssertEqual(cpu.group, "cpu")
		XCTAssertTrue(cpu.choices.contains { $0.value == "68000" })
		XCTAssertTrue(cpu.choices.contains { $0.value == "68060" })

		let addressing = try XCTUnwrap(SettingsCatalog.descriptor(for: "cpu_24bit_addressing"))
		XCTAssertEqual(addressing.kind, .bool)
		XCTAssertEqual(addressing.applyClass, .reset)
		XCTAssertNotNil(addressing.constraint)

		XCTAssertNil(SettingsCatalog.descriptor(for: "not_an_option"))
		XCTAssertTrue(SettingsCatalog.groups.contains("cpu"))
		XCTAssertTrue(SettingsCatalog.descriptors(in: "fpu").contains { $0.id == "fpu_model" })
		for descriptor in descriptors where descriptor.kind == .integer || descriptor.kind == .real {
			XCTAssertNotNil(descriptor.range, descriptor.id)
		}
	}

	func testPresetsMirrorTheLayer() throws {
		let a500 = try XCTUnwrap(MachinePresets.preset(for: "a500"))
		XCTAssertFalse(a500.configurations.isEmpty)
		XCTAssertFalse(MachinePresets.compatibilityLevels.isEmpty)
		XCTAssertTrue(MachinePresets.all.allSatisfy { !$0.configurations.isEmpty })
		XCTAssertTrue(MachinePresets.all.allSatisfy { $0.compatibilityMax < MachinePresets.compatibilityLevels.count })
		// The test runs on an empty ROM folder.
		XCTAssertFalse(MachinePresets.isInstalled(a500, configuration: 0))
	}
}

@MainActor
final class SettingsSessionTests: XCTestCase {
	func testValuesRoundTripWithDependentChanges() throws {
		let session = try SettingsSession()
		try session.setValue("68030", for: "cpu_model")
		XCTAssertEqual(try session.value(of: "cpu_model"), "68030")
		// A 68040 selects its internal FPU, as the ImGui CPU panel does.
		try session.setValue("68040", for: "cpu_model")
		XCTAssertEqual(try session.value(of: "fpu_model"), "68040")
		XCTAssertFalse(session.isChoiceAvailable("true", for: "cpu_24bit_addressing"))
		XCTAssertThrowsError(try session.setValue("true", for: "cpu_24bit_addressing")) {
			XCTAssertEqual($0 as? SettingsError, .unavailable("cpu_24bit_addressing"))
		}
	}

	func testRefusalsAreTyped() throws {
		let session = try SettingsSession()
		XCTAssertThrowsError(try session.setValue("68999", for: "cpu_model")) {
			XCTAssertEqual($0 as? SettingsError, .invalidValue(id: "cpu_model", value: "68999"))
		}
		XCTAssertThrowsError(try session.value(of: "not_an_option")) {
			XCTAssertEqual($0 as? SettingsError, .unknownID("not_an_option"))
		}
		XCTAssertThrowsError(try session.applyPreset("not_a_preset")) {
			XCTAssertEqual($0 as? SettingsError, .unknownID("not_a_preset"))
		}
		session.close()
		XCTAssertFalse(session.isOpen)
		XCTAssertThrowsError(try session.value(of: "cpu_model"))
	}

	func testRangesAndHostChoices() throws {
		let session = try SettingsSession()
		let integer = try XCTUnwrap(SettingsCatalog.descriptors.first { $0.kind == .integer })
		let range = try XCTUnwrap(session.range(for: integer.id))
		XCTAssertTrue(integer.range.map { $0.lowerBound <= range.lowerBound && range.upperBound <= $0.upperBound } ?? false)
		XCTAssertNil(session.range(for: "cpu_model"))
		XCTAssertEqual(session.choices(for: "cpu_model").map(\.value),
			SettingsCatalog.descriptor(for: "cpu_model")?.choices.map(\.value))
		SettingsSession.refreshHostChoices()
	}

	func testValidationAndCommitSeeChangesMadeElsewhere() throws {
		let reader = try SettingsSession()
		try reader.setValue("68020", for: "cpu_model")
		try reader.setValue("68881", for: "fpu_model")
		try reader.commit()

		// Another session switches to a 68040 and stores it.
		let other = try SettingsSession()
		try other.setValue("68040", for: "cpu_model")
		let stored = try other.commit()
		XCTAssertTrue(stored.changed)

		// This session's 68881 now meets a 68040, which uses its internal FPU.
		try reader.setValue("68882", for: "fpu_model")
		let adjustment = try XCTUnwrap(reader.validate().first { $0.optionID == "fpu_model" })
		XCTAssertEqual(adjustment.requested, "68882")
		XCTAssertEqual(adjustment.adjusted, "68040")
		XCTAssertNotNil(adjustment.reason)

		// Committed as validated: the adjusted FPU is the one already stored,
		// and the session continues from the stored configuration.
		let result = try reader.commit()
		XCTAssertFalse(result.changed)
		XCTAssertEqual(result.applyClass, .live)
		XCTAssertEqual(try reader.value(of: "fpu_model"), "68040")
		XCTAssertEqual(try reader.value(of: "cpu_model"), "68040")
		XCTAssertTrue(reader.validate().isEmpty)
	}

	func testPresetCommitsAtLeastAReset() throws {
		let session = try SettingsSession()
		try session.applyPreset("a1200")
		XCTAssertEqual(try session.value(of: "chipset"), "aga")
		XCTAssertEqual(try session.value(of: "cpu_model"), "68020")
		let result = try session.commit()
		XCTAssertTrue(result.changed)
		XCTAssertGreaterThanOrEqual(result.applyClass, .reset)
		XCTAssertThrowsError(try session.applyPreset("a1200", configuration: 99)) {
			XCTAssertEqual($0 as? SettingsError, .invalidValue(id: "a1200", value: "99:1"))
		}
	}

	func testMountsAreEditedInTheSession() throws {
		let created = FileManager.default.temporaryDirectory.appendingPathComponent("bridge-mount-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: created, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: created) }
		// The layer stores the folder's real path (/private/var, not /var);
		// Foundation's symlink resolution would strip /private again.
		let real = try XCTUnwrap(realpath(created.path, nil))
		defer { free(real) }
		let folder = URL(fileURLWithPath: String(cString: real))

		let session = try SettingsSession()
		let before = try session.mounts()
		let entry = "dir,rw,DH7:Work:\(folder.path),0"
		try session.addMount(entry)
		XCTAssertEqual(try session.mounts(), before + [entry])
		XCTAssertThrowsError(try session.addMount(entry), "a device name another entry uses")
		XCTAssertThrowsError(try session.addMount("not an entry")) {
			XCTAssertEqual($0 as? SettingsError, .invalidValue(id: "mount", value: "not an entry"))
		}
		let readOnly = "dir,ro,DH7:Work:\(folder.path),0"
		try session.replaceMount(at: before.count, with: readOnly)
		XCTAssertEqual(try session.mounts().last, readOnly)
		try session.removeMount(at: before.count)
		XCTAssertEqual(try session.mounts(), before)
		XCTAssertThrowsError(try session.removeMount(at: before.count)) {
			XCTAssertEqual($0 as? SettingsError, .unknownID("mount \(before.count)"))
		}
	}

	func testBoardTypesDescribeTheirFields() throws {
		XCTAssertFalse(BoardCatalog.types.isEmpty)
		XCTAssertNil(BoardCatalog.type("cpuboard"), "the accelerator is not an expansion board")
		let a2091 = try XCTUnwrap(BoardCatalog.type("a2091"))
		XCTAssertTrue(a2091.categories.contains(.scsi))
		XCTAssertGreaterThan(a2091.units, 1)
		XCTAssertEqual(a2091.fields.first?.id, "enabled")
		XCTAssertTrue(a2091.fields.contains { $0.id == "rom_file" && $0.kind == .path })
		XCTAssertTrue(a2091.fields.allSatisfy { $0.applyClass == .restart })
	}

	func testBoardUnitsAreEditedInTheSession() throws {
		let session = try SettingsSession()
		XCTAssertEqual(try session.boardValue("enabled", type: "a2091", unit: 2), "false")
		XCTAssertFalse(session.isBoardFieldAvailable("rom_file", type: "a2091", unit: 2))
		try session.setBoardValue("true", field: "enabled", type: "a2091", unit: 2)
		XCTAssertEqual(try session.boardValue("enabled", type: "a2091", unit: 2), "true")
		XCTAssertTrue(session.isBoardFieldAvailable("rom_file", type: "a2091", unit: 2))
		try session.setBoardValue("/roms/a2091.rom", field: "rom_file", type: "a2091", unit: 2)
		XCTAssertEqual(try session.boardValue("rom_file", type: "a2091", unit: 2), "/roms/a2091.rom")
		XCTAssertThrowsError(try session.setBoardValue("nope", field: "subtype", type: "a2091", unit: 2)) {
			XCTAssertEqual($0 as? SettingsError, .invalidValue(id: "a2091 2 subtype", value: "nope"))
		}
		XCTAssertThrowsError(try session.boardValue("enabled", type: "a2091", unit: 99)) {
			XCTAssertEqual($0 as? SettingsError, .unknownID("a2091 99 enabled"))
		}
	}
}

@MainActor
final class SettingsStoreTests: XCTestCase {
	func testStoreMirrorsTheSession() throws {
		let store = try SettingsStore()
		XCTAssertFalse(store.hasChanges)
		XCTAssertEqual(Set(store.values.keys), Set(SettingsCatalog.descriptors.map(\.id)), "every option reads")
		let original = try XCTUnwrap(store.value(of: "cpu_model"))
		let target = original == "68040" ? "68030" : "68040"

		let revision = store.revision
		XCTAssertTrue(store.set(target, for: "cpu_model"))
		XCTAssertGreaterThan(store.revision, revision)
		XCTAssertEqual(store.value(of: "cpu_model"), target)
		XCTAssertTrue(store.hasChanges)
		XCTAssertNil(store.lastError)
		if target == "68040" {
			XCTAssertEqual(store.value(of: "fpu_model"), "68040")
		}

		XCTAssertFalse(store.set("68999", for: "cpu_model"))
		XCTAssertEqual(store.lastError, .invalidValue(id: "cpu_model", value: "68999"))
		XCTAssertEqual(store.value(of: "cpu_model"), target)

		try store.revert()
		XCTAssertFalse(store.hasChanges)
		XCTAssertNil(store.lastError)
		XCTAssertEqual(store.value(of: "cpu_model"), original)

		// Setting a value back to what was stored is no change.
		XCTAssertTrue(store.set(target, for: "cpu_model"))
		XCTAssertTrue(store.set(original, for: "cpu_model"))
		XCTAssertFalse(store.hasChanges)

		XCTAssertTrue(store.applyPreset("a500"))
		XCTAssertTrue(store.hasChanges)
		XCTAssertEqual(store.value(of: "chipset"), "ocs")
		// The A4000 has only the most compatible level; the default uses it.
		XCTAssertEqual(MachinePresets.defaultCompatibility(for: "a4000"), 0)
		XCTAssertTrue(store.applyPreset("a4000"), "\(String(describing: store.lastError))")
		XCTAssertEqual(store.value(of: "chipset"), "aga")
		XCTAssertFalse(store.applyPreset("a4000", compatibility: 1))
		XCTAssertEqual(store.lastError, .unavailable("a4000"))
	}

	func testApplyCommitsAndBringsTheEmulatorUpToIt() async throws {
		let pump = ActionPump()
		pump.start(interval: 0.005)
		defer { pump.stop() }

		let store = try SettingsStore()
		let current = try XCTUnwrap(store.value(of: "immediate_blits"))
		XCTAssertTrue(store.set(current == "true" ? "false" : "true", for: "immediate_blits"))
		let applied = try await store.apply()
		XCTAssertEqual(applied, .live)
		XCTAssertFalse(store.hasChanges)
		XCTAssertNil(store.pendingApply)
		// No emulation runs, so the pending configuration is what starts next.
		XCTAssertEqual(try SettingsSession().value(of: "immediate_blits"), current == "true" ? "false" : "true")
		let nothing = try await store.apply()
		XCTAssertNil(nothing)
	}

	func testAnInterruptedApplyIsRetried() async throws {
		let store = try SettingsStore()
		let current = try XCTUnwrap(store.value(of: "immediate_blits"))
		XCTAssertTrue(store.set(current == "true" ? "false" : "true", for: "immediate_blits"))
		// Without a pump nothing runs the action; cancel the wait.
		let attempt = Task { try await store.apply() }
		try await Task.sleep(for: .milliseconds(50))
		attempt.cancel()
		do {
			_ = try await attempt.value
			XCTFail("the cancelled apply finished")
		} catch is CancellationError {
		}
		XCTAssertFalse(store.hasChanges, "committed")
		XCTAssertEqual(store.pendingApply, .live, "but not applied")

		let pump = ActionPump()
		pump.start(interval: 0.005)
		defer { pump.stop() }
		let applied = try await store.apply()
		XCTAssertEqual(applied, .live)
		XCTAssertNil(store.pendingApply)
	}

	func testChangesCommittedDuringAnApplyStayPending() async throws {
		let store = try SettingsStore()
		let current = try XCTUnwrap(store.value(of: "immediate_blits"))
		let other = current == "true" ? "false" : "true"
		XCTAssertTrue(store.set(other, for: "immediate_blits"))
		// Nothing runs the action until the pump starts.
		let first = Task { try await store.apply() }
		try await Task.sleep(for: .milliseconds(20))
		XCTAssertTrue(store.set(current, for: "immediate_blits"))
		try store.commit()

		let pump = ActionPump()
		pump.start(interval: 0.005)
		defer { pump.stop() }
		_ = try await first.value
		XCTAssertEqual(store.pendingApply, .live, "the later commit is not cleared by the earlier apply")
		_ = try await store.apply()
		XCTAssertNil(store.pendingApply)
	}
}

@MainActor
final class EmulatorActionsTests: XCTestCase {
	func testRejectedActionsNeverRun() {
		XCTAssertNil(EmulatorActions.post(.insertFloppy(drive: 9, path: "")))
		XCTAssertNil(EmulatorActions.post(.saveStateSlot(10)))
		XCTAssertNil(EmulatorActions.post(.insertSwapperEntry(index: EmulatorActions.swapperEntryCount, drive: 0)))
		XCTAssertNil(EmulatorActions.post(.chooseMedia(.floppy(drive: 4))))
		XCTAssertNil(EmulatorActions.post(.key(code: 0x80, pressed: true)))
		XCTAssertNil(EmulatorActions.post(.key(code: -1, pressed: false)))
	}

	/// The tests run where the settings UI would show, before any emulation:
	/// a key press has nothing to reach, a release always goes through.
	func testKeyPressesNeedARunningEmulation() throws {
		let press = try XCTUnwrap(EmulatorActions.post(.key(code: 0x45, pressed: true)))
		let release = try XCTUnwrap(EmulatorActions.post(.key(code: 0x45, pressed: false)))
		ActionPump.runPending()
		XCTAssertEqual(press.poll(), .failed("No emulation is running"))
		XCTAssertEqual(release.poll(), .done)
	}

	func testPostedActionsRunOnTheMainThreadInOrder() throws {
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("bridge-actions-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: folder) }
		let disk = folder.appendingPathComponent("disk.adf").path
		FileManager.default.createFile(atPath: disk, contents: Data(count: 901_120))

		let set = try XCTUnwrap(EmulatorActions.post(.setSwapperEntry(index: 3, path: disk)))
		let clear = try XCTUnwrap(EmulatorActions.post(.setSwapperEntry(index: 3, path: "")))
		XCTAssertEqual(set.poll(), .pending)
		let revision = EmulatorStatus.current().revision
		XCTAssertGreaterThanOrEqual(ActionPump.runPending(), 2)
		XCTAssertEqual(set.poll(), .done)
		XCTAssertEqual(set.poll(), .unknown, "a result is taken once")
		XCTAssertEqual(clear.poll(), .done)
		XCTAssertEqual(EmulatorActions.swapperEntry(3), "")
		XCTAssertNotEqual(EmulatorStatus.current().revision, revision)
	}

	func testPerformWaitsForThePump() async throws {
		let pump = ActionPump()
		pump.start(interval: 0.005)
		defer { pump.stop() }
		// No emulation runs: applying settings is done at once, and there is
		// no emulation to open settings or a requester over.
		try await EmulatorActions.perform(.applySettings(.reset))
		try await EmulatorActions.perform(.openSettings)
		try await EmulatorActions.perform(.chooseMedia(.cd))
		do {
			try await EmulatorActions.perform(.loadState(path: "/nonexistent/state.uss"))
			XCTFail("loading a missing state file failed")
		} catch let ActionError.failed(action, _) {
			XCTAssertEqual(action, .loadState(path: "/nonexistent/state.uss"))
		}
		do {
			try await EmulatorActions.perform(.insertFloppy(drive: 9, path: ""))
			XCTFail("an out-of-range drive is rejected")
		} catch let ActionError.rejected(action) {
			XCTAssertEqual(action, .insertFloppy(drive: 9, path: ""))
		}
	}

	func testStatusReportsAStoppedEmulator() {
		let status = EmulatorStatus.current()
		XCTAssertEqual(status.runState, .stopped)
		XCTAssertEqual(status.floppies.count, EmulatorActions.floppyDriveCount)

		let monitor = StatusMonitor()
		XCTAssertEqual(monitor.status.runState, .stopped)
		let before = monitor.status
		ActionPump.runPending()
		XCTAssertNotEqual(EmulatorStatus.current().revision, before.revision)
		monitor.refresh()
		XCTAssertEqual(monitor.status, before, "a new revision alone is not a change")
		XCTAssertTrue(monitor.status.hasSameState(as: EmulatorStatus.current()))
	}
}

@MainActor
final class MediaLibraryTests: XCTestCase {
	func testFoldersRoundTrip() throws {
		let original = try MediaLibrary.path(of: .floppies)
		XCTAssertFalse(original.isEmpty)
		let folder = FileManager.default.temporaryDirectory.appendingPathComponent("bridge-floppies-\(UUID().uuidString)")
		try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		defer {
			try? MediaLibrary.setPath(original, for: .floppies)
			try? FileManager.default.removeItem(at: folder)
		}
		try MediaLibrary.setPath(folder.path, for: .floppies)
		XCTAssertTrue(try MediaLibrary.path(of: .floppies).hasPrefix(folder.path))
		XCTAssertThrowsError(try MediaLibrary.setPath("", for: .floppies)) {
			XCTAssertEqual($0 as? SettingsError, .invalidValue(id: "floppies", value: ""))
		}
	}

	func testListsAndRecents() throws {
		let configurations = try MediaLibrary.path(of: .configurations)
		let name = "bridge-\(UUID().uuidString).uae"
		let file = URL(fileURLWithPath: configurations).appendingPathComponent(name)
		try "; bridge test\n".write(to: file, atomically: true, encoding: .utf8)
		defer { try? FileManager.default.removeItem(at: file) }
		XCTAssertTrue(MediaLibrary.entries(.configurations).contains { $0.name.hasPrefix("bridge-") && $0.path.hasSuffix(name) })

		let disk = FileManager.default.temporaryDirectory.appendingPathComponent("bridge-\(UUID().uuidString).adf")
		FileManager.default.createFile(atPath: disk.path, contents: Data(count: 901_120))
		defer { try? FileManager.default.removeItem(at: disk) }
		try MediaLibrary.addRecent(disk.path, to: .recentFloppies)
		XCTAssertEqual(MediaLibrary.entries(.recentFloppies).first?.path, disk.path)
		XCTAssertThrowsError(try MediaLibrary.addRecent(disk.path, to: .configurations))
		XCTAssertTrue(MediaLibrary.grantAccess(to: disk.path))
	}
}
