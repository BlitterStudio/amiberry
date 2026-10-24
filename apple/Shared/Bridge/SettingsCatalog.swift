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

/// An expansion board type: SCSI, IDE and network controllers, bridgeboards,
/// sound and graphics cards and the machines' built-in controllers. A unit
/// of it has `fields`: "enabled" first, then as the type has them
/// "subtype", "rom_file", "scsi_id", "autoboot_disabled", "inserted",
/// "dma24bit" and the board's own "setting:<name>" fields.
public struct BoardType: Hashable, Sendable, Identifiable {
	public struct Categories: OptionSet, Hashable, Sendable {
		public let rawValue: UInt32
		public init(rawValue: UInt32) { self.rawValue = rawValue }
		public static let builtIn = Categories(rawValue: AMIBERRY_BOARD_INTERNAL)
		public static let scsi = Categories(rawValue: AMIBERRY_BOARD_SCSI)
		public static let ide = Categories(rawValue: AMIBERRY_BOARD_IDE)
		public static let sasi = Categories(rawValue: AMIBERRY_BOARD_SASI)
		public static let custom = Categories(rawValue: AMIBERRY_BOARD_CUSTOM)
		public static let pciBridge = Categories(rawValue: AMIBERRY_BOARD_PCI_BRIDGE)
		public static let x86Bridge = Categories(rawValue: AMIBERRY_BOARD_X86_BRIDGE)
		public static let rtg = Categories(rawValue: AMIBERRY_BOARD_RTG)
		public static let sound = Categories(rawValue: AMIBERRY_BOARD_SOUND)
		public static let network = Categories(rawValue: AMIBERRY_BOARD_NETWORK)
		public static let floppy = Categories(rawValue: AMIBERRY_BOARD_FLOPPY)
		public static let x86Expansion = Categories(rawValue: AMIBERRY_BOARD_X86_EXPANSION)
	}

	/// The configuration name, for example "a2091".
	public let id: String
	public let label: String
	public let manufacturer: String?
	public let categories: Categories
	/// How many of it a configuration can have; units are numbered from 0.
	public let units: Int
	public let fields: [SettingDescriptor]

	init(_ pointer: UnsafePointer<amiberry_settings_board_type>) {
		let t = pointer.pointee
		id = layerString(t.id)
		label = layerString(t.label)
		manufacturer = optionalLayerString(t.manufacturer)
		categories = Categories(rawValue: UInt32(t.categories))
		units = Int(t.units)
		fields = (0..<t.field_count).compactMap { index in
			amiberry_settings_board_field_at(pointer, index).map { SettingDescriptor($0.pointee) }
		}
	}
}

/// Every expansion board type, in the layer's order.
public enum BoardCatalog {
	public static let types: [BoardType] = {
		(0..<amiberry_settings_board_type_count()).compactMap { index in
			amiberry_settings_board_type_at(index).map { BoardType($0) }
		}
	}()

	public static func type(_ id: String) -> BoardType? {
		types.first { $0.id == id }
	}
}

/// A gamepad button or axis of the joystick on a port, whose Amiga input
/// event a configuration sets (custom controls).
public struct CustomControl: Hashable, Sendable, Identifiable {
	/// The configuration name, for example "a", "dpup" or "leftx".
	public let id: String
	public let label: String
	public let isAxis: Bool
}

/// The custom controls and the input events they can trigger.
/// A Global Settings entry (amiberry.conf): process-wide, saved explicitly
/// with `HostSettings.save()`.
public struct HostSetting: Hashable, Sendable, Identifiable {
	public let id: String
	public let group: String
	public let label: String
	public let kind: SettingDescriptor.Kind
	public let defaultValue: String
	/// The whole descriptor: help, choices and range.
	public let descriptor: SettingDescriptor
}

/// The host settings, in the layer's order.
public enum HostSettings {
	public static let all: [HostSetting] = {
		(0..<amiberry_host_settings_count()).compactMap { index in
			amiberry_host_settings_at(index).map {
				let d = $0.pointee
				let descriptor = SettingDescriptor(d)
				return HostSetting(id: descriptor.id, group: descriptor.group,
					label: descriptor.label, kind: descriptor.kind,
					defaultValue: descriptor.defaultValue, descriptor: descriptor)
			}
		}
	}()

	@MainActor
	public static func value(of id: String) throws -> String {
		let (text, status) = readLayerString(capacity: 4096) {
			amiberry_host_settings_get(id, $0, $1)
		}
		try SettingsError.check(status, id: id)
		return text
	}

	@MainActor
	public static func set(_ value: String, for id: String) throws {
		try SettingsError.check(amiberry_host_settings_set(id, value), id: id, value: value)
	}

	/// Writes amiberry.conf. False when the file could not be written.
	@MainActor
	public static func save() -> Bool {
		amiberry_host_settings_save()
	}
}

public enum CustomControlCatalog {
	public static let controls: [CustomControl] = {
		(0..<amiberry_settings_custom_control_count()).compactMap { index in
			amiberry_settings_custom_control_at(index).map {
				CustomControl(id: layerString($0.pointee.id), label: layerString($0.pointee.label),
					isAxis: $0.pointee.axis)
			}
		}
	}()

	/// The events by the names configurations use ("Space",
	/// "Joy2 Fire/Mouse2 Left Button"); a control can also be "none" (does
	/// nothing) or "" (does what the port's mode gives it).
	public static let events: [String] = {
		(0..<amiberry_settings_input_event_count()).compactMap { index in
			amiberry_settings_input_event_at(index).map { layerString($0.pointee.value) }
		}
	}()
}
