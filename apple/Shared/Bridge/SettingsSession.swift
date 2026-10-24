/*
 * Copyright (C) 2026 BlitterStudio
 * An edit session of the settings layer: a private copy of the pending
 * configuration that the emulator applies next. The layer runs every
 * session call on the emulator's main thread, which on macOS and iOS is
 * the UI's main thread, so the session is isolated to the main actor.
 */

import AmiberrySettings
import Foundation

/// A change the emulator's checks would make to a value the user picked.
public struct SettingAdjustment: Hashable, Sendable, Identifiable {
	public let optionID: String
	public let requested: String
	public let adjusted: String
	/// The option's constraint text, when it has one.
	public let reason: String?
	public var id: String { optionID }
}

/// What `SettingsSession.commit()` stored.
public struct CommitResult: Hashable, Sendable {
	/// Whether the pending configuration changed.
	public let changed: Bool
	/// The strongest apply class among the changed options: the step that
	/// brings a running emulation up to the stored configuration
	/// (`EmulatorAction.applySettings`).
	public let applyClass: ApplyClass
}

/// The session handle. Only the main actor uses it; the wrapper lets the
/// (nonisolated) deinit hand it back there to be closed.
private struct SessionHandle: @unchecked Sendable {
	let pointer: OpaquePointer
}

@MainActor
public final class SettingsSession {
	private var storage: SessionHandle?

	private var handle: OpaquePointer? { storage?.pointer }

	/// Opens a session on the current pending configuration.
	public init() throws {
		guard let session = amiberry_settings_session_open() else {
			throw SettingsError.outOfMemory
		}
		storage = SessionHandle(pointer: session)
	}

	/// Closes the session; later calls fail as for an unknown option.
	/// Closing is also done when the session is released.
	public func close() {
		if let handle {
			amiberry_settings_session_close(handle)
			storage = nil
		}
	}

	deinit {
		guard let session = storage else { return }
		if Thread.isMainThread {
			amiberry_settings_session_close(session.pointer)
		} else {
			DispatchQueue.main.async { amiberry_settings_session_close(session.pointer) }
		}
	}

	public var isOpen: Bool { storage != nil }

	private func session(_ id: String) throws -> OpaquePointer {
		guard let handle else { throw SettingsError.unknownID(id) }
		return handle
	}

	/// The option's current value in .uae spelling. A choice option can
	/// hold a value outside its list when a configuration file set one.
	public func value(of id: String) throws -> String {
		let session = try session(id)
		let (text, status) = readLayerString { amiberry_settings_get(session, id, $0, $1) }
		try SettingsError.check(status, id: id)
		return text
	}

	/// Sets an option, with the dependent changes the ImGui GUI makes (for
	/// example a 68040 selects its internal FPU).
	public func setValue(_ value: String, for id: String) throws {
		let session = try session(id)
		try SettingsError.check(amiberry_settings_set(session, id, value), id: id, value: value)
	}

	public func isAvailable(_ id: String) -> Bool {
		guard let handle else { return false }
		return amiberry_settings_is_available(handle, id)
	}

	/// Whether a choice or bool option accepts this value now ("true" or
	/// "false" for a bool); a front end greys out the others.
	public func isChoiceAvailable(_ value: String, for id: String) -> Bool {
		guard let handle else { return false }
		return amiberry_settings_choice_is_available(handle, id, value)
	}

	/// The range an integer or real option accepts now.
	public func range(for id: String) -> ClosedRange<Double>? {
		guard let handle else { return nil }
		var low = 0.0
		var high = 0.0
		guard amiberry_settings_get_range(handle, id, &low, &high), low <= high else { return nil }
		return low...high
	}

	/// The choices that can be selected now, including the host's devices
	/// for options that list them.
	public func choices(for id: String) -> [SettingChoice] {
		guard let handle else { return [] }
		var list: UnsafePointer<amiberry_setting_choice>?
		let count = amiberry_settings_choices(handle, id, &list)
		guard let list else { return [] }
		return (0..<count).map { SettingChoice(value: layerString(list[$0].value), label: layerString(list[$0].label)) }
	}

	/// Re-reads the host's device lists, for example after a device was connected.
	public static func refreshHostChoices() {
		amiberry_settings_refresh_choices()
	}

	/// The value the running emulation uses now.
	public static func activeValue(of id: String) throws -> String {
		let (text, status) = readLayerString { amiberry_settings_get_active(id, $0, $1) }
		try SettingsError.check(status, id: id)
		return text
	}

	/// Every option the emulator's checks would change if the session were
	/// committed now, including changes made elsewhere since it was opened.
	public func validate() -> [SettingAdjustment] {
		guard let handle else { return [] }
		// Each call runs the core's checks, so ask once with room for the
		// usual few and again only when there were more.
		var capacity = 32
		var entries: [amiberry_settings_adjustment] = []
		var found = 0
		repeat {
			entries = [amiberry_settings_adjustment](repeating: amiberry_settings_adjustment(), count: capacity)
			found = entries.withUnsafeMutableBufferPointer {
				amiberry_settings_validate(handle, $0.baseAddress, $0.count)
			}
			if found <= capacity {
				break
			}
			capacity = found
		} while true
		return entries.prefix(found).map {
			SettingAdjustment(
				optionID: layerString($0.id), requested: tupleString($0.requested),
				adjusted: tupleString($0.adjusted), reason: optionalLayerString($0.reason))
		}
	}

