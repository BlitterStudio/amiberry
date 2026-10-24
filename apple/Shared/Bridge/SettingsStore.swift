/*
 * Copyright (C) 2026 BlitterStudio
 * The settings a SwiftUI view edits: an observable mirror of one edit
 * session. Every change goes through the session (and so through the
 * layer's rules), after which the store re-reads what the change touched.
 */

import AmiberrySettings
import Observation

@MainActor
@Observable
public final class SettingsStore {
	/// Every option's current value in the session, by option ID.
	public private(set) var values: [String: String] = [:]
	/// The mounted drives, each spelled as its uaehfN value.
	public private(set) var mounts: [String] = []
	/// What the emulator's checks would change on commit.
	public private(set) var adjustments: [SettingAdjustment] = []
	/// Whether the session's values or mounts differ from the pending
	/// configuration it was opened on (or last committed to), or a preset
	/// was applied (presets also set fields that have no option).
	public private(set) var hasChanges = false
	/// The last change the layer refused.
	public private(set) var lastError: SettingsError?
	/// What committed changes still need to reach the running emulation
	/// (`apply()`); nil when every commit was applied.
	public private(set) var pendingApply: ApplyClass?
	/// Increased by every change, so views that ask availability,
	/// ranges or choices update with it.
	public private(set) var revision = 0

	@ObservationIgnored private var session: SettingsSession
	@ObservationIgnored private var baselineValues: [String: String] = [:]
	@ObservationIgnored private var baselineMounts: [String] = []
	@ObservationIgnored private var presetApplied = false
	/// Expansion board units or custom controls were edited: they are not
	/// options, so the values do not show it.
	@ObservationIgnored private var otherEdits = false
	/// Counts the commits that changed the pending configuration, so an
	/// apply only clears `pendingApply` when nothing was committed after it.
	@ObservationIgnored private var commitGeneration = 0

	/// A store over a new session on the pending configuration.
	public init() throws {
		session = try SettingsSession()
		reload()
		markClean()
	}

	public func value(of id: String) -> String? {
		values[id]
	}

	public func isAvailable(_ id: String) -> Bool {
		_ = revision
		return session.isAvailable(id)
	}

	public func isChoiceAvailable(_ value: String, for id: String) -> Bool {
		_ = revision
		return session.isChoiceAvailable(value, for: id)
	}

	public func range(for id: String) -> ClosedRange<Double>? {
		_ = revision
		return session.range(for: id)
	}

	public func choices(for id: String) -> [SettingChoice] {
		_ = revision
		return session.choices(for: id)
	}

	/// Sets an option; false (with `lastError`) when the layer refused it.
	@discardableResult
	public func set(_ value: String, for id: String) -> Bool {
		perform { try session.setValue(value, for: id) }
	}

	/// Applies a machine preset to the session.
	@discardableResult
	public func applyPreset(_ id: String, configuration: Int = 0, compatibility: Int? = nil) -> Bool {
		perform {
			try session.applyPreset(id, configuration: configuration, compatibility: compatibility)
			presetApplied = true
		}
	}

	@discardableResult
	public func addMount(_ entry: String) -> Bool {
		perform { try session.addMount(entry) }
	}

	@discardableResult
	public func replaceMount(at index: Int, with entry: String) -> Bool {
		perform { try session.replaceMount(at: index, with: entry) }
	}

	@discardableResult
	public func removeMount(at index: Int) -> Bool {
		perform { try session.removeMount(at: index) }
	}

	// MARK: Expansion boards (see `BoardType`)

	/// A field of a board unit; nil for a field or unit the type does not have.
	public func boardValue(_ field: String, type: String, unit: Int) -> String? {
		_ = revision
		return try? session.boardValue(field, type: type, unit: unit)
	}

	/// Sets a field of a board unit; enabling a built-in controller sets the
	/// chipset option that has it, which the store re-reads.
	@discardableResult
	public func setBoardValue(_ value: String, field: String, type: String, unit: Int) -> Bool {
		perform {
			try session.setBoardValue(value, field: field, type: type, unit: unit)
			otherEdits = true
		}
	}

	public func isBoardFieldAvailable(_ field: String, type: String, unit: Int) -> Bool {
		_ = revision
		return session.isBoardFieldAvailable(field, type: type, unit: unit)
	}

	// MARK: Custom controls

	/// The event a control of the joystick on a port triggers ("" for what the
	/// port's mode gives it, "none" for nothing); nil while the port has no
	/// joystick.
	public func customControl(_ control: String, port: Int, hotkey: Bool) -> String? {
		_ = revision
		return try? session.customControl(control, port: port, hotkey: hotkey)
	}

	@discardableResult
	public func setCustomControl(_ event: String, control: String, port: Int, hotkey: Bool) -> Bool {
		perform {
			try session.setCustomControl(event, control: control, port: port, hotkey: hotkey)
			otherEdits = true
		}
	}

	public func isCustomControlAvailable(_ control: String, port: Int, hotkey: Bool) -> Bool {
		_ = revision
		return session.isCustomControlAvailable(control, port: port, hotkey: hotkey)
	}

	/// The hotkey function holding a control, or nil.
	public func customControlUse(_ control: String, port: Int, hotkey: Bool) -> String? {
		_ = revision
		return session.customControlUse(control, port: port, hotkey: hotkey)
	}

	/// Stores the session in the pending configuration. What it changed
	/// waits in `pendingApply` until `apply()` brings the emulation up to it.
	@discardableResult
	public func commit() throws -> CommitResult {
		let result = try session.commit()
		if result.changed {
			pendingApply = max(pendingApply ?? result.applyClass, result.applyClass)
			commitGeneration += 1
		}
		reload()
		markClean()
		return result
	}

	/// Commits, then brings the running emulation up to every committed
	/// change with the strongest apply class among them, and returns that
	/// class (nil when nothing needed applying). After a failure or a
	/// cancellation the changes stay pending, and the next call applies
	/// them; so do changes committed while this call waited.
	@discardableResult
	public func apply() async throws -> ApplyClass? {
		try commit()
		guard let apply = pendingApply else { return nil }
		let generation = commitGeneration
		try await EmulatorActions.perform(.applySettings(apply))
		if commitGeneration == generation {
			pendingApply = nil
		}
		return apply
	}

	/// Drops the session's changes and starts over from the pending
	/// configuration.
	public func revert() throws {
		let fresh = try SettingsSession()
		session.close()
		session = fresh
		lastError = nil
		reload()
		markClean()
	}

	/// Re-reads every value, the mounts and the adjustments from the session.
	public func reload() {
		var read: [String: String] = [:]
		for descriptor in SettingsCatalog.descriptors {
			if let value = try? session.value(of: descriptor.id) {
				read[descriptor.id] = value
			}
		}
		if read != values {
			values = read
		}
		let currentMounts = (try? session.mounts()) ?? []
		if currentMounts != mounts {
			mounts = currentMounts
		}
		let currentAdjustments = session.validate()
		if currentAdjustments != adjustments {
			adjustments = currentAdjustments
		}
		let changed = presetApplied || otherEdits || values != baselineValues || mounts != baselineMounts
		if changed != hasChanges {
			hasChanges = changed
		}
		revision += 1
	}

	private func markClean() {
		baselineValues = values
		baselineMounts = mounts
		presetApplied = false
		otherEdits = false
		hasChanges = false
	}

	private func perform(_ change: () throws -> Void) -> Bool {
		do {
			try change()
			lastError = nil
			reload()
			return true
		} catch let error as SettingsError {
			lastError = error
			return false
		} catch {
			lastError = .unavailable("\(error)")
			return false
		}
	}
}
