/*
 * Copyright (C) 2026 BlitterStudio
 * The settings layer's static data (src/osdep/settings/amiberry_settings.h)
 * as Swift values: option descriptors, machine presets and the errors the
 * layer reports. Descriptors and presets are static, so these types can be
 * used from any thread.
 */

import AmiberrySettings

/// When a committed change reaches the running emulation.
public enum ApplyClass: Int, Comparable, Sendable, CaseIterable {
	/// Picked up by the running emulation.
	case live
	/// Takes effect at the next hard reset.
	case reset
	/// Takes effect the next time emulation starts.
	case restart

	init(_ value: amiberry_apply_class) {
		switch value {
		case AMIBERRY_APPLY_RESET: self = .reset
		case AMIBERRY_APPLY_RESTART: self = .restart
		default: self = .live
		}
	}

	var layerValue: amiberry_apply_class {
		switch self {
		case .live: return AMIBERRY_APPLY_LIVE
		case .reset: return AMIBERRY_APPLY_RESET
		case .restart: return AMIBERRY_APPLY_RESTART
		}
	}

	public static func < (lhs: ApplyClass, rhs: ApplyClass) -> Bool {
		lhs.rawValue < rhs.rawValue
	}
}

/// One value of a choice option: `value` in .uae spelling, `label` to show.
public struct SettingChoice: Hashable, Sendable, Identifiable {
	public let value: String
	public let label: String
	public var id: String { value }

	public init(value: String, label: String) {
		self.value = value
		self.label = label
	}
}

/// An option of the settings layer. `id` is its .uae configuration key and
/// every value uses the .uae spelling.
public struct SettingDescriptor: Hashable, Sendable, Identifiable {
	public enum Kind: Hashable, Sendable {
		/// "true" or "false".
		case bool
		/// A decimal integer within `range`.
		case integer
		/// A decimal number within `range`.
		case real
		/// One of `choices`, or of the host's devices (see `SettingsSession.choices(for:)`).
		case choice
		/// A file system path a front end can attach a file picker to.
		case path
		/// Any other literal value, for example a hexadecimal address.
		case string
	}

	public let id: String
	/// A stable group ID, for example "cpu" or "jit".
	public let group: String
	public let label: String
	public let help: String
	public let kind: Kind
	public let applyClass: ApplyClass
	/// The widest range of an integer or real option; the range it accepts
	/// now can be narrower (`SettingsSession.range(for:)`).
	public let range: ClosedRange<Double>?
	public let step: Double?
	public let defaultValue: String
	/// The descriptor's own choices; empty for options that list the host's
	/// devices.
	public let choices: [SettingChoice]
	/// Why the emulator may change a value the user picked.
	public let constraint: String?

	init(_ d: amiberry_setting_descriptor) {
		id = layerString(d.id)
		group = layerString(d.group)
		label = layerString(d.label)
		help = layerString(d.help)
		switch d.type {
		case AMIBERRY_SETTING_BOOL: kind = .bool
		case AMIBERRY_SETTING_INT: kind = .integer
		case AMIBERRY_SETTING_REAL: kind = .real
		case AMIBERRY_SETTING_CHOICE: kind = .choice
		case AMIBERRY_SETTING_PATH: kind = .path
		default: kind = .string
		}
		applyClass = ApplyClass(d.apply)
		if kind == .integer || kind == .real {
			range = d.min <= d.max ? d.min...d.max : nil
			step = d.step
		} else {
			range = nil
			step = nil
		}
		defaultValue = layerString(d.default_value)
		var choices: [SettingChoice] = []
		if let list = d.choices {
			for i in 0..<d.choice_count {
				choices.append(SettingChoice(value: layerString(list[i].value), label: layerString(list[i].label)))
			}
		}
		self.choices = choices
		constraint = optionalLayerString(d.constraint)
	}
}

/// Every option of the settings layer, in the layer's order.
public enum SettingsCatalog {
	/// The version of amiberry_settings.h this module was built against.
	public static let apiVersion = Int(AMIBERRY_SETTINGS_API_VERSION)