	/// Stores the session in the pending configuration as `validate()`
	/// resolved it. The session continues from the stored configuration.
	@discardableResult
	public func commit() throws -> CommitResult {
		let session = try session("commit")
		var changed = false
		let apply = amiberry_settings_commit(session, &changed)
		return CommitResult(changed: changed, applyClass: ApplyClass(apply))
	}

	/// Applies a machine preset the way the Quickstart panel does; commit
	/// afterwards to store it. Without a compatibility level, the preset's
	/// default is used: level 1, or 0 for presets that have only that.
	public func applyPreset(_ id: String, configuration: Int = 0, compatibility: Int? = nil) throws {
		let session = try session(id)
		let level = compatibility ?? MachinePresets.defaultCompatibility(for: id)
		let status = amiberry_settings_preset_apply(
			session, id, Int32(clamping: configuration), Int32(clamping: level))
		try SettingsError.check(status, id: id, value: "\(configuration):\(level)")
	}

	// MARK: Mounted drives

	/// The mounted drives in configuration order, each spelled as the value
	/// of its uaehfN line ("dir,rw,DH0:Work:/home/amiga/work,0").
	public func mounts() throws -> [String] {
		let session = try session("mounts")
		return try (0..<amiberry_settings_mount_count(session)).map { index in
			let (text, status) = readLayerString(capacity: 1024) {
				amiberry_settings_mount_get(session, index, $0, $1)
			}
			try SettingsError.check(status, id: "mount \(index)")
			return text
		}
	}

	/// A field of an expansion board unit ("enabled", "rom_file",
	/// "setting:<name>"; see `BoardType`).
	public func boardValue(_ field: String, type: String, unit: Int) throws -> String {
		let session = try session(type)
		let (text, status) = readLayerString(capacity: 1024) {
			amiberry_settings_board_get(session, type, Int32(clamping: unit), field, $0, $1)
		}
		try SettingsError.check(status, id: "\(type) \(unit) \(field)")
		return text
	}

	/// Sets a field of an expansion board unit; enabling a built-in
	/// controller sets the chipset option that has it.
	public func setBoardValue(_ value: String, field: String, type: String, unit: Int) throws {
		let session = try session(type)
		try SettingsError.check(
			amiberry_settings_board_set(session, type, Int32(clamping: unit), field, value),
			id: "\(type) \(unit) \(field)", value: value)
	}

	public func isBoardFieldAvailable(_ field: String, type: String, unit: Int) -> Bool {
		guard let handle else { return false }
		return amiberry_settings_board_field_available(handle, type, Int32(clamping: unit), field)
	}

	/// The event a custom control of the joystick on a port triggers, alone
	/// or with the hotkey button held: an event from
	/// `CustomControlCatalog.events`, "none", or "" for the port mode's.
	/// Throws `unavailable` while the port has no joystick.
	public func customControl(_ control: String, port: Int, hotkey: Bool = false) throws -> String {
		let session = try session(control)
		let (text, status) = readLayerString(capacity: Int(AMIBERRY_SETTINGS_VALUE_MAX)) {
			amiberry_settings_custom_control_get(session, Int32(clamping: port), hotkey, control, $0, $1)
		}
		try SettingsError.check(status, id: "port \(port) \(control)")
		return text
	}

	/// Stages a custom control's event; commit applies it live.
	public func setCustomControl(_ event: String, control: String, port: Int, hotkey: Bool = false) throws {
		let session = try session(control)
		try SettingsError.check(
			amiberry_settings_custom_control_set(session, Int32(clamping: port), hotkey, control, event),
			id: "port \(port) \(control)", value: event)
	}

	/// Whether the port's joystick has the control and no hotkey function
	/// holds it.
	public func isCustomControlAvailable(_ control: String, port: Int, hotkey: Bool = false) -> Bool {
		guard let handle else { return false }
		return amiberry_settings_custom_control_available(handle, Int32(clamping: port), hotkey, control)
	}

	/// The hotkey function holding the control ("hotkey", "quit", "menu",
	/// "reset", "vkbd"), or nil.
	public func customControlUse(_ control: String, port: Int, hotkey: Bool = false) -> String? {
		guard let handle else { return nil }
		return optionalLayerString(amiberry_settings_custom_control_use(handle, Int32(clamping: port), hotkey, control))
	}

	public func addMount(_ entry: String) throws {
		let session = try session("mount")
		try SettingsError.check(amiberry_settings_mount_add(session, entry), id: "mount", value: entry)
	}

	public func replaceMount(at index: Int, with entry: String) throws {
		let session = try session("mount \(index)")
		guard index >= 0 else { throw SettingsError.unknownID("mount \(index)") }
		try SettingsError.check(
			amiberry_settings_mount_replace(session, index, entry), id: "mount \(index)", value: entry)
	}

	public func removeMount(at index: Int) throws {
		let session = try session("mount \(index)")
		guard index >= 0 else { throw SettingsError.unknownID("mount \(index)") }
		try SettingsError.check(amiberry_settings_mount_remove(session, index), id: "mount \(index)")
	}
}