	public static let descriptors: [SettingDescriptor] = {
		(0..<amiberry_settings_count()).compactMap { index in
			amiberry_settings_at(index).map { SettingDescriptor($0.pointee) }
		}
	}()

	private static let byID: [String: SettingDescriptor] = Dictionary(
		descriptors.map { ($0.id, $0) }, uniquingKeysWith: { first, _ in first })

	public static func descriptor(for id: String) -> SettingDescriptor? {
		byID[id]
	}

	/// The group IDs in the order their first option appears.
	public static let groups: [String] = {
		var seen = Set<String>()
		return descriptors.compactMap { seen.insert($0.group).inserted ? $0.group : nil }
	}()

	public static func descriptors(in group: String) -> [SettingDescriptor] {
		descriptors.filter { $0.group == group }
	}
}

/// A machine preset (a Quickstart model) and its configurations.
public struct MachinePreset: Hashable, Sendable, Identifiable {
	/// A stable ID, for example "a500".
	public let id: String
	public let label: String
	/// The configurations' labels; a configuration is its index.
	public let configurations: [String]
	/// The highest compatibility level (an index into `MachinePresets.compatibilityLevels`).
	public let compatibilityMax: Int
}

public enum MachinePresets {
	public static let all: [MachinePreset] = {
		(0..<amiberry_settings_preset_count()).compactMap { index in
			guard let preset = amiberry_settings_preset_at(index) else { return nil }
			let configurations = (0..<Int(preset.pointee.config_count)).map {
				layerString(amiberry_settings_preset_config_label(preset, Int32($0)))
			}
			return MachinePreset(
				id: layerString(preset.pointee.id), label: layerString(preset.pointee.label),
				configurations: configurations, compatibilityMax: Int(preset.pointee.compatibility_max))
		}
	}()

	/// The compatibility levels' labels, from the most compatible (0).
	public static let compatibilityLevels: [String] = {
		var levels: [String] = []
		while let label = amiberry_settings_preset_compatibility_label(Int32(levels.count)) {
			levels.append(String(cString: label))
		}
		return levels
	}()

	public static func preset(for id: String) -> MachinePreset? {
		all.first { $0.id == id }
	}

	/// The compatibility level a preset is applied with by default, as the
	/// Quickstart panel and --apply-preset do: 1, or the preset's highest
	/// when that is lower.
	public static func defaultCompatibility(for id: String) -> Int {
		min(1, preset(for: id)?.compatibilityMax ?? 0)
	}

	/// Whether every ROM the configuration needs is in the emulator's ROM
	/// list; never shows a dialog or changes anything.
	@MainActor
	public static func isInstalled(_ preset: MachinePreset, configuration: Int) -> Bool {
		amiberry_settings_preset_rom_installed(preset.id, Int32(clamping: configuration))
	}
}

/// What the settings layer refused.
public enum SettingsError: Error, Hashable, Sendable {
	/// No option, preset or entry has this ID or index.
	case unknownID(String)
	/// The value cannot be parsed, is out of range or is not a choice.
	case invalidValue(id: String, value: String)
	/// The option, this value or this request is not available now.
	case unavailable(String)
	/// The value does not fit any buffer the bridge offers.
	case valueTooLong(String)
	/// The layer could not allocate memory.
	case outOfMemory

	/// nil for AMIBERRY_SETTINGS_OK.
	init?(_ status: amiberry_settings_status, id: String, value: String = "") {
		switch status {
		case AMIBERRY_SETTINGS_OK: return nil
		case AMIBERRY_SETTINGS_UNKNOWN_ID: self = .unknownID(id)
		case AMIBERRY_SETTINGS_INVALID_VALUE: self = .invalidValue(id: id, value: value)
		case AMIBERRY_SETTINGS_UNAVAILABLE: self = .unavailable(id)
		default: self = .valueTooLong(id)
		}
	}

	static func check(_ status: amiberry_settings_status, id: String, value: String = "") throws {
		if let error = SettingsError(status, id: id, value: value) {
			throw error
		}
	}
}
